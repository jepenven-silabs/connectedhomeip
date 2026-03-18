/*
 *    Copyright (c) 2023 Project CHIP Authors
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

#include <em_device.h>
#include <lib/support/CodeUtils.h>
#include <platform/silabs/platformAbstraction/SilabsPlatform.h>
#if defined(_SILICON_LABS_32B_SERIES_2)
#include "em_msc.h"
#include "em_rmu.h"
#elif defined(_SILICON_LABS_32B_SERIES_3)
#include "sl_hal_emu.h"
#include "sl_se_manager.h"
#include "sl_se_manager_types.h"
#include <sl_se_manager_extmem.h>
#endif // _SILICON_LABS_32B_SERIES_2

// Use sl_system for projects upgraded to 2025.6, identified by the presence of SL_CATALOG_CUSTOM_MAIN_PRESENT
#if defined(SL_CATALOG_CUSTOM_MAIN_PRESENT)
#include "sl_system_kernel.h"
#endif

#if SL_MATTER_DEBUG_WATCHDOG_ENABLE
#include "sl_clock_manager.h"
#include "sl_hal_wdog.h"
#endif // SL_MATTER_DEBUG_WATCHDOG_ENABLE

#ifdef ENABLE_WSTK_LEDS
extern "C" {
#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
#include "sl_simple_rgb_pwm_led.h"
#include "sl_simple_rgb_pwm_led_instances.h"
#define SL_SIMPLE_LED_INSTANCE(x) (&sl_simple_rgb_pwm_led_rgb_led0)
#define SL_SIMPLE_LED_COUNT 1
#define SL_LED_INIT_INTANCES() sl_simple_rgb_pwm_led_init_instances();
#define SL_LED_GET_STATE(x) sl_led_get_state(&(x->led_common))
#define SL_LED_TURN_ON(x) sl_led_turn_on(&(x->led_common))
#define SL_LED_TURN_OFF(x) sl_led_turn_off(&(x->led_common))
#define SL_LED_TOGGLE(x) sl_led_toggle(&(x->led_common))
#else
#include "sl_simple_led_instances.h"
#define SL_LED_INIT_INTANCES() sl_simple_led_init_instances();
#define SL_LED_GET_STATE(x) sl_simple_led_get_state(const_cast<sl_led_t *>(x)->context)
#define SL_LED_TURN_ON(x) sl_simple_led_turn_on(const_cast<sl_led_t *>(x)->context)
#define SL_LED_TURN_OFF(x) sl_simple_led_turn_off(const_cast<sl_led_t *>(x)->context)
#define SL_LED_TOGGLE(x) sl_simple_led_toggle(const_cast<sl_led_t *>(x)->context)

#endif // (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
}

#endif

#ifdef SL_CATALOG_SIMPLE_BUTTON_PRESENT
#include "sl_simple_button_instances.h"
#endif

extern "C" {
#include <mbedtls/platform.h>

#if CHIP_ENABLE_OPENTHREAD
#include "platform-efr32.h"

#if defined(OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE) && OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE
#include "openthread/heap.h"
#endif // OPENTHREAD_CONFIG_HEAP_EXTERNAL_ENABLE

#endif // CHIP_ENABLE_OPENTHREAD

#include "sl_component_catalog.h"
#include "sl_mbedtls.h"
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#ifdef SL_CATALOG_CLI_PRESENT
#include "sl_iostream.h"
#include "sl_iostream_stdio.h"
#endif // SL_CATALOG_CLI_PRESENT
#include "uart.h"
#if (_SILICON_LABS_32B_SERIES < 3)
#include "em_core.h"
#include "em_usart.h"
#else
#include "sl_hal_eusart.h"
#endif //_SILICON_LABS_32B_SERIES
#include "uartdrv.h"
#ifdef SL_BOARD_NAME
#include "sl_board_control.h"
#endif
#include "sl_uartdrv_instances.h"
#if defined(SL_WIFI) && SL_WIFI
#include <platform/silabs/wifi/ncp/spi_multiplex.h>
#endif // SL_WIFI
#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
#include "sl_uartdrv_eusart_vcom_config.h"
#endif
#ifdef SL_CATALOG_UARTDRV_USART_PRESENT
#include "sl_uartdrv_usart_vcom_config.h"
#endif // SL_CATALOG_UARTDRV_USART_PRESENT
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
#include "sl_power_manager.h"
#endif
#endif // SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) || defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI)
       // && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

#ifdef SL_CATALOG_SYSTEMVIEW_TRACE_PRESENT
#include "SEGGER_SYSVIEW.h"
#endif
}

#if SILABS_LOG_ENABLED
#include "silabs_utils.h"
#endif

// UART hardware macros and definitions
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

#ifdef ENABLE_CHIP_SHELL
#include "MatterShell.h" // nogncheck
#endif

#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
#define HELPER1(x) EUSART##x##_RX_IRQn
#else
#define HELPER1(x) USART##x##_RX_IRQn
#endif

#define HELPER2(x) HELPER1(x)

#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
#define HELPER3(x) EUSART##x##_RX_IRQHandler
#else
#define HELPER3(x) USART##x##_RX_IRQHandler
#endif

#define HELPER4(x) HELPER3(x)

// On MG24 boards VCOM runs on the EUSART device, MG12 uses the UART device
#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
#define USART_IRQ HELPER2(SL_UARTDRV_EUSART_VCOM_PERIPHERAL_NO)
#define USART_IRQHandler HELPER4(SL_UARTDRV_EUSART_VCOM_PERIPHERAL_NO)
#define vcom_handle sl_uartdrv_eusart_vcom_handle

#if (_SILICON_LABS_32B_SERIES < 3)
#define EUSART_INT_ENABLE EUSART_IntEnable
#define EUSART_INT_DISABLE EUSART_IntDisable
#define EUSART_INT_CLEAR EUSART_IntClear
#define EUSART_CLEAR_RX(x) (void) x
#define EUSART_GET_PENDING_INT EUSART_IntGet
#define EUSART_ENABLE(eusart) EUSART_Enable(eusart, eusartEnable)
#else
#define EUSART_INT_ENABLE sl_hal_eusart_enable_interrupts
#define EUSART_INT_DISABLE sl_hal_eusart_disable_interrupts
#define EUSART_INT_SET sl_hal_eusart_set_interrupts
#define EUSART_INT_CLEAR sl_hal_eusart_clear_interrupts
#define EUSART_CLEAR_RX sl_hal_eusart_clear_rx
#define EUSART_GET_PENDING_INT sl_hal_eusart_get_pending_interrupts
#define EUSART_ENABLE(eusart)                                                                                                      \
    {                                                                                                                              \
        sl_hal_eusart_enable(eusart);                                                                                              \
        sl_hal_eusart_enable_tx(eusart);                                                                                           \
        sl_hal_eusart_enable_rx(eusart);                                                                                           \
    }
#endif //_SILICON_LABS_32B_SERIES

#else
#define USART_IRQ HELPER2(SL_UARTDRV_USART_VCOM_PERIPHERAL_NO)
#define USART_IRQHandler HELPER4(SL_UARTDRV_USART_VCOM_PERIPHERAL_NO)
#define vcom_handle sl_uartdrv_usart_vcom_handle
#endif // SL_CATALOG_UARTDRV_EUSART_PRESENT

namespace {
uint8_t sRxDmaBuffer[MAX_DMA_BUFFER_SIZE]  = { 0 };
uint8_t sRxDmaBuffer2[MAX_DMA_BUFFER_SIZE] = { 0 };
uint16_t lastCount                         = 0; // Nb of bytes already processed from the active dmaBuffer
} // namespace

static void UART_rx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t * data, UARTDRV_Count_t transferCount);

#endif // SILABS_LOG_OUT_UART || ENABLE_CHIP_SHELL || CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

#if defined(_SILICON_LABS_32B_SERIES_3)
// To remove any ambiguities regarding the Flash aliases, use the below macro to ignore the 8 MSB.
#define FLASH_GENERIC_MASK 0x00FFFFFF
#define GENERIC_ADDRESS(addr) ((addr) &FLASH_GENERIC_MASK)

// Transforms any address into an address using the same alias as FLASH_BASE from the CMSIS.
#define CMSIS_CONVERTED_ADDRESS(addr) (GENERIC_ADDRESS(addr) | FLASH_BASE)
namespace {
sl_se_command_context_t cmd_ctx;
}
#endif // _SILICON_LABS_32B_SERIES_3

namespace chip {
namespace DeviceLayer {
namespace Silabs {

SilabsPlatform SilabsPlatform::sSilabsPlatformAbstractionManager;

SilabsPlatform::SilabsButtonCb SilabsPlatform::mButtonCallback = nullptr;

CHIP_ERROR SilabsPlatform::Init(void)
{
    TEMPORARY_RETURN_IGNORED NvmInit();
#ifdef _SILICON_LABS_32B_SERIES_2
    // Read the cause of last reset.
    mRebootCause = RMU_ResetCauseGet();

    // Clear the register, as the causes cumulate over resets.
    RMU_ResetCauseClear();
#else
    // Read the cause of last reset.
    mRebootCause = sl_hal_emu_get_reset_cause();

    // Clear the register, as the causes cumulate over resets.
    sl_hal_emu_clear_reset_cause();
#endif // _SILICON_LABS_32B_SERIES_2

#if SILABS_LOG_OUT_UART && defined(SL_CATALOG_CLI_PRESENT)
    sl_iostream_set_default(sl_iostream_stdio_handle);
#endif

#ifdef SL_CATALOG_SYSTEMVIEW_TRACE_PRESENT
    SEGGER_SYSVIEW_Conf();
#endif

#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    uartConsoleInit();
#endif // SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) || defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI)
       // && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

#if SILABS_LOG_ENABLED
    silabsInitLog();
#endif // SILABS_LOG_ENABLED
    return CHIP_NO_ERROR;
}

void SilabsPlatform::SoftwareReset()
{
    NVIC_SystemReset();
}

CHIP_ERROR SilabsPlatform::FlashInit()
{
#if defined(SL_TRUSTZONE_NONSECURE)
    return CHIP_ERROR_NOT_IMPLEMENTED;
#elif defined(_SILICON_LABS_32B_SERIES_2)
    MSC_Init();
#elif defined(_SILICON_LABS_32B_SERIES_3)
    sl_status_t status;
    status = sl_se_init();
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
    status = sl_se_init_command_context(&cmd_ctx);
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
#endif
    return CHIP_NO_ERROR;
}

CHIP_ERROR SilabsPlatform::FlashErasePage(uint32_t addr)
{
#if defined(SL_TRUSTZONE_NONSECURE)
    return CHIP_ERROR_NOT_IMPLEMENTED;
#elif defined(_SILICON_LABS_32B_SERIES_2)
    MSC_ErasePage((uint32_t *) addr);
#elif defined(_SILICON_LABS_32B_SERIES_3)
    sl_status_t status;
    uint32_t * data_start = NULL;
    size_t data_size;

    status = sl_se_data_region_get_location(&cmd_ctx, (void **) &data_start, &data_size);
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
    VerifyOrReturnError(GENERIC_ADDRESS(addr) > GENERIC_ADDRESS((uint32_t) data_start), CHIP_ERROR_INVALID_ADDRESS);
    status = sl_se_data_region_erase(&cmd_ctx, (void *) addr, 1); // Erase one page
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
#endif
    return CHIP_NO_ERROR;
}

CHIP_ERROR SilabsPlatform::FlashWritePage(uint32_t addr, const uint8_t * data, size_t size)
{
#if defined(SL_TRUSTZONE_NONSECURE)
    return CHIP_ERROR_NOT_IMPLEMENTED;
#elif defined(_SILICON_LABS_32B_SERIES_2)
    MSC_WriteWord((uint32_t *) addr, data, size);
#elif defined(_SILICON_LABS_32B_SERIES_3)
    sl_status_t status;
    uint32_t * data_start = NULL;
    size_t data_size;

    status = sl_se_data_region_get_location(&cmd_ctx, (void **) &data_start, &data_size);
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
    VerifyOrReturnError(GENERIC_ADDRESS(addr) > GENERIC_ADDRESS((uint32_t) data_start), CHIP_ERROR_INVALID_ADDRESS);
    status = sl_se_data_region_write(&cmd_ctx, (void *) addr, data, size);
    VerifyOrReturnError(status == SL_STATUS_OK, CHIP_ERROR(status));
#endif
    return CHIP_NO_ERROR;
}

#ifdef ENABLE_WSTK_LEDS
void SilabsPlatform::InitLed(void)
{
    SL_LED_INIT_INTANCES();
}

CHIP_ERROR SilabsPlatform::SetLed(bool state, uint8_t led)
{
    if (led >= SL_SIMPLE_LED_COUNT)
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }

    (state) ? SL_LED_TURN_ON(SL_SIMPLE_LED_INSTANCE(led)) : SL_LED_TURN_OFF(SL_SIMPLE_LED_INSTANCE(led));
    return CHIP_NO_ERROR;
}

bool SilabsPlatform::GetLedState(uint8_t led)
{
    if (led >= SL_SIMPLE_LED_COUNT)
    {
        return false;
    }
    return SL_LED_GET_STATE(SL_SIMPLE_LED_INSTANCE(led));
}

CHIP_ERROR SilabsPlatform::ToggleLed(uint8_t led)
{
    if (led >= SL_SIMPLE_LED_COUNT)
    {
        return CHIP_ERROR_INVALID_ARGUMENT;
    }
    SL_LED_TOGGLE(SL_SIMPLE_LED_INSTANCE(led));
    return CHIP_NO_ERROR;
}
#endif // ENABLE_WSTK_LEDS

#if defined(SL_CATALOG_CUSTOM_MAIN_PRESENT)
// Use sl_system for projects upgraded to 2025.6, identified by the presence of SL_CATALOG_CUSTOM_MAIN_PRESENT
void SilabsPlatform::StartScheduler()
{
    sl_system_kernel_start();
}
#endif

#if (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)
bool SilabsPlatform::GetRGBLedState(uint8_t led)
{
    return SL_LED_GET_STATE(SL_SIMPLE_LED_INSTANCE(led));
}
CHIP_ERROR SilabsPlatform::SetLedColor(uint8_t led, uint8_t red, uint8_t green, uint8_t blue)
{
    sl_led_set_rgb_color(SL_SIMPLE_LED_INSTANCE(led), red, green, blue);
    return CHIP_NO_ERROR;
}
CHIP_ERROR SilabsPlatform::GetLedColor(uint8_t led, uint16_t & r, uint16_t & g, uint16_t & b)
{
    sl_led_get_rgb_color(SL_SIMPLE_LED_INSTANCE(led), &r, &g, &b);
    return CHIP_NO_ERROR;
}
#endif // (defined(SL_MATTER_RGB_LED_ENABLED) && SL_MATTER_RGB_LED_ENABLED == 1)

#ifdef SL_CATALOG_SIMPLE_BUTTON_PRESENT
extern "C" void sl_button_on_change(const sl_button_t * handle)
{
    if (Silabs::GetPlatform().mButtonCallback == nullptr)
    {
        return;
    }

    for (uint8_t i = 0; i < SL_SIMPLE_BUTTON_COUNT; i++)
    {
        if (SL_SIMPLE_BUTTON_INSTANCE(i) == handle)
        {
            Silabs::GetPlatform().mButtonCallback(i, sl_button_get_state(handle));
            break;
        }
    }
}

uint8_t SilabsPlatform::GetButtonState(uint8_t button)
{
    const sl_button_t * handle = SL_SIMPLE_BUTTON_INSTANCE(button);
    return nullptr == handle ? 0 : sl_button_get_state(handle);
}

#else
uint8_t SilabsPlatform::GetButtonState(uint8_t button)
{
    return 0;
}
#endif // SL_CATALOG_SIMPLE_BUTTON_PRESENT

// UART hardware-specific implementations
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

extern "C" void USART_IRQHandler(void)
{
#ifdef ENABLE_CHIP_SHELL
    chip::NotifyShellProcess();
#elif !defined(PW_RPC_ENABLED) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    otSysEventSignalPending();
#endif
#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
    // disable RXFL IRQ until data read by uartConsoleRead
    EUSART_INT_DISABLE(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, EUSART_IF_RXFL);
    EUSART_INT_CLEAR(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, EUSART_IF_RXFL);

    if (EUSART_GET_PENDING_INT(SL_UARTDRV_EUSART_VCOM_PERIPHERAL) & EUSART_IF_RXOF)
    {
        EUSART_CLEAR_RX(SL_UARTDRV_EUSART_VCOM_PERIPHERAL);
    }
#endif
}

static void UART_tx_callback(struct UARTDRV_HandleData * handle, Ecode_t transferStatus, uint8_t * data,
                              UARTDRV_Count_t transferCount)
{
    uartSignalTxComplete();
}

static void UART_rx_callback(UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t * data, UARTDRV_Count_t transferCount)
{
    (void) transferStatus;

    uint8_t writeSize = (transferCount - lastCount);
    uartCacheRxBytes(data + lastCount, writeSize);
    lastCount = 0;

    UARTDRV_Receive(vcom_handle, data, transferCount, UART_rx_callback);

#ifdef ENABLE_CHIP_SHELL
    chip::NotifyShellProcess();
#elif !defined(PW_RPC_ENABLED) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    otSysEventSignalPending();
#endif
}

#endif // SILABS_LOG_OUT_UART || ENABLE_CHIP_SHELL || CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI

void SilabsPlatform::UartConsoleInitHw(void)
{
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#ifdef SL_BOARD_NAME
    sl_board_enable_vcom();
#endif

    // Activate 2 dma queues to always have one active
    UARTDRV_Receive(vcom_handle, sRxDmaBuffer, MAX_DMA_BUFFER_SIZE, UART_rx_callback);
    UARTDRV_Receive(vcom_handle, sRxDmaBuffer2, MAX_DMA_BUFFER_SIZE, UART_rx_callback);

    // Enable USART0/EUSART0 interrupt to wake OT task when data arrives
    NVIC_ClearPendingIRQ(USART_IRQ);
    NVIC_EnableIRQ(USART_IRQ);

#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
    // Clear previous RX interrupts
    EUSART_INT_CLEAR(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, (EUSART_IF_RXFL | EUSART_IF_RXOF));
    EUSART_CLEAR_RX(SL_UARTDRV_EUSART_VCOM_PERIPHERAL);

    // Enable RX interrupts
    EUSART_INT_ENABLE(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, EUSART_IF_RXFL);

    // Enable EUSART
    EUSART_ENABLE(SL_UARTDRV_EUSART_VCOM_PERIPHERAL);
#else
    USART_IntEnable(SL_UARTDRV_USART_VCOM_PERIPHERAL, USART_IF_RXDATAV);
#endif // SL_CATALOG_UARTDRV_EUSART_PRESENT
#endif // SILABS_LOG_OUT_UART || ENABLE_CHIP_SHELL || CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
}

void SilabsPlatform::UartSendBytes(uint8_t * data, uint16_t length)
{
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    sl_power_manager_add_em_requirement(SL_POWER_MANAGER_EM1);
#endif // SL_CATALOG_POWER_MANAGER_PRESENT

#if defined(SL_UARTCTRL_MUX) && SL_UARTCTRL_MUX
    sl_wfx_host_pre_uart_transfer();
#endif // SL_UARTCTRL_MUX

#if (defined(EFR32MG24) && defined(WF200_WIFI))
    // Blocking transmit for the MG24 + WF200 since UART TX is multiplexed with WF200 SPI IRQ
    UARTDRV_ForceTransmit(vcom_handle, data, length);
#else
    // Non Blocking Transmit
    UARTDRV_Transmit(vcom_handle, data, length, UART_tx_callback);
    uartWaitForTxComplete();
#endif /* EFR32MG24 && WF200_WIFI */

