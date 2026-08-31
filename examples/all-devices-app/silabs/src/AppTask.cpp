/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
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
#include "AppConfig.h"
#include "AppEvent.h"
#include "AppKeys.h"
#include "SilabsIdentifyLedDelegate.h"

#ifdef ENABLE_CHIP_SHELL
#include <DeviceShellCommands.h>
#endif

#include <cstring>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <app/DefaultSafeAttributePersistenceProvider.h>
#include <app/persistence/DefaultAttributePersistenceProvider.h>

#include <app/EventManagement.h>
#include <app/InteractionModelEngine.h>
#include <app/icd/server/ICDServerConfig.h>
#include <app/server/Dnssd.h>
#include <app/server/Server.h>
#include <platform/CHIPDeviceLayer.h>
#include <setup_payload/OnboardingCodesUtil.h>

#include <app_config/enabled_devices.h>
#include <device-factory/DeviceFactory.h>
#include <device/api/PlatformIdentifyIntegration.h>
#include <device/api/allocator/ConsecutiveEndpointIdAllocator.h>
#include <device/types/root-node/RootNode.h>

#if defined(SL_MATTER_USE_SI70XX_SENSOR) && SL_MATTER_USE_SI70XX_SENSOR
#include "Si70xxHumiditySensor.h"
#include "Si70xxTemperatureSensor.h"
#endif // defined(SL_MATTER_USE_SI70XX_SENSOR) && SL_MATTER_USE_SI70XX_SENSOR

#if CHIP_ENABLE_OPENTHREAD
#include <device/types/root-node/ThreadRootNode.h>
#include <platform/NetworkCommissioning.h>
#endif

#if defined(CHIP_DEVICE_CONFIG_ENABLE_WIFI) && CHIP_DEVICE_CONFIG_ENABLE_WIFI
#include <device/types/root-node/WifiRootNode.h>            // nogncheck
#include <platform/silabs/NetworkCommissioningWiFiDriver.h> // nogncheck
#endif

#include <platform/silabs/platformAbstraction/SilabsPlatform.h>

#define APP_FUNCTION_BUTTON 0

using namespace chip;
using namespace chip::app;
using namespace ::chip::DeviceLayer;
using namespace ::chip::DeviceLayer::Silabs;

namespace {
chip::app::DefaultAttributePersistenceProvider sAttributePersistenceProvider;
chip::app::DefaultSafeAttributePersistenceProvider sSafeAttributePersistenceProvider;
std::unique_ptr<chip::app::CodeDrivenDataModelProvider> sDataModelProvider;
std::unique_ptr<chip::app::DeviceInterface> sRootNode;
std::vector<std::unique_ptr<chip::app::DeviceInterface>> sConstructedDevices;

#if CHIP_ENABLE_OPENTHREAD
chip::DeviceLayer::NetworkCommissioning::GenericThreadDriver sThreadDriver;
#endif

constexpr chip::EndpointId kDeviceEndpointId = 1;

/// Wraps an EndpointIdAllocator to capture the first endpoint id handed out
/// during a device's Register() call. This lets us log the primary endpoint
/// assigned to each instantiated device without depending on the concrete
/// DeviceInterface subclass (SingleEndpoint / composite / etc.).
class TrackingEndpointIdAllocator : public chip::app::EndpointIdAllocator
{
public:
    explicit TrackingEndpointIdAllocator(chip::app::EndpointIdAllocator & inner) : mInner(inner) {}

    chip::EndpointId Allocate() override
    {
        chip::EndpointId id = mInner.Allocate();
        if (mFirst == chip::kInvalidEndpointId)
        {
            mFirst = id;
        }
        return id;
    }

    /// Returns the first endpoint id allocated since the last Reset() and
    /// clears the tracked value.
    chip::EndpointId TakeFirst()
    {
        chip::EndpointId first = mFirst;
        mFirst                 = chip::kInvalidEndpointId;
        return first;
    }

private:
    chip::app::EndpointIdAllocator & mInner;
    chip::EndpointId mFirst = chip::kInvalidEndpointId;
};
} // namespace

AppTask AppTask::sAppTask;

CHIP_ERROR AppTask::StartAppTask()
{
    return BaseApplication::StartAppTask(AppTaskMain);
}

