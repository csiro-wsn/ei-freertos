/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stddef.h>

#include "log.h"
#include "logger.h"
#include "memory_operations.h"
#include "tdf.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define TDF_RELATIVE_TIMESTAMP_SIZE		2
#define TDF_GLOBAL_TIMESTAMP_SIZE		sizeof( xTdfTime_t )

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvClearBufferTime( xTdfLogger_t *pxTdfLog );
bool bCanUseARelativeTimestamp( xTdfLogger_t *pxTdfLog, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, int32_t lTimeDifferenceInSeconds, int32_t lTimeDifferenceInFractions );

/* Private Variables ----------------------------------------*/

extern struct xTdfLogger_t *const tdf_logs[];
extern uint8_t					  TDF_LOGGER_NUM;

/* Functions ------------------------------------------------*/

xTdfLogger_t *pxTdfLoggerGet( uint8_t ucLoggerMask )
{
	extern xTdfLogger_t xNullLog;
	xTdfLogger_t *		pxRet = &xNullLog;
	uint8_t				ucMask;
	for ( uint8_t i = 0; i < ( TDF_LOGGER_NUM - 1 ); i++ ) {
		ucMask = ( tdf_logs[i]->pxLog )->ucUniqueMask;
		if ( ucLoggerMask == ucMask ) {
			pxRet = tdf_logs[i];
			break;
		}
	}
	return pxRet;
}

/*-----------------------------------------------------------*/

/**
 * eTDFLoggerConfigure
 *
 * Used to configure certain settings in the TDF Logger. Can also be used to
 * configure values in the underlying Logger.
 */
eModuleError_t eTdfLoggerConfigure( xTdfLogger_t *pxTdfLog, int iSetting, void *pvConfValue )
{
	if ( iSetting == LOGGER_CONFIG_INIT_DEVICE ) {
		pxTdfLog->xTdfSemaphore = xSemaphoreCreateRecursiveMutex();
	}

	if ( iSetting == TDF_LOGGER_CONFIG_CHECK_TIME_NOT_BEFORE ) {
		/**
		 * Set a 'valid after' time. If we try to log anything before this time, it returns an error.
		 * pvConfValue is assumed to be a pointer to a xTdfTime_t.
		 **/
		pxTdfLog->xValidAfterTime.ulSecondsSince2000 = ( (xTdfTime_t *) pvConfValue )->ulSecondsSince2000;
		return ERROR_NONE;
	}
	else {
		return eLoggerConfigure( pxTdfLog->pxLog, iSetting, pvConfValue );
	}
}

/*-----------------------------------------------------------*/

/**
 * eTdfFlush
 *
 * Simply calls eLoggerCommit which commits the current logger block to memory.
 * Needed so that consecutive logger add's to make a single TDF message all
 * stay in the same block.
 */
