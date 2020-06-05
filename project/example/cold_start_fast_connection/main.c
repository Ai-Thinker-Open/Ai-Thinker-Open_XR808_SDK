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
#include "net/wlan/wlan.h"
#include "net/wlan/wlan_defs.h"
#include "common/framework/net_ctrl.h"
#include "common/framework/platform_init.h"
#include "common/framework/sysinfo.h"
#include "common/cmd/cmd.h"
#include "lwip/inet.h"
#include "sys/fdcm.h"
#include "sys/xr_debug.h"
#include "util/time_logger.h"

extern uint64_t HAL_RTC_GetFreeRunTime(void);
extern void stdout_enable(uint8_t en);
extern void dns_setserver(u8_t numdns, ip_addr_t *dnsserver);

#define FC_DEBUG_EN		0
#if FC_DEBUG_EN
#define FC_DEBUG(fmt, arg...)			printf(fmt, ##arg)
#else
#define FC_DEBUG(fmt, arg...)
#endif

uint8_t g_ap_connected = 0;
void net_msg_receiver(uint32_t event, uint32_t data, void *arg)
{
	uint16_t type = EVENT_SUBTYPE(event);

	switch (type) {
	case NET_CTRL_MSG_WLAN_CONNECTED:
		break;
	case NET_CTRL_MSG_WLAN_DISCONNECTED:
		g_ap_connected = 0;
		break;
	case NET_CTRL_MSG_WLAN_SCAN_SUCCESS:
	case NET_CTRL_MSG_WLAN_SCAN_FAILED:
		break;
	case NET_CTRL_MSG_WLAN_4WAY_HANDSHAKE_FAILED:
	case NET_CTRL_MSG_WLAN_CONNECT_FAILED:
	case NET_CTRL_MSG_CONNECTION_LOSS:
		g_ap_connected = 0;
		break;
	case NET_CTRL_MSG_NETWORK_UP:
		g_ap_connected = 1;
		break;
	case NET_CTRL_MSG_NETWORK_DOWN:
		g_ap_connected = 0;
		break;
	default:
		printf("unknown msg (%u, %u)\n", type, data);
		break;
	}
}

int net_msg_rcv_init(void)
{
	observer_base *ob = sys_callback_observer_create(CTRL_MSG_TYPE_NETWORK,
	                                                 NET_CTRL_MSG_ALL,
	                                                 net_msg_receiver,
	                                                 NULL);
	if (ob == NULL)
		return -1;
	if (sys_ctrl_attach(ob) != 0)
		return -1;

	return 0;
}


#define BSS_FLASH_NUM		(0)
#define BSS_FLASH_ADDR		((1024 + 128) * 1024)
#define BSS_FLASH_SIZE		(8*1024)
typedef struct bss_info {
	uint8_t		ssid[32];
	uint8_t		psk[32];
	uint32_t	bss_size;
	uint8_t		bss[800];
} bss_info_t;

bss_info_t g_bss_info;
char sta_ssid[100];
char sta_psk[100];
void connect_ap(void)
{
    printf("Wait for link up...\n");
	printf("use this cmd to connect your ap with your own ssid and password:\n"
			"\t\"net fc config your_ssid your_password\"\n\t\"net fc enable\"\n");
    while (!g_ap_connected) {
        OS_MSleep(10);
    }
	save_time((uint32_t)HAL_RTC_GetFreeRunTime(), 2);
    printf("Connect AP success!\n");
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		printf("sysinfo %p\n", sysinfo);
		return;
	}

    //Save IP info to flash
	sysinfo->sta_use_dhcp = 0;
	memcpy(&sysinfo->netif_sta_param.ip_addr, &g_wlan_netif->ip_addr, sizeof(ip_addr_t));
	memcpy(&sysinfo->netif_sta_param.gateway, &g_wlan_netif->gw, sizeof(ip_addr_t));
	memcpy(&sysinfo->netif_sta_param.net_mask, &g_wlan_netif->netmask, sizeof(ip_addr_t));
	sysinfo_save();
}

int save_bss_to_flash(bss_info_t * pbss_info)
{
	int ret = 0;
	uint32_t size;
	wlan_sta_bss_info_t bss_get;
	fdcm_handle_t * bss_fdcm_hdl;

	if (!g_ap_connected) {
		printf("Please connect AP first!\n");
		ret = -1;
		return ret;
	}
    //Try to get current bss info size
	ret = wlan_sta_get_bss_size(&size);
	if (ret != 0) {
    	printf("Get current bss info size failed!\n");
		return ret;
	}
	bss_get.size = size;
	bss_get.bss = malloc(size);

    //Try to get current bss info
	ret = wlan_sta_get_bss(&bss_get);
	if (ret != 0) {
    	printf("Get current bss info failed!\n");
		return ret;
	}

    //Gererate 32 bytes HEX psk, so that we don't need to calcute next time
	wlan_gen_psk_param_t param;
	if (strlen(sta_psk) == 64) {
		hex2bin(param.psk, sta_psk, 32);
	} else {
		param.ssid_len = strlen(sta_ssid);
		memcpy(param.ssid, sta_ssid, param.ssid_len);
		strlcpy(param.passphrase, sta_psk, sizeof(param.passphrase));
		ret = wlan_sta_gen_psk(&param);
		if (ret != 0) {
			printf("fail to generate psk\n");
			ret = -1;
			return ret;
		}
	}

    //Save current bss info to flash
	memset(pbss_info, 0, sizeof(bss_info_t));
	memcpy(pbss_info->ssid, sta_ssid, strlen(sta_ssid));
	memcpy(pbss_info->psk, param.psk, 32);
	pbss_info->bss_size = size;
	memcpy(pbss_info->bss, bss_get.bss, size);
	bss_fdcm_hdl = fdcm_open(BSS_FLASH_NUM, BSS_FLASH_ADDR, BSS_FLASH_SIZE);
	if (bss_fdcm_hdl == NULL) {
		printf("fdcm open failed, hdl %p\n", bss_fdcm_hdl);
		ret = -1;
		return ret;
	}
	fdcm_write(bss_fdcm_hdl, pbss_info, sizeof(bss_info_t));
	fdcm_close(bss_fdcm_hdl);
	free(bss_get.bss);
    printf("Save bss info done!\n");
	return ret;
}

