/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "lis3dh.h"
#include "lis3dh_device.h"

#include "gpio.h"
#include "log.h"

/* Private Defines ------------------------------------------*/

// clang-format off

#define TEMP_REF 	21

// clang-format on

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvLis3dhInterruptRoutine( void );

static eModuleError_t eConfigurePowerAndSampleRate( xLis3dhConfig_t *pxConfig, TickType_t xTimeout );
static eModuleError_t eConfigureInterrupts( xLis3dhConfig_t *pxConfig, TickType_t xTimeout );

static eModuleError_t eWriteRegister( uint8_t ucRegister, uint8_t ucData, TickType_t xTimeout );
static eModuleError_t eReadRegisters( uint8_t ucRegister, uint8_t *pucData, uint8_t ucLength, TickType_t xTimeout );

static void	vParseBytes( xLis3dhData_t *data );
static uint8_t ucMapRangeToRegisterRange( eLis3dhGRange_t eRange );

/* Private Variables ----------------------------------------*/

static xI2CModule_t *pxModule;

static xI2CConfig_t xLisBusConfig = {
	.ulMaximumBusFrequency = 400000,
	.ucAddress			   = 0x32
};

static eLis3dhInterruptPin_t eLis3dhInterruptPin;
static xGpio_t				 xInterruptGpio;

static SemaphoreHandle_t xLis3dhInterruptSemaphore = NULL;
static StaticSemaphore_t xLis3dhInterruptSemaphoreStorage;

/*-----------------------------------------------------------*/

