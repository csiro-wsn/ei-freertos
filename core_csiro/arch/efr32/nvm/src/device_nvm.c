/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "device_nvm.h"

#include "compiler_intrinsics.h"
#include "nvm3.h"

#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define NVM3_DEFAULT_CACHE_SIZE         20
#define NVM3_DEFAULT_NVM_SIZE           4 * FLASH_PAGE_SIZE
#define NVM3_DEFAULT_MAX_OBJECT_SIZE    NVM3_MAX_OBJECT_SIZE_LOW_LIMIT
#define NVM3_DEFAULT_REPACK_HEADROOM    0

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* Parameter names below this point are required for linking correctly */
// clang-format off
// clang-format on

uint8_t nvm3Storage[NVM3_DEFAULT_NVM_SIZE] ATTR_SECTION( ".nvm3" );

static nvm3_CacheEntry_t defaultCache[NVM3_DEFAULT_CACHE_SIZE];

nvm3_Handle_t  nvm3_defaultHandleData;
nvm3_Handle_t *nvm3_defaultHandle = &nvm3_defaultHandleData;

nvm3_Init_t nvm3_defaultInitData = {
	.nvmAdr			 = (nvm3_HalPtr_t) &nvm3Storage,
	.nvmSize		 = NVM3_DEFAULT_NVM_SIZE,
	.cachePtr		 = defaultCache,
	.cacheEntryCount = NVM3_DEFAULT_CACHE_SIZE,
	.maxObjectSize   = NVM3_DEFAULT_MAX_OBJECT_SIZE,
	.repackHeadroom  = NVM3_DEFAULT_REPACK_HEADROOM
};

nvm3_Init_t *nvm3_defaultInit = &nvm3_defaultInitData;

extern const uint32_t ulKeyLengthWords[];

// COMPILER_WARNING_ENABLE()

/*-----------------------------------------------------------*/

