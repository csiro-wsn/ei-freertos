/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "memory_operations.h"
#include "sd.h"
#include "sd_ll.h"

#include "board.h"
#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eInitialisationState_t {
	SDCARD_STATE_POWER_ON = 0,
	SDCARD_STATE_IDLE,
	SDCARD_STATE_VOLTAGE,
	SDCARD_STATE_WAIT_READY,
	SDCARD_READY,
	SDCARD_STATE_ERROR
} eInitialisationState_t;

typedef enum eCommand_t {
	SD_PARAMETERS,
	SD_BLOCK_READ,
	SD_BLOCK_WRITE,
	SD_BLOCKS_ERASE
} eCommand_t;

typedef struct xSdAction_t
{
	eCommand_t eCommand;
	uint32_t   ulBlock;
	uint32_t   ulBlockEnd;
	uint16_t   usBlockOffset;
	uint8_t *  pucData;
	uint32_t   ulDataLen;
} xSdAction_t;

/* Function Declarations ------------------------------------*/

static void prvSdTask( void *pvParameters );

static void			  prvSdGetAction( xSdAction_t *pxAction, xSdParameters_t *pxSdParams );
static eModuleError_t prvExecuteAction( xSdAction_t *pxAction, TickType_t xTimeout );

static void			  prvSdStartupSequence( xSdParameters_t *pxParameters );
static void			  prvSdShutdownSequence( void );
static eModuleError_t prvSdIdentify( eSdCardType_t *peCardType );
static eModuleError_t prvSdQueryParameters( xSdParameters_t *pxParameters );
static eModuleError_t prvSdReadRegister( eSdCommand_t eRegisterCommand, uint8_t *pucReg );
static eModuleError_t prvSdBlockRead( xSdParameters_t *pxParameters, uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen );
static eModuleError_t prvSdBlockWrite( xSdParameters_t *pxParameters, uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen );
static eModuleError_t prvSdEraseRange( xSdParameters_t *pxParameters, uint32_t ulBlockFirst, uint32_t ulBlockLast );

/* Private Variables ----------------------------------------*/

static xSdInit_t *pxConfig;

static eSdCardType_t eCardType;

STATIC_TASK_STRUCTURES( pxSdHandle, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 2 );
STATIC_QUEUE_STRUCTURES( pxSdQueue, sizeof( xSdAction_t ), 1 );
STATIC_QUEUE_STRUCTURES( pxSdResponse, sizeof( eModuleError_t ), 1 );

/*-----------------------------------------------------------*/

