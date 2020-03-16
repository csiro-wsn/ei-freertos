/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/

// clang-format off
// put defines here if you don't want them to be auto-formatted

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void *pvMemset( void *pvPtr, uint8_t ucValue, uint32_t ulLen )
{
	uint32_t ulIndex	 = 0;
	uint32_t ulBlocks	= ulLen >> 2;
	uint32_t ulRemaining = ulLen & 0x03;
	while ( ulBlocks-- ) {
		*( ( (uint8_t *) pvPtr ) + ulIndex + 0 ) = ucValue;
		*( ( (uint8_t *) pvPtr ) + ulIndex + 1 ) = ucValue;
		*( ( (uint8_t *) pvPtr ) + ulIndex + 2 ) = ucValue;
		*( ( (uint8_t *) pvPtr ) + ulIndex + 3 ) = ucValue;
		ulIndex += 4;
	}
	while ( ulRemaining-- ) {
		*( ( (uint8_t *) pvPtr ) + ulIndex++ ) = ucValue;
	}
	return pvPtr;
}

/*-----------------------------------------------------------*/

void *pvMemcpy( void *pvDestination, const void *pvSource, uint32_t ulLen )
{
	uint8_t *pucDest	 = (uint8_t *) pvDestination;
	uint8_t *pucSource   = (uint8_t *) pvSource;
	uint32_t ulIndex	 = 0;
	uint32_t ulBlocks	= ulLen >> 2;
	uint32_t ulRemaining = ulLen & 0x03;
	while ( ulBlocks-- ) {
		*( pucDest + ulIndex + 0 ) = *( pucSource + ulIndex + 0 );
		*( pucDest + ulIndex + 1 ) = *( pucSource + ulIndex + 1 );
		*( pucDest + ulIndex + 2 ) = *( pucSource + ulIndex + 2 );
		*( pucDest + ulIndex + 3 ) = *( pucSource + ulIndex + 3 );
		ulIndex += 4;
	}
	while ( ulRemaining-- ) {
		*( pucDest + ulIndex ) = *( pucSource + ulIndex );
		ulIndex++;
	}
	return pvDestination;
}

/*-----------------------------------------------------------*/

int32_t lMemcmp( const void *pvPtr1, const void *pvPtr2, uint32_t ulLen )
{
	uint8_t *pucPtr1 = (uint8_t *) pvPtr1;
	uint8_t *pucPtr2 = (uint8_t *) pvPtr2;
	uint32_t ulIndex = 0;
	while ( ulIndex < ulLen ) {
		if ( *( pucPtr1 + ulIndex ) != *( pucPtr2 + ulIndex ) ) {
			if ( *( pucPtr1 + ulIndex ) > *( pucPtr2 + ulIndex ) ) {
				return ulIndex + 1;
			}
			else {
				return -( ulIndex + 1 );
			}
		}
		ulIndex++;
	}
	return 0;
}

/*-----------------------------------------------------------*/

uint32_t ulStrLen( const void *pvPtr )
{
	uint32_t ulLen  = 0;
	uint8_t *pucStr = (uint8_t *) pvPtr;
	while ( *pucStr ) {
		ulLen++;
		pucStr++;
	}
	return ulLen;
}

/*-----------------------------------------------------------*/

uint32_t ulArraySearchByte( const uint8_t *pucArray, uint8_t ucArrayValue, uint32_t ulMaximumLength )
{
	for ( uint32_t i = 0; i < ulMaximumLength; i++ ) {
		if ( pucArray[i] == ucArrayValue ) {
			return i;
		}
	}
	return UINT32_MAX;
}

/*-----------------------------------------------------------*/

uint32_t ulArraySearchWord( const uint32_t *pulArray, uint32_t ulArrayValue, uint32_t ulMaximumLength )
{
	for ( uint32_t i = 0; i < ulMaximumLength; i++ ) {
		if ( pulArray[i] == ulArrayValue ) {
			return i;
		}
	}
	return UINT32_MAX;
}

/*-----------------------------------------------------------*/