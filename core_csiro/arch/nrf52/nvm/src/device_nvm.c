/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "bluetooth.h"

#include "device_constants.h"
#include "device_nvm.h"
#include "fds.h"
#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define NVM_FILE_ID 					0x1111
#define NVM_MIN_FREE_SPACE				60

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eNvmCommand_t {
	NVM_INIT,
	NVM_READ,
	NVM_WRITE,
	NVM_ERASE_ALL,
	NVM_ERASE_KEY
} eNvmCommand_t;

typedef struct xNvmAction_t
{
	eNvmCommand_t   eCommand;
	eNvmKey_t		eKey;
	TaskHandle_t	xReturnTask;
	eModuleError_t *peError;
	uint32_t		ulDataLength;
	void *			pvData;
} xNvmAction_t;

/* Function Declarations ------------------------------------*/

static void			  prvNvmTask( void *pvParameters );
static eModuleError_t prvNvmExecuteAction( xNvmAction_t *pxAction, TickType_t xTimeout );

/* Private Variables ----------------------------------------*/

STATIC_TASK_STRUCTURES( pxNvmHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 2 );
STATIC_QUEUE_STRUCTURES( pxNvmQueue, sizeof( xNvmAction_t ), 1 );

extern const uint32_t ulKeyLengthWords[];

/** 
 * Create an unused variable which represents the storage the fds will use so we can account
 * for that space. FDS driver just uses addresses with no checks for available space.
 */
#include "fds_internal_defs.h"
uint8_t pucFdsStorageArea[FDS_PHY_PAGES * FDS_PHY_PAGE_SIZE * sizeof( uint32_t )] ATTR_SECTION( ".nvm" );

/*-----------------------------------------------------------*/

