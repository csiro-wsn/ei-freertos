/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: gpio_arch.h
 * Creation_Date: 22/11/2018
 * Author: Luke Swift <Luke.Swift@data61.csiro.au>
 *
 * Platform Specific GPIO implementation
 * 
 */

#ifndef __CSIRO_CORE_INTERFACE_GPIO_ARCH
#define __CSIRO_CORE_INTERFACE_GPIO_ARCH

/* Includes -------------------------------------------------*/

#include "nrf_gpio.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define UNUSED_GPIO_ARCH	( xGpio_t ) { UINT8_MAX }

#define ASSERT_GPIO_ASSIGNED_ARCH( xGpio )			\
	configASSERT( xGpio.ucPin != UNUSED_GPIO.ucPin )

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xGpio_t
{
	uint8_t ucPin;
} xGpio_t;

/* Function Declarations ------------------------------------*/

void vGpioSetInterruptPull( xGpio_t xGpio, uint32_t ulPull );

static inline bool bGpioEqual( xGpio_t xGpioA, xGpio_t xGpioB )
{
	return ( xGpioA.ucPin == xGpioB.ucPin );
}

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_INTERFACE_GPIO_ARCH */