eModuleError_t eTdfFlush( xTdfLogger_t *pxTdflog )
{
	eModuleError_t eError;

	xSemaphoreTakeRecursive( pxTdflog->xTdfSemaphore, portMAX_DELAY );
	eLog( LOG_LOGGER, LOG_VERBOSE, "TDF: eTdfFlush called\r\n" );

	prvClearBufferTime( pxTdflog );

	/* Commit the TDF data log to the physical device */
	eError = eLoggerCommit( pxTdflog->pxLog );
	xSemaphoreGiveRecursive( pxTdflog->xTdfSemaphore );

	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eTdfFlushMulti( uint8_t ucLoggerMask )
{
	uint8_t		   i = 0;
	uint8_t		   ucMask;
	eModuleError_t eError, eReturn;
	eReturn = ERROR_NONE;
	for ( i = 0; i < ( TDF_LOGGER_NUM - 1 ); i++ ) {
		ucMask = ( tdf_logs[i]->pxLog )->ucUniqueMask;
		if ( ucLoggerMask & ucMask ) {
			eError = eTdfFlush( (xTdfLogger_t *) tdf_logs[i] );
			if ( eError != ERROR_NONE ) {
				eReturn = eError;
			}
		}
	}
	return eReturn;
}

/*-----------------------------------------------------------*/

uint16_t usTdfAddToBuffer( eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxTimestamp, void *pucTdfData, uint8_t ucBufferSize, void *pvBuffer )
{
	configASSERT( eTimestampType != TDF_TIMESTAMP_RELATIVE_OFFSET_MS );
	configASSERT( eTimestampType != TDF_TIMESTAMP_RELATIVE_OFFSET_S );

	/* Assert there is enough room in the supplied buffer for the TDF. */
	configASSERT( ucBufferSize >= pucTdfStructLengths[eTdfId] + ( eTimestampType == TDF_TIMESTAMP_NONE ? 2 : 8 ) );

	uint8_t  ucDataOffset = 2;
	uint16_t usTdfId	  = eTdfId;
	if ( eTimestampType == TDF_TIMESTAMP_GLOBAL ) {
		usTdfId |= TDF_TIMESTAMP_GLOBAL;
	}

	pvMemcpy( pvBuffer, &usTdfId, 2 );
	if ( eTimestampType == TDF_TIMESTAMP_GLOBAL ) {
		pvMemcpy( pvBuffer + ucDataOffset, pxTimestamp, sizeof( xTdfTime_t ) );
		ucDataOffset += sizeof( xTdfTime_t );
	}
	pvMemcpy( pvBuffer + ucDataOffset, pucTdfData, pucTdfStructLengths[eTdfId] );
	return ucDataOffset + pucTdfStructLengths[eTdfId];
}

/*-----------------------------------------------------------*/

/**
 * eTDFAdd
 *
 * Does the job of formatting the timestamp, the TDF ID, and the data so that
 * it fits the TDF3 standard. Once that's done, it passes the data to the
 * Logger layer.
 * 
 * Note: Do not call this from an ISR. It contains the use of non-ISR
 * semaphores so using it in an IRS will break things.
 * 
 * Note: usTdfId contains the timestamp type.
 */
eModuleError_t eTdfAdd( xTdfLogger_t *pxTdfLog, eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, void *pucData )
{
	int32_t		   lTimeDifferenceInSeconds   = 0;			// Contains the time difference in seconds. Used for calculations.
	int32_t		   lTimeDifferenceInFractions = 0;			// Contains the time difference in fractions of a second. Used for calculations.
	uint16_t	   usTimeDifference			  = 0;			// We only have 2 bytes to stuff the time into, so this is the final variable which will contain the time difference.
	uint8_t		   ucNumHeaderBytes			  = 2;			// The number of bytes used for the TDF header. (Header = id + timestamp).
	eModuleError_t eError					  = ERROR_NONE; // The return value.
	bool		   bClearBufferTimeFlag		  = false;		// Set to true when we want to clear the buffer time.

	xSemaphoreTakeRecursive( pxTdfLog->xTdfSemaphore, portMAX_DELAY );

	if ( eTimestampType != TDF_TIMESTAMP_NONE ) {

		/* Check the supplied global timestamp is valid */
		if ( ( pxGlobalTime == NULL ) || ( pxGlobalTime->ulSecondsSince2000 < pxTdfLog->xValidAfterTime.ulSecondsSince2000 ) ) {
			xSemaphoreGiveRecursive( pxTdfLog->xTdfSemaphore );
			return ERROR_INVALID_TIME;
		}

		/* If we are using a global timestamp */
		if ( eTimestampType == TDF_TIMESTAMP_GLOBAL ) {
			ucNumHeaderBytes += TDF_GLOBAL_TIMESTAMP_SIZE;
		}

		/* If we are using a relative timestamp */
		else {
			// Calculate time differences
			lTimeDifferenceInSeconds   = pxGlobalTime->ulSecondsSince2000 - pxTdfLog->xBufferTime.ulSecondsSince2000;
			lTimeDifferenceInFractions = ( ( lTimeDifferenceInSeconds * 65536 ) +
										   ( pxGlobalTime->usSecondsFraction - pxTdfLog->xBufferTime.usSecondsFraction ) );

			if ( bCanUseARelativeTimestamp( pxTdfLog, eTimestampType, pxGlobalTime, lTimeDifferenceInSeconds, lTimeDifferenceInFractions ) ) {
				/* Set the TDF timestamp to be in fractions or seconds */
				if ( eTimestampType == TDF_TIMESTAMP_RELATIVE_OFFSET_S ) {
					usTimeDifference = (uint16_t) lTimeDifferenceInSeconds;
				}
				else {
					usTimeDifference = (uint16_t) lTimeDifferenceInFractions;
				}

				/* Increment the number of header bytes by the size of a relative timestamp */
				ucNumHeaderBytes += TDF_RELATIVE_TIMESTAMP_SIZE;
			}
			else /* Not allowed to use a relative timestamp, so set timestamp to global */
			{
				eTimestampType = TDF_TIMESTAMP_GLOBAL;
				ucNumHeaderBytes += TDF_GLOBAL_TIMESTAMP_SIZE;
			}
		}
	}

	/* Assert the TDF we are logging actually fits in the logger */
	uint8_t ucTdfSize = ucNumHeaderBytes + pucTdfStructLengths[eTdfId];
	configASSERT( ucTdfSize < pxTdfLog->pxLog->usLogicalBlockSize );

	/**
	 * Now comes the second part of TDF add. We know how long our TDF is going to be,
	 * and we need to guarentee that the TDF is going to be stored together in the same
	 * block in the device logger. We can't have the header of the TDF in one block,
	 * and the data in another. If the block is a radio packet, times would get wacky.
	 * 
	 * So here we manually check if the next full tdf will overflow the logger buffer.
	 * If it will, we flush the buffer before writing our tdf to the logger.
	 */

	uint16_t usBytesRemainingInLog = pxTdfLog->pxLog->usLogicalBlockSize - pxTdfLog->pxLog->usBufferByteOffset;
	eLog( LOG_LOGGER, LOG_VERBOSE, "TDF: Required Space = %d Remaining Space = %d\r\n", ucTdfSize, usBytesRemainingInLog );
	if ( ucTdfSize > usBytesRemainingInLog ) {
		eError = eTdfFlush( pxTdfLog );
		if ( eError != ERROR_NONE ) {
			xSemaphoreGiveRecursive( pxTdfLog->xTdfSemaphore );
			return eError;
		}

		/* As long as we aren't logging TDF_TIMESTAMP_NONE, change timestamp to global. */
		if ( eTimestampType != TDF_TIMESTAMP_NONE ) {
			eTimestampType = TDF_TIMESTAMP_GLOBAL;
		}
	}

	if ( ucTdfSize == usBytesRemainingInLog ) {
		bClearBufferTimeFlag = true;
	}

	/**
	 * Now we simply log the tdf to the logger in three parts. First we log the ID,
	 * then the timestamp if it's requested, and finally the data.
	 * 
	 * I only check the first return value here, because if we can't commit the
	 * first few bytes properly, the whole page is ruined anyway.
	 */

	uint16_t usTdfId = eTdfId | eTimestampType;

	/* Log the TDF_ID */
	eError = eLoggerLog( pxTdfLog->pxLog, 2, &usTdfId );

	/* Log the Time Stamp */
	if ( eTimestampType == TDF_TIMESTAMP_GLOBAL ) { // If it's a global timestamp.
		eLoggerLog( pxTdfLog->pxLog, sizeof( xTdfTime_t ), pxGlobalTime );
		/* Update the current global time for this logger block */
		pxTdfLog->xBufferTime.ulSecondsSince2000 = pxGlobalTime->ulSecondsSince2000;
		pxTdfLog->xBufferTime.usSecondsFraction  = pxGlobalTime->usSecondsFraction;
	}
	else if ( eTimestampType != TDF_TIMESTAMP_NONE ) { // If it's a relative timestamp.
		eLoggerLog( pxTdfLog->pxLog, 2, &usTimeDifference );
	}

	/* Log the Sensor Data */
	eLoggerLog( pxTdfLog->pxLog, pucTdfStructLengths[eTdfId], pucData );

	if ( bClearBufferTimeFlag ) {
		prvClearBufferTime( pxTdfLog );
	}

	xSemaphoreGiveRecursive( pxTdfLog->xTdfSemaphore );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eTdfAddMulti( uint8_t ucLoggerMask, eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, void *pucData )
{
	eModuleError_t eError, eReturn;
	uint8_t		   ucMask;

	eReturn = ERROR_NONE;

	for ( uint8_t i = 0; i < ( TDF_LOGGER_NUM - 1 ); i++ ) {
		ucMask = ( tdf_logs[i]->pxLog )->ucUniqueMask;
		if ( ucLoggerMask & ucMask ) {
			eLog( LOG_LOGGER, LOG_INFO, "TDF Multi: log%d mask=%d id=%d\r\n", i, ucMask, eTdfId );
			eError = eTdfAdd( (xTdfLogger_t *) tdf_logs[i], eTdfId, eTimestampType, pxGlobalTime, pucData );
			if ( eError != ERROR_NONE ) {
				eReturn = eError;
			}
		}
	}

	return eReturn;
}

/*-----------------------------------------------------------*/

eModuleError_t eTdfSchedulerArgsParse( xTdfLoggerMapping_t *pxLoggerMapping, uint8_t ucTdfMask, eTdfIds_t eTdfId, xTdfTime_t *pxGlobalTime, void *pucData )
{
	eModuleError_t eError = ERROR_NONE;

	if ( pxLoggerMapping->xActivityTdfsMask & ucTdfMask ) {
		/* Get byte with only bits preceeding this mask */
		uint8_t ucCountMask = pxLoggerMapping->xActivityTdfsMask & ( ucTdfMask - 1 );

		/* Count the number of preceeding bits. This tells us which loggers mask index to use */
		uint32_t ulBitcount = COUNT_ONES( ucCountMask );

		/* Get the loggers mask struct */
		xTdfLoggersMask_t xLoggersMask = pxLoggerMapping->pxTdfLoggersMask[ulBitcount];

		/* Check if we want a timestamp. If not then use timestamp none */
		eTdfTimestampType_t eTimestampType = TDF_TIMESTAMP_NONE;
		if ( xLoggersMask.timestamp ) {
			eTimestampType = TDF_TIMESTAMP_RELATIVE_OFFSET_MS;
		}

		/* Add the TDF */
		eError = eTdfAddMulti( xLoggersMask.loggers_mask, eTdfId, eTimestampType, pxGlobalTime, pucData );
	}

	return eError;
}

/*-----------------------------------------------------------*/

/**
 * bWeAreAllowedToUseARelativeTimestamp
 *
 * Returns whether or not we are allowed to use a relative timestamp.
 * 
 * This is a super long if statement so I'm going to explain it in this comment:
 * I will enumerate the conditions for you.
 * 
 * 1: If there is an existing global time for this logger block,
 * 2: and the new time is in the future,
 * 3: and the time difference fits in 2 bytes, (The requirement for relative timestamps.)
 * 4: and if we want to log the relative time in ms, the time difference in seconds has
 * 		to be less than less than or equal to one. This is because if the time difference
 * 		if greater than one second, you can't represent it in fractions of a second with 
 * 		2 bytes. (because the fractions of a second are 1/65536ths of a second, and
 * 		65536 = 2^16).
 * 5: and also if we want to use a timestamp in relative ms, then the difference in ms
 * 		must also be smaller than 65536 (2^16) and not smaller than 0.
 */
bool bCanUseARelativeTimestamp( xTdfLogger_t *pxTdfLog, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, int32_t lTimeDifferenceInSeconds, int32_t lTimeDifferenceInFractions )
{
	return ( ( pxTdfLog->xBufferTime.ulSecondsSince2000 != TDF_INVALID_TIME ) &&
			 ( pxGlobalTime->ulSecondsSince2000 >= pxTdfLog->xBufferTime.ulSecondsSince2000 ) &&
			 ( lTimeDifferenceInSeconds <= (int) UINT16_MAX ) &&
			 !( ( eTimestampType == TDF_TIMESTAMP_RELATIVE_OFFSET_MS ) && ( lTimeDifferenceInSeconds > 1 ) ) &&
			 !( ( eTimestampType == TDF_TIMESTAMP_RELATIVE_OFFSET_MS ) && ( ( lTimeDifferenceInFractions > (int) UINT16_MAX ) || ( lTimeDifferenceInFractions < 0 ) ) ) );
}

/*-----------------------------------------------------------*/

void prvClearBufferTime( xTdfLogger_t *pxTdfLog )
{
	/* Reset the stored timestamp for this buffer */
	pxTdfLog->xBufferTime.ulSecondsSince2000 = TDF_INVALID_TIME;
	pxTdfLog->xBufferTime.usSecondsFraction  = 0;
}

/*-----------------------------------------------------------*/
