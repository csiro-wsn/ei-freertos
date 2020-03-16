/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "compiler_intrinsics.h"

#include "board.h"
#include "log.h"
#include "rtc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define STARTUP_TASK_PRIORITY       tskIDLE_PRIORITY + 7
#define HEARTBEAT_TASK_PRIORITY 	tskIDLE_PRIORITY + 1
#define HEARTBEAT_STACK_SIZE 	    configMINIMAL_STACK_SIZE

// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvHeartbeatTask( void *pvParameters );

/* Private Variables ----------------------------------------*/

static TaskHandle_t xHeartbeatHandle;

static uint32_t ulUptime;

/**
 * Force at least one variable to exist in the stack
 * section so that its size is included in size calculations
 */
uint8_t pucStack[0] ATTR_SECTION( ".stack" );

/*-----------------------------------------------------------*/

/* Return value is explicitly void as this function is marked noreturn */
int main( void )
{
	/* Configure minimal hardware required to run (Clocks etc) */
	vBoardSetupCore();

	/* Create the task which will initialise the rest of the board */
	BaseType_t xRet = xTaskCreate( prvHeartbeatTask,
								   "Heartbeat",
								   HEARTBEAT_STACK_SIZE,
								   NULL,
								   STARTUP_TASK_PRIORITY,
								   &xHeartbeatHandle );
	configASSERT( xRet == pdPASS );

	/* Start the scheduler. */
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the Idle and/or
	timer tasks to be created.  See the memory management section on the
	FreeRTOS web site for more details on the FreeRTOS heap
	http://www.freertos.org/a00111.html. */
	for ( ;; ) continue;
	/* Should not get here. */
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvHeartbeatTask( void *pvParameters )
{
	UNUSED( pvParameters );

	/* Initialise the board */
	vBoardInit();

	/* Call application startup callback */
	vApplicationStartupCallback();

	/* Now that initialisation is done, reduce task priority */
	vTaskPrioritySet( xHeartbeatHandle, HEARTBEAT_TASK_PRIORITY );

	ulUptime = 0;

	/* Tickle the watchdog before starting the main loop */
	vBoardWatchdogPeriodic();

	/* Output a heap summary after pvMalloc is disabled */
	size_t xPortGetTotalHeapSize( void );
	void   vPortDisableMalloc( void );

	vPortDisableMalloc();
	const size_t xFreeHeap  = xPortGetFreeHeapSize();
	const size_t xTotalHeap = xPortGetTotalHeapSize();
	const size_t xUsedHeap  = xTotalHeap - xFreeHeap;
	eLog( LOG_APPLICATION, LOG_ERROR, "\r\nHeap Usage %d%%: %d/%d bytes\r\n\r\n", xUsedHeap * 100 / xTotalHeap, xUsedHeap, xTotalHeap );

	/* Run the main heartbeat loop */
	for ( ;; ) {
		vRtcHeartbeatWait();
		vBoardWatchdogPeriodic();
		vApplicationTickCallback( ++ulUptime );
	}
}

/*-----------------------------------------------------------*/

uint32_t ulApplicationUptime( void )
{
	return ulUptime;
}

/*-----------------------------------------------------------*/
