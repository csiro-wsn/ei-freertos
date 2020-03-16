/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "csiro_math.h"
#include "misc_encodings.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define RANGE_MAX			0x1000000
#define POW_10_7(x) 		((x) * 10000000ll)

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

/**
 * Refer to header documentation for behavioural specification
 *
 * To achieve the desired rounding behaviour without floats,
 * a shift of half the resolution it applied to the input.
 * If the rounding was to the nearest 10:
 * Without the shift:
 * 				 9 => ( 9 / 10) * 10 => 0
 * 				10 => (10 / 10) * 10 => 1
 * 				11 => (11 / 10) * 10 => 1
 * 				15 => (15 / 10) * 10 => 1
 * With the shift:
 * 				 9 => (14 / 10) * 10 => 1
 * 				10 => (15 / 10) * 10 => 1
 * 				11 => (16 / 10) * 10 => 1
 * 				15 => (21 / 10) * 10 => 2
 * 
 * Desired behaviour (without +90 behaviour):
 * 		OUTPUT = (INPUT + shift + 90.0) * 0x1000000 / 180.0
 * 		SHIFT = 90.0 / 0x1000000
 * 
 * To reduce rounding errors, apply the 0x1000000 multiplication first
 * 		OUTPUT = ((INPUT * 0x1000000) + (90.0) + (90.0 * 0x1000000)) / 180.0
 */
uint32_t ulLatitudeTo24bit( int32_t lLatitude )
{
	const int32_t lMaximumLatitude = 900000000 - ( POW_10_7( 90 ) / RANGE_MAX ) - 1;
	configASSERT( lLatitude >= -900000000 );
	configASSERT( lLatitude <= +900000000 );
	/* Pull values near +90 back to valid range*/
	int64_t llLatitude = lLatitude > lMaximumLatitude ? lMaximumLatitude : lLatitude;
	/* Apply the optimised conversion */
	const uint64_t ullConstantShift = ( RANGE_MAX * POW_10_7( 90 ) ) + POW_10_7( 90 );
	const uint64_t ullNumerator		= ( uint64_t )( ( llLatitude * RANGE_MAX ) + ullConstantShift );
	return ( uint32_t )( ullNumerator / POW_10_7( 180 ) );
}

/*-----------------------------------------------------------*/

/**
 * Refer to header documentation for behavioural specification
 * Refer to latitude documentation for rounding behaviour
 * 
 * Desired behaviour (without +180 behaviour):
 * 		OUTPUT = (INPUT + shift + 180.0) * 0x1000000 / 360.0
 * 		SHIFT = 180.0 / 0x1000000
 * 
 * To reduce rounding errors, apply the 0x1000000 multiplication first
 * 		OUTPUT = ((INPUT * 0x1000000) + (180.0) + (180.0 * 0x1000000)) / 360.0
 */
uint32_t ulLongitudeTo24bit( int32_t lLongitude )
{
	configASSERT( lLongitude >= -1800000000 );
	configASSERT( lLongitude <= +1800000000 );
	/* Apply the optomised conversion */
	const uint64_t ullConstantShift = ( RANGE_MAX * POW_10_7( 180 ) ) + POW_10_7( 180 );
	const uint64_t ullNumerator		= ( uint64_t )( ( (int64_t) lLongitude * RANGE_MAX ) + ullConstantShift );
	/* The +180 degree wrapping is applied via the masking */
	return ( uint32_t )( ullNumerator / POW_10_7( 360 ) ) & 0xFFFFFF;
}

/*-----------------------------------------------------------*/
