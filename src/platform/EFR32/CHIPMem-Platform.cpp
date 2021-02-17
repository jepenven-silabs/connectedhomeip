/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Nest Labs, Inc.
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


/*
 *
 *    Copyright (c) 2020-2021 Project CHIP Authors
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

/**
 *    @file
 *      This file implements heap memory allocation APIs for CHIP. These functions are platform
 *      specific and might be C Standard Library heap functions re-direction in most of cases.
 *
 */

//#include <core/CHIPConfig.h>
#include <support/CHIPMem.h>

#include "FreeRTOS.h"
#include "task.h"

#include "sl_malloc.h"

#include <atomic>
#include <cstdio>
#include <stdlib.h>
#include <cstring>

#if CHIP_CONFIG_MEMORY_MGMT_PLATFORM

extern "C" void memMonitoringTrackAlloc(void * ptr, size_t size);
extern "C" void memMonitoringTrackFree(void * ptr, size_t size);

#ifndef trackAlloc
#define trackAlloc( pvAddress, uiSize ) memMonitoringTrackAlloc(pvAddress, uiSize)
#endif
#ifndef trackFree
#define trackFree( pvAddress, uiSize ) memMonitoringTrackFree( pvAddress, uiSize )
#endif

namespace chip {
namespace Platform {


#define VERIFY_INITIALIZED() VerifyInitialized(__func__)

static std::atomic_int memoryInitialized{ 0 };

static void VerifyInitialized(const char * func)
{
    if (!memoryInitialized)
    {
        fprintf(stderr, "ABORT: chip::Platform::%s() called before chip::Platform::MemoryInit()\n", func);
        abort();
    }
}

CHIP_ERROR MemoryAllocatorInit(void * buf, size_t bufSize)
{
#ifndef NDEBUG
    if (memoryInitialized++ > 0)
    {
        fprintf(stderr, "ABORT: chip::Platform::MemoryInit() called twice.\n");
        abort();
    }
#endif
    return CHIP_NO_ERROR;
}

void MemoryAllocatorShutdown()
{
#ifndef NDEBUG
    if (--memoryInitialized < 0)
    {
        fprintf(stderr, "ABORT: chip::Platform::MemoryShutdown() called twice.\n");
        abort();
    }
#endif
#if CHIP_CONFIG_MEMORY_DEBUG_DMALLOC
    dmalloc_shutdown();
#endif // CHIP_CONFIG_MEMORY_DEBUG_DMALLOC
}

void * MemoryAlloc(size_t size)
{
    void * ptr;
    ptr = sl_malloc(size);
    trackAlloc(ptr, size);
    return ptr;
}

void * MemoryAlloc(size_t size, bool isLongTermAlloc)
{
    void * ptr;
    ptr = sl_malloc(size);
    trackAlloc(ptr, size);
    return ptr;
}

void * MemoryCalloc(size_t num, size_t size)
{
    void * ptr;
    ptr = sl_calloc(num, size);
    trackAlloc(ptr, size * num);
    return ptr;
}

void * MemoryRealloc(void * p, size_t size)
{
    VERIFY_INITIALIZED();
    return sl_realloc(p, size);
}

void MemoryFree(void * p)
{
    trackFree(p, 0);
    sl_free(p);

}

bool MemoryInternalCheckPointer(const void * p, size_t min_size)
{
    return (p != nullptr);
}

} // namespace Platform
} // namespace chip

extern "C" void *pvPortMalloc( size_t xWantedSize )
{
void *pvReturn;

	vTaskSuspendAll();
	{
		pvReturn = sl_malloc( xWantedSize );
		trackAlloc( pvReturn, xWantedSize );
	}
	( void ) xTaskResumeAll();

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
	#endif

	return pvReturn;
}

extern "C"  void vPortFree( void *pv )
{
	if( pv )
	{
		vTaskSuspendAll();
		{
            trackFree(pv, 0);
			sl_free( pv );
		}
		( void ) xTaskResumeAll();
	}
}

extern "C" __WEAK void memMonitoringTrackAlloc(void * ptr, size_t size)
{

}

extern "C" __WEAK void memMonitoringTrackFree(void * ptr, size_t size)
{

}

#endif // CHIP_CONFIG_MEMORY_MGMT_PLATFORM
