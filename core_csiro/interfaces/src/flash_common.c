/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "flash_interface.h"

#include "board.h"
#include "compiler_intrinsics.h"
#include "crc.h"
#include "log.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eCommand_t {
	FLASH_READ,
	FLASH_WRITE,
	FLASH_ERASE_PAGES,
	FLASH_ERASE_ALL,
	FLASH_CRC,
	FLASH_ROM_STORE,
	FLASH_ROM_STORE_DELTAS,
	FLASH_ROM_START_READ
} eCommand_t;

typedef struct xFlashAction_t
{
	eCommand_t		eCommand;		 /**< Command to run */
	uint64_t		ullFlashAddress; /**< Starting byte address in Flash Memory */
	uint8_t *		pucArg1;		 /**< Primary argument pointer ( read/write buffers or ROM address ) */
	uint8_t *		pucArg2;		 /**< Secondary arguments */
	uint8_t *		pucArg3;		 /**< Ternary arguments */
	uint32_t		ulLength;		 /**< Size of the operation, typically in bytes, but can also be a count */
	TaskHandle_t	xResponseTask;   /**< Task to push the response at */
	eModuleError_t *peResult;		 /**< Location to store the response */
} xFlashAction_t;

typedef eModuleError_t ( *fnFlashOperation_t )( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext );

/* Function Declarations ------------------------------------*/

void prvFlashInterfaceTask( void *pvParameters );

eModuleError_t prvFlashExecuteAction( QueueHandle_t xQueue, xFlashAction_t *pxAction, TickType_t xTimeout );
static void	prvFlashWaitAction( xFlashDevice_t *pxDevice, xFlashAction_t *pxAction );

static eModuleError_t prvFlashIteratePages( xFlashDevice_t *pxDevice, fnFlashOperation_t fnOperation, void *pvContext, uint32_t ulFlashPage, uint16_t usFlashOffset, uint32_t ulNumBytes );

static eModuleError_t prvFlashIterateWrite( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext );
static eModuleError_t prvFlashIterateRead( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext );
static eModuleError_t prvFlashIterateCrc( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext );
static eModuleError_t prvFlashIterateRomStore( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext );

