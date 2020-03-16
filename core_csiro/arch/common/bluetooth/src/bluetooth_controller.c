/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"
#include "message_buffer.h"
#include "queue.h"
#include "task.h"

#include "linked_list.h"
#include "memory_pool.h"
#include "rtc.h"

/* API that is being implemented here */
#include "bluetooth.h"
#include "bluetooth_controller.h"
#include "bluetooth_stack.h"

#include "bluetooth_gap.h"
#include "bluetooth_gatt.h"

/* Private Defines ------------------------------------------*/

//clang-format off
#define ADVERTISING_PERIOD_MS 200
#define ADVERTISING_PERIOD pdMS_TO_TICKS( ADVERTISING_PERIOD_MS )

#define CONNECTION_PRESENT() ( xEventGroupGetBits( pxBluetoothState ) & ( BLUETOOTH_CONNECTING | BLUETOOTH_CONNECTED ) )

//clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eCommand_t {
	/* Configuration Commands */
	COMMAND_POWER_SET,
	COMMAND_CFG_SCAN,
	COMMAND_CFG_CONN,
	/* Placeholder Enum */
	COMMAND_STACK_MUST_BE_ON,
	/* Commands requiring stack to be on */
	COMMAND_SCAN_START,
	COMMAND_SCAN_STOP,
	COMMAND_ADVERTISE,
	COMMAND_CONNECT,
	COMMAND_DISCONNECT,
	COMMAND_RSSI,
	COMMAND_LOCAL_CHAR_WRITE,
	COMMAND_LOCAL_CHAR_DISTRIBUTE,
	COMMAND_REMOTE_CHAR_WRITE,
	COMMAND_REMOTE_CHAR_READ,
	COMMAND_REMOTE_CHAR_SUBSCRIBE
} eCommand_t;

typedef struct xCommand_t
{
	eCommand_t				eCommand;
	uint8_t					ucMode;
	xBluetoothConnection_t *pxConnection;
	TaskHandle_t			xReceiveTask;
	eModuleError_t *		peError;
	union
	{
		xBluetoothScanParameters_t *	  pxScanParams;
		xBluetoothConnectionParameters_t *pxConnParams;
		xBluetoothAdvertiseParameters_t * pxAdvParams;
		xGattRemoteCharacteristic_t *	 pxRemoteCharacteristic;
		xGattLocalCharacteristic_t *	  pxLocalCharacteristic;
		eBluetoothPhy_t					  ePHY;
		eBluetoothPhy_t *				  pePHY;
		int8_t							  cTxPowerDbm;
		int16_t *						  psRssi;
	} xParams;
} xCommand_t;

typedef struct xAdvertisingInfo_t
{
	xLinkedListItem_t		  xItem;
	xGapAdvertiseParameters_t xData;
	uint8_t					  ucRepeats;
} xAdvertisingInfo_t;

/* Function Declarations ------------------------------------*/

static void prvBtControllerTask( void *pvParameters );
static void prvBtCallbackTask( void *pvParameters );

static void prvStackGoLowPower( void );
void		vBluetoothControllerAdvertisingComplete( void );

/* Private Variables ----------------------------------------*/

/* Known Public Manufacturers, ordered by likelyhood */
const uint32_t pulBluetoothManufacturerIds[] = {
	0xD89790, /**< CSIRO */
	0xF4CE36, /**< Nordic Semiconductors */
	0xAC233F, /**< Shenzhen Minew Technologies Co. */
	0x000B57, /**< Silicon Laboratories */
};
#define BLUETOOTH_MANUFACTURERS ( sizeof( pulBluetoothManufacturerIds ) / sizeof( pulBluetoothManufacturerIds[0] ) )

static xBluetoothConnectionCallbacks_t xDefaultCallbacks = {
	.fnConnectionOpened = NULL,
	.fnConnectionRssi   = NULL,
	.fnConnectionClosed = NULL
};
static xBluetoothConnection_t xMasterContext = {
	.ucConnectionHandle   = UINT8_MAX,
	.xRemoteAddress		  = { 0 },
	.pxCallbacks		  = &xDefaultCallbacks,
	.bMaster			  = false,
	.eGattDiscovery		  = GATT_DISCOVERY_NONE,
	.ucNumServices		  = 0,
	.ucNumCharacteristics = 0,
	.pxServices			  = { {} },
	.pxCharacteristics	= { {} },
	.xPrivate			  = { 0 }
};
static xBluetoothConnection_t xSlaveContext = {
	.ucConnectionHandle   = UINT8_MAX,
	.xRemoteAddress		  = { 0 },
	.pxCallbacks		  = &xDefaultCallbacks,
	.bMaster			  = false,
	.eGattDiscovery		  = GATT_DISCOVERY_NONE,
	.ucNumServices		  = 0,
	.ucNumCharacteristics = 0,
	.pxServices			  = { {} },
	.pxCharacteristics	= { {} },
	.xPrivate			  = { 0 }
};

