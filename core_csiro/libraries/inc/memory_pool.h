/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: memory_pool.h
 * Creation_Date: 14/08/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Thread safe implementation of a shared pool of buffers
 * Buffers can be claimed and returned in any order
 * Buffers are always aligned to 8 byte boundaries, allowing aliasing to all data types
 * Use ptr = ALIGNED_POINTER(ptr, 8) on the returned pointer to tell the compiler this
 * 
 */
#ifndef __CSIRO_CORE_UTIL_MEMORY_POOL
#define __CSIRO_CORE_UTIL_MEMORY_POOL
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "csiro_math.h"

#include "cpu.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

#define MEMORY_POOL_GET( NAME ) xMemoryPool##NAME

#define MEMORY_POOL_CREATE( NAME, NUM_BUFFERS, BUFFER_SIZE )                           \
	CASSERT( NUM_BUFFERS > 0, memory_pool );                                           \
	CASSERT( NUM_BUFFERS <= 32, memory_pool );                                         \
	ATTR_ALIGNED( 8 )                                                                  \
	static int8_t ucMemoryPoolStorage##NAME[NUM_BUFFERS * ROUND_UP( BUFFER_SIZE, 8 )]; \
                                                                                       \
	static xMemoryPool_t xMemoryPool##NAME = {                                         \
		.ulBufferSize	  = ROUND_UP( BUFFER_SIZE, 8 ),                               \
		.ulFreeBuffers	 = 0,                                                        \
		.xSemaphoreHandle  = NULL,                                                     \
		.xSemaphoreStorage = { { 0 } },                                                \
		.pcMemory		   = ucMemoryPoolStorage##NAME,                                \
		.ucNumBuffers	  = NUM_BUFFERS                                               \
	}

/* Type Definitions -----------------------------------------*/

typedef struct
{
	uint32_t		  ulBufferSize;
	uint32_t		  ulFreeBuffers;
	SemaphoreHandle_t xSemaphoreHandle;
	StaticSemaphore_t xSemaphoreStorage;
	int8_t *		  pcMemory;
	uint8_t			  ucNumBuffers;
} xMemoryPool_t;

/* Function Declarations ------------------------------------*/

static inline void vMemoryPoolInit( xMemoryPool_t *pxPool )
{
	/* Initialise counting sempaphore */
	pxPool->xSemaphoreHandle = xSemaphoreCreateCountingStatic( pxPool->ucNumBuffers,
															   pxPool->ucNumBuffers,
															   &pxPool->xSemaphoreStorage );
	/* Set lowest ucNumBuffers bits of ulFreeBuffers */
	pxPool->ulFreeBuffers = 0xFFFFFFFF >> ( 32 - pxPool->ucNumBuffers );
}

static inline int8_t *pcMemoryPoolClaim( xMemoryPool_t *pxPool, TickType_t xTimeout )
{
	CRITICAL_SECTION_DECLARE;
	int8_t *pxBuffer = NULL;
	int32_t lBufferBit;
	int32_t lBufferIndex;
	/* Wait for a buffer to be available */
	if ( xSemaphoreTake( pxPool->xSemaphoreHandle, xTimeout ) != pdPASS ) {
		return NULL;
	}
	CRITICAL_SECTION_START();
	lBufferBit = FIND_FIRST_SET( pxPool->ulFreeBuffers );
	/* Semaphore take succeeded, so lBufferBit should never be 0 */
	configASSERT( lBufferBit != 0 );
	/* FIND_FIRST_SET is 1 indexed */
	lBufferIndex = lBufferBit - 1;
	/* Get the buffer pointer */
	pxBuffer = &pxPool->pcMemory[lBufferIndex * pxPool->ulBufferSize];
	/* Clear the free buffer bit*/
	pxPool->ulFreeBuffers &= ~( 0x01 << lBufferIndex );
	CRITICAL_SECTION_STOP();
	return (int8_t *) ALIGNED_POINTER( pxBuffer, 8 );
}

static inline uint8_t ucMemoryPoolUsedBuffers( xMemoryPool_t *pxPool )
{
	return pxPool->ucNumBuffers - COUNT_ONES( pxPool->ulFreeBuffers );
}

static inline void vMemoryPoolRelease( xMemoryPool_t *pxPool, int8_t *pcBuffer )
{
	CRITICAL_SECTION_DECLARE;
	uint32_t ulBufferIndex = ( uint32_t )( pcBuffer - pxPool->pcMemory ) / pxPool->ulBufferSize;
	CRITICAL_SECTION_START();
	/* Note the free buffer index */
	pxPool->ulFreeBuffers |= ( 0x01 << ulBufferIndex );
	CRITICAL_SECTION_STOP();
	/* Increment number of free buffers available */
	xSemaphoreGive( pxPool->xSemaphoreHandle );
}

static inline void vMemoryPoolReleaseFromISR( xMemoryPool_t *pxPool, int8_t *pcBuffer, BaseType_t *pxHigherPriorityTaskWoken )
{

	CRITICAL_SECTION_DECLARE;
	uint32_t ulBufferIndex = ( uint32_t )( pcBuffer - pxPool->pcMemory ) / pxPool->ulBufferSize;
	/* Critical section in an interrupt is non-ideal, but its only a single operation */
	CRITICAL_SECTION_START();
	/* Note the free buffer index */
	pxPool->ulFreeBuffers |= ( 0x01 << ulBufferIndex );
	CRITICAL_SECTION_STOP();
	/* Increment number of free buffers available */
	xSemaphoreGiveFromISR( pxPool->xSemaphoreHandle, pxHigherPriorityTaskWoken );
}

#endif /* __CSIRO_CORE_UTIL_MEMORY_POOL */
