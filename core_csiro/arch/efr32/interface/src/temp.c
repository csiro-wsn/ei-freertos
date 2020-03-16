/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "temp.h"

#include "tempdrv.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

STATIC_SEMAPHORE_STRUCTURES( xTempSemaphore );

/* Functions ------------------------------------------------*/

void vTempInit( void )
{
	STATIC_SEMAPHORE_CREATE_BINARY( xTempSemaphore );
	xSemaphoreGive( xTempSemaphore );

	TEMPDRV_Init();
	TEMPDRV_Enable( true );
}

/*-----------------------------------------------------------*/

eModuleError_t eTempMeasureMilliDegrees( int32_t *plTemperature )
{
	int8_t cTemp;

	if ( xSemaphoreTake( xTempSemaphore, 0 ) == pdFALSE ) {
		return ERROR_UNAVAILABLE_RESOURCE;
	}

	/* Gets temperature in degrees celcius */
	cTemp = TEMPDRV_GetTemp();

	*plTemperature = cTemp * 1000;

	xSemaphoreGive( xTempSemaphore );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
