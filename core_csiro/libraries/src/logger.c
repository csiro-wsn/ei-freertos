/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "log.h"
#include "logger.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

/* Functions ------------------------------------------------*/

/**
 * eLoggerLog - This function logs data to the logger buffer.
 *
 * Takes a data buffer and a date length. It copies that length of data from
 * the supplied buffer into the logger buffer. If this overflows the buffer it
 * commits the current buffer and starts writing to the next buffer.
 */
eModuleError_t eLoggerLog( xLogger_t *pxLog, uint16_t usNumBytes, void *pvLogData )
{
	uint16_t	   usMaxSize;
	eModuleError_t eError = ERROR_NONE;
	uint8_t *	  pucWriteDataPointer;

	eLog( LOG_LOGGER, LOG_VERBOSE, "eLoggerLog: FLAGS:%02X BLOCK:%lu ByteOffset:%d / %d\r\n",
		  pxLog->ucFlags, pxLog->ulCurrentBlockAddress, pxLog->usBufferByteOffset, pxLog->usLogicalBlockSize );

	// If data to log is too large to fit in the logger. (The allowable data size shrinks by one if wrapping is on.)
	usMaxSize = ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) ? pxLog->usLogicalBlockSize - 1 : pxLog->usLogicalBlockSize;
	if ( usNumBytes > usMaxSize ) {
		return ERROR_DATA_TOO_LARGE;
	}

	// Will the new data fit on this page? If not, then commit the current buffer
	if ( usNumBytes > ( pxLog->usLogicalBlockSize - pxLog->usBufferByteOffset ) ) {
		// Write current_buffer and switch the internal buffer to the other one
		eError = eLoggerCommit( pxLog );

		if ( eError != ERROR_NONE ) {
			return eError;
		}
	}

	// Copy data from pucLogBuffer to the logger buffer, depending on the current buffer.
	if ( pxLog->ucCurrentBuffer ) {
		pucWriteDataPointer = &( pxLog->pucBuffer[pxLog->usBufferByteOffset + pxLog->usLogicalBlockSize] );
	}
	else {
		pucWriteDataPointer = &( pxLog->pucBuffer[pxLog->usBufferByteOffset] );
	}
	pvMemcpy( pucWriteDataPointer, pvLogData, (uint32_t) usNumBytes );
	pxLog->usBufferByteOffset += usNumBytes;

	/* If the buffer is currently full, send it off for commiting */
	if ( pxLog->usBufferByteOffset == pxLog->usLogicalBlockSize ) {
		eError = eLoggerCommit( pxLog );
	}

	return eError;
}

/*---------------------------------------------------------------------------*/

/**
 * eLoggerCommit - Commits data to the device logger.
 *
 * When the logger fills up, it needs to dump it's current buffer to the logger
 * device. It does this by calling the function pointer defined by that
 * specific device. It also checks a whole bunch of different scenarios to
 * ensure that data is committed correctly.
 */
