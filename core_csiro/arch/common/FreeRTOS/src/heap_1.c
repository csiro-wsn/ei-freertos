/*
 * FreeRTOS Kernel V10.1.1
 * Copyright (C) 2018 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * The simplest possible implementation of pvPortMalloc().  Note that this
 * implementation does NOT allow allocated memory to be freed again.
 *
 * See heap_2.c, heap_3.c and heap_4.c for alternative implementations, and the
 * memory management pages of http://www.FreeRTOS.org for more information.
 */
#include <stdlib.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if ( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#ifdef HEAP_ARRAY_OVERRIDE

uint8_t pucHeap[HEAP_ARRAY_OVERRIDE] ATTR_ALIGNED( 32 );

uint8_t *const pucHeapStart = pucHeap;
uint8_t *const pucHeapEnd   = pucHeap + HEAP_ARRAY_OVERRIDE;
const uint32_t ulHeapSize   = HEAP_ARRAY_OVERRIDE;

#else

/**
 * Heap uses the values defined by the linker to define size 
 * We require __HeapBase to be at least 16 byte aligned
 */
extern uint8_t __HeapBase;
extern uint8_t __HeapLimit;
extern uint8_t __HeapSize;

uint8_t *const pucHeapStart = &__HeapBase;
uint8_t *const pucHeapEnd   = &__HeapLimit;
const uint32_t ulHeapSize   = ( uint32_t )( &__HeapSize );

#endif /* HEAP_ARRAY_OVERRIDE */

/* Index into the ucHeap array. */
static size_t  xNextFreeByte   = (size_t) 0;
static uint8_t ucMallocEnabled = 0xFF;

/*-----------------------------------------------------------*/

void *pvPortMalloc( size_t xWantedSize )
{
	void *pvReturn = NULL;
	configASSERT( ucMallocEnabled != 0x00 );

/* Ensure that blocks are always aligned to the required number of bytes. */
#if ( portBYTE_ALIGNMENT != 1 )
	{
		if ( xWantedSize & portBYTE_ALIGNMENT_MASK ) {
			/* Byte alignment required. */
			xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
		}
	}
#endif

	vTaskSuspendAll();
	{
		/* Check there is enough room left for the allocation. */
		if ( ( ( xNextFreeByte + xWantedSize ) < ulHeapSize ) &&
			 ( ( xNextFreeByte + xWantedSize ) > xNextFreeByte ) ) /* Check for overflow. */
		{
			/* Return the next free byte then increment the index past this
			block. */
			pvReturn = pucHeapStart + xNextFreeByte;
			xNextFreeByte += xWantedSize;
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
	(void) xTaskResumeAll();

#if ( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if ( pvReturn == NULL ) {
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
	}
#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
	/* Memory cannot be freed using this scheme.  See heap_2.c, heap_3.c and
	heap_4.c for alternative implementations, and the memory management pages of
	http://www.FreeRTOS.org for more information. */
	(void) pv;

	/* Force an assert as it is invalid to call this function. */
	configASSERT( 0 );
}
/*-----------------------------------------------------------*/

void vPortInitialiseBlocks( void )
{
	/* Only required when static memory is not cleared. */
	xNextFreeByte = (size_t) 0;
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return ( ulHeapSize - xNextFreeByte );
}

/*-----------------------------------------------------------*/

size_t xPortGetTotalHeapSize( void )
{
	return ulHeapSize;
}

/*-----------------------------------------------------------*/

void vPortDisableMalloc( void )
{
	ucMallocEnabled = 0x00;
}

/*-----------------------------------------------------------*/
