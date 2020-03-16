/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: logger_mirror.h
 * Creation_Date: 04/01/2019
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Routines for duplicating data from one logger to another.
 * Canonical use case is copying data from onboard flash to external SD card
 * Tests currently run across most common use cases, but has never been tested on actual hardware
 * 
 */
#ifndef __CSIRO_CORE_LOGGER_MIRROR
#define __CSIRO_CORE_LOGGER_MIRROR
/* Includes -------------------------------------------------*/

#include "logger.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

uint32_t ulLogMirrorLostSrcPages( uint32_t ulPagesWritten, uint32_t ulNumPages, uint8_t ucEraseUnit );

eModuleError_t eLogMirror( xLogger_t *pxSource, xLogger_t *pxDestination );

#endif /* __CSIRO_CORE_LOGGER_MIRROR */
