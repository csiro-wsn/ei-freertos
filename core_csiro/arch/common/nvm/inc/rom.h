/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: rom.h
 * Creation_Date: 05/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Helper functions for device ROM characteristics
 * 
 */
#ifndef __CSIRO_CORE_ROM
#define __CSIRO_CORE_ROM
/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xDeviceRomConfiguration_t
{
	uint32_t ulRomPageSize;
	uint32_t ulRomPages;
	uint8_t  ucEraseByte;
} xDeviceRomConfiguration_t;

/* Function Declarations ------------------------------------*/

void vRomConfigurationQuery( xDeviceRomConfiguration_t *pxConfiguration );

#endif /* __CSIRO_CORE_ROM */