void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;
    osMessageQueueId_t sAppEventQueue = *(static_cast<osMessageQueueId_t *>(pvParameter));

    CHIP_ERROR err = GetAppTask().Init();
    if (err != CHIP_NO_ERROR)
    {
        SILABS_LOG("AppTask.Init() failed");
        appError(err);
    }

    GetAppTask().StartStatusLEDTimer();

    SILABS_LOG("App Task started");

    while (true)
    {
        osStatus_t eventReceived = osMessageQueueGet(sAppEventQueue, &event, nullptr, osWaitForever);
        while (eventReceived == osOK)
        {
            GetAppTask().DispatchEvent(&event);
            eventReceived = osMessageQueueGet(sAppEventQueue, &event, nullptr, 0);
        }
    }
}

CHIP_ERROR AppTask::AppInit()
{
    chip::DeviceLayer::Silabs::GetPlatform().SetButtonsCb(&AppTask::ButtonEventHandler);
#ifdef ENABLE_CHIP_SHELL
    chip::Shell::DeviceCommands::GetInstance().Register();
#endif
    return CHIP_NO_ERROR;
}

void AppTask::ButtonEventHandler(uint8_t button, uint8_t btnAction)
{
    AppEvent button_event           = {};
    button_event.Type               = AppEvent::kEventType_Button;
    button_event.ButtonEvent.Action = btnAction;

    if (button == APP_FUNCTION_BUTTON)
    {
        button_event.Handler = BaseApplication::ButtonHandler;
        GetAppTask().PostEvent(&button_event);
    }
}

