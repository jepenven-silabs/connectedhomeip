/***************************************************************************/
/**
 * @file
 * @brief RGB LED driver for BRD4166A
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc.
 *www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon
 *Laboratories Inc. Your use of this software is
 *governed by the terms of Silicon Labs Master
 *Software License Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement.
 *This software is distributed to you in Source Code
 *format and is governed by the sections of the MSLA
 *applicable to Source Code.
 *
 ******************************************************************************/

#include "rgbled.h"
#include "em_gpio.h"
#include "sl_simple_rgbw_pwm_led.h"

// -----------------------------------------------------------------------------
// Private variables

//  Array to linearize the light level of the RGB LEDs
static const uint8_t light_levels[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x08, 0x08, 0x08, 0x09, 0x09,
    0x0A, 0x0A, 0x0B, 0x0B, 0x0C, 0x0C, 0x0D, 0x0D, 0x0E, 0x0F, 0x0F, 0x10, 0x11, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1F, 0x20, 0x21, 0x23, 0x24, 0x26, 0x27, 0x29, 0x2B, 0x2C, 0x2E, 0x30, 0x32, 0x34,
    0x36, 0x38, 0x3A, 0x3C, 0x3E, 0x40, 0x43, 0x45, 0x47, 0x4A, 0x4C, 0x4F, 0x51, 0x54, 0x57, 0x59, 0x5C, 0x5F, 0x62, 0x64,
    0x67, 0x6A, 0x6D, 0x70, 0x73, 0x76, 0x79, 0x7C, 0x7F, 0x82, 0x85, 0x88, 0x8B, 0x8E, 0x91, 0x94, 0x97, 0x9A, 0x9C, 0x9F,
    0xA2, 0xA5, 0xA7, 0xAA, 0xAD, 0xAF, 0xB2, 0xB4, 0xB7, 0xB9, 0xBB, 0xBE, 0xC0, 0xC2, 0xC4, 0xC6, 0xC8, 0xCA, 0xCC, 0xCE,
    0xD0, 0xD2, 0xD3, 0xD5, 0xD7, 0xD8, 0xDA, 0xDB, 0xDD, 0xDE, 0xDF, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9,
    0xEA, 0xEB, 0xEC, 0xED, 0xED, 0xEE, 0xEF, 0xEF, 0xF0, 0xF1, 0xF1, 0xF2, 0xF2, 0xF3, 0xF3, 0xF4, 0xF4, 0xF5, 0xF5, 0xF6,
    0xF6, 0xF6, 0xF7, 0xF7, 0xF7, 0xF8, 0xF8, 0xF8, 0xF9, 0xF9, 0xF9, 0xF9, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFB, 0xFB, 0xFB,
    0xFB, 0xFB, 0xFB, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD,
    0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFD, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF
};

sl_simple_rgbw_pwm_led_context_t simple_rgbw_pwm_led_rgb_context = {
    .port[SL_SIMPLE_RGBW_PWM_LED_COLOR_R]     = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_PORT,
    .pin[SL_SIMPLE_RGBW_PWM_LED_COLOR_R]      = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_PIN,
    .polarity[SL_SIMPLE_RGBW_PWM_LED_COLOR_R] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_POLARITY,
    .channel[SL_SIMPLE_RGBW_PWM_LED_COLOR_R]  = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_CHANNEL,
#if defined(SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_LOC)
    .location[SL_SIMPLE_RGBW_PWM_LED_COLOR_R] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_LOC,
#endif
    .port[SL_SIMPLE_RGBW_PWM_LED_COLOR_G]     = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_PORT,
    .pin[SL_SIMPLE_RGBW_PWM_LED_COLOR_G]      = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_PIN,
    .polarity[SL_SIMPLE_RGBW_PWM_LED_COLOR_G] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_POLARITY,
    .channel[SL_SIMPLE_RGBW_PWM_LED_COLOR_G]  = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_CHANNEL,
#if defined(SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_LOC)
    .location[SL_SIMPLE_RGBW_PWM_LED_COLOR_G] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_LOC,
#endif
    .port[SL_SIMPLE_RGBW_PWM_LED_COLOR_B]     = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_PORT,
    .pin[SL_SIMPLE_RGBW_PWM_LED_COLOR_B]      = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_PIN,
    .polarity[SL_SIMPLE_RGBW_PWM_LED_COLOR_B] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_POLARITY,
    .channel[SL_SIMPLE_RGBW_PWM_LED_COLOR_B]  = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_CHANNEL,
#if defined(SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_LOC)
    .location[SL_SIMPLE_RGBW_PWM_LED_COLOR_B] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_LOC,
#endif
    .port[SL_SIMPLE_RGBW_PWM_LED_COLOR_W]     = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_PORT,
    .pin[SL_SIMPLE_RGBW_PWM_LED_COLOR_W]      = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_PIN,
    .polarity[SL_SIMPLE_RGBW_PWM_LED_COLOR_W] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_POLARITY,
    .channel[SL_SIMPLE_RGBW_PWM_LED_COLOR_W]  = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_CHANNEL,
#if defined(SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_LOC)
    .location[SL_SIMPLE_RGBW_PWM_LED_COLOR_W] = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_LOC,
#endif
    .timer      = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_PERIPHERAL,
    .frequency  = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_FREQUENCY,
    .resolution = SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RESOLUTION,
};

