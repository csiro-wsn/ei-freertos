/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Filename: uart_arch.h
 * Creation_Date: 17/6/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * EFR Specific UART Functionality
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_UART_ARCH
#define __CORE_CSIRO_INTERFACE_UART_ARCH

/* Includes -------------------------------------------------*/

#include "gpio.h"

#include "em_usart.h"
#include "uartdrv.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define RX_DMA_NUM_BUFFERS 3
#define RX_DMA_BUFFER_SIZE 32

#define UART_MODULE_PLATFORM_PREFIX( NAME, NUM_BUFFERS, BUFFER_SIZE ) \
	DEFINE_BUF_QUEUE( NUM_BUFFERS, NAME##_txQueue );                  \
	DEFINE_BUF_QUEUE( 6, NAME##_rxQueue );

#define UART_MODULE_PLATFORM_SUFFIX( NAME, IRQ1, IRQ2 )                        \
	void vUartRxInterruptHandler( xUartModule_t *pxModule );                   \
	void vUartTxInterruptHandler( xUartModule_t *pxModule );                   \
	void IRQ1( void ) { vUartRxInterruptHandler( &UART_MODULE_GET( NAME ) ); } \
	void IRQ2( void ) { vUartTxInterruptHandler( &UART_MODULE_GET( NAME ) ); }

#define UART_MODULE_PLATFORM_DEFAULT( NAME, HANDLE )                             \
	{                                                                            \
		.pxHandle			   = HANDLE,                                         \
		.xDrvHandle			   = NULL,                                           \
		.xDrvStorage		   = { { 0 } },                                      \
		.xTxQueue			   = (UARTDRV_Buffer_FifoQueue_t *) &NAME##_txQueue, \
		.xRxQueue			   = (UARTDRV_Buffer_FifoQueue_t *) &NAME##_rxQueue, \
		.ulArchBaudrate		   = 0,                                              \
		.ppucReceived		   = { { 0 } },                                      \
		.bAlwaysReceiving	  = false,                                          \
		.bRecentlyReceivedByte = false,                                          \
		.ucReceivedIndex	   = 0,                                              \
		.ucTxLocation		   = 0,                                              \
		.ucRxLocation		   = 0,                                              \
		.ucRtsLocation		   = 0,                                              \
		.ucCtsLocation		   = 0,                                              \
		.xRts				   = UNUSED_GPIO,                                    \
		.xCts				   = UNUSED_GPIO,                                    \
	}

/* Type Definitions -----------------------------------------*/

typedef USART_TypeDef xUartHandle_t;

struct _xUartPlatform_t
{
	xUartHandle_t volatile *const pxHandle;
	UARTDRV_Handle_t			  xDrvHandle;
	UARTDRV_HandleData_t		  xDrvStorage;
	UARTDRV_Buffer_FifoQueue_t *  xTxQueue;
	UARTDRV_Buffer_FifoQueue_t *  xRxQueue;
	uint32_t					  ulArchBaudrate;
	uint8_t						  ppucReceived[RX_DMA_NUM_BUFFERS][RX_DMA_BUFFER_SIZE];
	bool						  bAlwaysReceiving;
	bool						  bRecentlyReceivedByte;
	uint8_t						  ucReceivedIndex;
	uint8_t						  ucTxLocation;
	uint8_t						  ucRxLocation;
	uint8_t						  ucRtsLocation;
	uint8_t						  ucCtsLocation;
	xGpio_t						  xRts;
	xGpio_t						  xCts;
};

/* Function Declarations ------------------------------------*/

/**
 * Check whether the UART can safely be put into low power mode
 * \param xUart The UART module
 * \return True if module can safely transition to sleep mode
 */
bool bUartCanDeepSleep( xUartModule_t *pxUart );

/**
 * Deinitialises the UART Module
 * \param xUart The UART module
 */
void vUartDeepSleep( xUartModule_t *pxUart );

#endif /* __CORE_CSIRO_INTERFACE_UART_ARCH */
