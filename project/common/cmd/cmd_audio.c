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
#include "cmd_audio.h"
#include "audio/pcm/audio_pcm.h"
#include "audio/manager/audio_manager.h"
#include "driver/chip/hal_snd_card.h"
#include "fs/fatfs/ff.h"
#include "common/framework/fs_ctrl.h"


#define CMD_AUDIO_SND_CARD			AUDIO_SND_CARD_DEFAULT
#define AUDIO_RECORD_TIME			(20)//s

#define AUDIO_THREAD_STACK_SIZE		(2 * 1024)
#define AUDIO_DELETE_THREAD(THREAD)  OS_ThreadDelete(&THREAD)
#define AUDIO_CREAT_THREAD(THREAD, TASK, ARG)         \
                        if (OS_ThreadIsValid(&THREAD)) { \
                                CMD_ERR("audio task is running\n"); \
                                return CMD_STATUS_FAIL; \
                        } \
                        if (OS_ThreadCreate(&THREAD, \
                                   "",               \
                                   TASK,    \
                                   (void *)ARG,         \
                                   OS_THREAD_PRIO_APP,   \
                                   AUDIO_THREAD_STACK_SIZE) != OS_OK) { \
                                CMD_ERR("audio task create failed\n"); \
                                return CMD_STATUS_FAIL; \
                       }

static OS_Thread_t g_audio_record_thread;
static OS_Thread_t g_audio_play_thread;
static OS_Thread_t g_audio_control_thread;
static uint8_t g_audio_task_end;
static uint32_t sampleRate[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, 96000, 192000};


static int AudioConfigIsValid(int samplerate, int channels)
{
	int i;
	int sr_num;

	//if ((channels != 1) && (channels != 2))
	//	return 0;

	sr_num = sizeof(sampleRate) / sizeof(sampleRate[0]);
	for (i = 0; i < sr_num; i++) {
		if (sampleRate[i] == samplerate) {
			return 1;
		}
	}

	return 0;
}

static int AudioSetConfig(int samplerate, int channels, struct pcm_config *config)
{
	if (!AudioConfigIsValid(samplerate, channels)) {
		return -1;
	}

	config->channels = channels;
	config->rate = samplerate;
	config->period_size = 2048;
	config->period_count = 2;
	config->format = PCM_FORMAT_S16_LE;

	return 0;
}

static void cap_exec(void *cmd)
{
	int argc, ret = 0;
	unsigned int writenum = 0;
	int samplerate;
	int channels;
	char *argv[5];
	FIL fp;
	char *file_path;
	unsigned int pcm_buf_size;
	char *pcm_data;
	uint32_t stopTime;
	struct pcm_config config;

	if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
		CMD_ERR("mount fail\n");
		goto exit_thread;
	}

	argc = cmd_parse_argv(cmd, argv, 5);
	if (argc < 3) {
		CMD_ERR("invalid audio capture cmd, argc %d\n", argc);
		goto exit_thread;
	}
	samplerate = cmd_atoi(argv[0]);
	channels = cmd_atoi(argv[1]);
	file_path = argv[2];

	CMD_DBG("CMD:drv audio cap (samplerate)%d (channel)%d (file)%s\n", samplerate, channels, file_path);

	if (AudioSetConfig(samplerate, channels, &config)) {
		CMD_ERR("invalid audio cap param.\n");
		goto exit_thread;
	}

	f_unlink(file_path);
	if (f_open(&fp, file_path, FA_OPEN_ALWAYS | FA_WRITE) != FR_OK) {
		CMD_ERR("open file fail\n");
		goto exit_thread;
	}

    pcm_buf_size = (config.channels)*2*(config.period_size);
    pcm_data = malloc(pcm_buf_size);
    if (pcm_data == NULL) {
		CMD_ERR("malloc buf failed\n");
		f_close(&fp);
		goto exit_thread;
    }
    memset(pcm_data, 0, pcm_buf_size);

    if (snd_pcm_open(CMD_AUDIO_SND_CARD, PCM_IN, &config) != 0)
    {
		CMD_ERR("sound card open err\n");
		goto exit_fs;
    }

	stopTime = OS_TicksToSecs(OS_GetTicks()) + AUDIO_RECORD_TIME;
	if (stopTime < AUDIO_RECORD_TIME) {
		CMD_ERR("stopTime overflow\n");
		goto exit_snd;
	}

	g_audio_task_end = 0;
	CMD_DBG("Capture run.\n");
    while (!g_audio_task_end) {// && OS_TicksToSecs(OS_GetTicks()) <= stopTime
            ret = snd_pcm_read(CMD_AUDIO_SND_CARD, pcm_data, pcm_buf_size);
            if (ret != pcm_buf_size) {
				CMD_ERR("read data failed(%d), line:%d\n", ret, __LINE__);
				break;
            }
            if (f_write(&fp, pcm_data, pcm_buf_size, &writenum) != FR_OK) {
				CMD_ERR("write failed.\n");
				break;
            } else {
				if (writenum != pcm_buf_size) {
					CMD_ERR("write failed %d, %d\n", writenum, __LINE__);
					break;
				}
            }
    }

