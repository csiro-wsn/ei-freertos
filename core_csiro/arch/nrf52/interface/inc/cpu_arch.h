/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: cpu_arch.h
 * Creation_Date: 15/10/2018
 * Author: Luke Swift <Luke.Swift@data61.csiro.au>
 *
 * Platform specific CPU functions
 * Mainly around critical sections
 * 
 */
#ifndef __CSIRO_CORE_CPU_PLATFORM
#define __CSIRO_CORE_CPU_PLATFORM
/* Includes -------------------------------------------------*/

#include "app_util_platform.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define CRITICAL_SECTION_DECLARE  uint8_t irqState
#define CRITICAL_SECTION_START()  app_util_critical_region_enter(&irqState);
#define CRITICAL_SECTION_STOP()   app_util_critical_region_exit(irqState);

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static inline uint32_t ulCpuClockFreq( void )
{
	return 64000000;
}

#endif /* __CSIRO_CORE_CPU_PLATFORM */
