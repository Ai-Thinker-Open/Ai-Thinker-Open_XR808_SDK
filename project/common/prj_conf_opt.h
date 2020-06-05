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

#ifndef _PRJ_CONF_OPT_H_
#define _PRJ_CONF_OPT_H_

#include "prj_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * project base config
 */

/* stack size for IRQ service */
#ifndef PRJCONF_MSP_STACK_SIZE
#define PRJCONF_MSP_STACK_SIZE          (1 * 1024)
#endif

/* main thread priority */
#ifndef PRJCONF_MAIN_THREAD_PRIO
#define PRJCONF_MAIN_THREAD_PRIO        OS_THREAD_PRIO_APP
#endif

/* main thread stack size */
#ifndef PRJCONF_MAIN_THREAD_STACK_SIZE
#define PRJCONF_MAIN_THREAD_STACK_SIZE  (1 * 1024)
#endif

/* sys ctrl enable/disable */
#ifndef PRJCONF_SYS_CTRL_EN
#define PRJCONF_SYS_CTRL_EN             1
#endif

/* sys ctrl thread priority */
#ifndef PRJCONF_SYS_CTRL_PRIO
#define PRJCONF_SYS_CTRL_PRIO           OS_THREAD_PRIO_SYS_CTRL
#endif

/* sys ctrl stack size */
#ifndef PRJCONF_SYS_CTRL_STACK_SIZE
#define PRJCONF_SYS_CTRL_STACK_SIZE     (2 * 1024)
#endif

/* sys ctrl queue length for receiving message */
#ifndef PRJCONF_SYS_CTRL_QUEUE_LEN
#define PRJCONF_SYS_CTRL_QUEUE_LEN      6
#endif

/* image flash ID */
#ifndef PRJCONF_IMG_FLASH
#define PRJCONF_IMG_FLASH               0
#endif

/* image start address, including bootloader */
#ifndef PRJCONF_IMG_ADDR
#define PRJCONF_IMG_ADDR                0x00000000
#endif

/* image max size, including bootloader */
#ifndef PRJCONF_IMG_MAX_SIZE
#define PRJCONF_IMG_MAX_SIZE            0
#endif

#if ((__CONFIG_OTA_POLICY != 0x00) && (PRJCONF_IMG_MAX_SIZE != 0))
#error "image max size MUST be defined in image.cfg file and set PRJCONF_IMG_MAX_SIZE to 0!"
#endif

/* save sysinfo to flash or not */
#ifndef PRJCONF_SYSINFO_SAVE_TO_FLASH
#define PRJCONF_SYSINFO_SAVE_TO_FLASH	1
#endif

#if PRJCONF_SYSINFO_SAVE_TO_FLASH

/* sysinfo flash ID */
#ifndef PRJCONF_SYSINFO_FLASH
#define PRJCONF_SYSINFO_FLASH           0
#endif

/* sysinfo start address */
#ifndef PRJCONF_SYSINFO_ADDR
#define PRJCONF_SYSINFO_ADDR            ((1024 - 4) * 1024)
#endif

/* sysinfo size */
#ifndef PRJCONF_SYSINFO_SIZE
#define PRJCONF_SYSINFO_SIZE            (4 * 1024)
#endif

/* enable/disable checking whether sysinfo is overlap with image */
#ifndef PRJCONF_SYSINFO_CHECK_OVERLAP
#define PRJCONF_SYSINFO_CHECK_OVERLAP	1
#endif

#endif /* PRJCONF_SYSINFO_SAVE_TO_FLASH */

/* MAC address source */
#ifndef PRJCONF_MAC_ADDR_SOURCE
#define PRJCONF_MAC_ADDR_SOURCE         SYSINFO_MAC_ADDR_CHIPID
#endif

/* watchdog enable/disable */
#ifndef PRJCONF_WDG_EN
#define PRJCONF_WDG_EN                  0
#endif

#ifndef PRJCONF_WDG_EVENT_TYPE
#define PRJCONF_WDG_EVENT_TYPE          WDG_EVT_RESET
#endif

#ifndef PRJCONF_WDG_RESET_CPU_MODE
#define PRJCONF_WDG_RESET_CPU_MODE      WDG_RESET_CPU_CORE
#endif

/* watchdog timeout value */
#ifndef PRJCONF_WDG_TIMEOUT
#define PRJCONF_WDG_TIMEOUT             WDG_TIMEOUT_16SEC
#endif

/* watchdog feeding period (in ms), MUST less than PRJCONF_WDG_TIMEOUT */
#ifndef PRJCONF_WDG_FEED_PERIOD
#define PRJCONF_WDG_FEED_PERIOD         (10 * 1000)
#endif

/*
 * project hardware feature
 */

/* gpio port bitmask for irq used */
#ifndef PRJCONF_GPIO_PORT_IRQ_USED
#define PRJCONF_GPIO_PORT_IRQ_USED      0xff
#endif