exit_snd:
    snd_pcm_close(CMD_AUDIO_SND_CARD, PCM_IN);

exit_fs:
	free(pcm_data);
	f_close(&fp);
	CMD_DBG("Capture end.\n");

exit_thread:
	AUDIO_DELETE_THREAD(g_audio_record_thread);
}

static void play_exec(void *cmd)
{
	int argc;
	unsigned int readnum = 0;
	int samplerate;
	int channels;
	char *argv[5];
	FIL fp;
	char *file_path;
	unsigned int pcm_buf_size;
	char *pcm_data;
	struct pcm_config config;

	if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
		CMD_ERR("mount fail\n");
		goto exit_thread;
	}

	argc = cmd_parse_argv(cmd, argv, 5);
	if (argc < 3) {
		CMD_ERR("invalid audio capture cmd, argc %d\n", argc);
		goto exit_thread;
	}
	samplerate = cmd_atoi(argv[0]);
	channels = cmd_atoi(argv[1]);
	file_path = argv[2];

	CMD_DBG("CMD:drv audio play (samplerate)%d (channel)%d (file)%s\n", samplerate, channels, file_path);

	if (AudioSetConfig(samplerate, channels, &config)) {
		CMD_ERR("invalid audio cap param.\n");
		goto exit_thread;
	}

	if (f_open(&fp, file_path, FA_OPEN_EXISTING | FA_READ) != FR_OK) {
		CMD_ERR("open file fail\n");
		goto exit_thread;
	}

    pcm_buf_size = (config.channels)*2*(config.period_size);
    pcm_data = malloc(pcm_buf_size);
    if (pcm_data == NULL) {
		CMD_ERR("malloc buf failed\n");
		f_close(&fp);
		goto exit_thread;;
    }
    if (snd_pcm_open(CMD_AUDIO_SND_CARD, PCM_OUT, &config) != 0)
    {
		CMD_ERR("sound card open err\n");
		goto exit_fs;
    }
	CMD_DBG("Play on.\n");
	g_audio_task_end = 0;
    while (!g_audio_task_end) {
		if (f_read(&fp, pcm_data, pcm_buf_size, &readnum) != FR_OK) {
	        CMD_ERR("read failed.\n");
			break;
	    }
		snd_pcm_write(CMD_AUDIO_SND_CARD, pcm_data, readnum);

        if (readnum != pcm_buf_size) {
			CMD_DBG("file end: file size = %d\n", readnum);
			break;
        }
    }

	snd_pcm_flush(CMD_AUDIO_SND_CARD);
    snd_pcm_close(CMD_AUDIO_SND_CARD, PCM_OUT);

exit_fs:
	f_close(&fp);
	free(pcm_data);
	CMD_DBG("Play end.\n");
exit_thread:
	AUDIO_DELETE_THREAD(g_audio_play_thread);
}

static void vol_exec(void *cmd)
{
	int argc;
	char *argv[3];

	argc = cmd_parse_argv(cmd, argv, 3);
	if (argc < 2) {
		CMD_ERR("invalid audio set vol cmd, argc %d\n", argc);
		goto exit;
	}

	uint16_t dev = cmd_atoi(argv[0]);
	int vol = cmd_atoi(argv[1]);
	CMD_DBG("CMD:drv audio vol (level)%d\n", vol);

	if (vol > VOLUME_MAX_LEVEL) {
		CMD_ERR("invalid audio vol.Range(0-%d)\n",VOLUME_MAX_LEVEL);
		goto exit;
	}

	audio_manager_handler(CMD_AUDIO_SND_CARD, AUDIO_MANAGER_SET_VOLUME_LEVEL, dev, vol);
exit:
	AUDIO_DELETE_THREAD(g_audio_control_thread);
}

