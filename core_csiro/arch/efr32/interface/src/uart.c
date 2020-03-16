/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "cpu.h"
#include "gpio.h"
#include "tiny_printf.h"
#include "uart.h"

#include "uartdrv.h"

/* Private Defines ------------------------------------------*/
/* Type Definitions -----------------------------------------*/
// clang-format off

/* USARTn TCMP0 will generate an interrupt 16 baud cycles after an RX byte if no other RX byte has started */
#define UART_RECEIVE_PACKET_TIMEOUT_ENABLE 			USART_TIMECMP0_TSTART_RXEOF | USART_TIMECMP0_TSTOP_RXACT | ( 16 & 0xFF )

/* USARTn TCMP1 will generate an interrupt 255 baud cycles after an RX byte if no other RX byte has started */
#define UART_RECEIVE_INTERFACE_TIMEOUT_ENABLE		USART_TIMECMP1_RESTARTEN | USART_TIMECMP1_TSTART_RXEOF | USART_TIMECMP1_TSTOP_RXACT | ( 255 & 0xFF )
#define UART_RECEIVE_INTERFACE_TIMEOUT_DISABLE		USART_TIMECMP1_TSTART_DISABLE
/** 
 * How many TCMP1 interrupts until we will let the interface go back to sleep
 * The time this value corresponds to is dependant on the baudrate the the cycles set in the TCMP1 interrupts
 * 
 * Timout in Seconds = (TIMEOUT_COUNT * BAUD_CYCLES) / BAUDRATE
 * 					 = (500 * 255) / 230400
 * 					 = 0.55 seconds
 **/
#define UART_RECEIVE_INTERFACE_TIMEOUT_COUNT		500

// clang-format on

/* Function Declarations ------------------------------------*/

void prvUartInit( xUartModule_t *pxUart );

