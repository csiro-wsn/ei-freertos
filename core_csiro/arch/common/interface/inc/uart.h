/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: uart.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * FreeRTOS abstraction for DMA based UART
 *
 */

#ifndef __LIBS_CSIRO_INTERFACE_UART
#define __LIBS_CSIRO_INTERFACE_UART

/* Includes -------------------------------------------------*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "stream_buffer.h"
#include "task.h"

#include "memory_operations.h"
#include "memory_pool.h"

#include "serial_interface.h"

/* Forward Declarations -------------------------------------*/

typedef struct _xUartModule_t   xUartModule_t;
typedef struct _xUartPlatform_t xUartPlatform_t;
typedef void ( *xSerialByteHandler_t )( char );

#include "uart_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define UART_MODULE_GET( NAME ) xModule_##NAME

#define UART_MODULE_CREATE( NAME, HANDLE, IRQ1, IRQ2, NUM_TX_BUFFERS, TX_BUFFER_SIZE, RX_STREAM_LENGTH ) \
	UART_MODULE_PLATFORM_PREFIX( NAME, NUM_TX_BUFFERS, TX_BUFFER_SIZE );                                 \
	MEMORY_POOL_CREATE( NAME, NUM_TX_BUFFERS, TX_BUFFER_SIZE );                                          \
	COMPILER_WARNING_DISABLE( "-Wcast-qual" )                                                            \
	static xUartModule_t xModule_##NAME = {                                                              \
		.pxMemPool				  = &MEMORY_POOL_GET( NAME ),                                            \
		.xPlatform				  = UART_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ),                        \
		.xRxStream				  = NULL,                                                                \
		.xRxStreamLength		  = RX_STREAM_LENGTH,                                                    \
		.xTxDone				  = NULL,                                                                \
		.xIncompleteTransmissions = NULL,                                                                \
		.ulBaud					  = 0,                                                                   \
		.ucNumTxBuffers			  = NUM_TX_BUFFERS,                                                      \
		.bInitialised			  = false,                                                               \
		.bHardwareFlowControl	 = false,                                                               \
	};                                                                                                   \
	COMPILER_WARNING_ENABLE()                                                                            \
	UART_MODULE_PLATFORM_SUFFIX( NAME, IRQ1, IRQ2 );

/* Type Definitions -----------------------------------------*/

typedef void ( *xSerialByteHandler_t )( char );

struct _xUartModule_t
{
	xMemoryPool_t * pxMemPool;
	xUartPlatform_t xPlatform;
	/* Parameters for FreeRTOS Message Stream */
	StreamBufferHandle_t xRxStream;
	uint8_t				 xRxStreamLength;
	/* Indicators */
	SemaphoreHandle_t xTxDone;
	SemaphoreHandle_t xIncompleteTransmissions;
	/* Interface Parameters */
	uint32_t ulBaud;
	uint8_t  ucNumTxBuffers;
	bool	 bInitialised;
	bool	 bHardwareFlowControl;
};

typedef struct xSerialReceiveArgs_t
{
	xUartModule_t *		 pxUart;
	xSerialByteHandler_t fnHandler;
} xSerialReceiveArgs_t;

/* Function Declarations ------------------------------------*/

/**
 * Setting up FreeRTOS memory for the UART driver
 * Should only be called once on program statup
 * \param xUart The UART module
 */
eModuleError_t eUartInit( xUartModule_t *pxUart, bool bFlowControl );

/**
 * Force UART hardware to always be receiving
 * \param xUart The UART module
 */
void vUartOn( xUartModule_t *pxUart );

/**
 * Stop forced receive on UART hardware
 * \param xUart The UART module
 */
void vUartOff( xUartModule_t *pxUart );

/** 
 * Serial task for the board file
 * \param vParameters is a pointer to the initilised xUartModule_t struct
 */
void vSerialReceiveTask( void *pvParameters );

/* External Variables ------------------------------------*/

extern xSerialBackend_t xUartBackend;

#endif /* __LIBS_CSIRO_INTERFACE_UART */
