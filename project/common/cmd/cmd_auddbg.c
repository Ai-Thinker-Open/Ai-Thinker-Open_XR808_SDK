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

#include "cmd_debug.h"
#include "cmd_util.h"
#include "driver/chip/hal_snd_card.h"
#include "audio/manager/audio_manager.h"
#include "driver/chip/chip.h"


void HAL_I2S_REG_DEBUG()
{
	int i = 0;
	for (i = 0; i < 0x58; i = i + 4)
		printf("REG:0x%x,VAL:0x%x\n",i, (*((__IO uint32_t *)(0x40042c00 + i))));
}

void HAL_CODEC_REG_DEBUG(uint8_t card)
{
	int i = 0;

	if (card == 0) {

		uint32_t reg_val;
		for (i = 0; i < 0x99; i = i+1) {
			reg_val = audio_manager_reg_read(AUDIO_SND_CARD_DEFAULT, i);
			if (reg_val)
				printf("REG: 0x%x, VAL: 0x%x\n", i, reg_val);
		}
	} else {
		for (i = 0; i <= 0x44; i = i + 4)
			printf("REG:0x%02x,VAL:0x%08x\n", i, (*((__IO uint32_t *)(0x40044000 + i))));
	}
}

static enum cmd_status cmd_auddbg_i2s_reg_exec(char *cmd)
{
	void HAL_I2S_REG_DEBUG();
	HAL_I2S_REG_DEBUG();
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_auddbg_codec_reg_exec(char *cmd)
{
	uint32_t card;
	cmd_sscanf(cmd, "%d", &card);

	if (card != 0 && card != 1)
	{
		CMD_ERR("card type only 0 or 1");
		return CMD_STATUS_INVALID_ARG;
	}

	HAL_CODEC_REG_DEBUG(card);
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_auddbg_pa_exec(char *cmd)
{
	uint32_t on;
	cmd_sscanf(cmd, "%d", &on);

	if (on != 0 && on != 1)
	{
		CMD_ERR("pa enable only 0 or 1");
		return CMD_STATUS_INVALID_ARG;
	}

	audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_MUTE, AUDIO_OUT_DEV_SPK, on ? AUDIO_UNMUTE : AUDIO_MUTE);

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_auddbg_codec_read_exec(char *cmd)
{
	int32_t cnt;
	uint32_t creg;
	cnt = cmd_sscanf(cmd, "0x%x", &creg);
	if (cnt != 1)
	{
		CMD_ERR("example: 0x10\n");
		return CMD_STATUS_INVALID_ARG;
	}

	CMD_DBG("CODEC REG:0x%x, VAL:0x%x ;\n", creg, audio_manager_reg_read(AUDIO_SND_CARD_DEFAULT, creg));

	return CMD_STATUS_OK;
}

static enum cmd_status cmd_auddbg_codec_write_exec(char *cmd)
{
	int32_t cnt;
	uint32_t creg;
	uint32_t val;
	cnt = cmd_sscanf(cmd, "0x%x 0x%x", &creg, &val);
	if (cnt != 2)
	{
		CMD_ERR("example: 0x10 0x10\n");
		return CMD_STATUS_INVALID_ARG;
	}

	audio_manager_reg_write(AUDIO_SND_CARD_DEFAULT, creg, val);
	CMD_DBG("CODEC REG:0x%x, VAL:0x%x ;\n", creg, audio_manager_reg_read(AUDIO_SND_CARD_DEFAULT, creg));
	return CMD_STATUS_OK;
}

static enum cmd_status cmd_auddbg_codec_dac_exec(char *cmd)
{
	CMD_DBG("CODEC ADC part debug\n");

	//audio_manager_handler(AUDIO_MANAGER_SET_ROUTE, AUDIO_OUT_DEV_SPK);
	audio_manager_reg_write(AUDIO_SND_CARD_DEFAULT, 0x4a, 0x40);

	return CMD_STATUS_OK;
}

/*
 * brief audio debug Command
 * command  audbg i2s
 *			audbg codec
 *          audbg pa 0
 *          audbg read 0x51
 *          audbg write 0x51 0x20
 *          audbg codec-dac
 */
static enum cmd_status cmd_auddbg_help_exec(char *cmd);

static const struct cmd_data g_auddbg_cmds[] = {
	{ "i2s",		cmd_auddbg_i2s_reg_exec, CMD_DESC("print the i2s register") },
	{ "codec",		cmd_auddbg_codec_reg_exec, CMD_DESC("print the codec register") },
	{ "pa",     	cmd_auddbg_pa_exec, CMD_DESC("open/close pa, auddbg pa {0|1}") },
	{ "read",    	cmd_auddbg_codec_read_exec, CMD_DESC("read register, auddbg read <addr>") },
	{ "write",    	cmd_auddbg_codec_write_exec, CMD_DESC("write register, auddbg write <addr> <value>") },
	{ "codec-dac",	cmd_auddbg_codec_dac_exec, CMD_DESC("codec ADC part debug") },
	{ "help",	    cmd_auddbg_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_auddbg_help_exec(char *cmd)
{
	return cmd_help_exec(g_auddbg_cmds, cmd_nitems(g_auddbg_cmds), 8);
}

enum cmd_status cmd_auddbg_exec(char *cmd)
{
	return cmd_exec(cmd, g_auddbg_cmds, cmd_nitems(g_auddbg_cmds));
}
