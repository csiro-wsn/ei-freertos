/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "compiler_intrinsics.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void NMI_Handler( void )
{
	while ( 1 ) continue;
}

/*-----------------------------------------------------------*/

ATTR_NAKED void HardFault_Handler( void )
{
	__asm volatile(
		" tst lr, #4                                                \n"
		" ite eq                                                    \n"
		" mrseq r0, msp                                             \n"
		" mrsne r0, psp                                             \n"
		" b prvGetRegistersFromStack                                \n" );
}

/*-----------------------------------------------------------*/

void MemManage_Handler( void )
{
	while ( 1 ) continue;
}

/*-----------------------------------------------------------*/

void BusFault_Handler( void )
{
	while ( 1 ) continue;
}

/*-----------------------------------------------------------*/

void UsageFault_Handler( void )
{
	while ( 1 ) continue;
}

/*-----------------------------------------------------------*/

void prvGetRegistersFromStack( uint32_t *pulFaultStackAddress )
{
	/**
	 * These are volatile to try and prevent the compiler/linker optimising
	 * them away as the variables never actually get used.  If the debugger
	 * won't show the values of the variables, make them global my moving their
	 * declaration outside of this function.
	 **/

	COMPILER_WARNING_DISABLE( "-Wunused-but-set-variable" );

	volatile uint32_t r0;
	volatile uint32_t r1;
	volatile uint32_t r2;
	volatile uint32_t r3;
	volatile uint32_t r12;
	volatile uint32_t lr;  /* Link register. */
	volatile uint32_t pc;  /* Program counter. */
	volatile uint32_t psr; /* Program status register. */

	r0 = pulFaultStackAddress[0];
	r1 = pulFaultStackAddress[1];
	r2 = pulFaultStackAddress[2];
	r3 = pulFaultStackAddress[3];

	r12 = pulFaultStackAddress[4];
	lr  = pulFaultStackAddress[5];
	pc  = pulFaultStackAddress[6];
	psr = pulFaultStackAddress[7];

	COMPILER_WARNING_ENABLE();

	/* When the following line is hit, the variables contain the register values. */
	for ( ;; )
		;
}

/*-----------------------------------------------------------*/
