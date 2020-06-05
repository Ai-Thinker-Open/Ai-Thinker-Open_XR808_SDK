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

#include "version.h"
#include "cmd_util.h"
#include "lwip/inet.h"
#include "common/framework/sysinfo.h"

/*
 * sysinfo default
 * sysinfo save
 * sysinfo load
 *
 * sysinfo get mac
 * sysinfo set mac <MAC>
 *   - sysinfo set mac 00:80:E1:29:E8:D1
 *
 * sysinfo get sta ssid
 * sysinfo set sta ssid <ssid>
 *   - sysinfo set sta ssid ssid_example
 *
 * sysinfo get sta psk
 * sysinfo set sta psk <psk>
 *   - sysinfo set sta psk psk_example
 *
 * sysinfo get ap ssid
 * sysinfo set ap ssid <ssid>
 *   - sysinfo set ap ssid ssid_example
 *
 * sysinfo get ap psk
 * sysinfo set ap psk <psk>
 *   - sysinfo set ap psk psk_example
 *
 * sysinfo get sta dhcp
 * sysinfo set sta dhcp <dhcp>
 *   - sysinfo set sta dhcp 1
 *
 * sysinfo get sta ip
 * sysinfo set sta ip <ip>
 *   - sysinfo set sta ip 192.168.51.100
 *
 * sysinfo get sta mask
 * sysinfo set sta mask <mask>
 *   - sysinfo set sta mask 255.255.255.0
 *
 * sysinfo get sta gateway
 * sysinfo set sta gateway <gateway>
 *   - sysinfo set sta gateway 192.168.51.1
 *
 * sysinfo get ap ip
 * sysinfo set ap ip <ip>
 *   - sysinfo set ap ip 192.168.51.1
 *
 * sysinfo get ap mask
 * sysinfo set ap mask <mask>
 *   - sysinfo set ap mask 255.255.255.0
 *
 * sysinfo get ap gateway
 * sysinfo set ap gateway <gateway>
 *   - sysinfo set ap gateway 192.168.51.1
 */

enum cmd_status cmd_sysinfo_default_exec(char *cmd)
{
	if (sysinfo_default() != 0)
		return CMD_STATUS_FAIL;
	else
		return CMD_STATUS_OK;
}

#if PRJCONF_SYSINFO_SAVE_TO_FLASH
enum cmd_status cmd_sysinfo_save_exec(char *cmd)
{
	if (sysinfo_save() != 0)
		return CMD_STATUS_FAIL;
	else
		return CMD_STATUS_OK;
}

enum cmd_status cmd_sysinfo_load_exec(char *cmd)
{
	if (sysinfo_load() != 0)
		return CMD_STATUS_FAIL;
	else
		return CMD_STATUS_OK;
}
#endif

#if PRJCONF_NET_EN

static int cmd_sysinfo_parse_int(const char *value, int min, int max, int *dst)
{
	int val;
	char *end;

	val = cmd_strtol(value, &end, 0);
	if (*end) {
		CMD_ERR("Invalid number '%s'", value);
		return -1;
	}

	if (val < min || val > max) {
		CMD_ERR("out of range value %d (%s), range is [%d, %d]\n",
			 val, value, min, max);
		return -1;
	}

	*dst = val;
	return 0;
}

static enum cmd_status cmd_sysinfo_set_mac(char *cmd, struct sysinfo *sysinfo)
{
#if (PRJCONF_MAC_ADDR_SOURCE == SYSINFO_MAC_ADDR_FLASH)
	int i;
	int cnt;
	uint32_t mac[SYSINFO_MAC_ADDR_LEN];

	cnt = cmd_sscanf(cmd, "%x:%x:%x:%x:%x:%x",
					 &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
	if (cnt != 6) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	for (i = 0; i < SYSINFO_MAC_ADDR_LEN; i++)
		sysinfo->mac_addr[i] = (uint8_t)mac[i];

	return CMD_STATUS_OK;
#else
	CMD_DBG("CMD not supported due to mac addr is not from flash\n");
	return CMD_STATUS_FAIL;
#endif
}

static enum cmd_status cmd_sysinfo_set_ssid(char *cmd, uint8_t *ssid, uint8_t *ssid_len)
{
	uint32_t len = cmd_strlen(cmd);

