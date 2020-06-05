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

#include "cmd_util.h"
#include "ota/ota.h"
#include "common/framework/fs_ctrl.h"

/*
 * ota file <url>
 * ota http <url>
 */

#if (OTA_OPT_PROTOCOL_FILE && PRJCONF_MMC_EN)
enum cmd_status cmd_ota_file_exec(char *cmd)
{
	uint32_t *verify_value;
	ota_verify_t verify_type;
	ota_verify_data_t verify_data;

	if (cmd[0] == '\0') {
		CMD_ERR("OTA empty file url\n");
		return CMD_STATUS_INVALID_ARG;
	}

	if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
		return CMD_STATUS_FAIL;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");

	if (ota_get_image(OTA_PROTOCOL_FILE, cmd) != OTA_STATUS_OK) {
		CMD_ERR("OTA file get image failed\n");
		return CMD_STATUS_ACKED;
	}

	if (ota_get_verify_data(&verify_data) != OTA_STATUS_OK) {
		verify_type = OTA_VERIFY_NONE;
		verify_value = NULL;
	} else {
		verify_type = verify_data.ov_type;
		verify_value = (uint32_t*)(verify_data.ov_data);
	}

	if (ota_verify_image(verify_type, verify_value)  != OTA_STATUS_OK) {
		CMD_ERR("OTA file verify image failed\n");
		return CMD_STATUS_ACKED;
	}

	ota_reboot();

	return CMD_STATUS_ACKED;
}
#endif /* OTA_OPT_PROTOCOL_FILE */

#if (OTA_OPT_PROTOCOL_HTTP && PRJCONF_NET_EN)
enum cmd_status cmd_ota_http_exec(char *cmd)
{
	uint32_t *verify_value;
	ota_verify_t verify_type;
	ota_verify_data_t verify_data;

	if (cmd[0] == '\0') {
		CMD_ERR("OTA empty http url\n");
		return CMD_STATUS_INVALID_ARG;
	}

	cmd_write_respond(CMD_STATUS_OK, "OK");

	if (ota_get_image(OTA_PROTOCOL_HTTP, cmd) != OTA_STATUS_OK) {
		CMD_ERR("OTA http get image failed\n");
		return CMD_STATUS_ACKED;
	}

	if (ota_get_verify_data(&verify_data) != OTA_STATUS_OK) {
		verify_type = OTA_VERIFY_NONE;
		verify_value = NULL;
	} else {
		verify_type = verify_data.ov_type;
		verify_value = (uint32_t*)(verify_data.ov_data);
	}

	if (ota_verify_image(verify_type, verify_value)  != OTA_STATUS_OK) {
		CMD_ERR("OTA http verify image failed\n");
		return CMD_STATUS_ACKED;
	}

	ota_reboot();

	return CMD_STATUS_ACKED;
}
#endif /* OTA_OPT_PROTOCOL_HTTP */

static enum cmd_status cmd_ota_help_exec(char *cmd);

static const struct cmd_data g_ota_cmds[] = {
#if (OTA_OPT_PROTOCOL_FILE && PRJCONF_MMC_EN)
    { "file",	cmd_ota_file_exec, CMD_DESC("file <url>, ota from file, eg. ota file file://0:/ota/xr_system.img") },
#endif
#if (OTA_OPT_PROTOCOL_HTTP && PRJCONF_NET_EN)
    { "http",	cmd_ota_http_exec, CMD_DESC("http <url>, ota from http, eg. ota http http://192.168.1.1/xr_system.img") },
#endif
    { "help",	cmd_ota_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_ota_help_exec(char *cmd)
{
	return cmd_help_exec(g_ota_cmds, cmd_nitems(g_ota_cmds), 8);
}

enum cmd_status cmd_ota_exec(char *cmd)
{
	return cmd_exec(cmd, g_ota_cmds, cmd_nitems(g_ota_cmds));
}
