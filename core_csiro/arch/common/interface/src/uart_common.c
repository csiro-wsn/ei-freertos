/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "uart.h"

#include "cpu.h"
#include "tiny_printf.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Functions expected to be implemented in SoC specific implementation */
void vUartQueueBuffer( xUartModule_t *pxModule, int8_t *pcBuffer, uint32_t ulBufferLen );

eModuleError_t eUartWrite( void *pvContext, const char *pcFormat, va_list pvArgs );
char *		   pcUartClaimBuffer( void *pvContext, uint32_t *pulBufferLen );
void		   vUartSendBuffer( void *pvContext, const char *pcBuffer, uint32_t ulBufferLen );
void		   vUartReleaseBuffer( void *pvContext, char *pucBuffer );

/* Private Variables ----------------------------------------*/

xSerialBackend_t xUartBackend = {
	.fnEnable		 = (fnSerialEnable_t) vUartOn,
	.fnDisable		 = (fnSerialDisable_t) vUartOff,
	.fnWrite		 = eUartWrite,
	.fnClaimBuffer   = pcUartClaimBuffer,
	.fnSendBuffer	= (fnSerialSendBuffer_t) vUartQueueBuffer,
	.fnReleaseBuffer = vUartReleaseBuffer
};

/*-----------------------------------------------------------*/

eModuleError_t eUartWrite( void *pvContext, const char *pcFormat, va_list pvArgs )
{
	xUartModule_t *const pxUart = (xUartModule_t *) pvContext;
	char *				 pcOutputBuffer;
	uint32_t			 ulBufferSize;
	uint32_t			 ulNumBytes = 0;
	/* Take a buffer from the memory pool */
	pcOutputBuffer = (char *) pcUartClaimBuffer( pxUart, &ulBufferSize );
	/* Check if buffer claim failed*/
	if ( pcOutputBuffer == NULL ) {
		return ERROR_TIMEOUT;
	}
	/* Push string into claimed buffer */
	ulNumBytes = tiny_vsnprintf( pcOutputBuffer, ulBufferSize, pcFormat, pvArgs );
	/* Queue the buffer onto the UART Hardware */
	vUartQueueBuffer( pxUart, (int8_t *) pcOutputBuffer, ulNumBytes );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

char *pcUartClaimBuffer( void *pvContext, uint32_t *pulBufferLen )
{
	xUartModule_t *pxUart = (xUartModule_t *) pvContext;
	*pulBufferLen		  = pxUart->pxMemPool->ulBufferSize;
	return (char *) pcMemoryPoolClaim( pxUart->pxMemPool, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

void vUartReleaseBuffer( void *pvContext, char *pucBuffer )
{
	xUartModule_t *pxUart = (xUartModule_t *) pvContext;
	vMemoryPoolRelease( pxUart->pxMemPool, (int8_t *) pucBuffer );
}

/*-----------------------------------------------------------*/

ATTR_NORETURN void vSerialReceiveTask( void *pvParameters )
{
	char				  pcBuffer[32];
	size_t				  xReceived, xIndex;
	xSerialReceiveArgs_t *pxArgs = (xSerialReceiveArgs_t *) pvParameters;

	xUartModule_t *		 pxUart				= pxArgs->pxUart;
	fnSerialByteHandler_t xSerialByteHandler = pxArgs->fnHandler;

	for ( ;; ) {
		xReceived = xStreamBufferReceive( pxUart->xRxStream, pcBuffer, 32, portMAX_DELAY );
		if ( xSerialByteHandler != NULL ) {
			for ( xIndex = 0; xIndex < xReceived; xIndex++ ) {
				xSerialByteHandler( pcBuffer[xIndex] );
			}
		}
	}
}

/*-----------------------------------------------------------*/
