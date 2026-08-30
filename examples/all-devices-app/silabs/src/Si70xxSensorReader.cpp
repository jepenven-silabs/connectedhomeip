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

#include "Si70xxSensorReader.h"

#include "Si70xxSensor.h"
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

namespace chip {
namespace app {

Si70xxSensorReader & Si70xxSensorReader::Instance()
{
    static Si70xxSensorReader sInstance;
    return sInstance;
}

bool Si70xxSensorReader::EnsureInitialized()
{
    if (mDriverInitialized)
    {
        return true;
    }
    if (mDriverInitFailed)
    {
        return false;
    }

    sl_status_t status = Si70xxSensor::Init();
    if (status != SL_STATUS_OK)
    {
        ChipLogError(AppServer, "Si70xxSensorReader: Init failed: 0x%lx", static_cast<unsigned long>(status));
        mDriverInitFailed = true;
        return false;
    }

    mDriverInitialized = true;
    return true;
}

void Si70xxSensorReader::EnsurePolling()
{
    if (mTimerRunning || mTimerDelegate == nullptr)
    {
        return;
    }
    if (mTempListener == nullptr && mHumidityListener == nullptr)
    {
        return;
    }

    LogErrorOnFailure(mTimerDelegate->StartTimer(this, kPollIntervalSec));
    mTimerRunning = true;
}

void Si70xxSensorReader::AttachTemperatureListener(TimerDelegate & timerDelegate, TemperatureListener listener, void * context)
{
    mTimerDelegate       = &timerDelegate;
    mTempListener        = listener;
    mTempListenerContext = context;
    (void) EnsureInitialized();
    EnsurePolling();
}

void Si70xxSensorReader::AttachHumidityListener(TimerDelegate & timerDelegate, HumidityListener listener, void * context)
{
    mTimerDelegate           = &timerDelegate;
    mHumidityListener        = listener;
    mHumidityListenerContext = context;
    (void) EnsureInitialized();
    EnsurePolling();
}

void Si70xxSensorReader::DetachTemperatureListener()
{
    mTempListener        = nullptr;
    mTempListenerContext = nullptr;
    if (mTempListener == nullptr && mHumidityListener == nullptr && mTimerDelegate != nullptr && mTimerRunning)
    {
        mTimerDelegate->CancelTimer(this);
        mTimerRunning = false;
    }
}

void Si70xxSensorReader::DetachHumidityListener()
{
    mHumidityListener        = nullptr;
    mHumidityListenerContext = nullptr;
    if (mTempListener == nullptr && mHumidityListener == nullptr && mTimerDelegate != nullptr && mTimerRunning)
    {
        mTimerDelegate->CancelTimer(this);
        mTimerRunning = false;
    }
}

void Si70xxSensorReader::TimerFired()
{
    mTimerRunning = false;

    if (EnsureInitialized())
    {
        uint16_t humidity   = 0;
        int16_t temperature = 0;
        sl_status_t status  = Si70xxSensor::GetSensorData(humidity, temperature);
        if (status == SL_STATUS_OK)
        {
            ChipLogProgress(AppServer, "Si70xxSensorReader: temp=%d.%d C, humidity=%u.%u %%",
                          static_cast<int>(temperature/100),static_cast<int>(temperature%100), static_cast<unsigned>(humidity/100),static_cast<unsigned>(humidity%100));

            if (mTempListener != nullptr)
            {
                mTempListener(mTempListenerContext, temperature);
            }
            if (mHumidityListener != nullptr)
            {
                mHumidityListener(mHumidityListenerContext, humidity);
            }
        }
        else
        {
            ChipLogError(AppServer, "Si70xxSensorReader: read failed: 0x%lx", static_cast<unsigned long>(status));
        }
    }

    // Re-arm the timer as long as we still have listeners.
    EnsurePolling();
}

} // namespace app
} // namespace chip
