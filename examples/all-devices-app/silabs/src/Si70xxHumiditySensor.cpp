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

#include "Si70xxHumiditySensor.h"

#include "Si70xxSensorReader.h"
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip::app::Clusters;

namespace chip {
namespace app {

namespace {

// Si70xx measures 0..100 %RH; expose the full range to the cluster.
constexpr uint16_t kMinRelativeHumidity = 0;     // 0.00 %
constexpr uint16_t kMaxRelativeHumidity = 10000; // 100.00 %
// Si7021 datasheet: ±3 %RH typical, expressed as 0.01 % units.
constexpr uint16_t kRelativeHumidityTolerance = 300;

const RelativeHumidityMeasurementCluster::Config kSi70xxHumidityConfig = []() {
    RelativeHumidityMeasurementCluster::Config config;
    config.minMeasuredValue = DataModel::MakeNullable<uint16_t>(kMinRelativeHumidity);
    config.maxMeasuredValue = DataModel::MakeNullable<uint16_t>(kMaxRelativeHumidity);
    config.WithTolerance(kRelativeHumidityTolerance);
    return config;
}();

} // namespace

Si70xxHumiditySensor::Si70xxHumiditySensor(TimerDelegate & timerDelegate) :
    HumiditySensor(timerDelegate, kSi70xxHumidityConfig), mTimerDelegate(timerDelegate)
{}

Si70xxHumiditySensor::~Si70xxHumiditySensor()
{
    Si70xxSensorReader::Instance().DetachHumidityListener();
}

CHIP_ERROR Si70xxHumiditySensor::Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                          EndpointComposition composition)
{
    ReturnErrorOnFailure(HumiditySensor::Register(endpoint, provider, composition));
    Si70xxSensorReader::Instance().AttachHumidityListener(mTimerDelegate, &Si70xxHumiditySensor::OnHumidityUpdate, this);
    return CHIP_NO_ERROR;
}

void Si70xxHumiditySensor::Unregister(CodeDrivenDataModelProvider & provider)
{
    Si70xxSensorReader::Instance().DetachHumidityListener();
    HumiditySensor::Unregister(provider);
}

void Si70xxHumiditySensor::OnHumidityUpdate(void * context, uint16_t centiPercent)
{
    auto * self = static_cast<Si70xxHumiditySensor *>(context);
    LogErrorOnFailure(
        self->RelativeHumidityMeasurementCluster().SetMeasuredValue(DataModel::MakeNullable<uint16_t>(centiPercent)));
}

} // namespace app
} // namespace chip
