/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/
#include "FreeRTOS.h"
#include "semphr.h"

#include "bluetooth.h"
#include "bluetooth_controller.h"
#include "bluetooth_gap.h"
#include "bluetooth_gatt.h"
#include "bluetooth_utility.h"

#include "memory_operations.h"
#include "rtc.h"

#include "gatt_efr32.h"
#include "rtos_gecko.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Begin a complete discovery of all services on the currently connected remote device
 *
 * @param[in] pxConnection				Context
 * 
 * @retval ::ERROR_NONE 				Discovery initiated successfully
 * @retval ::ERROR_INVALID_STATE 		No connection is present
 */
static eModuleError_t eBluetoothGattServiceDiscovery( xBluetoothConnection_t *pxContext );

static eModuleError_t eBluetoothGattCharacteristicDiscovery( xBluetoothConnection_t *pxContext, uint32_t ulServiceId );

static eModuleError_t eBluetoothGattCCCDDiscovery( xBluetoothConnection_t *pxContext, uint8_t ucCharacteristericIndex );

static eModuleError_t eGattDiscoveryFinalise( xBluetoothConnection_t *pxContext );

/**@brief Handle completion of GATT procedures
 *
 * @param[in] pxConnection			Context
 * @param[in] usResult				GATT Result
 */
static void prvGattProcedureDone( xBluetoothConnection_t *pxContext, uint16_t usResult );

/* Private Variables ----------------------------------------*/

/* Stores the pointer for the connection we are initiating before we get a connection ID for it */
static xBluetoothConnection_t *pxContextForInitiatedConnection;

/* This should form a mapping of some description from Connection ID's to the context pointer associated with that connection */
static xBluetoothConnection_t *pxConnectionContexts[1] = {
	NULL
};

/*-----------------------------------------------------------*/

