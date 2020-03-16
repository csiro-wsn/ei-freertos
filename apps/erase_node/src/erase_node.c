/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"
#include "device_nvm.h"
#include "flash_interface.h"
#include "leds.h"
#include "log.h"
#include "rtc.h"
#include "watchdog.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static void prvNodeResetTask( void *pvParameters );

/* Private Variables ----------------------------------------*/

STATIC_TASK_STRUCTURES( pxResetHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 2 );

static bool bEraseRunning;

/*-----------------------------------------------------------*/

void vApplicationSetLogLevels( void )
{
	eLogSetLogLevel( LOG_APPLICATION, LOG_INFO );
	eLogSetLogLevel( LOG_BLUETOOTH_GAP, LOG_ERROR );
	eLogSetLogLevel( LOG_FLASH_DRIVER, LOG_ERROR );
}

/*-----------------------------------------------------------*/

void vApplicationStartupCallback( void )
{
	WatchdogReboot_t *pxRebootData;

	vLedsOn( LEDS_ALL );

	/* Get Reboot reasons */
	pxRebootData = xWatchdogRebootReason();
	if ( pxRebootData != NULL ) {
		vWatchdogPrintRebootReason( LOG_APPLICATION, LOG_INFO, pxRebootData );
	}

	bEraseRunning = true;
	STATIC_TASK_CREATE( pxResetHandle, prvNodeResetTask, "Reset", NULL );

	vLedsOff( LEDS_ALL );
}

/*-----------------------------------------------------------*/

void vApplicationTickCallback( uint32_t ulUptime )
{
	UNUSED( ulUptime );

	if ( bEraseRunning ) {
		vLedsToggle( LEDS_BLUE );
		eLog( LOG_APPLICATION, LOG_ERROR, "App ticking...\r\n" );
	}
	else {
		vLedsOn( LEDS_BLUE );
	}
}

/*-----------------------------------------------------------*/

static void prvNodeResetTask( void *pvParameters )
{
	UNUSED( pvParameters );
	eLog( LOG_APPLICATION, LOG_ERROR, "Reset Starting...\r\n" );

	/* Erase NVM */
	eNvmEraseData();
	/* Erase Flash */
	eFlashEraseAll( pxOnboardFlash, portMAX_DELAY );

	bEraseRunning = false;
	eLog( LOG_APPLICATION, LOG_ERROR, "Reset Done\r\n" );

	for ( ;; ) {
		vTaskSuspend( NULL );
	}
}

/*-----------------------------------------------------------*/
