/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: cpu_arch.h
 * Creation_Date: 15/10/2018
 * Author: cpu_platform <jordan.yates@data61.csiro.au>
 *
 * Platform specific CPU functions
 * Mainly around critical sections
 * 
 */
#ifndef __CSIRO_CORE_CPU_PLATFORM
#define __CSIRO_CORE_CPU_PLATFORM
/* Includes -------------------------------------------------*/

#include "em_cmu.h"
#include "em_core.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define CRITICAL_SECTION_DECLARE        CORE_DECLARE_IRQ_STATE
#define CRITICAL_SECTION_START()        CORE_ENTER_CRITICAL()
#define CRITICAL_SECTION_STOP()         CORE_EXIT_CRITICAL()

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static inline uint32_t ulCpuClockFreq( void )
{
	return CMU_ClockFreqGet( cmuClock_CORE );
}

#endif /* __CSIRO_CORE_CPU_PLATFORM */