eModuleError_t eLoggerCommit( xLogger_t *pxLog )
{
	eModuleError_t eError = ERROR_NONE;
	uint8_t		   ucWrappingEnabled;
	uint8_t *	  pucWriteDataAddress;

	eLog( LOG_LOGGER, LOG_VERBOSE, "eLoggerCommit: FLAGS:%02X BLOCK:%lu ByteOffset:%d\r\n",
		  pxLog->ucFlags, pxLog->ulCurrentBlockAddress, pxLog->usBufferByteOffset );

	/** 
	 * Make sure that the buffer we are committing actually contains data.
	*  Basically if wrapping is one, the next block will already contain
	*  the wrap number, but the block is still technically empty.
	**/
	ucWrappingEnabled = ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) != 0; // Will be one or zero.
	if ( pxLog->usBufferByteOffset > ucWrappingEnabled ) {
		/* If current logical block address is larger than the total number of logical blocks in this logger. */
		if ( pxLog->ulCurrentBlockAddress > pxLog->ulNumBlocks - 1 )
			return ERROR_DEVICE_FULL;

		/* Optionally write the 'unused byte' to all remaining unused bytes in the buffer. */
		if ( pxLog->ucFlags & LOGGER_FLAG_CLEAR_UNUSED_BYTES ) {
			pucWriteDataAddress = &( pxLog->pucBuffer[( pxLog->ucCurrentBuffer ? pxLog->usLogicalBlockSize : 0 ) + pxLog->usBufferByteOffset] );
			pvMemset( pucWriteDataAddress, pxLog->ucClearByte, pxLog->usLogicalBlockSize - pxLog->usBufferByteOffset );
		}

		uint32_t ulBlockNum  = pxLog->ulStartBlockAddress + pxLog->ulCurrentBlockAddress;
		uint8_t *pucData	 = &pxLog->pucBuffer[pxLog->ucCurrentBuffer ? pxLog->usLogicalBlockSize : 0];
		uint32_t ulBlockSize = ( ( pxLog->ucFlags & LOGGER_FLAG_COMMIT_ONLY_USED_BYTES ) ? pxLog->usBufferByteOffset : pxLog->usLogicalBlockSize );

		eLog( LOG_LOGGER, LOG_INFO, "Logger TX: length = %i\r\n", ulBlockSize );
		eError = pxLog->pxLoggerDevice->fnWriteBlock( ulBlockNum, pucData, ulBlockSize );

		/* If the fnWriteBlock command works */
		if ( eError == ERROR_NONE ) {
			/* Switch the buffer to the empty one */
			pxLog->ucCurrentBuffer = !pxLog->ucCurrentBuffer;
			/* Increment internal state */
			pxLog->ulCurrentBlockAddress++;
			pxLog->ulPagesWritten++;
			/* If the device is full and wrapping is enabled, reset back to block 0 of the device */
			if ( ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) && ( pxLog->ulCurrentBlockAddress >= pxLog->ulNumBlocks ) ) {
				pxLog->ulCurrentBlockAddress = 0;
				pxLog->ucWrapCounter++;
			}
		}
		/* In wrap mode prepare the next block for writing */
		if ( ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) && ( pxLog->ulCurrentBlockAddress < pxLog->ulNumBlocks ) ) {
			pxLog->pxLoggerDevice->fnPrepareBlock( pxLog->ulCurrentBlockAddress );
		}

		pxLog->usBufferByteOffset = 0;
		/* If wrapping is on, set the first byte in the new buffer to the wrap number */
		if ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) {
			pxLog->pucBuffer[pxLog->ucCurrentBuffer ? pxLog->usLogicalBlockSize : 0] = PHYSICAL_WRAP_NUMBER( pxLog );
			pxLog->usBufferByteOffset												 = 1;
		}
	}

	return eError;
}

/*---------------------------------------------------------------------------*/

/**
 * eLoggerConfigure - Configures the logger.
 *
 * Used to initialise the logger and also configure a number of different
 * settings in the logger. 
 */