CHIP_ERROR AppTask::InitCodeDrivenDataModel(chip::PersistentStorageDelegate & storage,
                                            chip::Credentials::GroupDataProvider * groupDataProvider)
{
    ReturnErrorOnFailure(sAttributePersistenceProvider.Init(&storage));
    ReturnErrorOnFailure(sSafeAttributePersistenceProvider.Init(&storage));
    chip::app::SetSafeAttributePersistenceProvider(&sSafeAttributePersistenceProvider);

    sDataModelProvider = std::make_unique<chip::app::CodeDrivenDataModelProvider>(storage, sAttributePersistenceProvider);
    VerifyOrReturnError(sDataModelProvider != nullptr, CHIP_ERROR_NO_MEMORY);

    chip::DeviceLayer::DeviceInstanceInfoProvider * deviceInfoProvider = chip::DeviceLayer::GetDeviceInstanceInfoProvider();
    VerifyOrReturnError(deviceInfoProvider != nullptr, CHIP_ERROR_INCORRECT_STATE);

    static chip::app::DefaultTimerDelegate sTimerDelegate;

    // Install the SiLabs status-LED identify delegate for every code-driven
    // endpoint that registers an IdentifyCluster. Also advertise
    // `kVisibleIndicator` so commissioners (e.g. Home Assistant) surface the
    // identify action.
    static chip::app::SilabsIdentifyLedDelegate sIdentifyLedDelegate;
    chip::app::PlatformIdentifyIntegration::GetInstance().SetDelegate(&sIdentifyLedDelegate);
    chip::app::PlatformIdentifyIntegration::GetInstance().SetIdentifyType(
        chip::app::Clusters::Identify::IdentifyTypeEnum::kVisibleIndicator);

    chip::app::RootNode::Context rootNodeContext = {
        .commissioningWindowManager = chip::Server::GetInstance().GetCommissioningWindowManager(),
        .configurationManager       = chip::DeviceLayer::ConfigurationMgr(),
        .deviceControlServer        = chip::DeviceLayer::DeviceControlServer::DeviceControlSvr(),
        .fabricTable                = chip::Server::GetInstance().GetFabricTable(),
        .accessControl              = chip::Server::GetInstance().GetAccessControl(),
        .persistentStorage          = storage,
        .failSafeContext            = chip::Server::GetInstance().GetFailSafeContext(),
        .deviceInstanceInfoProvider = *deviceInfoProvider,
        .platformManager            = chip::DeviceLayer::PlatformMgr(),
        .groupDataProvider          = *groupDataProvider,
        .sessionManager             = chip::Server::GetInstance().GetSecureSessionManager(),
        .dnssdServer                = chip::app::DnssdServer::Instance(),
        .deviceLoadStatusProvider   = *chip::app::InteractionModelEngine::GetInstance(),
        .diagnosticDataProvider     = chip::DeviceLayer::GetDiagnosticDataProvider(),
        .testEventTriggerDelegate   = nullptr,
        .dacProvider                = *chip::Credentials::GetDeviceAttestationCredentialsProvider(),
        .eventManagement            = chip::app::EventManagement::GetInstance(),
        .timerDelegate              = sTimerDelegate,
    };

#if CHIP_CONFIG_ENABLE_ICD_SERVER
    // When the build is configured as an Intermittently Connected Device, hand the RootNode
    // the SessionKeystore so it can construct the ICDManagement cluster on the root endpoint.
    // The ICDManager itself is already owned/initialized by chip::Server when
    // CHIP_CONFIG_ENABLE_ICD_SERVER=1, so no extra setup is required here.
    rootNodeContext.icdSymmetricKeystore = chip::Server::GetInstance().GetSessionKeystore();
    ChipLogProgress(AppServer, "ICD server enabled: registering ICDManagement cluster on the root endpoint");
#endif // CHIP_CONFIG_ENABLE_ICD_SERVER

#if CHIP_ENABLE_OPENTHREAD
    sRootNode = std::make_unique<chip::app::ThreadRootNode>(rootNodeContext,
                                                            chip::app::ThreadRootNode::ThreadContext{
                                                                .threadDriver = sThreadDriver,
                                                            });
#elif defined(CHIP_DEVICE_CONFIG_ENABLE_WIFI) && CHIP_DEVICE_CONFIG_ENABLE_WIFI
    sRootNode = std::make_unique<chip::app::WifiRootNode>(
        rootNodeContext,
        chip::app::WifiRootNode::WifiContext{
            .wifiDriver = *chip::DeviceLayer::NetworkCommissioning::SlWiFiDriver::GetInstance(),
        });
#else
    sRootNode = std::make_unique<chip::app::RootNode>(rootNodeContext);
#endif

    VerifyOrReturnError(sRootNode != nullptr, CHIP_ERROR_NO_MEMORY);

    chip::app::ConsecutiveEndpointIdAllocator rootAllocator(kRootEndpointId);
    ReturnErrorOnFailure(sRootNode->Register(rootAllocator, *sDataModelProvider));

    chip::app::DeviceFactory::GetInstance().Init(chip::app::DeviceFactory::Context{
        .groupDataProvider      = *groupDataProvider,
        .fabricTable            = chip::Server::GetInstance().GetFabricTable(),
        .timerDelegate          = sTimerDelegate,
        .storageDelegate        = storage,
        .diagnosticDataProvider = chip::DeviceLayer::GetDiagnosticDataProvider(),
        .platformManager        = chip::DeviceLayer::PlatformMgr(),
        .failSafeContext        = chip::Server::GetInstance().GetFailSafeContext(),
        .bindingTable           = chip::app::Clusters::Binding::Table::GetInstance(),
        .bindingManager         = chip::app::Clusters::Binding::Manager::GetInstance(),
    });

    auto & deviceFactory = chip::app::DeviceFactory::GetInstance();

#if defined(SL_MATTER_USE_SI70XX_SENSOR) && SL_MATTER_USE_SI70XX_SENSOR
    // Override the default simulated humidity/temperature sensors with
    // implementations backed by the on-board Si70xx driver so the device
    // reports real hardware readings when the board supports it.
    if constexpr (ALL_DEVICES_ENABLE_HUMIDITY_SENSOR)
    {
        // sTimerDelegate has static storage duration, so it is referenced directly rather than
        // captured (capturing statics is disallowed under -Werror per C++ rules).
        deviceFactory.RegisterCreator("humidity-sensor",
                                      []() { return std::make_unique<chip::app::Si70xxHumiditySensor>(sTimerDelegate); });
    }
    if constexpr (ALL_DEVICES_ENABLE_TEMPERATURE_SENSOR)
    {
        deviceFactory.RegisterCreator("temperature-sensor",
                                      []() { return std::make_unique<chip::app::Si70xxTemperatureSensor>(); });
    }
#endif // defined(SL_MATTER_USE_SI70XX_SENSOR) && SL_MATTER_USE_SI70XX_SENSOR

    ConsecutiveEndpointIdAllocator allocator(kDeviceEndpointId);
    TrackingEndpointIdAllocator trackingAllocator(allocator);

    // Log the list of device types this build can instantiate. This mirrors the
    // registry populated by DeviceFactory::Init() plus any RegisterCreator()
    // overrides above (e.g. Si70xx sensors).
    {
        auto supportedDeviceTypes = deviceFactory.SupportedDeviceTypes();
        ChipLogProgress(AppServer, "Supported device types (%u):",
                        static_cast<unsigned>(supportedDeviceTypes.size()));
        for (const auto & type : supportedDeviceTypes)
        {
            ChipLogProgress(AppServer, "  - %s", type.c_str());
        }
    }

    // Records (device type, primary endpoint id) for every device that is
    // successfully registered so a summary can be emitted below.
    std::vector<std::pair<std::string, chip::EndpointId>> registeredDevices;

    auto instantiateDevice = [&](const std::string & type) -> CHIP_ERROR {
        if (!deviceFactory.IsValidDevice(type))
        {
            ChipLogError(AppServer, "Invalid device type: %s, skipping", type.c_str());
            return CHIP_ERROR_INVALID_ARGUMENT;
        }
        auto device = deviceFactory.Create(type);
        VerifyOrReturnError(device != nullptr, CHIP_ERROR_NO_MEMORY);
        ReturnErrorOnFailure(device->Register(trackingAllocator, *sDataModelProvider));
        chip::EndpointId endpoint = trackingAllocator.TakeFirst();
        ChipLogProgress(AppServer, "Registered device type '%s' on endpoint %u", type.c_str(),
                        static_cast<unsigned>(endpoint));
        registeredDevices.emplace_back(type, endpoint);
        sConstructedDevices.push_back(std::move(device));
        return CHIP_NO_ERROR;
    };

    auto logRegisteredDevices = [&]() {
        ChipLogProgress(AppServer, "Instantiated device summary (%u):",
                        static_cast<unsigned>(registeredDevices.size()));
        for (const auto & entry : registeredDevices)
        {
            ChipLogProgress(AppServer, "  - '%s' on endpoint %u", entry.first.c_str(),
                            static_cast<unsigned>(entry.second));
        }
    };

    // Build-time device list (see all_devices_default_devices in enabled_devices.gni).
    // When the list is non-empty, it fully drives the device topology and the
    // KVS override / factory default is ignored — matching the "no shell needed"
    // build configuration.
    constexpr std::string_view kBuildTimeDevices{ ALL_DEVICES_DEFAULT_DEVICES };

    // Helper that (when this build is configured as an ICD) instantiates an extra
    // `power-source` endpoint so commissioners can display a battery level and
    // battery voltage. The primary device stays whatever the user selected.
    auto maybeAddPowerSource = [&]() -> CHIP_ERROR {
#if CHIP_CONFIG_ENABLE_ICD_SERVER
        if (!deviceFactory.IsValidDevice("power-source"))
        {
            ChipLogError(AppServer,
                         "ICD build requested a power-source endpoint but the device factory has no 'power-source' entry");
            return CHIP_NO_ERROR;
        }
        // Skip if the user already explicitly registered a power-source device.
        for (const auto & entry : registeredDevices)
        {
            if (entry.first == "power-source")
            {
                return CHIP_NO_ERROR;
            }
        }
        return instantiateDevice("power-source");
#else
        return CHIP_NO_ERROR;
#endif // CHIP_CONFIG_ENABLE_ICD_SERVER
    };

    if (!kBuildTimeDevices.empty())
    {
        std::string_view remaining = kBuildTimeDevices;
        while (!remaining.empty())
        {
            auto comma      = remaining.find(',');
            auto tokenView  = remaining.substr(0, comma);
            std::string tok(tokenView);
            ReturnErrorOnFailure(instantiateDevice(tok));
            if (comma == std::string_view::npos)
            {
                break;
            }
            remaining.remove_prefix(comma + 1);
        }
        ReturnErrorOnFailure(maybeAddPowerSource());
        logRegisteredDevices();
        return CHIP_NO_ERROR;
    }

    // No build-time selection: fall back to the KVS-stored device type (set via
    // the `devtype` shell command) or the factory default.
    std::string deviceType = deviceFactory.GetDefaultDevice();

    char storedDeviceType[64] = {};
    uint16_t storedLen        = sizeof(storedDeviceType);
    CHIP_ERROR storedErr      = storage.SyncGetKeyValue(chip::kDeviceTypeKey, storedDeviceType, storedLen);
    if (storedErr == CHIP_NO_ERROR && storedLen > 0)
    {
        deviceType = std::string(storedDeviceType, strnlen(storedDeviceType, storedLen));
    }

    if (!deviceFactory.IsValidDevice(deviceType))
    {
        ChipLogError(AppServer, "Invalid device type: %s, falling back to default", deviceType.c_str());
        deviceType = deviceFactory.GetDefaultDevice();
    }

    ReturnErrorOnFailure(instantiateDevice(deviceType));
    ReturnErrorOnFailure(maybeAddPowerSource());
    logRegisteredDevices();
    return CHIP_NO_ERROR;
}

chip::app::CodeDrivenDataModelProvider * AppTask::GetDataModelProvider()
{
    return sDataModelProvider.get();
}
