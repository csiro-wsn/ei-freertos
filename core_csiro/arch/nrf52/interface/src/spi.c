/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "spi.h"
#include "spi_arch.h"

#include "nrf_spim.h"
#include "nrfx_common.h"
#include "nrfx_spim.h"

#include "gpio.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static void prvSpiTransfer( xSpiModule_t *pxSpi, void *pvTxBuffer, uint32_t ulTxLength, void *pvRxBuffer, uint32_t ulRxLength );

static nrf_spim_mode_t		xClockModeConversion( eSpiClockMode_t eClockMode );
static nrf_spim_frequency_t xGetOptimumFrequency( uint32_t ulFrequency );
static nrf_spim_bit_order_t xMostSignifigantBitFirst( uint8_t bMsbFirst );

/* Private Variables ----------------------------------------*/

#ifdef NRF52840_XXAA
/** 
 * NRF52840 Errata 198 
 * 	Data accessed by CPU location in the same RAM block as where the SPIM3 TXD.PTR is pointing, 
 *  and CPU does a read or write operation at the same clock cycle as the SPIM3 EasyDMA is fetching data.
 * 
 * As we can't control where the data lives before the SPI functions are called,
 * we declare a buffer that takes a complete RAM block in a special section which is
 * placed at the end of RAM. If SPIM3 is used, TX data is copied and sent from this buffer
 * rather than the provided array.
 * 
 * To be safe we declare this buffer to be the size of a RAM block
*/
uint8_t pucSPIM3WorkaroundTxBuffer[8 * 1024] ATTR_SECTION( ".errata" );
#endif /* NRF52840_XXAA */

/*-----------------------------------------------------------*/