eModuleError_t eLoggerConfigure( xLogger_t *pxLog, uint16_t usSetting, void *pvConfValue )
{
	eModuleError_t eError = ERROR_NONE;
	uint32_t	   ulDeviceLength;
	uint8_t		   usMatchBuffer;

	uint8_t *pucCurrentBuffer = &( pxLog->pucBuffer[( pxLog->ucCurrentBuffer ) ? pxLog->usLogicalBlockSize : 0] );
	uint8_t *pucUnusedBuffer  = &( pxLog->pucBuffer[( !pxLog->ucCurrentBuffer ) ? pxLog->usLogicalBlockSize : 0] );

	switch ( usSetting ) {
		case LOGGER_CONFIG_INIT_DEVICE:
			/* Logger length should be automatically configured */
			if ( pxLog->ulNumBlocks == LOGGER_LENGTH_REMAINING_BLOCKS ) {
				/* Query the device for its total length */
				eLoggerConfigure( pxLog, LOGGER_CONFIG_GET_NUM_BLOCKS, &ulDeviceLength );
				pxLog->ulNumBlocks = ulDeviceLength - pxLog->ulStartBlockAddress;
			}
			/* Query the device for the erase byte */
			eLoggerConfigure( pxLog, LOGGER_CONFIG_GET_CLEAR_BYTE, &pxLog->ucClearByte );
			/* Setup initial flags */
			pxLog->ucFlags = LOGGER_FLAG_DEVICE_INITIALISED | LOGGER_FLAG_CLEAR_UNUSED_BYTES;
			/* Setup internal variables */
			pxLog->ulPagesWritten		 = 0;
			pxLog->ulCurrentBlockAddress = 0;
			pxLog->usBufferByteOffset	= 0;
			pxLog->ucCurrentBuffer		 = 0;
			pxLog->ucWrapCounter		 = 0;
			break;
		case LOGGER_CONFIG_CLEAR_UNUSED_BYTES:
			// Sets the 'clear unused values' flag and sets conf value.
			// Note: LOGGER_FLAG_COMMIT_ONLY_USED_BYTES cannot be used with LOGGER_FLAG_CLEAR_UNUSED_BYTES
			pxLog->ucFlags |= LOGGER_FLAG_CLEAR_UNUSED_BYTES;
			pxLog->ucFlags &= ~LOGGER_FLAG_COMMIT_ONLY_USED_BYTES;
			break;

		case LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES:
			// Note: LOGGER_FLAG_CLEAR_UNUSED_BYTES cannot be used with LOGGER_FLAG_COMMIT_ONLY_USED_BYTES
			pxLog->ucFlags |= LOGGER_FLAG_COMMIT_ONLY_USED_BYTES;
			pxLog->ucFlags &= ~LOGGER_FLAG_CLEAR_UNUSED_BYTES;
			break;

		case LOGGER_CONFIG_WRAP_MODE:
			pxLog->ucFlags |= LOGGER_FLAG_WRAPPING_ON;
			/* Get the current wrap number */
			eLoggerReadBlock( pxLog, 0, 0, pucUnusedBuffer );
			pxLog->ucWrapCounter	 = LOGICAL_WRAP_NUMBER( pxLog, pucUnusedBuffer[0] );
			uint8_t ucCompletedWraps = pxLog->ucWrapCounter;
			/* Only need to scan if the first byte does not match the erase byte */
			if ( pucUnusedBuffer[0] != pxLog->ucClearByte ) {
				usMatchBuffer = pucUnusedBuffer[0];
				eLog( LOG_LOGGER, LOG_INFO, "LOGGER_CONFIG: wrap/match:%02X\r\n", usMatchBuffer );
				eLoggerSearch( pxLog, 1, &usMatchBuffer, LOGGER_SEARCH_BINARY_SEARCH | LOGGER_SEARCH_NOT_MATCH, &pxLog->ulCurrentBlockAddress );
				/* If all the pages have the same wrap number, we are back at the start of the device again */
				if ( pxLog->ulCurrentBlockAddress >= pxLog->ulNumBlocks ) {
					pxLog->ucWrapCounter++;
					pxLog->ulCurrentBlockAddress = 0;
				}
			}
			else {
				pxLog->ucWrapCounter		 = 0;
				pxLog->ulCurrentBlockAddress = 0;
				ucCompletedWraps			 = 0;
			}
			pxLog->usBufferByteOffset = 1;							   // reset the byte offset
			pucCurrentBuffer[0]		  = PHYSICAL_WRAP_NUMBER( pxLog ); // set first byte to num_wraps
			pxLog->ulPagesWritten	 = ( ucCompletedWraps * pxLog->ulNumBlocks ) + pxLog->ulCurrentBlockAddress;
			/* Prepare the first block for writing */
			pxLog->pxLoggerDevice->fnPrepareBlock( pxLog->ulCurrentBlockAddress );
			break;
		case LOGGER_CONFIG_APPEND_MODE:
			usMatchBuffer = pxLog->ucClearByte;
			eLog( LOG_LOGGER, LOG_INFO, "LOGGER_CONFIG: match:%02X\r\n", usMatchBuffer );
			eLoggerSearch( pxLog, 1, &usMatchBuffer, LOGGER_SEARCH_BINARY_SEARCH, &pxLog->ulCurrentBlockAddress );
			pxLog->ulPagesWritten = pxLog->ulCurrentBlockAddress;
			break;
		default:
			eError = pxLog->pxLoggerDevice->fnConfigure( (uint32_t) usSetting, pvConfValue );
	}

	return eError;
}

