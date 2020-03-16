/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: adc_arch.h
 * Creation_Date: 28/08/2018
 * Author: John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * Platform Specific ADC implementation
 * 
 */

#ifndef __CSIRO_CORE_INTERFACE_ADC_PLATFORM
#define __CSIRO_CORE_INTERFACE_ADC_PLATFORM

/* Includes -------------------------------------------------*/

#include "em_adc.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

#define ADC_MODULE_PLATFORM_PREFIX( NAME )

#define ADC_MODULE_PLATFORM_SUFFIX( NAME, IRQ )

#define ADC_MODULE_PLATFORM_DEFAULT( handle ) \
	{                                         \
		.pxADC = handle,                      \
	}

/* 
 * The only real important thing we need to know about the ADC on this platform
 * is the pointer to the ADC registers. So that's the only thing in this
 * struct. 
 */
struct _xAdcPlatform_t
{
	ADC_TypeDef *pxADC;
};

// Resolution of the sampled voltage.
typedef enum eAdcResolution_t {
	ADC_RESOLUTION_12BIT,
	ADC_RESOLUTION_16BIT
} eAdcResolution_t;

// The reference voltage source for ADC conversion.
typedef enum eAdcReferenceVoltage_t {
	ADC_REFERENCE_VOLTAGE_1V25,
	ADC_REFERENCE_VOLTAGE_2V5,
	ADC_REFERENCE_VOLTAGE_5V,
	ADC_REFERENCE_VOLTAGE_VDD
} eAdcReferenceVoltage_t;

/* Function Declarations ------------------------------------*/

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_INTERFACE_ADC_PLATFORM */
