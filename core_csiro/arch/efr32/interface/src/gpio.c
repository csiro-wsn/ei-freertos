/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "gpio.h"
#include "cpu.h"

#include "em_cmu.h"
#include "em_gpio.h"

#include "gpiointerrupt.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define UNUSED_GPIO_PORT	0xFF

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

uint8_t ucFindInterruptLine( xGpio_t xGpio );

/* Private Variables ----------------------------------------*/

static xGpio_t xUnusedInterrupt			  = { UNUSED_GPIO_PORT, 0 };
static xGpio_t xInterruptLineMappings[16] = { { 0 } };

/*-----------------------------------------------------------*/

void vGpioInit( void )
{
	CMU_ClockEnable( cmuClock_GPIO, true );
	/* Configure all pins as disabled */
	for ( uint8_t ucPort = 0; ucPort < GPIO_COUNT; ucPort++ ) {
		GPIO->P[ucPort].DOUT  = 0x00;
		GPIO->P[ucPort].MODEL = 0x00;
		GPIO->P[ucPort].MODEH = 0x00;
		GPIO->P[ucPort].MODEL = 0x00;
	}
	/* Setup interrupts */
	GPIOINT_Init();
	for ( uint8_t i = 0; i < 16; i++ ) {
		xInterruptLineMappings[i] = xUnusedInterrupt;
	}
	vInterruptSetPriority( GPIO_EVEN_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptSetPriority( GPIO_ODD_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
}

/*-----------------------------------------------------------*/

void vGpioSetup( xGpio_t xGpio, eGpioType_t eType, uint32_t ulParam )
{
	GPIO_Mode_TypeDef eMode;
	switch ( eType ) {
		case GPIO_DISABLED:
			eMode = gpioModeDisabled;
			break;
		case GPIO_INPUT:
			eMode = gpioModeInput;
			break;
		case GPIO_INPUTPULL:
			eMode = gpioModeInputPull;
			break;
		case GPIO_PUSHPULL:
			eMode = gpioModePushPull;
			break;
		case GPIO_OPENDRAIN:
			eMode = gpioModeWiredAnd;
			break;
		default:
			eMode = gpioModeDisabled;
			break;
	}
	GPIO_PinModeSet( xGpio.ePort, xGpio.ucPin, eMode, ulParam );
}

/*-----------------------------------------------------------*/

void vGpioWrite( xGpio_t xGpio, bool bValue )
{
	bValue ? GPIO_PinOutSet( xGpio.ePort, xGpio.ucPin ) : GPIO_PinOutClear( xGpio.ePort, xGpio.ucPin );
}

/*-----------------------------------------------------------*/

void vGpioSet( xGpio_t xGpio )
{
	GPIO_PinOutSet( xGpio.ePort, xGpio.ucPin );
}

/*-----------------------------------------------------------*/

void vGpioClear( xGpio_t xGpio )
{
	return GPIO_PinOutClear( xGpio.ePort, xGpio.ucPin );
}

/*-----------------------------------------------------------*/

void vGpioToggle( xGpio_t xGpio )
{
	return GPIO_PinOutToggle( xGpio.ePort, xGpio.ucPin );
}

/*-----------------------------------------------------------*/
bool bGpioRead( xGpio_t xGpio )
{
	return GPIO_PinInGet( xGpio.ePort, xGpio.ucPin );
}

/*-----------------------------------------------------------*/

/* ExtInt Pin Mapping to Interrupt Lines is non-trivial
 * See section 33.3.5.1 of the reference manual, Edge Interrupt Generation
 */

eModuleError_t eGpioConfigureInterrupt( xGpio_t xGpio, bool bEnable, eGpioInterruptEdge_t eInterruptEdge, fnGpioInterrupt_t fnCallback )
{
	// Verify that the pin number still has an interrupt line available
	// Pins 0->3 on all ports compete for the 4 interrupt lines 0->3
	// Pins 4->7 on all ports compete for the 4 interrupt lines 4->7
	// Etc etc up to pins 12->15
	uint8_t ucInterruptNumber;
	bool	bRisingEdge  = false;
	bool	bFallingEdge = false;

	switch ( eInterruptEdge ) {
		case GPIO_INTERRUPT_RISING_EDGE:
			bRisingEdge = true;
			break;
		case GPIO_INTERRUPT_FALLING_EDGE:
			bFallingEdge = true;
			break;
		case GPIO_INTERRUPT_BOTH_EDGE:
			bRisingEdge  = true;
			bFallingEdge = true;
			break;
		default:
			break;
	}
	/* Find an interrupt line to either associate with xGpio, or is already associated */
	ucInterruptNumber = ucFindInterruptLine( xGpio );
	/* If we didn't find an empty interrupt line and we are trying to enable an interrupt */
	if ( ( ucInterruptNumber == UINT8_MAX ) && bEnable ) {
		return ERROR_UNAVAILABLE_RESOURCE;
	}

	if ( bEnable ) {
		/* Note that this GPIO is mapped to the interrupt line */
		xInterruptLineMappings[ucInterruptNumber] = xGpio;
		/* Register the callback with the IRQ handler */
		GPIOINT_CallbackRegister( ucInterruptNumber, (GPIOINT_IrqCallbackPtr_t) fnCallback );
	}
	/* Only perform deregistration operations if interrupt was previously enabled */
	else if ( ucInterruptNumber != UINT8_MAX ) {
		/* Clear the mapping */
		xInterruptLineMappings[ucInterruptNumber] = xUnusedInterrupt;
		/* Unregister the callback from the IRQ handler */
		GPIOINT_CallbackUnRegister( ucInterruptNumber );
	}

	/* Configure the interrupt line */
	GPIO_ExtIntConfig( xGpio.ePort, xGpio.ucPin, ucInterruptNumber, bRisingEdge, bFallingEdge, bEnable );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

uint8_t ucFindInterruptLine( xGpio_t xGpio )
{
	uint8_t ucInterruptLine  = UINT8_MAX;
	uint8_t ucInterruptGroup = xGpio.ucPin / 4;
	xGpio_t xInterruptGpio;
	uint8_t i;
	/* Loop through the 4 interrupts that this pin could be assigned to */
	for ( i = 4 * ucInterruptGroup; i < ( 4 * ucInterruptGroup ) + 4; i++ ) {
		xInterruptGpio = xInterruptLineMappings[i];
		/* If this GPIO has already been assigned an interrupt line, return immediately */
		if ( ( xInterruptGpio.ePort == xGpio.ePort ) && ( xInterruptGpio.ucPin == xGpio.ucPin ) ) {
			ucInterruptLine = i;
			break;
		}
		/* If we haven't found a free interrupt line, and this line is free, store it as a free location */
		if ( ( ucInterruptLine == UINT8_MAX ) && ( xInterruptGpio.ePort == UNUSED_GPIO_PORT ) ) {
			ucInterruptLine = i;
		}
	}
	return ucInterruptLine;
}

/*-----------------------------------------------------------*/
