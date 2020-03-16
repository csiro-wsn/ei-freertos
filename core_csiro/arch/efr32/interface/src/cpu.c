/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "cpu.h"
#include "em_cmu.h"
#include "em_device.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

void vInterruptSetPriority( int32_t IRQn, uint32_t ulPriority )
{
	NVIC_SetPriority( (IRQn_Type) IRQn, ulPriority );
}

/*-----------------------------------------------------------*/

void vInterruptClearPending( int32_t IRQn )
{
	NVIC_ClearPendingIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vInterruptEnable( int32_t IRQn )
{
	NVIC_EnableIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vInterruptDisable( int32_t IRQn )
{
	NVIC_DisableIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vPendContextSwitch( void )
{
	portNVIC_INT_CTRL_REG = portNVIC_PENDSVSET_BIT;
}

/*-----------------------------------------------------------*/

void vSystemReboot( void )
{
	NVIC_SystemReset();
}

/*-----------------------------------------------------------*/
