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
 * Creation_Date: 7/03/2019
 * Author: John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * nrf52 Specific SPI Functionality
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_SPI_ARCH
#define __CORE_CSIRO_INTERFACE_SPI_ARCH

/* Includes -------------------------------------------------*/

#include "leds.h"

#include "nrf_spim.h"
#include "nrfx_spim.h"

#include "gpio.h"
#include "gpio_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define SPI_MODULE_PLATFORM_PREFIX( NAME )

#define SPI_MODULE_PLATFORM_SUFFIX( NAME, IRQ )              \
	void IRQ( void )                                         \
	{                                                        \
		void vSpiInterruptHandler( xSpiModule_t *pxModule ); \
		vSpiInterruptHandler( &SPI_MODULE_GET( NAME ) );     \
	}

#define SPI_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ) \
	{                                               \
		.xInstance = NRFX_SPIM_INSTANCE( HANDLE ),  \
		.xMosi	 = UNUSED_GPIO,                   \
		.xMiso	 = UNUSED_GPIO,                   \
		.xSclk	 = UNUSED_GPIO,                   \
	}

/* Type Definitions -----------------------------------------*/

struct _xSpiPlatform_t
{
	nrfx_spim_evt_handler_t xTransactionDoneCallback;
	nrfx_spim_t				xInstance;
	xGpio_t					xMosi;
	xGpio_t					xMiso;
	xGpio_t					xSclk;
};

/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_INTERFACE_SPI_ARCH */
