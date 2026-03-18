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
#include "AppConfig.h"
#ifdef ENABLE_CHIP_SHELL
#include "MatterShell.h" // nogncheck
#endif
#include <cmsis_os2.h>
#include <platform/CHIPDeviceLayer.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>
#include <sl_cmsis_os2_common.h>

#include <platform/silabs/Logging.h>

// Ugly fix but seems that this defines only happens in 917 specific builds
#ifndef SLI_SI91X_MCU_INTERFACE
#define SLI_SI91X_MCU_INTERFACE 0
#endif


#ifdef __cplusplus
extern "C" {
#endif

#include "uart.h"
#include <stddef.h>
#include <string.h>

#define UART_CONSOLE_ERR -1 // Negative value in case of UART Console action failed. Triggers a failure for PW_RPC

typedef struct
{
    // The data buffer
    uint8_t * pBuffer;
    // The offset of the first item written to the list.
    volatile uint16_t Head;
    // The offset of the next item to be written to the list.
    volatile uint16_t Tail;
    // Maxium size of data that can be hold in buffer before overwriting
    uint16_t MaxSize;
} Fifo_t;

#if defined(SLI_SI91X_MCU_INTERFACE) && SLI_SI91X_MCU_INTERFACE
#define UART_MAX_QUEUE_SIZE 125
#else
#if CHIP_DETAIL_LOGGING
#define UART_MAX_QUEUE_SIZE 60
#else
#define UART_MAX_QUEUE_SIZE 20
#endif
#endif

#define UART_TX_MAX_BUF_LEN 100 // Just enough for the QR code

#define SILABS_TRUNCATED_TERMINATOR "....."

static constexpr uint32_t kUartTxCompleteFlag = 1;
static osThreadId_t sUartTaskHandle;
constexpr uint32_t kUartTaskSize = 1024;
static uint8_t uartStack[kUartTaskSize];
static osThread_t sUartTaskControlBlock;
constexpr osThreadAttr_t kUartTaskAttr = {
    .name       = "UART",
    .attr_bits  = osThreadDetached,
    .cb_mem     = &sUartTaskControlBlock,
    .cb_size    = osThreadCbSize,
    .stack_mem  = uartStack,
    .stack_size = kUartTaskSize,
#if defined(SLI_SI91X_MCU_INTERFACE) && SLI_SI91X_MCU_INTERFACE
    .priority = osPriorityBelowNormal, // for SOC, must be below Matter Task priority
#else
    .priority = osPriorityRealtime6, // Must be above Matter Task priority
#endif // SLI_SI91X_MCU_INTERFACE
};     // Must be above Matter Task priority

static uint32_t sMissedLogCount = 0; // Count of logs that were not sent to the UART due to queue full
static bool initInProgress = true; // To prevent having a huge buffer for Init sequence
namespace SilabsCoreLogs = chip::Logging::Platform;
// sizeof struct on arm is 4+8 +sizeof(data) so 12 + number of character in the string
typedef struct
{
    uint8_t data[UART_TX_MAX_BUF_LEN];
    uint64_t timestamp                   = 0;
    uint8_t length                       = 0;
    SilabsCoreLogs::LogCategory category = SilabsCoreLogs::kLog_None;
    bool isLog                           = false; // True if this is a log message, false if it is a command line message

} UartTxStruct_t;

constexpr size_t kLogMaxSize = kHeaderSize + SilabsCoreLogs::kTimeStampStringSize + SilabsCoreLogs::kMaxCategoryStrLen +
                          UART_TX_MAX_BUF_LEN + kEndOfLineSize +
                          kFooterSize; // Header + Timestamp + Category + Data + \r\n + Footer
                                      // SILABS_LOG_ENABLED

static osMessageQueueId_t sUartTxQueue;
static osMessageQueue_t sUartTxQueueStruct;
uint8_t sUartTxQueueBuffer[UART_MAX_QUEUE_SIZE * sizeof(UartTxStruct_t)];
constexpr osMessageQueueAttr_t kUartTxQueueAttr = { .cb_mem  = &sUartTxQueueStruct,
                                                    .cb_size = osMessageQueueCbSize,
                                                    .mq_mem  = sUartTxQueueBuffer,
                                                    .mq_size = sizeof(sUartTxQueueBuffer) };

// Rx buffer for the receive Fifo
static uint8_t sRxFifoBuffer[MAX_BUFFER_SIZE];
static Fifo_t sReceiveFifo;

static void uartSendBytes(uint8_t * data, uint16_t length);
static void uartTransmit(UartTxStruct_t * uart, bool force = false);

static bool InitFifo(Fifo_t * fifo, uint8_t * pDataBuffer, uint16_t bufferSize)
{
    if (fifo == NULL || pDataBuffer == NULL)
    {
        return false;
    }

    fifo->pBuffer = pDataBuffer;
    fifo->MaxSize = bufferSize;
    fifo->Tail = fifo->Head = 0;

    return true;
}

/*
 *   @brief Get the amount of unprocessed bytes in the fifo buffer
 *   @param Ptr to the fifo
 *   @return Nb of "unread" bytes available in the fifo
 */
static uint16_t AvailableDataCount(Fifo_t * fifo)
{
    uint16_t size = 0;

    // if equal there is no data return 0 directly
    if (fifo->Tail != fifo->Head)
    {
        // determine if a wrap around occurred to get the right data size avalaible.
        size = (fifo->Tail < fifo->Head) ? (fifo->MaxSize - fifo->Head + fifo->Tail) : (fifo->Tail - fifo->Head);
    }

    return size;
}

/*
 *   @brief Get the available space in the fifo buffer to insert new data
 *   @param Ptr to the fifo
 *   @return Nb of free bytes left in te buffer
 */
static uint16_t RemainingSpace(Fifo_t * fifo)
{
    return fifo->MaxSize - AvailableDataCount(fifo);
}

/*
 *   @brief Write data in the fifo as a circular buffer
 *   @param Ptr to the fifo, ptr of the data to write, nb of bytes to write
 */
static void WriteToFifo(Fifo_t * fifo, uint8_t * pDataToWrite, uint16_t SizeToWrite)
{
    VerifyOrDie(fifo != nullptr);
    VerifyOrDie(pDataToWrite != nullptr);
    VerifyOrDie(SizeToWrite <= fifo->MaxSize);

    // Overwrite is not allowed
    if (RemainingSpace(fifo) >= SizeToWrite)
    {
        uint16_t nBytesBeforWrap = (fifo->MaxSize - fifo->Tail);
        if (SizeToWrite > nBytesBeforWrap)
        {
            // The number of bytes to write is bigger than the remaining bytes
            // in the buffer, we have to wrap around
            memcpy(fifo->pBuffer + fifo->Tail, pDataToWrite, nBytesBeforWrap);
            memcpy(fifo->pBuffer, pDataToWrite + nBytesBeforWrap, SizeToWrite - nBytesBeforWrap);
        }
        else
        {
            memcpy(fifo->pBuffer + fifo->Tail, pDataToWrite, SizeToWrite);
        }

        fifo->Tail = (fifo->Tail + SizeToWrite) % fifo->MaxSize; // increment tail with wraparound
    }
}

/*
 *   @brief Write data in the fifo as a circular buffer
 *   @param Ptr to the fifo, ptr to contain the data to process, nb of bytes to pull from the fifo
 *   @return Nb of bytes that were retrieved.
 */
static uint16_t RetrieveFromFifo(Fifo_t * fifo, uint8_t * pData, uint16_t SizeToRead)
{
    VerifyOrDie(fifo != nullptr);
    VerifyOrDie(pData != nullptr);
    VerifyOrDie(SizeToRead <= fifo->MaxSize);

    uint16_t ReadSize        = std::min(SizeToRead, AvailableDataCount(fifo));
    uint16_t nBytesBeforWrap = (fifo->MaxSize - fifo->Head);

    if (ReadSize > nBytesBeforWrap)
    {
        memcpy(pData, fifo->pBuffer + fifo->Head, nBytesBeforWrap);
        memcpy(pData + nBytesBeforWrap, fifo->pBuffer, ReadSize - nBytesBeforWrap);
    }
    else
    {
        memcpy(pData, (fifo->pBuffer + fifo->Head), ReadSize);
    }

    fifo->Head = (fifo->Head + ReadSize) % fifo->MaxSize; // increment tail with wraparound

    return ReadSize;
}

/*
 *   @brief Cache received bytes into the receive fifo.
 *          Used by platform-specific UART implementations to store incoming data.
 */
void uartCacheRxBytes(uint8_t * data, uint16_t length)
{
    if (RemainingSpace(&sReceiveFifo) >= length)
    {
        WriteToFifo(&sReceiveFifo, data, length);
    }
}

void uartSignalTxComplete(void)
{
    osThreadFlagsSet(sUartTaskHandle, kUartTxCompleteFlag);
}

void uartWaitForTxComplete(void)
{
    osThreadFlagsWait(kUartTxCompleteFlag, osFlagsWaitAny, osWaitForever);
}

/*
 *   @brief Init the the UART for serial communication, Start DMA reception
 *          and init Fifo to handle the received data from this uart
 *
 *   @Note This UART is used for pigweed rpc
 */
void uartConsoleInit(void)
{
    if (sUartTaskHandle != NULL)
    {
        // Init was already done
        return;
    }

    sUartTxQueue    = osMessageQueueNew(UART_MAX_QUEUE_SIZE, sizeof(UartTxStruct_t), &kUartTxQueueAttr);
    sUartTaskHandle = osThreadNew(uartMainLoop, nullptr, &kUartTaskAttr);

    // Init a fifo for the data received on the uart
    InitFifo(&sReceiveFifo, sRxFifoBuffer, MAX_BUFFER_SIZE);

    VerifyOrDie(sUartTaskHandle != nullptr);
    VerifyOrDie(sUartTxQueue != nullptr);

    chip::DeviceLayer::Silabs::GetPlatform().UartConsoleInitHw();
}

/**
 * @brief Read the data available from the console Uart
 *
 * @param Buf Buffer that contains the data to write
 * @param BufLength number bytes to write
 * @return int16_t Amount of bytes written or ERROR (-1)
 */
int16_t uartConsoleWrite(const char * Buf, uint16_t BufLength)
{
    if (Buf == NULL || BufLength < 1 || BufLength > UART_TX_MAX_BUF_LEN)
    {
        return UART_CONSOLE_ERR;
    }

    if (NULL == sUartTxQueue)
    {
        // This is to prevent the first prompt from OTCLI to be rejected and to break the OTCli output
        uartConsoleInit();
    }

#ifdef PW_RPC_ENABLED
    // Pigweed Logger is already thread safe.
    uartForceTransmit(Buf, BufLength);
    return BufLength;
#endif

    UartTxStruct_t workBuffer;
    memcpy(workBuffer.data, Buf, BufLength);
    workBuffer.length = BufLength;

    // this is usually a command response. Wait on queue if full.
    if (osMessageQueuePut(sUartTxQueue, &workBuffer, osPriorityNormal, osWaitForever) == osOK)
    {
        return BufLength;
    }

    return UART_CONSOLE_ERR;
}

/**
 * @brief Write Logs to the Uart. Appends a return character
 *
 * @param log pointer to the logs
 * @param length number of bytes to write
 * @return int16_t Amount of bytes written or ERROR (-1)
 */
int16_t uartLogWrite(const char * log, uint8_t length, uint8_t category, uint64_t timestamp)
{
    if (log == NULL || length < 2)
    {
        return UART_CONSOLE_ERR;
    }

    



    bool truncated = false;
    if (length > UART_TX_MAX_BUF_LEN)
    {
        length    = UART_TX_MAX_BUF_LEN - sizeof(SILABS_TRUNCATED_TERMINATOR); // Reserve space for headers and ...
        truncated = true;
    }

    UartTxStruct_t workBuffer;
    memcpy(workBuffer.data, log, length);
    if (truncated)
    {
        memcpy(workBuffer.data + length, SILABS_TRUNCATED_TERMINATOR, sizeof(SILABS_TRUNCATED_TERMINATOR));
        length += sizeof(SILABS_TRUNCATED_TERMINATOR);
    }
    workBuffer.length    = length;
    workBuffer.isLog     = true; // This is a log message
    workBuffer.category  = SilabsCoreLogs::LogCategory(category);
    workBuffer.timestamp = timestamp;

    // Corner case here : During Init a task is created with max priority and fills the buffer queue
    // Solution check if we are in the Init phase. If so force transmit everything 
    if (initInProgress)
    {
        static osThreadId_t mainThreadId = NULL; 

        if (mainThreadId == NULL) {
            osThreadId_t threadId = osThreadGetId();    
            const char* name = osThreadGetName(threadId);
            if (strcmp(name, "main") == 0) {
                mainThreadId = threadId;
            }
        }
        else if (osThreadGetId() != mainThreadId)
        {
            initInProgress = false; // We are no longer in the init phase after the main thread has been deleted
        }
        uartTransmit(&workBuffer);
        return length;
    }
            
    // Don't wait when queue is full. Drop the log and return UART_CONSOLE_ERR
    if (osMessageQueuePut(sUartTxQueue, &workBuffer, osPriorityNormal, 0) == osOK)
    {
        return length;
    }
    else
    {
        sMissedLogCount++;
    }
    
    return UART_CONSOLE_ERR;
}

/**
 *   @brief Read the data available from the console Uart
 *   @param Buffer for the data to be read, number bytes to read.
 *   @return Amount of bytes that was read from the rx fifo or ERROR (-1)
 */
int16_t uartConsoleRead(char * Buf, uint16_t NbBytesToRead)
{
    chip::DeviceLayer::Silabs::GetPlatform().UartFlushRxBuffer();

    if (Buf == NULL || NbBytesToRead < 1)
    {
        return UART_CONSOLE_ERR;
    }

    return (int16_t) RetrieveFromFifo(&sReceiveFifo, (uint8_t *) Buf, NbBytesToRead);
}

void uartMainLoop(void * args)
{
    UartTxStruct_t workBuffer;
    while (1)
    {
        osStatus_t eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, osWaitForever);
        while (eventReceived == osOK)
        {
            uartTransmit(&workBuffer);
            if (sMissedLogCount)
            {
                // If there are missed logs, log the count

                workBuffer.length = sprintf(reinterpret_cast<char *>(workBuffer.data), "\r\nMissed Logs: %lu\r\n", sMissedLogCount);
                sMissedLogCount   = 0; // Reset the count after logging
                uartSendBytes(workBuffer.data, workBuffer.length);
            }
            eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, 0);
        }
    }
}

