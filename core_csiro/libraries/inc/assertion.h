/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: assertion.h
 * Creation_Date: 05/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Runtime Assertions
 * 
 */
#ifndef __CSIRO_CORE_LIBRARIES_STANDALONE_ASSERTION
#define __CSIRO_CORE_LIBRARIES_STANDALONE_ASSERTION
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

#define configASSERT( x )                                               \
	if ( ( x ) == 0 ) {                                                 \
		vAssertionFailed( __FILE__, __LINE__, __get_PC(), __get_LR() ); \
	}

/* Function Declarations ------------------------------------*/

void vAssertionFailed( const char *pcFile, int32_t lLine, uint32_t ulPC, uint32_t ulLR );

#endif /* __CSIRO_CORE_LIBRARIES_STANDALONE_ASSERTION */