static void path_exec(void *cmd)
{
	int argc;
	char *argv[2];

	argc = cmd_parse_argv(cmd, argv, 2);
	if (argc < 1) {
		CMD_ERR("invalid audio set path cmd, argc %d\n", argc);
		goto exit;
	}
	int path = cmd_atoi(argv[0]);
	CMD_DBG("CMD:drv audio out-path %d\n", path);

	if (path > AUDIO_OUT_DEV_SPK) {
		CMD_ERR("invalid audio out-path.Range(1-2)\n");
		goto exit;
	}

	audio_manager_handler(CMD_AUDIO_SND_CARD, AUDIO_MANAGER_SET_ROUTE, path, cmd_atoi(argv[1]));
exit:
	AUDIO_DELETE_THREAD(g_audio_control_thread);
}

static enum cmd_status audio_play_task(char *arg)
{
	char *cmd = (char *)arg;

	AUDIO_CREAT_THREAD(g_audio_play_thread, play_exec, cmd);
	return CMD_STATUS_OK;
}

static enum cmd_status audio_cap_task(char *arg)
{
	char *cmd = (char *)arg;

	AUDIO_CREAT_THREAD(g_audio_record_thread, cap_exec, cmd);
	return CMD_STATUS_OK;
}

static enum cmd_status audio_vol_task(char *arg)
{
	char *cmd = (char *)arg;

	AUDIO_CREAT_THREAD(g_audio_control_thread, vol_exec, cmd);
	return CMD_STATUS_OK;
}

static enum cmd_status audio_path_task(char *arg)
{
	char *cmd = (char *)arg;

	AUDIO_CREAT_THREAD(g_audio_control_thread, path_exec, cmd);
	return CMD_STATUS_OK;
}

static enum cmd_status audio_end_task(char *arg)
{
	char *cmd = (char *)arg;
	(void)cmd;

	g_audio_task_end = 1;
	CMD_DBG("audio task end\n");

	return CMD_STATUS_OK;
}

/*
 * brief audio Test Command
 * command
 *		1)cap:	$ audio cap [samplerate] [channels] [file-path]
 *		2)play:	$ audio play [samplerate] [channels] [file-path]
 *		3)vol:	$ audio vol [dev] [vol]
 *		4)path:	$ audio path [dev] [en]
 *		5)end : $ audio end
 *		samplerate: [8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000]
 *		channels:   [1~2]
 *		vol:   	    [0~31]
 *		dev:		device mask
 *		en:			[0~1]
 * example
 *		audio cap 16000 1 record.pcm
 *		audio play 44100 2 music.pcm
 *      audio vol 12
 *		audio path	1 1
 *		audio end
 */

#if CMD_DESCRIBE
#define audio_cap_help_info \
"audio cap [samplerate] [channels] [file-path]\n"\
"\t\t\teg. audio cap 16000 1 record.pcm"
#define audio_play_help_info \
"audio play [samplerate] [channels] [file-path]\n"\
"\t\t\teg. audio play 44100 2 music.pcm"
#define audio_vol_help_info \
"audio vol [dev] [vol]\n"\
"\t\t\teg. audio vol 12"
#define audio_path_help_info \
"audio path [dev] [en]\n"\
"\t\t\teg. audio path	1 1"
#define audio_end_help_info "audio end"
#endif

static enum cmd_status cmd_audio_help_exec(char *cmd);

static const struct cmd_data g_audio_cmds[] = {
	{ "cap",     audio_cap_task, CMD_DESC(audio_cap_help_info) },
	{ "play",    audio_play_task, CMD_DESC(audio_play_help_info) },
	{ "vol",     audio_vol_task, CMD_DESC(audio_vol_help_info) },
	{ "path",    audio_path_task, CMD_DESC(audio_path_help_info) },
	{ "end",     audio_end_task, CMD_DESC(audio_end_help_info) },
	{ "help",    cmd_audio_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_audio_help_exec(char *cmd)
{
	return cmd_help_exec(g_audio_cmds, cmd_nitems(g_audio_cmds), 8);
}

enum cmd_status cmd_audio_exec(char *cmd)
{
	return cmd_exec(cmd, g_audio_cmds, cmd_nitems(g_audio_cmds));
}
