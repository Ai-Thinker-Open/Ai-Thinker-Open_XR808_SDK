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

#include "board_common.h"
#include "board.h"
#include "driver/chip/psram/psram.h"

HAL_Status board_pinmux_cfg(HAL_BoardIoctlReq req,
                            const GPIO_PinMuxParam *pinmux, uint32_t count)
{
	switch (req) {
	case HAL_BIR_PINMUX_INIT:
		HAL_GPIO_PinMuxConfig(pinmux, count);
		break;
	case HAL_BIR_PINMUX_DEINIT:
		HAL_GPIO_PinMuxDeConfig(pinmux, count);
		break;
	default:
		return HAL_INVALID;
	}

	return HAL_OK;
}

void board_chip_clock_init(void)
{
	PRCM_LDO1Volt volt;
	PRCM_SysClkFactor factor = BOARD_CPU_CLK_FACTOR;

	if (BOARD_CPU_CLK_SRC == PRCM_CPU_CLK_SRC_SYSCLK) {
		uint32_t clkHz = HAL_PRCM_SysClkFactor2Hz(BOARD_CPU_CLK_FACTOR);
		if (clkHz <= (160 * 1000 * 1000)) {
			volt = PRCM_LDO1_VOLT_1225MV;
		} else if (clkHz <= (240 * 1000 * 1000)) {
			volt = PRCM_LDO1_VOLT_1225MV;
		} else {
#if SYS_AVS_EN
			volt = PRCM_LDO1_VOLT_1225MV;
			factor = PRCM_SYS_CLK_FACTOR_240M;
#else
            if (HAL_GlobalGetChipVer() >= 0xE) {
                volt = PRCM_LDO1_VOLT_1325MV;
            } else {
                volt = PRCM_LDO1_VOLT_1375MV;
            }
#endif
		}
	} else {
		volt = PRCM_LDO1_VOLT_1125MV;
	}
	HAL_PRCM_SetLDO1Volt(volt, PRCM_LDO1_RET_VOLT_725MV);

#if (__CONFIG_HOSC_TYPE == 24)
	HAL_PRCM_SetHOSCType(PRCM_HOSC_TYPE_24M);
	HAL_PRCM_SetSysPLL(PRCM_SYS_PLL_PARAM_HOSC24M);
#elif (__CONFIG_HOSC_TYPE == 26)
	HAL_PRCM_SetHOSCType(PRCM_HOSC_TYPE_26M);
	HAL_PRCM_SetSysPLL(PRCM_SYS_PLL_PARAM_HOSC26M);
#elif (__CONFIG_HOSC_TYPE == 40)
	HAL_PRCM_SetHOSCType(PRCM_HOSC_TYPE_40M);
	HAL_PRCM_SetSysPLL(PRCM_SYS_PLL_PARAM_HOSC40M);
#elif (__CONFIG_HOSC_TYPE == 52)
	HAL_PRCM_SetHOSCType(PRCM_HOSC_TYPE_52M);
	HAL_PRCM_SetSysPLL(PRCM_SYS_PLL_PARAM_HOSC52M);
#else
	#error "Invalid HOSC value!"
#endif

#if (defined(BOARD_LOSC_EXTERNAL) && BOARD_LOSC_EXTERNAL)
	HAL_PRCM_SetLFCLKSource(PRCM_LFCLK_SRC_EXT32K);
	HAL_PRCM_DisableInter32KCalib();
#else
	HAL_PRCM_SetLFCLKSource(PRCM_LFCLK_SRC_INTER32K);
	HAL_PRCM_EnableInter32KCalib();
#endif

	HAL_PRCM_SetCPUAClk(BOARD_CPU_CLK_SRC, factor);
	HAL_PRCM_SetDevClock(BOARD_DEV_CLK_FACTOR);
	HAL_CCM_BusSetClock(BOARD_AHB2_CLK_DIV, BOARD_APB_CLK_SRC, BOARD_APB_CLK_DIV);
#ifdef __CONFIG_PSRAM
	HAL_PRCM_SetDev2Clock(PRCM_DEV2_CLK_FACTOR_384M);
	HAL_PRCM_EnableDev2Clock();
#endif
#if (__CONFIG_CHIP_ARCH_VER == 2)
	HAL_CCM_BusSetAPBSClock(BOARD_APBS_CLK_SRC, BOARD_APBS_CLK_FACTOR);
#endif
}

