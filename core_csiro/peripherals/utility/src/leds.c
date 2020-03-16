/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "leds.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static void prvLedsUpdate( void );

/* Private Variables ----------------------------------------*/

static eLEDs_t		 eEnabledLeds = 0;
static xLEDConfig_t *pxLeds;

/*-----------------------------------------------------------*/

void vLedsInit( xLEDConfig_t *pxConfig )
{
	pxLeds					= pxConfig;
	eEnabledLeds			= LEDS_NONE;
	uint32_t ulDefaultState = ( pxLeds->ePolarity == LED_ACTIVE_HIGH ) ? GPIO_PUSHPULL_LOW : GPIO_PUSHPULL_HIGH;

	if ( !bGpioEqual( pxLeds->xRed, UNUSED_GPIO ) ) {
		vGpioSetup( pxLeds->xRed, GPIO_PUSHPULL, ulDefaultState );
	}
	if ( !bGpioEqual( pxLeds->xGreen, UNUSED_GPIO ) ) {
		vGpioSetup( pxLeds->xGreen, GPIO_PUSHPULL, ulDefaultState );
	}
	if ( !bGpioEqual( pxLeds->xBlue, UNUSED_GPIO ) ) {
		vGpioSetup( pxLeds->xBlue, GPIO_PUSHPULL, ulDefaultState );
	}
	if ( !bGpioEqual( pxLeds->xYellow, UNUSED_GPIO ) ) {
		vGpioSetup( pxLeds->xYellow, GPIO_PUSHPULL, ulDefaultState );
	}
}

/*-----------------------------------------------------------*/

void vLedsSet( eLEDs_t eLeds )
{
	/* Update our state of leds */
	eEnabledLeds = ( LEDS_ALL & eLeds );
	prvLedsUpdate();
}

/*-----------------------------------------------------------*/

void vLedsOn( eLEDs_t eLeds )
{
	/* Update our state of leds */
	eEnabledLeds |= ( LEDS_ALL & eLeds );
	prvLedsUpdate();
}

/*-----------------------------------------------------------*/

void vLedsOff( eLEDs_t eLeds )
{
	/* Update our state of leds */
	eEnabledLeds &= ~( LEDS_ALL & eLeds );
	prvLedsUpdate();
}

/*-----------------------------------------------------------*/

void vLedsToggle( eLEDs_t eLeds )
{
	/* Update our state of leds */
	eEnabledLeds ^= ( LEDS_ALL & eLeds );
	prvLedsUpdate();
}

/*-----------------------------------------------------------*/

eLEDs_t eLedsState( void )
{
	return eEnabledLeds;
}

/*-----------------------------------------------------------*/

static void prvLedsUpdate( void )
{
	bool bInvert = ( pxLeds->ePolarity == LED_ACTIVE_HIGH ) ? false : true;
	/* !! results in true if any bit is set */
	bool bRedOn	= bInvert ^ !!( eEnabledLeds & LEDS_RED );
	bool bGreenOn  = bInvert ^ !!( eEnabledLeds & LEDS_GREEN );
	bool bBlueOn   = bInvert ^ !!( eEnabledLeds & LEDS_BLUE );
	bool bYellowOn = bInvert ^ !!( eEnabledLeds & LEDS_YELLOW );

	if ( !bGpioEqual( pxLeds->xRed, UNUSED_GPIO ) ) {
		vGpioWrite( pxLeds->xRed, bRedOn );
	}
	if ( !bGpioEqual( pxLeds->xGreen, UNUSED_GPIO ) ) {
		vGpioWrite( pxLeds->xGreen, bGreenOn );
	}
	if ( !bGpioEqual( pxLeds->xBlue, UNUSED_GPIO ) ) {
		vGpioWrite( pxLeds->xBlue, bBlueOn );
	}
	if ( !bGpioEqual( pxLeds->xYellow, UNUSED_GPIO ) ) {
		vGpioWrite( pxLeds->xYellow, bYellowOn );
	}
}

/*-----------------------------------------------------------*/
