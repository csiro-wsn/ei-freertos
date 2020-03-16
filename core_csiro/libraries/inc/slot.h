/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: slot.h
 * Creation_Date: 25/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Library for calculating slots for scheduler or more generic purposes
 * 
 */
#ifndef __CSIRO_CORE_SLOT
#define __CSIRO_CORE_SLOT
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eRadioSlotMethod_t {
	SLOT_METHOD_DISABLE,	/* No radio slotting */
	SLOT_METHOD_HASH,		/* Runtime constant hash */
	SLOT_METHOD_RANDOM,		/* Random slot within provided window*/
	SLOT_METHOD_RANDOM_TIME /* Completely random time within the window, no slotting */
} eRadioSlotMethod_t;

/* Function Declarations ------------------------------------*/

uint32_t ulSlotGetDelay( eRadioSlotMethod_t eMethod, uint32_t ulWindowLength, uint32_t ulSlotLength );

#endif /* __CSIRO_CORE_SLOT */