/**
 * @brief Send Bytes to UART. Delegates to platform-specific implementation.
 */
void uartSendBytes(uint8_t * data, uint16_t length)
{
    if (data == nullptr || length == 0)
    {
        return;
    }
    chip::DeviceLayer::Silabs::GetPlatform().UartSendBytes(data, length);
}

/**
 * @brief Flush the UART TX queue in a blocking manner.
 *   UART logs are non blocking, so we need to flush the queue here otherwise the logs will not get logged in case of a hard
 *   fault as they rely on the UART task to send the logs.
 */
void uartFlushTxQueue(void)
{
    UartTxStruct_t workBuffer;

    while (osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, 0) == osOK)
    {
        uartTransmit(&workBuffer, true);
    }
}

void uartTransmit(UartTxStruct_t * dataStruct, bool force)
{
    if (dataStruct == nullptr || dataStruct->length == 0)
    {
        return;
    }
    
    if(dataStruct->isLog)
    {
        #if defined(SILABS_LOG_ENABLED) && SILABS_LOG_ENABLED
        uint8_t timeStampString[SilabsCoreLogs::kTimeStampStringSize];
        uint8_t logWorkBuffer[kLogMaxSize]; // Header + Timestamp + Category + Data + \r\n + Footer
        SilabsCoreLogs::FormatTimestamp(reinterpret_cast<char *>(timeStampString), sizeof(timeStampString),
                                        dataStruct->timestamp);
        int32_t len = snprintf(reinterpret_cast<char *>(logWorkBuffer), sizeof(logWorkBuffer), "%c%s%s%.*s\r\n%c",
                                kLogHeader, timeStampString, SilabsCoreLogs::GetCategoryString(dataStruct->category),
                                dataStruct->length, dataStruct->data, kLogFooter);
        if (len > 0)
        {
            if (osKernelGetState() != osKernelRunning || initInProgress || force)
            {
                // either we are in the init phase or something bad happen.
                uartForceTransmit(reinterpret_cast<const char*>(logWorkBuffer), static_cast<uint16_t>(len));
            }
            else
            {
                uartSendBytes(logWorkBuffer, static_cast<uint16_t>(len));
            }
        }
        #endif // SILABS_LOG_ENABLED
    } 
    else
    {
        if (osKernelGetState() != osKernelRunning || initInProgress || force)
        {
            // either we are in the init phase or something bad happen.
            uartForceTransmit(reinterpret_cast<const char*>(dataStruct->data), static_cast<uint16_t>(dataStruct->length));
        }
        else
        {
            uartSendBytes(dataStruct->data, static_cast<uint16_t>(dataStruct->length));
        }
    }
}

void uartForceTransmit(const char * data, uint16_t length)
{
    VerifyOrReturn(data != nullptr && length > 0);
    chip::DeviceLayer::Silabs::GetPlatform().UartForceTransmit(data, length);
}

#ifdef __cplusplus
}
#endif
