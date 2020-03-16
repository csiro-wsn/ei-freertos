/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "buzzer.h"
#include "gpio.h"

#include "pwm.h"

/* Private Defines ------------------------------------------*/

// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static xPwmModule_t *pxBuzzerPwmModule;
static xGpio_t		 xEnableGpio;

/*-----------------------------------------------------------*/

void vBuzzerInit( xPwmModule_t *pxBuzzerPwm, xGpio_t xEnGpio )
{
	pxBuzzerPwmModule = pxBuzzerPwm;
	xEnableGpio		  = xEnGpio;
	vPwmInit( pxBuzzerPwmModule );
	vGpioSetup( xEnableGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
}

/*-----------------------------------------------------------*/

void vBuzzerStart( uint32_t ulFrequencyMilliHz )
{
	vGpioClear( xEnableGpio );
	vPwmStart( pxBuzzerPwmModule, ulFrequencyMilliHz, 50 );
}

/*-----------------------------------------------------------*/

void vBuzzerStop()
{
	vGpioSet( xEnableGpio );
	vPwmStop( pxBuzzerPwmModule );
}

/*-----------------------------------------------------------*/
