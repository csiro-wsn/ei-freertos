/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"

#include "compiler_intrinsics.h"
#include "csiro_math.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* Functions ------------------------------------------------*/

uint32_t ulSquareRoot( uint32_t ulInput )
{
	uint32_t op  = ulInput;
	uint32_t res = 0;
	uint32_t one = 1uL << 30; // The second-to-top bit is set: use 1u << 14 for uint16_t type; use 1uL<<30 for int32_t type
	// "one" starts at the highest power of four <= than the argument.
	while ( one > op ) {
		one >>= 2;
	}

	while ( one != 0 ) {
		if ( op >= res + one ) {
			op  = op - ( res + one );
			res = res + 2 * one;
		}
		res >>= 1;
		one >>= 2;
	}
	return res;
}

/*-----------------------------------------------------------*/

uint32_t ulBitmaskIterate( uint32_t ulBitmask, uint32_t ulCurrentBit )
{
	/* ulCurrentBit must be either 0 or a single bit */
	configASSERT( COUNT_ONES( ulCurrentBit ) < 2 );
	/* An empty bitmask has no bits to iterate through*/
	if ( ulBitmask == 0 ) {
		return 0;
	}
	/* Create a mask that has all bits lower than ulCurrentBit inclusive set */
	const uint32_t ulLowerMask		  = ( ulCurrentBit - 1 ) | ulCurrentBit;
	const uint32_t ulHigherMask		  = ~ulLowerMask;
	const uint32_t ulBitmaskRemaining = ulBitmask & ulHigherMask;
	const uint32_t ulNextBitIndex	 = ( ulBitmaskRemaining == 0 ) ? FIND_FIRST_SET( ulBitmask ) : FIND_FIRST_SET( ulBitmaskRemaining );
	/* FIND_FIRST_SET is 1 indexed */
	return 0x01 << ( ulNextBitIndex - 1 );
}

/*-----------------------------------------------------------*/

/**
 * Testing the following comparison for index i:
 *          	(pucBins[i] <= ucValue < pucBins[i+1])
 */
uint8_t ucBinIndexByte( uint8_t ucValue, const uint8_t *pucBins, uint8_t ucNumBins )
{
	uint8_t ucBin;
	for ( ucBin = 0; ucBin < ucNumBins; ucBin++ ) {
		if ( ucValue < pucBins[ucBin] ) {
			break;
		}
	}
	return ucBin;
}

/*-----------------------------------------------------------*/

/**
 * Testing the following comparison for index i:
 *          	(pulBins[i] <= ulValue < pulBins[i+1])
 */
uint8_t ucBinIndexLong( uint32_t ulValue, const uint32_t *pulBins, uint8_t ucNumBins )
{
	uint8_t ucBin;
	for ( ucBin = 0; ucBin < ucNumBins; ucBin++ ) {
		if ( ulValue < pulBins[ucBin] ) {
			break;
		}
	}
	return ucBin;
}

/*-----------------------------------------------------------*/

uint32_t ulValueBin( uint32_t ulValue, uint32_t ulBinsStart, uint32_t ulBinsEnd, uint32_t ulDivisions )
{
	configASSERT( ulBinsStart < ulBinsEnd );
	configASSERT( ulDivisions > 1 );

	ulValue = CLAMP( ulValue, ulBinsEnd, ulBinsStart );

	const uint32_t ulDivisor = ( ( ulBinsEnd - ulBinsStart ) * 100 ) / ( ulDivisions - 1 );

	return ( ( ulValue - ulBinsStart ) * 100 ) / ulDivisor;
}

/*-----------------------------------------------------------*/

uint32_t ulPercentageToNumber( uint32_t ulLow, uint32_t ulHigh, uint8_t ulPercentage )
{
	/* Assert numbers are reasonable. */
	configASSERT( ulLow <= ulHigh );
	COMPILER_WARNING_DISABLE( "-Wtype-limits" );
	uint32_t ulClampedPercentage = CLAMP( ulPercentage, 100, 0 );
	COMPILER_WARNING_ENABLE();
	uint32_t ulNormHigh = ulHigh - ulLow;
	return ( ( ulNormHigh * ulClampedPercentage / 100 ) + ulLow );
}

/*-----------------------------------------------------------*/
