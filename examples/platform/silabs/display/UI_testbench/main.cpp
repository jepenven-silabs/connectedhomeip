/*
 * Copyright (c) 2019 Jan Van Winkel <jan.van_winkel@dxplore.eu>
 *
 * Based on ST7789V sample:
 * Copyright (c) 2019 Marc Reilly
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(sample, LOG_LEVEL_INF);

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/display.h>
#include <zephyr/sys/byteorder.h>



#include "demo-ui-bitmaps.h"

#include "lcd_2.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/gpio.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <lvgl_input_device.h>

static uint32_t count;

#ifdef CONFIG_RESET_COUNTER_SW0
static struct gpio_dt_spec button_gpio = GPIO_DT_SPEC_GET_OR(
		DT_ALIAS(sw0), gpios, {0});
static struct gpio_callback button_callback;

static void button_isr_callback(const struct device *port,
				struct gpio_callback *cb,
				uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(cb);
	ARG_UNUSED(pins);

	count = 0;
}
#endif /* CONFIG_RESET_COUNTER_SW0 */


static void lv_btn_click_callback(lv_event_t *e)
{
	ARG_UNUSED(e);

	count = 0;
}

static void reverse_bitmap_bits(uint8_t *data, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        uint8_t b = data[i];
        b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
        b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
        b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
        data[i] = b;
    }
}

int main(void)
{
	char count_str[11] = {0};
	SilabsLCD lcd;
	int ret = lcd.Init();
	if (ret != 0) {
		LOG_ERR("Failed to initialize LCD");
		return -1;
	}
	uint8_t zigbeeLogoBitmap[] = { ZIGBEE_BITMAP };
	uint8_t matterLogoBitmap[] = { MATTER_LOGO_BITMAP };
	uint8_t threadLogoBitmap[] = { THREAD_BITMAP };



	lcd.PrintBitmap(threadLogoBitmap, THREAD_BITMAP_WIDTH, THREAD_BITMAP_HEIGHT, 0, 25);
	lcd.PrintBitmap(matterLogoBitmap, MATTER_LOGO_WIDTH, MATTER_LOGO_HEIGHT, 50, 25);
	// lcd.PrintBitmap(zigbeeLogoBitmap, ZIGBEE_BITMAP_WIDTH, ZIGBEE_BITMAP_HEIGHT, 75, 25);


// reverse_bitmap_bits(threadLogoBitmap, sizeof(threadLogoBitmap));

// LOG_ERR("Bitmap size: %d bytes, expected: %d bytes", 
//         sizeof(threadLogoBitmap), 
//         (16 * 16 + 7) / 8);  // Should be 32 bytes for 16x16

// // For monochrome bitmap, use I1 (1-bit indexed) format
// lv_image_dsc_t my_img_dsc = {
//     .header = {
//         .magic = LV_IMAGE_HEADER_MAGIC,
//         .cf = LV_COLOR_FORMAT_I1,  // 1-bit indexed (black/white)
// 		.flags = LV_IMAGE_FLAGS_PREMULTIPLIED,  
//         .w = 16,
//         .h = 16,
// 		.stride = 2, // Number of bytes per row (1 bit per pixel)
//     },
//     .data_size = sizeof(threadLogoBitmap),
//     .data = threadLogoBitmap,
// };


// 	// Set palette on the image descriptor, not the object
// 	lv_image_buf_set_palette(&my_img_dsc, 1, white); // Index 0 -> black
// 	lv_image_buf_set_palette(&my_img_dsc, 0, black); // Index 1 -> white

// 	// Create an image object and set the source to our custom image descriptor
// 	lv_obj_t * img = lv_image_create(lv_screen_active());
// 	lv_image_set_src(img, &my_img_dsc);
// 	lv_obj_align(img, LV_ALIGN_TOP_MID, 0, 4);


	while (1) {
		k_sleep(K_MSEC(100));
	}
}
