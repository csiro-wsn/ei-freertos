/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "temp.h"

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
}

/*-----------------------------------------------------------*/

eModuleError_t eTempMeasureMilliDegrees( int32_t *plTemperature )
{
	configASSERT( plTemperature != NULL );

	int32_t lSdTemperature;

	if ( xSemaphoreTake( xTempSemaphore, 0 ) == pdFALSE ) {
		return ERROR_UNAVAILABLE_RESOURCE;
	}

	if ( sd_temp_get( &lSdTemperature ) != NRF_SUCCESS ) {
		xSemaphoreGive( xTempSemaphore );
		return ERROR_GENERIC;
	}

	*plTemperature = lSdTemperature * 250;

	xSemaphoreGive( xTempSemaphore );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
