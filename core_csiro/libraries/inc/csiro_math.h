/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: csiro_math.h
 * Creation_Date: 13/08/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Utility math functions
 * 
 */
#ifndef __CSIRO_CORE_UTIL_MATH
#define __CSIRO_CORE_UTIL_MATH

/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

/*
 * Calculate the Squared Magnitude of a 3 dimensional vector
 * 
 * VECTOR_SQR_MAGNITUDE(x,y,z) = (x^2) + (y^2) + (z^2)
 */
#define VECTOR_SQR_MAGNITUDE( x, y, z ) ( ( ( int32_t )( x ) * ( int32_t )( x ) ) + ( ( int32_t )( y ) * ( int32_t )( y ) ) + ( ( int32_t )( z ) * ( int32_t )( z ) ) )

/*
 * Convert an error to its equivalent error when the value to compare against is squared
 * 
 * For example if we want to check that 850 < 1000 * 90%
 * Once both sides are squared 90% needs to become 81%
 * Useful when comparing vectors (VECTOR_SQR_MAGNITUDE), whos squared magnitude can be calculated easily,
 * but whose magnitude requires an expensive and slow square root
 */
#define VECTOR_SQR_PERCENTAGE_ERROR( error_percentage ) ( ( error_percentage * error_percentage ) / 100 )

/*
 * Simple macro which returns true if the supplied value is a power of 2
 */
#define IS_POWER_2( x ) ( ( x ) && !( ( x ) & ( (x) -1 ) ) )

/*
 * Integer round down function
 */
#define ROUND_DOWN( x, n ) ( ( ( x ) / ( n ) ) * ( n ) )

/*
 * Integer round up function
 */
#define ROUND_UP( x, n ) ( ROUND_DOWN( ( x ) + (n) -1, ( n ) ) )

/*
 * Clamping one value between two others.
 */
#define CLAMP( val, max, min ) ( ( val > max ) ? max : ( ( val < min ) ? min : val ) )

/*
 * Return maximum of two values
 */
#define MAX( a, b ) ( ( a ) < ( b ) ? ( b ) : ( a ) )

/*
 * Return minimum of two values
 */
#define MIN( a, b ) ( ( a ) < ( b ) ? ( a ) : ( b ) )

/*
 * Boolean check if an integer is between a minimum and maximum (inclusive)
 */
#define VALUE_IN_RANGE( min, val, max ) ( ( min <= val ) && ( val <= max ) )

/*
 * True when two integers have the same sign
 * Zero is treated as positive
 */
#define SIGNS_MATCH( x, y ) ( ( ( x ) >= 0 ) == ( ( y ) >= 0 ) )

#define __DIVISION_ROUNDED_SAME_SIGN( num, denom ) ( ( ( num ) + ( ( denom ) / 2 ) ) / ( denom ) )
#define __DIVISION_ROUNDED_DIFF_SIGN( num, denom ) ( ( ( num ) - ( ( denom ) / 2 ) ) / ( denom ) )

/*
 * Unsigned integer division with rounding
 *      // (62 / 25) = 2.48
 *      UNSIGNED_DIVISION_ROUNDED( 62, 25 ) = 2 
 *      // (63 / 25) = 2.52
 *      UNSIGNED_DIVISION_ROUNDED( 63, 25 ) = 3 
 * 
 *  http://dystopiancode.blogspot.com/2012/08/integer-rounding-in-ansi-c.html
 */
#define UNSIGNED_DIVISION_ROUNDED( numerator, denominator ) __DIVISION_ROUNDED_SAME_SIGN( numerator, denominator )

/*
 * Signed integer division with rounding
 *      // (-38 / 25) = -1.52
 *      UNSIGNED_DIVISION_ROUNDED( -38, 25 ) = -2
 *      // (-37 / 25) = -1.48
 *      UNSIGNED_DIVISION_ROUNDED( -37, 25 ) = -1
 * 
 *  @note Both parameters MUST either be the same type
 * 
 *  Adapted from: http://dystopiancode.blogspot.com/2012/08/integer-rounding-in-ansi-c.html
 */
