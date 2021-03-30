/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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


#include <stdint.h>
// NAME     Red level       Green Level     Blue Level
#ifdef HAS_RGB_LED
#define LED_COLOR_X_TABLE(ENTRY) \
    ENTRY(NONE,       0,              0,              0)    \
    ENTRY(RED,        255,            0,              0)    \
    ENTRY(GREEN,      0,              255,            0)    \
    ENTRY(BLUE,       0,              0,              255)  \
    ENTRY(WHITE,      0,              0,              0)

#define EXPAND_AS_ENUM(NAME,RL,GL,BL) LEDCOLOR_ ## NAME,
#define EXPAND_AS_STRUCT(NAME,RL,GL,BL) {RL,GL,BL},
#endif
class LEDWidget
{
public:
#ifdef HAS_RGB_LED
    typedef enum LEDColor {
    LED_COLOR_X_TABLE(EXPAND_AS_ENUM)
    COLOR_COUNT,
    } LedColor_e;

    typedef struct RGBColor {
        uint8_t red_level;
        uint8_t green_level;
        uint8_t blue_level;
    } RGBColor_t;
#endif


    static void InitGpio(void);
    void Init(int ledNum, bool isRGB=false);
    void Set(bool state);
#ifdef HAS_RGB_LED
    void Set(LedColor_e color);
    void Set(uint16_t x, uint16_t y);
#endif
    // void Set(uint8_t m, uint8_t r, uint8_t g, uint8_t b);
    void Invert(void);
    void Blink(uint32_t changeRateMS);
    void Blink(uint32_t onTimeMS, uint32_t offTimeMS);
    void Animate();

private:
    int64_t mLastChangeTimeUS;
    uint32_t mBlinkOnTimeMS;
    uint32_t mBlinkOffTimeMS;
    int mLedNum;
    bool mState;
    bool mIsRGB;
#ifdef HAS_RGB_LED
    LedColor_e mCurrentColor = LEDCOLOR_NONE;


    RGBColor_t mColorTable[COLOR_COUNT] = {LED_COLOR_X_TABLE(EXPAND_AS_STRUCT)};
    // RGBColor_t ConvertXYToRGB(uint16_t x, uint16_t y);
#endif

    void DoSet(bool state);

};
