/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "gpio.h"
#include "i2c.h"

#include "mlx90614.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

// remember WAKEUP_WAIT_TIME = 35ms

/* Type Definitions -----------------------------------------*/

/**
 * Note: This chip doesn't expect I2C messages like a regular chip. You can't
 * transmit the address of the register you want to read, then send a seperate
 * read request, that fails. You need to do the exact same thing, but using a
 * 'I2C transfer' to jam both messages together into the same 'transaction'.
 **/

typedef enum eMlx90614EepromAddressMap_t {
	TO_MAX							  = 0x00,
	TO_MIN							  = 0x01,
	PWM_CONTROL						  = 0x02,
	TA_RANGE						  = 0x03,
	EMISSIVITY_CORRECTION_COEFFICIENT = 0x04,
	CONFIG_REGISTER_1				  = 0x05,
	I2C_ADDRESS_LSB					  = 0x0E,
	ID_NUMBER_1						  = 0x1C, // Read Only
	ID_NUMBER_2						  = 0x1D, // Read Only
	ID_NUMBER_3						  = 0x1E, // Read Only
	ID_NUMBER_4						  = 0x1F  // Read Only
} eMlx90614EepromAddressMap_t;

typedef enum eMlx90614RamAddressMap_t {
	RAW_DATA_IR_1 = 0x04,
	RAW_DATA_IR_2 = 0x05,
	TEMP_AMBIENT  = 0x06, // Ambient Temperature. (T_A in datasheet.)
	TEMP_IR_1	 = 0x07, // Temperature measured from IR. (T_O1 in datasheet).
	TEMP_IR_2	 = 0x08  // Temperature measured from IR. (T_O2 in datasheet). (Only used for dual zone chips).
} eMlx90614RamAddressMap_t;

typedef enum eMlx90614CommandMask_t {
	RAM			= 0b00000000, // 000x xxxx
	EEPROM		= 0b00100000, // 001x xxxx
	READ_FLAGS  = 0b11110000, // 1111 0000
	ENTER_SLEEP = 0b11111111  // 1111 1111
} eMlx90614MemoryTypeMask_t;

/* Function Declarations ------------------------------------*/

void prvConvertTemperatureToCentiDegrees( xMlx90614Data_t *pxData );

/* Private Variables ----------------------------------------*/

static xI2CModule_t *pxModule;

static xI2CConfig_t xMlx90614Config = {
	.ulMaximumBusFrequency = 100000,
	.ucAddress			   = MLX90614_ADDRESS << 1
};

/* Functions ------------------------------------------------*/