static eModuleError_t prvFlashErase( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint32_t ulDataLength );
static eModuleError_t prvFlashRomCopyDeltas( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint8_t *pucRomAddress, uint8_t *pucDeltas, uint8_t *pucDeltaData, uint8_t ucNumDeltas );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t eFlashInit( xFlashDevice_t *pxDevice )
{
	pxDevice->xCommandQueue = xQueueCreate( 1, sizeof( xFlashAction_t ) );
	xTaskCreate( prvFlashInterfaceTask, pxDevice->pcName, configMINIMAL_STACK_SIZE, pxDevice, tskIDLE_PRIORITY + 2, NULL );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashRead( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucData, uint32_t ulLength, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_READ,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = pucData,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = ulLength,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashWrite( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucData, uint32_t ulLength, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_WRITE,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = pucData,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = ulLength,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashErase( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_ERASE_PAGES,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = NULL,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = ulLength,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashEraseAll( xFlashDevice_t *pxDevice, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_ERASE_ALL,
		.ullFlashAddress = 0x00,
		.pucArg1		 = NULL,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = 0,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashCrc( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, uint16_t *pusCrc, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_CRC,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = (uint8_t *) pusCrc,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = ulLength,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashRomStore( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint32_t ulLength, uint8_t *pucRomAddress, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_ROM_STORE,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = (uint8_t *) pucRomAddress,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = ulLength,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashRomStoreDeltas( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, uint8_t *pucRomAddress, uint8_t *pucDeltas, uint8_t *pucDeltaData, uint8_t ucNumDeltas, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_ROM_STORE_DELTAS,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = (uint8_t *) pucRomAddress,
		.pucArg2		 = pucDeltas,
		.pucArg3		 = pucDeltaData,
		.ulLength		 = ucNumDeltas,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eFlashStartRead( xFlashDevice_t *pxDevice, uint64_t ullFlashAddress, TickType_t xTimeout )
{
	eModuleError_t eResult;
	xFlashAction_t xAction = {
		.eCommand		 = FLASH_ROM_START_READ,
		.ullFlashAddress = ullFlashAddress,
		.pucArg1		 = NULL,
		.pucArg2		 = NULL,
		.pucArg3		 = NULL,
		.ulLength		 = 0x00,
		.xResponseTask   = xTaskGetCurrentTaskHandle(),
		.peResult		 = &eResult
	};
	return prvFlashExecuteAction( pxDevice->xCommandQueue, &xAction, xTimeout );
}

/* Private Functions ----------------------------------------*/

eModuleError_t prvFlashExecuteAction( QueueHandle_t xQueue, xFlashAction_t *pxAction, TickType_t xTimeout )
{
	eModuleError_t eError;
	pxAction->xResponseTask = xTaskGetCurrentTaskHandle();
	pxAction->peResult		= &eError;
	/* Send our command to the task */
	if ( xQueueSendToBack( xQueue, pxAction, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	/** 
	 * Wait until the task signifies it is done
	 * We use portMAX_DELAY so that 
	 * 		* Once an operation has triggered, we are actually returning the result of that operation
	 * 		* eError doesn't go out of scope before flash task writes to it
	 * 		* We don't get into a bad state where a task is receiving the result of the previous operation
	 */
	if ( ulTaskNotifyTake( pdTRUE, portMAX_DELAY ) == 0 ) {
		return ERROR_TIMEOUT;
	}
	return eError;
}

/*-----------------------------------------------------------*/

static void prvFlashWaitAction( xFlashDevice_t *pxDevice, xFlashAction_t *pxAction )
{
	bool bPowerApplied;
	/* Wait for 2 seconds to see if someone wants to use the device */
	if ( xQueueReceive( pxDevice->xCommandQueue, pxAction, pdMS_TO_TICKS( 2000 ) ) == pdPASS ) {
		/* We received a command, return */
		return;
	}
	/* Device is idle, put device into sleep mode */
	pxDevice->pxImplementation->fnSleep( pxDevice );
	/* Power down any external circuitry */
	vBoardDisablePeripheral( PERIPHERAL_ONBOARD_FLASH );
	/* Wait forever for a command */
	configASSERT( xQueueReceive( pxDevice->xCommandQueue, pxAction, portMAX_DELAY ) == pdPASS );
	/* Power up external circuitry */
	eBoardEnablePeripheral( PERIPHERAL_ONBOARD_FLASH, &bPowerApplied, portMAX_DELAY );
	/* Wake the device */
	pxDevice->pxImplementation->fnWake( pxDevice, bPowerApplied );
}

/*-----------------------------------------------------------*/

ATTR_NORETURN void prvFlashInterfaceTask( void *pvParameters )
{
	xFlashDevice_t *  pxDevice   = (xFlashDevice_t *) pvParameters;
	xFlashSettings_t *pxSettings = &pxDevice->xSettings;
	xFlashAction_t	xAction;
	eModuleError_t	eError;

	uint32_t ulFlashPage;
	uint16_t usFlashOffset;

	/* Initialise the device */
	eBoardEnablePeripheral( PERIPHERAL_ONBOARD_FLASH, NULL, portMAX_DELAY );
	pxDevice->pxImplementation->fnInit( pxDevice );

	eLog( LOG_FLASH_DRIVER, LOG_INFO, "%s: %d pages, %d bytes\r\n", pxDevice->pcName, pxDevice->xSettings.ulNumPages, pxDevice->xSettings.usPageSize );

	for ( ;; ) {
		/* Wait for an action */
		prvFlashWaitAction( pxDevice, &xAction );
		ulFlashPage   = ( uint32_t )( xAction.ullFlashAddress >> pxSettings->ucPageSizePower );
		usFlashOffset = ( uint16_t )( xAction.ullFlashAddress & pxSettings->usPageOffsetMask );
		eError		  = ERROR_NONE;

		eLog( LOG_FLASH_DRIVER, LOG_VERBOSE, "%s Action: %d  Addr %llu = %d.%d\r\n", pxDevice->pcName, xAction.eCommand, xAction.ullFlashAddress, ulFlashPage, usFlashOffset );

		switch ( xAction.eCommand ) {
			case FLASH_READ:
				eError = prvFlashIteratePages( pxDevice, prvFlashIterateRead, xAction.pucArg1, ulFlashPage, usFlashOffset, xAction.ulLength );
				break;
			case FLASH_WRITE:
				eError = prvFlashIteratePages( pxDevice, prvFlashIterateWrite, xAction.pucArg1, ulFlashPage, usFlashOffset, xAction.ulLength );
				break;
			case FLASH_ERASE_PAGES:
				eError = prvFlashErase( pxDevice, ulFlashPage, usFlashOffset, xAction.ulLength );
				break;
			case FLASH_ERASE_ALL:
				eError = pxDevice->pxImplementation->fnEraseAll( pxDevice );
				break;
			case FLASH_CRC:
				eError = prvFlashIteratePages( pxDevice, prvFlashIterateCrc, xAction.pucArg1, ulFlashPage, usFlashOffset, xAction.ulLength );
				break;
			case FLASH_ROM_STORE:
				eError = prvFlashIteratePages( pxDevice, prvFlashIterateRomStore, xAction.pucArg1, ulFlashPage, usFlashOffset, xAction.ulLength );
				break;
			case FLASH_ROM_STORE_DELTAS:
				eError = prvFlashRomCopyDeltas( pxDevice, ulFlashPage, usFlashOffset, xAction.pucArg1, xAction.pucArg2, xAction.pucArg3, xAction.ulLength );
				break;
			case FLASH_ROM_START_READ:
				eError = pxDevice->pxImplementation->fnReadStart( pxDevice, ulFlashPage, usFlashOffset );
				break;
			default:
				configASSERT( 0 );
		}
		/* Return the result */
		*xAction.peResult = eError;
		xTaskNotifyGive( xAction.xResponseTask );
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashIteratePages( xFlashDevice_t *pxDevice, fnFlashOperation_t fnOperation, void *pvContext, uint32_t ulFlashPage, uint16_t usFlashOffset, uint32_t ulNumBytes )
{
	xFlashSettings_t *pxSettings = &pxDevice->xSettings;
	uint32_t		  ulFirstPageBytes, ulLastPageBytes, ulNumPages;
	eModuleError_t	eError = ERROR_NONE;
	/* Number of bytes on the first page to read/write */
	ulFirstPageBytes = pxSettings->usPageSize - usFlashOffset;
	ulFirstPageBytes = ulFirstPageBytes > ulNumBytes ? ulNumBytes : ulFirstPageBytes;
	/* Number of trailing bytes on the last page to read/write */
	ulLastPageBytes = ( ulNumBytes - ulFirstPageBytes ) & pxSettings->usPageOffsetMask;
	/* Total number of pages to read/write */
	ulNumPages = 1 + ( ( ulNumBytes - ulFirstPageBytes - ulLastPageBytes ) >> pxSettings->ucPageSizePower ) + ( ulLastPageBytes == 0 ? 0 : 1 );

	uint16_t usNumBytes  = ulFirstPageBytes;
	uint32_t ulByteIndex = 0;
	for ( uint32_t i = 0; i < ulNumPages; i++ ) {
		/* Run the provided operation for this flash page */
		eError = fnOperation( pxDevice, ulFlashPage + i, usFlashOffset, usNumBytes, ulByteIndex, pvContext );
		/* After the first page, all offsets are 0 */
		usFlashOffset = 0;
		ulByteIndex += usNumBytes;
		/* Calculate size of next operation */
		usNumBytes = pxSettings->usPageSize;
		if ( ( ulLastPageBytes != 0 ) && ( i == ( ulNumPages - 2 ) ) ) {
			usNumBytes = ulLastPageBytes;
		}
	}
	/* Run operation one last time at termination */
	eError = fnOperation( pxDevice, 0, 0, 0, ulByteIndex, pvContext );
	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashIterateRead( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext )
{
	uint8_t *pucOutputBuffer = (uint8_t *) pvContext;
	if ( usNumBytes == 0 ) {
		/* Nothing to do on termination */
		return ERROR_NONE;
	}
	return pxDevice->pxImplementation->fnReadSubpage( pxDevice, ulFlashPage, usFlashOffset, pucOutputBuffer + ulByteIndex, usNumBytes );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashIterateWrite( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext )
{
	uint8_t *pucInputBuffer = (uint8_t *) pvContext;
	if ( usNumBytes == 0 ) {
		/* Nothing to do on termination */
		return ERROR_NONE;
	}
	return pxDevice->pxImplementation->fnWriteSubpage( pxDevice, ulFlashPage, usFlashOffset, pucInputBuffer + ulByteIndex, usNumBytes );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashIterateCrc( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext )
{
	eModuleError_t eError;
	if ( ulByteIndex == 0 ) {
		/* First iteration, begin the CRC */
		vCrcStart( CRC16_CCITT, 0xFFFF );
	}
	if ( usNumBytes == 0 ) {
		/* Last iteration, finalise the CRC and return */
		*( (uint16_t *) pvContext ) = (uint16_t) ulCrcCalculate( NULL, 0, true );
		return ERROR_NONE;
	}
	/* Read the provided data into the local buffer */
	eError = pxDevice->pxImplementation->fnReadSubpage( pxDevice, ulFlashPage, usFlashOffset, pxDevice->xSettings.pucPage, usNumBytes );
	/* Run the CRC across the local buffer */
	ulCrcCalculate( pxDevice->xSettings.pucPage, usNumBytes, false );
	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashIterateRomStore( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint16_t usNumBytes, uint32_t ulByteIndex, void *pvContext )
{
	uint8_t *pucRomBase = (uint8_t *) pvContext;
	if ( usNumBytes == 0 ) {
		/* Nothing to do on termination */
		return ERROR_NONE;
	}
	/* Copy appropriate ROM to local buffer */
	pvMemcpy( pxDevice->xSettings.pucPage, pucRomBase + ulByteIndex, usNumBytes );
	/* Write the ROM to provided flash page */
	return pxDevice->pxImplementation->fnWriteSubpage( pxDevice, ulFlashPage, usFlashOffset, pxDevice->xSettings.pucPage, usNumBytes );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashErase( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint32_t ulDataLength )
{
	xFlashSettings_t *pxSettings = &pxDevice->xSettings;
	/* Check the provided start address is correctly aligned */
	if ( ( usFlashOffset != 0 ) || ( ( ulFlashPage % pxSettings->usErasePages ) != 0 ) ) {
		return ERROR_INVALID_ADDRESS;
	}
	/* Check the erase length is a valid multiple of the minimum erase block */
	if ( ( ulDataLength & pxSettings->usPageOffsetMask ) != 0 ) {
		return ERROR_INVALID_DATA;
	}
	if ( ( ulDataLength >> pxSettings->ucPageSizePower ) % pxSettings->usErasePages != 0 ) {
		return ERROR_INVALID_DATA;
	}
	/* Run the erase operation */
	eLog( LOG_FLASH_DRIVER, LOG_DEBUG, "%s erasing %d pages from page %d\r\n", pxDevice->pcName, ulDataLength >> pxSettings->ucPageSizePower, ulFlashPage );
	return pxDevice->pxImplementation->fnErasePages( pxDevice, ulFlashPage, ulDataLength >> pxSettings->ucPageSizePower );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvFlashRomCopyDeltas( xFlashDevice_t *pxDevice, uint32_t ulFlashPage, uint16_t usFlashOffset, uint8_t *pucRomAddress, uint8_t *pucDeltas, uint8_t *pucDeltaData, uint8_t ucNumDeltas )
{
	eModuleError_t eError;
	uint8_t *	  pucBuffer	 = pxDevice->xSettings.pucPage;
	uint16_t	   usPageSize	= pxDevice->xSettings.usPageSize;
	uint16_t	   usBufferIndex = 0;

	/* Loop across remaining deltas */
	uint8_t ucIndex = 0;
	while ( ucIndex < ucNumDeltas ) {
		/* If we are at another byte that needs to be modified */
		if ( pucDeltas[ucIndex] == 0 ) {
			pucBuffer[usBufferIndex++] = pucDeltaData[ucIndex];
			pucRomAddress++;
			ucIndex++;
			/* Loop again if we have space in the buffer */
			if ( usBufferIndex < ( usPageSize - usFlashOffset ) ) {
				continue;
			}
		}
		/**
	     * ( ulFlashPage, usFlashOffset ) points at the location to write the next data
	     *
	     * (pucRomAddress + ulByteOffset) points to the the last ROM data that has not been copied or modified
	     * pucDeltas[i] is the number of bytes until the next ROM byte that needs to change
	     *
	     * We want to copy in pucDeltas[i] bytes from ROM to the buffer, capped at usFlashPageRemaining
	     */
		uint16_t usFlashPageRemaining = ( usPageSize - usFlashOffset - usBufferIndex );
		uint16_t usDesiredRomCopy	 = pucDeltas[ucIndex];
		uint16_t usRomToCopy		  = usDesiredRomCopy > usFlashPageRemaining ? usFlashPageRemaining : usDesiredRomCopy;
		pvMemcpy( pucBuffer + usBufferIndex, (void *) pucRomAddress, usRomToCopy );
		usBufferIndex += usRomToCopy;
		pucRomAddress += usRomToCopy;
		usFlashPageRemaining -= usRomToCopy;
		/* We have usRomToCopy number of bytes left to copy before the next difference */
		pucDeltas[ucIndex] -= usRomToCopy;
		/**
	     * If usFlashPageRemaining is 0, we have hit the flash page limit, and need to write the buffer to flash
	     */
		if ( usFlashPageRemaining == 0 ) {
			eError = pxDevice->pxImplementation->fnWriteSubpage( pxDevice, ulFlashPage, usFlashOffset, pucBuffer, usBufferIndex );
			ulFlashPage++;
			usFlashOffset = 0;
			usBufferIndex = 0;
			if ( eError != ERROR_NONE ) {
				break;
			}
		}
	}
	/* We have exited the main loop, cleanup the last buffer if we exited cleanly */
	if ( ( eError == ERROR_NONE ) && ( usBufferIndex > 0 ) ) {
		eError = pxDevice->pxImplementation->fnWriteSubpage( pxDevice, ulFlashPage, usFlashOffset, pucBuffer, usBufferIndex );
	}
	return eError;
}

/*-----------------------------------------------------------*/
