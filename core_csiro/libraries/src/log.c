/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "board.h"
#include "log.h"
#include "tiny_printf.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static inline bool bIsValidLog( SerialLog_t eLog );
static inline bool bIsValidLogLevel( LogLevel_t eLevel );

/* Private Variables ----------------------------------------*/

static LogLevel_t xLoggerLevels[LOG_MODULE_LAST + 1] = { [0 ... LOG_MODULE_LAST] = LOG_ERROR };

/*-----------------------------------------------------------*/

static inline bool bIsValidLog( SerialLog_t eLog )
{
	return ( eLog < LOG_MODULE_LAST );
}

/*-----------------------------------------------------------*/

static inline bool bIsValidLogLevel( LogLevel_t eLevel )
{
	return ( eLevel < LOG_LEVEL_LAST );
}

/*-----------------------------------------------------------*/

void vLogResetLogLevels( void )
{
	uint8_t i;
	for ( i = 0; i < LOG_MODULE_LAST; i++ ) {
		xLoggerLevels[i] = LOG_ERROR;
	}
}

/*-----------------------------------------------------------*/

LogLevel_t xLogGetLogLevel( SerialLog_t eLog )
{
	if ( bIsValidLog( eLog ) ) {
		return xLoggerLevels[eLog];
	}
	else {
		return ERROR_INVALID_LOG_LEVEL;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eLogSetLogLevel( SerialLog_t eLog, LogLevel_t eLevel )
{
	if ( !bIsValidLog( eLog ) ) {
		return ERROR_INVALID_LOGGER;
	}
	if ( !bIsValidLogLevel( eLevel ) ) {
		return ERROR_INVALID_LOG_LEVEL;
	}
	xLoggerLevels[eLog] = eLevel;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eLog( SerialLog_t eLog, LogLevel_t eLevel, const char *pcFormat, ... )
{
	eModuleError_t eError = ERROR_NONE;
	va_list		   va;
	/* Check for valid logger parameters */
	if ( !bIsValidLog( eLog ) ) {
		return ERROR_INVALID_LOGGER;
	}
	if ( !bIsValidLogLevel( eLevel ) ) {
		return ERROR_INVALID_LOG_LEVEL;
	}
	if ( eLevel <= xLoggerLevels[eLog] ) {
		va_start( va, pcFormat );
		eError = pxSerialOutput->pxImplementation->fnWrite( pxSerialOutput->pvContext, pcFormat, va );
		va_end( va );
	}
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eLogBuilderStart( xLogBuilder_t *pxBuilder, SerialLog_t eLog )
{
	/* Check for valid logger parameters */
	if ( !bIsValidLog( eLog ) ) {
		return ERROR_INVALID_LOGGER;
	}
	pxBuilder->eLog		= eLog;
	pxBuilder->ulIndex  = 0;
	pxBuilder->pcString = pxSerialOutput->pxImplementation->fnClaimBuffer( pxSerialOutput->pvContext, &pxBuilder->ulMaxLen );
	pxBuilder->bValid   = ( pxBuilder->pcString != NULL );
	return pxBuilder->bValid ? ERROR_NONE : ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

eModuleError_t eLogBuilderPush( xLogBuilder_t *pxBuilder, LogLevel_t eLevel, const char *format, ... )
{
	va_list va;
	if ( !pxBuilder->bValid ) {
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	if ( !bIsValidLogLevel( eLevel ) ) {
		return ERROR_INVALID_LOG_LEVEL;
	}
	if ( eLevel <= xLoggerLevels[pxBuilder->eLog] ) {
		va_start( va, format );
		pxBuilder->ulIndex += tiny_vsnprintf( (char *) pxBuilder->pcString + pxBuilder->ulIndex, pxBuilder->ulMaxLen - pxBuilder->ulIndex, format, va );
		va_end( va );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eLogBuilderFinish( xLogBuilder_t *pxBuilder )
{
	if ( !pxBuilder->bValid ) {
		pxSerialOutput->pxImplementation->fnReleaseBuffer( pxSerialOutput->pvContext, pxBuilder->pcString );
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	/* Only queue up the buffer for transmission if data is actually there */
	if ( pxBuilder->ulIndex > 0 ) {
		pxSerialOutput->pxImplementation->fnSendBuffer( pxSerialOutput->pvContext, pxBuilder->pcString, pxBuilder->ulIndex );
	}
	else {
		pxSerialOutput->pxImplementation->fnReleaseBuffer( pxSerialOutput->pvContext, pxBuilder->pcString );
	}
	pxBuilder->bValid = false;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
