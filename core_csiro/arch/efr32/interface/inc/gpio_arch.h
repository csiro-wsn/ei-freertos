/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: gpio_arch.h
 * Creation_Date: 02/08/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Platform Specific GPIO implementation
 * 
 */
#ifndef __CSIRO_CORE_INTERFACE_GPIO_ARCH
#define __CSIRO_CORE_INTERFACE_GPIO_ARCH
/* Includes -------------------------------------------------*/

#include "em_gpio.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define UNUSED_GPIO_ARCH		( xGpio_t ) { ( GPIO_Port_TypeDef ) 0xFF, 0xFF }
#define UNUSED_LOCATION			UINT32_MAX

#define ASSERT_GPIO_ASSIGNED_ARCH( xGpio )			\
	configASSERT( xGpio.ucPin != UNUSED_GPIO.ucPin );	\
	configASSERT( xGpio.ePort != UNUSED_GPIO.ePort )

#define ASSERT_LOCATION_ASSIGNED( ulLocation )			\
	configASSERT( ulLocation != UNUSED_LOCATION )

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xGpio_t
{
	GPIO_Port_TypeDef ePort;
	uint8_t			  ucPin;
} xGpio_t;

/* Function Declarations ------------------------------------*/

uint8_t ucFindInterruptLine( xGpio_t xGpio );

static inline bool bGpioEqual( xGpio_t xGpioA, xGpio_t xGpioB )
{
	return ( xGpioA.ePort == xGpioB.ePort ) && ( xGpioA.ucPin == xGpioB.ucPin );
}

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_INTERFACE_GPIO_ARCH */