void vRxDoneCallback( UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount );
void vTxDoneCallback( UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t eUartInit( xUartModule_t *pxUart, bool bFlowControl )
{
	vInterruptSetPriority( USART0_RX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( USART0_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( USART1_RX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( USART1_TX_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );

	vMemoryPoolInit( pxUart->pxMemPool );

	pxUart->xRxStream	= xStreamBufferCreate( pxUart->xRxStreamLength, 1 );
	pxUart->bInitialised = false;

	pxUart->xTxDone					 = xSemaphoreCreateBinary();
	pxUart->xIncompleteTransmissions = xSemaphoreCreateCounting( pxUart->ucNumTxBuffers, 0 );

	pxUart->xPlatform.ulArchBaudrate = pxUart->ulBaud;

	pxUart->xPlatform.xDrvHandle = &pxUart->xPlatform.xDrvStorage;
	pxUart->bHardwareFlowControl = bFlowControl;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vUartOn( xUartModule_t *pxUart )
{
	CRITICAL_SECTION_DECLARE;
	pxUart->xPlatform.bAlwaysReceiving = true;
	/* Initialise UART if it is not currently initialised to start receiving */
	CRITICAL_SECTION_START();
	if ( !pxUart->bInitialised ) {
		prvUartInit( pxUart );
	}
	CRITICAL_SECTION_STOP();
}

/*-----------------------------------------------------------*/

void vUartOff( xUartModule_t *pxUart )
{
	pxUart->xPlatform.bAlwaysReceiving = false;
	/* No further action needed, UART will be automatically stopped due to vUartDeepSleep() */
}

/*-----------------------------------------------------------*/

void prvUartInit( xUartModule_t *pxUart )
{
	UARTDRV_Init_t xInitData;
	xInitData.port	 = (USART_TypeDef *) pxUart->xPlatform.pxHandle;
	xInitData.baudRate = pxUart->xPlatform.ulArchBaudrate;

	xInitData.portLocationTx = pxUart->xPlatform.ucTxLocation;
	xInitData.portLocationRx = pxUart->xPlatform.ucRxLocation;
	xInitData.txQueue		 = pxUart->xPlatform.xTxQueue;
	xInitData.rxQueue		 = pxUart->xPlatform.xRxQueue;

	if ( pxUart->bHardwareFlowControl ) {
		xInitData.fcType		  = uartdrvFlowControlHwUart;
		xInitData.portLocationRts = pxUart->xPlatform.ucRtsLocation;
		xInitData.portLocationCts = pxUart->xPlatform.ucCtsLocation;
		xInitData.ctsPort		  = pxUart->xPlatform.xCts.ePort;
		xInitData.ctsPin		  = pxUart->xPlatform.xCts.ucPin;
		xInitData.rtsPort		  = pxUart->xPlatform.xRts.ePort;
		xInitData.rtsPin		  = pxUart->xPlatform.xRts.ucPin;
	}
	else {
		xInitData.fcType = uartdrvFlowControlNone;
	}

	xInitData.stopBits	 = usartStopbits1;
	xInitData.parity	   = usartNoParity;
	xInitData.oversampling = usartOVS16;
	xInitData.mvdis		   = false;

	UARTDRV_Init( pxUart->xPlatform.xDrvHandle, &xInitData );
	pxUart->xPlatform.xDrvHandle->context = pxUart;

	/* Setup our receive timeout interrupts */
	xInitData.port->TIMECMP0 = UART_RECEIVE_PACKET_TIMEOUT_ENABLE;
	/* Timer restarts each interrupt so we can create longer delays */
	xInitData.port->TIMECMP1 = UART_RECEIVE_INTERFACE_TIMEOUT_ENABLE;
	/* Enable TCMP0 so we can flush out the DMA transfer if we haven't received a byte in a while */
	/* Enable TCMP1 so we can turn off the UART interface if we haven't received a byte in a longer while */
	/* Enable TXC so we wake up out of EM1 immedietely on UART completion so we can potentially drop to EM2  */
	xInitData.port->IEN |= USART_IEN_TCMP0 | USART_IEN_TCMP1 | USART_IEN_TXC;
	vInterruptEnable( USART0_RX_IRQn );
	vInterruptEnable( USART0_TX_IRQn );

	/**
	 *  Queue up several receives so we don't lose data if the interrupt processing is slow 
	 * 		We queue up one less than the number of buffers available because a new buffer is pushed on
	 * 		the queue before the previous one is taken off due to the order of function calls in UARTDRV
	 **/
	pxUart->xPlatform.ucReceivedIndex = 0;
	for ( uint8_t i = 0; i < RX_DMA_NUM_BUFFERS - 1; i++ ) {
		UARTDRV_Receive( pxUart->xPlatform.xDrvHandle, pxUart->xPlatform.ppucReceived[i], RX_DMA_BUFFER_SIZE, vRxDoneCallback );
	}
	pxUart->bInitialised = true;
}

/*-----------------------------------------------------------*/

void vUartRxInterruptHandler( xUartModule_t *pxModule )
{
	static volatile uint32_t	  ulCountSinceLastByte;
	volatile xUartHandle_t *const pxHandle = pxModule->xPlatform.pxHandle;

	/* If TIMERRESTARTED bit is not set, we've had a new RXEOF event, so restart our counter */
	if ( !( pxHandle->STATUS & USART_STATUS_TIMERRESTARTED ) ) {
		ulCountSinceLastByte = 0;
	}
	/* Our receive packet timeout interrupt has fired */
	if ( pxHandle->IF & USART_IFC_TCMP0 ) {
		UARTDRV_ReceiveTimeout( pxModule->xPlatform.xDrvHandle );
	}
	/* Our receive interface timeout interrupt has fired */
	if ( pxHandle->IF & USART_IFC_TCMP1 ) {
		if ( ++ulCountSinceLastByte > UART_RECEIVE_INTERFACE_TIMEOUT_COUNT ) {
			/* Disable the TCMP module to stop it from restarting */
			pxHandle->TIMECMP1 = UART_RECEIVE_INTERFACE_TIMEOUT_DISABLE;
			/* Tell the interdace we haven't received a byte for a while */
			pxModule->xPlatform.bRecentlyReceivedByte = false;
			/* Renable the TCMP module so it will start running again on the next byte */
			pxHandle->TIMECMP1 = UART_RECEIVE_INTERFACE_TIMEOUT_ENABLE;
		}
	}
	/* Clear all recieved interrupts after handling */
	pxHandle->IFC |= USART_IFC_TCMP0 | USART_IFC_TCMP1;
}

/*-----------------------------------------------------------*/

void vUartTxInterruptHandler( xUartModule_t *pxModule )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	/* Only purpose of this interrupt is to wake the chip up so we can move to EM2 */
	/* Clear all recieved interrupts after handling */
	pxModule->xPlatform.pxHandle->IFC |= USART_IFC_TXC;
	/* Also notifies people the TX has completed */
	xSemaphoreGiveFromISR( pxModule->xTxDone, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

void vRxDoneCallback( UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount )
{
	xUartModule_t *   pxModule				   = (xUartModule_t *) handle->context;
	xUartPlatform_t * xPlatform				   = &pxModule->xPlatform;
	static BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if ( transferStatus != ECODE_EMDRV_UARTDRV_ABORTED ) {
		pxModule->xPlatform.bRecentlyReceivedByte = true;
		xStreamBufferSendFromISR( pxModule->xRxStream, data, transferCount, &xHigherPriorityTaskWoken );
		xPlatform->ucReceivedIndex = ( xPlatform->ucReceivedIndex + 1 ) % RX_DMA_NUM_BUFFERS;
		UARTDRV_Receive( handle, xPlatform->ppucReceived[xPlatform->ucReceivedIndex], RX_DMA_BUFFER_SIZE, vRxDoneCallback );
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

void vTxDoneCallback( UARTDRV_Handle_t handle, Ecode_t transferStatus, uint8_t *data, UARTDRV_Count_t transferCount )
{
	UNUSED( handle );
	UNUSED( transferStatus );
	UNUSED( transferCount );
	xUartModule_t *pxModule					= (xUartModule_t *) handle->context;
	BaseType_t	 xHigherPriorityTaskWoken = pdFALSE;
	vMemoryPoolReleaseFromISR( pxModule->pxMemPool, (int8_t *) data, &xHigherPriorityTaskWoken );
	xSemaphoreTakeFromISR( pxModule->xIncompleteTransmissions, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

bool bUartCanDeepSleep( xUartModule_t *pxUart )
{
	uint32_t ulBytesReceived, ulBytesRemaining;
	uint8_t *pucBuffer;
	/* Query driver status */
	UARTDRV_Status_t xTxStatus = UARTDRV_GetPeripheralStatus( pxUart->xPlatform.xDrvHandle );
	UARTDRV_GetReceiveStatus( pxUart->xPlatform.xDrvHandle, &pucBuffer, &ulBytesReceived, &ulBytesRemaining );
	/** 
	 * We can sleep if the following conditions are met:
	 * 		The Module is not initialised
	 * 					OR
	 * 		We are not transmitting
	 * 		There are no bytes sitting in the DMA transfer buffer
	 * 		We have not recently received a serial byte
	 * 		We are not always receiving
	 * 
	 * Bytes sitting in the DMA transfer buffer will be flushed quickly by the TCMP0 interrupt
	 * Recently received serial bytes is cleared by the TCMP1 interrupt
	 **/
	bool bAlwaysReceiving = pxUart->xPlatform.bAlwaysReceiving;
	bool bNoRecentData	= !pxUart->xPlatform.bRecentlyReceivedByte;
	bool bNoPendingData   = ( ulBytesReceived == 0 );
	bool bTxIdle		  = xTxStatus & UARTDRV_STATUS_TXIDLE;

	return !pxUart->bInitialised || ( bTxIdle && bNoRecentData && bNoPendingData && !bAlwaysReceiving );
}

/*-----------------------------------------------------------*/

void vUartDeepSleep( xUartModule_t *pxUart )
{
	if ( pxUart->bInitialised ) {
		/* Deinitialise the USART Module */
		UARTDRV_DeInit( pxUart->xPlatform.xDrvHandle );
		/* Disable the interrupts we enabled */
		pxUart->xPlatform.pxHandle->IEN &= ~( USART_IEN_TCMP0 | USART_IFC_TCMP1 | USART_IEN_TXC );
		vInterruptDisable( USART0_RX_IRQn );
		vInterruptDisable( USART0_TX_IRQn );
		/* Update our current state */
		pxUart->bInitialised = false;
	}
}

/*-----------------------------------------------------------*/

void vUartQueueBuffer( xUartModule_t *pxUart, int8_t *pcBuffer, uint32_t ulBufferLen )
{
	CRITICAL_SECTION_DECLARE;
	/* Critical section so that a second thread doesn't start another initialisation sequence halfway through */
	CRITICAL_SECTION_START();
	if ( !pxUart->bInitialised ) {
		prvUartInit( pxUart );
	}
	UARTDRV_Transmit( pxUart->xPlatform.xDrvHandle, (uint8_t *) pcBuffer, ulBufferLen, vTxDoneCallback );
	xSemaphoreGive( pxUart->xIncompleteTransmissions );
	CRITICAL_SECTION_STOP();
}

/*-----------------------------------------------------------*/
