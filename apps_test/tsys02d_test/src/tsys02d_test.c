/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* Board configuration */
#include "board.h"

/* Architecture Specific peripherals */
#include "gpio.h"
#include "leds.h"
#include "log.h"
#include "rtc.h"
#include "watchdog.h"

/* External peripherals */
#include "i2c.h"
#include "tsys02d.h"

/* Libraries */
#include "tdf.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vTest( void *pvParams );

/* Private Variables ----------------------------------------*/

STATIC_TASK_STRUCTURES( Test, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1 );

/*-----------------------------------------------------------*/

void vApplicationSetLogLevels( void )
{
	eLogSetLogLevel( LOG_APPLICATION, LOG_INFO );
}

/*-----------------------------------------------------------*/

void vApplicationStartupCallback( void )
{
	WatchdogReboot_t *pxRebootData;

	vLedsOn( LEDS_ALL );

	/* Get Reboot reasons and clear */
	pxRebootData = xWatchdogRebootReason();
	if ( pxRebootData != NULL ) {
		vWatchdogPrintRebootReason( LOG_APPLICATION, LOG_INFO, pxRebootData );
	}

	STATIC_TASK_CREATE( Test, vTest, "TSYS", NULL );

	vLedsOff( LEDS_ALL );
}

/*-----------------------------------------------------------*/

void vTest( void *pvParams )
{
	UNUSED( pvParams );

	eModuleError_t eError;
	int32_t		   lData;

	while ( 1 ) {
		vTaskDelay( pdMS_TO_TICKS( 1000 ) );

		eError = eTsysReadMilliDegrees( &lData, pdMS_TO_TICKS( 100 ) );
		if ( eError == ERROR_NONE ) {
			eLog( LOG_APPLICATION, LOG_INFO, "Temperature in Degrees: %d.%03d\r\n", lData / 1000, lData % 1000 );
		}
		else {
			eLog( LOG_APPLICATION, LOG_ERROR, "Error: %d\r\n", eError );
		}
	}
}

/*-----------------------------------------------------------*/
