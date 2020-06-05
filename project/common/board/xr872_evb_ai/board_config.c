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

#include <string.h>
#include "common/board/board_debug.h"
#include "common/board/board_common.h"
#include "board_config.h"
#include "driver/chip/hal_snd_card.h"
#include "common/apps/buttons/buttons.h"
#include "common/apps/buttons/buttons_low_level.h"
#include "common/apps/buttons/matrix_buttons_low_level.h"

/* Note: Default SWD pins are multiplexing with flash pins.
 *       Using/Enabling SWD may cause flash read/write error.
 */
#define BOARD_SWD_EN	PRJCONF_SWD_EN

static const GPIO_PinMuxParam g_pinmux_uart0[] = {
	{ GPIO_PORT_B, GPIO_PIN_0,  { GPIOB_P0_F2_UART0_TX,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* TX */
	{ GPIO_PORT_B, GPIO_PIN_1,  { GPIOB_P1_F2_UART0_RX,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* RX */
};

static const GPIO_PinMuxParam g_pinmux_uart1[] = {
	{ GPIO_PORT_A, GPIO_PIN_13, { GPIOA_P13_F5_UART1_TX,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* TX */
	{ GPIO_PORT_A, GPIO_PIN_14, { GPIOA_P14_F5_UART1_RX,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* RX */
	//{ GPIO_PORT_A, GPIO_PIN_15, { GPIOA_P15_F5_UART1_CTS,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* CTS */
	//{ GPIO_PORT_A, GPIO_PIN_16, { GPIOA_P16_F5_UART1_RTS,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* RTS */
};

static const GPIO_PinMuxParam g_pinmux_uart2[] = {
	{ GPIO_PORT_A, GPIO_PIN_21, { GPIOA_P21_F2_UART2_RX,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* TX */
	{ GPIO_PORT_A, GPIO_PIN_22, { GPIOA_P22_F2_UART2_TX,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* RX */
	//{ GPIO_PORT_A, GPIO_PIN_19, { GPIOA_P19_F2_UART2_CTS,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* CTS */
	//{ GPIO_PORT_A, GPIO_PIN_20, { GPIOA_P20_F2_UART2_RTS,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } }, /* RTS */
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_dmic[] = {
	{ GPIO_PORT_A, GPIO_PIN_21, { GPIOA_P21_F3_DMIC_CLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_22, { GPIOA_P22_F3_DMIC_DATA, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_i2s[] = {
	{ GPIO_PORT_A, GPIO_PIN_12, { GPIOA_P12_F4_I2S_MCLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_13, { GPIOA_P13_F4_I2S_BCLK,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_14, { GPIOA_P14_F4_I2S_DI,    GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_15, { GPIOA_P15_F4_I2S_DO,    GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_16, { GPIOA_P16_F4_I2S_LRCLK, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_irrx[] = {
	{ GPIO_PORT_A, GPIO_PIN_17, { GPIOA_P17_F3_IR_RX,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_irtx[] = {
	{ GPIO_PORT_A, GPIO_PIN_18, { GPIOA_P18_F3_IR_TX,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_DOWN } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_i2c0[] = {
	{ GPIO_PORT_A, GPIO_PIN_19,  { GPIOA_P19_F3_I2C0_SCL,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_20,  { GPIOA_P20_F3_I2C0_SDA,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_i2c1[] = {
	{ GPIO_PORT_A, GPIO_PIN_8, { GPIOA_P8_F4_I2C1_SCL,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
	{ GPIO_PORT_A, GPIO_PIN_9, { GPIOA_P9_F4_I2C1_SDA,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_adc[] = {
	{ GPIO_PORT_A, GPIO_PIN_10,  { GPIOA_P10_F2_ADC_CH0,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_11,  { GPIOA_P11_F2_ADC_CH1,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_12, { GPIOA_P12_F2_ADC_CH2, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_13, { GPIOA_P13_F2_ADC_CH3, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_14, { GPIOA_P14_F2_ADC_CH4, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_15, { GPIOA_P15_F2_ADC_CH5, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_16, { GPIOA_P16_F2_ADC_CH6, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi0[] = {
	{ GPIO_PORT_B, GPIO_PIN_4,  { GPIOB_P4_F2_SPI0_MOSI,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_5,  { GPIOB_P5_F2_SPI0_MISO,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_7,  { GPIOB_P7_F2_SPI0_CLK,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi0_cs0[] = {
	{ GPIO_PORT_B, GPIO_PIN_6,  { GPIOB_P6_F2_SPI0_CS0,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi1[] = {
	{ GPIO_PORT_A, GPIO_PIN_19, { GPIOA_P19_F5_SPI1_MOSI,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_21, { GPIOA_P21_F5_SPI1_CLK,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi1_cs0[] = {
	{ GPIO_PORT_A, GPIO_PIN_22, { GPIOA_P22_F5_SPI1_CS0,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi1_cs1[] = {
	{ GPIO_PORT_A, GPIO_PIN_6,  { GPIOA_P6_F3_SPI1_CS1,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_spi1_cs2[] = {
	{ GPIO_PORT_A, GPIO_PIN_7,  { GPIOA_P7_F3_SPI1_CS2,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP } },
};

static const GPIO_PinMuxParam g_pinmux_flashc[] = {
	{ GPIO_PORT_B, GPIO_PIN_4,  { GPIOB_P4_F5_FLASH_MOSI, GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_5,  { GPIOB_P5_F5_FLASH_MISO, GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
	{ GPIO_PORT_B, GPIO_PIN_6,  { GPIOB_P6_F5_FLASH_CS,   GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP   } },
	{ GPIO_PORT_B, GPIO_PIN_7,  { GPIOB_P7_F5_FLASH_CLK,  GPIO_DRIVING_LEVEL_3, GPIO_PULL_NONE } },
#if (!BOARD_SWD_EN)
	{ GPIO_PORT_B, GPIO_PIN_2,  { GPIOB_P2_F5_FLASH_WP,   GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP   } },
	{ GPIO_PORT_B, GPIO_PIN_3,  { GPIOB_P3_F5_FLASH_HOLD, GPIO_DRIVING_LEVEL_3, GPIO_PULL_UP   } },
#endif
};

#if BOARD_SWD_EN
static const GPIO_PinMuxParam g_pinmux_swd[] = {
	{ GPIO_PORT_B, GPIO_PIN_2,  { GPIOB_P2_F2_SWD_TMS,    GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP   } },
	{ GPIO_PORT_B, GPIO_PIN_3,  { GPIOB_P3_F2_SWD_TCK,    GPIO_DRIVING_LEVEL_1, GPIO_PULL_UP   } },
};
#endif

/* flash */
static const FlashBoardCfg g_flash_cfg[] = {
	{
		.type = FLASH_DRV_FLASHC,
		.mode = FLASH_READ_DUAL_O_MODE,
		.flashc.clk = (96 * 1000 * 1000),
	},
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_pwm[] = {
	{ GPIO_PORT_A, GPIO_PIN_8,  { GPIOA_P8_F3_PWM0_ECT0,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_9,  { GPIOA_P9_F3_PWM1_ECT1,  GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_10, { GPIOA_P10_F3_PWM2_ECT2, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_11, { GPIOA_P11_F3_PWM3_ECT3, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_12, { GPIOA_P12_F3_PWM4_ECT4, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_13, { GPIOA_P13_F3_PWM5_ECT5, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_14, { GPIOA_P14_F3_PWM6_ECT6, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_15, { GPIOA_P15_F3_PWM7_ECT7, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

#define BOARD_SD0_DATA_BITS   	1
#define BOARD_SD0_DET_VALID   	0
#define BOARD_SD0_DET_PORT    	GPIO_PORT_A
#define BOARD_SD0_DET_PIN     	GPIO_PIN_3
#define BOARD_SD0_DET_PIN_MODE	GPIOx_Pn_F6_EINT
#define BOARD_SD0_DET_DELAY    	500

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_sd0[BOARD_SD0_DATA_BITS + 2] = {
	{ GPIO_PORT_B, GPIO_PIN_16,  { GPIOB_P16_F3_SD_CMD,     GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* CMD */
	{ GPIO_PORT_B, GPIO_PIN_18,  { GPIOB_P18_F3_SD_CLK,     GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* CLK */
	{ GPIO_PORT_B, GPIO_PIN_17,  { GPIOB_P17_F3_SD_DATA0,   GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* D0 */
//	{ GPIO_PORT_A, GPIO_PIN_3,  { GPIOA_P3_F3_SD_DATA1,   GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* D1 */
//	{ GPIO_PORT_A, GPIO_PIN_4,  { GPIOA_P4_F3_SD_DATA2,   GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* D2 */
//	{ GPIO_PORT_A, GPIO_PIN_5,  { GPIOA_P5_F3_SD_DATA3,   GPIO_DRIVING_LEVEL_2, GPIO_PULL_UP } },	/* D3 */
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_sd0_det[] = {
	{ BOARD_SD0_DET_PORT, BOARD_SD0_DET_PIN, { BOARD_SD0_DET_PIN_MODE,  GPIO_DRIVING_LEVEL_2, GPIO_PULL_NONE } },	/* DET */
};

__xip_rodata
static const HAL_SDCGPIOCfg g_sd0_cfg = {
	.data_bits       = BOARD_SD0_DATA_BITS,
	.has_detect_gpio = BOARD_SD0_DET_VALID,
	.detect_port     = BOARD_SD0_DET_PORT,
	.detect_pin      = BOARD_SD0_DET_PIN,
	.detect_pin_present_val = GPIO_PIN_LOW,
	.detect_delay    = BOARD_SD0_DET_DELAY
};

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_csi[] = {
	{ GPIO_PORT_A, GPIO_PIN_0,  { GPIOA_P0_F5_CSI_D0,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_1,  { GPIOA_P1_F5_CSI_D1,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_2,  { GPIOA_P2_F5_CSI_D2,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_3,  { GPIOA_P3_F5_CSI_D3,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_4,  { GPIOA_P4_F5_CSI_D4,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_5,  { GPIOA_P5_F5_CSI_D5,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_6,  { GPIOA_P6_F5_CSI_D6,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_7,  { GPIOA_P7_F5_CSI_D7,     GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_8,  { GPIOA_P8_F5_CSI_PCLK,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_9,  { GPIOA_P9_F5_CSI_MCLK,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_10, { GPIOA_P10_F5_CSI_HSYNC, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
	{ GPIO_PORT_A, GPIO_PIN_11, { GPIOA_P11_F5_CSI_VSYNC, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

/* do not set const */
static ad_button g_ad_buttons[] = {
	{.name = "play+pause",    .mask = KEY1, .channel = ADC_CHANNEL_6, .value = 704,  .debounce_time = 50},
	{.name = "pre+vol_up",    .mask = KEY2, .channel = ADC_CHANNEL_6, .value = 1343, .debounce_time = 50},
	{.name = "next+vol_down", .mask = KEY3, .channel = ADC_CHANNEL_6, .value = 1933, .debounce_time = 50},
	{.name = "mode",          .mask = KEY4, .channel = ADC_CHANNEL_6, .value = 2572, .debounce_time = 50},
	{.name = "ai",            .mask = KEY5, .channel = ADC_CHANNEL_6, .value = 3244, .debounce_time = 50},
	{.name = "pre+next",      .mask = KEY2 | KEY3, .channel = ADC_CHANNEL_6, .value = 940,  .debounce_time = 50},
};

__xip_rodata
static const gpio_button g_gpio_buttons[] = {
};

__xip_rodata
static const matrix_button g_matrix_buttons_row[] = {
	{.name = "row1", { GPIO_PORT_A, GPIO_PIN_0,	{ GPIOx_Pn_F1_OUTPUT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE }}, 0, 0, 0},
	{.name = "row2", { GPIO_PORT_A, GPIO_PIN_1, { GPIOx_Pn_F1_OUTPUT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE }}, 0, 0, 0},
	{.name = "row3", { GPIO_PORT_A, GPIO_PIN_2, { GPIOx_Pn_F1_OUTPUT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE }}, 0, 0, 0},
	{.name = "row4", { GPIO_PORT_A, GPIO_PIN_3, { GPIOx_Pn_F1_OUTPUT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE }}, 0, 0, 0},
};

__xip_rodata
static const matrix_button g_matrix_buttons_col[] = {
	{.name = "col1", { GPIO_PORT_A, GPIO_PIN_4, { GPIOx_Pn_F6_EINT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_DOWN }}, 1, WKUPIO_WK_MODE_RISING_EDGE, GPIO_PULL_DOWN},
	{.name = "col2", { GPIO_PORT_A, GPIO_PIN_5, { GPIOx_Pn_F6_EINT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_DOWN }}, 1, WKUPIO_WK_MODE_RISING_EDGE, GPIO_PULL_DOWN},
	{.name = "col3", { GPIO_PORT_A, GPIO_PIN_6, { GPIOx_Pn_F6_EINT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_DOWN }}, 1, WKUPIO_WK_MODE_RISING_EDGE, GPIO_PULL_DOWN},
	{.name = "col4", { GPIO_PORT_A, GPIO_PIN_7, { GPIOx_Pn_F6_EINT, GPIO_DRIVING_LEVEL_1, GPIO_PULL_DOWN }}, 1, WKUPIO_WK_MODE_RISING_EDGE, GPIO_PULL_DOWN},
};

#define BOARD_PA_PORT           	GPIO_PORT_A
#define BOARD_PA_PIN            	GPIO_PIN_23
#define BOARD_PA_ON_DELAY_BEFORE	5
#define BOARD_PA_ON_DELAY_AFTER		100

__xip_rodata
static const GPIO_PinMuxParam g_pinmux_pa_switch[] = {
	{ BOARD_PA_PORT, BOARD_PA_PIN, { GPIOx_Pn_F1_OUTPUT,   GPIO_DRIVING_LEVEL_1, GPIO_PULL_NONE } },
};

__xip_rodata
static const Pa_Switch_Ctl pa_switch_ctl = {
	.on_state  = GPIO_PIN_HIGH,
	.on_delay_before = BOARD_PA_ON_DELAY_BEFORE,
	.on_delay_after  = BOARD_PA_ON_DELAY_AFTER,
	.pin_param = g_pinmux_pa_switch,
	.pin_param_cnt = HAL_ARRAY_SIZE(g_pinmux_pa_switch),
};

#if PRJCONF_INTERNAL_SOUNDCARD_EN
__xip_rodata const static struct snd_card_board_config xradio_internal_codec_snd_card = {
	.card_num = SND_CARD_0,
	.card_name = HAL_SND_CARD_NAME(XRADIO_INTERNAL_CODEC_NAME, SND_CARD_SUFFIX),
	.codec_link = XRADIO_CODEC_INTERNAL,
	.platform_link = XRADIO_PLATFORM_NULL,

	.pa_switch_ctl = &pa_switch_ctl,

	.codec_sysclk_src = SYSCLK_SRC_PLL,
	.codec_pllclk_src = 0,	//xradio_internal_codec not use
	.codec_pll_freq_in = 0,	//xradio_internal_codec not use
	.i2s_fmt = 0,			//xradio_internal_codec not use
};
#endif

#if PRJCONF_AC107_SOUNDCARD_EN
__xip_rodata const static struct snd_card_board_config ac107_codec_snd_card = {
	.card_num = SND_CARD_1,
	.card_name = HAL_SND_CARD_NAME(AC107_CODEC_NAME, SND_CARD_SUFFIX),
	.codec_link = XRADIO_CODEC_AC107,
	.platform_link = XRADIO_PLATFORM_I2S,

	.pa_switch_ctl = NULL,

	.codec_sysclk_src = SYSCLK_SRC_MCLK,
	.codec_pllclk_src = 0,
	.codec_pll_freq_in = 0,
	.i2s_fmt = DAIFMT_CBS_CFS | DAIFMT_I2S | DAIFMT_NB_NF,
};
#endif

#if PRJCONF_AC101_SOUNDCARD_EN
__xip_rodata const static struct snd_card_board_config ac101_codec_snd_card = {
	.card_num = SND_CARD_2,
	.card_name = HAL_SND_CARD_NAME(AC101_CODEC_NAME, SND_CARD_SUFFIX),
	.codec_link = XRADIO_CODEC_AC101,
	.platform_link = XRADIO_PLATFORM_I2S,

	.pa_switch_ctl = &pa_switch_ctl,

	.codec_sysclk_src = SYSCLK_SRC_MCLK,
	.codec_pllclk_src = 0,
	.codec_pll_freq_in = 0,
	.i2s_fmt = DAIFMT_CBS_CFS | DAIFMT_I2S | DAIFMT_NB_NF,
};
#endif

#if PRJCONF_I2S_NULL_SOUNDCARD_EN
__xip_rodata const static struct snd_card_board_config xradio_i2s_null_snd_card = {
	.card_num = SND_CARD_3,
	.card_name = HAL_SND_CARD_NAME(XRADIO_CODEC_NULL_NAME, SND_CARD_SUFFIX),
	.codec_link = XRADIO_CODEC_NULL,
	.platform_link = XRADIO_PLATFORM_I2S,

	.pa_switch_ctl = NULL,

	.codec_sysclk_src = 0,
	.codec_pllclk_src = 0,
	.codec_pll_freq_in = 0,
	.i2s_fmt = DAIFMT_CBS_CFS | DAIFMT_I2S | DAIFMT_NB_NF,
};
#endif

const static struct snd_card_board_config *snd_cards_board_cfg[] = {
#if PRJCONF_INTERNAL_SOUNDCARD_EN
	&xradio_internal_codec_snd_card,
#endif

#if PRJCONF_AC107_SOUNDCARD_EN
	&ac107_codec_snd_card,
#endif

#if PRJCONF_AC101_SOUNDCARD_EN
	&ac101_codec_snd_card,
#endif

#if PRJCONF_I2S_NULL_SOUNDCARD_EN
	&xradio_i2s_null_snd_card,
#endif
};

struct board_pinmux_info {
	const GPIO_PinMuxParam *pinmux;
	uint32_t count;
};

#define BOARD_PINMUX_INFO_MAX	2

static HAL_Status board_get_pinmux_info(uint32_t major, uint32_t minor, uint32_t param,
                                        struct board_pinmux_info info[])
{
	uint8_t i;
	HAL_Status ret = HAL_OK;

	switch (major) {
	case HAL_DEV_MAJOR_UART:
		if (minor == UART0_ID) {
			info[0].pinmux = g_pinmux_uart0;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_uart0);
		} else if (minor == UART1_ID) {
			info[0].pinmux = g_pinmux_uart1;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_uart1);
		} else if (minor == UART2_ID) {
			info[0].pinmux = g_pinmux_uart2;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_uart2);
		} else {
			ret = HAL_INVALID;
		}
		break;
	case HAL_DEV_MAJOR_I2C:
		if (minor == I2C0_ID) {
			info[0].pinmux = g_pinmux_i2c0;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_i2c0);
		} else if (minor == I2C1_ID) {
			info[0].pinmux = g_pinmux_i2c1;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_i2c1);
		} else {
			ret = HAL_INVALID;
		}
		break;
	case HAL_DEV_MAJOR_SPI:
		if (minor == SPI0) {
			info[0].pinmux = g_pinmux_spi0;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_spi0);
			info[1].pinmux = g_pinmux_spi0_cs0;
			info[1].count = HAL_ARRAY_SIZE(g_pinmux_spi0_cs0);
		} else if (minor == SPI1) {
			info[0].pinmux = g_pinmux_spi1;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_spi1);
			switch (param) {
			case SPI_TCTRL_SS_SEL_SS0:
				info[1].pinmux = g_pinmux_spi1_cs0;
				info[1].count = HAL_ARRAY_SIZE(g_pinmux_spi1_cs0);
				break;
			case SPI_TCTRL_SS_SEL_SS1:
				info[1].pinmux = g_pinmux_spi1_cs1;
				info[1].count = HAL_ARRAY_SIZE(g_pinmux_spi1_cs1);
				break;
			case SPI_TCTRL_SS_SEL_SS2:
				info[1].pinmux = g_pinmux_spi1_cs2;
				info[1].count = HAL_ARRAY_SIZE(g_pinmux_spi1_cs2);
				break;
			default:
				ret = HAL_INVALID;
				break;
			}
		} else {
			ret = HAL_INVALID;
		}
		break;
	case HAL_DEV_MAJOR_IRRX:
		info[0].pinmux = g_pinmux_irrx;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_irrx);
		break;
	case HAL_DEV_MAJOR_IRTX:
		info[0].pinmux = g_pinmux_irtx;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_irtx);
		break;
	case HAL_DEV_MAJOR_I2S:
		info[0].pinmux = g_pinmux_i2s;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_i2s);
		break;
	case HAL_DEV_MAJOR_DMIC:
		info[0].pinmux = g_pinmux_dmic;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_dmic);
		break;
	case HAL_DEV_MAJOR_ADC:
		if (minor < HAL_ARRAY_SIZE(g_pinmux_adc)) {
			info[0].pinmux = &g_pinmux_adc[minor];
			info[0].count = 1;
		} else {
			ret = HAL_INVALID;
		}
		break;
	case HAL_DEV_MAJOR_PWM:
		if (minor < HAL_ARRAY_SIZE(g_pinmux_pwm)) {
			info[0].pinmux = &g_pinmux_pwm[minor];
			info[0].count = 1;
		} else if (minor != ADC_CHANNEL_VBAT) {
			ret = HAL_INVALID;
		}
		break;
	case HAL_DEV_MAJOR_FLASHC:
		if (board_get_flashc_sip_pinmux_cfg(&info[0].pinmux,
		                                    &info[0].count) != HAL_OK) {
			info[0].pinmux = g_pinmux_flashc;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_flashc);
		}
		break;
#ifdef __CONFIG_PSRAM
	case HAL_DEV_MAJOR_PSRAM:
		board_get_psram_sip_pinmux_cfg(&info[0].pinmux, &info[0].count);
		break;
#endif
	case HAL_DEV_MAJOR_SDC:
		if (minor == SDC0) {
			info[0].pinmux = g_pinmux_sd0;
			info[0].count = HAL_ARRAY_SIZE(g_pinmux_sd0);
			info[1].pinmux = g_pinmux_sd0_det;
			info[1].count = 1;
		} else if (minor == SDC1) {
			info[0].pinmux = NULL;
			info[0].count = 0;
			info[1].pinmux = NULL;
			info[1].count = 0;
		}
		break;
	case HAL_DEV_MAJOR_CSI:
		info[0].pinmux = g_pinmux_csi;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_csi);
		break;
	case HAL_DEV_MAJOR_AUDIO_CODEC:
		for(i=0; i<HAL_ARRAY_SIZE(snd_cards_board_cfg); i++){
			if(snd_cards_board_cfg[i]->card_num == minor){
				if(snd_cards_board_cfg[i]->pa_switch_ctl){
					info[0].pinmux = snd_cards_board_cfg[i]->pa_switch_ctl->pin_param;
					info[0].count  = snd_cards_board_cfg[i]->pa_switch_ctl->pin_param_cnt;
				}
			}
		}
		break;
#if BOARD_SWD_EN
	case HAL_DEV_MAJOR_SWD:
		info[0].pinmux = g_pinmux_swd;
		info[0].count = HAL_ARRAY_SIZE(g_pinmux_swd);
		break;
#endif
	default:
		BOARD_ERR("unknow major %u\n", major);
		ret = HAL_INVALID;
	}

	return ret;
}

static HAL_Status board_get_cfg(uint32_t major, uint32_t minor, uint32_t param)
{
	uint8_t i;
	HAL_Status ret = HAL_OK;

	switch (major) {
	case HAL_DEV_MAJOR_SDC:
		if (minor == SDC0) {
			*((HAL_SDCGPIOCfg **)param) = (HAL_SDCGPIOCfg *)&g_sd0_cfg;
		} else if (minor == SDC1) {
			*((HAL_SDCGPIOCfg **)param) = NULL;
		}
		break;
	case HAL_DEV_MAJOR_FLASH:
		if (minor <= (sizeof(g_flash_cfg) / sizeof(FlashBoardCfg)))
			*((FlashBoardCfg **)param) = (FlashBoardCfg *)&g_flash_cfg[minor];
		else
			*((FlashBoardCfg **)param) = NULL;
		break;
	case HAL_DEV_MAJOR_AUDIO_CODEC:
		for(i=0; i<HAL_ARRAY_SIZE(snd_cards_board_cfg); i++){
			if(snd_cards_board_cfg[i]->card_num == minor){
				*((const struct snd_card_board_config **)param) = snd_cards_board_cfg[i];
			}
		}
		break;
	case HAL_DEV_MAJOR_AD_BUTTON:
		((ad_button_info *)param)->ad_buttons_p = g_ad_buttons;
		((ad_button_info *)param)->count = sizeof(g_ad_buttons) / sizeof(g_ad_buttons[0]);
		break;
	case HAL_DEV_MAJOR_GPIO_BUTTON:
		((gpio_button_info *)param)->gpio_buttons_p = g_gpio_buttons;
		((gpio_button_info *)param)->count = sizeof(g_gpio_buttons) / sizeof(g_gpio_buttons[0]);
		break;
	case HAL_DEV_MAJOR_MATRIX_BUTTON:
		((matrix_button_info *)param)->matrix_buttons_row_p = g_matrix_buttons_row;
		((matrix_button_info *)param)->count_row = sizeof(g_matrix_buttons_row) / sizeof(g_matrix_buttons_row[0]);
		((matrix_button_info *)param)->matrix_buttons_col_p = g_matrix_buttons_col;
		((matrix_button_info *)param)->count_col = sizeof(g_matrix_buttons_col) / sizeof(g_matrix_buttons_col[0]);
		break;
	default:
		BOARD_ERR("unknow major %u\n", major);
		ret = HAL_INVALID;
	}

	return ret;
}

#ifdef __CONFIG_PM
void board_flash_pinmux_deinit(const GPIO_PinMuxParam *pinmux, uint32_t count)
{
	uint32_t i;

	for (i = 0; i < count; ++i) {
		/* reset driving and mode, but keep pull to avoid current leakage */
		HAL_GPIO_SetDriving(pinmux[i].port, pinmux[i].pin, GPIO_DRIVING_LEVEL_1);
		HAL_GPIO_SetMode(pinmux[i].port, pinmux[i].pin, GPIOx_Pn_F7_DISABLE);
	}
}
#endif

HAL_Status board_ioctl(HAL_BoardIoctlReq req, uint32_t param0, uint32_t param1)
{
	HAL_Status ret = HAL_OK;
	uint32_t major, minor, i;
	struct board_pinmux_info info[BOARD_PINMUX_INFO_MAX];

	switch (req) {
	case HAL_BIR_PINMUX_INIT:
	case HAL_BIR_PINMUX_DEINIT:
		memset(info, 0, sizeof(info));
		major = HAL_DEV_MAJOR((HAL_Dev_t)param0);
		minor = HAL_DEV_MINOR((HAL_Dev_t)param0);
		ret = board_get_pinmux_info(major, minor, param1, info);
		if (major == HAL_DEV_MAJOR_SDC) {
			if (minor == SDC0) {
				if (param1 == SDCGPIO_BAS) {
					board_pinmux_cfg(req, info[0].pinmux, info[0].count);
				} else if (param1 == SDCGPIO_DET) {
					board_pinmux_cfg(req, info[1].pinmux, info[1].count);
				}
			} else if (minor == SDC1) {
				; /* do nothing */
			}
#ifdef __CONFIG_PM
		} else if (major == HAL_DEV_MAJOR_FLASHC && req == HAL_BIR_PINMUX_DEINIT) {
			for (i = 0; i < BOARD_PINMUX_INFO_MAX; ++i) {
				if (info[i].pinmux != NULL && info[i].count != 0) {
					board_flash_pinmux_deinit(info[i].pinmux, info[i].count);
				} else {
					break;
				}
			}
#endif
		} else {
			for (i = 0; i < BOARD_PINMUX_INFO_MAX; ++i) {
				if (info[i].pinmux != NULL && info[i].count != 0) {
					board_pinmux_cfg(req, info[i].pinmux, info[i].count);
				} else {
					break;
				}
			}
		}
		break;
	case HAL_BIR_CHIP_CLOCK_INIT:
		board_chip_clock_init();
		break;
	case HAL_BIR_GET_CFG:
		major = HAL_DEV_MAJOR((HAL_Dev_t)param0);
		minor = HAL_DEV_MINOR((HAL_Dev_t)param0);
		ret = board_get_cfg(major, minor, param1);
		break;
	default:
		BOARD_ERR("req %d not suppport\n", req);
		ret = HAL_INVALID;
		break;
	}

	if (ret != HAL_OK) {
		BOARD_ERR("req %d, param0 %#x, param1 %#x, ret %d\n", req, param0, param1, ret);
	}

	return ret;
}
