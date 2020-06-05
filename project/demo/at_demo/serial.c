/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <ctype.h>
#include "kernel/os/os.h"
#include "atcmd/at_types.h"
#include <stdarg.h>

#include "serial.h"
#include "serial_debug.h"

#define SERIAL_CACHE_BUF_NUM    16
#define SERIAL_CACHE_BUF_SIZE   (64/2)

#define ATCMDSEND_MAX_BUFF_SIZE (1024L+64L)

typedef enum {
    SERIAL_STATE_STOP = 0,
    SERIAL_STATE_START,
    SERIAL_STATE_TERMINATE, /* to be terminated */
} serial_state;


#define ATCMDSEND_THREAD_STACK_SIZE (1 * 1024)
static OS_Thread_t g_atcmdSend_thread;


#define QLEN_ATCMDSEND       2
static OS_Queue_t q_atcmdSend;
static OS_Semaphore_t   sem_atSend;


typedef struct atcmdSend_queueinf_def{
    int datalen;
    uint8_t *p_senddata;
}atcmdSend_queueinf_t;


typedef struct serial_priv {
    UART_ID         uartID;
    serial_state    state;

    OS_Semaphore_t   cmd_sem;

    serial_cmd_exec_func cmd_exec;

    struct {
        volatile uint8_t cnt;
        uint8_t widx;
        uint8_t ridx;
        uint8_t len[SERIAL_CACHE_BUF_NUM];
        uint8_t buf[SERIAL_CACHE_BUF_NUM][SERIAL_CACHE_BUF_SIZE];
    } cache;
} serial_priv_t;

static serial_priv_t g_serial;
void atcmd_sendTask(void *pvParameters);


/* Note: only support line end with "\r\n" or "\n", not support "\r" */
static void serial_rx_callback(void *arg)
{
    serial_priv_t *serial;
    UART_T *uart;
    uint32_t cnt;
    uint32_t idx;
    uint32_t len;
    uint8_t data;

    uart = (UART_T *)arg;
    serial = &g_serial;

    cnt = serial->cache.cnt;

    if(cnt < SERIAL_CACHE_BUF_NUM) {
        idx = serial->cache.widx;
        len = 0;
        while (1) {
            if (HAL_UART_IsRxReady(uart)) {
                data = HAL_UART_GetRxData(uart);

                serial->cache.buf[idx][len++] = data;

                if (len >= SERIAL_CACHE_BUF_SIZE) {
                    serial->cache.len[idx] = len;
                    idx++;
                    if (idx >= SERIAL_CACHE_BUF_NUM) {
                        idx = 0;
                    }
                    cnt++;
                    serial->cache.widx = idx;
                    serial->cache.cnt = cnt;
                    OS_SemaphoreRelease(&serial->cmd_sem);

                    if (cnt >= SERIAL_CACHE_BUF_NUM) {
                        if (HAL_UART_IsRxReady(uart)) {
                            /* discard data */
                            while (HAL_UART_IsRxReady(uart)) {
                                data = HAL_UART_GetRxData(uart);
                            }

                            SERIAL_WARN("no buf for rx, discard received data\n");
                        }

                        break;
                    }
                    len = 0;
                }
            }
            else {
                if (len > 0) {
                    serial->cache.len[idx] = len;
                    idx++;
                    if (idx >= SERIAL_CACHE_BUF_NUM) {
                        idx = 0;
                    }
                    cnt++;
                    serial->cache.widx = idx;
                    serial->cache.cnt = cnt;
                    OS_SemaphoreRelease(&serial->cmd_sem);

                    break;
                }
                break;

            }
        }
    }
    else {
        /* discard data */
        while (HAL_UART_IsRxReady(uart)) {
            data = HAL_UART_GetRxData(uart);
        }
        SERIAL_WARN("no buf for rx, discard received data\n");
    }
}

int serial_config(UART_ID uart_id, int baudrate, int data_bits, int parity, int stop_bits, int hwfc)
{
    UART_InitParam uart_param;

    uart_param.baudRate = baudrate;
    uart_param.parity = UART_PARITY_NONE;
    uart_param.stopBits = UART_STOP_BITS_1;
    uart_param.dataBits = UART_DATA_BITS_8;
    uart_param.isAutoHwFlowCtrl = hwfc;

    HAL_UART_Init(uart_id, &uart_param);

    return 0;
}

