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

#ifdef __CONFIG_XPLAYER

#include "cmd_util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cdx_log.h>
#include "xplayer.h"
#include "fs/fatfs/ff.h"
#include "common/framework/fs_ctrl.h"
#include "xrecord.h"
#include "CaptureControl.h"
#include "driver/chip/hal_snd_card.h"
#include "audio/manager/audio_manager.h"

extern SoundCtrl* SoundDeviceCreate();

typedef struct DemoPlayerContext
{
    XPlayer*        mAwPlayer;
    int             mSeekable;
    int             mError;
}DemoPlayerContext;

static uint8_t cedarx_inited = 0;
static int fs_init(void)
{
    if (cedarx_inited++ == 0) {
        if (fs_ctrl_mount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
            printf("mount fail\n");
            return -1;
        } else {
            printf("mount success\n");
        }
    }
    return 0;
}

static void fs_exit(void)
{
    if (--cedarx_inited == 0) {
        if (fs_ctrl_unmount(FS_MNT_DEV_TYPE_SDCARD, 0) != 0) {
            printf("unmount fail\n");
        }
    }
}

static void set_source(DemoPlayerContext *demoPlayer, char* pUrl)
{
    demoPlayer->mSeekable = 1;

    //* set url to the AwPlayer.
    if(XPlayerSetDataSourceUrl(demoPlayer->mAwPlayer,
                 (const char*)pUrl, NULL, NULL) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::setDataSource() return fail.\n");
        return;
    }
    printf("setDataSource end\n");

    if ((!strncmp(pUrl, "http://", 7)) || (!strncmp(pUrl, "https://", 8))) {
        if(XPlayerPrepareAsync(demoPlayer->mAwPlayer) != 0)
        {
            printf("error:\n");
            printf("    AwPlayer::prepareAsync() return fail.\n");
            return;
        }
    }
    printf("preparing...\n");
}

