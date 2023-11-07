/*
 *
 *    Copyright (c) 2023 Project CHIP Authors
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

#include "ICDCheckInSender.h"

#include <app/InteractionModelEngine.h>
#include <system/SystemPacketBuffer.h>

#include <protocols/secure_channel/CheckinMessage.h>

#include <lib/dnssd/Resolver.h>

namespace chip {
namespace app {

using namespace Protocols::SecureChannel;

ICDCheckInSender::ICDCheckInSender(PersistentStorageDelegate * storage, FabricTable * fabricTable,
                            Crypto::SymmetricKeystore * symmetricKeyStore, ReleaseCb cb)
{
    VerifyOrDie(storage != nullptr);
    VerifyOrDie(fabricTable != nullptr);
    VerifyOrDie(symmetricKeyStore != nullptr);    

    mStorage           = storage;
    mFabricTable       = fabricTable;
    mSymmetricKeystore = symmetricKeyStore;
    mReleaseCb        = cb;

    mAddressLookupHandle.SetListener(this);

}

ICDCheckInSender::ICDCheckInSender()
{
    mAddressLookupHandle.SetListener(this);
}

ICDCheckInSender::~ICDCheckInSender()
{
    // TODO
}

void ICDCheckInSender::Init(PersistentStorageDelegate * storage, FabricTable * fabricTable,
                            Crypto::SymmetricKeystore * symmetricKeyStore, ReleaseCb cb)
{
    VerifyOrDie(storage != nullptr);
    VerifyOrDie(fabricTable != nullptr);
    VerifyOrDie(symmetricKeyStore != nullptr);

    mStorage           = storage;
    mFabricTable       = fabricTable;
    mSymmetricKeystore = symmetricKeyStore;
    mReleaseCb        = cb;
}

void ICDCheckInSender::Release()
{
    if (mReleaseCb != nullptr)
    {
        mReleaseCb(this);
    }
}

void ICDCheckInSender::OnNodeAddressResolved(const PeerId & peerId, const AddressResolve::ResolveResult & result)
{
    mResolveInProgress = false;

    ChipLogProgress(AppServer, "Node Address resolution Succeed!!! Node ID %lu has address", peerId.GetNodeId());
    ICDMonitoringEntry entry(mSymmetricKeystore);

    const FabricInfo * f = mFabricTable->FindFabricWithCompressedId(peerId.GetCompressedFabricId());
    ICDMonitoringTable table(*mStorage, f->GetFabricIndex(), CHIP_CONFIG_ICD_CLIENTS_SUPPORTED_PER_FABRIC, mSymmetricKeystore);
    table.Find(peerId.GetNodeId(), entry);

    SendCheckInMsg(entry, result.address);
    //Release();
}

void ICDCheckInSender::OnNodeAddressResolutionFailed(const PeerId & peerId, CHIP_ERROR reason)
{
    mResolveInProgress = false;
    ChipLogProgress(AppServer, "Node Address resolution failed for ICD Check-In with Node ID %lu", peerId.GetNodeId());

    //Release();
}

CHIP_ERROR ICDCheckInSender::SendCheckInMsg(ICDMonitoringEntry & entry, const Transport::PeerAddress & addr)
{
    // ByteSpan appData;
    uint8_t b[CheckinMessage::sMinPayloadSize] = { "123456789" };
    MutableByteSpan output{ b };
    CHIP_ERROR err;

    // TODO retrieve Check-in counter
    CounterType counter = 0;

    ChipLogProgress(AppServer, "Address resolution completed Sending Check-In Messages");

    // Prepare Check-in payload
    err = CheckinMessage::GenerateCheckinMessagePayload(entry.key, counter, ByteSpan(), output);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(AppServer, "Failed to prepare Check-In Payload");
        return err;
    }

    if (InteractionModelEngine::GetInstance() == nullptr) {
        return CHIP_ERROR_INTERNAL;
    }

    if (InteractionModelEngine::GetInstance()->GetExchangeManager() == nullptr) {
        return CHIP_ERROR_INTERNAL;
    }

    if (InteractionModelEngine::GetInstance()
            ->GetExchangeManager()
            ->GetSessionManager() == nullptr) {
        return CHIP_ERROR_INTERNAL;
    }


    Messaging::ExchangeContext * exchangeContext = InteractionModelEngine::GetInstance()->GetExchangeManager()->NewContext(
        InteractionModelEngine::GetInstance()
            ->GetExchangeManager()
            ->GetSessionManager()
            ->CreateUnauthenticatedSession(addr, GetLocalMRPConfig().ValueOr(GetDefaultMRPConfig()))
            .Value(),
        nullptr);

    VerifyOrReturnError(exchangeContext != nullptr, CHIP_ERROR_NO_MEMORY);

    System::PacketBufferHandle buffer = MessagePacketBuffer::NewWithData(output.data(), output.size());

    err = exchangeContext->SendMessage(MsgType::ICD_CheckIn, std::move(buffer), Messaging::SendMessageFlags::kNoAutoRequestAck);

    // TBD why this is causing a seg fault
    //exchangeContext->Close();

    return err;
}

CHIP_ERROR ICDCheckInSender::RequestResolve(FabricIndex fabricIndex, NodeId checkInNodeID)
{
    VerifyOrReturnError(mFabricTable != nullptr, CHIP_ERROR_INTERNAL);
    const FabricInfo * fabricInfo = mFabricTable->FindFabricWithIndex(fabricIndex);
    PeerId peerId(fabricInfo->GetCompressedFabricId(), checkInNodeID);
    ChipLogProgress(AppServer, "Going to lookUp Node ID");
    AddressResolve::NodeLookupRequest request(peerId);
    mCurrentNodeId = checkInNodeID;

    CHIP_ERROR err = AddressResolve::Resolver::Instance().LookupNode(request, mAddressLookupHandle);

    if (err == CHIP_NO_ERROR)
    {
        mResolveInProgress = true;
    }

    return err;
}

} // namespace app
} // namespace chip