int serial_init(UART_ID uart_id, int baudrate, int data_bits, int parity, int stop_bits, int hwfc)
{
    serial_priv_t *serial;

    switch (baudrate) {
        case 115200:
            baudrate = 115200;
            break;
        case 230400:
            baudrate = 230400;
            break;
        case 460800:
            baudrate = 460800;
            break;
        case 921600:
            baudrate = 921600;
            break;
        default:
            baudrate = 115200;
            break;
    }

    serial_config(uart_id, baudrate, data_bits, parity, stop_bits, hwfc);

    serial = &g_serial;
    if (serial->state != SERIAL_STATE_STOP) {
        SERIAL_ERR("serial state %d\n", serial->state);
        return -1;
    }

    memset(serial, 0, sizeof(*serial));
    serial->uartID = uart_id;

    if (OS_SemaphoreCreate(&serial->cmd_sem, 0, OS_SEMAPHORE_MAX_COUNT) != OS_OK) {
        SERIAL_ERR("create semaphore failed\n");
        return -1;
    }

    return 0;
}

int serial_deinit(UART_ID uart_id)
{
    serial_priv_t *serial;

    serial = &g_serial;

    HAL_UART_DeInit(uart_id);

    OS_SemaphoreDelete(&serial->cmd_sem);


    return 0;
}

/* NB: Make sure uart is inited before calling this function. */
int serial_start(void)
{
    UART_T *uart;
    serial_priv_t *serial;

    serial = &g_serial;

    uart = HAL_UART_GetInstance(serial->uartID);
    HAL_UART_EnableRxCallback(serial->uartID, serial_rx_callback, uart);
    serial->state = SERIAL_STATE_START;

        /* start atcmd task */
    if (OS_ThreadCreate(&g_atcmdSend_thread,
                        "atcmdSend",
                        atcmd_sendTask,
                        NULL,
                        OS_THREAD_PRIO_APP,
                        ATCMDSEND_THREAD_STACK_SIZE) != OS_OK) {
        SERIAL_DBG("create serial task failed\n");
        return -1;
    }

    return 0;
}

void serial_stop(void)
{
    serial_priv_t *serial;

    serial = &g_serial;
    HAL_UART_DisableRxCallback(serial->uartID);
    serial->state = SERIAL_STATE_STOP;
}

void serial_bezero_g_serial(void)
{
    memset(&g_serial,0,sizeof(serial_priv_t));

}
int serial_read(uint8_t *buf, int32_t size)
{
    serial_priv_t *serial;
    uint32_t cnt;
    uint32_t idx;
    int rlen = 0;

//  SERIAL_DBG("%s() start...\n", __func__);

    serial = &g_serial;

    idx = serial->cache.ridx;

    while (1) {
/*
        if (OS_SemaphoreWait(&serial->cmd_sem, OS_WAIT_FOREVER) != OS_OK)
            continue;
*/
        if (OS_SemaphoreWait(&serial->cmd_sem, 10) != OS_OK)
            break;

        if (serial->state != SERIAL_STATE_START)
            break;

        arch_irq_disable();
        cnt = serial->cache.cnt;
        arch_irq_enable();

        if (cnt > 0) {
            rlen = serial->cache.len[idx];
            if (rlen > size) {
                return -1; /* buffer size is too small */
            }

            memcpy(buf, serial->cache.buf[idx], rlen);

            idx++;
            if (idx >= SERIAL_CACHE_BUF_NUM) {
                idx = 0;
            }

            serial->cache.ridx = idx;

            arch_irq_disable();
            serial->cache.cnt--;
            arch_irq_enable();

            break;
        }
        else {
            SERIAL_WARN("no valid command\n");
            return -2; /* no data  */
        }
    }

    return rlen;
}

int serial_write(uint8_t *buf, int32_t len)
{
    serial_priv_t *serial;

    serial = &g_serial;
    return HAL_UART_Transmit_Poll(serial->uartID, buf, len);
}

