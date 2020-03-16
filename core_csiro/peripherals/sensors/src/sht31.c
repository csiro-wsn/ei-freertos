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
#include "sht31.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static xI2CModule_t *pxModule;

static xI2CConfig_t xI2CConfig = {
	.ulMaximumBusFrequency = 1000000,
	.ucAddress			   = SHT31_ADDRESS << 1
};

/* Functions ------------------------------------------------*/

eModuleError_t eSht31Init( xSht31Init_t *pxInit )
{
	configASSERT( pxInit->pxModule != NULL );

	pxModule = pxInit->pxModule;

	vTaskDelay( pdMS_TO_TICKS( 2 ) );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSht31Read( xSht31Data_t *pxSht31Data, eSht31Accuracy_t eAccuracy, TickType_t xTimeout )
{
	uint8_t		   pucCommand[2];
	uint8_t		   pucTemperature[6] = { 0, 0 };
	TickType_t	 xConversionTime;
	TickType_t	 xEndTime = xTaskGetTickCount() + xTimeout;
	uint16_t	   usTemp, usHumidity;
	eModuleError_t eError;

	configASSERT( pxModule != NULL );

	pucCommand[0] = 0x24;
	switch ( eAccuracy ) {
		case SHT31_ACCURACY_HIGH:
			pucCommand[1]   = 0x00;
			xConversionTime = pdMS_TO_TICKS( 16 );
			break;

		case SHT31_ACCURACY_MEDIUM:
			pucCommand[1]   = 0x0B;
			xConversionTime = pdMS_TO_TICKS( 7 );
			break;

		case SHT31_ACCURACY_LOW:
			pucCommand[1]   = 0x16;
			xConversionTime = pdMS_TO_TICKS( 5 );
			break;

		default:
			return ERROR_NO_MATCH;
	}

	// Start the I2C bus
	eError = eI2CBusStart( pxModule, &xI2CConfig, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	// Build a buffer of things to send and send it.
	eError = eI2CTransmit( pxModule, pucCommand, 2, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	// Finish using the I2C line so other processes can use it during the delay.
	eI2CBusEnd( pxModule );

	// Wait for the temperature conversion.
	vTaskDelay( xConversionTime );

	// Start up the bus again.
	eError = eI2CBusStart( pxModule, &xI2CConfig, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	// Receive data.
	eError = eI2CReceive( pxModule, pucTemperature, 6, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	// Finish using the I2C line.
	eI2CBusEnd( pxModule );

	// Do put the data where it belongs, and convert it into useful units.
	usTemp					= BE_U16_EXTRACT( pucTemperature );
	usHumidity				= BE_U16_EXTRACT( pucTemperature + 3 );
	pxSht31Data->usTemp		= usTemp;
	pxSht31Data->usHumidity = usHumidity;

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Converts the raw value for temperature into centidegrees Celcius.
 * (Hundredths of a degree).
 */
int32_t iSht31TemperatureConversion( uint16_t usTemp )
{
	return ( ( ( usTemp * 17500 ) >> 16 ) - 4500 );
}

/*-----------------------------------------------------------*/

/**
 * Converts the raw value for humidity into centipercentage.
 * (Hundredths of a percentage).
 */
int32_t iSht31HumidityConversion( uint16_t usHumidity )
{
	return ( ( usHumidity * 10000 ) >> 16 );
}

/*-----------------------------------------------------------*/
