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
 * Filename: adc.h
 * Creation_Date: 28/8/2018
 * Author: John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * Wrapper around ADC functionality.
 * 
 * ADC's are hard. The ADC on the EFR32 for example has tonnes of features.
 * From scan modes, to differential sampling, high frequency sampling, and
 * recalibration algorithms. Rather than wade through the em_adc functions
 * every time anyone wants to implement an ADC, this file serves to create a
 * layer above the "em_adc" file, which people can use if they want a simple
 * fast, non-differential, ADC.
 */

#ifndef __CORE_CSIRO_INTERFACE_ADC
#define __CORE_CSIRO_INTERFACE_ADC

/* Includes -----------------------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "gpio.h"

/* Forward Declarations -------------------------------------*/

typedef struct _xAdcModule_t   xAdcModule_t;
typedef struct _xAdcPlatform_t xAdcPlatform_t;

#include "adc_arch.h"

/* Module Defines -----------------------------------------------------------*/
// clang-format off

#define ADC_MODULE_CREATE( NAME, HANDLE, IRQ )                      \
    ADC_MODULE_PLATFORM_PREFIX( NAME );                             \
    static xAdcModule_t xModule_##NAME = {                          \
        .xPlatform = ADC_MODULE_PLATFORM_DEFAULT( HANDLE ),         \
        .xModuleAvailableHandle     = NULL,                         \
		.xModuleAvailableStorage    = { { 0 } },                    \
    };                                                              \
    ADC_MODULE_PLATFORM_SUFFIX( NAME, IRQ );                        \

#define ADC_MODULE_GET( NAME )                                      \
    xModule_##NAME

// clang-format on
/* Type Definitions ---------------------------------------------------------*/

struct _xAdcModule_t
{
	xAdcPlatform_t	xPlatform;
	SemaphoreHandle_t xModuleAvailableHandle;
	StaticSemaphore_t xModuleAvailableStorage;
};

/* Function Declarations ----------------------------------------------------*/

void vAdcInit( xAdcModule_t *xAdcModule );

uint32_t ulAdcSample( xAdcModule_t *xAdcModule, xGpio_t xGpio, eAdcResolution_t eResolution, eAdcReferenceVoltage_t eReferenceVoltage );

eModuleError_t eAdcRecalibrate( xAdcModule_t *xAdcModule );

/*---------------------------------------------------------------------------*/

#endif /* __CORE_CSIRO_INTERFACE_ADC */
