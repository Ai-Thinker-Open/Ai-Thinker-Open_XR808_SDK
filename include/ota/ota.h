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

#ifndef _OTA_OTA_H_
#define _OTA_OTA_H_

#include <stdint.h>
#include "ota/ota_opt.h"
#include "image/image.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief OTA status definition
 */
typedef enum ota_status {
	OTA_STATUS_OK		= 0,
	OTA_STATUS_ERROR	= -1,
} ota_status_t;

/**
 * @brief OTA protocol definition
 */
typedef enum ota_protocol {
#if OTA_OPT_PROTOCOL_FILE
	OTA_PROTOCOL_FILE	= 0,
#endif
#if OTA_OPT_PROTOCOL_HTTP
	OTA_PROTOCOL_HTTP	= 1,
#endif
} ota_protocol_t;

/**
 * @brief OTA image verification algorithm definition
 */
typedef enum ota_verify {
	OTA_VERIFY_NONE		= 0,
#if OTA_OPT_EXTRA_VERIFY_CRC32
	OTA_VERIFY_CRC32	= 1,
#endif
#if OTA_OPT_EXTRA_VERIFY_MD5
	OTA_VERIFY_MD5		= 2,
#endif
#if OTA_OPT_EXTRA_VERIFY_SHA1
	OTA_VERIFY_SHA1		= 3,
#endif
#if OTA_OPT_EXTRA_VERIFY_SHA256
	OTA_VERIFY_SHA256	= 4,
#endif
} ota_verify_t;


/**
 * @brief OTA image verification data structure definition
 */
#define OTA_VERIFY_MAGIC        0x0055AAFF
#define OTA_VERIFY_DATA_SIZE	32
typedef struct ota_verify_data {
	uint32_t ov_magic;             /* OTA Verify Header Magic Number */
	uint16_t ov_length;            /* OTA Verify Data Length              */
	uint16_t ov_version;           /* OTA Verify Version: 0.0              */
	uint16_t ov_type;              /* OTA Verify Type                        */
	uint16_t ov_reserve;
	uint8_t ov_data[OTA_VERIFY_DATA_SIZE];
} ota_verify_data_t;

typedef enum ota_upgrade_status {
	OTA_UPGRADE_STOP,
	OTA_UPGRADE_START,
	OTA_UPGRADE_SUCCESS,
	OTA_UPGRADE_FAIL,
	OTA_UPGRADE_UPDATING
} ota_upgrade_status_t;

typedef void (*ota_callback) (ota_upgrade_status_t status, uint32_t data_size, uint32_t percentage);

ota_status_t ota_init(void);
void ota_deinit(void);
ota_status_t ota_set_skip_size(int32_t skip_size);
ota_status_t ota_set_cb(ota_callback cb);

ota_status_t ota_push_init(void);
ota_status_t ota_push_start(void);
ota_status_t ota_push_data(uint8_t *data, uint32_t size);
ota_status_t ota_push_finish(void);
ota_status_t ota_push_stop(void);

ota_status_t ota_get_image(ota_protocol_t protocol, void *url);
ota_status_t ota_get_verify_data(ota_verify_data_t *data);
ota_status_t ota_verify_image(ota_verify_t verify, uint32_t *value);
void ota_reboot(void);

#ifdef __cplusplus
}
#endif

#endif /* _OTA_OTA_H_ */
