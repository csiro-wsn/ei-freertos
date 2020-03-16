/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "logger_mirror.h"

#include "memory_operations.h"
#include "tdf.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

#define COPY_BUFFER_SIZE 512

/* Type Definitions -----------------------------------------*/

typedef struct xMissingPageTdf_t
{
	uint16_t usTdfId;
	uint32_t ulMissingPage;
} ATTR_PACKED xMissingPageTdf_t;

/* Function Declarations ------------------------------------*/

eModuleError_t prvMirrorSameSizes( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages );
eModuleError_t prvMirrorSrcLarger( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages );
eModuleError_t prvMirrorDstLarger( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages );

/* Private Variables ----------------------------------------*/

uint8_t pucCopyBuffer[COPY_BUFFER_SIZE];

/*-----------------------------------------------------------*/

uint32_t ulLogMirrorLostSrcPages( uint32_t ulPagesWritten, uint32_t ulNumPages, uint8_t ucEraseUnit )
{
	/* Pages are only lost once the logger has completely filled one time */
	if ( ulPagesWritten >= ulNumPages ) {
		return ulPagesWritten - ulNumPages + ucEraseUnit - ( ulPagesWritten % ucEraseUnit );
	}
	return 0;
}

/*-----------------------------------------------------------*/

eModuleError_t eLogMirror( xLogger_t *pxSrc, xLogger_t *pxDst )
{
	uint32_t ulTotalLostPages;
	uint8_t  ucEraseUnit;

	configASSERT( pxSrc->usLogicalBlockSize <= COPY_BUFFER_SIZE );
	configASSERT( pxDst->usLogicalBlockSize <= COPY_BUFFER_SIZE );
	/* Verifies that one page size is a multiple of the other */
	configASSERT( ( pxSrc->usLogicalBlockSize % pxDst->usLogicalBlockSize ) == 0 ||
				  ( pxDst->usLogicalBlockSize % pxSrc->usLogicalBlockSize ) == 0 );
	/** 
	 * Mirroring currently does not support destination loggers with wrapping enabled
	 * Doesn't make much sense as data will be being lost, which is the problem this module is solving
	 **/
	configASSERT( ( pxDst->ucFlags & LOGGER_FLAG_WRAPPING_ON ) == 0 );

	/* Query Required Data */
	eLoggerConfigure( pxSrc, LOGGER_CONFIG_GET_ERASE_UNIT, &ucEraseUnit );
	ulTotalLostPages = ulLogMirrorLostSrcPages( pxSrc->ulPagesWritten, pxSrc->ulNumBlocks, ucEraseUnit );

	/* Initialise the copy buffer to the destination clear byte */
	pvMemset( pucCopyBuffer, pxDst->ucClearByte, sizeof( uint8_t ) * COPY_BUFFER_SIZE );

	/* Based on relative logger page sizes, perform different operations */
	if ( pxSrc->usLogicalBlockSize > pxDst->usLogicalBlockSize ) {
		return prvMirrorSrcLarger( pxSrc, pxDst, ulTotalLostPages );
	}
	else if ( pxDst->usLogicalBlockSize > pxSrc->usLogicalBlockSize ) {
		return prvMirrorDstLarger( pxSrc, pxDst, ulTotalLostPages );
	}
	else {
		return prvMirrorSameSizes( pxSrc, pxDst, ulTotalLostPages );
	}
}

/*-----------------------------------------------------------*/

