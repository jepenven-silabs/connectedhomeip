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

#pragma once

#include <device/types/dimmable-light/impl/LoggingDimmableLight.h>

namespace chip {
namespace app {

/// dimmable-light device type that also drives the physical SiLabs LED so the
/// OnOff and LevelControl cluster state is observable on the board.
///
/// The LED intensity is driven through
/// `SilabsPlatform::SetLedLevel(kSilabsLightLed, level)`, which uses PWM on
/// boards that expose a PWM-capable LED (currently the on-board RGB LED on
/// e.g. BRD2601B) and falls back to on/off thresholding elsewhere.
class SilabsDimmableLight : public LoggingDimmableLight
{
public:
    using LoggingDimmableLight::LoggingDimmableLight;
    ~SilabsDimmableLight() override = default;

protected:
    // OnOffDelegate
    void OnOffStartup(bool on) override;
    void OnOnOffChanged(bool on) override;

    // LevelControlDelegate
    void OnLevelChanged(uint8_t level) override;
};

} // namespace app
} // namespace chip
