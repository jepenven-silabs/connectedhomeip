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

#include <cstdint>

namespace chip {
namespace app {

/// LED index used by light device types in the all-devices-app for silabs.
///
/// Boards with two LEDs (LED0 + LED1) reserve LED0 for the
/// advertisement/identify status feedback and LED1 for the application
/// (i.e. the light being driven by the OnOff / LevelControl clusters).
///
/// On boards that only expose a single LED (mono or RGB), the platform
/// abstraction (`SilabsPlatform::SetLed*`) transparently redirects requests
/// for LED index 1 to LED index 0. This keeps a single constant here and
/// avoids scattering per-board conditionals throughout the app code.
inline constexpr uint8_t kSilabsLightLed = 1;

} // namespace app
} // namespace chip