void vBluetoothGattEventHandler( struct gecko_cmd_packet *pxEvent )
{
	const uint32_t ulEventId = BGLIB_MSG_ID( pxEvent->header );
	xDateTime_t	xDateTime;
	uint8_t		   ucIndex;

	/* Replace with the connection index from the event when implemented */
	xBluetoothConnection_t *pxEventConnection = pxConnectionContexts[0];

	struct gecko_msg_le_connection_opened_evt_t *pxConnOpened = &pxEvent->data.evt_le_connection_opened;
	struct gecko_msg_le_connection_rssi_evt_t *  pxConnRssi   = &pxEvent->data.evt_le_connection_rssi;
	struct gecko_msg_le_connection_closed_evt_t *pxConnClosed = &pxEvent->data.evt_le_connection_closed;

	struct gecko_msg_le_connection_parameters_evt_t *pxConnParams  = &pxEvent->data.evt_le_connection_parameters;
	struct gecko_msg_le_connection_phy_status_evt_t *pxPhyStatus   = &pxEvent->data.evt_le_connection_phy_status;
	struct gecko_msg_gatt_mtu_exchanged_evt_t *		 pxMtuExchange = &pxEvent->data.evt_gatt_mtu_exchanged;

	struct gecko_msg_gatt_procedure_completed_evt_t *   pxGattComplete = &pxEvent->data.evt_gatt_procedure_completed;
	struct gecko_msg_gatt_service_evt_t *				pxGattService  = &pxEvent->data.evt_gatt_service;
	struct gecko_msg_gatt_characteristic_evt_t *		pxGattChar	 = &pxEvent->data.evt_gatt_characteristic;
	struct gecko_msg_gatt_characteristic_value_evt_t *  pxGattRead	 = &pxEvent->data.evt_gatt_characteristic_value;
	struct gecko_msg_gatt_descriptor_evt_t *			pxGattDesc	 = &pxEvent->data.evt_gatt_descriptor;
	struct gecko_msg_gatt_server_attribute_value_evt_t *pxGattWrite	= &pxEvent->data.evt_gatt_server_attribute_value;

	struct gecko_msg_gatt_server_characteristic_status_evt_t *pxCharacteristicStatus = &pxEvent->data.evt_gatt_server_characteristic_status;

	xStackCallback_t			 xCallback;
	xGattRemoteCharacteristic_t *pxRemote;

	bRtcGetDatetime( &xDateTime );
	switch ( ulEventId ) {
			/* Handled Externally */
		case gecko_evt_system_boot_id:
		case gecko_evt_le_gap_scan_response_id:
		case gecko_evt_le_gap_adv_timeout_id:
			break;

		case gecko_evt_le_connection_opened_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT %2d.%05d: Device Connected - Handle=%d MAC=%:6R\r\n",
				  xDateTime.xTime.ucSecond, xDateTime.xTime.usSecondFraction,
				  pxEventConnection->ucConnectionHandle,
				  pxConnOpened->address.addr );
			xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING );
			xEventGroupSetBits( pxBluetoothState, BLUETOOTH_CONNECTED );

			/* Initialise connection context appropriately */
			if ( pxConnOpened->master ) {
				/* Index 0 should instead be based off connection index */
				pxConnectionContexts[0] = pxContextForInitiatedConnection;
				pxEventConnection		= pxConnectionContexts[0];
				/* This device initiated the connection  */
				pxEventConnection->bMaster			  = true;
				pxEventConnection->ucConnectionHandle = pxConnOpened->connection;

				/* Resume scanning if it was interrupted by the connection initiation */
				EventBits_t eState = xEventGroupGetBits( pxBluetoothState );
				if ( ( eState & BLUETOOTH_SCANNING_ALL ) && !( eState & BLUETOOTH_ADVERTISING ) ) {
					if ( eBluetoothGapScanStart( eBluetoothScanningStateToPhy( eState ) ) != ERROR_NONE ) {
						eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to resume scanning\r\n" );
					}
				}
			}
			else {
				/* If the remote end initiated the connection */
				pxConnectionContexts[0] = pxBluetoothSlaveConfiguration();
				pxEventConnection		= pxConnectionContexts[0];
				/* Update connection information */
				pxEventConnection->ucConnectionHandle		   = pxConnOpened->connection;
				pxEventConnection->bMaster					   = false;
				pxEventConnection->xRemoteAddress.eAddressType = pxConnOpened->address_type;
				pvMemcpy( pxEventConnection->xRemoteAddress.pucAddress, pxConnOpened->address.addr, BLUETOOTH_MAC_ADDRESS_LENGTH );
			}
			xEventGroupClearBits( pxEventConnection->xConnectionState, BT_CONNECTION_PENDING | BT_CONNECTION_OPERATION_DONE );
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_CONNECTED );

			/* Trigger specified GATT discovery */
			if ( pxEventConnection->eGattDiscovery == GATT_DISCOVERY_NONE ) {
				/* No discovery required, run the connection callback */
				xCallback.pxConnection = pxEventConnection;
				xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
				vBluetoothControllerCallbackRun( &xCallback );
			}
			else if ( pxEventConnection->eGattDiscovery == GATT_DISCOVERY_AUTOMATIC ) {
				/* Automatic discovery of services was requested */
				eBluetoothGattServiceDiscovery( pxEventConnection );
			}
			else {
				/* Perform manual discovery of specified services */
				eLog( LOG_BLUETOOTH_GATT, LOG_APOCALYPSE, "BT: Manual Service Discovery of %d ---- TBI\r\n", pxEventConnection->ucNumServices );
			}
			break;
		case gecko_evt_le_connection_closed_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT %2d.%05d: Device Disconnected 0x%X\r\n",
				  xDateTime.xTime.ucSecond, xDateTime.xTime.usSecondFraction,
				  pxConnClosed->reason );
			xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING | BLUETOOTH_CONNECTED );
			xEventGroupClearBits( pxEventConnection->xConnectionState, BT_CONNECTION_PENDING | BT_CONNECTION_CONNECTED );
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_IDLE );
			/* Unblock any blocking operation */
			pxEventConnection->xPrivate.eError = ERROR_INVALID_STATE;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			/* Run the connection complete callback */
			xCallback.pxConnection = pxEventConnection;
			xCallback.eCallback	= STACK_CALLBACK_DISCONNECTED;
			vBluetoothControllerCallbackRun( &xCallback );
			pxEventConnection->ucConnectionHandle = UINT8_MAX;
			break;
		case gecko_evt_le_connection_parameters_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT: Conn param updated\r\n\tMin Int: %d\r\n\tMax Int: %d\r\n\tLatency: %d\r\n\tTimeout: %dms\r\n",
				  pxConnParams->interval, pxConnParams->interval, pxConnParams->latency, 10 * pxConnParams->timeout );
			break;
		case gecko_evt_le_connection_phy_status_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Conn PHY %d\r\n", pxPhyStatus->phy );
			break;
		case gecko_evt_gatt_mtu_exchanged_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Gatt MTU %d\r\n", pxMtuExchange->mtu );
			break;
		case gecko_evt_le_connection_rssi_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Conn RSSI %d\r\n", pxConnRssi->rssi );
			break;
		/**
         *  Low Level GATT Events
         */
		case gecko_evt_gatt_service_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT Gatt Service Discovered %X %d\r\n", pxGattService->service, pxGattService->uuid.len );
			ucIndex = pxEventConnection->ucNumServices;
			/* Fill in the discovered service */
			if ( ucIndex < BLUETOOTH_GATT_MAX_SERVICES ) {
				pxEventConnection->pxServices[ucIndex].uServiceReference.ulServiceHandle		= pxGattService->service;
				pxEventConnection->pxServices[ucIndex].xUUID.uUUID.xCustomUUID.ucStackReference = 0;
				if ( pxGattService->uuid.len == 2 ) {
					pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID = true;
					pxEventConnection->pxServices[ucIndex].xUUID.uUUID.usOfficialUUID   = LE_U16_EXTRACT( pxGattService->uuid.data );
				}
				else {
					pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID = false;
					pvMemcpy( &pxEventConnection->pxServices[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128, pxGattService->uuid.data, pxGattService->uuid.len );
				}
				pxEventConnection->ucNumServices++;
			}
			else {
				eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Too many GATT Services\r\n" );
			}
			break;

		case gecko_evt_gatt_characteristic_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT Gatt Characteristic Discovered %d %d\r\n", pxGattChar->characteristic, pxGattChar->uuid.len );
			/* Fill in the discovered characteristic */
			ucIndex = pxEventConnection->ucNumCharacteristics;
			if ( ucIndex < BLUETOOTH_GATT_MAX_CHARACTERISTICS ) {
				pxEventConnection->pxCharacteristics[ucIndex].uServiceReference.ulServiceHandle = pxEventConnection->pxServices[pxEventConnection->xPrivate.ucServicesQueried].uServiceReference.ulServiceHandle;
				pxEventConnection->pxCharacteristics[ucIndex].usCharacteristicHandle			= pxGattChar->characteristic;
				pxEventConnection->pxCharacteristics[ucIndex].usCCCDHandle						= 0;
				pxEventConnection->pxCharacteristics[ucIndex].eCharacteristicProperties			= pxGattChar->properties;
				if ( pxGattChar->uuid.len == 2 ) {
					pxEventConnection->pxCharacteristics[ucIndex].xUUID.bBluetoothOfficialUUID = true;
					pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.usOfficialUUID   = LE_U16_EXTRACT( pxGattChar->uuid.data );
				}
				else {
					pxEventConnection->pxCharacteristics[ucIndex].xUUID.bBluetoothOfficialUUID = false;
					pvMemcpy( &pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128, pxGattChar->uuid.data, pxGattChar->uuid.len );
				}
				ucIndex = pxEventConnection->ucNumCharacteristics++;
			}
			else {
				eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Too many GATT Characteristics\r\n" );
			}
			break;

		case gecko_evt_gatt_descriptor_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Gatt Descriptor\r\n" );
			/* Check if this is the CCCD Attribute */
			if ( ( pxGattDesc->uuid.len == 2 ) && ( LE_U16_EXTRACT( pxGattDesc->uuid.data ) == BLE_ATTRIBUTE_TYPE_CLIENT_CHARACTERISTIC_CONFIGURATION ) ) {
				pxEventConnection->pxCharacteristics[pxEventConnection->xPrivate.ucIndex].usCCCDHandle = pxGattDesc->descriptor;
			}
			break;

		case gecko_evt_gatt_procedure_completed_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Gatt Proc Done\r\n" );
			prvGattProcedureDone( pxEventConnection, pxGattComplete->result );
			break;

		/**
         *  GATT Events
         */
		case gecko_evt_gatt_characteristic_value_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT Gatt Value Read\r\n" );

			switch ( pxGattRead->att_opcode ) {
				case gatt_handle_value_indication:
					gecko_cmd_gatt_send_characteristic_confirmation( pxEventConnection->ucConnectionHandle );
					ATTR_FALLTHROUGH;
				case gatt_handle_value_notification:
					pxRemote = pxBluetoothSearchCharacteristicHandle( pxEventConnection, pxGattRead->characteristic );
					if ( pxRemote != NULL ) {
						pxRemote->usDataLen = pxGattRead->value.len;
						pxRemote->pucData   = pxGattRead->value.data;

						xCallback.eCallback		   = STACK_CALLBACK_REMOTE_CHANGED;
						xCallback.pxConnection	 = pxEventConnection;
						xCallback.xParams.pxRemote = pxRemote;
						vBluetoothControllerCallbackRun( &xCallback );
					}
					else {
						eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "Couldn't find characteristic\r\n" );
					}
					break;
				case gatt_read_response:
					pxRemote = pxBluetoothSearchCharacteristicHandle( pxEventConnection, pxGattRead->characteristic );
					if ( pxRemote != NULL ) {
						pxRemote->usDataLen = pxGattRead->value.len;
						pxRemote->pucData   = pxGattRead->value.data;

						xCallback.eCallback		   = STACK_CALLBACK_REMOTE_READ;
						xCallback.pxConnection	 = pxEventConnection;
						xCallback.xParams.pxRemote = pxRemote;
						vBluetoothControllerCallbackRun( &xCallback );
					}
					else {
						eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "Couldn't find characteristic\r\n" );
					}
					break;
				default:
					break;
			}
			break;

		case gecko_evt_gatt_server_attribute_value_id:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "GATT Write: Handle %d, %d bytes\r\n", pxGattWrite->attribute, pxGattWrite->value.len );

			xGattLocalCharacteristic_t xLocal;
			xLocal.usCharacteristicHandle = pxGattWrite->attribute;
			xLocal.usCCCDValue			  = 0;
			xLocal.usDataLen			  = pxGattWrite->value.len;
			xLocal.pucData				  = pxGattWrite->value.data;
			/* Run the callback */
			xCallback.pxConnection	= pxEventConnection;
			xCallback.eCallback		  = STACK_CALLBACK_LOCAL_WRITTEN;
			xCallback.xParams.pxLocal = &xLocal;
			vBluetoothControllerCallbackRun( &xCallback );
			break;

		case gecko_evt_gatt_server_characteristic_status_id:

			if ( pxCharacteristicStatus->status_flags == gatt_server_confirmation ) {
				/** We wrote to a CCCD and this is the confirmation 
				 * 		No need to do anything as a gecko_evt_gatt_procedure_completed_id event is also generated
				 */
			}
			else {
				/* A remote client has written to one of our CCCD characteristics */
				xGattLocalCharacteristic_t xLocal;
				xLocal.usCharacteristicHandle = pxCharacteristicStatus->characteristic;
				xLocal.usCCCDValue			  = pxCharacteristicStatus->client_config_flags;
				xLocal.usDataLen			  = 0;
				xLocal.pucData				  = NULL;
				/* Run Callback */
				xCallback.pxConnection	= pxEventConnection;
				xCallback.eCallback		  = STACK_CALLBACK_LOCAL_SUBSCRIBED;
				xCallback.xParams.pxLocal = &xLocal;
				vBluetoothControllerCallbackRun( &xCallback );
			}
			break;

		default:
			eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Unhandled EVENT 0x%X\r\n", ulEventId );
			break;
	}
}

