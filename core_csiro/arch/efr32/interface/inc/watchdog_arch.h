/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: watchdog.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Watchdog Abstraction for EFR32
 *
 */

#ifndef __CORE_CSIRO_UTIL_WATCHDOG_PLATFORM
#define __CORE_CSIRO_UTIL_WATCHDOG_PLATFORM

/* Includes -------------------------------------------------*/

#include "em_wdog.h"

/* Module Defines -------------------------------------------*/

#define WATCHDOG_INT_CLEAR( HANDLE ) WDOGn_IntClear( HANDLE, WDOG_IEN_TOUT )

#if defined __arm__

#define WATCHDOG_HANDLER_BUILD( IRQ_NAME )     \
	ATTR_NAKED void IRQ_NAME( void )           \
	{                                          \
		__asm __volatile(                      \
			"tst   LR, #4\n"                   \
			"ite   EQ\n"                       \
			"mrseq R0, MSP\n"                  \
			"mrsne R0, PSP\n"                  \
			"b     vWatchdogRunInterrupt\n" ); \
	}

#else

#define WATCHDOG_HANDLER_BUILD( IRQ_NAME ) \
	void IRQ_NAME( void )                  \
	{                                      \
		uint32_t pulStack[6] = { 0 };      \
		vWatchdogRunInterrupt( pulStack ); \
	}

#endif

/* Type Definitions -----------------------------------------*/

typedef WDOG_TypeDef xWatchdogHandle_t;

/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_UTIL_WATCHDOG_PLATFORM */
