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
#include <sl_cmsis_os2_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "uart.h"
#include <stddef.h>
#include <string.h>

#define UART_CONSOLE_ERR -1 // Negative value in case of UART Console action failed. Triggers a failure for PW_RPC
#ifdef CHIP_SHELL_MAX_LINE_SIZE
#define MAX_BUFFER_SIZE CHIP_SHELL_MAX_LINE_SIZE
#else
#define MAX_BUFFER_SIZE 256
#endif
#define MAX_DMA_BUFFER_SIZE (MAX_BUFFER_SIZE / 2)

#if SLI_SI91X_MCU_INTERFACE
#include "USART.h"
#if defined(SL_SI91X_BOARD_INIT)
#include "rsi_board.h"
#endif // SL_SI91X_BOARD_INIT
#include "rsi_debug.h"
#include "rsi_rom_egpio.h"
#endif

#if SL_WIFI
#include <platform/silabs/wifi/ncp/spi_multiplex.h>
#endif // SL_WIFI

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif




#include "sl_iostream.h"
#include "sl_iostream_handles.h"


// uart transmit
#if SILABS_LOG_OUT_UART
#define UART_MAX_QUEUE_SIZE 5
#endif

#define UART_TX_MAX_BUF_LEN (255)

static constexpr uint32_t kUartTxCompleteFlag = 1;
static osThreadId_t sUartTaskHandle;
constexpr uint32_t kUartTaskSize = 1024;
static uint8_t uartStack[kUartTaskSize];
static osThread_t sUartTaskControlBlock;
constexpr osThreadAttr_t kUartTaskAttr = { .name       = "UART",
                                           .attr_bits  = osThreadDetached,
                                           .cb_mem     = &sUartTaskControlBlock,
                                           .cb_size    = osThreadCbSize,
                                           .stack_mem  = uartStack,
                                           .stack_size = kUartTaskSize,
                                           .priority   = osPriorityRealtime1 };

typedef struct
{
    char data[UART_TX_MAX_BUF_LEN];
    uint8_t length = 0;
    bool isLog = false;
} UartTxStruct_t;

static osMessageQueueId_t sUartTxQueue;
static osMessageQueue_t sUartTxQueueStruct;
uint8_t sUartTxQueueBuffer[UART_MAX_QUEUE_SIZE * sizeof(UartTxStruct_t)];
constexpr osMessageQueueAttr_t kUartTxQueueAttr = { .cb_mem  = &sUartTxQueueStruct,
                                                    .cb_size = osMessageQueueCbSize,
                                                    .mq_mem  = sUartTxQueueBuffer,
                                                    .mq_size = sizeof(sUartTxQueueBuffer) };

static uint32_t sNumberOfMsgDropped = 0; // Number of messages dropped in the UART Tx queue
static uint8_t sBiggestLogSize = 0;
static uint8_t newline[] = "\r\n";


void uartConsoleInit(void)
{
    if (sUartTaskHandle != NULL)
    {
        // Init was already done
        return;
    }

    sUartTxQueue    = osMessageQueueNew(UART_MAX_QUEUE_SIZE, sizeof(UartTxStruct_t), &kUartTxQueueAttr);
    sUartTaskHandle = osThreadNew(uartMainLoop, nullptr, &kUartTaskAttr);

    sl_iostream_set_default(sl_iostream_vcom_handle);

    VerifyOrDie(sUartTaskHandle != nullptr);
    VerifyOrDie(sUartTxQueue != nullptr);

}

#if SLI_SI91X_MCU_INTERFACE
void cache_uart_rx_data(char character)
{
    // if (RemainingSpace(&sReceiveFifo) >= 1)
    // {
    //     WriteToFifo(&sReceiveFifo, (uint8_t *) &character, 1);
    // }
#ifdef ENABLE_CHIP_SHELL
    chip::NotifyShellProcess();
#endif // ENABLE_CHIP_SHELL
}
#endif // SLI_SI91X_MCU_INTERFACE


/**
 * @brief Read the data available from the console Uart
 *
 * @param Buf Buffer that contains the data to write
 * @param BufLength number bytes to write
 * @return int16_t Amount of bytes written or ERROR (-1)
 */
int16_t uartConsoleWrite(const char * Buf, uint16_t BufLength)
{
    if (Buf == NULL || BufLength < 1)
    {
        return UART_CONSOLE_ERR;
    }

    if (BufLength > UART_TX_MAX_BUF_LEN)
    {
        // If the buffer is too long, truncate it to fit in the buffer
        BufLength = UART_TX_MAX_BUF_LEN;
    }

    if (NULL == sUartTxQueue)
    {
        // This is to prevent the first prompt from OTCLI to be rejected and to break the OTCli output
        uartConsoleInit();
    }

#ifdef PW_RPC_ENABLED
    // TODO replace with iostream api
    // UARTDRV_ForceTransmit(vcom_handle, (uint8_t *) Buf, BufLength);
    return BufLength;
#endif

#if SLI_SI91X_MCU_INTERFACE == 0
    // TODO replace with iostream api
    // UARTDRV_Receive(vcom_handle, sRxDmaBuffer, MAX_DMA_BUFFER_SIZE, UART_rx_callback);
    // TODO replace with iostream api
    // UARTDRV_Receive(vcom_handle, sRxDmaBuffer2, MAX_DMA_BUFFER_SIZE, UART_rx_callback);
#endif

#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
    // TODO replace with iostream api
    // UARTDRV_ForceTransmit(vcom_handle, reinterpret_cast<uint8_t *>(workBuffer.data), workBuffer.length);
    // TODO replace with iostream api
    // UARTDRV_ForceTransmit(vcom_handle, newline, 2);
#endif

#if SLI_SI91X_MCU_INTERFACE == 0
    CORE_ATOMIC_SECTION(
        // TODO replace with iostream api
        // UARTDRV_GetReceiveStatus(vcom_handle, &data, &count, &remaining);
        // if (count > lastCount) {
            // WriteToFifo(&sReceiveFifo, data + lastCount, count - lastCount);
            // lastCount = count;
        // }
    )
#endif

#if SLI_SI91X_MCU_INTERFACE == 0
    // TODO replace with iostream api
    // UARTDRV_Receive(vcom_handle, data, transferCount, UART_rx_callback);
#endif
}