eModuleError_t prvMirrorSameSizes( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages )
{
	uint32_t ulActualLostPages = ulLostSrcPages - pxDst->ulPagesWritten;
	uint32_t ulSrcOffset	   = pxSrc->ucFlags & LOGGER_FLAG_WRAPPING_ON ? 1 : 0;
	uint32_t ulIterCount, ulReadPage;
	uint32_t i;

	/* Log missing pages */
	xMissingPageTdf_t xTdf = { .usTdfId = TDF_LOST_DATA | TDF_TIMESTAMP_NONE, .ulMissingPage = pxSrc->ulPagesWritten - ulActualLostPages };
	for ( i = 0; i < ulActualLostPages; i++ ) {
		eLoggerLog( pxDst, 6, (uint8_t *) &xTdf );
		eLoggerCommit( pxDst );
		xTdf.ulMissingPage++;
	}
	/* Copy across existing data */
	ulIterCount = pxSrc->ulPagesWritten - pxDst->ulPagesWritten;
	ulReadPage  = ( pxSrc->ulPagesWritten - ulActualLostPages ) % pxSrc->ulNumBlocks;
	for ( i = 0; i < ulIterCount; i++ ) {
		eLoggerReadBlock( pxSrc, ulReadPage, ulSrcOffset, pucCopyBuffer );
		eLoggerLog( pxDst, pxDst->usLogicalBlockSize, pucCopyBuffer );
		ulReadPage = ( ulReadPage + 1 ) % pxSrc->ulNumBlocks;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 *  Mirroring from a large logger to a small logger should not be used when the logger contains TDF Data
 *  Quite likely that the TDF's on a single large page will not split nicely across two small pages
 *  TDF's cannot span multiple pages
 *  The 'easy' solution is to run the read page through tdf_parse and just relog the TDF's to the new logger
 * 		This is not only slow, but also destroys our nice maths for working out how much data needs to copied across
 *  Alternate solution is to add another parameter to decoding tools to indicate that data can span pages
 *  Not required at the moment, so ignoring
 **/
eModuleError_t prvMirrorSrcLarger( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages )
{
	uint32_t ulSizeRatio		  = pxSrc->usLogicalBlockSize / pxDst->usLogicalBlockSize;
	uint32_t ulEquivalentSrcPages = pxDst->ulPagesWritten / ulSizeRatio;
	uint32_t ulActualLostPages	= 0;
	uint32_t ulSrcOffset		  = pxSrc->ucFlags & LOGGER_FLAG_WRAPPING_ON ? 1 : 0;
	uint32_t ulIterCount, ulReadPage;
	uint32_t i, j;

	if ( ulLostSrcPages && ( ulLostSrcPages > ulEquivalentSrcPages + pxSrc->ulNumBlocks ) ) {
		ulActualLostPages = ulLostSrcPages - ulEquivalentSrcPages;
	}

	eLog( LOG_LOGGER, LOG_WARNING, "Mirroring should not be used for TDF Data\r\n" );
	/* Log missing pages */
	xMissingPageTdf_t xTdf = { .usTdfId = TDF_LOST_DATA | TDF_TIMESTAMP_NONE, .ulMissingPage = pxSrc->ulPagesWritten - ulActualLostPages };
	for ( i = 0; i < ulActualLostPages; i++ ) {
		for ( j = 0; j < ulSizeRatio; j++ ) {
			eLoggerLog( pxDst, 6, (uint8_t *) &xTdf );
			eLoggerCommit( pxDst );
		}
		xTdf.ulMissingPage++;
	}
	/* Copy across existing data */
	ulIterCount = pxSrc->ulPagesWritten - ( pxDst->ulPagesWritten / ulSizeRatio );
	ulReadPage  = ( pxSrc->ulCurrentBlockAddress - ulIterCount ) % pxSrc->ulNumBlocks;
	for ( i = 0; i < ulIterCount; i++ ) {
		eLoggerReadBlock( pxSrc, ulReadPage, ulSrcOffset, pucCopyBuffer );
		for ( j = 0; j < ulSizeRatio; j++ ) {
			eLoggerLog( pxDst, pxDst->usLogicalBlockSize, pucCopyBuffer + ( j * pxDst->usLogicalBlockSize ) );
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t prvMirrorDstLarger( xLogger_t *pxSrc, xLogger_t *pxDst, uint32_t ulLostSrcPages )
{
	uint32_t ulSizeRatio		  = pxDst->usLogicalBlockSize / pxSrc->usLogicalBlockSize;
	uint32_t ulEquivalentSrcPages = pxDst->ulPagesWritten * ulSizeRatio;
	uint32_t ulActualLostPages	= 0;
	uint32_t ulSrcOffset		  = pxSrc->ucFlags & LOGGER_FLAG_WRAPPING_ON ? 1 : 0;
	uint32_t ulIterCount, ulReadPage;
	uint16_t usPageOffset;
	uint32_t i, j;

	if ( ulLostSrcPages && ( ulLostSrcPages > ulEquivalentSrcPages + pxSrc->ulNumBlocks ) ) {
		ulActualLostPages = ulLostSrcPages - ulEquivalentSrcPages;
	}

	/* Log missing pages */
	xMissingPageTdf_t xTdf = { .usTdfId = TDF_LOST_DATA | TDF_TIMESTAMP_NONE, .ulMissingPage = pxDst->ulPagesWritten * ulSizeRatio };
	for ( i = 0; i < ulActualLostPages / ulSizeRatio; i++ ) {
		for ( j = 0; j < ulSizeRatio; j++ ) {
			eLoggerLog( pxDst, 6, (uint8_t *) &xTdf );
			xTdf.ulMissingPage++;
		}
		eLoggerCommit( pxDst );
	}
	/* Copy across existing data */
	ulIterCount = ( pxSrc->ulPagesWritten / ulSizeRatio ) - pxDst->ulPagesWritten;
	ulReadPage  = ( pxSrc->ulCurrentBlockAddress - ulSizeRatio * ulIterCount ) % pxSrc->ulNumBlocks;
	for ( i = 0; i < ulIterCount; i++ ) {
		for ( j = 0; j < ulSizeRatio; j++ ) {
			usPageOffset = j * ( pxSrc->usLogicalBlockSize - ulSrcOffset );
			// fprintf( stderr, "STL %d %d %d\r\n", j, ulReadPage, usPageOffset );
			eLoggerReadBlock( pxSrc, ulReadPage, ulSrcOffset, pucCopyBuffer + usPageOffset );
			ulReadPage = ( ulReadPage + 1 ) % pxSrc->ulNumBlocks;
		}
		eLoggerLog( pxDst, pxDst->usLogicalBlockSize, pucCopyBuffer );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
