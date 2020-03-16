/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: leds.h
 * Creation_Date: 07/07/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * LED Control
 * 
 */
#ifndef __CSIRO_CORE_PLATFORM_LEDS
#define __CSIRO_CORE_PLATFORM_LEDS
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "gpio.h"

/* Module Defines -------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eLEDs_t {
	LEDS_NONE   = 0x00,
	LEDS_RED	= 0x01,
	LEDS_GREEN  = 0x02,
	LEDS_BLUE   = 0x04,
	LEDS_YELLOW = 0x08,
	LEDS_ALL	= 0x0F
} eLEDs_t;

typedef enum eLEDsPolarity_t {
	LED_ACTIVE_LOW,
	LED_ACTIVE_HIGH
} eLEDsPolarity_t;

typedef struct xLEDsConfig_t
{
	eLEDsPolarity_t ePolarity;
	xGpio_t			xRed;
	xGpio_t			xGreen;
	xGpio_t			xBlue;
	xGpio_t			xYellow;
} xLEDConfig_t;

/* Function Declarations ------------------------------------*/

void vLedsInit( xLEDConfig_t *pxConfig );

eLEDs_t eLedsState( void );

void vLedsSet( eLEDs_t eLeds );
void vLedsOn( eLEDs_t eLeds );
void vLedsOff( eLEDs_t eLeds );
void vLedsToggle( eLEDs_t eLeds );

#endif /* __CSIRO_CORE_PLATFORM_LEDS */
