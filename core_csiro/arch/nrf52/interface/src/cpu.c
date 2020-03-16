/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "cpu.h"
#include "nrf_nvic.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

void vInterruptSetPriority( int32_t IRQn, uint32_t ulPriority )
{
	sd_nvic_SetPriority( (IRQn_Type) IRQn, ulPriority );
}

/*-----------------------------------------------------------*/

void vInterruptClearPending( int32_t IRQn )
{
	sd_nvic_ClearPendingIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vInterruptEnable( int32_t IRQn )
{
	sd_nvic_EnableIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vInterruptDisable( int32_t IRQn )
{
	sd_nvic_DisableIRQ( (IRQn_Type) IRQn );
}

/*-----------------------------------------------------------*/

void vPendContextSwitch( void )
{
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;
}

/*-----------------------------------------------------------*/

void vSystemReboot( void )
{
	sd_nvic_SystemReset();
}

/*-----------------------------------------------------------*/
