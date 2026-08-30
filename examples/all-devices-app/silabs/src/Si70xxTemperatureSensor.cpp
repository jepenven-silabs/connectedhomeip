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

#include "Si70xxTemperatureSensor.h"

#include "Si70xxSensorReader.h"
#include <app/clusters/temperature-measurement-server/TemperatureMeasurementCluster.h>
#include <lib/support/CodeUtils.h>
#include <lib/support/logging/CHIPLogging.h>

using namespace chip::app::Clusters;

namespace chip {
namespace app {

namespace {

// Reasonable indoor operating range for the Si70xx sensor, expressed in 0.01 °C.
constexpr int16_t kMinTemperatureCentiCelsius = -1000; // -10.00 °C
constexpr int16_t kMaxTemperatureCentiCelsius = 6000;  //  60.00 °C
// Si7021 datasheet: ±0.4 °C typical, expressed as 0.01 °C units.
constexpr uint16_t kTemperatureTolerance = 40;

const TemperatureMeasurementCluster::StartupConfiguration kSi70xxTemperatureConfig = {
    .minMeasuredValue = DataModel::MakeNullable<int16_t>(kMinTemperatureCentiCelsius),
    .maxMeasuredValue = DataModel::MakeNullable<int16_t>(kMaxTemperatureCentiCelsius),
    .tolerance        = kTemperatureTolerance,
};

TemperatureMeasurementCluster::OptionalAttributeSet MakeOptionalAttributes()
{
    TemperatureMeasurementCluster::OptionalAttributeSet optionalAttributes;
    optionalAttributes.Set<TemperatureMeasurement::Attributes::Tolerance::Id>();
    return optionalAttributes;
}

} // namespace

Si70xxTemperatureSensor::Si70xxTemperatureSensor() :
    TemperatureSensor(mTimerDelegate, kSi70xxTemperatureConfig, MakeOptionalAttributes())
{}

Si70xxTemperatureSensor::~Si70xxTemperatureSensor()
{
    Si70xxSensorReader::Instance().DetachTemperatureListener();
}

CHIP_ERROR Si70xxTemperatureSensor::Register(EndpointId endpoint, CodeDrivenDataModelProvider & provider,
                                             EndpointComposition composition)
{
    ReturnErrorOnFailure(TemperatureSensor::Register(endpoint, provider, composition));
    Si70xxSensorReader::Instance().AttachTemperatureListener(mTimerDelegate, &Si70xxTemperatureSensor::OnTemperatureUpdate, this);
    return CHIP_NO_ERROR;
}

void Si70xxTemperatureSensor::Unregister(CodeDrivenDataModelProvider & provider)
{
    Si70xxSensorReader::Instance().DetachTemperatureListener();
    TemperatureSensor::Unregister(provider);
}

void Si70xxTemperatureSensor::OnTemperatureUpdate(void * context, int16_t centiCelsius)
{
    auto * self = static_cast<Si70xxTemperatureSensor *>(context);
    LogErrorOnFailure(
        self->TemperatureMeasurementCluster().SetMeasuredValue(DataModel::MakeNullable<int16_t>(centiCelsius)));
}

} // namespace app
} // namespace chip
