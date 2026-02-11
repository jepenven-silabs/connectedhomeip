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

#include "stdint.h"

class SilabsLCD
{

public:
    typedef enum screen
    {
        DemoScreen = 0,
        StatusScreen,
        QRCodeScreen,
        CycleScreen,
        InvalidScreen,
    } Screen_e;

    int Init();
    int ShowScreen(Screen_e screen);
    int ShowQRCode(void);

    int PrintBitmap(uint8_t * bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight, uint8_t x, uint8_t y);

    int PaintHeader(void);
    int PaintFooter(void);
    int PaintDemoScreen(void);
    int PaintStatusScreen(void);

private:
    int WriteToDisplay(const uint8_t * bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight, uint8_t x, uint8_t y);

    void reverse_bitmap_bits(uint8_t *data, uint16_t size);
};