	if ((len == 0) || (len > SYSINFO_SSID_LEN_MAX)) {
		CMD_ERR("len %d\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(ssid, 0, SYSINFO_SSID_LEN_MAX);
	cmd_memcpy(ssid, cmd, len);
	*ssid_len = (uint8_t)len;

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sysinfo_set_psk(char *cmd, uint8_t *psk)
{
	uint32_t len = cmd_strlen(cmd);

	if ((len < 8) || (len >= SYSINFO_PSK_LEN_MAX)) {
		CMD_ERR("len %d\n", len);
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_memset(psk, 0, SYSINFO_PSK_LEN_MAX);
	cmd_memcpy(psk, cmd, len);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_sysinfo_set_dhcp(char *cmd, struct sysinfo *sysinfo)
{
	int dhcp;

	if (cmd_sysinfo_parse_int(cmd, 0, 1, &dhcp) != 0) {
		CMD_ERR("invalid dhcp '%s'\n", cmd);
		return CMD_STATUS_INVALID_ARG;
	}

	sysinfo->sta_use_dhcp = dhcp;

	return CMD_STATUS_OK;
}

#ifdef __CONFIG_LWIP_V1
static enum cmd_status cmd_sysinfo_set_netif(char *cmd, ip_addr_t *addr)
#elif LWIP_IPV4 /* now only for IPv4 */
static enum cmd_status cmd_sysinfo_set_netif(char *cmd, ip4_addr_t *addr)
#else
#error "IPv4 not support!"
#endif
{
	int cnt;
	uint32_t val[4];

	cnt = cmd_sscanf(cmd, "%u.%u.%u.%u", &val[0], &val[1], &val[2], &val[3]);
	if (cnt != 4) {
		CMD_ERR("cnt %d\n", cnt);
		return CMD_STATUS_INVALID_ARG;
	}

	if (((val[0] | val[1] | val[2] | val[3]) & 0xffffff00) != 0) {
		CMD_ERR("val[0] %d, val[1] %d, val[2] %d, val[3] %d\n",
				val[0], val[1], val[2], val[3]);
		return CMD_STATUS_INVALID_ARG;
	}

	IP4_ADDR(addr, val[0], val[1], val[2], val[3]);

	return CMD_STATUS_OK;
}

#endif /* PRJCONF_NET_EN */

enum cmd_status cmd_sysinfo_set_exec(char *cmd)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		CMD_ERR("sysinfo %p\n", sysinfo);
		return CMD_STATUS_FAIL;
	}

#if PRJCONF_NET_EN

	if (cmd_strncmp(cmd, "mac ", 4) == 0) {
		return cmd_sysinfo_set_mac(cmd + 4, sysinfo);
	} else if (cmd_strncmp(cmd, "sta ", 4) == 0) {
		if (cmd_strncmp(cmd + 4, "ssid ", 5) == 0) {
			return cmd_sysinfo_set_ssid(cmd + 9, sysinfo->wlan_sta_param.ssid,
										&(sysinfo->wlan_sta_param.ssid_len));
		} else if (cmd_strncmp(cmd + 4, "psk ", 4) == 0) {
			return cmd_sysinfo_set_psk(cmd + 8, sysinfo->wlan_sta_param.psk);
		} else if (cmd_strncmp(cmd + 4, "dhcp ", 5) == 0) {
			return cmd_sysinfo_set_dhcp(cmd + 9, sysinfo);
		} else if (cmd_strncmp(cmd + 4, "ip ", 3) == 0) {
			return cmd_sysinfo_set_netif(cmd + 7, &sysinfo->netif_sta_param.ip_addr);
		} else if (cmd_strncmp(cmd + 4, "mask ", 5) == 0) {
			return cmd_sysinfo_set_netif(cmd + 9, &sysinfo->netif_sta_param.net_mask);
		} else if (cmd_strncmp(cmd + 4, "gateway ", 8) == 0) {
			return cmd_sysinfo_set_netif(cmd + 12, &sysinfo->netif_sta_param.gateway);
		} else {
			CMD_ERR("invalid arg '%s'\n", cmd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (cmd_strncmp(cmd, "ap ", 3) == 0) {
		if (cmd_strncmp(cmd + 3, "ssid ", 5) == 0) {
			return cmd_sysinfo_set_ssid(cmd + 8, sysinfo->wlan_ap_param.ssid,
										&(sysinfo->wlan_ap_param.ssid_len));
		} else if (cmd_strncmp(cmd + 3, "psk ", 4) == 0) {
			return cmd_sysinfo_set_psk(cmd + 7, sysinfo->wlan_ap_param.psk);
		} else if (cmd_strncmp(cmd + 3, "ip ", 3) == 0) {
			return cmd_sysinfo_set_netif(cmd + 6, &sysinfo->netif_ap_param.ip_addr);
		} else if (cmd_strncmp(cmd + 3, "mask ", 5) == 0) {
			return cmd_sysinfo_set_netif(cmd + 8, &sysinfo->netif_ap_param.net_mask);
		} else if (cmd_strncmp(cmd + 3, "gateway ", 8) == 0) {
			return cmd_sysinfo_set_netif(cmd + 11, &sysinfo->netif_ap_param.gateway);
		} else {
			CMD_ERR("invalid arg '%s'\n", cmd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		CMD_ERR("invalid arg '%s'\n", cmd);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;

#else /* PRJCONF_NET_EN */

	CMD_ERR("invalid arg '%s'\n", cmd);
	return CMD_STATUS_INVALID_ARG;

#endif /* PRJCONF_NET_EN */
}

enum cmd_status cmd_sysinfo_get_exec(char *cmd)
{
	struct sysinfo *sysinfo = sysinfo_get();
	if (sysinfo == NULL) {
		CMD_ERR("sysinfo %p\n", sysinfo);
		return CMD_STATUS_FAIL;
	}

#if PRJCONF_NET_EN

	if (cmd_strcmp(cmd, "mac") == 0) {
		CMD_LOG(1, "MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
				sysinfo->mac_addr[0], sysinfo->mac_addr[1], sysinfo->mac_addr[2],
				sysinfo->mac_addr[3], sysinfo->mac_addr[4], sysinfo->mac_addr[5]);
	} else if (cmd_strncmp(cmd, "sta ", 4) == 0) {
		if (cmd_strcmp(cmd + 4, "ssid") == 0) {
			CMD_LOG(1, "ssid: %.32s\n", sysinfo->wlan_sta_param.ssid);
		} else if (cmd_strcmp(cmd + 4, "psk") == 0) {
			CMD_LOG(1, "psk: %.64s\n", sysinfo->wlan_sta_param.psk);
		} else if (cmd_strcmp(cmd + 4, "dhcp") == 0) {
			CMD_LOG(1, "use dhcp: %d\n", sysinfo->sta_use_dhcp);
		} else if (cmd_strcmp(cmd + 4, "ip") == 0) {
			CMD_LOG(1, "ip: %s\n", inet_ntoa(sysinfo->netif_sta_param.ip_addr));
		} else if (cmd_strcmp(cmd + 4, "mask") == 0) {
			CMD_LOG(1, "net mask: %s\n", inet_ntoa(sysinfo->netif_sta_param.net_mask));
		} else if (cmd_strcmp(cmd + 4, "gateway") == 0) {
			CMD_LOG(1, "gateway: %s\n", inet_ntoa(sysinfo->netif_sta_param.gateway));
		} else {
			CMD_ERR("invalid arg '%s'\n", cmd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else if (cmd_strncmp(cmd, "ap ", 3) == 0) {
		if (cmd_strcmp(cmd + 3, "ssid") == 0) {
			CMD_LOG(1, "ssid: %.32s\n", sysinfo->wlan_ap_param.ssid);
		} else if (cmd_strcmp(cmd + 3, "psk") == 0) {
			CMD_LOG(1, "psk: %.64s\n", sysinfo->wlan_ap_param.psk);
		} else if (cmd_strcmp(cmd + 3, "ip") == 0) {
			CMD_LOG(1, "ip: %s\n", inet_ntoa(sysinfo->netif_ap_param.ip_addr));
		} else if (cmd_strcmp(cmd + 3, "mask") == 0) {
			CMD_LOG(1, "net mask: %s\n", inet_ntoa(sysinfo->netif_ap_param.net_mask));
		} else if (cmd_strcmp(cmd + 3, "gateway") == 0) {
			CMD_LOG(1, "gateway: %s\n", inet_ntoa(sysinfo->netif_ap_param.gateway));
		} else {
			CMD_ERR("invalid arg '%s'\n", cmd);
			return CMD_STATUS_INVALID_ARG;
		}
	} else {
		CMD_ERR("invalid arg '%s'\n", cmd);
		return CMD_STATUS_INVALID_ARG;
	}

	return CMD_STATUS_OK;

#else /* PRJCONF_NET_EN */

	CMD_ERR("invalid arg '%s'\n", cmd);
	return CMD_STATUS_INVALID_ARG;

#endif /* PRJCONF_NET_EN */
}

enum cmd_status cmd_sysinfo_chip_exec(char *cmd)
{
	CMD_LOG(1, "chip arch version %d\n", __CONFIG_CHIP_ARCH_VER);
	return CMD_STATUS_OK;
}

enum cmd_status cmd_sysinfo_version_exec(char *cmd)
{
	CMD_LOG(1, "version: %s\n", SDK_VERSION_STR);

	return CMD_STATUS_OK;
}

#if CMD_DESCRIBE
#define sysinfo_default_help_info "set default value to sysinfo"
#define sysinfo_save_help_info "save sysinfo to flash"
#define sysinfo_load_help_info "load sysinfo from flash"
#define sysinfo_set_help_info "sysinfo set <field> <arg> <value>\n"\
"\t\tsysinfo set mac <MAC>, eg. sysinfo set mac 00:80:E1:29:E8:D1\n"\
"\t\tsysinfo set sta ssid <ssid>, eg. sysinfo set sta ssid ssid_example\n"\
"\t\tsysinfo set sta psk <psk>, eg. sysinfo set sta psk psk_example\n"\
"\t\tsysinfo set sta dhcp <dhcp>, eg. sysinfo set sta dhcp 1\n"\
"\t\tsysinfo set sta ip <ip>, eg. sysinfo set sta ip 192.168.51.100\n"\
"\t\tsysinfo set sta mask <mask>, eg. sysinfo set sta mask 255.255.255.0\n"\
"\t\tsysinfo set sta gateway <gateway>, eg. sysinfo set sta gateway 192.168.51.1\n"\
"\t\tsysinfo set ap ssid <ssid>, eg. sysinfo set ap ssid ssid_example\n"\
"\t\tsysinfo set ap psk <psk>, eg. sysinfo set ap psk psk_example\n"\
"\t\tsysinfo set ap ip <ip>, eg. sysinfo set ap ip 192.168.51.1\n"\
"\t\tsysinfo set ap mask <mask>, eg. sysinfo set ap mask 255.255.255.0\n"\
"\t\tsysinfo set ap gateway <gateway>, eg. sysinfo set ap gateway 192.168.51.1"
#define sysinfo_get_help_info "sysinfo get <field> <arg>\n"\
"\t\tsysinfo get mac\n"\
"\t\tsysinfo get sta ssid\n"\
"\t\tsysinfo get sta psk\n"\
"\t\tsysinfo get sta dhcp\n"\
"\t\tsysinfo get sta ip\n"\
"\t\tsysinfo get sta mask\n"\
"\t\tsysinfo get sta gateway\n"\
"\t\tsysinfo get ap ssid\n"\
"\t\tsysinfo get ap psk\n"\
"\t\tsysinfo get ap ip\n"\
"\t\tsysinfo get ap mask\n"\
"\t\tsysinfo get ap gateway"
#define sysinfo_chip_help_info "get chip arch version"
#define sysinfo_version_help_info "get SDK version"
#endif

static enum cmd_status cmd_sysinfo_help_exec(char *cmd);

static const struct cmd_data g_sysinfo_cmds[] = {
    { "default", cmd_sysinfo_default_exec, CMD_DESC(sysinfo_default_help_info) },
#if PRJCONF_SYSINFO_SAVE_TO_FLASH
    { "save",    cmd_sysinfo_save_exec, CMD_DESC(sysinfo_save_help_info) },
    { "load",    cmd_sysinfo_load_exec, CMD_DESC(sysinfo_load_help_info) },
#endif
    { "set",     cmd_sysinfo_set_exec, CMD_DESC(sysinfo_set_help_info) },
    { "get",     cmd_sysinfo_get_exec, CMD_DESC(sysinfo_get_help_info) },
    { "chip",    cmd_sysinfo_chip_exec, CMD_DESC(sysinfo_chip_help_info) },
    { "version", cmd_sysinfo_version_exec, CMD_DESC(sysinfo_version_help_info) },
    { "help",    cmd_sysinfo_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_sysinfo_help_exec(char *cmd)
{
	return cmd_help_exec(g_sysinfo_cmds, cmd_nitems(g_sysinfo_cmds), 8);
}

enum cmd_status cmd_sysinfo_exec(char *cmd)
{
	return cmd_exec(cmd, g_sysinfo_cmds, cmd_nitems(g_sysinfo_cmds));
}
