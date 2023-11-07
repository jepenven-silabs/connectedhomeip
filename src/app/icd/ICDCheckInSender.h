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
#pragma once

#include <app/icd/ICDMonitoringTable.h>
#include <credentials/FabricTable.h>
#include <lib/address_resolve/AddressResolve.h>

namespace chip {
namespace app {

// Forward declaration for test    
class TestICDCheckInSender;
class ICDCheckInSender;

typedef void (*ReleaseCb)(ICDCheckInSender *);

/**
 * @brief ICD Manager is responsible of processing the events and triggering the correct action for an ICD
 */
class ICDCheckInSender : public AddressResolve::NodeListener
{
public:
    ICDCheckInSender(PersistentStorageDelegate * storage, FabricTable * fabricTable, Crypto::SymmetricKeystore * symmetricKeyStore, ReleaseCb cb = nullptr);
    ICDCheckInSender();
    ~ICDCheckInSender();

    void Init(PersistentStorageDelegate * storage, FabricTable * fabricTable, Crypto::SymmetricKeystore * symmetricKeyStore, ReleaseCb cb = nullptr);
    CHIP_ERROR RequestResolve(FabricIndex fabricIndex, NodeId checkInNodeID);

    // AddressResolve::NodeListener - notifications when dnssd finds a node IP address
    void OnNodeAddressResolved(const PeerId & peerId, const AddressResolve::ResolveResult & result) override;
    void OnNodeAddressResolutionFailed(const PeerId & peerId, CHIP_ERROR reason) override;

    NodeId mCurrentNodeId                          = kUndefinedNodeId;
    bool mResolveInProgress = false;

private:
    CHIP_ERROR SendCheckInMsg(ICDMonitoringEntry & entry, const Transport::PeerAddress & addr);

    inline void Release();


    // This is used when a node address is required.
    AddressResolve::NodeLookupHandle mAddressLookupHandle;

    

    PersistentStorageDelegate * mStorage           = nullptr;
    FabricTable * mFabricTable                     = nullptr;
    Crypto::SymmetricKeystore * mSymmetricKeystore = nullptr;
    ReleaseCb  mReleaseCb                          = nullptr;
    
};

} // namespace app
} // namespace chip