eModuleError_t eLis3dhInit( xLis3dhInit_t *pxInit, TickType_t xTimeout )
{
	eModuleError_t eError;

	configASSERT( xTimeout > pdMS_TO_TICKS( 6 ) );

	/* Store Configuration */
	pxModule			= pxInit->pxI2C;
	eLis3dhInterruptPin = pxInit->eLis3dhInterruptPin;
	xInterruptGpio		= pxInit->xInterrupt;

	/* Get ourselves a semaphore for interrupts. */
	xLis3dhInterruptSemaphore = xSemaphoreCreateBinaryStatic( &xLis3dhInterruptSemaphoreStorage );

	/* Set up the interrupt pin */
	vGpioSetup( xInterruptGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	/* Wait for sensor to power up */
	vTaskDelay( pdMS_TO_TICKS( 6 ) + 1 ); // Startup time is 5ms

	/* Put Chip into low power state */
	eError = eWriteRegister( CTRL_REG1, SAMPLING_RATE_OFF | CTRL_LOW_POWER_MODE_ENABLE, xTimeout );
	if ( eError == ERROR_NONE ) {
		eLog( LOG_IMU_DRIVER, LOG_INFO, "LIS3DH Initialisation Complete\r\n" );
	}
	else {
		eLog( LOG_IMU_DRIVER, LOG_ERROR, "LIS3DH Initialisation Failed\r\n" );
	}
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eLis3dhConfigure( xLis3dhConfig_t *pxConfig, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xEndTime = xTaskGetTickCount() + xTimeout;

	/* Configure Interrupts */
	eError = eConfigureInterrupts( pxConfig, xTimeout );

	if ( pxConfig->bEnable == false ) {

		/* Put Chip into low power state */
		eError = eWriteRegister( CTRL_REG1, SAMPLING_RATE_OFF | CTRL_LOW_POWER_MODE_ENABLE, xEndTime - xTaskGetTickCount() );

		return eError;
	}

	/* Set the range register values */
	uint8_t ucDataRegister = ucMapRangeToRegisterRange( pxConfig->eGRange ) | CTRL_DATA_BDU;
	eError				   = eWriteRegister( CTRL_REG4, ucDataRegister, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Set the filtering register values */
	eError = eWriteRegister( CTRL_REG2, 0x00, xEndTime - xTaskGetTickCount() );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	eError = eWriteRegister( TEMP_CFG_REG, ADC_EN | TEMP_EN, xEndTime - xTaskGetTickCount() );

	/* Configure power and sample rate */
	eError = eConfigurePowerAndSampleRate( pxConfig, xEndTime - xTaskGetTickCount() );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eLis3dhRead( xLis3dhData_t *xData, TickType_t xTimeout )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;

	/* Burst read all the data registers */
	eError = eReadRegisters( OUT_X_L, (uint8_t *) xData, 6, xTimeout );

	/* 
	 * Read lower byte of ADC3 and discard it, 
	 * needed for BDU mode to update temperature readings in the higher byte of ADC3
	 */
	eError = eReadRegisters( OUT_ADC3_L, &ucResponse, 1, xTimeout );

	/* Read temperature from higher byte of ADC3 */
	eError = eReadRegisters( OUT_ADC3_H, (uint8_t *) xData + 6, 1, xTimeout );

	/*
	 * Parse accelerometer and temperature values from I2C to standard format
	 * Shift accelerometer values which were previously left aligned
	 * and add an offset to get a valid temperature
	 */
	vParseBytes( xData );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eLis3dhWhoAmI( uint8_t *pucWhoAmI, TickType_t xTimeout )
{
	eModuleError_t eError;

	/* Read the chip ID */
	eError = eReadRegisters( WHO_AM_I, pucWhoAmI, 1, xTimeout );

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * Basic Interrupt Service Routine.
 *
 * This function essentially just signals that data is ready to be taken. It
 * allows our main task to wait for an interrupt to read data.
 **/
void prvLis3dhInterruptRoutine( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	xSemaphoreGiveFromISR( xLis3dhInterruptSemaphore, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

/**
 * Basic 'Wait for Interrupt' function.
 *
 * Waits for an interrupt from the LIS3DH chip.
 **/
eModuleError_t eLis3dhWaitForInterrupt( TickType_t xTimeout )
{
	if ( xSemaphoreTake( xLis3dhInterruptSemaphore, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * Gets the type of interrupt which was triggered last.
 **/
eModuleError_t eLis3dhGetInterruptType( eLis3dhInterruptType_t *pxInterruptType, TickType_t xTimeout )
{
	eModuleError_t eError = ERROR_NONE;
	uint8_t		   pucData;

	/* Read Interrupt 1 source register */
	eError = eReadRegisters( INT1_SRC, &pucData, 1, xTimeout );

	/*
	 * I haven't implemented all the possible interrupts in this case
	 * statement, only the ones we use. If you wish to use another interrupt
	 * type, add it to this switch/case statement.
	 */
	if ( ( pucData & INT_SRC_IA ) != 0 ) {
		*pxInterruptType = INT_NEW_DATA;
	}

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t eConfigurePowerAndSampleRate( xLis3dhConfig_t *pxConfig, TickType_t xTimeout )
{
	if ( !pxConfig->bEnable ) {
		pxConfig->eSampleRate = SAMPLE_RATE_OFF;
	}
	eModuleError_t eError = eWriteRegister( CTRL_REG1, ( pxConfig->eSampleRate << 4 ) | CTRL_LOW_POWER_MODE_ENABLE | CTRL_X_ENABLE | CTRL_Y_ENABLE | CTRL_Z_ENABLE, xTimeout );

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t eConfigureInterrupts( xLis3dhConfig_t *pxConfig, TickType_t xTimeout )
{
	configASSERT( pxModule != NULL );
	eModuleError_t eError;
	uint8_t		   ucData = 0;

	/* Only configure interrupt pin if chip is enabled */
	if ( pxConfig->bEnable ) {
		/* Enable interrupt on pin INT1 */
		ucData = CTRL_INT1_IA1;
		/* The list below is not complete, add config code for your interrupt types if they're not yet covered */
		/* Data ready interrupt */
		if ( pxConfig->usInterruptEnable & INT_NEW_DATA ) {
			ucData |= CTRL_INT1_ZYXDA;
		}
		eError = eWriteRegister( CTRL_REG3, ucData, xTimeout );

		/* Configure GPIO interrupt */
		vGpioSetup( xInterruptGpio, GPIO_INPUT, GPIO_INPUT_NOFILTER );
		eError = eGpioConfigureInterrupt( xInterruptGpio, true, GPIO_INTERRUPT_RISING_EDGE, prvLis3dhInterruptRoutine );
	}
	else {
		/* Explicitly disable interrupt pin when chip is disabled */
		vGpioSetup( xInterruptGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
		eError = eGpioConfigureInterrupt( xInterruptGpio, false, GPIO_INTERRUPT_BOTH_EDGE, NULL );
	}

	/* If there is an error setting up the interrupts, print it out. */
	if ( eError != ERROR_NONE ) {
		eLog( LOG_IMU_DRIVER, LOG_ERROR, "Error Setting up Interrupts: %d\r\n", eError );
	}

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t eReadRegisters( uint8_t ucRegister, uint8_t *pucData, uint8_t ucLength, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xEndTime		= xTaskGetTickCount() + xTimeout;
	uint8_t		   ucCommand[1] = { LIS3DH_AUTO_INCREMENT | ucRegister };

	eError = eI2CBusStart( pxModule, &xLisBusConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Transmit and Receive Data */
	eError = eI2CTransmit( pxModule, ucCommand, 1, xEndTime - xTaskGetTickCount() );
	if ( eError == ERROR_NONE ) {
		eError = eI2CReceive( pxModule, pucData, ucLength, xEndTime - xTaskGetTickCount() );
	}
	eI2CBusEnd( pxModule );
	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteRegister( uint8_t ucRegister, uint8_t ucData, TickType_t xTimeout )
{
	eModuleError_t eError;
	TickType_t	 xEndTime = xTaskGetTickCount() + xTimeout;

	uint8_t pucCommand[2] = { ucRegister, ucData };
	eError				  = eI2CBusStart( pxModule, &xLisBusConfig, xTimeout );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Transfer Data */
	eError = eI2CTransmit( pxModule, pucCommand, 2, xEndTime - xTaskGetTickCount() );
	eI2CBusEnd( pxModule );
	return eError;
}

/*-----------------------------------------------------------*/

static uint8_t ucMapRangeToRegisterRange( eLis3dhGRange_t eRange )
{
	switch ( eRange ) {
		case RANGE_2G:
			return FS_RANGE_2G;
		case RANGE_4G:
			return FS_RANGE_4G;
		case RANGE_8G:
			return FS_RANGE_8G;
		case RANGE_16G:
			return FS_RANGE_16G;
		default:
			return FS_RANGE_2G;
	}
}

/*-----------------------------------------------------------*/

static void vParseBytes( xLis3dhData_t *pxData )
{
	/* Shift bytes right because the 8 bit values are left aligned. */
	pxData->sAccX = ( pxData->sAccX ) >> 8;
	pxData->sAccY = ( pxData->sAccY ) >> 8;
	pxData->sAccZ = ( pxData->sAccZ ) >> 8;

	/*
	 * Adjust raw temperature reading with a reference to obtain a realistic temperature 
	 * The temperature resolution is 8-bit and ranges from -40 to +85 degrees
	 */
	pxData->cTemp = pxData->cTemp + TEMP_REF;
}

/*-----------------------------------------------------------*/

uint32_t ulLis3dhGetPeriodUS( eLis3dhSampleRate_t eRate )
{
	uint32_t ulTimeout = 0;
	switch ( eRate ) {
		case SAMPLE_RATE_1HZ:
			ulTimeout = 1000000;
			break;
		case SAMPLE_RATE_10HZ:
			ulTimeout = 100000;
			break;
		case SAMPLE_RATE_25HZ:
			ulTimeout = 40000;
			break;
		case SAMPLE_RATE_50HZ:
			ulTimeout = 20000;
			break;
		case SAMPLE_RATE_100HZ:
			ulTimeout = 10000;
			break;
		case SAMPLE_RATE_200HZ:
			ulTimeout = 5000;
			break;
		case SAMPLE_RATE_400HZ:
			ulTimeout = 2500;
			break;
		case SAMPLE_RATE_1600HZ:
			ulTimeout = 625;
			break;
		default:
			configASSERT( 0 );
	}
	return ulTimeout;
}

/*-----------------------------------------------------------*/
