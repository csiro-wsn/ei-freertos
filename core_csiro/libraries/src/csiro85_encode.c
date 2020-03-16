/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "csiro85_encode.h"

#include "FreeRTOS.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static const uint32_t ulEncodingBase	  = 85;
static const uint8_t  ucEncodingOffset	= (uint8_t) '!';
static const uint8_t  ucEncodingOutputMax = 85 + '!' - 1;

/*-----------------------------------------------------------*/

uint32_t ulCsiro85Encode( uint8_t *pucBinary, uint32_t ulBinaryLen, uint8_t *pucEncoded, uint32_t ulEncodedMaxLen )
{
	uint32_t ulEncodedLen = ( ulBinaryLen * 5 / 4 );
	configASSERT( ulBinaryLen % 4 == 0 );
	configASSERT( ulEncodedLen <= ulEncodedMaxLen );

	uint32_t ulNumChunks = ulBinaryLen / 4;
	uint32_t ulChunk;
	/* Pointers the start of the last encoded and raw chunks */
	uint8_t *pucInput  = pucBinary + ulBinaryLen - 4;
	uint8_t *pucOutput = pucEncoded + ( ulBinaryLen * 5 / 4 ) - 5;

	while ( ulNumChunks-- ) {
		ulChunk			   = BE_U32_EXTRACT( pucInput );
		*( pucOutput + 4 ) = ( ulChunk % ulEncodingBase ) + ucEncodingOffset;
		ulChunk /= ulEncodingBase;
		*( pucOutput + 3 ) = ( ulChunk % ulEncodingBase ) + ucEncodingOffset;
		ulChunk /= ulEncodingBase;
		*( pucOutput + 2 ) = ( ulChunk % ulEncodingBase ) + ucEncodingOffset;
		ulChunk /= ulEncodingBase;
		*( pucOutput + 1 ) = ( ulChunk % ulEncodingBase ) + ucEncodingOffset;
		ulChunk /= ulEncodingBase;
		*( pucOutput + 0 ) = ulChunk + ucEncodingOffset;
		pucOutput -= 5;
		pucInput -= 4;
	}
	return ulEncodedLen;
}

/*-----------------------------------------------------------*/

uint32_t ulCsiro85Decode( uint8_t *pucEncoded, uint32_t ulEncodedLen, uint8_t *pucBinary, uint32_t ulBinaryMaxLen )
{
	uint32_t ulDecodedLen = ( ulEncodedLen * 4 / 5 );
	/* Validate the various lengths */
	configASSERT( ulEncodedLen % 5 == 0 );
	configASSERT( ulDecodedLen <= ulBinaryMaxLen );
	/* Chunk variables */
	uint32_t ulNumChunks = ulEncodedLen / 5;
	uint32_t ulChunk;
	/* Decoding occurs from front to back, 5 bytes compressing to 4 bytes */
	/* Supports in place operation as the chunk value is calculated before values are overwritten */
	while ( ulNumChunks-- ) {
		/* Remove the encoding offset */
		pucEncoded[0] -= ucEncodingOffset;
		pucEncoded[1] -= ucEncodingOffset;
		pucEncoded[2] -= ucEncodingOffset;
		pucEncoded[3] -= ucEncodingOffset;
		pucEncoded[4] -= ucEncodingOffset;
		/* 85^4 * buffer[0] + 85^3 * buffer[1] + 85^2 * buffer[2] + 85^1 * buffer[3] + 85^0 * buffer[4] */
		ulChunk = ( 52200625 * (uint32_t) pucEncoded[0] ) + ( 614125 * (uint32_t) pucEncoded[1] ) + ( 7225 * (uint32_t) pucEncoded[2] ) + ( 85 * (uint32_t) pucEncoded[3] ) + ( pucEncoded[4] );
		/* Extract bits into bytes */
		pucBinary[0] = ( ulChunk >> 24 ) & 0xFF;
		pucBinary[1] = ( ulChunk >> 16 ) & 0xFF;
		pucBinary[2] = ( ulChunk >> 8 ) & 0xFF;
		pucBinary[3] = ( ulChunk >> 0 ) & 0xFF;
		/* Update pointers */
		pucEncoded += 5;
		pucBinary += 4;
	}
	return ulDecodedLen;
}

/*-----------------------------------------------------------*/

bool bCsiro85Valid( uint8_t *pucEncoded, uint32_t ulEncodedLen )
{
	uint8_t ucByte;
	for ( uint32_t i = 0; i < ulEncodedLen; i++ ) {
		ucByte = pucEncoded[i];
		/* Check byte is in valid range */
		if ( ( ucByte < ucEncodingOffset ) || ( ucByte > ucEncodingOutputMax ) ) {
			return false;
		}
	}
	return true;
}

/*-----------------------------------------------------------*/
