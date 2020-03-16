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
 * Filename: adc_arch.h
 * Creation_Date: 02/04/2019
 * Author: John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * nrf52 Architecture Specific ADC Implementation
 * 
 */
#ifndef __CSIRO_CORE_INTERFACE_ADC_ARCH
#define __CSIRO_CORE_INTERFACE_ADC_ARCH

/* Includes -------------------------------------------------*/

#include "adc.h"

#include "nrf_saadc.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define ADC_MODULE_PLATFORM_PREFIX( NAME )

#define ADC_MODULE_PLATFORM_SUFFIX( NAME, IRQ )

#define ADC_MODULE_PLATFORM_DEFAULT( handle )                \
	{                                                        \
		.pxAdc									= handle,    \
		.lLastCalibratedTemperatureMilliDegrees = UINT16_MAX \
	}

/* Type Definitions -----------------------------------------*/

struct _xAdcPlatform_t
{
	NRF_SAADC_Type *pxAdc;
	int32_t			lLastCalibratedTemperatureMilliDegrees;
};

/* Availible resolution of the sampled voltage. */
typedef enum eAdcResolution_t {
	ADC_RESOLUTION_8BIT  = NRF_SAADC_RESOLUTION_8BIT,
	ADC_RESOLUTION_10BIT = NRF_SAADC_RESOLUTION_10BIT,
	ADC_RESOLUTION_12BIT = NRF_SAADC_RESOLUTION_12BIT,
	ADC_RESOLUTION_14BIT = NRF_SAADC_RESOLUTION_14BIT
} eAdcResolution_t;

/**
 * Available reference voltages for ADC conversions.
 * It should be noted that there are a few more availible than mentioned here.
 * If you have a niche application, check the datasheet to see if it is
 * supported and add it yourself.
 **/
typedef enum eAdcReferenceVoltage_t {
	ADC_REFERENCE_VOLTAGE_0V6,
	ADC_REFERENCE_VOLTAGE_1V2,
	ADC_REFERENCE_VOLTAGE_1V8,
	ADC_REFERENCE_VOLTAGE_2V4,
	ADC_REFERENCE_VOLTAGE_3V,
	ADC_REFERENCE_VOLTAGE_3V6,
	ADC_REFERENCE_VOLTAGE_VDD
} eAdcReferenceVoltage_t;

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_INTERFACE_ADC_ARCH */
