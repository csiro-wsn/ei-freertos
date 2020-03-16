/*
 * Copyright (c) 2019, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * The functions defined in this file are specific to the softdevice of the cpu 
 * that this file was created for. 
 * 
 * Currently, the only functionality is rounding a given transmit power to the 
 * nearest valid value supported by the softdevice in use.
 *
 */

/* Includes -------------------------------------------------*/

#include "fpu_handler.h"
#include "board.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define FPU_EXCEPTION_MASK 0x0000009F

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vInitFPU( void )
{
	vInterruptSetPriority( FPU_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
	vInterruptEnable( FPU_IRQn );
}

/*-----------------------------------------------------------*/

void FPU_IRQHandler( void )
{
	uint32_t *fpscr = (uint32_t *) ( FPU->FPCAR + 0x40 );
	(void) __get_FPSCR();

	*fpscr = *fpscr & ~( FPU_EXCEPTION_MASK );
}

/*-----------------------------------------------------------*/