void serial_disable(void)
{
    serial_priv_t *serial;

    serial = &g_serial;
    if (serial->state == SERIAL_STATE_START) {
        HAL_UART_DisableRxCallback(serial->uartID);
    }
}

void serial_enable(void)
{
    serial_priv_t *serial;
    UART_T *uart;

    serial = &g_serial;
    if (serial->state == SERIAL_STATE_START) {
        uart = HAL_UART_GetInstance(serial->uartID);
        HAL_UART_EnableRxCallback(serial->uartID, serial_rx_callback, uart);
    }
}

UART_ID serial_get_uart_id(void)
{
    serial_priv_t *serial;

    serial = &g_serial;
    if (serial->state == SERIAL_STATE_START) {
        return serial->uartID;
    } else {
        return UART_INVALID_ID;
    }
}


int at_data_output(char *buf, int size)
{
    atcmdSend_queueinf_t sendinf;

    if (OS_SemaphoreWait(&sem_atSend, OS_WAIT_FOREVER) != OS_OK)
    {
        SERIAL_DBG("wait semaphore failed \n");
        return -1;
    }

    while(1)
    {
        sendinf.p_senddata = malloc(ATCMDSEND_MAX_BUFF_SIZE);
        if(sendinf.p_senddata == NULL)
        {
            SERIAL_DBG("malloc buffer for atcmdSend failed \n");
            OS_MSleep(200);
        }else{
            break;
        }
    }

    memset(sendinf.p_senddata, 0x00, ATCMDSEND_MAX_BUFF_SIZE);
    memcpy(sendinf.p_senddata,buf, size);
    sendinf.datalen = size;
    SERIAL_DBG("data output xQueueSend!");
    OS_QueueSend(&q_atcmdSend, &sendinf, 0);
    return 0;
}

s32 at_dump(char* format, ...)
{
    int len;
    va_list vp;
    atcmdSend_queueinf_t sendinf;

    SERIAL_DBG("%s() start...\n", __func__);
    if (OS_SemaphoreWait(&sem_atSend, OS_WAIT_FOREVER) != OS_OK)
    {
        SERIAL_DBG("wait semaphore failed \n");
        return -1;
    }

    while(1)
    {
        sendinf.p_senddata = malloc(ATCMDSEND_MAX_BUFF_SIZE);
        if(sendinf.p_senddata == NULL)
        {
            SERIAL_DBG("malloc buffer for atcmdSend failed \n");
            OS_MSleep(200);
        }else{
            break;
        }
    }

    memset(sendinf.p_senddata, 0x00, ATCMDSEND_MAX_BUFF_SIZE);

    va_start(vp, format);
    len = vsnprintf((char *)sendinf.p_senddata, ATCMDSEND_MAX_BUFF_SIZE, format, vp);
    va_end(vp);
    SERIAL_DBG("%s() len = %d strlen = %d \n", __func__, len, strlen((char *)sendinf.p_senddata));

    if (strlen((char *)sendinf.p_senddata) < len) {
        SERIAL_DBG("dump info is too long!");
        free(sendinf.p_senddata);
        OS_SemaphoreRelease(&sem_atSend);
        return -1;
    }
    sendinf.datalen = len;
    SERIAL_DBG("at_dump xQueueSend!");
    OS_QueueSend(&q_atcmdSend, &sendinf, 0);
    return 0;
}


void atcmd_sendTask(void *pvParameters)
{
    atcmdSend_queueinf_t sendinf;

    OS_QueueCreate(&q_atcmdSend, QLEN_ATCMDSEND, sizeof(atcmdSend_queueinf_t));

    if (OS_SemaphoreCreate(&sem_atSend, QLEN_ATCMDSEND, QLEN_ATCMDSEND) != OS_OK) {
        SERIAL_DBG("create semaphore failed\n");
        return;
    }

    while (1) {
        if (OS_QueueReceive(&q_atcmdSend, &sendinf, OS_WAIT_FOREVER) != OS_OK)
            continue;

        SERIAL_DBG("%s() datalen = %d...\n", __func__, sendinf.datalen);

        serial_write(sendinf.p_senddata, sendinf.datalen);
        free(sendinf.p_senddata);
        OS_SemaphoreRelease(&sem_atSend);
    }
    return;
}

