/*
 *
 *    Copyright (c) 2021 Project CHIP Authors
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

#include <stdint.h>

constexpr uint8_t kLogHeader     = 0x01; // ASCII Start of Heading
constexpr uint8_t kLogFooter     = 0x04; // ASCII End of Transmission
constexpr uint8_t kHeaderSize    = 1;
constexpr uint8_t kFooterSize    = 1;
constexpr uint8_t kEndOfLineSize = 2; // \r\n

#ifdef CHIP_SHELL_MAX_LINE_SIZE
#define MAX_BUFFER_SIZE CHIP_SHELL_MAX_LINE_SIZE
#else
#define MAX_BUFFER_SIZE 256
#endif
#define MAX_DMA_BUFFER_SIZE (MAX_BUFFER_SIZE / 2)

#ifdef __cplusplus
extern "C" {
#endif

void uartConsoleInit(void);
int16_t uartConsoleWrite(const char * Buf, uint16_t BufLength);
int16_t uartLogWrite(const char * log, uint8_t length, uint8_t category, uint64_t timestamp);
int16_t uartConsoleRead(char * Buf, uint16_t NbBytesToRead);
void uartFlushTxQueue(void);
void uartForceTransmit(const char * data, uint16_t length);

void uartMainLoop(void * args);

// Platform abstraction helpers for hardware-specific UART implementations
void uartCacheRxBytes(uint8_t * data, uint16_t length);
void uartSignalTxComplete(void);
void uartWaitForTxComplete(void);

// Implemented by in openthread code
#ifndef PW_RPC_ENABLED
extern void otPlatUartReceived(const uint8_t * aBuf, uint16_t aBufLength);
extern void otPlatUartSendDone(void);
extern void otSysEventSignalPending(void);
#endif

#ifdef __cplusplus
} // extern "C"
#endif