eModuleError_t eNvmInit( void )
{
	uint32_t ulKey;
	Ecode_t  eError;

	/* Open the NVM module */
	/* Will return OK if already open but initialisation parameters the same as successful open */
	eError = nvm3_open( nvm3_defaultHandle, nvm3_defaultInit );
	if ( eError != ECODE_NVM3_OK ) {
		eLog( LOG_NVM, LOG_APOCALYPSE, "NVM: Open failed with error code 0x%X\r\n", eError );
		return ERROR_INITIALISATION_FAILURE;
	}

	/* Log how many objects we have stored */
	eLog( LOG_NVM, LOG_INFO, "NVM Number Objects: %d\r\n", nvm3_countObjects( nvm3_defaultHandle ) );

	/* Check that our valid key is set */
	eError = eNvmReadData( NVM_KEY, &ulKey );
	if ( ( eError != ERROR_NONE ) || ( ulKey != ulApplicationNvmValidKey ) ) {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Key=0x%X Error=0x%X\r\n", ulKey, eError );
		/* Erase the current NVM, can't trust its values */
		if ( eNvmEraseData() != ERROR_NONE ) {
			return ERROR_INITIALISATION_FAILURE;
		}
		/* Save the valid key */
		ulKey = ulApplicationNvmValidKey;
		if ( eNvmWriteData( NVM_KEY, &ulKey ) != ERROR_NONE ) {
			return ERROR_INITIALISATION_FAILURE;
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmEraseData( void )
{
	Ecode_t eError;
	eLog( LOG_NVM, LOG_DEBUG, "NVM: Erasing ALL data %d\r\n" );
	eError = nvm3_eraseAll( nvm3_defaultHandle );
	if ( eError != ECODE_OK ) {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Erase failed with code 0x%X\r\n", eError );
		return ERROR_FLASH_OPERATION_FAIL;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmEraseKey( eNvmKey_t eKey )
{
	Ecode_t eError;
	eError = nvm3_deleteObject( nvm3_defaultHandle, eKey );
	return ( eError == ECODE_OK ) ? ERROR_NONE : ERROR_FLASH_OPERATION_FAIL;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmWriteData( eNvmKey_t eKey, void *pvData )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];
	Ecode_t		   eError;

	eLog( LOG_NVM, LOG_DEBUG, "NVM: Saving key %d\r\n", eKey );
	/* Requires different functions dependant on if value is a counter or not */
	if ( ulDataLen == NVM_COUNTER_VARIABLE ) {
		eError = nvm3_writeCounter( nvm3_defaultHandle, eKey, *(uint32_t *) pvData );
	}
	else {
		eError = nvm3_writeData( nvm3_defaultHandle, eKey, pvData, sizeof( uint32_t ) * ulDataLen );
	}
	if ( eError != ECODE_OK ) {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Write failed with code 0x%x\r\n", eError );
		return ERROR_FLASH_OPERATION_FAIL;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmIncrementData( eNvmKey_t eKey, uint32_t *pulNewData )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];
	uint32_t	   ulCounterValue;
	Ecode_t		   eError;

	eLog( LOG_NVM, LOG_DEBUG, "NVM: Incrementing key %d\r\n", eKey );

	/* Function is only valid on counter variables */
	if ( ulDataLen != NVM_COUNTER_VARIABLE ) {
		return ERROR_INVALID_ADDRESS;
	}
	/* Validate that data associated with the key exists */
	eError = nvm3_readCounter( nvm3_defaultHandle, eKey, &ulCounterValue );
	if ( eError == ECODE_OK ) {
		/* Increment the data */
		eError = nvm3_incrementCounter( nvm3_defaultHandle, eKey, pulNewData );
		if ( eError != ECODE_OK ) {
			eLog( LOG_NVM, LOG_ERROR, "NVM: Failed to increment key %d\r\n", eKey );
			return ERROR_FLASH_OPERATION_FAIL;
		}
	}
	/* If the key doesn't exist */
	else if ( eError == ECODE_NVM3_ERR_KEY_NOT_FOUND ) {
		/* Create the counter with a value of 0 */
		*pulNewData = 0;
		eError		= eNvmWriteData( eKey, pulNewData );
		if ( eError != ECODE_OK ) {
			eLog( LOG_NVM, LOG_ERROR, "NVM: Failed to set counter %d to 0\r\n", eKey );
			return ERROR_FLASH_OPERATION_FAIL;
		}
	}
	/* Reading the counter reu*/
	else {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Failed to reaad  %d\r\n", eKey );
		return ERROR_FLASH_OPERATION_FAIL;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmReadData( eNvmKey_t eKey, void *pvData )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];
	Ecode_t		   eError;

	eLog( LOG_NVM, LOG_DEBUG, "NVM: Loading key %d\r\n", eKey );
	if ( ulDataLen == NVM_COUNTER_VARIABLE ) {
		eError = nvm3_readCounter( nvm3_defaultHandle, eKey, (uint32_t *) pvData );
	}
	else {
		eError = nvm3_readData( nvm3_defaultHandle, eKey, pvData, sizeof( uint32_t ) * ulDataLen );
	}
	if ( eError == ECODE_NVM3_ERR_KEY_NOT_FOUND ) {
		eLog( LOG_NVM, LOG_DEBUG, "NVM: Key data does not exist\r\n" );
		return ERROR_INVALID_ADDRESS;
	}
	else if ( eError != ECODE_OK ) {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Read failed with code 0x%x\r\n", eError );
		return ERROR_FLASH_OPERATION_FAIL;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmReadDataDefault( eNvmKey_t eKey, void *pvData, void *pvDefault )
{
	eModuleError_t eError;
	eLog( LOG_NVM, LOG_DEBUG, "NVM: Loading key with fallback %d\r\n", eKey );
	/* Try and load the data associated with eKey */
	eError = eNvmReadData( eKey, pvData );
	/* If the key doesn't exist */
	if ( eError == ERROR_INVALID_ADDRESS ) {
		/* Save the default data into eKey */
		eError = eNvmWriteData( eKey, pvDefault );
		if ( eError != ERROR_NONE ) {
			return ERROR_FLASH_OPERATION_FAIL;
		}
		/* Validate that the key now exists */
		eError = eNvmReadData( eKey, pvData );
		if ( eError != ERROR_NONE ) {
			return ERROR_FLASH_OPERATION_FAIL;
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmReadFlag( eNvmKey_t eKey, bool *pbState )
{
	eModuleError_t eError;
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];
	if ( ulDataLen != NVM_BOOLEAN_VARIABLE ) {
		*pbState = false;
		return ERROR_INVALID_ADDRESS;
	}
	eError   = eNvmReadData( eKey, NULL );
	*pbState = ( eError == ERROR_NONE );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmWriteFlag( eNvmKey_t eKey, bool bSet )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];
	bool		   bCurrent  = false;
	eModuleError_t eError	= ERROR_NONE;
	/* Validate key type */
	if ( ulDataLen != NVM_BOOLEAN_VARIABLE ) {
		return ERROR_INVALID_ADDRESS;
	}
	/* Get the current value */
	eNvmReadFlag( eKey, &bCurrent );
	/* We only need to do something if our value is changing */
	eLog( LOG_NVM, LOG_DEBUG, "NVM Update Flag: Old=%d New=%d\r\n", bCurrent, bSet );
	if ( bSet != bCurrent ) {
		if ( bSet ) {
			eError = eNvmWriteData( eKey, NULL );
		}
		else {
			eError = eNvmEraseKey( eKey );
		}
	}
	return eError;
}

/*-----------------------------------------------------------*/
