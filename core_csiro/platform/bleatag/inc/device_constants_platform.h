/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: device_constants_platform.h
 * Creation_Date: 11/06/2019
 * Author: Karl Von Richter <karl.vonrichter@data61.csiro.au>
 *
 * Sat specific device constants
 * 
 */

#ifndef __CSIRO_CORE_DEVICE_CONSTANTS_PLATFORM
#define __CSIRO_CORE_DEVICE_CONSTANTS_PLATFORM

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/

typedef struct xDeviceConstants_t
{
	uint32_t ulKey;
	uint8_t  ucApplicationImageSlots;
	uint8_t  pucIEEEMac[6];
	uint8_t  pucOwnerKey[16];
} ATTR_PACKED xDeviceConstants_t;

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_DEVICE_CONSTANTS_PLATFORM */
