/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "em_cmu.h"
#include "em_gpio.h"
#include "em_i2c.h"

#include "gpio.h"
#include "i2c.h"
#include "i2c_arch.h"

#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* Functions ------------------------------------------------*/

eModuleError_t eI2CInit( xI2CModule_t *pxModule )
{
	// Assert that platform pins have been set, or we get really hard to debug errors.
	ASSERT_GPIO_ASSIGNED( pxModule->xPlatform.xSda );
	ASSERT_GPIO_ASSIGNED( pxModule->xPlatform.xScl );
	ASSERT_LOCATION_ASSIGNED( pxModule->xPlatform.ulLocationScl );
	ASSERT_LOCATION_ASSIGNED( pxModule->xPlatform.ulLocationSda );

	// Create a mutex for our I2C module.
	pxModule->xBusMutexHandle = xSemaphoreCreateMutexStatic( &( pxModule->xBusMutexStorage ) );

	// Turn on the I2C clock. TODO: Maybe move this later when power testing.
	if ( pxModule->xPlatform.pxI2C == I2C0 ) {
		CMU_ClockEnable( cmuClock_I2C0, true );
	}
	else if ( pxModule->xPlatform.pxI2C == I2C1 ) {
		CMU_ClockEnable( cmuClock_I2C1, true );
	}

	vGpioSetup( pxModule->xPlatform.xSda, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( pxModule->xPlatform.xScl, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	pxModule->xPlatform.pxI2C->ROUTELOC0 = ( pxModule->xPlatform.ulLocationSda | pxModule->xPlatform.ulLocationScl );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

bool bI2CCanDeepSleep( xI2CModule_t *xModule )
{
	return !( xModule->bBusClaimed );
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CBusStart( xI2CModule_t *pxModule, xI2CConfig_t *pxConfig, TickType_t xTimeout )
{
	configASSERT( pxConfig != NULL );

	// Take I2C bus mutex to guarentee we are the only task using this module at this time.
	if ( xSemaphoreTake( pxModule->xBusMutexHandle, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}

	// Claim the bus and set the current configuration.
	pxModule->bBusClaimed	 = true;
	pxModule->pxCurrentConfig = pxConfig;

	// Set up the GPIO into a mode which works for I2C.
	vGpioSetup( pxModule->xPlatform.xSda, GPIO_OPENDRAIN, GPIO_OPENDRAIN_HIGH );
	vGpioSetup( pxModule->xPlatform.xScl, GPIO_OPENDRAIN, GPIO_OPENDRAIN_HIGH );

	// Check that the clock speed required by the current chip isn't exceeded by EFR32 default settings.
	I2C_Init_TypeDef xDefaultConfig = I2C_INIT_DEFAULT;
	if ( pxModule->pxCurrentConfig->ulMaximumBusFrequency < xDefaultConfig.freq ) {
		xDefaultConfig.freq = pxModule->pxCurrentConfig->ulMaximumBusFrequency;
	}

	// Connect the I2C module to the pins
	pxModule->xPlatform.pxI2C->ROUTEPEN = ( I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN );

	// Configure everything and enable I2C
	I2C_Init( pxModule->xPlatform.pxI2C, &xDefaultConfig );
	I2C_Enable( pxModule->xPlatform.pxI2C, true );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CBusEnd( xI2CModule_t *pxModule )
{
	configASSERT( pxModule->bBusClaimed == true );

	pxModule->bBusClaimed	 = false;
	pxModule->pxCurrentConfig = NULL;

	// Return the GPIO pins back to a normal state
	vGpioSetup( pxModule->xPlatform.xSda, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( pxModule->xPlatform.xScl, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	// Disconnect the I2C module from the pins
	pxModule->xPlatform.pxI2C->ROUTEPEN = ( 0 );

	// Return the mutex
	xSemaphoreGive( pxModule->xBusMutexHandle );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CTransmit( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout )
{
	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	// Create and set up a data transfer struct.
	I2C_TransferSeq_TypeDef xTransfer = { 0 };

	xTransfer.addr		  = pxModule->pxCurrentConfig->ucAddress;
	xTransfer.flags		  = I2C_FLAG_WRITE;
	xTransfer.buf[0].data = pvBuffer;
	xTransfer.buf[0].len  = usLength;

	// Send off the struct and wait for the I2C transaction to finish.
	I2C_TransferReturn_TypeDef eResult = I2C_TransferInit( pxModule->xPlatform.pxI2C, &xTransfer );
	while ( eResult == i2cTransferInProgress ) {
		if ( ( xTaskGetTickCount() - xEntryTime ) > xTimeout ) {
			break;
		}
		eResult = I2C_Transfer( pxModule->xPlatform.pxI2C );
	}

	switch ( eResult ) {

		case i2cTransferDone:
			return ERROR_NONE;

		case i2cTransferNack:
			return ERROR_NO_ACKNOWLEDGEMENT;

		case i2cTransferInProgress:
			return ERROR_TIMEOUT;

		default:
			return ERROR_GENERIC;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CReceive( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout )
{
	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	// Create and set up a data transfer struct.
	I2C_TransferSeq_TypeDef xTransfer = { 0 };

	xTransfer.addr		  = pxModule->pxCurrentConfig->ucAddress;
	xTransfer.flags		  = I2C_FLAG_READ;
	xTransfer.buf[0].data = pvBuffer;
	xTransfer.buf[0].len  = usLength;

	// Send off the struct and wait for the I2C transaction to finish.
	I2C_TransferReturn_TypeDef eResult = I2C_TransferInit( pxModule->xPlatform.pxI2C, &xTransfer );
	while ( eResult == i2cTransferInProgress ) {
		if ( xTaskGetTickCount() - xEntryTime > xTimeout ) {
			break;
		}
		eResult = I2C_Transfer( pxModule->xPlatform.pxI2C );
	}

	switch ( eResult ) {

		case i2cTransferDone:
			return ERROR_NONE;

		case i2cTransferNack:
			return ERROR_NO_ACKNOWLEDGEMENT;

		case i2cTransferInProgress:
			return ERROR_TIMEOUT;

		default:
			return ERROR_GENERIC;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CTransfer( xI2CModule_t *pxModule, void *pvSendBuffer, uint16_t usSendLength, void *pvReceiveBuffer, uint16_t usReceiveLength, TickType_t xTimeout )
{
	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	// Create and set up a data transfer struct.
	I2C_TransferSeq_TypeDef xTransfer = { 0 };

	xTransfer.addr		  = pxModule->pxCurrentConfig->ucAddress;
	xTransfer.flags		  = I2C_FLAG_WRITE_READ;
	xTransfer.buf[0].data = pvSendBuffer;
	xTransfer.buf[0].len  = usSendLength;
	xTransfer.buf[1].data = pvReceiveBuffer;
	xTransfer.buf[1].len  = usReceiveLength;

	// Send off the struct and wait for the I2C transaction to finish.
	I2C_TransferReturn_TypeDef eResult = I2C_TransferInit( pxModule->xPlatform.pxI2C, &xTransfer );
	while ( eResult == i2cTransferInProgress ) {
		if ( xTaskGetTickCount() - xEntryTime > xTimeout ) {
			break;
		}
		eResult = I2C_Transfer( pxModule->xPlatform.pxI2C );
	}

	switch ( eResult ) {

		case i2cTransferDone:
			return ERROR_NONE;

		case i2cTransferNack:
			return ERROR_NO_ACKNOWLEDGEMENT;

		case i2cTransferInProgress:
			return ERROR_TIMEOUT;

		default:
			return ERROR_GENERIC;
	}
}

/*-----------------------------------------------------------*/