#define SIGNED_DIVISION_ROUNDED( numerator, denominator ) ( ( SIGNS_MATCH( numerator, denominator ) ) ? ( __DIVISION_ROUNDED_SAME_SIGN( numerator, denominator ) ) : ( __DIVISION_ROUNDED_DIFF_SIGN( numerator, denominator ) ) )

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

uint32_t ulSquareRoot( uint32_t ulInput );

/**
 * Iterate through the bits in a bitmask, from low to high
 * 
 * \param ulMask        Bit mask to iterate through
 * \param ulCurrentBit  Current bitmask bit, or 0 to start iteration
 * \return              The next bit in the mask
 */
uint32_t ulBitmaskIterate( uint32_t ulMask, uint32_t ulCurrentBit );

/**
 * Determine the bin index that ucValue falls in for the provided bins
 * 
 * pulBins must be sorted in ascending order
 *          pucComparisonBins[0]      = 0
 *          pucComparisonBins[1:n-1]  Are ascending
 *          pucComparisonBins[n]      = UINT32_MAX
 * 
 * ulValue falls in bin index i if the following comparison is true:
 *          (pulBins[i] <= ulValue < pulBins[i+1])
 * 
 * \param   ucValue     Value to test against bins
 * \param   pucBins     pulComparisonBins[1:n-1]
 * \param   ucNumBins   Number of bins in pucBins
 * \return              The bin index
 */
uint8_t ucBinIndexByte( uint8_t ucValue, const uint8_t *pucBins, uint8_t ucNumBins );

/**
 * Determine the bin index that ulValue falls in for the provided bins
 * 
 * pulBins must be sorted in ascending order
 *          pulComparisonBins[0]      = 0
 *          pulComparisonBins[1:n-1]  Are ascending
 *          pulComparisonBins[n]      = UINT32_MAX
 * 
 * ulValue falls in bin index i if the following comparison is true:
 *          (pulBins[i] <= ulValue < pulBins[i+1])
 * 
 * \param   ulValue     Value to test against bins
 * \param   pulBins     pulComparisonBins[1:n-1]
 * \param   ucNumBins   Number of bins in pucBins
 * \return              The bin index
 */
uint8_t ucBinIndexLong( uint32_t ulValue, const uint32_t *pulBins, uint8_t ucNumBins );

/**
 * Compress a provided value to a bin index
 * Bins are created from 'ulBinsStart' to 'ulBinsEnd'
 * There are 'ulDivisions' bins created
 * Values smaller than 'ulBinsStart' are treated as 'ulBinsStart'
 * Values larger than 'ulBinsEnd' are treated as 'ulBinsEnd'
 * 
 * \param   ulValue         Value to compress
 * \param   ulBinsStart     Start of the compression range
 * \param   ulBinsEnd       End of the compression range
 * \param   ulDivisions     Number of indicies within the compression range
 * \returns Bin index       Index of the value within the compression range
 */
uint32_t ulValueBin( uint32_t ulValue, uint32_t ulBinsStart, uint32_t ulBinsEnd, uint32_t ulDivisions );

/**
 * Given a low number and a high number, and a percentage from 0 to 100,
 * returns the number that is that percentage between the low and high numbers.
 * 
 * For example: If ulLow = 100, ulHigh = 200, and ulPercentage = 50,
 * then it will return 150. Check out the unit tests at csiro_math_test.cpp
 * for more examples.
 * 
 * \param   ulLow       The low number
 * \param   ulHigh      The high number
 * \param   ulPercentage    The number you wish to find the fraction of
 * \return  ulNumber    The fraction between ulLow and ulHigh that ulNumber is.
 *                      Value is in percent from 0 to 100.    
 **/
uint32_t ulPercentageToNumber( uint32_t ulLow, uint32_t ulHigh, uint8_t ulPercentage );

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_UTIL_MATH */