/* Commands & State */
EventGroupHandle_t   pxBluetoothState;
static QueueHandle_t pxCommandQueue;
static int8_t		 cCurrentTxPower = 0;

static MessageBufferHandle_t pxCallbackCommand;

/* Advertising packet memory buffers */
MEMORY_POOL_CREATE( AdvertisingPackets, BLUETOOTH_MAX_QUEUED_ADV_PACKETS, sizeof( xAdvertisingInfo_t ) );
xMemoryPool_t *pxAdvertisingPackets = &MEMORY_POOL_GET( AdvertisingPackets );

static xLinkedList_t	  xAdvList;
static xLinkedListItem_t *pxCurrentlyAdvertising;
static xLinkedListItem_t *pxLastToAdvertise;

/*-----------------------------------------------------------*/

static inline eModuleError_t eBluetoothCommand( xCommand_t *pxCommand )
{
	eModuleError_t eError;

	pxCommand->peError		= &eError;
	pxCommand->xReceiveTask = xTaskGetCurrentTaskHandle();

	/* Send command onto the queue to be processed */
	xQueueSend( pxCommandQueue, pxCommand, portMAX_DELAY );

	/* Wait for a Task Notification to know that we are done */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

	return eError;
}

/*-----------------------------------------------------------*/

void vBluetoothControllerInit( void )
{
	BaseType_t xResult;
	pxBluetoothState  = xEventGroupCreate();
	pxCommandQueue	= xQueueCreate( 1, sizeof( xCommand_t ) );
	pxCallbackCommand = xMessageBufferCreate( 4 + sizeof( xStackCallback_t ) );

	xMasterContext.xConnectionState = xEventGroupCreate();
	xSlaveContext.xConnectionState  = xEventGroupCreate();

	xEventGroupSetBits( xMasterContext.xConnectionState, BT_CONNECTION_IDLE );
	xEventGroupSetBits( xSlaveContext.xConnectionState, BT_CONNECTION_IDLE );

	configASSERT( pxBluetoothState );
	configASSERT( pxCommandQueue );

	xResult = xTaskCreate( prvBtControllerTask,
						   "BT CTRL",
						   configMINIMAL_STACK_SIZE,
						   NULL,
						   tskIDLE_PRIORITY + 1,
						   NULL );
	configASSERT( xResult == pdPASS );

	xResult = xTaskCreate( prvBtCallbackTask,
						   "BT CB",
						   configMINIMAL_STACK_SIZE,
						   NULL,
						   tskIDLE_PRIORITY + 1,
						   NULL );
	configASSERT( xResult == pdPASS );
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtControllerTask( void *pvParameters )
{
	UNUSED( pvParameters );
	/* General State */
	xCommand_t	 xCommand;
	EventBits_t	eState;
	eModuleError_t eError;
	/* Timing State */
	TickType_t xNextEvent;
	TickType_t xLastEvent = xTaskGetTickCount();
	/* Advertising State */
	xAdvertisingInfo_t *pxBuffer;
	bool				bWaitingData		 = false; /**< Multi-packet construction is ongoing */
	bool				bNewAdvertisingData  = false; /**< New advertising data that we would like to try advertising immediately */
	uint8_t				ucRemainingSequences = 0;	 /**< Number of remaining times controller needs to start an advertising chain */
	/* Parameters */
	xGapScanParameters_t	   xScanParams;
	xGapConnectionParameters_t xConnParams;

	/* Advertising State Init */
	vMemoryPoolInit( pxAdvertisingPackets );
	vLinkedListInit( &xAdvList );
	pxCurrentlyAdvertising = NULL;

	/* Put driver to low power */
	eBluetoothStackOff();
	xEventGroupSetBits( pxBluetoothState, BLUETOOTH_OFF );

	for ( ;; ) {
		/** 
		 * Time until next event, by default wait forever until a command is received
		 * 
		 * When the last sequence we started had a packet with more than 1 TX, we want to
		 * wake up ADVERTISING_PERIOD_MS milliseconds after the sequence was started
		 * in order to start the next round.
		 * 
		 * If at some point we start receiving another multi-packet sequence, don't run 
		 * the next event until we have the complete packet.
		 **/
		xNextEvent = portMAX_DELAY;
		if ( !bWaitingData && ( ucRemainingSequences > 0 ) ) {
			xNextEvent = ( xLastEvent + ADVERTISING_PERIOD ) - xTaskGetTickCount();
			/* Protect against overflow on the subtraction */
			xNextEvent = xNextEvent > ADVERTISING_PERIOD ? 0 : xNextEvent;
		}
		/* Wait for the next time slot */
		if ( xQueueReceive( pxCommandQueue, &xCommand, xNextEvent ) == pdPASS ) {
			/* Command was received, handle it */
			eError				= ERROR_NONE;
			bNewAdvertisingData = false;
			/* If our command requires the stack to be on, enable it if its not */
			if ( ( xCommand.eCommand > COMMAND_STACK_MUST_BE_ON ) && ( xEventGroupGetBits( pxBluetoothState ) & BLUETOOTH_OFF ) ) {
				eBluetoothStackOn();
				xEventGroupClearBits( pxBluetoothState, BLUETOOTH_OFF );
			}

			/* If a command was received, handle it */
			switch ( xCommand.eCommand ) {
				case COMMAND_POWER_SET:
					cCurrentTxPower = cBluetoothStackGetValidTxPower( xCommand.xParams.cTxPowerDbm );
					break;
				case COMMAND_CFG_SCAN:
					/* Convert to stack scanning parameters */
					xScanParams.ePhy			 = xCommand.xParams.pxScanParams->ePhy;
					xScanParams.bActiveScanning  = false;
					xScanParams.usScanIntervalMs = xCommand.xParams.pxScanParams->usScanIntervalMs;
					xScanParams.usScanWindowMs   = xCommand.xParams.pxScanParams->usScanWindowMs;
					xScanParams.fnCallback		 = xCommand.xParams.pxScanParams->fnCallback;
					eError						 = eBluetoothGapScanConfigure( &xScanParams );
					break;
				case COMMAND_CFG_CONN:
					xConnParams.usConnectionInterval  = xCommand.xParams.pxConnParams->usConnectionInterval;
					xConnParams.usSlaveLatency		  = xCommand.xParams.pxConnParams->usSlaveLatency;
					xConnParams.usSupervisorTimeoutMs = xCommand.xParams.pxConnParams->usSupervisorTimeoutMs;
					eError							  = eBluetoothGapConnectionParameters( &xConnParams );
					break;
				case COMMAND_SCAN_START:
					/* Update our intended scanning state */
					xEventGroupSetBits( pxBluetoothState, eBluetoothPhyToScanningState( xCommand.xParams.ePHY ) );
					/* Check if scanning can begin immediately */
					if ( !( xEventGroupGetBits( pxBluetoothState ) & BLUETOOTH_ADVERTISING ) ) {
						eError = eBluetoothGapScanStart( xCommand.xParams.ePHY );
					}
					break;
				case COMMAND_SCAN_STOP:
					/* Store how we were scanning in return struct */
					if ( xCommand.xParams.pePHY != NULL ) {
						eBluetoothState_t eState = xEventGroupGetBits( pxBluetoothState );
						*xCommand.xParams.pePHY  = eBluetoothScanningStateToPhy( eState );
					}
					xEventGroupClearBits( pxBluetoothState, BLUETOOTH_SCANNING_ALL );
					/* If advertising is currently running, scanning has already been stopped */
					if ( !( xEventGroupGetBits( pxBluetoothState ) & BLUETOOTH_ADVERTISING ) ) {
						eError = eBluetoothGapScanStop();
						/* Turn stack off if possible */
						prvStackGoLowPower();
					}
					break;
				case COMMAND_ADVERTISE:
					/* Claim a space to store this advertising data */
					pxBuffer = (xAdvertisingInfo_t *) ALIGNED_POINTER( pcMemoryPoolClaim( pxAdvertisingPackets, 0 ), 8 );
					if ( pxBuffer == NULL ) {
						eError = ERROR_UNAVAILABLE_RESOURCE;
					}
					else {
						bWaitingData		= !xCommand.xParams.pxAdvParams->bStartSequence;
						bNewAdvertisingData = !bWaitingData;
						/* Convert to stack advertising parameters */
						pxBuffer->xData.ePhy				= xCommand.xParams.pxAdvParams->ePhy;
						pxBuffer->xData.eType				= xCommand.xParams.pxAdvParams->bAdvertiseConnectable ? BLUETOOTH_ADV_CONNECTABLE_SCANNABLE : BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE;
						pxBuffer->xData.ucDataLen			= xCommand.xParams.pxAdvParams->ucDataLen;
						pxBuffer->ucRepeats					= xCommand.xParams.pxAdvParams->ucAdvertiseCount;
						pxBuffer->xData.cTransmitPowerDbm   = cCurrentTxPower;
						pxBuffer->xData.ucAdvertiseCount	= 1;
						pxBuffer->xData.usAdvertisePeriodMs = ADVERTISING_PERIOD_MS;
						pvMemcpy( &pxBuffer->xData.pucData, xCommand.xParams.pxAdvParams->pucData, pxBuffer->xData.ucDataLen );
						/* Store it in the linked list */
						vLinkedListAddToBack( &xAdvList, &pxBuffer->xItem );
						/* Update number of sequences we have to start */
						ucRemainingSequences = MAX( xCommand.xParams.pxAdvParams->ucAdvertiseCount, ucRemainingSequences );
					}
					break;
				case COMMAND_CONNECT:
					/* Scanning must be stopped if currently running */
					if ( xEventGroupGetBits( pxBluetoothState ) & BLUETOOTH_SCANNING_ALL ) {
						eBluetoothGapScanStop();
					}
					/* Determine Address Type if not specified */
					if ( xCommand.pxConnection->xRemoteAddress.eAddressType == BLUETOOTH_ADDR_TYPE_UNKNOWN ) {
						xCommand.pxConnection->xRemoteAddress.eAddressType = eBluetoothAddressType( &xCommand.pxConnection->xRemoteAddress );
					}
					/* Trigger the connection */
					xEventGroupClearBits( xCommand.pxConnection->xConnectionState, BT_CONNECTION_IDLE );
					xEventGroupSetBits( pxBluetoothState, BLUETOOTH_CONNECTING );
					xEventGroupSetBits( xCommand.pxConnection->xConnectionState, BT_CONNECTION_PENDING );
					/* Tell GATT which context to use for the initiated connection */
					vBluetoothGattRegisterInitiatedConnection( xCommand.pxConnection );
					eError = eBluetoothGapConnect( xCommand.pxConnection );
					if ( eError != ERROR_NONE ) {
						xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING );
						xEventGroupClearBits( xCommand.pxConnection->xConnectionState, BT_CONNECTION_PENDING );
						xEventGroupSetBits( xCommand.pxConnection->xConnectionState, BT_CONNECTION_IDLE );
						prvStackGoLowPower();
					}
					break;
				case COMMAND_DISCONNECT:
					/* Store state before disconnection */
					eState = xEventGroupGetBits( pxBluetoothState );
					/* Try a disconnection */
					eError = eBluetoothGapDisconnect( xCommand.pxConnection );
					/* Update state */
					xEventGroupClearBits( xCommand.pxConnection->xConnectionState, UINT16_MAX );
					xEventGroupSetBits( xCommand.pxConnection->xConnectionState, BT_CONNECTION_IDLE );
					xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING | BLUETOOTH_CONNECTED );
					/* Resume scanning if a connection had not been finalised and we are supposed to be scanning */
					if ( ( eState & BLUETOOTH_CONNECTING ) && ( eState & BLUETOOTH_SCANNING_ALL ) && !( eState & BLUETOOTH_ADVERTISING ) ) {
						eBluetoothGapScanStart( eBluetoothScanningStateToPhy( xEventGroupGetBits( pxBluetoothState ) ) );
					}
					/* Turn off the stack if possible */
					prvStackGoLowPower();
					break;
				case COMMAND_RSSI:
					*xCommand.xParams.psRssi = sBluetoothGattConnectionRssi( xCommand.pxConnection );
					break;
				case COMMAND_LOCAL_CHAR_WRITE:
					eError = eBluetoothGattLocalWrite( xCommand.xParams.pxLocalCharacteristic );
					break;
				case COMMAND_LOCAL_CHAR_DISTRIBUTE:
					eError = eBluetoothGattLocalDistribute( xCommand.pxConnection, xCommand.xParams.pxLocalCharacteristic );
					break;
				case COMMAND_REMOTE_CHAR_WRITE:
					eError = eBluetoothGattRemoteWrite( xCommand.pxConnection, xCommand.xParams.pxRemoteCharacteristic, (bool) xCommand.ucMode );
					break;
				case COMMAND_REMOTE_CHAR_READ:
					eError = eBluetoothGattRemoteRead( xCommand.pxConnection, xCommand.xParams.pxRemoteCharacteristic );
					break;
				case COMMAND_REMOTE_CHAR_SUBSCRIBE:
					/* Subscriptions are just a write to the remote CCCD handle */
					{
						xGattRemoteCharacteristic_t xCCCD;
						uint16_t					usCCCD = (uint16_t) xCommand.ucMode;
						xCCCD.usCharacteristicHandle	   = xCommand.xParams.pxRemoteCharacteristic->usCCCDHandle;
						xCCCD.pucData					   = (uint8_t *) &usCCCD;
						xCCCD.usDataLen					   = 2;
						eError							   = eBluetoothGattRemoteWrite( xCommand.pxConnection, &xCCCD, GATT_WRITE_DESCRIPTOR | GATT_WRITE_RESPONSE );
					}
					break;
				default:
					configASSERT( 0 );
					break;
			}
			/* Return the response */
			*xCommand.peError = eError;
			xTaskNotifyGive( xCommand.xReceiveTask );
			/* Return to waiting for the next event if there is no new data to try advertising */
			if ( !bNewAdvertisingData ) {
				continue;
			}
		}

		/* Skip this event if we are in the middle of receiving a multi-packet transmission */
		if ( bWaitingData ) {
			continue;
		}

		/**
		 * If we are still advertising the previous sequence, wait for the next window
		 * This can occur when GATT connections delay transmission times.
		 **/
		if ( xEventGroupGetBits( pxBluetoothState ) & BLUETOOTH_ADVERTISING ) {
			xLastEvent = xTaskGetTickCount();
			continue;
		}

		/* At this point we know there is data to advertise */
		configASSERT( !bLinkedListEmpty( &xAdvList ) );

		/* Setup the start and end points of the advertising chain */
		pxCurrentlyAdvertising		 = pxLinkedListHead( &xAdvList );
		pxLastToAdvertise			 = pxLinkedListTail( &xAdvList );
		xAdvertisingInfo_t *pxParams = (xAdvertisingInfo_t *) pxCurrentlyAdvertising;
		configASSERT( pxCurrentlyAdvertising );

		/* Stop any scanning that may be occuring */
		eBluetoothGapScanStop();
		eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "Advertising sequence starting\r\n" );
		/* Send the correct type of packet depending on our connection state */
		pxParams->xData.eType = CONNECTION_PRESENT() ? BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE : pxParams->xData.eType;
		/* Update our state knowledge */
		xEventGroupSetBits( pxBluetoothState, BLUETOOTH_ADVERTISING );
		eError = eBluetoothGapAdvertise( &pxParams->xData );
		/* Handle Errors */
		if ( eError != ERROR_NONE ) {
			eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "Advertising sequence failed to start\r\n" );
			/* Pretend it finished so that next advertisment starts */
			vBluetoothControllerAdvertisingComplete();
		}
		/* One less advertising chain to initiate */
		ucRemainingSequences--;
		/* Increment to next advertising time */
		xLastEvent = xTaskGetTickCount();
	}
}