int clear_bss_in_flash(void)
{
	int ret = 0;
	fdcm_handle_t * bss_fdcm_hdl;

    printf("Clear bss info in flash!\n");
	memset(&g_bss_info, 0, sizeof(bss_info_t));
	bss_fdcm_hdl = fdcm_open(BSS_FLASH_NUM, BSS_FLASH_ADDR, BSS_FLASH_SIZE);
	if (bss_fdcm_hdl == NULL) {
		printf("fdcm open failed, hdl %p\n", bss_fdcm_hdl);
		ret = -1;
		return ret;
	}
	fdcm_write(bss_fdcm_hdl, &g_bss_info, sizeof(bss_info_t));
	fdcm_close(bss_fdcm_hdl);

	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		printf("sysinfo %p\n", sysinfo);
		ret = -1;
		return ret;
	}

    printf("Clear IP info inflash!\n");
	sysinfo->sta_use_dhcp = 1;
	sysinfo_save();
	return ret;
}

void connect_ap_normal(void)
{
	connect_ap();

	//Save new bss info to flash
	save_bss_to_flash(&g_bss_info);
}


void connect_ap_fast(bss_info_t * pbss_info)
{
	char psk_buf[64];
	char *p;
	int i;
	p = psk_buf;

    FC_DEBUG("Set old bss info!\n");
	for (i = 0;i < 32;i++) {
		sprintf(p, "%02x", pbss_info->psk[i]);
		p += 2;
	}
	wlan_sta_set((uint8_t *)pbss_info->ssid, strlen((char *)pbss_info->ssid), (uint8_t *)psk_buf);

    FC_DEBUG("Try to connect AP\n");
	wlan_sta_enable();

    FC_DEBUG("Wait for link up...\n");
    while (!g_ap_connected) {
        OS_MSleep(10);
    }
	save_time((uint32_t)HAL_RTC_GetFreeRunTime(), 2);
    //Fast connect AP success
}

int get_bss_from_flash(bss_info_t * pbss_info)
{
	int ret;
	uint32_t size;
	wlan_sta_bss_info_t bss_set;
	fdcm_handle_t * bss_fdcm_hdl;

	bss_fdcm_hdl = fdcm_open(BSS_FLASH_NUM, BSS_FLASH_ADDR, BSS_FLASH_SIZE);
	if (bss_fdcm_hdl == NULL) {
		printf("fdcm open failed, hdl %p\n", bss_fdcm_hdl);
		ret = -1;
		return ret;
	}
	size = fdcm_read(bss_fdcm_hdl, pbss_info, sizeof(bss_info_t));
	fdcm_close(bss_fdcm_hdl);
	if (size != sizeof(bss_info_t)) {
		printf("fdcm read failed, size %d\n", size);
		ret = -1;
		return ret;
	}
	if (pbss_info->bss_size == 0) {
		printf("empty bss info\n");
		ret = -1;
		return ret;
	}

	FC_DEBUG("SSID:%s\n", pbss_info->ssid);
	FC_DEBUG("PSK:");
	for (int i = 1; i < 33; ++i)
		FC_DEBUG("%02x", pbss_info->psk[i-1]);
	FC_DEBUG("\n");
	FC_DEBUG("BSS size:%d\n", pbss_info->bss_size);

	FC_DEBUG("Set current bss info!\n");
	bss_set.size = pbss_info->bss_size;
	bss_set.bss = malloc(bss_set.size);
	memcpy(bss_set.bss, pbss_info->bss, bss_set.size);
	ret = wlan_sta_set_bss(&bss_set);
	free(bss_set.bss);
	return ret;
}

void fast_connect_example(void)
{
    FC_DEBUG("Init wlan message receiver\n");
	net_msg_rcv_init();

	FC_DEBUG("Begin fast connect example\n");
	FC_DEBUG("Try to get old bss info in flash...\n");
	if (get_bss_from_flash(&g_bss_info)) {
#if !FC_DEBUG_EN
		stdout_enable(1);
#endif
		printf("Get old bss failed!\n");
		printf("Begin normal connection\n");
		connect_ap_normal();
    	printf("The first connection is complete, please reboot to run fast connection!\n");
	} else {
		FC_DEBUG("Get old bss info success!\n");
		FC_DEBUG("Begin fast connection\n");
		connect_ap_fast(&g_bss_info);
#if !FC_DEBUG_EN
		stdout_enable(1);
#endif
	}
}

int main(void)
{
	uint32_t time_eob, time_eop;
	time_eob = (uint32_t)HAL_RTC_GetFreeRunTime();
#if !FC_DEBUG_EN
	stdout_enable(0);
#endif
	platform_init();
	time_eop = (uint32_t)HAL_RTC_GetFreeRunTime();
	save_time(time_eob, 0);
	save_time(time_eop, 1);
	fast_connect_example();
	get_time();

	ip_addr_t dnsserver;
	ip4_addr_set_u32(&dnsserver, ipaddr_addr("180.76.76.76"));
	dns_setserver(0, &dnsserver);
	printf("Try to ping www.baidu.com\n");
	cmd_ping_exec("www.baidu.com");

	return 0;
}