/*---------------------------------------------------------------------------*/

/**
 * eLoggerStatus - Returns the status of the logger.
 *
 * You give this function the log and the type of status you want, and this
 * gives the answer.
 */
eModuleError_t eLoggerStatus( xLogger_t *pxLog, uint16_t usType, void *pvStatus )
{
	switch ( usType ) {
		case LOGGER_STATUS_BLOCKS_WRITTEN:
			*( (uint32_t *) pvStatus ) = pxLog->ulPagesWritten;
			break;
		case LOGGER_STATUS_NUM_BLOCKS:
			*( (uint32_t *) pvStatus ) = pxLog->ulNumBlocks;
			break;
		case LOGGER_STATUS_WRAP_COUNT:
			*( (uint8_t *) pvStatus ) = pxLog->ucWrapCounter;
			break;
		case LOGGER_STATUS_DEVICE_STATUS:
			/* TODO: Make this do something logical... */
			*( (uint32_t *) pvStatus ) = pxLog->pxLoggerDevice->fnStatus( 0 );
			break;
		default:
			return ERROR_DEFAULT_CASE;
	}
	return ERROR_NONE;
}

/*---------------------------------------------------------------------------*/

/**
 * eLoggerReadBlock - Reads a block from the logger device.
 *
 * Reads a logical block of data from the log at the specified BlockNum.
 * A BlockNum of 0 will be the first page of the logger. On underlying hardware
 * which support reading, loggers can read from them.
 */
eModuleError_t eLoggerReadBlock( xLogger_t *pxLog, uint32_t ulBlockNum, uint16_t usBlockOffset, void *pvBlockData )
{
	eLog( LOG_LOGGER, LOG_INFO, "Logger Read Block Num: %lu\r\n", ulBlockNum );
	if ( ulBlockNum >= pxLog->ulNumBlocks ) {
		return ERROR_INVALID_ADDRESS;
	}
	return pxLog->pxLoggerDevice->fnReadBlock( pxLog->ulStartBlockAddress + ulBlockNum, usBlockOffset, pvBlockData, pxLog->usLogicalBlockSize - usBlockOffset );
}

/*-----------------------------------------------------------*/

/**
 * eLoggerSearch - Searches a logger to find matching data.
 *
 * Finds the first occurance that matches / doesn't match the specified bytes.
 * Used interally in eLoggerConfigure.
 * 
 * pxLog is the logger.
 * usNumBytes is the number of bytes that need to match. (The length of pucMatchdata which is checked.)
 * pucMatchData is a pointer to a string of information which needs to match.
 * ucSearchFlags are flags which set how the search is performed.
 * pulPageNum is a pointer to the page number. It is changed by this search algorithm.
 */