/**
 * @brief Write Logs to the Uart. Appends a return character
 *
 * @param log pointer to the logs
 * @param length number of bytes to write
 * @return int16_t Amount of bytes written or ERROR (-1)
 */
int16_t uartLogWrite(const char * log, uint16_t length)
{
    if (log == NULL || length < 1 )
    {
        return UART_CONSOLE_ERR;
    }

    bool appendDotDotDot = false;

    if (length > UART_TX_MAX_BUF_LEN)
    {
        // If the log is too long, truncate it to fit in the buffer
        length = UART_TX_MAX_BUF_LEN - 3; // to add ...
        appendDotDotDot = true;
    }

    if (sBiggestLogSize < length)
    {
        sBiggestLogSize = length;
    }

    UartTxStruct_t workBuffer;
    workBuffer.isLog = true;
    workBuffer.length = length;
    memcpy(workBuffer.data, log, length);

    if (appendDotDotDot)
    {
        memcpy(workBuffer.data + length, "...", 3);
        workBuffer.length = length + 3;
    }

    // Don't wait when queue is full. Drop the log and return UART_CONSOLE_ERR
    if (osMessageQueuePut(sUartTxQueue, &workBuffer, osPriorityNormal, 0) == osOK)
    {
        return length;
    } else 
    {
        sNumberOfMsgDropped++;
    }

    return UART_CONSOLE_ERR;
}

/*
 *   @brief Read the data available from the console Uart
 *   @param Buffer for the data to be read, number bytes to read.
 *   @return Amount of bytes that was read from the rx fifo or ERROR (-1)
 */
int16_t uartConsoleRead(char * Buf, uint16_t NbBytesToRead)
{
#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
    // EUSART_INT_ENABLE(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, EUSART_IF_RXFL);
#endif

    if (Buf == NULL || NbBytesToRead < 1)
    {
        return UART_CONSOLE_ERR;
    }
#if SLI_SI91X_MCU_INTERFACE == 0
    // uint8_t * data;
    // if (NbBytesToRead > AvailableDataCount(&sReceiveFifo))
    // {
    //     UARTDRV_Count_t count, remaining;
    //     // Not enough data available in the fifo for the read size request
    //     // If there is data available in dma buffer, get it now.
    //     // CORE_ATOMIC_SECTION(UARTDRV_GetReceiveStatus(vcom_handle, &data, &count, &remaining); if (count > lastCount) {
    //     //     WriteToFifo(&sReceiveFifo, data + lastCount, count - lastCount);
    //     //     lastCount = count;
    //     })
    // }
#endif // SLI_SI91X_MCU_INTERFACE == 0
    return 0;
    // return (int16_t) RetrieveFromFifo(&sReceiveFifo, (uint8_t *) Buf, NbBytesToRead);
}

void uartMainLoop(void * args)
{
    UartTxStruct_t workBuffer;
    
    while (1)
    {
        osStatus_t eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, osWaitForever);
        while (eventReceived == osOK)
        {
            sl_iostream_write(sl_iostream_vcom_handle, workBuffer.data, workBuffer.length);
            if(workBuffer.isLog)
            {
                // If this is a log, append a new line to the end of the log
                memcpy(workBuffer.data, newline,2);
                workBuffer.length = 2; 
                sl_iostream_write(sl_iostream_vcom_handle, workBuffer.data, workBuffer.length);
            }

            if (sNumberOfMsgDropped)
            {
                int32_t nb = sprintf(workBuffer.data, "\r\n\r\n%ld Logs dropped. Biggest Log size %d  !!! \r\n\r\n", sNumberOfMsgDropped, sBiggestLogSize);
                if (nb > 0) 
                {
                    sNumberOfMsgDropped=0;
                    workBuffer.length = static_cast<uint8_t>(nb);
                    sl_iostream_write(sl_iostream_vcom_handle, workBuffer.data, workBuffer.length);
                }
            }
            eventReceived = osMessageQueueGet(sUartTxQueue, &workBuffer, nullptr, 0);
        }
    }
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
#if SLI_SI91X_MCU_INTERFACE
        // ensuring null termination of buffer
        if (workBuffer.length < MATTER_ARRAY_SIZE(workBuffer.data) && workBuffer.data[workBuffer.length - 1] != '\0')
        {
            workBuffer.data[workBuffer.length] = '\0';
        }
        else
        {
            workBuffer.data[MATTER_ARRAY_SIZE(workBuffer.data) - 1] = '\0';
        }
        Board_UARTPutSTR(workBuffer.data);
#else
        // TODO add sl_iostream_write() to silabs platform
        // UARTDRV_ForceTransmit(vcom_handle, reinterpret_cast<uint8_t *>(workBuffer.data), workBuffer.length);
#endif
    }
}

#ifdef __cplusplus
}
#endif
