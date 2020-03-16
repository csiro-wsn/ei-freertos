/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "i2c.h"

#include "amg88xx.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

/*
 * Register Map for the AMG88xx.
 * Note: I have left out the mappings for all pixels from 2 to 64.
 * */
typedef enum eAmg88xxRegisterMap_t {
	POWER_CONTROL_REGISTER			= 0x00,
	RESET_REGISTER					= 0x01,
	FRAME_RATE_REGISTER				= 0x02,
	INTERRUPT_CONTROL_REGISTER		= 0x03,
	STATUS_REGISTER					= 0x04,
	STATUS_CLEAR_REGISTER			= 0x05,
	AVERAGE_REGISTER				= 0x07,
	INTERRUPT_LEVEL_UPPER_LIMIT_L	= 0x08,
	INTERRUPT_LEVEL_UPPER_LIMIT_H	= 0x09,
	INTERRUPT_LEVEL_LOWER_LIMIT_L	= 0x0A,
	INTERRUPT_LEVEL_LOWER_LIMIT_H	= 0x0B,
	INTERRUPT_LEVEL_HYSTERESIS_L	= 0x0C,
	INTERRUPT_LEVEL_HYSTERESIS_H	= 0x0D,
	THERMISTOR_REGISTER_L			= 0x0E,
	THERMISTOR_REGISTER_H			= 0x0F,
	INTERRUPT_TABLE_REGISTER_0		= 0x10,
	INTERRUPT_TABLE_REGISTER_1		= 0x11,
	INTERRUPT_TABLE_REGISTER_2		= 0x12,
	INTERRUPT_TABLE_REGISTER_3		= 0x13,
	INTERRUPT_TABLE_REGISTER_4		= 0x14,
	INTERRUPT_TABLE_REGISTER_5		= 0x15,
	INTERRUPT_TABLE_REGISTER_6		= 0x16,
	INTERRUPT_TABLE_REGISTER_7		= 0x17,
	// Here is an example of the temperature data from a single pixel.
	TEMPERATURE_REGISTER_PIXEL_1_L	= 0x80,
	TEMPERATURE_REGISTER_PIXEL_1_H	= 0x81,
	// These temperature registers continue in this pattern from pixel 1 to 64.
	// From memory address 0x80 to 0xFF.
	TEMPERATURE_REGISTER_BASE		= 0x80,
	TEMPERATURE_REGISTER_LAST		= 0xFF
} eAmg88xxRegisterMap_t;

/**
 * List of different power modes the chip has
 **/
typedef enum eAmg88xxPowerMode_t {
	NORMAL_MODE		  = 0x00, // Uses 4.5mA
	SLEEP_MODE		  = 0x10, // Uses 0.2mA
	STAND_BY_MODE_60S = 0x20,
	STAND_BY_MODE_10S = 0x21
} eAmg88xxPowerMode_t;

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static xI2CModule_t *pxModule;

static xI2CConfig_t xAmg88xxConfig = {
	.ulMaximumBusFrequency = 400000,
	.ucAddress			   = AMG88XX_ADDRESS_2 << 1
};

/* Functions ------------------------------------------------*/

eModuleError_t eAmg88xxInit( xAmg88xxInit_t *pxInit, TickType_t xTimeout )
{
	TickType_t	 xStartTime		= xTaskGetTickCount();
	TickType_t	 xChangingTimeout = xTimeout;
	eModuleError_t eError;
	uint8_t		   pucSleepMessage[2] = { POWER_CONTROL_REGISTER, SLEEP_MODE };

	configASSERT( pxInit->pxModule != NULL );

	/* Register the I2C module */
	pxModule = pxInit->pxModule;

	/* Start up I2C bus */
	eError = eI2CBusStart( pxModule, &xAmg88xxConfig, xChangingTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Update the timeout */
	xChangingTimeout -= xTaskGetTickCount() - xStartTime;

	/* Put the GridEye into sleep mode immediately */
	eError = eI2CTransmit( pxModule, pucSleepMessage, 2, xChangingTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Relinquish control of I2C bus. */
	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eAmg88xxTurnOn( eAmg88xxFrameRate_t eFrameRate, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   pucMessageBuffer[2] = { POWER_CONTROL_REGISTER, NORMAL_MODE };

	/* Start the bus and set the power mode */
	eError = eI2CBusStart( pxModule, &xAmg88xxConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, pucMessageBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Set the refresh rate */
	pucMessageBuffer[0] = FRAME_RATE_REGISTER;
	pucMessageBuffer[1] = eFrameRate;
	eError				= eI2CTransmit( pxModule, pucMessageBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	/* Reliquish control of the I2C bus */
	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eAmg88xxRead( xAmg88xxData_t *pxData, TickType_t xTimeout )
{
	uint8_t		   pucAddressToRead[1] = { TEMPERATURE_REGISTER_BASE };
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;

	/* Read data */
	eError = eI2CBusStart( pxModule, &xAmg88xxConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, pucAddressToRead, 1, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CReceive( pxModule, (uint8_t *) pxData->psData, 128, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eAmg88xxTurnOff( TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   pucMessageBuffer[2] = { POWER_CONTROL_REGISTER, SLEEP_MODE };

	/* Start the bus */
	eError = eI2CBusStart( pxModule, &xAmg88xxConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Set the power mode */
	eError = eI2CTransmit( pxModule, pucMessageBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	/* Reliquish control of the I2C bus */
	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eAmg88xxReadSingle( xAmg88xxData_t *pxData, TickType_t xTimeout )
{
	configASSERT( xTimeout > pdMS_TO_TICKS( 111 ) );

	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   pucBuffer[2] = { POWER_CONTROL_REGISTER, NORMAL_MODE };
	eModuleError_t eError;

	/* Turn chip on and configure mode */
	eError = eI2CBusStart( pxModule, &xAmg88xxConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, pucBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	pucBuffer[0] = FRAME_RATE_REGISTER;
	pucBuffer[1] = FRAME_RATE_10Hz;
	eError		 = eI2CTransmit( pxModule, pucBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	/* Delay to aquire data */
	vTaskDelay( pdMS_TO_TICKS( 110 ) );
	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Read data from chip */
	pucBuffer[0] = TEMPERATURE_REGISTER_BASE;
	eError		 = eI2CTransmit( pxModule, pucBuffer, 1, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CReceive( pxModule, (uint8_t *) pxData->psData, 128, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Turn chip off */
	pucBuffer[0] = POWER_CONTROL_REGISTER;
	pucBuffer[1] = SLEEP_MODE;
	eError		 = eI2CTransmit( pxModule, pucBuffer, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/
