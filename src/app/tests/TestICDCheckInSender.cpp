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

#include <app/icd/ICDMonitoringTable.h>
#include <crypto/CHIPCryptoPAL.h>
#include <lib/core/CHIPError.h>
#include <lib/support/DefaultStorageKeyAllocator.h>
#include <lib/support/TestPersistentStorageDelegate.h>
#include <lib/support/UnitTestRegistration.h>
#include <nlunit-test.h>

#include <crypto/DefaultSessionKeystore.h>
#include <app/icd/ICDCheckInSender.h>

using namespace chip;

using TestSessionKeystoreImpl = Crypto::DefaultSessionKeystore;

namespace {

constexpr uint16_t kMaxTestClients1     = 2;
constexpr uint16_t kMaxTestClients2     = 1;
constexpr FabricIndex kTestFabricIndex1 = 1;
constexpr FabricIndex kTestFabricIndex2 = kMaxValidFabricIndex;
constexpr uint64_t kClientNodeId11      = 0x100001;
constexpr uint64_t kClientNodeId12      = 0x100002;
constexpr uint64_t kClientNodeId13      = 0x100003;
constexpr uint64_t kClientNodeId21      = 0x200001;
constexpr uint64_t kClientNodeId22      = 0x200002;

constexpr uint8_t kKeyBuffer0a[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
constexpr uint8_t kKeyBuffer0b[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
                                     0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

constexpr uint8_t kKeyBuffer1a[] = {
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f
};
constexpr uint8_t kKeyBuffer1b[] = {
    0xf1, 0xe1, 0xd1, 0xc1, 0xb1, 0xa1, 0x91, 0x81, 0x71, 0x61, 0x51, 0x14, 0x31, 0x21, 0x11, 0x01
};
constexpr uint8_t kKeyBuffer2a[] = {
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f
};
constexpr uint8_t kKeyBuffer2b[] = {
    0xf2, 0xe2, 0xd2, 0xc2, 0xb2, 0xa2, 0x92, 0x82, 0x72, 0x62, 0x52, 0x42, 0x32, 0x22, 0x12, 0x02
};
constexpr uint8_t kKeyBuffer3a[] = {
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f
};
// constexpr uint8_t kKeyBuffer3b[] = { 0xf3, 0xe3, 0xd3, 0xc3, 0xb3, 0xa3, 0x93, 0x83, 0x73, 0x63, 0x53, 0x14, 0x33, 0x23, 0x13,
// 0x03 };

void TestEntryKeyFunctions(nlTestSuite * aSuite, void * aContext)
{
    TestSessionKeystoreImpl keystore;
    ICDMonitoringEntry entry(&keystore);

    // Test Setting Key
    NL_TEST_ASSERT(aSuite, CHIP_NO_ERROR == entry.SetKey(ByteSpan(kKeyBuffer1a)));

    // Test Setting Key again
    NL_TEST_ASSERT(aSuite, CHIP_NO_ERROR == entry.SetKey(ByteSpan(kKeyBuffer1b)));

    // Test Comparing Key
    NL_TEST_ASSERT(aSuite, !entry.IsKeyEquivalent(ByteSpan(kKeyBuffer1a)));

    NL_TEST_ASSERT(aSuite, entry.IsKeyEquivalent(ByteSpan(kKeyBuffer1b)));

    // Test Deleting Key
    NL_TEST_ASSERT(aSuite, CHIP_NO_ERROR == entry.DeleteKey());
}

void TestSaveAndLoadRegistrationValue(nlTestSuite * aSuite, void * aContext)
{
    TestPersistentStorageDelegate storage;
    TestSessionKeystoreImpl keystore;
    ICDMonitoringTable mMonitoringTable(storage, kTestFabricIndex1, kMaxTestClients1, &keystore);
    ICDMonitoringEntry entry(&keystore);
    CHIP_ERROR err;

    // Insert first entry
    entry.checkInNodeID    = kClientNodeId11;
    entry.monitoredSubject = kClientNodeId12;
    NL_TEST_ASSERT(aSuite, CHIP_NO_ERROR == entry.SetKey(ByteSpan(kKeyBuffer1a)));
    err = mMonitoringTable.Set(0, entry);
    NL_TEST_ASSERT(aSuite, CHIP_ERROR_INVALID_ARGUMENT == err);

}

void TestSaveAllInvalidRegistrationValues(nlTestSuite * aSuite, void * aContext)
{

}

void TestSaveLoadRegistrationValueForMultipleFabrics(nlTestSuite * aSuite, void * aContext)
{

}

void TestDeleteValidEntryFromStorage(nlTestSuite * aSuite, void * context)
{

}

} // namespace

/**
 *  Set up the test suite.
 */
int Test_Setup(void * inContext)
{
    CHIP_ERROR err;
    // Setup ICDMonitoringTable
    TestPersistentStorageDelegate storage;
    TestSessionKeystoreImpl keystore;
    ICDMonitoringTable table(storage, kTestFabricIndex1, kMaxTestClients1, &keystore);
    ICDMonitoringEntry entry(&keystore);

    entry.checkInNodeID    = kClientNodeId11;
    entry.monitoredSubject = kClientNodeId12;
    entry.SetKey(ByteSpan(kKeyBuffer1a));
    err = table.Set(0, entry);

    return (err == CHIP_NO_ERROR) ? SUCCESS : FAILURE;
}

int TestClientMonitoringRegistrationTable()
{
    static nlTest sTests[] = { NL_TEST_DEF("TestEntryKeyFunctions", TestEntryKeyFunctions),
                               NL_TEST_DEF("TestSaveAndLoadRegistrationValue", TestSaveAndLoadRegistrationValue),
                               NL_TEST_DEF("TestSaveAllInvalidRegistrationValues", TestSaveAllInvalidRegistrationValues),
                               NL_TEST_DEF("TestSaveLoadRegistrationValueForMultipleFabrics",
                                           TestSaveLoadRegistrationValueForMultipleFabrics),
                               NL_TEST_DEF("TestDeleteValidEntryFromStorage", TestDeleteValidEntryFromStorage),
                               NL_TEST_SENTINEL() };

    nlTestSuite cmSuite = { "TestClientMonitoringRegistrationTable", &sTests[0], &Test_Setup, nullptr };

    nlTestRunner(&cmSuite, nullptr);
    return (nlTestRunnerStats(&cmSuite));
}

CHIP_REGISTER_TEST_SUITE(TestClientMonitoringRegistrationTable)
