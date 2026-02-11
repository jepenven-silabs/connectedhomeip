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

#include "lcd_2.h"
#include "demo-ui-bitmaps.h"


// Status Icon defines
#define STATUS_ICON_LINE 0
#define SILABS_ICON_POSITION_X 0
#define BLE_ICON_POSITION_X 72
#define NETWORK_ICON_POSITION_X 90
#define MATTER_ICON_POSITION_X 108

// Bitmap
static const uint8_t silabsLogo[]       = { SILABS_LOGO_SMALL };
static const uint8_t matterLogoBitmap[] = { MATTER_LOGO_BITMAP };

static const uint8_t wifiLogo[]   = { WIFI_BITMAP };
static const uint8_t threadLogo[] = { THREAD_BITMAP };
static const uint8_t bleLogo[]    = { BLUETOOTH_ICON_SMALL };

#ifdef __ZEPHYR__
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/kernel.h>

#include <lvgl.h>
struct device *display_dev;

static uint8_t workBuffer[(128*128)/8];

lv_color32_t black = {
	.blue = 0,
	.green = 0,
	.red = 0,
	.alpha = 255
	};
lv_color32_t white = {
	.blue = 255,
	.green = 255,
	.red = 255,
	.alpha = 255
	};
#endif // __ZEPHYR__
int SilabsLCD::Init()
{
    #ifdef __ZEPHYR__
    display_dev = const_cast<struct device *>(DEVICE_DT_GET(DT_CHOSEN(zephyr_display)));
	if (!device_is_ready(display_dev)) {
		return -1;
	}

    return display_blanking_off(display_dev);
    #endif // __ZEPHYR__
    return 0;
}
int SilabsLCD::ShowScreen(Screen_e screen)
{
    switch (screen)
    {
    case DemoScreen:
        return PaintDemoScreen();
    case StatusScreen:
        return PaintStatusScreen();
    case QRCodeScreen:
        return ShowQRCode();
    default:
        return -1;
    }
}   
int SilabsLCD::PaintHeader()
{
    // Draw Silabs Corner icon
    struct display_buffer_descriptor buf_desc;
    buf_desc.buf_size = sizeof(matterLogoBitmap);
    buf_desc.width = MATTER_LOGO_WIDTH;
    buf_desc.height = MATTER_LOGO_HEIGHT;
    buf_desc.pitch = MATTER_LOGO_WIDTH;
    buf_desc.frame_incomplete = false;
    display_blanking_off(display_dev);
    display_write(display_dev, 0, 15, &buf_desc, matterLogoBitmap);
    display_blanking_on(display_dev);
    display_write(display_dev, 0, 15, &buf_desc, matterLogoBitmap);
    // WriteToDisplay(silabsLogo, SILABS_LOGO_HEIGHT, SILABS_LOGO_HEIGHT, SILABS_ICON_POSITION_X,  STATUS_ICON_LINE);
    // Draw BLE Icon
    // WriteToDisplay(bleLogo, BLE_ICON_POSITION_X, STATUS_ICON_LINE, BLUETOOTH_ICON_SIZE, BLUETOOTH_ICON_SIZE);
    // Draw WiFi/OpenThread Icon
    // WriteToDisplay((UI_WIFI) ? wifiLogo : threadLogo, NETWORK_ICON_POSITION_X, STATUS_ICON_LINE,atter Icon
    // WriteToDisplay(matterLogoBitmap, MATTER_ICON_POSITION_X, STATUS_ICON_LINE, MATTER_LOGO_WIDTH, MATTER_LOGO_HEIGHT, matterLogoBitmap);

    return 0;
}
int SilabsLCD::PaintFooter()
{

    return 0;
}

int SilabsLCD::PrintBitmap(uint8_t * bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight, uint8_t x, uint8_t y)
{
    #ifdef __ZEPHYR__
    uint16_t bitmapSize = (bitmapWidth * bitmapHeight) / 8;
    uint16_t bufferSize = 8 + bitmapSize; // 8 bytes for the header
 // Should be 32 bytes for 16x16
    memset(workBuffer, 0, bufferSize);
    memcpy(workBuffer + 8, bitmap, bitmapSize); // Copy bitmap data after the header
    reverse_bitmap_bits(workBuffer, bufferSize);
    lv_image_dsc_t my_img_dsc = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_I1,  // 1-bit indexed (black/white)
		.flags = LV_IMAGE_FLAGS_PREMULTIPLIED,  
        .w = bitmapWidth,
        .h = bitmapHeight,
		// .stride = (bitmapWidth + 7) / 8, // Number of bytes per row (1 bit per pixel)
        .stride = 2,
    },
    .data_size = bufferSize,
    .data = workBuffer,
    };

    lv_image_buf_set_palette(&my_img_dsc, 1, white); // Index 0 -> black
	lv_image_buf_set_palette(&my_img_dsc, 0, black); // Index 1 -> white
    lv_obj_t * img = lv_image_create(lv_screen_active());
    lv_image_set_src(img, &my_img_dsc);
    lv_obj_set_pos(img, x,y);
    

    lv_timer_handler();
    #endif // __ZEPHYR__
    return 0;
}

int SilabsLCD::PaintDemoScreen()
{
    return 0;
}
int SilabsLCD::PaintStatusScreen()
{       
    return 0;
}
int SilabsLCD::ShowQRCode(void)
{
    return 0;
}

int SilabsLCD::WriteToDisplay(const uint8_t * bitmap, uint8_t bitmapWidth, uint8_t bitmapHeight, uint8_t x, uint8_t y)
{

#ifdef __ZEPHYR__
    struct display_buffer_descriptor buf_desc;
    buf_desc.buf_size = bitmapWidth * bitmapHeight;
    buf_desc.width = bitmapWidth;
    buf_desc.height = bitmapHeight;
    buf_desc.pitch = bitmapWidth;

    return display_write(display_dev, x, y, &buf_desc, bitmap);
    
#else
    GLIB_drawBitmap(glibContext, x, y, bitmapWidth, bitmapHeight, bitmap);
    #if SL_LCDCTRL_MUX
    sl_wfx_host_pre_lcd_spi_transfer();
    #endif // SL_LCDCTRL_MUX
    DMD_updateDisplay();
    #if SL_LCDCTRL_MUX
    sl_wfx_host_post_lcd_spi_transfer();
    #endif // SL_LCDCTRL_MUX
#endif
    return 0;
}

void SilabsLCD::reverse_bitmap_bits(uint8_t *data, uint16_t size)
{
    for (size_t i = 0; i < size; i++) {
        uint8_t b = data[i];
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        data[i] = b;
    }
}