/*-----------------------------------------------------------*/

void vBluetoothControllerAdvertisingComplete( void )
{
	xAdvertisingInfo_t *pxAdv				  = (xAdvertisingInfo_t *) pxCurrentlyAdvertising;
	bool				bWasLastPacketInChain = ( pxCurrentlyAdvertising == pxLastToAdvertise );
	xDateTime_t			xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Advertising packet complete\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	/* If this data is done, remove it from the list */
	if ( --pxAdv->ucRepeats == 0 ) {
		vLinkedListRemoveItem( &xAdvList, pxCurrentlyAdvertising );
		vMemoryPoolRelease( pxAdvertisingPackets, (int8_t *) pxCurrentlyAdvertising );
	}
	/* Only advertise until our provided end point */
	if ( bWasLastPacketInChain ) {
		/** 
		 * We are no longer advertising, so the priority list is:
		 * 		* If we are initiating a connection, do nothing
		 * 		* If we are supposed to be scanning, resume that scanning
		 * 		* If there is no connection present, turn off the stack
		 **/
		EventBits_t eState = xEventGroupGetBits( pxBluetoothState );
		if ( eState & BLUETOOTH_CONNECTING ) {
		}
		else if ( eState & BLUETOOTH_SCANNING_ALL ) {
			/* Resume the scanning */
			if ( eBluetoothGapScanStart( eBluetoothScanningStateToPhy( xEventGroupGetBits( pxBluetoothState ) ) ) != ERROR_NONE ) {
				eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to resume scanning\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
			}
		}
		/* Update State */
		xEventGroupClearBits( pxBluetoothState, BLUETOOTH_ADVERTISING );
		/* Turn off the stack if possible */
		prvStackGoLowPower();
	}
	else {
		/* Get the next advertising data from the queue */
		pxCurrentlyAdvertising = pxLinkedListNextItem( &xAdvList, pxCurrentlyAdvertising );
		configASSERT( pxCurrentlyAdvertising );

		pxAdv = (xAdvertisingInfo_t *) pxCurrentlyAdvertising;
		eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "Advertising sequence continuing\r\n" );

		pxAdv->xData.eType	= CONNECTION_PRESENT() ? BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE : pxAdv->xData.eType;
		eModuleError_t eError = eBluetoothGapAdvertise( &pxAdv->xData );
		if ( ( eError == ERROR_UNAVAILABLE_RESOURCE ) && ( pxAdv->xData.eType == BLUETOOTH_ADV_CONNECTABLE_SCANNABLE ) ) {
			pxAdv->xData.eType = BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE;
			eBluetoothGapAdvertise( &pxAdv->xData );
		}
	}
}

