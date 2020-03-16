/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "si1133.h"

#include "i2c.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

eModuleError_t eReset( void );
eModuleError_t eForceMeasurement( void );
eModuleError_t eReadMeasurements( xSi1133Data_t *pxData );
eModuleError_t eWaitUntilSleep( void );
eModuleError_t eSendCommand( eSi1133Command_t eCommand );
eModuleError_t eSetParameter( eSi1133Parameter_t eParameter, uint8_t ucValue );

eModuleError_t eReadRegisterBlock( eSi1133Register_t eRegister, uint8_t ucLength, uint8_t *pucData );
eModuleError_t eWriteRegisterBlock( eSi1133Register_t eRegister, uint8_t ucLength, uint8_t *pucData );
eModuleError_t eReadRegister( eSi1133Register_t eRegister, uint8_t *pucData );
eModuleError_t eWriteRegister( eSi1133Register_t eRegister, uint8_t ucData );

/* Private Variables ----------------------------------------*/

xI2CModule_t *pxModule = NULL;

static xI2CConfig_t xConfig = {
	.ulMaximumBusFrequency = 1000000,
	.ucAddress			   = SI1133_ADDRESS << 1
};

/* Public Functions -----------------------------------------*/

eModuleError_t eSi1133Init( xSi1133Init_t *pxInit )
{
	configASSERT( pxInit->pxModule );
	pxModule = pxInit->pxModule;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSi1133Config( void )
{
	eModuleError_t eError;

	/* Allow some time for the part to power up */
	vTaskDelay( pdMS_TO_TICKS( 5 ) );

	eError = eReset();

	vTaskDelay( pdMS_TO_TICKS( 10 ) );

	eError |= eSetParameter( PARAM_CH_LIST, 0x0F );
	eError |= eSetParameter( PARAM_ADCCONFIG0, 0x78 );
	eError |= eSetParameter( PARAM_ADCSENS0, 0x71 );
	eError |= eSetParameter( PARAM_ADCPOST0, 0x40 );
	eError |= eSetParameter( PARAM_ADCCONFIG1, 0x4D );
	eError |= eSetParameter( PARAM_ADCSENS1, 0xE1 );
	eError |= eSetParameter( PARAM_ADCPOST1, 0x40 );
	eError |= eSetParameter( PARAM_ADCCONFIG2, 0x41 );
	eError |= eSetParameter( PARAM_ADCSENS2, 0xE1 );
	eError |= eSetParameter( PARAM_ADCPOST2, 0x50 );
	eError |= eSetParameter( PARAM_ADCCONFIG3, 0x4D );
	eError |= eSetParameter( PARAM_ADCSENS3, 0x87 );
	eError |= eSetParameter( PARAM_ADCPOST3, 0x40 );

	eError |= eWriteRegister( REG_IRQ_ENABLE, 0x0F );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Read samples from the Si1133 chip.
 **/
eModuleError_t eSi1133Read( xSi1133Data_t *pxSht31Data, TickType_t xTimeout )
{
	UNUSED( xTimeout );
	configASSERT( pxModule );
	eModuleError_t eError;
	uint8_t		   ucResponse;

	/* Force measurement */
	eError = eForceMeasurement();

	/* Go to sleep while the sensor does the conversion */
	vTaskDelay( pdMS_TO_TICKS( 200 ) );

	/* Check if the measurement finished, if not then wait */
	eError = eReadRegister( REG_IRQ_STATUS, &ucResponse );
	while ( ucResponse != 0x0F ) {
		vTaskDelay( pdMS_TO_TICKS( 5 ) );
		eError += eReadRegister( REG_IRQ_STATUS, &ucResponse );
	}

	/* Get the results */
	eReadMeasurements( pxSht31Data );
	return ERROR_NONE;
}

/* Private Functions ----------------------------------------*/

/**
 * Resets the Si1133
 * Returns zero on OK, non-zero otherwise
 **/
eModuleError_t eReset( void )
{
	eModuleError_t eError;

	/* Do not access the Si1133 earlier than 25 ms from power-up */
	vTaskDelay( pdMS_TO_TICKS( 30 ) );

	/* Perform the Reset Command */
	eError = eWriteRegister( REG_COMMAND, (uint8_t) CMD_RESET );

	/* Delay for 10 ms. This delay is needed to allow the Si1133   */
	/* to perform internal reset sequence.                         */
	vTaskDelay( pdMS_TO_TICKS( 10 ) );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Sends a FORCE command to the Si1133.
 */
eModuleError_t eForceMeasurement( void )
{
	return eSendCommand( CMD_FORCE_CH );
}

/*-----------------------------------------------------------*/

/**
 * Read samples from the Si1133 chip
 **/
eModuleError_t eReadMeasurements( xSi1133Data_t *pxData )
{
	eModuleError_t eError;
	uint8_t		   pucBuffer[13];

	eError = eReadRegisterBlock( REG_IRQ_STATUS, 13, pucBuffer );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	pxData->lUltraVioletCh0 = pucBuffer[1] << 16;
	pxData->lUltraVioletCh0 |= pucBuffer[2] << 8;
	pxData->lUltraVioletCh0 |= pucBuffer[3];
	if ( pxData->lUltraVioletCh0 & 0x800000 ) {
		pxData->lUltraVioletCh0 |= 0xFF000000;
	}

	pxData->lAmbientCh1 = pucBuffer[4] << 16;
	pxData->lAmbientCh1 |= pucBuffer[5] << 8;
	pxData->lAmbientCh1 |= pucBuffer[6];
	if ( pxData->lAmbientCh1 & 0x800000 ) {
		pxData->lAmbientCh1 |= 0xFF000000;
	}

	pxData->lAmbientCh2 = pucBuffer[7] << 16;
	pxData->lAmbientCh2 |= pucBuffer[8] << 8;
	pxData->lAmbientCh2 |= pucBuffer[9];
	if ( pxData->lAmbientCh2 & 0x800000 ) {
		pxData->lAmbientCh2 |= 0xFF000000;
	}

	pxData->lAmbientCh3 = pucBuffer[10] << 16;
	pxData->lAmbientCh3 |= pucBuffer[11] << 8;
	pxData->lAmbientCh3 |= pucBuffer[12];
	if ( pxData->lAmbientCh3 & 0x800000 ) {
		pxData->lAmbientCh3 |= 0xFF000000;
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * Waits until the Si1133 is sleeping before proceeding
 **/
eModuleError_t eWaitUntilSleep( void )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;
	uint8_t		   ucCount = 0;

	/* This loops until the Si1133 is known to be in its sleep state  */
	/* or if an i2c error occurs                                      */
	while ( ucCount < 5 ) {
		eError = eReadRegister( REG_RESPONSE0, &ucResponse );
		if ( eError != ERROR_NONE ) {
			return eError;
		}

		if ( ( ucResponse & (uint8_t) RSP0_CHIPSTAT_MASK ) == (uint8_t) RSP0_SLEEP ) {
			return ERROR_NONE;
		}

		ucCount++;
	}

	return ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

/**
 *    Helper function to send a command to the Si1133
 **/
eModuleError_t eSendCommand( eSi1133Command_t eCommand )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;
	uint8_t		   ucResponseStored;

	/* Get the response register contents */
	eError = eReadRegister( REG_RESPONSE0, &ucResponseStored );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	ucResponseStored = ucResponseStored & (uint8_t) RSP0_COUNTER_MASK;

	uint8_t ucCount = 0;
	/* Double-check the response register is consistent */
	while ( ucCount < 5 ) {
		eError = eWaitUntilSleep();
		if ( eError != ERROR_NONE ) {
			return eError;
		}
		/* Skip if the command is RESET COMMAND COUNTER */
		if ( eCommand == CMD_RESET_CMD_CTR ) {
			break;
		}

		eError = eReadRegister( REG_RESPONSE0, &ucResponse );

		if ( ( ucResponse & (uint8_t) RSP0_COUNTER_MASK ) == ucResponseStored ) {
			break;
		}
		else {
			if ( eError != ERROR_NONE ) {
				return eError;
			}
			else {
				ucResponseStored = ucResponse & (uint8_t) RSP0_COUNTER_MASK;
			}
		}

		ucCount++;
	}

	/* Send the command */
	eError = eWriteRegister( REG_COMMAND, eCommand );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	ucCount = 0;
	/* Expect a change in the response register */
	while ( ucCount < 5 ) {
		/* Skip if the command is RESET COMMAND COUNTER */
		if ( eCommand == CMD_RESET_CMD_CTR ) {
			break;
		}

		eError = eReadRegister( REG_RESPONSE0, &ucResponse );
		if ( ( ucResponse & (uint8_t) RSP0_COUNTER_MASK ) != ucResponseStored ) {
			break;
		}
		else {
			if ( eError != ERROR_NONE ) {
				return eError;
			}
		}

		ucCount++;
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * Writes a byte to an Si1133 Parameter
 **/
eModuleError_t eSetParameter( eSi1133Parameter_t eParameter, uint8_t ucValue )
{
	eModuleError_t eError;
	uint8_t		   pucData[2];
	uint8_t		   ucResponseStored;
	uint8_t		   ucResponse;

	eError = eWaitUntilSleep();
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	eReadRegister( REG_RESPONSE0, &ucResponseStored );
	ucResponseStored &= (uint8_t) RSP0_COUNTER_MASK;

	pucData[0] = ucValue;
	pucData[1] = 0x80 + ( (uint8_t) eParameter & 0x3F );

	eError = eWriteRegisterBlock( REG_HOSTIN0, 2, (uint8_t *) pucData );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Wait for command to finish */
	uint8_t ucCount = 0;
	/* Expect a change in the response register */
	while ( ucCount < 5 ) {
		eError = eReadRegister( REG_RESPONSE0, &ucResponse );
		if ( ( ucResponse & (uint8_t) RSP0_COUNTER_MASK ) != ucResponseStored ) {
			break;
		}
		else {
			if ( eError != ERROR_NONE ) {
				return eError;
			}
		}

		ucCount++;
	}

	if ( ucCount >= 5 ) {
		return ERROR_TIMEOUT;
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 *    Reads a block of data from the Si1133 sensor.
 **/
eModuleError_t eReadRegisterBlock( eSi1133Register_t eRegister, uint8_t ucLength, uint8_t *pucData )
{
	eModuleError_t eError;
	uint8_t		   ucRegister = (uint8_t) eRegister;

	eError = eI2CBusStart( pxModule, &xConfig, portMAX_DELAY );
	eError = eI2CTransmit( pxModule, &ucRegister, 1, portMAX_DELAY );
	eError = eI2CReceive( pxModule, pucData, ucLength, portMAX_DELAY );
	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Writes a block of data to the Si1133 sensor.
 **/
eModuleError_t eWriteRegisterBlock( eSi1133Register_t eRegister, uint8_t ucLength, uint8_t *pucData )
{
	eModuleError_t eError;
	uint8_t		   pucBuffer[3];
	pucBuffer[0] = (uint8_t) eRegister;

	configASSERT( ucLength <= 2 );

	pvMemcpy( &pucBuffer[1], pucData, ucLength );

	eError = eI2CBusStart( pxModule, &xConfig, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		return ERROR_GENERIC;
	}

	eError = eI2CTransmit( pxModule, pucBuffer, ucLength + 1, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return ERROR_GENERIC;
	}

	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Reads register from the Si1133 sensor
 **/
eModuleError_t eReadRegister( eSi1133Register_t eRegister, uint8_t *pucData )
{
	eModuleError_t eError;
	uint8_t		   ucBuffer = (uint8_t) eRegister;

	eI2CBusStart( pxModule, &xConfig, portMAX_DELAY );

	eError = eI2CTransmit( pxModule, &ucBuffer, 1, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eError = eI2CReceive( pxModule, pucData, 1, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Writes register to the Si1133 sensor.
 **/
eModuleError_t eWriteRegister( eSi1133Register_t eRegister, uint8_t ucData )
{
	eModuleError_t eError;
	uint8_t		   pucBuffer[2];
	pucBuffer[0] = eRegister;
	pucBuffer[1] = ucData;

	eError = eI2CBusStart( pxModule, &xConfig, portMAX_DELAY );

	eError = eI2CTransmit( pxModule, pucBuffer, 2, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	eI2CBusEnd( pxModule );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
