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
#include <lib/support/TimerDelegate.h>
#include <system/SystemClock.h>

namespace chip {
namespace app {

/**
 * Shared reader that owns the single on-board Si70xx (temperature/relative-humidity)
 * sensor and pushes readings to per-cluster listeners.
 *
 * The Si70xx returns temperature (int16_t, centi-Celsius) and relative humidity
 * (uint16_t, centi-percent) with a single I2C transaction, so temperature-sensor
 * and humidity-sensor device instances share this reader to avoid concurrent I2C
 * access and duplicated initialization.
 *
 * Lifetime: a single static instance is created via `Instance()`. The reader
 * lazily initializes the driver on the first `AttachTemperatureListener` /
 * `AttachHumidityListener` call and (re)arms a periodic timer to poll the
 * sensor.
 */
class Si70xxSensorReader : public TimerContext
{
public:
    /// Callback signature for temperature updates. Value is in 0.01 °C, matching
    /// the Matter TemperatureMeasurement cluster's MeasuredValue units.
    using TemperatureListener = void (*)(void * context, int16_t centiCelsius);

    /// Callback signature for humidity updates. Value is in 0.01 %, matching
    /// the Matter RelativeHumidityMeasurement cluster's MeasuredValue units.
    using HumidityListener = void (*)(void * context, uint16_t centiPercent);

    static Si70xxSensorReader & Instance();

    /// Register interest in temperature updates. `context` is passed back to the
    /// listener callback verbatim. Only one listener is supported.
    void AttachTemperatureListener(TimerDelegate & timerDelegate, TemperatureListener listener, void * context);

    /// Register interest in humidity updates. `context` is passed back to the
    /// listener callback verbatim. Only one listener is supported.
    void AttachHumidityListener(TimerDelegate & timerDelegate, HumidityListener listener, void * context);

    /// Detach a previously-attached temperature listener.
    void DetachTemperatureListener();

    /// Detach a previously-attached humidity listener.
    void DetachHumidityListener();

    // TimerContext
    void TimerFired() override;

private:
    Si70xxSensorReader() = default;

    /// Ensures the underlying Si70xx driver is initialized. Logs and returns
    /// false on failure. Safe to call repeatedly.
    bool EnsureInitialized();

    /// Starts the polling timer if not already running and at least one
    /// listener is attached.
    void EnsurePolling();

    // Poll every 10 seconds — matches the cadence of the simulated
    // Increasing{Temperature,Humidity}Sensor and keeps I2C traffic low.
    static constexpr System::Clock::Seconds16 kPollIntervalSec = System::Clock::Seconds16(10);

    TimerDelegate * mTimerDelegate       = nullptr;
    TemperatureListener mTempListener    = nullptr;
    void * mTempListenerContext          = nullptr;
    HumidityListener mHumidityListener   = nullptr;
    void * mHumidityListenerContext      = nullptr;
    bool mDriverInitialized              = false;
    bool mDriverInitFailed               = false;
    bool mTimerRunning                   = false;
};

} // namespace app
} // namespace chip
