/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "gpio.h"
#include "i2c.h"
#include "i2c_arch.h"

#include "nrfx_errors.h"
#include "nrfx_twim.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static nrf_twim_frequency_t xGetOptimumFrequency( uint32_t ulFrequency );
static eModuleError_t		eGetErrorMessage( nrfx_err_t eError );
void						vErrata89Workaround( xI2CModule_t *pxModule );

/* Private Variables ----------------------------------------*/

/* Functions ------------------------------------------------*/

eModuleError_t eI2CInit( xI2CModule_t *pxModule )
{
	ASSERT_GPIO_ASSIGNED( pxModule->xPlatform.xSda );
	ASSERT_GPIO_ASSIGNED( pxModule->xPlatform.xScl );

	// Create a mutex for our I2C module.
	pxModule->xBusMutexHandle = xSemaphoreCreateMutexStatic( &( pxModule->xBusMutexStorage ) );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CBusStart( xI2CModule_t *pxModule, xI2CConfig_t *pxConfig, TickType_t xTimeout )
{
	configASSERT( pxConfig != NULL );

	nrfx_err_t eError;

	// Take I2C bus mutex to guarentee we are the only task using this module at this time.
	if ( xSemaphoreTake( pxModule->xBusMutexHandle, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}

	nrfx_twim_config_t xNrfDriverConfig = NRFX_TWIM_DEFAULT_CONFIG;
	xNrfDriverConfig.scl				= pxModule->xPlatform.xScl.ucPin;
	xNrfDriverConfig.sda				= pxModule->xPlatform.xSda.ucPin;
	xNrfDriverConfig.frequency			= xGetOptimumFrequency( pxConfig->ulMaximumBusFrequency );

	eError = nrfx_twim_init( &pxModule->xPlatform.xInstance, &xNrfDriverConfig, NULL, NULL );
	if ( eError != NRFX_SUCCESS ) {
		return ERROR_INITIALISATION_FAILURE;
	}

	/* Enable the module */
	nrfx_twim_enable( &pxModule->xPlatform.xInstance );

	// Claim the bus and set the current configuration.
	pxModule->bBusClaimed	 = true;
	pxModule->pxCurrentConfig = pxConfig;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CBusEnd( xI2CModule_t *pxModule )
{
	configASSERT( pxModule != NULL );
	configASSERT( pxModule->bBusClaimed == true );

	nrfx_twim_disable( &pxModule->xPlatform.xInstance );
	nrfx_twim_uninit( &pxModule->xPlatform.xInstance );

	/* Workaround to reduce current draw of NRF52832 after closing I2C bus */
	vErrata89Workaround( pxModule );

	pxModule->bBusClaimed	 = false;
	pxModule->pxCurrentConfig = NULL;

	/* Return the mutex */
	xSemaphoreGive( pxModule->xBusMutexHandle );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CTransmit( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout )
{
	nrfx_err_t eResult;
	bool	   bTransferring = true;

	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	eResult = nrfx_twim_tx( &pxModule->xPlatform.xInstance,
							pxModule->pxCurrentConfig->ucAddress >> 1,
							pvBuffer,
							usLength,
							true );

	while ( bTransferring ) {
		if ( xTaskGetTickCount() - xEntryTime >= xTimeout ) {
			eResult = NRFX_ERROR_TIMEOUT;
			break;
		}
		bTransferring = nrfx_twim_is_busy( &pxModule->xPlatform.xInstance );
	}

	return eGetErrorMessage( eResult );
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CReceive( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout )
{
	nrfx_err_t eResult;
	bool	   bTransferring = true;

	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	eResult = nrfx_twim_rx( &pxModule->xPlatform.xInstance,
							pxModule->pxCurrentConfig->ucAddress >> 1,
							pvBuffer,
							usLength );

	while ( bTransferring ) {
		if ( xTaskGetTickCount() - xEntryTime > xTimeout ) {
			eResult = NRFX_ERROR_TIMEOUT;
			break;
		}
		bTransferring = nrfx_twim_is_busy( &pxModule->xPlatform.xInstance );
	}

	return eGetErrorMessage( eResult );
}

/*-----------------------------------------------------------*/

eModuleError_t eI2CTransfer( xI2CModule_t *pxModule, void *pvSendBuffer, uint16_t usSendLength, void *pvReceiveBuffer, uint16_t usReceiveLength, TickType_t xTimeout )
{
	nrfx_err_t eResult;
	bool	   bTransferring = true;

	configASSERT( pxModule->bBusClaimed == true );

	TickType_t xEntryTime = xTaskGetTickCount();

	eResult = nrfx_twim_tx( &pxModule->xPlatform.xInstance,
							pxModule->pxCurrentConfig->ucAddress >> 1,
							pvSendBuffer,
							usSendLength,
							true );

	while ( bTransferring ) {
		if ( xTaskGetTickCount() - xEntryTime >= xTimeout ) {
			eResult = NRFX_ERROR_TIMEOUT;
			break;
		}
		bTransferring = nrfx_twim_is_busy( &pxModule->xPlatform.xInstance );
	}

	if ( eResult != NRFX_SUCCESS ) {
		return eGetErrorMessage( eResult );
	}

	eResult = nrfx_twim_rx( &pxModule->xPlatform.xInstance,
							pxModule->pxCurrentConfig->ucAddress >> 1,
							pvReceiveBuffer,
							usReceiveLength );

	while ( bTransferring ) {
		if ( xTaskGetTickCount() - xEntryTime > xTimeout ) {
			eResult = NRFX_ERROR_TIMEOUT;
			break;
		}
		bTransferring = nrfx_twim_is_busy( &pxModule->xPlatform.xInstance );
	}

	return eGetErrorMessage( eResult );
}

/*-----------------------------------------------------------*/

static nrf_twim_frequency_t xGetOptimumFrequency( uint32_t ulFrequency )
{
	/**
	 * If you are getting stuck with this assert, it is because the nRF52 can
	 * only run I2C at 100kHz, 250kHz, and 400kHz. Apparently nothing else.
	 * Your options are:
	 * 	1: Get a better sensor / find a way to make it run faster.
	 * 	2: Use a different microcontroller which can support slower I2C rates. (EFR32)
	 * 	3: Bit bang an I2C implimentation.
	 **/
	configASSERT( ulFrequency >= 100000 );

	if ( ulFrequency >= 400000 ) {
		return NRF_TWIM_FREQ_400K;
	}
	else if ( ( ulFrequency < 400000 ) && ( ulFrequency >= 250000 ) ) {
		return NRF_TWIM_FREQ_250K;
	}
	else {
		return NRF_TWIM_FREQ_100K;
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t eGetErrorMessage( nrfx_err_t eError )
{
	switch ( eError ) {

		case NRFX_SUCCESS:
			return ERROR_NONE;

		case NRFX_ERROR_DRV_TWI_ERR_ANACK:
			return ERROR_NO_ACKNOWLEDGEMENT;

		case NRFX_ERROR_TIMEOUT:
			return ERROR_TIMEOUT;

		default:
			return ERROR_GENERIC;
	}
}

/*-----------------------------------------------------------*/

/**
 * Errata 89:
 * TWI: Static 400 µA current while using GPIOTE.
 * See: https://infocenter.nordicsemi.com/pdf/nRF52832_Rev_1_Errata_v1.6.pdf
 **/
void vErrata89Workaround( xI2CModule_t *pxModule )
{
	if ( pxModule->xPlatform.xInstance.drv_inst_idx == 0 ) {
		*(volatile uint32_t *) 0x40003FFC = 0;
		*(volatile uint32_t *) 0x40003FFC;
		*(volatile uint32_t *) 0x40003FFC = 1;
	}
	else if ( pxModule->xPlatform.xInstance.drv_inst_idx == 1 ) {
		*(volatile uint32_t *) 0x40004FFC = 0;
		*(volatile uint32_t *) 0x40004FFC;
		*(volatile uint32_t *) 0x40004FFC = 1;
	}
}

/*-----------------------------------------------------------*/
