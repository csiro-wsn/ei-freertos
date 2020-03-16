/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: freertos_helpers.h
 * Creation_Date: 04/08/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Helper macros for FreeRTOS static allocation
 * 
 */
#ifndef __CSIRO_CORE_FREERTOS_HELPERS
#define __CSIRO_CORE_FREERTOS_HELPERS

/* Includes -------------------------------------------------*/

/* Type Definitions -----------------------------------------*/

#define STATIC_TASK_STRUCTURES( pxHandle, ulStackSize, xPriority ) \
	static xTaskHandle		 pxHandle			 = NULL;           \
	static const uint32_t	pxHandle##StackSize = ulStackSize;    \
	static const UBaseType_t pxHandle##Priority  = xPriority;      \
	static StaticTask_t		 pxHandle##Struct;                     \
	static StackType_t		 pxHandle##Stack[ulStackSize]

#define STATIC_TASK_CREATE( pxHandle, fnFunction, pcDescription, pvParameters ) \
	pxHandle = xTaskCreateStatic( fnFunction, pcDescription, pxHandle##StackSize, pvParameters, pxHandle##Priority, pxHandle##Stack, &pxHandle##Struct )

#define STATIC_SEMAPHORE_STRUCTURES( pxHandle ) \
	static SemaphoreHandle_t pxHandle;          \
	static StaticSemaphore_t pxHandle##Struct;

#define STATIC_SEMAPHORE_CREATE_BINARY( pxHandle ) \
	pxHandle = xSemaphoreCreateBinaryStatic( &pxHandle##Struct )

#define STATIC_SEMAPHORE_CREATE_MUTEX( pxHandle ) \
	pxHandle = xSemaphoreCreateMutexStatic( &pxHandle##Struct )

#define STATIC_SEMAPHORE_CREATE_COUNTING( pxHandle, ulMaxCount, ulStartingCount ) \
	pxHandle = xSemaphoreCreateCountingStatic( ulMaxCount, ulStartingCount, &pxHandle##Struct )

#define STATIC_QUEUE_STRUCTURES( pxHandle, ulQueueElementSize, ulQueueNumElements ) \
	static QueueHandle_t  pxHandle;                                                 \
	static StaticQueue_t  pxHandle##QueueStructures;                                \
	static const uint32_t pxHandle##QueueElementSize = ulQueueElementSize;          \
	static const uint32_t pxHandle##QueueNumElements = ulQueueNumElements;          \
	static uint8_t		  pxHandle##StorageArea[ulQueueElementSize * ulQueueNumElements]

#define STATIC_QUEUE_CREATE( pxHandle ) \
	pxHandle = xQueueCreateStatic( pxHandle##QueueNumElements, pxHandle##QueueElementSize, pxHandle##StorageArea, &pxHandle##QueueStructures )

#define STATIC_TIMER_STRUCTURES( pxHandle ) \
	TimerHandle_t pxHandle;                 \
	StaticTimer_t pxHandle##Struct

#define STATIC_TIMER_CREATE( pxHandle, fnCallback, pcDescription, pvContext, xPeriod, bAutoReload ) \
	pxHandle = xTimerCreateStatic( pcDescription, xPeriod, bAutoReload, pvContext, fnCallback, &pxHandle##Struct )

#define STATIC_EVENT_GROUP_STRUCTURES( pxHandle ) \
	static EventGroupHandle_t pxHandle;           \
	static StaticEventGroup_t pxHandle##Structures

#define STATIC_EVENT_GROUP_CREATE( pxHandle ) \
	pxHandle = xEventGroupCreateStatic( &pxHandle##Structures )

/* Type Definitions -----------------------------------------*/

#endif /* __CSIRO_CORE_FREERTOS_HELPERS */