eModuleError_t eNvmInit( void )
{
	STATIC_QUEUE_CREATE( pxNvmQueue );
	STATIC_TASK_CREATE( pxNvmHandle, prvNvmTask, "NVM", NULL );
	uint32_t	   ulKey;
	eModuleError_t eError, eValidEndTime;

	xNvmAction_t xAction = {
		.eCommand	 = NVM_INIT,
		.eKey		  = 0x0,
		.ulDataLength = 0x0,
		.pvData		  = NULL
	};
	eError = prvNvmExecuteAction( &xAction, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	/* Check that our valid key is set */
	eError = eNvmReadData( NVM_KEY, &ulKey );
	if ( ( eError != ERROR_NONE ) || ( ulKey != ulApplicationNvmValidKey ) ) {
		eLog( LOG_NVM, LOG_ERROR, "NVM: Key=0x%X Error=0x%X\r\n", ulKey, eError );
		/* Special case save the device decommisioning time */
		uint32_t ulEndTime = 0;
		eValidEndTime = eNvmReadData( NVM_DEVICE_END_TIME, &ulEndTime );
		/* Erase the current NVM, can't trust its values */
		if ( eNvmEraseData() != ERROR_NONE ) {
			return ERROR_INITIALISATION_FAILURE;
		}
		/* Save the valid key */
		ulKey = ulApplicationNvmValidKey;
		if ( eNvmWriteData( NVM_KEY, &ulKey ) != ERROR_NONE ) {
			return ERROR_INITIALISATION_FAILURE;
		}
		/* Save the end time, if it existed previously */
		if (eValidEndTime == ERROR_NONE) {
			if ( eNvmWriteData( NVM_DEVICE_END_TIME, &ulEndTime ) != ERROR_NONE ) {
				return ERROR_INITIALISATION_FAILURE;
			}
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmEraseData( void )
{
	xNvmAction_t xAction = {
		.eCommand	 = NVM_ERASE_ALL,
		.eKey		  = 0x0,
		.ulDataLength = 0x0,
		.pvData		  = NULL
	};
	return prvNvmExecuteAction( &xAction, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmEraseKey( eNvmKey_t eKey )
{
	xNvmAction_t xAction = {
		.eCommand	 = NVM_ERASE_KEY,
		.eKey		  = eKey,
		.ulDataLength = 0x0,
		.pvData		  = NULL,
	};
	return prvNvmExecuteAction( &xAction, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmWriteData( eNvmKey_t eKey, void *pvData )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];

	xNvmAction_t xAction = {
		.eCommand = NVM_WRITE,
		.eKey	 = eKey + 1, /* Record keys should be in the range 0x0001 - 0xBFFF */
		.pvData   = pvData,
	};

	if ( ( ulDataLen == NVM_COUNTER_VARIABLE ) || ( ulDataLen == NVM_BOOLEAN_VARIABLE ) ) {
		xAction.ulDataLength = 1;
	}
	else {
		xAction.ulDataLength = ulDataLen;
	}

	return prvNvmExecuteAction( &xAction, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmReadData( eNvmKey_t eKey, void *pvData )
{
	const uint32_t ulDataLen = ulKeyLengthWords[eKey];

	xNvmAction_t xAction = {
		.eCommand = NVM_READ,
		.eKey	 = eKey + 1, /* Record keys should be in the range 0x0001 - 0xBFFF */
		.pvData   = pvData,
	};

	if ( ( ulDataLen == NVM_COUNTER_VARIABLE ) || ( ulDataLen == NVM_BOOLEAN_VARIABLE ) ) {
		xAction.ulDataLength = 1;
	}
	else {
		xAction.ulDataLength = ulDataLen;
	}

	return prvNvmExecuteAction( &xAction, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmIncrementData( eNvmKey_t eKey, uint32_t *pulNewData )
{
	uint32_t		ulDataLen = ulKeyLengthWords[eKey];
	eModuleError_t  eError;
	static uint32_t ulCounterValue;

	if ( ulDataLen != NVM_COUNTER_VARIABLE ) {
		return ERROR_INVALID_ADDRESS;
	}

	eError		= eNvmReadData( eKey, &ulCounterValue );
	*pulNewData = ( eError == ERROR_INVALID_ADDRESS ) ? 0x01 : ulCounterValue + 1;
	return eNvmWriteData( eKey, pulNewData );
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
	uint32_t	   ulDataLen = ulKeyLengthWords[eKey];
	eModuleError_t eError;
	uint32_t	   ulDefault = 0;
	uint32_t	   ulValue;
	if ( ulDataLen != NVM_BOOLEAN_VARIABLE ) {
		return ERROR_INVALID_ADDRESS;
	}
	eError   = eNvmReadDataDefault( eKey, &ulValue, &ulDefault );
	*pbState = !!( ulValue );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eNvmWriteFlag( eNvmKey_t eKey, bool bSet )
{
	uint32_t ulDataLen = ulKeyLengthWords[eKey];
	uint32_t ulValue   = bSet ? 0x01 : 0x00;
	if ( ulDataLen != NVM_BOOLEAN_VARIABLE ) {
		return ERROR_INVALID_ADDRESS;
	}
	return eNvmWriteData( eKey, &ulValue );
}

/*-----------------------------------------------------------*/
/* Private Functions */
/*-----------------------------------------------------------*/

static void prvFdsEvtHandler( fds_evt_t const *p_evt )
{
	eLog( LOG_NVM, LOG_VERBOSE, "NVM Result: EVENT=%d RESULT=%d\r\n", p_evt->id, p_evt->result );
	/* xTaskNotify doesn't play nicely with sending 0's, so we offset by 1 */
	xTaskNotify( pxNvmHandle, p_evt->result + 1, eSetValueWithOverwrite );
}

/*-----------------------------------------------------------*/

eModuleError_t prvFdsInit( void )
{
	ret_code_t eRet;

	eLog( LOG_NVM, LOG_VERBOSE, "NVM Init: Starting\r\n" );

	if ( fds_init() != FDS_SUCCESS ) {
		eLog( LOG_NVM, LOG_APOCALYPSE, "NVM: Failed to start initialisation\r\n" );
		return ERROR_FLASH_OPERATION_FAIL;
	}
	eRet = (ret_code_t) ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) - 1;
	if ( eRet != FDS_SUCCESS ) {
		eLog( LOG_NVM, LOG_APOCALYPSE, "NVM: Failed to initialise with error %d\r\n", eRet );
	}
	return ( eRet == FDS_SUCCESS ) ? ERROR_NONE : ERROR_FLASH_OPERATION_FAIL;
}

/*-----------------------------------------------------------*/

eModuleError_t prvFdsWrite( xNvmAction_t *pxParameters )
{
	ret_code_t		  eRet;
	fds_record_t	  xDeviceRecord;
	fds_record_desc_t xDeviceDesc;
	fds_find_token_t  xToken = { 0 };

	xDeviceRecord = ( fds_record_t ){
		.file_id		   = NVM_FILE_ID,
		.key			   = pxParameters->eKey,
		.data.p_data	   = pxParameters->pvData,
		.data.length_words = pxParameters->ulDataLength,
	};
	/* FDS writes require word aligned data */
	configASSERT( IS_ALIGNED( pxParameters->pvData, 4 ) );

	/* Find if record exits */
	eRet = fds_record_find( NVM_FILE_ID, pxParameters->eKey, &xDeviceDesc, &xToken );

	if ( eRet == FDS_SUCCESS ) {
		eLog( LOG_NVM, LOG_VERBOSE, "NVM Updating key %d\r\n", pxParameters->eKey - 1 );
		eRet = fds_record_update( &xDeviceDesc, &xDeviceRecord );
	}
	else {
		eLog( LOG_NVM, LOG_VERBOSE, "NVM Writing key %d\r\n", pxParameters->eKey - 1 );
		eRet = fds_record_write( &xDeviceDesc, &xDeviceRecord );
	}

	/* Catch Write Error */
	if ( eRet != FDS_SUCCESS ) {
		return ERROR_FLASH_OPERATION_FAIL;
	}

	eRet = (ret_code_t) ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) - 1;
	return ( eRet == FDS_SUCCESS ) ? ERROR_NONE : ERROR_FLASH_OPERATION_FAIL;
}

/*-----------------------------------------------------------*/

eModuleError_t prvFdsRead( xNvmAction_t *pxParameters )
{
	ret_code_t		   eRet;
	fds_record_desc_t  xRecordDesc = { 0 };
	fds_find_token_t   xToken	  = { 0 };
	fds_flash_record_t config	  = { 0 };

	eRet = fds_record_find( NVM_FILE_ID, pxParameters->eKey, &xRecordDesc, &xToken );

	if ( eRet != FDS_SUCCESS ) {
		eLog( LOG_NVM, LOG_VERBOSE, "NVM Key %d doesn't exist\r\n", pxParameters->eKey - 1 );
		return ERROR_INVALID_ADDRESS;
	}

	eRet = fds_record_open( &xRecordDesc, &config );
	eLog( LOG_NVM, LOG_VERBOSE, "NVM Key %d loaded 0x%08X\r\n", pxParameters->eKey - 1, *(uint32_t *) config.p_data );

	pvMemcpy( pxParameters->pvData, (void *) config.p_data, sizeof( uint32_t ) * pxParameters->ulDataLength );

	/* Close the record when done reading. */
	eRet = fds_record_close( &xRecordDesc );

	if ( eRet != FDS_SUCCESS ) {
		return ERROR_FLASH_OPERATION_FAIL;
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t prvFdsErase( void )
{
	ret_code_t eRet;

	if ( fds_file_delete( NVM_FILE_ID ) != FDS_SUCCESS ) {
		return ERROR_FLASH_OPERATION_FAIL;
	}

	eRet = (ret_code_t) ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) - 1;
	return ( eRet == FDS_SUCCESS ) ? ERROR_NONE : ERROR_FLASH_OPERATION_FAIL;
}

/*-----------------------------------------------------------*/

eModuleError_t prvFdsEraseKey( eNvmKey_t eKey )
{
	ret_code_t		  eRet;
	fds_record_desc_t desc = { 0 };
	fds_find_token_t  ftok = { 0 };

	eRet = fds_record_find( NVM_FILE_ID, eKey, &desc, &ftok );

	if ( eRet != FDS_SUCCESS ) {
		return ERROR_INVALID_ADDRESS;
	}

	if ( fds_record_delete( &desc ) != FDS_SUCCESS ) {
		return ERROR_FLASH_OPERATION_FAIL;
	}

	eRet = (ret_code_t) ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) - 1;
	return ( eRet == FDS_SUCCESS ) ? ERROR_NONE : ERROR_FLASH_OPERATION_FAIL;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvNvmExecuteAction( xNvmAction_t *pxAction, TickType_t xTimeout )
{
	eModuleError_t eError = ERROR_NONE;

	pxAction->peError	 = &eError;
	pxAction->xReturnTask = xTaskGetCurrentTaskHandle();

	/* Send our command to the task */
	if ( xQueueSendToBack( pxNvmQueue, pxAction, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	/* Wait until the task signifies it is done */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	return eError;
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvNvmTask( void *pvParameters )
{

	UNUSED( pvParameters );
	xNvmAction_t	xAction;
	eModuleError_t  eError = ERROR_NONE;
	eBluetoothPhy_t ePHY;
	ret_code_t		eRet;
	fds_stat_t		fStat = { 0 };

	bool bWriteCommand;

	eRet = fds_register( prvFdsEvtHandler );

	if ( eRet != FDS_SUCCESS ) {
		configASSERT( 0 );
	}

	for ( ;; ) {

		if ( xQueueReceive( pxNvmQueue, &xAction, portMAX_DELAY ) != pdPASS ) {
			configASSERT( 0 );
		}
		/* All Commands other than NVM_READ are writing to the flash */
		bWriteCommand = ( xAction.eCommand != NVM_READ );

		if ( bWriteCommand ) {
			/* Flash won't write while scanning */
			eBluetoothScanStop( &ePHY );
		}
		eLog( LOG_NVM, LOG_INFO, "NVM Command: %d\r\n", xAction.eCommand );

		switch ( xAction.eCommand ) {
			case NVM_INIT:
				eError = prvFdsInit();
				break;
			case NVM_READ:
				eError = prvFdsRead( &xAction );
				break;
			case NVM_WRITE:
				eError = prvFdsWrite( &xAction );
				break;
			case NVM_ERASE_ALL:
				eError = prvFdsErase();
				break;
			case NVM_ERASE_KEY:
				eError = prvFdsEraseKey( xAction.eKey );
				break;
			default:
				configASSERT( 0 );
				break;
		}
		/* Perform post write operations */
		if ( bWriteCommand ) {
			/* Resume the scanning */
			if ( ePHY != 0 ) {
				eBluetoothScanStart( ePHY );
			}
		}
		/* Send the response off before doing slow garbage collection */
		*xAction.peError = eError;
		xTaskNotifyGive( xAction.xReturnTask );

		/* Perform garbage collection if needed */
		if ( bWriteCommand ) {
			fds_stat( &fStat );
			if ( fStat.largest_contig <= NVM_MIN_FREE_SPACE ) {
				eLog( LOG_NVM, LOG_INFO, "NVM: Running GC\r\n" );
				eBluetoothScanStop( &ePHY );
				fds_gc();
				ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
				if ( ePHY != 0 ) {
					eBluetoothScanStart( ePHY );
				}
			}
		}
	}
}

/*-----------------------------------------------------------*/