/* gpio port bitmask for pm backup */
#ifndef PRJCONF_GPIO_PORT_PM_BACKUP
#define PRJCONF_GPIO_PORT_PM_BACKUP     0xff
#endif

/* uart enable/disable */
#ifndef PRJCONF_UART_EN
#define PRJCONF_UART_EN                 1
#endif

/* h/w crypto engine enable/disable */
#ifndef PRJCONF_CE_EN
#define PRJCONF_CE_EN                   1
#endif

/* init prng seed or not */
#ifndef PRJCONF_PRNG_INIT_SEED
#define PRJCONF_PRNG_INIT_SEED          1
#endif

/* spi enable/disable */
#ifndef PRJCONF_SPI_EN
#define PRJCONF_SPI_EN                  0
#endif

/* mmc enable/disable */
#ifndef PRJCONF_MMC_EN
#define PRJCONF_MMC_EN                  0
#endif

/* mmc detect mode */
#ifndef PRJCONF_MMC_DETECT_MODE
#define PRJCONF_MMC_DETECT_MODE         CARD_ALWAYS_PRESENT
#endif

/* Xradio internal codec sound card enable/disable */
#ifndef PRJCONF_INTERNAL_SOUNDCARD_EN
#define PRJCONF_INTERNAL_SOUNDCARD_EN   0
#endif

/* AC107 sound card enable/disable */
#ifndef PRJCONF_AC107_SOUNDCARD_EN
#define PRJCONF_AC107_SOUNDCARD_EN		0
#endif

/* AC101 sound card enable/disable */
#ifndef PRJCONF_AC101_SOUNDCARD_EN
#define PRJCONF_AC101_SOUNDCARD_EN		0
#endif

/* I2S NULL sound card enable/disable */
#ifndef PRJCONF_I2S_NULL_SOUNDCARD_EN
#define PRJCONF_I2S_NULL_SOUNDCARD_EN	0
#endif

/* audio control enable/disable */
#ifndef PRJCONF_AUDIO_CTRL_EN
#define PRJCONF_AUDIO_CTRL_EN           0
#endif

/* swd enable/disable */
#ifndef PRJCONF_SWD_EN
#define PRJCONF_SWD_EN                  0
#endif

#if PRJCONF_AC107_SOUNDCARD_EN || PRJCONF_AC101_SOUNDCARD_EN || PRJCONF_I2S_NULL_SOUNDCARD_EN
#define PRJCONF_PLATFORM_I2S_EN			1
#else
#define PRJCONF_PLATFORM_I2S_EN			0
#endif


#if PRJCONF_INTERNAL_SOUNDCARD_EN || PRJCONF_PLATFORM_I2S_EN
#define PRJCONF_AUDIO_SNDCARD_EN		1
#else
#define PRJCONF_AUDIO_SNDCARD_EN		0
#endif

/*
 * project service feature
 */

/* console enable/disable */
#ifndef PRJCONF_CONSOLE_EN
#define PRJCONF_CONSOLE_EN              1
#endif

/* console stack size */
#ifndef PRJCONF_CONSOLE_STACK_SIZE
#ifdef __CONFIG_ETF_CLI
#define PRJCONF_CONSOLE_STACK_SIZE      (16 * 1024)
#else
#define PRJCONF_CONSOLE_STACK_SIZE      (2 * 1024)
#endif
#endif

/* app pm mode enable/disable */
#ifndef PRJCONF_PM_EN
#define PRJCONF_PM_EN                   1
#endif

/* app pm mode */
#ifndef PRJCONF_PM_MODE
#define PRJCONF_PM_MODE                 (PM_SUPPORT_SLEEP       | \
                                         PM_SUPPORT_STANDBY     | \
                                         PM_SUPPORT_HIBERNATION)
#endif

/* network and wlan enable/disable */
#ifndef PRJCONF_NET_EN
#ifdef __CONFIG_WLAN
#define PRJCONF_NET_EN                  1
#else
#define PRJCONF_NET_EN                  0
#endif
#endif

/* net pm mode enable/disable */
#ifndef PRJCONF_NET_PM_EN
#ifdef __CONFIG_WLAN
#define PRJCONF_NET_PM_EN               1
#else
#define PRJCONF_NET_PM_EN               0
#endif
#endif

/* net pm mode */
#ifndef PRJCONF_NET_PM_MODE
#define PRJCONF_NET_PM_MODE             (PM_SUPPORT_SLEEP       | \
                                         PM_SUPPORT_HIBERNATION)
#endif

/* environment variable "TZ" for time zone setting */
#ifndef PRJCONF_ENV_TZ
#define PRJCONF_ENV_TZ                  "TZ=GMT-8"
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PRJ_CONF_OPT_H_ */
