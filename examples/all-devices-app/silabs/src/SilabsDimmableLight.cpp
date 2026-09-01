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
#include "SilabsDimmableLight.h"

#include "SilabsLightLed.h"

#include <lib/support/logging/CHIPLogging.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

namespace chip {
namespace app {

namespace {

// Read the current LevelControl::CurrentLevel from the DimmableLoad wrapper
// so that OnOff transitions restore the last known intensity (as required by
// the OnOff/LevelControl cluster spec) instead of always turning on at full
// brightness.
uint8_t CurrentLevelOr(DimmableLoad & load, uint8_t fallback)
{
    auto currentLevel = load.LevelControlCluster().GetCurrentLevel();
    if (currentLevel.IsNull() || currentLevel.Value() == 0)
    {
        return fallback;
    }
    return currentLevel.Value();
}

void DriveLedLevel(uint8_t level)
{
    TEMPORARY_RETURN_IGNORED chip::DeviceLayer::Silabs::GetPlatform().SetLedLevel(kSilabsLightLed, level);
}

} // namespace

void SilabsDimmableLight::OnOffStartup(bool on)
{
    LoggingDimmableLoad::OnOffStartup(on);
    // Persisted OnOff state at startup: reflect it on the LED using the
    // persisted LevelControl::CurrentLevel (or full brightness as a fallback
    // if the attribute hasn't been initialized yet).
    DriveLedLevel(on ? CurrentLevelOr(*this, 254) : 0);
}

void SilabsDimmableLight::OnOnOffChanged(bool on)
{
    LoggingDimmableLoad::OnOnOffChanged(on);
    DriveLedLevel(on ? CurrentLevelOr(*this, 254) : 0);
}

void SilabsDimmableLight::OnLevelChanged(uint8_t level)
{
    LoggingDimmableLoad::OnLevelChanged(level);
    // LevelControl only updates the physical output when the OnOff state is
    // ON. When OFF, the level attribute is stored but not rendered on the LED
    // so the "off" state remains visible until an explicit On command.
    if (OnOffCluster().GetOnOff())
    {
        DriveLedLevel(level);
    }
}

} // namespace app
} // namespace chip
