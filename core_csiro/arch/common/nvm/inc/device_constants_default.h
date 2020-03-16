/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: device_constants_default.h
 * Creation_Date: 04/04/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Fallback include file for device constants
 * 
 */

#ifndef __CSIRO_CORE_DEVICE_CONSTANTS_DEFAULT
#define __CSIRO_CORE_DEVICE_CONSTANTS_DEFAULT

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/

typedef struct xDeviceConstants_t
{
	uint32_t ulKey;
	uint8_t  ucApplicationImageSlots;
} ATTR_PACKED xDeviceConstants_t;

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_DEVICE_CONSTANTS_DEFAULT */