static void play(DemoPlayerContext *demoPlayer)
{
    if(XPlayerStart(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::start() return fail.\n");
        return;
    }
    printf("playing.\n");
}

static void pause(DemoPlayerContext *demoPlayer)
{
    if(XPlayerPause(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::pause() return fail.\n");
        return;
    }
    printf("paused.\n");
}

static void stop(DemoPlayerContext *demoPlayer)
{
    if(XPlayerReset(demoPlayer->mAwPlayer) != 0)
    {
        printf("error:\n");
        printf("    AwPlayer::reset() return fail.\n");
        return;
    }
    printf("stopped.\n");
}

//* a callback for awplayer.
static int CallbackForAwPlayer(void* pUserData, int msg, int ext1, void* param)
{
    DemoPlayerContext* pDemoPlayer = (DemoPlayerContext*)pUserData;

    switch(msg)
    {
        case AWPLAYER_MEDIA_INFO:
        {
            switch(ext1)
            {
                case AW_MEDIA_INFO_NOT_SEEKABLE:
                {
                    pDemoPlayer->mSeekable = 0;
                    printf("info: media source is unseekable.\n");
                    break;
                }
                case AW_MEDIA_INFO_RENDERING_START:
                {
                    printf("info: start to show pictures.\n");
                    break;
                }
            }
            break;
        }

        case AWPLAYER_MEDIA_ERROR:
        {
            printf("error: open media source fail.\n");
            pDemoPlayer->mError = 1;
            loge(" error : how to deal with it");
            break;
        }

        case AWPLAYER_MEDIA_PREPARED:
        {
            logd("info : preared");
            printf("info: prepare ok.\n");
            break;
        }

        case AWPLAYER_MEDIA_PLAYBACK_COMPLETE:
        {
            printf("info: play complete.\n");
            break;
        }

        case AWPLAYER_MEDIA_SEEK_COMPLETE:
        {
            printf("info: seek ok.\n");
            break;
        }

        case AWPLAYER_MEDIA_CHANGE_URL:
        {
            printf("suggest to play:%s\n", (char *)param);
            break;
        }

        default:
        {
            //printf("warning: unknown callback from AwPlayer.\n");
            break;
        }
    }

    return 0;
}

static DemoPlayerContext *demoPlayer;

static enum cmd_status cmd_cedarx_create_exec(char *cmd)
{
    demoPlayer = malloc(sizeof(*demoPlayer));

    fs_init();
    audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_OUT_DEV_SPK, 14);

    //* create a player.
    memset(demoPlayer, 0, sizeof(DemoPlayerContext));

    demoPlayer->mAwPlayer = XPlayerCreate();
    if(demoPlayer->mAwPlayer == NULL)
    {
        printf("can not create AwPlayer, quit.\n");
        return -1;
    } else {
        printf("create AwPlayer success.\n");
    }

    //* set callback to player.
    XPlayerSetNotifyCallback(demoPlayer->mAwPlayer, CallbackForAwPlayer, (void*)demoPlayer);

    //* check if the player work.
    if(XPlayerInitCheck(demoPlayer->mAwPlayer) != 0)
    {
        printf("initCheck of the player fail, quit.\n");
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
        return -1;
    } else
        printf("AwPlayer check success.\n");

    SoundCtrl* sound = SoundDeviceCreate();

    XPlayerSetAudioSink(demoPlayer->mAwPlayer, (void*)sound);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_destroy_exec(char *cmd)
{
    if(demoPlayer->mAwPlayer != NULL)
    {
        XPlayerDestroy(demoPlayer->mAwPlayer);
        demoPlayer->mAwPlayer = NULL;
    }
    printf("destroy AwPlayer.\n");

    fs_exit();

    free(demoPlayer);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_play_exec(char *cmd)
{
    play(demoPlayer);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_stop_exec(char *cmd)
{
    stop(demoPlayer);
    msleep(5);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_pause_exec(char *cmd)
{
    pause(demoPlayer);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_seturl_exec(char *cmd)
{
    set_source(demoPlayer, cmd);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_getpos_exec(char *cmd)
{
    int msec;
    XPlayerGetCurrentPosition(demoPlayer->mAwPlayer, &msec);
    cmd_write_respond(CMD_STATUS_OK, "played time: %d ms", msec);
    return CMD_STATUS_ACKED;
}

static enum cmd_status cmd_cedarx_seek_exec(char *cmd)
{
    int nSeekTimeMs = atoi(cmd);

    XPlayerSeekTo(demoPlayer->mAwPlayer, nSeekTimeMs);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_setvol_exec(char *cmd)
{
    int vol = atoi(cmd);
    if (vol > 31)
        vol = 31;

    audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_OUT_DEV_SPK, vol);

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_playdic_exec(char *cmd)
{
    return CMD_STATUS_OK;
}

static XRecord *xrecord;

static enum cmd_status cmd_cedarx_rec_exec(char *cmd)
{
    XRECODER_AUDIO_ENCODE_TYPE type;
    if (strstr(cmd, ".amr"))
        type = XRECODER_AUDIO_ENCODE_AMR_TYPE;
    else if (strstr(cmd, ".pcm"))
        type = XRECODER_AUDIO_ENCODE_PCM_TYPE;
    else {
        printf("do not support this encode type\n");
        return CMD_STATUS_INVALID_ARG;
    }

    fs_init();
    audio_manager_handler(AUDIO_SND_CARD_DEFAULT, AUDIO_MANAGER_SET_VOLUME_LEVEL, AUDIO_IN_DEV_AMIC, 3);

    xrecord = XRecordCreate();
    if (xrecord == NULL) {
        printf("xrecord create failed\n");
        return CMD_STATUS_FAIL;
    }

    CaptureCtrl* cap = CaptureDeviceCreate();
    if (!cap) {
        printf("cap device create failed\n");
        XRecordDestroy(xrecord);
        xrecord = NULL;
        return CMD_STATUS_FAIL;
    }
    XRecordSetAudioCap(xrecord, cap);

    XRecordConfig audioConfig;
    audioConfig.nChan = 1;
    audioConfig.nSamplerate = 8000;
    audioConfig.nSamplerBits = 16;
    audioConfig.nBitrate = 12200;

    XRecordSetDataDstUrl(xrecord, cmd, NULL, NULL);
    XRecordSetAudioEncodeType(xrecord, type, &audioConfig);

    XRecordPrepare(xrecord);
    XRecordStart(xrecord);
    printf("record start\n");

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_end_exec(char *cmd)
{
    XRecordStop(xrecord);
    XRecordDestroy(xrecord);
    printf("record destroy\n");

    fs_exit();

    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_log_exec(char *cmd)
{
    int level = atoi(cmd);
    if (level > 6)
        level = 6;

    void log_set_level(unsigned level);
    log_set_level(level);
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_version_exec(char *cmd)
{
    XPlayerShowVersion();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_showbuf_exec(char *cmd)
{
    XPlayerShowBuffer();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_setbuf_exec(char *cmd)
{
    int argc;
    char *argv[4];
    XPlayerBufferConfig cfg;
    int maxAudioFrameSize;
    argc = cmd_parse_argv(cmd, argv, 4);
    if (argc < 4) {
        CMD_ERR("invalid cedarx set buf cmd, argc %d\n", argc);
        goto exit;
    }

    cfg.maxStreamBufferSize = cmd_atoi(argv[0]);
    cfg.maxBitStreamBufferSize = cmd_atoi(argv[1]);
    cfg.maxPcmBufferSize = cmd_atoi(argv[2]);
    cfg.maxStreamFrameCount = 0;
    cfg.maxBitStreamFrameCount = 0;
    cfg.maxMovStszBufferSize = 0;
    cfg.maxMovStcoBufferSize = 0;
    maxAudioFrameSize = cmd_atoi(argv[3]);

    XPlayerSetBuffer(demoPlayer->mAwPlayer, &cfg);
    XPlayerSetPcmFrameSize(demoPlayer->mAwPlayer, maxAudioFrameSize);
    XPlayerShowBuffer();
exit:
    return CMD_STATUS_OK;
}

extern void CdxBufAutoStatStart(void);
static enum cmd_status cmd_cedarx_bufinfo_exec(char *cmd)
{
    CdxBufAutoStatStart();
    return CMD_STATUS_OK;
}

static enum cmd_status cmd_cedarx_aacsbr_exec(char *cmd)
{
    int use_sbr = atoi(cmd);

    if (demoPlayer->mAwPlayer)
    {
        XPlayerSetAacSbr(demoPlayer->mAwPlayer, use_sbr);
        printf("set aac sbr success\n");
    }
    else
    {
        printf("set aac sbr fail\n");
    }
    return CMD_STATUS_OK;
}

/*
 * brief cedarx Test Command
 *          cedarx play
 *          cedarx stop
 *          cedarx pause
 *          cedarx seturl file://music/01.mp3
 *          cedarx getpos
 *          cedarx seek 6000
 *          cedarx setvol 8
 *          cedarx playdic file://music
 *          cedarx rec file://record/wechat.amr
 *          cedarx end
 */
static enum cmd_status cmd_cedarx_help_exec(char *cmd);

static const struct cmd_data g_cedarx_cmds[] = {
    { "create",     cmd_cedarx_create_exec, CMD_DESC("create cedarx object") },
    { "destroy",    cmd_cedarx_destroy_exec, CMD_DESC("destroy cedarx object") },
    { "play",       cmd_cedarx_play_exec, CMD_DESC("cedarx play") },
    { "stop",       cmd_cedarx_stop_exec, CMD_DESC("cedarx stop") },
    { "pause",      cmd_cedarx_pause_exec, CMD_DESC("cedarx pause") },
    { "seturl",     cmd_cedarx_seturl_exec, CMD_DESC("set url, eg. cedarx seturl file://music/01.mp3") },
    { "getpos",     cmd_cedarx_getpos_exec, CMD_DESC("get current position") },
    { "seek",       cmd_cedarx_seek_exec, CMD_DESC("seek to position, eg. cedarx seek 6000") },
    { "setvol",     cmd_cedarx_setvol_exec, CMD_DESC("set volume, 0~31, eg. cedarx setvol 8") },
    { "playdic",    cmd_cedarx_playdic_exec, CMD_DESC("not support") },
    { "log",        cmd_cedarx_log_exec, CMD_DESC("set log level") },
    { "version",    cmd_cedarx_version_exec, CMD_DESC("show version") },
    { "showbuf",    cmd_cedarx_showbuf_exec, CMD_DESC("show buffer") },
    { "setbuf",     cmd_cedarx_setbuf_exec, CMD_DESC("set buffer size") },
    { "bufinfo",    cmd_cedarx_bufinfo_exec, CMD_DESC("show buffer info") },
    { "aacsbr",     cmd_cedarx_aacsbr_exec, CMD_DESC("set use aac sbr") },
    { "rec",        cmd_cedarx_rec_exec, CMD_DESC("record start, eg. cedarx rec file://record/wechat.amr or wechat.pcm") },
    { "end",        cmd_cedarx_end_exec, CMD_DESC("record destroy") },
    { "help",       cmd_cedarx_help_exec, CMD_DESC(CMD_HELP_DESC) },
};

static enum cmd_status cmd_cedarx_help_exec(char *cmd)
{
	return cmd_help_exec(g_cedarx_cmds, cmd_nitems(g_cedarx_cmds), 8);
}

enum cmd_status cmd_cedarx_exec(char *cmd)
{
    return cmd_exec(cmd, g_cedarx_cmds, cmd_nitems(g_cedarx_cmds));
}

#endif /* __CONFIG_XPLAYER */