#if defined(SL_UARTCTRL_MUX) && SL_UARTCTRL_MUX
    sl_wfx_host_post_uart_transfer();
#endif // SL_UARTCTRL_MUX

#if defined(SL_CATALOG_POWER_MANAGER_PRESENT)
    sl_power_manager_remove_em_requirement(SL_POWER_MANAGER_EM1);
#endif // SL_CATALOG_POWER_MANAGER_PRESENT
#endif // SILABS_LOG_OUT_UART || ENABLE_CHIP_SHELL || CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
}

void SilabsPlatform::UartForceTransmit(const char * data, uint16_t length)
{
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
    UARTDRV_ForceTransmit(vcom_handle, reinterpret_cast<uint8_t *>(const_cast<char *>(data)), length);
#endif
}

void SilabsPlatform::UartFlushRxBuffer(void)
{
#if SILABS_LOG_OUT_UART || (defined(ENABLE_CHIP_SHELL) && ENABLE_CHIP_SHELL) ||                                                    \
    defined(CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI) && CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
#ifdef SL_CATALOG_UARTDRV_EUSART_PRESENT
    EUSART_INT_ENABLE(SL_UARTDRV_EUSART_VCOM_PERIPHERAL, EUSART_IF_RXFL);
#endif
    uint8_t * data;
    UARTDRV_Count_t count, remaining;
    CORE_ATOMIC_SECTION(UARTDRV_GetReceiveStatus(vcom_handle, &data, &count, &remaining); if (count > lastCount) {
        uartCacheRxBytes(data + lastCount, count - lastCount);
        lastCount = count;
    })
#endif // SILABS_LOG_OUT_UART || ENABLE_CHIP_SHELL || CHIP_DEVICE_CONFIG_THREAD_ENABLE_CLI
}

