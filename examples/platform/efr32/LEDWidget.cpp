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

#include "LEDWidget.h"

#include "bsp.h"

#ifdef HAS_RGB_LED
#include "rgbled.h"
#include "sl_simple_rgbw_pwm_led.h"

#endif

#include <platform/CHIPDeviceLayer.h>


void LEDWidget::InitGpio(void)
{
    // Sets gpio pin mode for ALL board Leds.
    #ifdef HAS_RGB_LED
    rgb_led_init();
    #else
    BSP_LedsInit();
    #endif

}

void LEDWidget::Init(int ledNum, bool isRGB)
{
    mLastChangeTimeUS = 0;
    mBlinkOnTimeMS    = 0;
    mBlinkOffTimeMS   = 0;
    mLedNum           = ledNum;
    mIsRGB            = isRGB;

    Set(false);
}

void LEDWidget::Invert(void)
{
    Set(!mState);
}

void LEDWidget::Set(bool state)
{
    mLastChangeTimeUS = mBlinkOnTimeMS = mBlinkOffTimeMS = 0;
    DoSet(state);
}

#ifdef HAS_RGB_LED
void LEDWidget::Set(LedColor_e color)
{
    if (color >= COLOR_COUNT) {
        return;
    }
    mLastChangeTimeUS = mBlinkOnTimeMS = mBlinkOffTimeMS = 0;
    mCurrentColor = color;
    rgb_led_set(0xff, mColorTable[color].red_level, mColorTable[color].green_level, mColorTable[color].blue_level);
}
void LEDWidget::Set(uint16_t x, uint16_t y)
{

}
#endif

void LEDWidget::Blink(uint32_t changeRateMS)
{
    Blink(changeRateMS, changeRateMS);
}

void LEDWidget::Blink(uint32_t onTimeMS, uint32_t offTimeMS)
{
    mBlinkOnTimeMS  = onTimeMS;
    mBlinkOffTimeMS = offTimeMS;
    Animate();
}

void LEDWidget::Animate()
{
    if (mBlinkOnTimeMS != 0 && mBlinkOffTimeMS != 0)
    {
        int64_t nowUS            = ::chip::System::Layer::GetClock_MonotonicHiRes();
        int64_t stateDurUS       = ((mState) ? mBlinkOnTimeMS : mBlinkOffTimeMS) * 1000LL;
        int64_t nextChangeTimeUS = mLastChangeTimeUS + stateDurUS;

        if (nowUS > nextChangeTimeUS)
        {
            DoSet(!mState);
            mLastChangeTimeUS = nowUS;
        }
    }
}

void LEDWidget::DoSet(bool state)
{
    mState = state;

    if (mIsRGB)
    {
        #ifdef HAS_RGB_LED
        uint8_t mask = (state) ? 0xFF : 0x00;
        rgb_led_set(mask, mColorTable[mCurrentColor].red_level, mColorTable[mCurrentColor].green_level, mColorTable[mCurrentColor].blue_level);
        #endif
    }
    else
    {
        if (state)
        {
            BSP_LedSet(mLedNum);
        }
        else
        {
            BSP_LedClear(mLedNum);
        }
    }

}

// RGBColor_t LEDWidget::ConvertXYToRGB(uint16_t x, uint16_t y)
// {
//     // if (rgb == nullptr) {
//     //     return;
//     // }
//     // if (y == 0) return [0,0,0];
// 	// return [
// 	// 	normalize((x*yy)/y),
// 	// 	normalize(yy),
// 	// 	normalize(((1-x-y)*yy)/y)
// 	// ];
//     return
// }
