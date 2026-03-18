/***************************************************************************
 * @file IMShellCommands.cpp
 * @brief Shell commands for the Interaction Model on the Silicon Labs platform.
 *******************************************************************************
 * # License
 * <b>Copyright 2025 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/

#include "IMShellCommands.h"

#include <app/InteractionModelEngine.h>
#include <app/ReadClient.h>
#include <app/ReadPrepareParams.h>
#include <app/server/Server.h>
#include <credentials/FabricTable.h>
#include <lib/core/ScopedNodeId.h>
#include <lib/shell/Command.h>
#include <lib/shell/Engine.h>
#include <lib/shell/commands/Help.h>
#include <lib/support/CHIPArgParser.hpp>
#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>

using namespace chip;
using namespace chip::app;
using Shell::Engine;
using Shell::shell_command_t;
using Shell::streamer_get;
using Shell::streamer_printf;

namespace {

Engine sShellIMSubCommands;

// ----------------------------------------------------------------------------
// Subscription callback and state
// ----------------------------------------------------------------------------

constexpr uint16_t kDefaultMinIntervalSeconds = 0;
constexpr uint16_t kDefaultMaxIntervalSeconds = 60;

class SubscriptionCallback : public ReadClient::Callback
{
public:
    void OnAttributeData(const ConcreteDataAttributePath & aPath, TLV::TLVReader * apData, const StatusIB & aStatus) override
    {
        if (aStatus.IsSuccess())
        {
            streamer_printf(streamer_get(), "IM: Attribute report ep=%u cluster=0x%08" PRIX32 " attr=0x%08" PRIX32 "\r\n",
                            aPath.mEndpointId, aPath.mClusterId, aPath.mAttributeId);
        }
        else
        {
            streamer_printf(streamer_get(), "IM: Attribute error ep=%u cluster=0x%08" PRIX32 " attr=0x%08" PRIX32 " status=0x%02x\r\n",
                            aPath.mEndpointId, aPath.mClusterId, aPath.mAttributeId,
                            to_underlying(aStatus.mStatus));
        }
    }

    void OnSubscriptionEstablished(SubscriptionId aSubscriptionId) override
    {
        streamer_printf(streamer_get(), "IM: Subscription established (id=0x%" PRIX32 ")\r\n", aSubscriptionId);
    }

    void OnError(CHIP_ERROR aError) override
    {
        streamer_printf(streamer_get(), "IM: Subscription error: %s\r\n", ErrorStr(aError));
    }

    void OnDone(ReadClient * apReadClient) override
    {
        streamer_printf(streamer_get(), "IM: Subscription terminated\r\n");
        mActiveClient = nullptr;
        Platform::Delete(apReadClient);
    }

    void OnDeallocatePaths(ReadPrepareParams && aReadPrepareParams) override
    {
        if (aReadPrepareParams.mpAttributePathParamsList != nullptr)
        {
            Platform::Delete(aReadPrepareParams.mpAttributePathParamsList);
            aReadPrepareParams.mpAttributePathParamsList    = nullptr;
            aReadPrepareParams.mAttributePathParamsListSize = 0;
        }
    }

    ReadClient * mActiveClient = nullptr;
};

SubscriptionCallback sSubscriptionCallback;

// ----------------------------------------------------------------------------
// Shell handlers
// ----------------------------------------------------------------------------

CHIP_ERROR HelpHandler(int argc, char ** argv)
{
    sShellIMSubCommands.ForEachCommand(Shell::PrintCommandHelp, nullptr);
    return CHIP_NO_ERROR;
}

CHIP_ERROR IMCommandHandler(int argc, char ** argv)
{
    if (argc == 0)
    {
        return HelpHandler(argc, argv);
    }

    return sShellIMSubCommands.ExecCommand(argc, argv);
}

CHIP_ERROR ShowFabricHandler(int argc, char ** argv)
{
    const FabricTable & fabricTable = Server::GetInstance().GetFabricTable();
    uint8_t fabricCount             = fabricTable.FabricCount();

    streamer_printf(streamer_get(), "Fabric count: %u\r\n", fabricCount);

    for (const FabricInfo & fabric : fabricTable)
    {
        streamer_printf(streamer_get(), "  [%u] FabricId=0x" ChipLogFormatX64 " NodeId=0x" ChipLogFormatX64 "\r\n",
                        fabric.GetFabricIndex(), ChipLogValueX64(fabric.GetFabricId()), ChipLogValueX64(fabric.GetNodeId()));
    }

    return CHIP_NO_ERROR;
}

CHIP_ERROR SubscribeHandler(int argc, char ** argv)
{
    // Usage: im subscribe <fabricIndex> <nodeId> <endpointId> <clusterId> <attributeId>
    VerifyOrReturnError(argc == 5, CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(),
                                        "Usage: im subscribe <fabricIndex> <nodeId> <endpointId> <clusterId> <attributeId>\r\n"););

    if (sSubscriptionCallback.mActiveClient != nullptr)
    {
        streamer_printf(streamer_get(), "A subscription is already active. Only one subscription at a time is supported.\r\n");
        return CHIP_ERROR_INCORRECT_STATE;
    }

    FabricIndex fabricIndex = 0;
    NodeId nodeId           = 0;
    EndpointId endpointId   = 0;
    ClusterId clusterId     = 0;
    AttributeId attributeId = 0;

    VerifyOrReturnError(ArgParser::ParseInt(argv[0], fabricIndex), CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(), "Invalid fabricIndex\r\n"););
    VerifyOrReturnError(ArgParser::ParseInt(argv[1], nodeId), CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(), "Invalid nodeId\r\n"););
    VerifyOrReturnError(ArgParser::ParseInt(argv[2], endpointId), CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(), "Invalid endpointId\r\n"););
    VerifyOrReturnError(ArgParser::ParseInt(argv[3], clusterId), CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(), "Invalid clusterId\r\n"););
    VerifyOrReturnError(ArgParser::ParseInt(argv[4], attributeId), CHIP_ERROR_INVALID_ARGUMENT,
                        streamer_printf(streamer_get(), "Invalid attributeId\r\n"););

    const FabricTable & fabricTable = Server::GetInstance().GetFabricTable();
    const FabricInfo * fabricInfo   = fabricTable.FindFabricWithIndex(fabricIndex);
    VerifyOrReturnError(fabricInfo != nullptr, CHIP_ERROR_INVALID_FABRIC_INDEX,
                        streamer_printf(streamer_get(), "Fabric index %u not found\r\n", fabricIndex););

    auto * pathParams = Platform::New<AttributePathParams>(endpointId, clusterId, attributeId);
    VerifyOrReturnError(pathParams != nullptr, CHIP_ERROR_NO_MEMORY);

    ReadPrepareParams readParams;
    readParams.mpAttributePathParamsList    = pathParams;
    readParams.mAttributePathParamsListSize = 1;
    readParams.mMinIntervalFloorSeconds     = kDefaultMinIntervalSeconds;
    readParams.mMaxIntervalCeilingSeconds   = kDefaultMaxIntervalSeconds;
    readParams.mKeepSubscriptions           = true;
    readParams.mIsFabricFiltered            = true;

    auto * readClient = Platform::New<ReadClient>(InteractionModelEngine::GetInstance(), nullptr, sSubscriptionCallback,
                                                  ReadClient::InteractionType::Subscribe);
    if (readClient == nullptr)
    {
        Platform::Delete(pathParams);
        return CHIP_ERROR_NO_MEMORY;
    }

    ScopedNodeId peer(nodeId, fabricIndex);

    CHIP_ERROR err = readClient->SendAutoResubscribeRequest(peer, std::move(readParams));
    if (err != CHIP_NO_ERROR)
    {
        streamer_printf(streamer_get(), "Failed to send subscribe request: %s\r\n", ErrorStr(err));
        Platform::Delete(readClient);
        return err;
    }

    sSubscriptionCallback.mActiveClient = readClient;
    streamer_printf(streamer_get(),
                    "Subscribing to node 0x" ChipLogFormatX64 " on fabric %u ep=%u cluster=0x%08" PRIX32 " attr=0x%08" PRIX32 "\r\n",
                    ChipLogValueX64(nodeId), fabricIndex, endpointId, clusterId, attributeId);

    return CHIP_NO_ERROR;
}

} // namespace

namespace IMShellCommands {

void RegisterCommands()
{
    static const Shell::Command sIMSubCommands[] = {
        { &HelpHandler, "help", "Usage: im <subcommand>" },
        { &ShowFabricHandler, "fabrics", "Show commissioned fabrics" },
        { &SubscribeHandler, "subscribe", "Subscribe to an attribute." },
          //"Subscribe to an attribute. Args: <fabricIndex> <nodeId> <endpointId> <clusterId> <attributeId>" },
    };
    static const Shell::Command sIMCmd = { &IMCommandHandler, "im", "Interaction Model commands." };

    sShellIMSubCommands.RegisterCommands(sIMSubCommands, MATTER_ARRAY_SIZE(sIMSubCommands));
    Engine::Root().RegisterCommands(&sIMCmd, 1);
}

} // namespace IMShellCommands
