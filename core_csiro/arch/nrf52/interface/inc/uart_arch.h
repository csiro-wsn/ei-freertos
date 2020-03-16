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
 * Creation_Date: 22/11/2018
 * Author: Luke Swift <Luke.Swift@data61.csiro.au>
 *
 * nrf52 Specific UART Functionality
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_UART_ARCH
#define __CORE_CSIRO_INTERFACE_UART_ARCH

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "queue.h"

#include "gpio.h"
#include "nrfx_ppi.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

#define NRF_BAUDRATE( baud ) ( ( baud * 0x100000000ULL ) / 16000000 + 0x800 ) & 0xfffff000

#define RX_DMA_BUFFER_SIZE 32

#define UART_MODULE_PLATFORM_PREFIX( NAME, NUM_BUFFERS, BUFFER_SIZE )

#define UART_MODULE_PLATFORM_SUFFIX( NAME, IRQ1, IRQ2 )    \
	void vUartInterruptHandler( xUartModule_t *pxModule ); \
	void IRQ1( void ) { vUartInterruptHandler( &UART_MODULE_GET( NAME ) ); }

#define UART_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ) \
	{                                                \
		.pxUart					= HANDLE,            \
		.pxTimer				= NULL,              \
		.ulArchBaudrate			= 0,                 \
		.pucReceived			= { 0 },             \
		.xQueuedTransmits		= NULL,              \
		.xRxActivityChannel		= 0,                 \
		.xFlushChannel			= 0,                 \
		.xTimeoutExpiredChannel = 0,                 \
		.bAlwaysReceiving		= false,             \
		.xRx					= UNUSED_GPIO,       \
		.xTx					= UNUSED_GPIO,       \
		.xRts					= UNUSED_GPIO,       \
		.xCts					= UNUSED_GPIO        \
	}

/* Type Definitions -----------------------------------------*/

typedef struct xPendingTransmit_t
{
	uint8_t *pucBuffer;
	uint32_t ulBufferLen;
} xPendingTransmit_t;

struct _xUartPlatform_t
{
	NRF_UARTE_Type *  pxUart;
	NRF_TIMER_Type *  pxTimer;
	uint32_t		  ulArchBaudrate;
	uint8_t			  pucReceived[RX_DMA_BUFFER_SIZE];
	QueueHandle_t	 xQueuedTransmits;
	nrf_ppi_channel_t xRxActivityChannel;
	nrf_ppi_channel_t xFlushChannel;
	nrf_ppi_channel_t xTimeoutExpiredChannel;
	bool			  bAlwaysReceiving;
	xGpio_t			  xRx;
	xGpio_t			  xTx;
	xGpio_t			  xRts;
	xGpio_t			  xCts;
};

/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_INTERFACE_UART_ARCH */
