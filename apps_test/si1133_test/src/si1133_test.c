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
#include "si1133.h"

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

	STATIC_TASK_CREATE( Test, vTest, "SI1133", NULL );

	vLedsOff( LEDS_ALL );
}

/*-----------------------------------------------------------*/

void vTest( void *pvParams )
{
	UNUSED( pvParams );

	eModuleError_t eError;
	xSi1133Data_t  xData;

	/* Turn on power to all the environmental sensors */
	eError = eBoardEnablePeripheral( PERIPHERAL_ENVIRONMENTAL_SENSOR, NULL, portMAX_DELAY );

	eError = eSi1133Config();
	if ( eError == ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_INFO, "Si1133 configured correctly\r\n" );
	}
	else {
		eLog( LOG_APPLICATION, LOG_INFO, "Si1133 configured with error code: %d\r\n", eError );
	}

	while ( 1 ) {
		vTaskDelay( pdMS_TO_TICKS( 1000 ) );

		eError = eSi1133Read( &xData, portMAX_DELAY );
		if ( eError == ERROR_NONE ) {
			eLog( LOG_APPLICATION, LOG_INFO, "Raw data from light sensor:\r\nCH0: %d\r\nCH1: %d\r\nCH2: %d\r\nCH3: %d\r\n",
				  xData.lUltraVioletCh0,
				  xData.lAmbientCh1,
				  xData.lAmbientCh2,
				  xData.lAmbientCh3 );
		}
		else {
			eLog( LOG_APPLICATION, LOG_ERROR, "Error: %d\r\n", eError );
		}
	}
}

/*-----------------------------------------------------------*/
