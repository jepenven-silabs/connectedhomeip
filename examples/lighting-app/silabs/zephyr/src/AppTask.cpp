/*
 *
 *    Copyright (c) 2025 Project CHIP Authors
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

#include "AppTask.h"
#include "CHIPDeviceManager.h"

#include <app/util/attribute-storage.h>
#include <platform/CHIPDeviceLayer.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#ifdef MATTER_DM_PLUGIN_BINDING
#include "binding-handler.h"
#endif

LOG_MODULE_DECLARE(app, CONFIG_CHIP_APP_LOG_LEVEL);

using namespace chip;
using namespace chip::DeviceLayer;

/* -------------------------------------------------------------------------- */
/*                          LED devicetree bindings                           */
/* -------------------------------------------------------------------------- */

#if DT_NODE_EXISTS(DT_ALIAS(led0))
#define HAS_LED0 1
static const struct gpio_dt_spec sLed0 = GPIO_DT_SPEC_GET(DT_ALIAS(led0), gpios);
static struct k_timer sLed0BlinkTimer;
static bool sLed0State;
#endif

#if DT_NODE_EXISTS(DT_ALIAS(led1))
#define HAS_LED1 1
static const struct gpio_dt_spec sLed1 = GPIO_DT_SPEC_GET(DT_ALIAS(led1), gpios);
#endif

/* -------------------------------------------------------------------------- */
/*                        Button devicetree bindings                          */
/* -------------------------------------------------------------------------- */

#if DT_NODE_EXISTS(DT_ALIAS(sw0))
#define HAS_BUTTON0 1
static const struct gpio_dt_spec sButton0 = GPIO_DT_SPEC_GET(DT_ALIAS(sw0), gpios);
static struct gpio_callback sButton0CbData;
#endif

#if DT_NODE_EXISTS(DT_ALIAS(sw1))
#define HAS_BUTTON1 1
static const struct gpio_dt_spec sButton1 = GPIO_DT_SPEC_GET(DT_ALIAS(sw1), gpios);
static struct gpio_callback sButton1CbData;
#endif

/* -------------------------------------------------------------------------- */
/*                           LED helper functions                             */
/* -------------------------------------------------------------------------- */

#ifdef HAS_LED0
static void Led0BlinkTimerHandler(struct k_timer * timer)
{
    sLed0State = !sLed0State;
    gpio_pin_set_dt(&sLed0, static_cast<int>(sLed0State));
}
#endif

/* -------------------------------------------------------------------------- */
/*                             Button callbacks                               */
/* -------------------------------------------------------------------------- */

#ifdef HAS_BUTTON0
static void Button0PressedHandler(const struct device * dev, struct gpio_callback * cb, uint32_t pins)
{
    LOG_INF("Button 0 pressed");
}
#endif

#ifdef HAS_BUTTON1
static void Button1PressedHandler(const struct device * dev, struct gpio_callback * cb, uint32_t pins)
{
#ifdef HAS_LED1
    gpio_pin_toggle_dt(&sLed1);
#endif
    LOG_INF("Button 1 pressed – LED 1 toggled");
}
#endif

/* -------------------------------------------------------------------------- */
/*                          AppTask implementation                            */
/* -------------------------------------------------------------------------- */

void AppTask::PreInitMatterStack()
{
    ChipLogProgress(DeviceLayer, "Welcome to Zephyr Matter Light Demo App");

#ifdef HAS_LED0
    if (gpio_is_ready_dt(&sLed0))
    {
        gpio_pin_configure_dt(&sLed0, GPIO_OUTPUT_INACTIVE);
    }
    k_timer_init(&sLed0BlinkTimer, Led0BlinkTimerHandler, nullptr);
#endif

#ifdef HAS_LED1
    if (gpio_is_ready_dt(&sLed1))
    {
        gpio_pin_configure_dt(&sLed1, GPIO_OUTPUT_INACTIVE);
    }
#endif

#ifdef HAS_BUTTON0
    if (gpio_is_ready_dt(&sButton0))
    {
        gpio_pin_configure_dt(&sButton0, GPIO_INPUT);
        gpio_pin_interrupt_configure_dt(&sButton0, GPIO_INT_EDGE_TO_ACTIVE);
        gpio_init_callback(&sButton0CbData, Button0PressedHandler, BIT(sButton0.pin));
        gpio_add_callback(sButton0.port, &sButton0CbData);
    }
#endif

#ifdef HAS_BUTTON1
    if (gpio_is_ready_dt(&sButton1))
    {
        gpio_pin_configure_dt(&sButton1, GPIO_INPUT);
        gpio_pin_interrupt_configure_dt(&sButton1, GPIO_INT_EDGE_TO_ACTIVE);
        gpio_init_callback(&sButton1CbData, Button1PressedHandler, BIT(sButton1.pin));
        gpio_add_callback(sButton1.port, &sButton1CbData);
    }
#endif
}

void AppTask::PostInitMatterStack()
{
#ifdef MATTER_DM_PLUGIN_BINDING
    CHIP_ERROR err = InitBindingHandlers();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(DeviceLayer, "InitBindingHandlers failed: %" CHIP_ERROR_FORMAT, err.Format());
    }
#endif

#ifdef HAS_LED0
    if (ConnectivityMgr().IsBLEAdvertisingEnabled())
    {
        sLed0State = false;
        k_timer_start(&sLed0BlinkTimer, K_MSEC(500), K_MSEC(500));
    }
#endif
}

void AppTask::PostInitMatterServerInstance() {}

AppTask & AppTask::GetDefaultInstance()
{
    static AppTask sAppTask;
    return sAppTask;
}

chip::Zephyr::App::AppTaskBase & chip::Zephyr::App::GetAppTask()
{
    return AppTask::GetDefaultInstance();
}
