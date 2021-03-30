/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#pragma once
#ifdef __cplusplus
extern "C" {
#endif
#ifdef HAS_RGB_LED
#include "sl_simple_rgbw_pwm_led.h"



extern const sl_led_rgbw_pwm_t sl_led_rgb;


void sl_simple_rgbw_pwm_led_init_instances(void);

#define BOARD_RGBLED_PRESENT       1              /**< RGB LED present on board        */
#define BOARD_RGBLED_PWR_EN_PORT   gpioPortJ      /**< RGB LED Power Enable port       */
#define BOARD_RGBLED_PWR_EN_PIN    14             /**< RGB LED Power Enable pin        */
#define BOARD_RGBLED_COM_PORT      gpioPortI      /**< RGB LED COM port                */
#define BOARD_RGBLED_COM0_PORT     gpioPortI      /**< RGB LED COM0 port               */
#define BOARD_RGBLED_COM0_PIN      0              /**< RGB LED COM0 pin                */
#define BOARD_RGBLED_COM1_PORT     gpioPortI      /**< RGB LED COM1 port               */
#define BOARD_RGBLED_COM1_PIN      1              /**< RGB LED COM1 pin                */
#define BOARD_RGBLED_COM2_PORT     gpioPortI      /**< RGB LED COM2 port               */
#define BOARD_RGBLED_COM2_PIN      2              /**< RGB LED COM2 pin                */
#define BOARD_RGBLED_COM3_PORT     gpioPortI      /**< RGB LED COM3 port               */
#define BOARD_RGBLED_COM3_PIN      3              /**< RGB LED COM3 pin                */

// <h>Simple RGBW PWM LED Configuration
// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_FREQUENCY> PWM frequency [Hz]
// <i> Sets the frequency of the PWM signal
// <i> 0 = Don't care
// <i> Default: 10000
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_FREQUENCY      10000

// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RESOLUTION> PWM resolution <2-65536>
// <i> Specifies the PWM (dimming) resolution. I.e. if you want a
// <i> dimming resolution that takes the input values from 0 to 99,
// <i> set this value to 100
// <i> Default: 256
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RESOLUTION     256

// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_POLARITY> Red LED Polarity
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_POLARITY   SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH

// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_POLARITY> Green LED Polarity
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_POLARITY SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH

// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_POLARITY> Blue LED Polarity
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_POLARITY  SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH

// <o SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_POLARITY> White LED Polarity
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_LOW=> Active low
// <SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH=> Active high
// <i> Default: SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_POLARITY SL_SIMPLE_RGBW_PWM_LED_POLARITY_ACTIVE_HIGH
// </h> end led configuration

// <<< end of configuration section >>>

// <<< sl:start pin_tool >>>

// <timer channel=RED,GREEN,BLUE,WHITE> SL_SIMPLE_RGBW_PWM_LED_LED_RGB
// $[TIMER_SL_SIMPLE_RGBW_PWM_LED_LED_RGB]
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_PERIPHERAL TIMER1
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_PERIPHERAL_NO 1

#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_CHANNEL 0
// TIMER1 CC0 on PD11
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_PORT  gpioPortD
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_PIN   11
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_RED_LOC   19

#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_CHANNEL 1
// TIMER1 CC1 on PD12
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_PORT gpioPortD
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_PIN 12
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_GREEN_LOC 19

#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_CHANNEL 2
// TIMER1 CC2 on PD13
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_PORT gpioPortD
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_PIN  13
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_BLUE_LOC  19

#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_CHANNEL 3
// TIMER1 CC3 on PC6
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_PORT gpioPortC
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_PIN 6
#define SL_SIMPLE_RGBW_PWM_LED_LED_RGB_WHITE_LOC 8
// [TIMER_SL_SIMPLE_RGBW_PWM_LED_LED_RGB]$

void rgb_led_init(void);
void rgb_led_deinit(void);
void rgb_led_set(uint8_t m, uint8_t r, uint8_t g, uint8_t b);
#endif

#ifdef __cplusplus
}
#endif