eModuleError_t eLoggerSearch( xLogger_t *pxLog, uint16_t usNumBytes, uint8_t *pucMatchData, uint8_t ucSearchFlags, uint32_t *pulPageNum )
{
	/* Use the idle buffer for reading in blocks */
	uint8_t *pucIdleBuffer = &pxLog->pucBuffer[( !pxLog->ucCurrentBuffer ) ? pxLog->usLogicalBlockSize : 0];
	uint32_t ulLow		   = 0;
	uint32_t ulHigh		   = pxLog->ulNumBlocks - 1;
	uint32_t ulMidpoint	= 0;
	uint8_t  ucMatch	   = 0;
	uint16_t i;

	// Start searching.
	while ( ulLow <= ulHigh ) {
		// If we are binary searching.
		if ( ucSearchFlags & LOGGER_SEARCH_BINARY_SEARCH ) {
			ulMidpoint = ulLow + ( ulHigh - ulLow ) / 2;
		}
		else {
			ulMidpoint = ulLow;
		}

		// Use the other buffer to read the page.
		eLoggerReadBlock( pxLog, ulMidpoint, 0, pucIdleBuffer );

		// Scan through bytes checking if they match.
		ucMatch = 2;
		for ( i = 0; i < usNumBytes; i++ ) {
			eLog( LOG_LOGGER, LOG_INFO, "SEARCH: comp %u : %u\r\n", pucIdleBuffer[i], pucMatchData[i] );
			if ( pucIdleBuffer[i] != pucMatchData[i] ) {
				ucMatch = 0;
			}
		}

		// If match isn't found.
		if ( ucSearchFlags & LOGGER_SEARCH_NOT_MATCH ) {
			ucMatch = !ucMatch * 2;
		}

		eLog( LOG_LOGGER, LOG_INFO, "SEARCH: match:%u\r\n", ucMatch );

		// If we are binary searching and have fully converged, or we aren't binary searching and we have just already matched.
		if ( ( ( ( ulHigh - ulMidpoint ) == 0 ) && ( ( ulMidpoint - ulLow ) <= 1 ) ) ||
			 ( !( ucSearchFlags & LOGGER_SEARCH_BINARY_SEARCH ) && ucMatch ) ) {
			*pulPageNum = ulMidpoint + ( ucMatch ? 0 : 1 );
			eLog( LOG_LOGGER, LOG_INFO, "SEARCH: Match Found\r\n" );
			return ERROR_NONE; // Match Found
		}

		// Move our midpoint max, min, and midpoint.
		if ( 1 < ucMatch ) {
			ulHigh = ulMidpoint ? ( ulMidpoint - 1 ) : 0;
		}
		else {
			ulLow = ulMidpoint + 1;
		}
	}
	/* No idea how this code can be reached */
	*pulPageNum = ulMidpoint + ( ucMatch ? 0 : 1 );
	return ERROR_NONE; // Match Found
}

/*-----------------------------------------------------------*/

void vLoggerPrint( xLogger_t *pxLog, SerialLog_t eLogger, LogLevel_t eLevel )
{
	const char *pucOn  = "Enabled";
	const char *pucOff = "Disabled";

	eLog( eLogger, eLevel, "Logger: %s\r\n"
						   "\tStatic Block Info\r\n"
						   "\t\tSize  : %d\r\n"
						   "\t\tStart : %d\r\n"
						   "\t\tNumber: %d\r\n"
						   "\t\tErase : 0x%02X\r\n",
		  pxLog->pucDescription,
		  pxLog->usLogicalBlockSize,
		  pxLog->ulStartBlockAddress,
		  pxLog->ulNumBlocks,
		  pxLog->ucClearByte );

	eLog( eLogger, eLevel, "\tDynamic Block Info\r\n"
						   "\t\tWritten: %d\r\n"
						   "\t\tWraps  : %d\r\n"
						   "\t\tCurrent: %d\r\n"
						   "\t\tOffset : %d\r\n",
		  pxLog->ulPagesWritten,
		  pxLog->ucWrapCounter,
		  pxLog->ulCurrentBlockAddress,
		  pxLog->usBufferByteOffset );

	eLog( eLogger, eLevel, "\tOptions\r\n"
						   "\t\tWrap        : %s\r\n"
						   "\t\tOnly Used   : %s\r\n"
						   "\t\tClear Unused: %s\r\n",
		  ( pxLog->ucFlags & LOGGER_FLAG_WRAPPING_ON ) ? pucOn : pucOff,
		  ( pxLog->ucFlags & LOGGER_FLAG_COMMIT_ONLY_USED_BYTES ) ? pucOn : pucOff,
		  ( pxLog->ucFlags & LOGGER_FLAG_CLEAR_UNUSED_BYTES ) ? pucOn : pucOff );
}

/*-----------------------------------------------------------*/