/*-----------------------------------------------------------*/

static void prvGattProcedureDone( xBluetoothConnection_t *pxContext, uint16_t usResult )
{
	xStackCallback_t xCallback;

	switch ( pxContext->xPrivate.ulGattOperation ) {
		case gecko_cmd_gatt_discover_primary_services_id:
			/* All services have been discovered */
			if ( pxContext->ucNumServices == 0 ) {
				/* Remote device has no Gatt services */
				xCallback.pxConnection = pxContext;
				xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
				vBluetoothControllerCallbackRun( &xCallback );
			}
			else if ( pxContext->ucNumCharacteristics == 0 ) {
				/* Automatic discovery of Characteristics */
				pxContext->xPrivate.ucServicesQueried = 0;
				eBluetoothGattCharacteristicDiscovery( pxContext, pxContext->pxServices[pxContext->xPrivate.ucServicesQueried].uServiceReference.ulServiceHandle );
			}
			else {
				/* Manual discovery of Characteristics */
				eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Manual Characteristic Discovery of %d\r\n", pxContext->ucNumCharacteristics );
			}
			break;
		case gecko_cmd_gatt_discover_characteristics_id:
			/* Move on to the next service */
			pxContext->xPrivate.ucServicesQueried++;
			if ( pxContext->xPrivate.ucServicesQueried == pxContext->ucNumServices ) {
				/* Set index to max for initial value */
				pxContext->xPrivate.ucIndex = UINT8_MAX;
				/* Discovery may be complete */
				eGattDiscoveryFinalise( pxContext );
			}
			else {
				/* Begin discovery of next service */
				eBluetoothGattCharacteristicDiscovery( pxContext, pxContext->pxServices[pxContext->xPrivate.ucServicesQueried].uServiceReference.ulServiceHandle );
			}
			break;
		case gecko_cmd_gatt_discover_descriptors_id:
			/* Discovery may be complete */
			eGattDiscoveryFinalise( pxContext );
			break;
		case gecko_cmd_gatt_write_characteristic_value_id:
		case gecko_cmd_gatt_write_descriptor_value_id:
			pxContext->xPrivate.eError = ( usResult == 0 ) ? ERROR_NONE : ERROR_INVALID_STATE;
			xEventGroupSetBits( pxContext->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			break;

		default:
			/* Unknown state, reboot */
			configASSERT( 0 );
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t eGattDiscoveryFinalise( xBluetoothConnection_t *pxContext )
{
	xStackCallback_t xCallback;

	/* Discovery may be done, scan through characteristics for undiscovered CCCD handles */
	for ( uint8_t i = 0; i < pxContext->ucNumCharacteristics; i++ ) {
		xGattRemoteCharacteristic_t *pxChar = &pxContext->pxCharacteristics[i];
		/* If we expect a CCCD handle, and its not found */
		if ( ( pxChar->eCharacteristicProperties & ( BLE_CHARACTERISTIC_PROPERTY_NOTIFY | BLE_CHARACTERISTIC_PROPERTY_INDICATE ) ) && ( pxChar->usCCCDHandle == 0 ) ) {
			return eBluetoothGattCCCDDiscovery( pxContext, i );
		}
	}
	/* We have completed discovery */
	xCallback.pxConnection = pxContext;
	xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
	vBluetoothControllerCallbackRun( &xCallback );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vBluetoothGattRegisterInitiatedConnection( xBluetoothConnection_t *pxContext )
{
	pxContextForInitiatedConnection = pxContext;
}

/*-----------------------------------------------------------*/

static eModuleError_t eBluetoothGattServiceDiscovery( xBluetoothConnection_t *pxContext )
{
	struct gecko_msg_gatt_discover_primary_services_rsp_t *pxGattResp;

	pxContext->ucNumServices		= 0;
	pxContext->ucNumCharacteristics = 0;

	pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_discover_primary_services_id;
	pxGattResp							= gecko_cmd_gatt_discover_primary_services( pxContext->ucConnectionHandle );
	/* Check if the operation failed to start */
	if ( pxGattResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Discover Services Error: 0x%X\r\n", pxGattResp->result );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eBluetoothGattCharacteristicDiscovery( xBluetoothConnection_t *pxContext, uint32_t ulServiceId )
{
	struct gecko_msg_gatt_discover_characteristics_rsp_t *pxGattResp;

	pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_discover_characteristics_id;
	pxGattResp							= gecko_cmd_gatt_discover_characteristics( pxContext->ucConnectionHandle, ulServiceId );
	/* Check if the operation failed to start */
	if ( pxGattResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Discover Characteristics Error: 0x%X\r\n", pxGattResp->result );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eBluetoothGattCCCDDiscovery( xBluetoothConnection_t *pxContext, uint8_t ucCharacteristericIndex )
{
	struct gecko_msg_gatt_discover_descriptors_rsp_t *pxGattResp;

	/* If we're, being asked to discover the same characteristic again */
	if ( ucCharacteristericIndex == pxContext->xPrivate.ucIndex ) {
		pxContext->pxCharacteristics[ucCharacteristericIndex].usCCCDHandle = UINT16_MAX;
		eGattDiscoveryFinalise( pxContext );
	}
	else {
		pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_discover_descriptors_id;
		pxGattResp							= gecko_cmd_gatt_discover_descriptors( pxContext->ucConnectionHandle, pxContext->pxCharacteristics[ucCharacteristericIndex].usCharacteristicHandle );
		/* Check if the operation failed to start */
		if ( pxGattResp->result != 0 ) {
			eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Discover Descriptors Error: 0x%X\r\n", pxGattResp->result );
			return ERROR_INVALID_STATE;
		}
		pxContext->xPrivate.ucIndex = ucCharacteristericIndex;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

int16_t sBluetoothGattConnectionRssi( xBluetoothConnection_t *pxContext )
{
	UNUSED( pxContext );
	/* EFR RSSI measurements appear to be asynchronous, annoying to implement now ( cmd_le_connection_get_rssi ) */
	return INT16_MIN;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattLocalWrite( xGattLocalCharacteristic_t *pxCharacteristic )
{
	struct gecko_msg_gatt_server_write_attribute_value_rsp_t *pxGattResp;

	pxGattResp = gecko_cmd_gatt_server_write_attribute_value( pxCharacteristic->usCharacteristicHandle, 0, pxCharacteristic->usDataLen, pxCharacteristic->pucData );
	/* Check if the operation failed to start */
	if ( pxGattResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Gatts write: 0x%X\r\n", pxGattResp->result );
		return ERROR_INVALID_DATA;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattLocalDistribute( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic )
{
	struct gecko_msg_gatt_server_send_characteristic_notification_rsp_t *pxGattResp;

	pxGattResp = gecko_cmd_gatt_server_send_characteristic_notification( pxConnection->ucConnectionHandle, pxCharacteristic->usCharacteristicHandle, pxCharacteristic->usDataLen, pxCharacteristic->pucData );
	/* Check if the operation failed to start */
	if ( pxGattResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Gatts notify: 0x%X\r\n", pxGattResp->result );
		return ERROR_INVALID_DATA;
	}
	/* Wait for indications to complete here  */

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattRemoteRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic )
{
	struct gecko_msg_gatt_read_characteristic_value_rsp_t *pxGattResp;

	pxGattResp = gecko_cmd_gatt_read_characteristic_value( pxConnection->ucConnectionHandle, pxCharacteristic->usCharacteristicHandle );
	if ( pxGattResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Gatts READ: 0x%X\r\n", pxGattResp->result );
		return ERROR_INVALID_DATA;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattRemoteWrite( xBluetoothConnection_t *pxContext, xGattRemoteCharacteristic_t *pxCharacteristic, eGattWriteOptions_t eOptions )
{
	uint16_t usResult;

	if ( eOptions & GATT_WRITE_CHARACTERISTIC ) {
		/* Writing to a characteristic */
		if ( eOptions & GATT_WRITE_RESPONSE ) {
			/* Write has a response */
			struct gecko_msg_gatt_write_characteristic_value_rsp_t *pxWriteResp;
			pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_write_characteristic_value_id;
			pxWriteResp							= gecko_cmd_gatt_write_characteristic_value( pxContext->ucConnectionHandle,
																	 pxCharacteristic->usCharacteristicHandle,
																	 pxCharacteristic->usDataLen,
																	 pxCharacteristic->pucData );
			usResult							= pxWriteResp->result;
		}
		else {
			/* No response to write */
			struct gecko_msg_gatt_write_characteristic_value_without_response_rsp_t *pxWriteResp;
			pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_write_characteristic_value_without_response_id;
			pxWriteResp							= gecko_cmd_gatt_write_characteristic_value_without_response( pxContext->ucConnectionHandle,
																					  pxCharacteristic->usCharacteristicHandle,
																					  pxCharacteristic->usDataLen,
																					  pxCharacteristic->pucData );
			usResult							= pxWriteResp->result;
		}
	}
	else {
		/* Writing to a descriptor */
		struct gecko_msg_gatt_write_descriptor_value_rsp_t *pxWriteResp;
		pxContext->xPrivate.ulGattOperation = gecko_cmd_gatt_write_descriptor_value_id;
		pxWriteResp							= gecko_cmd_gatt_write_descriptor_value( pxContext->ucConnectionHandle,
															 pxCharacteristic->usCharacteristicHandle,
															 pxCharacteristic->usDataLen,
															 pxCharacteristic->pucData );
		usResult							= pxWriteResp->result;
	}
	/* Check if the operation failed to start */
	if ( usResult != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT Gattc write: 0x%X ACK: %d\r\n", usResult, !!( eOptions & GATT_WRITE_RESPONSE ) );
		return ERROR_GENERIC;
	}
	/* For all writes except write_characteristic_without_response, a gatt_procedure_complete event is generated */
	if ( pxContext->xPrivate.ulGattOperation != gecko_cmd_gatt_write_characteristic_value_without_response_id ) {
		/* Wait for the error code from the stack */
		xEventGroupWaitBits( pxContext->xConnectionState, BT_CONNECTION_OPERATION_DONE, pdTRUE, pdTRUE, portMAX_DELAY );
		return (eModuleError_t) pxContext->xPrivate.eError;
	}

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
