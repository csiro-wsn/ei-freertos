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
 * Filename: spi_arch.h
 * Creation_Date: 2/6/2018
 * Author: AddMe <add.me@data61.csiro.au>
 *
 * EFR Specific SPI Functionality
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_SPI_ARCH
#define __CORE_CSIRO_INTERFACE_SPI_ARCH

/* Includes -------------------------------------------------*/

#include "em_usart.h"

#include "spidrv.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define SPIDRV_CALLBACK_DECLARATION( NAME ) \
	void NAME##DoneCallback( struct SPIDRV_HandleData *handle, Ecode_t transferStatus, int itemsTransferred )

#define SPIDRV_CALLBACK_BUILDER( NAME )                                                                    \
	SPIDRV_CALLBACK_DECLARATION( NAME )                                                                    \
	{                                                                                                      \
		UNUSED( handle );                                                                                  \
		UNUSED( transferStatus );                                                                          \
		UNUSED( itemsTransferred );                                                                        \
		static BaseType_t xHigherPriorityTaskWoken = pdFALSE;                                              \
		xSemaphoreGiveFromISR( SPI_MODULE_GET( NAME ).xTransactionDoneHandle, &xHigherPriorityTaskWoken ); \
		portYIELD_FROM_ISR( xHigherPriorityTaskWoken );                                                    \
	}

#define SPI_MODULE_PLATFORM_PREFIX( NAME ) \
	SPIDRV_CALLBACK_DECLARATION( NAME );

#define SPI_MODULE_PLATFORM_SUFFIX( NAME, IRQ ) \
	SPIDRV_CALLBACK_BUILDER( NAME );

#define SPI_MODULE_PLATFORM_DEFAULT( NAME, HANDLE )     \
	{                                                   \
		.xInstance				  = HANDLE,             \
		.xDrvHandle				  = NULL,               \
		.xDrvStorage			  = { { 0 } },          \
		.xTransactionDoneCallback = NAME##DoneCallback, \
		.ucPortLocationMosi		  = 0,                  \
		.ucPortLocationMiso		  = 0,                  \
		.ucPortLocationSclk		  = 0,                  \
	}

#define SPI_TEST_CREATE( NAME ) \
	static USART_TypeDef NAME;

/* Type Definitions -----------------------------------------*/

struct _xSpiPlatform_t
{
	USART_TypeDef *		xInstance;
	SPIDRV_Handle_t		xDrvHandle;
	SPIDRV_HandleData_t xDrvStorage;
	SPIDRV_Callback_t   xTransactionDoneCallback;
	uint8_t				ucPortLocationMosi;
	uint8_t				ucPortLocationMiso;
	uint8_t				ucPortLocationSclk;
};

/* Function Declarations ------------------------------------*/

/**
 * Check whether the SPI can safely be put into low power mode
 * \param xSpi The SPI module
 * \return True if module can safely transition to sleep mode
 */
bool bSpiCanDeepSleep( xSpiModule_t *pxSpi );

#endif /* __CORE_CSIRO_INTERFACE_SPI_ARCH */