/*-----------------------------------------------------------*/

ATTR_NORETURN static void prvBtCallbackTask( void *pvParameters )
{
	UNUSED( pvParameters );
	xStackCallback_t xCallback;
	uint8_t			 pucDataBuffer[BLUETOOTH_GATT_MAX_MTU];

	xGattLocalCharacteristic_t xLocal;

	for ( ;; ) {
		/* Wait for a callback to run */
		xMessageBufferReceive( pxCallbackCommand, &xCallback, sizeof( xStackCallback_t ), portMAX_DELAY );
		/* Save relevant parameters so we can release the provided buffers */
		switch ( xCallback.eCallback ) {
			case STACK_CALLBACK_LOCAL_SUBSCRIBED:
				/* Local buffer for characteristic value */
				pvMemcpy( &xLocal, xCallback.xParams.pxLocal, sizeof( xGattLocalCharacteristic_t ) );
				break;
			case STACK_CALLBACK_LOCAL_WRITTEN:
				/* Local buffer for characteristic value */
				pvMemcpy( &xLocal, xCallback.xParams.pxLocal, sizeof( xGattLocalCharacteristic_t ) );
				pvMemcpy( pucDataBuffer, xLocal.pucData, xLocal.usDataLen );
				xCallback.xParams.pxLocal->pucData = pucDataBuffer;
				break;
			case STACK_CALLBACK_REMOTE_CHANGED:
				/* Local buffer for characteristic value */
				pvMemcpy( pucDataBuffer, xCallback.xParams.pxRemote->pucData, xCallback.xParams.pxRemote->usDataLen );
				xCallback.xParams.pxRemote->pucData = pucDataBuffer;
				break;
			default:
				break;
		}
		/* Release the provided buffers */
		xTaskNotifyGive( xCallback.xTaskToResume );
		/* Run the callback */
		xBluetoothConnectionCallbacks_t *pxCallbacks = xCallback.pxConnection->pxCallbacks;
		switch ( xCallback.eCallback ) {
			case STACK_CALLBACK_CONNECTED:
				if ( pxCallbacks->fnConnectionOpened != NULL ) {
					pxCallbacks->fnConnectionOpened( xCallback.pxConnection );
				}
				break;
			case STACK_CALLBACK_DISCONNECTED:
				if ( pxCallbacks->fnConnectionClosed != NULL ) {
					pxCallbacks->fnConnectionClosed( xCallback.pxConnection );
				}
				break;
			case STACK_CALLBACK_LOCAL_WRITTEN:
				if ( pxCallbacks->fnLocalCharacteristicWritten != NULL ) {
					pxCallbacks->fnLocalCharacteristicWritten( xCallback.pxConnection, &xLocal );
				}
				break;
			case STACK_CALLBACK_LOCAL_SUBSCRIBED:
				if ( pxCallbacks->fnLocalCharacteristicSubscribed != NULL ) {
					pxCallbacks->fnLocalCharacteristicSubscribed( xCallback.pxConnection, &xLocal );
				}
				break;
			case STACK_CALLBACK_REMOTE_CHANGED:
				if ( pxCallbacks->fnRemoteCharacteristicChanged != NULL ) {
					pxCallbacks->fnRemoteCharacteristicChanged( xCallback.pxConnection, xCallback.xParams.pxRemote );
				}
				break;
			case STACK_CALLBACK_REMOTE_READ:
				if ( pxCallbacks->fnRemoteCharacteristicRead != NULL ) {
					pxCallbacks->fnRemoteCharacteristicRead( xCallback.pxConnection, xCallback.xParams.pxRemote );
				}
				break;
			default:
				break;
		}
	}
}