eModuleError_t eSpiInit( xSpiModule_t *pxSpi )
{
	ASSERT_GPIO_ASSIGNED( pxSpi->xPlatform.xMiso );
	ASSERT_GPIO_ASSIGNED( pxSpi->xPlatform.xMosi );
	ASSERT_GPIO_ASSIGNED( pxSpi->xPlatform.xSclk );

	const IRQn_Type xIRQn = nrfx_get_irq_number( pxSpi->xPlatform.xInstance.p_reg );

	pxSpi->xBusMutexHandle		  = xSemaphoreCreateRecursiveMutexStatic( &( pxSpi->xBusMutexStorage ) );
	pxSpi->xTransactionDoneHandle = xSemaphoreCreateBinaryStatic( &( pxSpi->xTransactionDoneStorage ) );

	/* Set priority of SPI interrupt */
	vInterruptSetPriority( xIRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSpiBusStart( xSpiModule_t *pxSpi, const xSpiConfig_t *pxConfig, TickType_t xTimeout )
{
	/* Take a semaphore so nobody else can start this SPI bus */
	if ( xSemaphoreTakeRecursive( pxSpi->xBusMutexHandle, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}

	pxSpi->bBusClaimed	 = true;
	pxSpi->bCsAsserted	 = false;
	pxSpi->pxCurrentConfig = pxConfig;

	/* Shorter names to make things more readable. */
	const xSpiConfig_t *pxCurrentConfig = pxSpi->pxCurrentConfig;
	xSpiPlatform_t *	pxPlatform		= &pxSpi->xPlatform;
	NRF_SPIM_Type *		pxControlBlock  = pxPlatform->xInstance.p_reg;
	const IRQn_Type		xIRQn			= nrfx_get_irq_number( pxControlBlock );

	/* Setup SCLK Pin */
	uint8_t ucDefaultSCLK = xClockModeConversion( pxCurrentConfig->eClockMode ) <= NRF_SPIM_MODE_1 ? GPIO_PUSHPULL_LOW : GPIO_PUSHPULL_HIGH;
	vGpioSetup( pxPlatform->xSclk, GPIO_PUSHPULL, ucDefaultSCLK );
	/* Setup other pins */
	vGpioSetup( pxPlatform->xMosi, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( pxPlatform->xMiso, GPIO_INPUT, GPIO_INPUT_NOFILTER );

	/* Connect all pins to the module */
	nrf_spim_pins_set( pxControlBlock, pxPlatform->xSclk.ucPin, pxPlatform->xMosi.ucPin, pxPlatform->xMiso.ucPin );

	/* Set up SPI Module configuration settings */
	nrf_spim_frequency_set( pxControlBlock, xGetOptimumFrequency( pxCurrentConfig->ulMaxBitrate ) );
	nrf_spim_configure( pxControlBlock, xClockModeConversion( pxCurrentConfig->eClockMode ), xMostSignifigantBitFirst( pxCurrentConfig->ucMsbFirst ) );
	nrf_spim_orc_set( pxControlBlock, pxCurrentConfig->ucDummyTx );

	/* Trigger an interrupt at the end of a transmission */
	nrf_spim_int_enable( pxControlBlock, NRF_SPIM_INT_END_MASK );

	/* Enable global interrupt handler */
	NVIC_ClearPendingIRQ( xIRQn );
	NVIC_EnableIRQ( xIRQn );

	/* Enable the module */
	nrf_spim_enable( pxControlBlock );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vSpiBusEnd( xSpiModule_t *pxSpi )
{
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( !pxSpi->bCsAsserted );

	/* Shorter names to make things more readable. */
	xSpiPlatform_t *pxPlatform	 = &pxSpi->xPlatform;
	NRF_SPIM_Type * pxControlBlock = pxSpi->xPlatform.xInstance.p_reg;

	pxSpi->pxCurrentConfig = NULL;
	pxSpi->bBusClaimed	 = false;

	/* Disable all module interrupts, and finally disable the module */
	nrf_spim_int_disable( pxControlBlock, NRF_SPIM_ALL_INTS_MASK );

	nrf_spim_disable( pxControlBlock );

/** 
 * NRF52840 Errata 195
 * 	SPIM3 continues to draw current after disable.
 * 	Current consumption around 900 μA higher than specified.
 */
#ifdef NRF52840_XXAA
	if ( pxSpi->xPlatform.xInstance.p_reg == NRF_SPIM3 ) {
		*(volatile uint32_t *) 0x4002F004 = 1;
	}
#endif /* NRF52840_XXAA */

	/**
	 * Reset the pins to default to save power.
	 * Also allows other applications to take control of the pins when not in
	 * use for SPI.
	 **/
	vGpioSetup( pxPlatform->xMiso, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( pxPlatform->xMosi, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( pxPlatform->xSclk, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );

	/* Return the SPI bus semaphore */
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
	vGpioSetup( pxSpi->pxCurrentConfig->xCsGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	pxSpi->bCsAsserted = true;
}
/*-----------------------------------------------------------*/

void vSpiCsRelease( xSpiModule_t *pxSpi )
{
	configASSERT( pxSpi->bBusClaimed );
	vGpioSetup( pxSpi->pxCurrentConfig->xCsGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	pxSpi->bCsAsserted = false;
}

/*-----------------------------------------------------------*/

void vSpiTransmit( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen )
{
	configASSERT( ulBufferLen != 0 );
	configASSERT( nrfx_is_in_ram( pvBuffer ) );

	prvSpiTransfer( pxSpi, pvBuffer, ulBufferLen, NULL, 0 );
}

/*-----------------------------------------------------------*/

void vSpiReceive( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen )
{
	configASSERT( ulBufferLen != 0 );
	configASSERT( nrfx_is_in_ram( pvBuffer ) );

	prvSpiTransfer( pxSpi, NULL, 0, pvBuffer, ulBufferLen );
}

/*-----------------------------------------------------------*/

void vSpiTransfer( xSpiModule_t *pxSpi, void *pvTxBuffer, void *pvRxBuffer, uint32_t ulBufferLen )
{
	configASSERT( ulBufferLen != 0 );
	configASSERT( nrfx_is_in_ram( pvTxBuffer ) );
	configASSERT( nrfx_is_in_ram( pvRxBuffer ) );

	prvSpiTransfer( pxSpi, pvTxBuffer, ulBufferLen, pvRxBuffer, ulBufferLen );
}

/*-----------------------------------------------------------*/

static void prvSpiTransfer( xSpiModule_t *pxSpi, void *pvTxBuffer, uint32_t ulTxLen, void *pvRxBuffer, uint32_t ulRxLen )
{
	/* Check that bus is in the correct state for a transaction */
	configASSERT( pxSpi->bBusClaimed );
	configASSERT( pxSpi->bCsAsserted );

#ifdef NRF52840_XXAA
	if ( pxSpi->xPlatform.xInstance.p_reg == NRF_SPIM3 ) {
		pvMemcpy( pucSPIM3WorkaroundTxBuffer, pvTxBuffer, ulTxLen );
		pvTxBuffer = pucSPIM3WorkaroundTxBuffer;
	}
#endif /* NRF52840_XXAA */

	/* Set the buffers */
	nrf_spim_tx_buffer_set( pxSpi->xPlatform.xInstance.p_reg, pvTxBuffer, ulTxLen );
	nrf_spim_rx_buffer_set( pxSpi->xPlatform.xInstance.p_reg, pvRxBuffer, ulRxLen );

	/* Clear any past END events, and trigger a start */
	nrf_spim_event_clear( pxSpi->xPlatform.xInstance.p_reg, NRF_SPIM_EVENT_END );
	nrf_spim_task_trigger( pxSpi->xPlatform.xInstance.p_reg, NRF_SPIM_TASK_START );

	/* Wait forever for the transaction to complete */
	if ( xSemaphoreTake( pxSpi->xTransactionDoneHandle, portMAX_DELAY ) != pdPASS ) {
		/* This assertion is used for error checking in CPPUTEST */
		configASSERT( 0 );
	}
}

/*-----------------------------------------------------------*/

static nrf_spim_frequency_t xGetOptimumFrequency( uint32_t ulFrequency )
{
	configASSERT( ulFrequency >= 125000 );

	if ( ulFrequency >= 8000000 ) {
		return SPIM_FREQUENCY_FREQUENCY_M8;
	}
	else if ( ulFrequency >= 4000000 ) {
		return SPIM_FREQUENCY_FREQUENCY_M4;
	}
	else if ( ulFrequency >= 2000000 ) {
		return SPIM_FREQUENCY_FREQUENCY_M2;
	}
	else if ( ulFrequency >= 1000000 ) {
		return SPIM_FREQUENCY_FREQUENCY_M1;
	}
	else if ( ulFrequency >= 500000 ) {
		return SPIM_FREQUENCY_FREQUENCY_K500;
	}
	else if ( ulFrequency >= 250000 ) {
		return SPIM_FREQUENCY_FREQUENCY_K250;
	}
	else {
		return SPIM_FREQUENCY_FREQUENCY_K125;
	}
}

/*-----------------------------------------------------------*/

static nrf_spim_mode_t xClockModeConversion( eSpiClockMode_t eClockMode )
{
	switch ( eClockMode ) {
		case eSpiClockMode0:
			return NRF_SPIM_MODE_0;
		case eSpiClockMode1:
			return NRF_SPIM_MODE_1;
		case eSpiClockMode2:
			return NRF_SPIM_MODE_2;
		case eSpiClockMode3:
			return NRF_SPIM_MODE_3;
		default:
			/* This should never happen */
			configASSERT( 0 );
			return NRF_SPIM_MODE_0;
	}
}

/*-----------------------------------------------------------*/

static nrf_spim_bit_order_t xMostSignifigantBitFirst( uint8_t bMsbFirst )
{
	if ( bMsbFirst ) {
		return NRF_SPIM_BIT_ORDER_MSB_FIRST;
	}
	else {
		return NRF_SPIM_BIT_ORDER_LSB_FIRST;
	}
}

/*-----------------------------------------------------------*/

void vSpiInterruptHandler( xSpiModule_t *pxSpi )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;

	nrf_spim_event_clear( pxSpi->xPlatform.xInstance.p_reg, NRF_SPIM_EVENT_END );

	xSemaphoreGiveFromISR( pxSpi->xTransactionDoneHandle, &xHigherPriorityTaskWoken );

	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
