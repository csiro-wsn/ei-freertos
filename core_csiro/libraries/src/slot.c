/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "slot.h"

#include "address.h"
#include "board.h"
#include "random.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

static uint32_t ulGenerateHash( uint32_t ulKey );
static uint32_t ulCalculateWait( uint32_t ulSlotNum, uint32_t ulSlotLength, uint32_t ulNumSlots );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

uint32_t ulSlotGetDelay( eRadioSlotMethod_t eMethod, uint32_t ulWindowLength, uint32_t ulSlotLength )
{
	uint32_t ulDelay;
	uint32_t ulNumSlots = ulWindowLength / ulSlotLength;
	uint32_t ulRandom;

	switch ( eMethod ) {
		case SLOT_METHOD_HASH:
			ulDelay = ulCalculateWait( ulGenerateHash( LOCAL_ADDRESS & UINT32_MAX ), ulSlotLength, ulNumSlots );
			break;
		case SLOT_METHOD_RANDOM:
			eRandomGenerate( (uint8_t *) &ulRandom, 4 );
			ulDelay = ulCalculateWait( ulRandom, ulSlotLength, ulNumSlots );
			break;
		case SLOT_METHOD_RANDOM_TIME:
			eRandomGenerate( (uint8_t *) &ulRandom, 4 );
			ulDelay = ulRandom % ( ulWindowLength - ulSlotLength );
			break;
		default:
			ulDelay = 0;
			break;
	}
	return ulDelay;
}

/*-----------------------------------------------------------*/

static uint32_t ulCalculateWait( uint32_t ulSlotNum, uint32_t ulSlotLength, uint32_t ulNumSlots )
{
	if ( ulNumSlots == 0 ) {
		return 0;
	}
	else {
		return ( ulSlotNum % ulNumSlots ) * ulSlotLength;
	}
}

/*-----------------------------------------------------------*/

/**
 * From https://gist.github.com/badboy/6267743
 **/
static uint32_t ulGenerateHash( uint32_t ulKey )
{
	ulKey = ( ulKey + 0x7ed55d16 ) + ( ulKey << 12 );
	ulKey = ( ulKey ^ 0xc761c23c ) ^ ( ulKey >> 19 );
	ulKey = ( ulKey + 0x165667b1 ) + ( ulKey << 5 );
	ulKey = ( ulKey + 0xd3a2646c ) ^ ( ulKey << 9 );
	ulKey = ( ulKey + 0xfd7046c5 ) + ( ulKey << 3 );
	ulKey = ( ulKey ^ 0xb55a4f09 ) ^ ( ulKey >> 16 );
	return (uint16_t) ulKey;
}

/*-----------------------------------------------------------*/
