/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "cpu.h"
#include "gpio.h"
#include "uart.h"

#include "nrfx_gpiote.h"
#include "nrfx_ppi.h"
#include "nrfx_timer.h"
#include "nrfx_uarte.h"

/* Private Defines ------------------------------------------*/

/* If these channels change, the Shortcuts must be manually changed */
#define RX_FLUSH_CC_CHANNEL 0
#define RX_TIMEOUT_CC_CHANNEL 1

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

void vUartInterruptHandler( xUartModule_t *pxModule );

void prvUartInit( xUartModule_t *pxModule );
void prvUartDisable( xUartModule_t *pxModule );

/* Private Variables ----------------------------------------*/
/*-----------------------------------------------------------*/

eModuleError_t eUartInit( xUartModule_t *pxModule, bool bFlowControl )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;
	NRF_TIMER_Type *const  pxTimer	= pxPlatform->pxTimer;
	const IRQn_Type		   xIRQn	  = nrfx_get_irq_number( pxPlatform->pxUart );

	/* Validate hardware pointers */
	configASSERT( pxUart );
	configASSERT( pxTimer );

	/* Initialise our memory pool */
	vMemoryPoolInit( pxModule->pxMemPool );

	pxModule->xTxDone				   = xSemaphoreCreateBinary();
	pxModule->xIncompleteTransmissions = xSemaphoreCreateCounting( pxModule->ucNumTxBuffers, 0 );

	/* Initialise our pending transmits queue */
	pxPlatform->xQueuedTransmits = xQueueCreate( pxModule->ucNumTxBuffers, sizeof( xPendingTransmit_t ) );

	/* Setup the internal baudrate needed */
	pxModule->xPlatform.ulArchBaudrate = NRF_BAUDRATE( pxModule->ulBaud );

	/* Initialise the received character */
	pxModule->xRxStream			   = xStreamBufferCreate( pxModule->xRxStreamLength, 1 );
	pxModule->bInitialised		   = false;
	pxModule->bHardwareFlowControl = bFlowControl;

	/** 
	 * Setting these pins into their default states costs about 3uA in Idle current
	 * TX must be held to stop spurious transmissions on line
	 * RTS must be held to inform receivers we are not listening
	 **/
	vGpioSetup( pxPlatform->xTx, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( pxPlatform->xRx, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	if ( pxModule->bHardwareFlowControl ) {
		if ( !bGpioEqual( pxPlatform->xRts, UNUSED_GPIO ) ) {
			vGpioSetup( pxPlatform->xRts, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
		}

		if ( !bGpioEqual( pxPlatform->xCts, UNUSED_GPIO ) ) {
			vGpioSetup( pxPlatform->xCts, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
		}
	}

	/* Setup timer to generate our two compare events for flushing and timeout */
	nrf_timer_bit_width_set( pxTimer, TIMER_BITMODE_BITMODE_32Bit );
	nrf_timer_frequency_set( pxTimer, 9 ); /* 16MHz / (2^7) = 125,000 Hz */
	nrf_timer_shorts_enable( pxTimer, TIMER_SHORTS_COMPARE1_STOP_Msk | TIMER_SHORTS_COMPARE1_CLEAR_Msk );
	nrf_timer_mode_set( pxTimer, TIMER_MODE_MODE_Timer );

	uint32_t ulBytesPerSecond = pxModule->ulBaud / 8;
	uint32_t ulOneByteTime	= 125000 / ulBytesPerSecond;

	nrf_timer_cc_write( pxTimer, RX_FLUSH_CC_CHANNEL, 2 * ulOneByteTime ); /* Flush Delay   = 2 bytes with no data */
	nrf_timer_cc_write( pxTimer, RX_TIMEOUT_CC_CHANNEL, 62500 );		   /* Timeout Delay = 500ms = 125000 * 0.5 = 62500 */

	/* High accuracy may not be needed, but it is negligible current compared to TX and RX */
	nrfx_gpiote_in_config_t xRxPinConfig = NRFX_GPIOTE_RAW_CONFIG_IN_SENSE_TOGGLE( true );
	nrfx_gpiote_in_init( pxPlatform->xRx.ucPin, &xRxPinConfig, NULL );

	/* RX Activity both starts and clears timer */
	uint32_t ulRxActivityEvent = nrfx_gpiote_in_event_addr_get( pxPlatform->xRx.ucPin );
	uint32_t ulTimerStartTask  = (uint32_t) nrf_timer_task_address_get( pxTimer, NRF_TIMER_TASK_START );
	uint32_t ulTimerClearTask  = (uint32_t) nrf_timer_task_address_get( pxTimer, NRF_TIMER_TASK_CLEAR );
	nrfx_ppi_channel_alloc( &pxPlatform->xRxActivityChannel );
	nrfx_ppi_channel_assign( pxPlatform->xRxActivityChannel, ulRxActivityEvent, ulTimerStartTask );
	nrfx_ppi_channel_fork_assign( pxPlatform->xRxActivityChannel, ulTimerClearTask );

	/* An expiry on timer compare channel 0 stops RX to flush the pending DMA transfer  */
	uint32_t ulFlushChannelEvent = (uint32_t) nrf_timer_event_address_get( pxTimer, NRF_TIMER_EVENT_COMPARE0 );
	uint32_t ulUartRxStopTask	= (uint32_t) nrf_uarte_task_address_get( pxUart, NRF_UARTE_TASK_STOPRX );
	nrfx_ppi_channel_alloc( &pxPlatform->xFlushChannel );
	nrfx_ppi_channel_assign( pxPlatform->xFlushChannel, ulFlushChannelEvent, ulUartRxStopTask );

	/* An expiry on timer compare channel 1 triggers an end to RX */
	uint32_t ulTimeoutChannelEvent = (uint32_t) nrf_timer_event_address_get( pxTimer, NRF_TIMER_EVENT_COMPARE1 );
	nrfx_ppi_channel_alloc( &pxPlatform->xTimeoutExpiredChannel );
	nrfx_ppi_channel_assign( pxPlatform->xTimeoutExpiredChannel, ulTimeoutChannelEvent, ulUartRxStopTask );

	/* Set priority of UART interrupt */
	vInterruptSetPriority( xIRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vUartOn( xUartModule_t *pxModule )
{
	CRITICAL_SECTION_DECLARE;
	CRITICAL_SECTION_START();
	/* Initialise UART if it is not currently enabled */
	if ( !pxModule->xPlatform.pxUart->ENABLE ) {
		prvUartInit( pxModule );
	}
	pxModule->xPlatform.bAlwaysReceiving = true;
	CRITICAL_SECTION_STOP();
}

/*-----------------------------------------------------------*/

void vUartOff( xUartModule_t *pxModule )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;
	NRF_TIMER_Type *const  pxTimer	= pxPlatform->pxTimer;

	CRITICAL_SECTION_DECLARE;
	CRITICAL_SECTION_START();

	const bool bTransmitting   = uxSemaphoreGetCount( pxModule->xIncompleteTransmissions ) > 0;
	const bool bTimeoutRunning = pxUart->EVENTS_RXDRDY && ( !pxTimer->EVENTS_COMPARE[RX_TIMEOUT_CC_CHANNEL] );

	/* Set receive state for RX IRQ */
	pxPlatform->bAlwaysReceiving = false;
	/** 
	 * If UART is currently transmitting or in the middle of receiving, 
	 * it will automatically move to low power mode on completion.
	 * 
	 * If it is not doing either of these things, manually stop it
	 */
	if ( !bTransmitting && !bTimeoutRunning ) {
		/* Deinitialisation of the hardware is handled in the IRQ */
		pxUart->TASKS_STOPRX = 0x01;
	}
	CRITICAL_SECTION_STOP();
}

/*-----------------------------------------------------------*/

/**
 * nrfx_uarte.c is bypassed to gain more fine grained control over the UARTE behaviour
 * 
 * Configures the UARTE peripheral to a state where Transmissions can be started and
 * a receive operation is pending
 **/
void prvUartInit( xUartModule_t *pxModule )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;
	NRF_TIMER_Type *const  pxTimer	= pxPlatform->pxTimer;
	const IRQn_Type		   xIRQn	  = nrfx_get_irq_number( pxPlatform->pxUart );

	pxUart->BAUDRATE = pxModule->xPlatform.ulArchBaudrate;
	/* Setup Flow Control, Parity and Stop bits (latter two are disabled) */
	pxUart->CONFIG = pxModule->bHardwareFlowControl ? 0x01 : 0x00;

	vGpioSetup( pxPlatform->xRx, GPIO_INPUTPULL, GPIO_INPUTPULL_PULLUP );

	/* Setup pin initial states */
	pxUart->PSEL.TXD = pxPlatform->xTx.ucPin;
	pxUart->PSEL.RXD = pxPlatform->xRx.ucPin;
	if ( pxModule->bHardwareFlowControl ) {
		pxUart->PSEL.RTS = pxPlatform->xRts.ucPin;
		pxUart->PSEL.CTS = pxPlatform->xCts.ucPin;
	}

	/* Clear all pending events */
	pxUart->EVENTS_ENDRX  = 0x00;
	pxUart->EVENTS_ENDTX  = 0x00;
	pxUart->EVENTS_ERROR  = 0x00;
	pxUart->EVENTS_RXTO   = 0x00;
	pxUart->EVENTS_RXDRDY = 0x00;

	/* Clear previous timer events */
	pxTimer->EVENTS_COMPARE[RX_FLUSH_CC_CHANNEL]   = 0x00;
	pxTimer->EVENTS_COMPARE[RX_TIMEOUT_CC_CHANNEL] = 0x00;

	/* Enable interrupt sources */
	pxUart->INTEN = NRF_UARTE_INT_ENDRX_MASK | NRF_UARTE_INT_ENDTX_MASK |
					NRF_UARTE_INT_ERROR_MASK | NRF_UARTE_INT_RXTO_MASK;

	/* Enable global interrupt handler */
	NVIC_ClearPendingIRQ( xIRQn );
	NVIC_EnableIRQ( xIRQn );

	/* Enable the module */
	pxUart->ENABLE		   = UARTE_ENABLE_ENABLE_Enabled;
	pxModule->bInitialised = true;

	/* Trigger STOPTX so that EVENTS_TXSTOPPED is set */
	pxUart->TASKS_STOPTX = 0x01;

	/* Start receiver */
	nrfx_gpiote_in_event_enable( pxPlatform->xRx.ucPin, false );
	nrfx_ppi_channel_enable( pxPlatform->xRxActivityChannel );
	nrfx_ppi_channel_enable( pxPlatform->xFlushChannel );
	nrfx_ppi_channel_enable( pxPlatform->xTimeoutExpiredChannel );
	pxUart->RXD.PTR		  = (uint32_t) pxPlatform->pucReceived;
	pxUart->RXD.MAXCNT	= RX_DMA_BUFFER_SIZE;
	pxUart->TASKS_STARTRX = 0x01;
}

/*-----------------------------------------------------------*/

void vUartQueueBuffer( xUartModule_t *pxModule, int8_t *pcBuffer, uint32_t ulBufferLen )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;

	configASSERT( nrfx_is_in_ram( pcBuffer ) );

	const xPendingTransmit_t xTransmit = {
		.pucBuffer   = (uint8_t *) pcBuffer,
		.ulBufferLen = ulBufferLen
	};

	CRITICAL_SECTION_DECLARE;
	/* Critical section so that a second thread doesn't start another initialisation sequence halfway through */
	CRITICAL_SECTION_START();
	if ( uxSemaphoreGetCount( pxModule->xIncompleteTransmissions ) == 0 ) {
		/* Enable the Hardware if not currently on */
		if ( !pxUart->ENABLE ) {
			prvUartInit( pxModule );
		}
		/* There are no buffers currently TX'ing, which means we must start it ourselves */
		pxUart->TXD.PTR		  = (uint32_t) pcBuffer;
		pxUart->TXD.MAXCNT	= ulBufferLen;
		pxUart->TASKS_STARTTX = 0x01;
	}
	else {
		/* Buffer already TX'ing, add it to the queue for the interrupt handler to start */
		xQueueSendToBack( pxPlatform->xQueuedTransmits, &xTransmit, portMAX_DELAY );
	}
	/* Increment the number of buffers that have not yet been completed */
	xSemaphoreGive( pxModule->xIncompleteTransmissions );
	CRITICAL_SECTION_STOP();
}

/*-----------------------------------------------------------*/

void vUartInterruptHandler( xUartModule_t *pxModule )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;
	NRF_TIMER_Type *const  pxTimer	= pxPlatform->pxTimer;
	xPendingTransmit_t	 xTransmit;
	BaseType_t			   xHigherPriorityTaskWoken = pdFALSE;
	int8_t *			   pucPtr;

	/**
	 * 		We want to continue receiving data under the following conditions
	 * 
	 * 		We are always receiving (via vUartOn/vUartOff)
	 * 						OR
	 * 		We are currently transmitting (via EVENTS_TXSTARTED)
	 * 						OR
	 * 		We have received a byte, and the timeout timer channel hasn't expired
	 *
	 **/

	/* There is potentially more data if we have recently received data but our RX timer event hasn't fired yet */
	const bool bAlwaysReceiving   = pxPlatform->bAlwaysReceiving;
	const bool bTransmitting	  = pxUart->EVENTS_TXSTARTED;
	const bool bTimeoutRunning	= pxUart->EVENTS_RXDRDY && ( !pxTimer->EVENTS_COMPARE[RX_TIMEOUT_CC_CHANNEL] );
	const bool bContinueReceiving = bAlwaysReceiving || bTransmitting || bTimeoutRunning;

	if ( pxUart->EVENTS_ERROR ) {
		/* Undefined error has occured, what do? */
		pxUart->EVENTS_ERROR = 0x00;
		uint8_t ucError		 = pxUart->ERRORSRC;
		configASSERT( ucError == 0 );
	}
	if ( pxUart->EVENTS_ENDRX ) {
		/* Receive buffer has filled up or RX lines has gone idle */
		pxUart->EVENTS_ENDRX = 0x00;
		/* Push recevied data out onto stream */
		if ( pxUart->RXD.AMOUNT > 0 ) {
			xStreamBufferSendFromISR( pxModule->xRxStream, (uint8_t *) pxUart->RXD.PTR, pxUart->RXD.AMOUNT, &xHigherPriorityTaskWoken );
		}
		if ( bContinueReceiving ) {
			pxUart->TASKS_STARTRX = 0x01;
		}
	}
	if ( pxUart->EVENTS_RXTO ) {
		/* Generated when RX is stopped via TASKS_STOPRX */
		pxUart->EVENTS_RXTO = 0x00;
		/* TXing is part of the continue receiving condition, so we won't accidently cut off TX here */
		if ( !bContinueReceiving ) {
			pxUart->EVENTS_RXSTARTED = 0x00;
			prvUartDisable( pxModule );
		}
		else {
			pxUart->TASKS_STARTRX = 0x01;
		}
	}
	if ( pxUart->EVENTS_ENDTX ) {
		/* Transmission has completed */
		pxUart->EVENTS_ENDTX = 0x00;
		pucPtr				 = (int8_t *) pxUart->TXD.PTR;
		/* Check if there are more buffers pending to transmit */
		if ( xQueueReceiveFromISR( pxPlatform->xQueuedTransmits, &xTransmit, &xHigherPriorityTaskWoken ) == pdTRUE ) {
			/* Start the pending transmit */
			pxUart->TXD.PTR		  = (uint32_t) xTransmit.pucBuffer;
			pxUart->TXD.MAXCNT	= xTransmit.ulBufferLen;
			pxUart->TASKS_STARTTX = 0x01;
		}
		else {
			/* Trigger STOPTX to move to lower power state and to set EVENT_TXSTOPPED */
			pxUart->EVENTS_TXSTARTED = 0x00;
			pxUart->TASKS_STOPTX	 = 0x01;
			/* Stop RX if we're not always receiving and timeout isn't running */
			if ( !bAlwaysReceiving && !bTimeoutRunning ) {
				pxUart->TASKS_STOPRX = 0x01;
			}
		}
		/* Return buffer to the available memory pool */
		vMemoryPoolReleaseFromISR( pxModule->pxMemPool, pucPtr, &xHigherPriorityTaskWoken );
		xSemaphoreGiveFromISR( pxModule->xTxDone, &xHigherPriorityTaskWoken );
		xSemaphoreTakeFromISR( pxModule->xIncompleteTransmissions, &xHigherPriorityTaskWoken );
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

void prvUartDisable( xUartModule_t *pxModule )
{
	xUartPlatform_t *const pxPlatform = &pxModule->xPlatform;
	NRF_UARTE_Type *const  pxUart	 = pxPlatform->pxUart;
	const IRQn_Type		   xIRQn	  = nrfx_get_irq_number( pxUart );

	/* Disable PPI channels */
	nrfx_gpiote_in_event_disable( pxPlatform->xRx.ucPin );
	nrfx_ppi_channel_disable( pxPlatform->xRxActivityChannel );
	nrfx_ppi_channel_disable( pxPlatform->xFlushChannel );
	nrfx_ppi_channel_disable( pxPlatform->xTimeoutExpiredChannel );

	/* Setup pin initial states */
	vGpioSetup( pxPlatform->xTx, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( pxPlatform->xRx, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	pxUart->PSEL.TXD = NRF_UARTE_PSEL_DISCONNECTED;
	pxUart->PSEL.RXD = NRF_UARTE_PSEL_DISCONNECTED;
	if ( pxModule->bHardwareFlowControl ) {
		if ( !bGpioEqual( pxPlatform->xRts, UNUSED_GPIO ) ) {
			vGpioSetup( pxPlatform->xRts, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
		}

		if ( !bGpioEqual( pxPlatform->xCts, UNUSED_GPIO ) ) {
			vGpioSetup( pxPlatform->xCts, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
		}

		pxUart->PSEL.RTS = NRF_UARTE_PSEL_DISCONNECTED;
		pxUart->PSEL.CTS = NRF_UARTE_PSEL_DISCONNECTED;
	}

	/* RX and TX have already been stopped */
	pxUart->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
	/* Disable all interrupts */
	NVIC_DisableIRQ( xIRQn );
	NVIC_ClearPendingIRQ( xIRQn );
	pxUart->INTEN		   = 0x00;
	pxModule->bInitialised = false;
}

/*-----------------------------------------------------------*/
