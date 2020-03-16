/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "spi.h"
#include "gpio.h"

#include "spidrv.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvSpiInit( xSpiModule_t *pxSpi );

static SPIDRV_ClockMode_t prvClockModeConversion( eSpiClockMode_t eClockMode );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t eSpiInit( xSpiModule_t *pxSpi )
{
	ASSERT_LOCATION_ASSIGNED( pxSpi->xPlatform.ucPortLocationMiso );
	ASSERT_LOCATION_ASSIGNED( pxSpi->xPlatform.ucPortLocationMosi );
	ASSERT_LOCATION_ASSIGNED( pxSpi->xPlatform.ucPortLocationSclk );

	pxSpi->xBusMutexHandle		  = xSemaphoreCreateRecursiveMutexStatic( &pxSpi->xBusMutexStorage );
	pxSpi->xTransactionDoneHandle = xSemaphoreCreateBinaryStatic( &pxSpi->xTransactionDoneStorage );
	pxSpi->xPlatform.xDrvHandle   = &pxSpi->xPlatform.xDrvStorage;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSpiBusStart( xSpiModule_t *pxSpi, const xSpiConfig_t *xConfig, TickType_t xTimeout )
{
	if ( xSemaphoreTakeRecursive( pxSpi->xBusMutexHandle, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	pxSpi->bBusClaimed	 = true;
	pxSpi->bCsAsserted	 = false;
	pxSpi->pxCurrentConfig = xConfig;

	vGpioSetup( pxSpi->pxCurrentConfig->xCsGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	prvSpiInit( pxSpi );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vSpiBusEnd( xSpiModule_t *pxSpi )
{
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( !pxSpi->bCsAsserted );

	/* We are done with the bus, deinitialise it to save power */
	SPIDRV_DeInit( pxSpi->xPlatform.xDrvHandle );
	pxSpi->bBusClaimed	 = false;
	pxSpi->pxCurrentConfig = NULL;

	/* Return the semaphore */
	xSemaphoreGiveRecursive( pxSpi->xBusMutexHandle );
}

/*-----------------------------------------------------------*/

eModuleError_t eSpiBusLockout( xSpiModule_t *pxSpi, bool bEnableLockout, TickType_t xTimeout )
{
	BaseType_t xRet;
	if ( bEnableLockout ) {
		xRet = xSemaphoreTakeRecursive( pxSpi->xBusMutexHandle, xTimeout );
	}
	else {
		xRet = xSemaphoreGiveRecursive( pxSpi->xBusMutexHandle );
	}
	return ( xRet == pdPASS ) ? ERROR_NONE : ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

void vSpiCsAssert( xSpiModule_t *pxSpi )
{
	configASSERT( pxSpi->bBusClaimed );
	vGpioSetup( pxSpi->pxCurrentConfig->xCsGpio, GPIO_OPENDRAIN, GPIO_OPENDRAIN_LOW );
	pxSpi->bCsAsserted = true;
}
/*-----------------------------------------------------------*/

void vSpiCsRelease( xSpiModule_t *pxSpi )
{
	configASSERT( pxSpi->bBusClaimed );
	vGpioSetup( pxSpi->pxCurrentConfig->xCsGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	pxSpi->bCsAsserted = false;
}

/*-----------------------------------------------------------*/

bool bSpiCanDeepSleep( xSpiModule_t *pxSpi )
{
	/* If bus is claimed, it is initialised and most likely transferring data */
	return !( pxSpi->bBusClaimed );
}

/*-----------------------------------------------------------*/

void vSpiTransmit( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen )
{
	Ecode_t eError;
	/* Check that bus is in the correct state for a transaction */
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( pxSpi->bCsAsserted );
	/* Check that we are trying to send at least one byte */
	configASSERT( ulBufferLen > 0 );

	/* Transmit the buffer */
	eError = SPIDRV_MTransmit( pxSpi->xPlatform.xDrvHandle, pvBuffer, ulBufferLen, pxSpi->xPlatform.xTransactionDoneCallback );
	configASSERT( eError == ECODE_EMDRV_SPIDRV_OK );

	/**
	 *  Wait up to a second for the transaction to complete.
	 *  If the semaphore has not been returned at this point, something is terribly wrong, so assert
	 **/
	configASSERT( xSemaphoreTake( pxSpi->xTransactionDoneHandle, pdMS_TO_TICKS( 1000 ) ) == pdPASS );
}

/*-----------------------------------------------------------*/

void vSpiReceive( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen )
{
	Ecode_t eError;
	/* Check that bus is in the correct state for a transaction */
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( pxSpi->bCsAsserted );
	/* Check that we are trying to receive at least one byte */
	configASSERT( ulBufferLen > 0 );

	/* Transmit the buffer */
	eError = SPIDRV_MReceive( pxSpi->xPlatform.xDrvHandle, pvBuffer, ulBufferLen, pxSpi->xPlatform.xTransactionDoneCallback );
	configASSERT( eError == ECODE_EMDRV_SPIDRV_OK );

	/**
	 *  Wait up to a second for the transaction to complete.
	 *  If the semaphore has not been returned at this point, something is terribly wrong, so assert
	 **/
	configASSERT( xSemaphoreTake( pxSpi->xTransactionDoneHandle, pdMS_TO_TICKS( 1000 ) ) == pdPASS );
}

/*-----------------------------------------------------------*/

void vSpiTransfer( xSpiModule_t *pxSpi, void *pvTxBuffer, void *pvRxBuffer, uint32_t ulBufferLen )
{
	Ecode_t eError;
	/* Check that bus is in the correct state for a transaction */
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( pxSpi->bCsAsserted );
	/* Check that we are trying to receive at least one byte */
	configASSERT( ulBufferLen > 0 );

	eError = SPIDRV_MTransfer( pxSpi->xPlatform.xDrvHandle, pvTxBuffer, pvRxBuffer, ulBufferLen, pxSpi->xPlatform.xTransactionDoneCallback );
	configASSERT( eError == ECODE_EMDRV_SPIDRV_OK );

	/**
	 *  Wait up to a second for the transaction to complete.
	 *  If the semaphore has not been returned at this point, something is terribly wrong, so assert
	 **/
	configASSERT( xSemaphoreTake( pxSpi->xTransactionDoneHandle, pdMS_TO_TICKS( 1000 ) ) == pdPASS );
}

/*-----------------------------------------------------------*/

static void prvSpiInit( xSpiModule_t *pxSpi )
{
	SPIDRV_Init_t xInitData;

	xInitData.port			  = pxSpi->xPlatform.xInstance;
	xInitData.portLocationTx  = pxSpi->xPlatform.ucPortLocationMosi;
	xInitData.portLocationRx  = pxSpi->xPlatform.ucPortLocationMiso;
	xInitData.portLocationClk = pxSpi->xPlatform.ucPortLocationSclk;
	xInitData.portLocationCs  = 0;

	xInitData.bitRate		 = pxSpi->pxCurrentConfig->ulMaxBitrate;
	xInitData.frameLength	= 8;
	xInitData.dummyTxValue   = pxSpi->pxCurrentConfig->ucDummyTx;
	xInitData.type			 = spidrvMaster;
	xInitData.bitOrder		 = ( pxSpi->pxCurrentConfig->ucMsbFirst ) ? spidrvBitOrderMsbFirst : spidrvBitOrderLsbFirst;
	xInitData.clockMode		 = prvClockModeConversion( pxSpi->pxCurrentConfig->eClockMode );
	xInitData.slaveStartMode = spidrvSlaveStartImmediate;

	/**
	 *  We use manual control because some chipsets use CS line for signalling
	 *  It's easier to force all drivers to manually control it than enforce
	 *  some sort of 'Best Practice' as to when it is ok to manually control the port
	 **/
	xInitData.csControl = spidrvCsControlApplication;

	SPIDRV_Init( pxSpi->xPlatform.xDrvHandle, &xInitData );
}

/*-----------------------------------------------------------*/

SPIDRV_ClockMode_t prvClockModeConversion( eSpiClockMode_t eClockMode )
{
	switch ( eClockMode ) {
		case eSpiClockMode0:
			return spidrvClockMode0;
		case eSpiClockMode1:
			return spidrvClockMode1;
		case eSpiClockMode2:
			return spidrvClockMode2;
		case eSpiClockMode3:
			return spidrvClockMode3;
		default:
			/* This should never happen */
			configASSERT( 0 );
			return spidrvClockMode0;
	}
}

/*-----------------------------------------------------------*/