eModuleError_t eSdInit( xSdInit_t *pxInit )
{
	configASSERT( pxInit != NULL );
	pxConfig  = pxInit;
	eCardType = SDCARD_TYPE_NONE;

	vSdLLInit( pxConfig->pxSpi, pxConfig->xChipSelect );

	STATIC_QUEUE_CREATE( pxSdQueue );
	STATIC_QUEUE_CREATE( pxSdResponse );
	STATIC_TASK_CREATE( pxSdHandle, prvSdTask, "SD", NULL );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSdParameters( xSdParameters_t *pxParameters )
{
	xSdAction_t xAction = {
		.eCommand = SD_PARAMETERS,
		.pucData  = (uint8_t *) pxParameters
	};
	return prvExecuteAction( &xAction, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

eModuleError_t eSdBlockRead( uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint16_t usBufferLen, TickType_t xTimeout )
{
	xSdAction_t xAction = {
		.eCommand	  = SD_BLOCK_READ,
		.ulBlock	   = ulBlockAddress,
		.usBlockOffset = usBlockOffset,
		.pucData	   = pucBuffer,
		.ulDataLen	 = (uint32_t) usBufferLen
	};
	return prvExecuteAction( &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eSdBlockWrite( uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint16_t usBufferLen, TickType_t xTimeout )
{
	xSdAction_t xAction = {
		.eCommand	  = SD_BLOCK_WRITE,
		.ulBlock	   = ulBlockAddress,
		.usBlockOffset = usBlockOffset,
		.pucData	   = pucBuffer,
		.ulDataLen	 = (uint32_t) usBufferLen
	};
	return prvExecuteAction( &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

eModuleError_t eSdEraseBlocks( uint32_t ulFirstBlockAddress, uint32_t ulLastBlockAddress, TickType_t xTimeout )
{
	xSdAction_t xAction = {
		.eCommand   = SD_BLOCKS_ERASE,
		.ulBlock	= ulFirstBlockAddress,
		.ulBlockEnd = ulLastBlockAddress
	};
	return prvExecuteAction( &xAction, xTimeout );
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvSdTask( void *pvParameters )
{
	eModuleError_t  eError = ERROR_NONE;
	xSdParameters_t xSdParams;
	xSdAction_t		xAction;
	UNUSED( pvParameters );

	/* On startup, query the inserted SD card */
	eBoardEnablePeripheral( PERIPHERAL_EXTERNAL_FLASH, NULL, portMAX_DELAY );
	prvSdStartupSequence( &xSdParams );

	for ( ;; ) {
		/* Get an action from the queue */
		prvSdGetAction( &xAction, &xSdParams );

		/* Validate that we found an SD card */
		if ( xSdParams.eCardType != SDCARD_TYPE_NONE ) {
			switch ( xAction.eCommand ) {
				case SD_PARAMETERS:
					pvMemcpy( xAction.pucData, &xSdParams, sizeof( xSdParameters_t ) );
					eError = ERROR_NONE;
					break;
				case SD_BLOCK_WRITE:
					eError = prvSdBlockWrite( &xSdParams, xAction.ulBlock, xAction.usBlockOffset, xAction.pucData, xAction.ulDataLen );
					break;
				case SD_BLOCK_READ:
					eError = prvSdBlockRead( &xSdParams, xAction.ulBlock, xAction.usBlockOffset, xAction.pucData, xAction.ulDataLen );
					break;
				case SD_BLOCKS_ERASE:
					eError = prvSdEraseRange( &xSdParams, xAction.ulBlock, xAction.ulBlockEnd );
					break;
				default:
					configASSERT( 0 );
					break;
			}
		}
		else {
			eError = ERROR_UNAVAILABLE_RESOURCE;
		}
		/* Send the result */
		xQueueSend( pxSdResponse, &eError, 0 );
	}
}

/*-----------------------------------------------------------*/

void prvSdStartupSequence( xSdParameters_t *pxParameters )
{
	vGpioSetup( pxConfig->xChipSelect, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	vSdLLWakeSequence();

	if ( prvSdIdentify( &pxParameters->eCardType ) == ERROR_NONE ) {
		prvSdQueryParameters( pxParameters );
		const char *pcCardString = ( pxParameters->eCardType == SDCARD_TYPE_NONE ) ? "N/A" : ( pxParameters->eCardType == SDCARD_TYPE_SD ) ? "SD" : "SDHC/SDXC";
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD Card Parameters:\r\n"
										"\tTYPE    : %s\r\n"
										"\tSIZE    : %d MB\r\n"
										"\tBLK CNT : %d blocks\r\n"
										"\tBLK SIZE: %d bytes\r\n",
			  pcCardString, pxParameters->ulDeviceSizeMB, pxParameters->ulNumBlocks, pxParameters->ulBlockSize );
	}
}

/*-----------------------------------------------------------*/

void prvSdShutdownSequence( void )
{
	return;
}

/*-----------------------------------------------------------*/

static void prvSdGetAction( xSdAction_t *pxAction, xSdParameters_t *pxSdParams )
{
	eModuleError_t eError;
	/* Wait for 2 seconds to see if someone wants to use the device */
	if ( xQueueReceive( pxSdQueue, pxAction, pdMS_TO_TICKS( 2000 ) ) != pdPASS ) {
		/* Device is effectively idle, put device in sleep mode */
		prvSdShutdownSequence();
		vBoardDisablePeripheral( PERIPHERAL_EXTERNAL_FLASH );
		/* Wait forever for another thread to request an action */
		if ( xQueueReceive( pxSdQueue, pxAction, portMAX_DELAY ) != pdPASS ) {
			configASSERT( 0 );
		}
		/* Exit sleep mode */
		eError = eBoardEnablePeripheral( PERIPHERAL_EXTERNAL_FLASH, NULL, portMAX_DELAY );
		if ( eError == ERROR_NONE ) {
			prvSdStartupSequence( pxSdParams );
		}
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t prvExecuteAction( xSdAction_t *pxAction, TickType_t xTimeout )
{
	TickType_t	 xEndTime = xTaskGetTickCount() + xTimeout;
	eModuleError_t eError;
	/* Send our command to the task */
	if ( xQueueSendToBack( pxSdQueue, pxAction, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	/* Wait until the task signifies it is done */
	xTimeout = xEndTime - xTaskGetTickCount();
	if ( xQueueReceive( pxSdResponse, &eError, ( xTimeout == 0 ) ? 1 : xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdIdentify( eSdCardType_t *peCardType )
{
	eModuleError_t		   eError;
	TickType_t			   xEndTime;
	eInitialisationState_t eState = SDCARD_STATE_POWER_ON;
	uint8_t				   pucResponse[5];
	uint32_t			   ulOCR;
	bool				   bRunning = true;
	while ( bRunning ) {
		switch ( eState ) {
			case SDCARD_STATE_POWER_ON:
				/* Move from SD Mode to SPI Mode */
				eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Switching to SPI\r\n" );
				eError = eSdCommand( SD_GO_IDLE_STATE, 0x00, pucResponse );
				if ( eError != ERROR_NONE ) {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Failed to detect card\r\n" );
					/* SD identify should return ERROR_NONE, as no card inserted is not an error */
					eError = ERROR_NONE;
					eState = SDCARD_STATE_ERROR;
				}
				else if ( pucResponse[0] & SD_RESP_IN_IDLE ) {
					eState = SDCARD_STATE_IDLE;
				}
				else {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Card Communication Error\r\n" );
					eState = SDCARD_STATE_ERROR;
				}
				break;
			case SDCARD_STATE_IDLE:
				/* Check that we can communicate with the device at our voltage level */
				eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Checking communication\r\n" );
				eError = eSdCommand( SD_SEND_IF_COND, ( IF_COND_VOLTAGE_27V_36V << 8 ) | SD_CHECK_PATTERN, pucResponse );
				eState = SDCARD_STATE_VOLTAGE;
				if ( eError != ERROR_NONE ) {
					eState = SDCARD_STATE_ERROR;
				}
				if ( ( pucResponse[3] & 0x0F ) != 1 ) {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Unacceptable supply voltage\r\n" );
					eState = SDCARD_STATE_ERROR;
				}
				if ( pucResponse[4] != SD_CHECK_PATTERN ) {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: IF_COND Pattern Mismatch 0x%02X\r\n", pucResponse[4] );
					eState = SDCARD_STATE_ERROR;
				}
				break;
			case SDCARD_STATE_VOLTAGE:
				/* Query the voltages supported by the SD Card */
				eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Querying chip voltages\r\n" );
				eError = eSdCommand( SD_READ_OCR, 0x00, pucResponse );
				if ( eError != ERROR_NONE ) {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Error while querying voltages %d\r\n", eError );
					eState = SDCARD_STATE_ERROR;
				}
				ulOCR = BE_U32_EXTRACT( pucResponse + 1 );
				if ( ( ulOCR & SD_OCR_27_36 ) == 0 ) {
					eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Unsupported voltage range, OCR - 0x%08X\r\n", ulOCR );
					eState = SDCARD_STATE_ERROR;
				}
				eState = SDCARD_STATE_WAIT_READY;
				break;
			case SDCARD_STATE_WAIT_READY:
				/* Wait until device internal initialisation sequence has completed */
				eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Waiting on card initialisation\r\n" );
				/* Loop until the card inidicates initialisation has completed or timed out*/
				xEndTime = xTaskGetTickCount() + pdMS_TO_TICKS( 500 );
				do {
					eError = eSdCommand( SD_SEND_OP_COND, OP_COND_HIGH_CAPACITY_SUPPORT, pucResponse );
					vTaskDelay( pdMS_TO_TICKS( 50 ) );
				} while ( ( eError == ERROR_NONE ) && ( pucResponse[0] & SD_RESP_IN_IDLE ) && ( xTaskGetTickCount() < xEndTime ) );
				/* Check the result of our loop */
				if ( ( eError != ERROR_NONE ) || ( pucResponse[0] ) ) {
					eLog( LOG_SD_DRIVER, LOG_INFO, "SD: SD card failed to initialise\r\n" );
					eState = SDCARD_STATE_ERROR;
				}
				eState = SDCARD_READY;
				break;
			case SDCARD_READY:
				/* Determine whether the SD card is a SD or SDHC card */
				eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Querying chip CCS\r\n" );
				/* Loop until the CCS bit becomes valid */
				xEndTime = xTaskGetTickCount() + pdMS_TO_TICKS( 500 );
				do {
					eError = eSdCommand( SD_READ_OCR, 0x00, pucResponse );
					ulOCR  = BE_U32_EXTRACT( pucResponse + 1 );
					vTaskDelay( pdMS_TO_TICKS( 50 ) );
				} while ( ( eError == ERROR_NONE ) && !( ulOCR & SD_OCR_POWER_UP_STATUS ) && ( xTaskGetTickCount() < xEndTime ) );
				/* Check the result of our loop */
				if ( eError != ERROR_NONE ) {
					eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Error while querying CCS %d\r\n", eError );
					eState = SDCARD_STATE_ERROR;
				}
				/* CCS bit determines whether the Card is High-Capacity or Standard-Capactity */
				if ( ulOCR & SD_OCR_CCS ) {
					eCardType = SDCARD_TYPE_SDHC;
				}
				else {
					eCardType = SDCARD_TYPE_SD;
				}
				bRunning = false;
				break;
			case SDCARD_STATE_ERROR:
			default:
				eCardType = SDCARD_TYPE_NONE;
				bRunning  = false;
				break;
		}
		vTaskDelay( pdMS_TO_TICKS( 1 ) + 1 );
	}
	/* Return card type */
	if ( peCardType != NULL ) {
		*peCardType = eCardType;
	}
	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdReadRegister( eSdCommand_t eRegisterCommand, uint8_t *pucReg )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;
	eError = eSdCommand( eRegisterCommand, 0x00, &ucResponse );
	if ( eError != ERROR_NONE ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Failed to read register 0x%02X\r\n", eRegisterCommand );
		return eError;
	}

	eSdReadRegister( pucReg, 16 );

	eLog( LOG_SD_DRIVER, LOG_VERBOSE, "SD: REG - 0x%02X DATA - % 16A\r\n", eRegisterCommand, pucReg );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdQueryParameters( xSdParameters_t *pxParameters )
{
	eModuleError_t eError;
	uint8_t		   pucReg[16];
	if ( eCardType == SDCARD_TYPE_NONE ) {
		pxParameters->ulDeviceSizeMB = 0;
		pxParameters->ulBlockSize	= 0;
		pxParameters->ulNumBlocks	= 0;
		return ERROR_NONE;
	}
	/* Read the Card Specific Data */
	eError = prvSdReadRegister( SD_SEND_CSD, pucReg );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	vSdParseCSD( pucReg, pxParameters );
	/* Read the Card Identification */
	eError = prvSdReadRegister( SD_SEND_CID, pucReg );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	vSdPrintCID( pucReg );
	/* Read the SD Configuration Register  */
	eError = prvSdReadRegister( SD_SEND_SCR, pucReg );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	vSdParseSCR( pucReg, pxParameters );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdBlockRead( xSdParameters_t *pxParameters, uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	UNUSED( pxParameters );
	eModuleError_t eError;
	uint8_t		   ucResponse;

	eError = eSdCommand( SD_READ_SINGLE_BLOCK, ulBlockAddress, &ucResponse );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	eError = eSdReadBytes( usBlockOffset, pucBuffer, ulBufferLen );

	eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Read Addr  - 0x%08X Offset - %3d Data - % 4A...\r\n", ulBlockAddress, usBlockOffset, pucBuffer );

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdBlockWrite( xSdParameters_t *pxParameters, uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;

	eError = eSdCommand( SD_WRITE_BLOCK, ulBlockAddress, &ucResponse );
	if ( eError != ERROR_NONE ) {
		return eError;
	}

	eError = eSdWriteBytes( pxParameters, usBlockOffset, pucBuffer, ulBufferLen );

	eLog( LOG_SD_DRIVER, LOG_INFO, "SD: Write Addr - 0x%08X Offset - %3d Data - % 4A...\r\n", ulBlockAddress, usBlockOffset, pucBuffer );

	return eError;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvSdEraseRange( xSdParameters_t *pxParameters, uint32_t ulBlockFirst, uint32_t ulBlockLast )
{
	eModuleError_t eError;
	uint8_t		   ucResponse;
	UNUSED( pxParameters );

	/* Erase is a 3 stage operation, set start address, set end address, send erase command */
	eError = eSdCommand( SD_ERASE_WR_BLK_START_ADDR, ulBlockFirst, &ucResponse );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	eError = eSdCommand( SD_ERASE_WR_BLK_END_ADDR, ulBlockLast, &ucResponse );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	eError = eSdCommand( SD_ERASE, 0x00, &ucResponse );
	if ( eError != ERROR_NONE ) {
		return eError;
	}
	/** 
	 * Note that we don't need to wait for the erase to complete at this point.
	 * All calls to eSdCommand() block until eWaitReady(), which will indicate erase completion.
	 **/
	return eError;
}

/*-----------------------------------------------------------*/