/*-----------------------------------------------------------*/

void vBluetoothControllerCallbackRun( xStackCallback_t *pxCallback )
{
	/* We need to be unblocked when buffer is free to use again */
	pxCallback->xTaskToResume = xTaskGetCurrentTaskHandle();
	/* Send callback off to task to run */
	xMessageBufferSend( pxCallbackCommand, pxCallback, sizeof( xStackCallback_t ), portMAX_DELAY );
	/* Wait for notification that buffer is free to reuse */
	ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

static void prvStackGoLowPower( void )
{
	EventBits_t eState = xEventGroupGetBits( pxBluetoothState );
	if ( !( eState & ( BLUETOOTH_ADVERTISING | BLUETOOTH_SCANNING_ALL | BLUETOOTH_CONNECTING | BLUETOOTH_CONNECTED ) ) ) {
		eBluetoothStackOff();
		xEventGroupSetBits( pxBluetoothState, BLUETOOTH_OFF );
	}
}

/*-----------------------------------------------------------*/

eBluetoothAddressType_t eBluetoothAddressType( xBluetoothAddress_t *pxAddress )
{
	/* First check the address against our known manufacturer list */
	uint32_t ulCompanyId = LE_U24_EXTRACT( &pxAddress->pucAddress[3] );

	for ( uint8_t i = 0; i < BLUETOOTH_MANUFACTURERS; i++ ) {
		if ( ulCompanyId == pulBluetoothManufacturerIds[i] ) {
			return BLUETOOTH_ADDR_TYPE_PUBLIC;
		}
	}
	/* Look at the top 2 bits to make a best effort guess */
	uint8_t ucTopBits = ( pxAddress->pucAddress[5] & 0b11000000 ) >> 6;
	switch ( ucTopBits ) {
		case 0b00:
			return BLUETOOTH_ADDR_TYPE_PRIVATE_NON_RESOLVABLE;
		case 0b01:
			return BLUETOOTH_ADDR_TYPE_PRIVATE_RESOLVABLE;
		case 0b11:
			return BLUETOOTH_ADDR_TYPE_RANDOM_STATIC;
		default:
			return BLUETOOTH_ADDR_TYPE_PUBLIC;
	}
}

/*-----------------------------------------------------------*/

xBluetoothConnection_t *pxBluetoothMasterConfiguration( void )
{
	return &xMasterContext;
}

/*-----------------------------------------------------------*/

xBluetoothConnection_t *pxBluetoothSlaveConfiguration( void )
{
	return &xSlaveContext;
}

/*-----------------------------------------------------------*/

int8_t cBluetoothSetTxPower( int8_t cTxPowerDbm )
{
	xCommand_t xCommand = {
		.eCommand			 = COMMAND_POWER_SET,
		.xParams.cTxPowerDbm = cTxPowerDbm
	};
	configASSERT( eBluetoothCommand( &xCommand ) == ERROR_NONE );
	return cCurrentTxPower;
}

/*-----------------------------------------------------------*/

int8_t cBluetoothGetTxPower( void )
{
	return cCurrentTxPower;
}

/*-----------------------------------------------------------*/

bool bBluetoothUUIDsEqual( xBluetoothUUID_t *pxUUID_A, xBluetoothUUID_t *pxUUID_B )
{
	if ( !( pxUUID_A->bBluetoothOfficialUUID == pxUUID_B->bBluetoothOfficialUUID ) ) {
		return false;
	}
	if ( pxUUID_A->bBluetoothOfficialUUID ) {
		return pxUUID_A->uUUID.usOfficialUUID == pxUUID_B->uUUID.usOfficialUUID;
	}
	else {
		return lMemcmp( pxUUID_A->uUUID.xCustomUUID.pucUUID128, pxUUID_B->uUUID.xCustomUUID.pucUUID128, 16 ) == 0;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothConfigureScanning( xBluetoothScanParameters_t *pxScanParameters )
{
	xCommand_t xCommand = {
		.eCommand			  = COMMAND_CFG_SCAN,
		.xParams.pxScanParams = pxScanParameters
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothConfigureConnections( xBluetoothConnectionParameters_t *pxConnectionsParameters )
{
	xCommand_t xCommand = {
		.eCommand			  = COMMAND_CFG_CONN,
		.xParams.pxConnParams = pxConnectionsParameters
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothScanStart( eBluetoothPhy_t ePHY )
{
	xCommand_t xCommand = {
		.eCommand	 = COMMAND_SCAN_START,
		.xParams.ePHY = ePHY
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothScanStop( eBluetoothPhy_t *pePHY )
{
	xCommand_t xCommand = {
		.eCommand	  = COMMAND_SCAN_STOP,
		.xParams.pePHY = pePHY
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothAdvertisePing( void )
{
	xBluetoothAdvertiseParameters_t xParams;
	const xADFlagsStructure_t		xFlags = {
		  .xHeader = {
			  .ucLength = 0x02,
			  .ucType   = BLE_AD_TYPE_FLAGS },
		  .ucFlags = BLE_ADV_FLAGS_LE_GENERAL_DISC_MODE | BLE_ADV_FLAGS_BR_EDR_NOT_SUPPORTED
	};

	xParams.ePhy				  = BLUETOOTH_PHY_1M;
	xParams.ucAdvertiseCount	  = 1;
	xParams.bStartSequence		  = true;
	xParams.bAdvertiseConnectable = true;
	xParams.ucDataLen			  = sizeof( xADFlagsStructure_t );
	pvMemcpy( xParams.pucData, &xFlags, sizeof( xADFlagsStructure_t ) );

	xCommand_t xCommand = {
		.eCommand			 = COMMAND_ADVERTISE,
		.xParams.pxAdvParams = &xParams
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothAdvertise( xBluetoothAdvertiseParameters_t *pxParams )
{
	xCommand_t xCommand = {
		.eCommand			 = COMMAND_ADVERTISE,
		.xParams.pxAdvParams = pxParams
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothConnect( xBluetoothConnection_t *pxConnection )
{
	xCommand_t xCommand = {
		.eCommand	 = COMMAND_CONNECT,
		.pxConnection = pxConnection
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothConnectWait( xBluetoothConnection_t *pxConnection, TickType_t xTimeout )
{
	return ( xEventGroupWaitBits( pxConnection->xConnectionState, BT_CONNECTION_CONNECTED, 0x00, pdTRUE, xTimeout ) & BT_CONNECTION_CONNECTED ) ? ERROR_NONE : ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothDisconnect( xBluetoothConnection_t *pxConnection )
{
	xCommand_t xCommand = {
		.eCommand	 = COMMAND_DISCONNECT,
		.pxConnection = pxConnection
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

int16_t sBluetoothRssi( xBluetoothConnection_t *pxConnection )
{
	int16_t	sRssi;
	xCommand_t xCommand = {
		.eCommand		= COMMAND_RSSI,
		.pxConnection   = pxConnection,
		.xParams.psRssi = &sRssi
	};
	/* COMMAND_RSSI can't fail */
	configASSERT( eBluetoothCommand( &xCommand ) == ERROR_NONE );
	return sRssi;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothWriteLocalCharacteristic( xGattLocalCharacteristic_t *pxCharacteristic )
{
	xCommand_t xCommand = {
		.eCommand					   = COMMAND_LOCAL_CHAR_WRITE,
		.xParams.pxLocalCharacteristic = pxCharacteristic
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothDistributeLocalCharacteristic( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic )
{
	xCommand_t xCommand = {
		.eCommand					   = COMMAND_LOCAL_CHAR_DISTRIBUTE,
		.pxConnection				   = pxConnection,
		.xParams.pxLocalCharacteristic = pxCharacteristic
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothReadRemoteCharicteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic )
{
	xCommand_t xCommand = {
		.eCommand						= COMMAND_REMOTE_CHAR_READ,
		.pxConnection					= pxConnection,
		.xParams.pxRemoteCharacteristic = pxCharacteristic
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothWriteRemoteCharacteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, bool bAcknowledged )
{
	xCommand_t xCommand = {
		.eCommand						= COMMAND_REMOTE_CHAR_WRITE,
		.pxConnection					= pxConnection,
		.ucMode							= (uint8_t) bAcknowledged,
		.xParams.pxRemoteCharacteristic = pxCharacteristic
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothSubscribeRemoteCharacteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, eBleClientCharacteristicConfiguration_t eSubscriptionMode )
{
	/* Check that a maximum of one mode is requested */
	if ( COUNT_ONES( eSubscriptionMode ) > 1 ) {
		return ERROR_INVALID_DATA;
	}
	/* Validate the requested mode is supported by the remote characteristic */
	if ( ( eSubscriptionMode == BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_NOTIFICATION ) && !( pxCharacteristic->eCharacteristicProperties & BLE_CHARACTERISTIC_PROPERTY_NOTIFY ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( ( eSubscriptionMode == BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_INDICATION ) && !( pxCharacteristic->eCharacteristicProperties & BLE_CHARACTERISTIC_PROPERTY_INDICATE ) ) {
		return ERROR_INVALID_DATA;
	}
	xCommand_t xCommand = {
		.eCommand						= COMMAND_REMOTE_CHAR_SUBSCRIBE,
		.pxConnection					= pxConnection,
		.ucMode							= (uint8_t) eSubscriptionMode,
		.xParams.pxRemoteCharacteristic = pxCharacteristic
	};
	return eBluetoothCommand( &xCommand );
}

/*-----------------------------------------------------------*/