#if SL_MATTER_DEBUG_WATCHDOG_ENABLE
void SilabsPlatform::WatchdogInit()
{
    // Initialize WDOG with default configuration
    sl_hal_wdog_init_t wdogInit = SL_HAL_WDOG_INIT_DEFAULT;
    wdogInit.reset_disable      = true;                // For debug, do not trigger a system reset on timeout
    wdogInit.period_select      = SL_WDOG_PERIOD_128k; // Set timeout period. 4s with our default LF clock at 32.768kHz

    //  Initialize WDOG with our configuration
    sl_clock_manager_enable_bus_clock(SL_BUS_CLOCK_WDOG0);
    sl_hal_wdog_init(WDOG0, &wdogInit);

    // Enable Watchdog Timeout interrupt
    sl_hal_wdog_clear_interrupts(WDOG0, WDOG_IF_TOUT);
    sl_hal_wdog_enable_interrupts(WDOG0, WDOG_IF_TOUT);

    WatchdogEnable();
}

void SilabsPlatform::WatchdogFeed()
{
    sl_hal_wdog_feed(WDOG0);
}

void SilabsPlatform::WatchdogEnable()
{
    // Enable NVIC interrupt for WDOG
    sl_interrupt_manager_clear_irq_pending(WDOG0_IRQn);
    sl_interrupt_manager_enable_irq(WDOG0_IRQn);

    sl_hal_wdog_enable(WDOG0);
}

void SilabsPlatform::WatchdogDisable()
{
    sl_hal_wdog_disable(WDOG0);
    sl_interrupt_manager_disable_irq(WDOG0_IRQn);
}
#endif // SL_MATTER_DEBUG_WATCHDOG_ENABLE

} // namespace Silabs
} // namespace DeviceLayer
} // namespace chip
