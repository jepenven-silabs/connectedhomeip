/*
 *
 *    Copyright (c) 2026 Project CHIP Authors
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
#include "SilabsOnOffLight.h"

#include "SilabsLightLed.h"

#include <lib/support/logging/CHIPLogging.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

namespace chip {
namespace app {

namespace {

// On boards with an RGB LED, `SetLed(true, ...)` retains the last-set color
// from the SDK PWM context (which is undefined right after boot). Explicitly
// drive the LED via the level API so RGB boards render a white intensity of
// 254 (fully on) instead of a stale/undefined color. On plain LEDs the level
// API falls back to on/off thresholding, so the same call works everywhere.
constexpr uint8_t kOnLevel = 254;

void DriveLed(bool on)
{
    TEMPORARY_RETURN_IGNORED chip::DeviceLayer::Silabs::GetPlatform().SetLedLevel(kSilabsLightLed, on ? kOnLevel : 0);
}

} // namespace

void SilabsOnOffLight::OnOffStartup(bool on)
{
    LoggingOnOffLoad::OnOffStartup(on);
    DriveLed(on);
}

void SilabsOnOffLight::OnOnOffChanged(bool on)
{
    LoggingOnOffLoad::OnOnOffChanged(on);
    DriveLed(on);
}

} // namespace app
} // namespace chip