const sl_led_rgbw_pwm_t sl_led_rgb = {
    .led_common.context   = &simple_rgbw_pwm_led_rgb_context,
    .led_common.init      = sl_simple_rgbw_pwm_led_init,
    .led_common.turn_on   = sl_simple_rgbw_pwm_led_turn_on,
    .led_common.turn_off  = sl_simple_rgbw_pwm_led_turn_off,
    .led_common.toggle    = sl_simple_rgbw_pwm_led_toggle,
    .led_common.get_state = sl_simple_rgbw_pwm_led_get_state,
    .set_rgbw_color       = sl_simple_rgbw_pwm_led_set_color,
    .get_rgbw_color       = sl_simple_rgbw_pwm_led_get_color,
};

void sl_simple_rgbw_pwm_led_init_instances(void)
{

    sl_led_init((sl_led_t *) &sl_led_rgb);
}

// -----------------------------------------------------------------------------
// Private function definitions

static void rgb_led_enable(bool enable, uint8_t mask)
{
    if (((mask != 0) && (enable == true)) || (((mask & 0xf) == 0xf) && (enable == false)))
    {
        if (enable)
        {
            GPIO_PinOutSet(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN);
        }
        else
        {
            GPIO_PinOutClear(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN);
        }
    }

    int i;
    uint8_t pins[4] = { BOARD_RGBLED_COM0_PIN, BOARD_RGBLED_COM1_PIN, BOARD_RGBLED_COM2_PIN, BOARD_RGBLED_COM3_PIN };

    for (i = 0; i < 4; i++)
    {
        if (((mask >> i) & 1) == 1)
        {
            if (enable)
            {
                GPIO_PinOutSet(BOARD_RGBLED_COM_PORT, pins[3 - i]);
            }
            else
            {
                GPIO_PinOutClear(BOARD_RGBLED_COM_PORT, pins[3 - i]);
            }
        }
    }

    return;
}

// -----------------------------------------------------------------------------
// Public function definitions

void rgb_led_init(void)
{
    GPIO_PinModeSet(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BOARD_RGBLED_COM0_PORT, BOARD_RGBLED_COM0_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BOARD_RGBLED_COM1_PORT, BOARD_RGBLED_COM1_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BOARD_RGBLED_COM2_PORT, BOARD_RGBLED_COM2_PIN, gpioModePushPull, 0);
    GPIO_PinModeSet(BOARD_RGBLED_COM3_PORT, BOARD_RGBLED_COM3_PIN, gpioModePushPull, 0);
    sl_simple_rgbw_pwm_led_init_instances();
}

void rgb_led_deinit(void)
{
    rgb_led_enable(false, 0xf);
}

void rgb_led_set(uint8_t m, uint8_t r, uint8_t g, uint8_t b)
{
    rgb_led_enable(false, ~m);
    rgb_led_enable(true, m);
    sl_led_set_rgbw_color(&sl_led_rgb, light_levels[r], light_levels[g], light_levels[b], 0);
}