static GPIO_PinMuxParam g_pinmux_flashc_sip[] = {
	{ GPIO_PORT_B, GPIO_PIN_10,  { GPIOB_P10_F2_FLASH_MOSI, GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_11,  { GPIOB_P11_F2_FLASH_MISO, GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_12,  { GPIOB_P12_F2_FLASH_CS,	GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP	 } },
	{ GPIO_PORT_B, GPIO_PIN_13,  { GPIOB_P13_F2_FLASH_CLK,	GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_8,	 { GPIOB_P8_F2_FLASH_WP,	GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP	 } },
	{ GPIO_PORT_B, GPIO_PIN_9,	 { GPIOB_P9_F2_FLASH_HOLD,	GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP	 } },
};

#ifdef __CONFIG_PSRAM
static GPIO_PinMuxParam g_pinmux_psram_sip[] = {
#if (defined __CONFIG_PSRAM_CHIP_SQPI)
	{ GPIO_PORT_C, GPIO_PIN_4,  { GPIOC_P4_F4_PSRAM_SIO3, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_5,  { GPIOC_P5_F4_PSRAM_CLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_7,  { GPIOC_P7_F4_PSRAM_CE,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_9,  { GPIOC_P9_F4_PSRAM_SIO1, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_10, { GPIOC_P10_F4_PSRAM_SIO2,GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_11, { GPIOC_P11_F4_PSRAM_SIO0,GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
#elif (defined __CONFIG_PSRAM_CHIP_OPI32)
	{ GPIO_PORT_C, GPIO_PIN_0,  { GPIOC_P0_F2_PSRAM_DM,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_1,  { GPIOC_P1_F2_PSRAM_DQ0,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_2,  { GPIOC_P2_F2_PSRAM_DQ1,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_3,  { GPIOC_P3_F2_PSRAM_DQ2,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_4,  { GPIOC_P4_F2_PSRAM_DQ3,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_5,  { GPIOC_P5_F2_PSRAM_CE,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_6,  { GPIOC_P6_F2_PSRAM_CLK_N, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_7,  { GPIOC_P7_F2_PSRAM_CLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_8,  { GPIOC_P8_F2_PSRAM_DQ4,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_9,  { GPIOC_P9_F2_PSRAM_DQ5,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_10, { GPIOC_P10_F2_PSRAM_DQ6, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_11, { GPIOC_P11_F2_PSRAM_DQ7, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_12, { GPIOC_P12_F2_PSRAM_DQS, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
#elif (defined __CONFIG_PSRAM_CHIP_OPI64)
	{ GPIO_PORT_C, GPIO_PIN_0,  { GPIOC_P0_F3_PSRAM_DQS,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_1,  { GPIOC_P1_F3_PSRAM_DQ7,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_2,  { GPIOC_P2_F3_PSRAM_DQ6,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_3,  { GPIOC_P3_F3_PSRAM_DQ5,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_4,  { GPIOC_P4_F3_PSRAM_DQ4,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_5,  { GPIOC_P5_F3_PSRAM_CLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_7,  { GPIOC_P7_F3_PSRAM_CE,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_8,  { GPIOC_P8_F3_PSRAM_DQ3,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_9,  { GPIOC_P9_F3_PSRAM_DQ2,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_10, { GPIOC_P10_F3_PSRAM_DQ1, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_C, GPIO_PIN_11, { GPIOC_P11_F3_PSRAM_DQ0, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
#endif
};
#endif

HAL_Status board_get_flashc_sip_pinmux_cfg(const GPIO_PinMuxParam **param,
                                           uint32_t *count)
{
	if (HAL_PRCM_IsFlashSip()) {
#if (__CONFIG_CHIP_ARCH_VER == 1)
		if (HAL_PRCM_GetFlashSipMode() == PRCM_FLASH_SIP_MODE0) {
#endif
			g_pinmux_flashc_sip[4].pin = GPIO_PIN_8; /* FLASH_WP */
			g_pinmux_flashc_sip[5].pin = GPIO_PIN_9; /* FLASH_HOLD */

#if (__CONFIG_CHIP_ARCH_VER == 1)
		} else {
			g_pinmux_flashc_sip[4].pin = GPIO_PIN_14; /* FLASH_WP */
			g_pinmux_flashc_sip[5].pin = GPIO_PIN_15; /* FLASH_HOLD */
		}
#endif
		*param = g_pinmux_flashc_sip;
		*count = HAL_ARRAY_SIZE(g_pinmux_flashc_sip);
		return HAL_OK;
	} else {
		return HAL_ERROR;
	}
}

#ifdef __CONFIG_PSRAM
HAL_Status board_get_psram_sip_pinmux_cfg(const GPIO_PinMuxParam **param,
                                          uint32_t *count)
{
	//if (HAL_PRCM_IsPsramSip()) {
		*param = g_pinmux_psram_sip;
		*count = HAL_ARRAY_SIZE(g_pinmux_psram_sip);
		return HAL_OK;
	//} else {
	//	return HAL_ERROR;
	//}
}
#endif
