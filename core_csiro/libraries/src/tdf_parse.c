/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "tdf_auto.h"

#include "memory_operations.h"
#include "tdf_parse.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vTdfParseStart( xTdfParser_t *pxParser, uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	pxParser->pucBuffer						 = pucBuffer;
	pxParser->ulBufferLen					 = ulBufferLen;
	pxParser->ulCurrentOffset				 = 0;
	pxParser->xBufferTime.ulSecondsSince2000 = 0;
	pxParser->xBufferTime.usSecondsFraction  = 0;
}

/*-----------------------------------------------------------*/

eModuleError_t eTdfParse( xTdfParser_t *pxParser, xTdf_t *pxTdf )
{
	uint8_t  ucByte = 0x00;
	uint16_t usTdfIdTimestamp, usTdf, usTimestampType, usTemp;
	uint8_t  ucTdfLen;
	if ( pxParser->ulCurrentOffset >= pxParser->ulBufferLen ) {
		return ERROR_INVALID_DATA;
	}
	/* Loop until we find a non-empty byte that indicates the start of a TDF */
	while ( pxParser->ulCurrentOffset < pxParser->ulBufferLen ) {
		ucByte = pxParser->pucBuffer[pxParser->ulCurrentOffset];
		if ( ( ucByte != 0x00 ) && ( ucByte != 0xFF ) ) {
			break;
		}
		pxParser->ulCurrentOffset++;
	}
	/* No valid start byte was found */
	if ( ( ucByte == 0x00 ) || ( ucByte == 0xFF ) ) {
		return ERROR_INVALID_DATA;
	}
	/* Extract the TDF ID */
	usTdfIdTimestamp = LE_U16_EXTRACT( pxParser->pucBuffer + pxParser->ulCurrentOffset );
	usTdf			 = TDF_ID_MASK & usTdfIdTimestamp;
	usTimestampType  = TDF_TIMESTAMP_MASK & usTdfIdTimestamp;
	ucTdfLen		 = 2 + pucTdfStructLengths[usTdf];
	switch ( usTimestampType ) {
		case TDF_TIMESTAMP_NONE:
			break;
		case TDF_TIMESTAMP_GLOBAL:
			/* We check later if these values are actually out of range of the buffer, in which case pxParser is already invalid */
			pxParser->xBufferTime.ulSecondsSince2000 = LE_U32_EXTRACT( pxParser->pucBuffer + pxParser->ulCurrentOffset + 2 );
			pxParser->xBufferTime.usSecondsFraction  = LE_U16_EXTRACT( pxParser->pucBuffer + pxParser->ulCurrentOffset + 6 );
			ucTdfLen += 6;
			break;
		case TDF_TIMESTAMP_RELATIVE_OFFSET_S:
			pxParser->xBufferTime.ulSecondsSince2000 += LE_U16_EXTRACT( pxParser->pucBuffer + pxParser->ulCurrentOffset + 2 );
			ucTdfLen += 2;
			break;
		case TDF_TIMESTAMP_RELATIVE_OFFSET_MS:
			/* Convert MS to seconds fractions */
			usTemp = ( (uint32_t) LE_U16_EXTRACT( pxParser->pucBuffer + pxParser->ulCurrentOffset + 2 ) );
			/* Check for fractional second overflow */
			if ( (uint32_t) pxParser->xBufferTime.usSecondsFraction + (uint32_t) usTemp > 0xFFFF ) {
				pxParser->xBufferTime.ulSecondsSince2000++;
			}
			pxParser->xBufferTime.usSecondsFraction += usTemp;
			ucTdfLen += 2;
			break;
		default:
			return ERROR_INVALID_DATA;
	}
	/* Validate TDF fits in remaining buffer */
	if ( pxParser->ulCurrentOffset + ucTdfLen > pxParser->ulBufferLen ) {
		return ERROR_INVALID_DATA;
	}
	pxParser->ulCurrentOffset += ucTdfLen;
	/* Fill out the TDF parameters */
	pxTdf->usId						= usTdfIdTimestamp;
	pxTdf->pucData					= pxParser->pucBuffer + pxParser->ulCurrentOffset - pucTdfStructLengths[usTdf];
	pxTdf->ucDataLen				= pucTdfStructLengths[usTdf];
	pxTdf->xTime.ulSecondsSince2000 = pxParser->xBufferTime.ulSecondsSince2000;
	pxTdf->xTime.usSecondsFraction  = pxParser->xBufferTime.usSecondsFraction;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
