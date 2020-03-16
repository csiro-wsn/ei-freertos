/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "board.h"
#include "gpio.h"

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vGpioInit( void )
{
	nrfx_gpiote_init();
	/* Set all pins as disconnected inputs */
	for ( uint8_t ucPin = 0; ucPin < NUMBER_OF_PINS; ucPin++ ) {
		nrf_gpio_cfg( ucPin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE );
	}
}

/*-----------------------------------------------------------*/

void vGpioSetup( xGpio_t xGpio, eGpioType_t eType, uint32_t ulParam )
{
	ASSERT_GPIO_ASSIGNED( xGpio );

	switch ( eType ) {
		case GPIO_DISABLED:
			nrf_gpio_cfg( xGpio.ucPin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, ( ulParam == GPIO_DISABLED_NOPULL ) ? NRF_GPIO_PIN_NOPULL : NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE );
			break;
		case GPIO_INPUT:
			nrf_gpio_cfg( xGpio.ucPin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE );
			break;
		case GPIO_INPUTPULL:
			nrf_gpio_cfg( xGpio.ucPin, NRF_GPIO_PIN_DIR_INPUT, NRF_GPIO_PIN_INPUT_CONNECT, ( ulParam == GPIO_INPUTPULL_PULLUP ) ? NRF_GPIO_PIN_PULLUP : NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE );
			break;
		case GPIO_PUSHPULL:
			nrf_gpio_pin_write( xGpio.ucPin, ulParam );
			nrf_gpio_cfg( xGpio.ucPin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE );
			break;
		case GPIO_OPENDRAIN:
			nrf_gpio_cfg( xGpio.ucPin, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0D1, NRF_GPIO_PIN_NOSENSE );
			break;
		default:
			break;
	}
}

/*-----------------------------------------------------------*/

void vGpioWrite( xGpio_t xGpio, bool bValue )
{
	nrf_gpio_pin_write( xGpio.ucPin, bValue );
}

/*-----------------------------------------------------------*/

void vGpioSet( xGpio_t xGpio )
{
	nrf_gpio_pin_set( xGpio.ucPin );
}

/*-----------------------------------------------------------*/

void vGpioClear( xGpio_t xGpio )
{
	nrf_gpio_pin_clear( xGpio.ucPin );
}

/*-----------------------------------------------------------*/

void vGpioToggle( xGpio_t xGpio )
{
	nrf_gpio_pin_toggle( xGpio.ucPin );
}

/*-----------------------------------------------------------*/

bool bGpioRead( xGpio_t xGpio )
{
	return !!nrf_gpio_pin_read( xGpio.ucPin );
}

/*-----------------------------------------------------------*/

eModuleError_t eGpioConfigureInterrupt( xGpio_t xGpio, bool bEnable, eGpioInterruptEdge_t eInterruptEdge, fnGpioInterrupt_t fnCallback )
{
	/* Initialise the interrupt event if not already initialised */
	nrfx_err_t xNrfErrorCode = NRFX_SUCCESS;

	nrf_gpiote_polarity_t ePolarity = ( eInterruptEdge == GPIO_INTERRUPT_RISING_EDGE ) ? NRF_GPIOTE_POLARITY_LOTOHI : ( ( eInterruptEdge == GPIO_INTERRUPT_FALLING_EDGE ) ? NRF_GPIOTE_POLARITY_HITOLO : NRF_GPIOTE_POLARITY_TOGGLE );

	/* Disabling an interrupt is the easy case */
	if ( bEnable == false ) {
		nrfx_gpiote_in_event_disable( xGpio.ucPin );
		nrfx_gpiote_in_uninit( xGpio.ucPin );
	}
	else {
		/* Interrupt configuration */
		nrfx_gpiote_in_config_t xGpioConfig = {
			.sense			 = ePolarity,
			.pull			 = NRF_GPIO_PIN_NOPULL,
			.is_watcher		 = false,
			.hi_accuracy	 = true,
			.skip_gpio_setup = true
		};
		/* Attempt to configure the interrupt */
		xNrfErrorCode = nrfx_gpiote_in_init( xGpio.ucPin, &xGpioConfig, (nrfx_gpiote_evt_handler_t) fnCallback );
		if ( xNrfErrorCode == NRF_SUCCESS ) {
			nrfx_gpiote_in_event_enable( xGpio.ucPin, bEnable );
			return ERROR_NONE;
		}
		else if ( xNrfErrorCode == NRF_ERROR_NO_MEM ) {
			return ERROR_UNAVAILABLE_RESOURCE;
		}
		/* Pin was already configured, reconfigure */
		CRITICAL_SECTION_DECLARE;
		CRITICAL_SECTION_START();
		nrfx_gpiote_in_event_disable( xGpio.ucPin );
		nrfx_gpiote_in_uninit( xGpio.ucPin );
		xNrfErrorCode = nrfx_gpiote_in_init( xGpio.ucPin, &xGpioConfig, (nrfx_gpiote_evt_handler_t) fnCallback );
		/* As we just freed the interrupt, there will be one free */
		nrfx_gpiote_in_event_enable( xGpio.ucPin, bEnable );
		CRITICAL_SECTION_STOP();
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vGpioSetInterruptPull( xGpio_t xGpio, uint32_t ulPull )
{
	vGpioSetup( xGpio, GPIO_INPUT, ulPull );
}

/*-----------------------------------------------------------*/