eModuleError_t eMlx90614Init( xMlx90614Init_t *pxInit, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   pucSleepCommand[2] = { ENTER_SLEEP, 0xE8 }; // 0xE8 is a CRC. I copied it from an example in the datasheet.

	configASSERT( pxInit->pxModule != NULL );

	pxModule = pxInit->pxModule;

	/* Put chip into sleep mode */
	eError = eI2CBusStart( pxModule, &xMlx90614Config, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, pucSleepCommand, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Reads the slave address of the chip. It should be 0x5A.
 **/

eModuleError_t eMlx90614WhoAmI( uint8_t *pucAddress, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   ucCommand[2] = { I2C_ADDRESS_LSB | EEPROM };
	uint16_t	   usStorage;

	/* Start I2C Bus */
	eError = eI2CBusStart( pxModule, &xMlx90614Config, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Get register via I2C */
	eError = eI2CTransfer( pxModule, ucCommand, 1, (uint8_t *) &usStorage, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	/* Stop I2C interface */
	eError = eI2CBusEnd( pxModule );

	/* Store address */
	*pucAddress = usStorage;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eMlx90614TurnOn( void )
{
	/**
     * Pull chip out of sleep mode.
     * Essentially just pull the SDA pin low for at *least* 33ms.
     **/

	vGpioSetup( pxModule->xPlatform.xScl, GPIO_OPENDRAIN, GPIO_OPENDRAIN_HIGH );
	vGpioSetup( pxModule->xPlatform.xSda, GPIO_OPENDRAIN, GPIO_OPENDRAIN_LOW );

	vTaskDelay( pdMS_TO_TICKS( 50 ) );

	vGpioSetup( pxModule->xPlatform.xScl, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( pxModule->xPlatform.xSda, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * Note: This chip seems to produce data at a rate slightly slower than 10Hz.
 * Don't try to read faster than that. It will work, but you'll get loads of
 * duplicate data. This is known from trial and error, because I can't find
 * anything useful in the damn datasheet.
 **/

eModuleError_t eMlx90614Read( xMlx90614Data_t *pxData, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;
	uint8_t		   pucReadAmbient[1] = { TEMP_AMBIENT | RAM };
	uint8_t		   pucReadObj1[1]	= { TEMP_IR_1 | RAM };
	uint8_t		   pucStorage[3];

	/* Start bus */
	eError = eI2CBusStart( pxModule, &xMlx90614Config, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Read ambient temperature */
	eError = eI2CTransfer( pxModule, pucReadAmbient, 1, pucStorage, 3, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}
	pvMemcpy( (void *) &( pxData->sAmbientTemperature ), pucStorage, 2 );

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	/* Read object IR temperature */
	eError = eI2CTransfer( pxModule, pucReadObj1, 1, pucStorage, 3, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}
	pvMemcpy( (void *) &( pxData->sObjectTemperature ), pucStorage, 2 );

	/* Relinquish bus control */
	eError = eI2CBusEnd( pxModule );

	/* Convert Data */
	prvConvertTemperatureToCentiDegrees( pxData );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eMlx90614TurnOff( TickType_t xTimeout )
{
	eModuleError_t eError;
	uint8_t		   pucSleepCommand[2] = { 0xFF, 0xE8 };
	TickType_t	 xStartTime		  = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;

	/* Put chip back into sleep mode */
	eError = eI2CBusStart( pxModule, &xMlx90614Config, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eI2CTransmit( pxModule, pucSleepCommand, 2, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		eI2CBusEnd( pxModule );
		return eError;
	}

	eError = eI2CBusEnd( pxModule );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * For the time being, this function simply calls the other functions in order
 * and handles the timeouts. Potentially in the future if it is needed, this
 * can be extended to change configurations and the like. For now it just reads
 * a single piece of data.
 * 
 * Note: A certain length timeout is required to exit sleep mode.
 **/
eModuleError_t eMlx90614ReadSingle( xMlx90614Data_t *pxData, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xStartTime = xTaskGetTickCount();
	TickType_t	 xCumulativeTimeout;

	configASSERT( xTimeout > pdMS_TO_TICKS( 300 ) );

	eMlx90614TurnOn();

	vTaskDelay( pdMS_TO_TICKS( 250 ) );

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eMlx90614Read( pxData, xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	xCumulativeTimeout = xTimeout - ( xTaskGetTickCount() - xStartTime );

	eError = eMlx90614TurnOff( xCumulativeTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Convert raw data to centi-degrees celcius.
 * 
 * Note: By using an int16 instead of an int32 to store the final answer, this
 * means our maximum range is +- 327.68 degrees, not all the way up to 382.19
 * offered by the chip. Since I don't imaging this effecting any of our use
 * cases, I am going to ignore this.
 * 
 * Conversion equation: Temp_centideg = (( Temp_raw * 100 ) / 50 ) - 27315
 **/
void prvConvertTemperatureToCentiDegrees( xMlx90614Data_t *pxData )
{
	int32_t lStorage;

	/* Conversion 1 */
	lStorage = 2 * (int32_t) pxData->sAmbientTemperature - 27315;
	if ( lStorage > INT16_MAX ) {
		lStorage = INT16_MAX;
	}
	pxData->sAmbientTemperature = lStorage;

	/* Conversion 2 */
	lStorage = 2 * (int32_t) pxData->sObjectTemperature - 27315;
	if ( lStorage > INT16_MAX ) {
		lStorage = INT16_MAX;
	}
	pxData->sObjectTemperature = lStorage;
}

/*-----------------------------------------------------------*/
