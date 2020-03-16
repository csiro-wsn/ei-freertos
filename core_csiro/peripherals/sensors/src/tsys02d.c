/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "i2c.h"
#include "log.h"
#include "memory_operations.h"
#include "tsys02d.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static xI2CModule_t *pxModule;

static xI2CConfig_t xI2CConfig = {
	.ulMaximumBusFrequency = 400000,
	.ucAddress			   = TSYS02D_ADDRESS << 1
};

/* Functions ------------------------------------------------*/

eModuleError_t eTsysInit( xTsysInit_t *pxInit )
{
	configASSERT( pxInit->pxModule != NULL );

	pxModule = pxInit->pxModule;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eTsysReadRaw( uint16_t *pusData, TickType_t xTimeout )
{
	eModuleError_t eError;
	uint8_t		   pucData[3];
	uint8_t		   pucCommand = TSYS02D_COMMAND_READ_TEMPERATURE;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;

	eError = eI2CBusStart( pxModule, &xI2CConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, &pucCommand, 1, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	vTaskDelay( pdMS_TO_TICKS( TSYS02D_CONVERSION_TIME_MS ) );

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CReceive( pxModule, pucData, 3, xCumulativeTimeout );

	eI2CBusEnd( pxModule );

	*pusData = BE_U16_EXTRACT( pucData );

	return eError;
}

/*-----------------------------------------------------------*/
