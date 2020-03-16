/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/
#include "FreeRTOS.h"
#include "semphr.h"

#include "rtc.h"

#include "bluetooth.h"
#include "bluetooth_controller.h"
#include "bluetooth_gap.h"
#include "bluetooth_gatt.h"
#include "bluetooth_utility.h"

#include "gatt_nrf52.h"

#include "ble.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static eModuleError_t prvBluetoothGattServiceDiscovery( xBluetoothConnection_t *pxContext, uint16_t usStartHandle );

static eModuleError_t prvBluetoothGattUUIDDiscover( xBluetoothConnection_t *pxContext, uint16_t usAttributeHandle );

static eModuleError_t prvBluetoothGattCharacteristicDiscovery( xBluetoothConnection_t *pxContext, uServiceReference_t uService );

static eModuleError_t prvBluetoothGattCCCDDiscovery( xBluetoothConnection_t *pxContext, uint8_t ucIndex );

static eModuleError_t prvBluetoothContinueGattDiscovery( xBluetoothConnection_t *pxContext );

static inline uint8_t prvCharacteristicPropertiesConversion( ble_gatt_char_props_t xStackProperties );

/* Private Variables ----------------------------------------*/

/* Stores the pointer for the connection we are initiating before we get a connection ID for it */
static xBluetoothConnection_t *pxContextForInitiatedConnection;

/* This should form a mapping of some description from Connection ID's to the context pointer associated with that connection */
static xBluetoothConnection_t *pxConnectionContexts[1] = {
	NULL
};

/* Due to the workaround reading directly from services to determine 128bit UUID, we need to intercept read responses */
static bool bDirectServiceRead = false;

/*-----------------------------------------------------------*/

/**
 * !!!!!!!!!!!!!!!!!!!!!!
 * !!!!!!!! TODO !!!!!!!!
 * !!!!!!!!!!!!!!!!!!!!!!
 * 
 * Find a happier path through the GATT connection setup
 * Formalise which side of the connection (or both) is triggering the larger MTU's and DLE
 * Determine most optimal point at which to begin the discovery process
 */
void prvBluetoothGattEventHandler( ble_evt_t const *pxEvent )
{
	const uint32_t ulEventId = pxEvent->header.evt_id;
	xDateTime_t	xDateTime;
	ret_code_t	 eError;
	uint8_t		   ucIndex;

	/* Replace with the connection index from the event when implemented */
	xBluetoothConnection_t *pxEventConnection = pxConnectionContexts[0];

	/* Pre cast all our events types for simplicity later */
	const ble_gap_evt_connected_t *					pxConnected	= &pxEvent->evt.gap_evt.params.connected;
	const ble_gap_evt_disconnected_t *				pxDisconnected = &pxEvent->evt.gap_evt.params.disconnected;
	const ble_gap_evt_data_length_update_request_t *pxDataLenReq   = &pxEvent->evt.gap_evt.params.data_length_update_request;
	const ble_gap_evt_data_length_update_t *		pxDataLen	  = &pxEvent->evt.gap_evt.params.data_length_update;
	const ble_gap_evt_conn_param_update_t *			pxConnParam	= &pxEvent->evt.gap_evt.params.conn_param_update;
	const ble_gattc_evt_exchange_mtu_rsp_t *		pxMtuRsp	   = &pxEvent->evt.gattc_evt.params.exchange_mtu_rsp;

	const ble_gatts_evt_exchange_mtu_request_t *pxMtuRequest = &pxEvent->evt.gatts_evt.params.exchange_mtu_request;

	const ble_gattc_evt_prim_srvc_disc_rsp_t *pxGattService = &pxEvent->evt.gattc_evt.params.prim_srvc_disc_rsp;
	const ble_gattc_evt_char_disc_rsp_t *	 pxGattChar	= &pxEvent->evt.gattc_evt.params.char_disc_rsp;
	const ble_gattc_evt_desc_disc_rsp_t *	 pxGattDesc	= &pxEvent->evt.gattc_evt.params.desc_disc_rsp;
	const ble_gattc_evt_attr_info_disc_rsp_t *pxGattInfo	= &pxEvent->evt.gattc_evt.params.attr_info_disc_rsp;

	const ble_gattc_evt_read_rsp_t * pxGattRead  = &pxEvent->evt.gattc_evt.params.read_rsp;
	const ble_gattc_evt_write_rsp_t *pxWriteResp = &pxEvent->evt.gattc_evt.params.write_rsp;
	const ble_gatts_evt_write_t *	pxGattWrite = &pxEvent->evt.gatts_evt.params.write;
	const ble_gattc_evt_hvx_t *		 pxGattHvx   = &pxEvent->evt.gattc_evt.params.hvx;

	xStackCallback_t			 xCallback;
	xGattRemoteCharacteristic_t *pxRemote;

	bRtcGetDatetime( &xDateTime );
	if ( ulEventId != BLE_GAP_EVT_ADV_REPORT ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT %2d.%05d: EVT %d\r\n",
			  xDateTime.xTime.ucSecond, xDateTime.xTime.usSecondFraction, ulEventId );
	}

	switch ( ulEventId ) {
		/* Handled Externally */
		case BLE_GAP_EVT_ADV_REPORT:
		case BLE_GAP_EVT_ADV_SET_TERMINATED:
			break;

		case BLE_GAP_EVT_CONNECTED:
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT %2d.%05d: Device Connected - Handle=%d MAC=%:6R\r\n",
				  xDateTime.xTime.ucSecond, xDateTime.xTime.usSecondFraction,
				  pxEvent->evt.gap_evt.conn_handle,
				  pxConnected->peer_addr.addr );
			xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING );
			xEventGroupSetBits( pxBluetoothState, BLUETOOTH_CONNECTED );

			/* Initialise connection context appropriately */
			if ( pxConnected->role != BLE_GAP_ROLE_PERIPH ) {
				/* Index 0 should instead be based off connection index */
				pxConnectionContexts[0] = pxContextForInitiatedConnection;
				pxEventConnection		= pxConnectionContexts[0];
				/* This device initiated the connection  */
				pxEventConnection->bMaster			  = true;
				pxEventConnection->ucConnectionHandle = pxEvent->evt.gap_evt.conn_handle;
				/* Resume scanning if it was interrupted by the connection initiation */
				EventBits_t eState = xEventGroupGetBits( pxBluetoothState );
				if ( ( eState & BLUETOOTH_SCANNING_ALL ) && !( eState & BLUETOOTH_ADVERTISING ) ) {
					if ( eBluetoothGapScanStart( eBluetoothScanningStateToPhy( eState ) ) != ERROR_NONE ) {
						eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to resume scanning\r\n" );
					}
				}
				/* Negotiate connection parameters */
				eError = sd_ble_gap_data_length_update( pxEvent->evt.gap_evt.conn_handle, NULL, NULL );
				configASSERT( eError == NRF_SUCCESS );
			}
			else {
				/* If the remote end initiated the connection */
				pxConnectionContexts[0] = pxBluetoothSlaveConfiguration();
				pxEventConnection		= pxConnectionContexts[0];
				/* Update connection information */
				pxEventConnection->ucConnectionHandle		   = pxEvent->evt.gap_evt.conn_handle;
				pxEventConnection->bMaster					   = false;
				pxEventConnection->xRemoteAddress.eAddressType = pxConnected->peer_addr.addr_type;
				pvMemcpy( pxEventConnection->xRemoteAddress.pucAddress, pxConnected->peer_addr.addr, BLUETOOTH_MAC_ADDRESS_LENGTH );
			}
			/* Before beginning discovery, request the maximum possible MTU from the opposite end */
			eError = sd_ble_gattc_exchange_mtu_request( pxEvent->evt.gap_evt.conn_handle, BLUETOOTH_GATT_MAX_MTU );
			configASSERT( eError == NRF_SUCCESS );

			xEventGroupClearBits( pxEventConnection->xConnectionState, BT_CONNECTION_PENDING | BT_CONNECTION_OPERATION_DONE );
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_CONNECTED );

			/* Enable RSSI querying, no events generated, query only. Error conditions should not be possible */
			configASSERT( sd_ble_gap_rssi_start( pxEventConnection->ucConnectionHandle, BLE_GAP_RSSI_THRESHOLD_INVALID, UINT8_MAX ) == NRF_SUCCESS );

			/* We will begin discovery once the MTU has been negotiated */
			break;

		case BLE_GAP_EVT_DISCONNECTED:
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT %2d.%05d: Device Disconnected %d\r\n",
				  xDateTime.xTime.ucSecond, xDateTime.xTime.usSecondFraction, pxDisconnected->reason );
			xEventGroupClearBits( pxBluetoothState, BLUETOOTH_CONNECTING | BLUETOOTH_CONNECTED );
			xEventGroupClearBits( pxEventConnection->xConnectionState, BT_CONNECTION_PENDING | BT_CONNECTION_CONNECTED );
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_IDLE );
			/* Unblock any GATT tasks */
			pxEventConnection->xPrivate.eError = ERROR_INVALID_STATE;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			/* Run the connection complete callback */
			xCallback.pxConnection = pxEventConnection;
			xCallback.eCallback	= STACK_CALLBACK_DISCONNECTED;
			vBluetoothControllerCallbackRun( &xCallback );
			pxEventConnection->ucConnectionHandle = UINT8_MAX;
			break;

		case BLE_GATTC_EVT_EXCHANGE_MTU_RSP:
			/* GATT Server responded to MTU request, begin discovery */
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT: Server accepted MTU of %d\r\n", pxMtuRsp->server_rx_mtu );
			/* Trigger specified GATT discovery */
			if ( pxEventConnection->eGattDiscovery == GATT_DISCOVERY_NONE ) {
				pxEventConnection->ucNumServices		= 0;
				pxEventConnection->ucNumCharacteristics = 0;
				/* No discovery required, run the connection callback */
				xCallback.pxConnection = pxEventConnection;
				xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
				vBluetoothControllerCallbackRun( &xCallback );
			}
			else if ( pxEventConnection->eGattDiscovery == GATT_DISCOVERY_AUTOMATIC ) {
				/* Automatic discovery of services was requested */
				pxEventConnection->ucNumServices			  = 0;
				pxEventConnection->ucNumCharacteristics		  = 0;
				pxEventConnection->xPrivate.ucServicesQueried = 0;
				prvBluetoothGattServiceDiscovery( pxEventConnection, 0x01 );
			}
			else {
				/* Perform manual discovery of specified services */
				eLog( LOG_BLUETOOTH_GATT, LOG_APOCALYPSE, "BT: Manual Service Discovery of %d ---- TBI\r\n", pxEventConnection->ucNumServices );
			}
			break;

		case BLE_GAP_EVT_CONN_PARAM_UPDATE:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT: Conn param updated\r\n\tMin Int: %d\r\n\tMax Int: %d\r\n\tLatency: %d\r\n\tTimeout: %d ms\r\n",
				  pxConnParam->conn_params.min_conn_interval,
				  pxConnParam->conn_params.max_conn_interval,
				  pxConnParam->conn_params.slave_latency,
				  10 * pxConnParam->conn_params.conn_sup_timeout );
			break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE_REQUEST:
			/* The remote device has requested a new GAP event length, respond with whatever we can support */
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "BT: Client event len request TX:%d RX:%d\r\n", pxDataLenReq->peer_params.max_tx_octets, pxDataLenReq->peer_params.max_rx_octets );
			/* Passing NULL results in automatic updates */
			eError = sd_ble_gap_data_length_update( pxEvent->evt.gap_evt.conn_handle, NULL, NULL );
			/* Hard fault if we cannot support the event length, look at BLE_CONN_CFG_GAP in the stack initialisation */
			configASSERT( eError == NRF_SUCCESS );
			break;
		case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
			/* Data Length successfully updated */
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT: Event len updated Tx:%d Rx:%d\r\n", pxDataLen->effective_params.max_tx_octets, pxDataLen->effective_params.max_rx_octets );
			break;

		case BLE_GATTS_EVT_EXCHANGE_MTU_REQUEST:
			/* The slave device has requested a new GATT MTU, use the requested value capped at BLUETOOTH_GATT_MAX_MTU */
			eLog( LOG_BLUETOOTH_GATT, LOG_INFO, "BT: Client MTU Request %d\r\n", pxMtuRequest->client_rx_mtu );
			eError = sd_ble_gatts_exchange_mtu_reply( pxEvent->evt.gatts_evt.conn_handle, pxMtuRequest->client_rx_mtu > BLUETOOTH_GATT_MAX_MTU ? BLUETOOTH_GATT_MAX_MTU : pxMtuRequest->client_rx_mtu );
			/* Hard fault if we cannot support the MTU length, look at BLE_CONN_CFG_GATT in the stack initialisation */
			configASSERT( eError == NRF_SUCCESS );
			break;

		case BLE_GATTC_EVT_PRIM_SRVC_DISC_RSP:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT: Service %d %d %d %d\r\n", pxGattService->count, pxGattService->services[0].handle_range.start_handle, pxGattService->services[0].handle_range.end_handle, pxGattService->services[0].uuid.type );
			/* Loop over discovered services to store them */
			pxEventConnection->xPrivate.ucIndex = 0;
			for ( uint8_t i = 0; i < pxGattService->count; i++ ) {
				const ble_gattc_service_t *pxService = &pxGattService->services[i];
				ucIndex								 = pxEventConnection->ucNumServices;
				/* Fill in the discovered service */
				if ( ucIndex < BLUETOOTH_GATT_MAX_SERVICES ) {
					pxEventConnection->pxServices[ucIndex].uServiceReference.xHandleRange.usRangeStart = pxService->handle_range.start_handle;
					pxEventConnection->pxServices[ucIndex].uServiceReference.xHandleRange.usRangeStop  = pxService->handle_range.end_handle;
					if ( pxService->uuid.type == BLE_UUID_TYPE_BLE ) {
						/* Normal 16bit Bluetooth SIG UUID */
						pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID = true;
						pxEventConnection->pxServices[ucIndex].xUUID.uUUID.usOfficialUUID   = pxService->uuid.uuid;
					}
					else if ( pxService->uuid.type == BLE_UUID_TYPE_UNKNOWN ) {
						/* We need to manually discover the 128bit UUID, silly Softdevice */
						pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID = false;
						/* We need to perform a secondary action (UUID discovery) on the base attribute of the service */
						pxEventConnection->xPrivate.ucIndex = ucIndex;
					}
					else {
						/* 128bit UUID already exists in the stack, pull it out */
						pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID				= false;
						pxEventConnection->pxServices[ucIndex].xUUID.uUUID.xCustomUUID.ucStackReference = pxService->uuid.type;
						vBluetoothStackUUIDResolve( &pxEventConnection->pxServices[ucIndex].xUUID );
						/* Set bytes 12 and 13 */
						pxEventConnection->pxServices[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128[12] = BYTE_READ( pxService->uuid.uuid, 0 );
						pxEventConnection->pxServices[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128[13] = BYTE_READ( pxService->uuid.uuid, 1 );
					}
					pxEventConnection->ucNumServices++;
				}
				else {
					eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Too many GATT Services\r\n" );
				}
			}
			/* Next action to take */
			if ( pxEventConnection->xPrivate.ucIndex != 0 ) {
				/* Perform 128-bit UUID Discovery */
				prvBluetoothGattUUIDDiscover( pxEventConnection, pxEventConnection->pxServices[pxEventConnection->xPrivate.ucIndex].uServiceReference.xHandleRange.usRangeStart );
			}
			else {
				/* Continue GATT Discovery */
				prvBluetoothContinueGattDiscovery( pxEventConnection );
			}
			break;

		case BLE_GATTC_EVT_CHAR_DISC_RSP:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT: Char %d %d %X\r\n", pxGattChar->count, pxGattChar->chars[0].uuid.type, pxGattChar->chars[0].uuid.uuid );
			/* Final terminating condition is an ATTRIBUTE_NOT_FOUND error */
			if ( pxEvent->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_ATTERR_ATTRIBUTE_NOT_FOUND ) {
				/* Final service has finished */
				pxEventConnection->xPrivate.ucServicesQueried++;
			}
			/* The current service that could still have characteristics on it */
			xGattService_t *pxCurrentService = &pxEventConnection->pxServices[pxEventConnection->xPrivate.ucServicesQueried];
			/* Loop over discovered characteristics, not garaunteed to contain all characteristics in a service, or to only contain characteristics from one service */
			pxEventConnection->xPrivate.ucIndex = 0;
			for ( uint8_t i = 0; i < pxGattChar->count; i++ ) {
				const ble_gattc_char_t *pxChar = &pxGattChar->chars[i];
				ucIndex						   = pxEventConnection->ucNumCharacteristics;
				/* Check against the storage space we have left */
				if ( ucIndex < BLUETOOTH_GATT_MAX_CHARACTERISTICS ) {
					/* Check if the characteristic handle falls outside the current service range */
					if ( pxChar->handle_value > pxCurrentService->uServiceReference.xHandleRange.usRangeStop ) {
						/* We have finished discovering the previous service */
						pxEventConnection->xPrivate.ucServicesQueried++;
						/* Reload the current service */
						pxCurrentService = &pxEventConnection->pxServices[pxEventConnection->xPrivate.ucServicesQueried];
					}
					/* Save the characteristic to the table */
					pxEventConnection->pxCharacteristics[ucIndex].uServiceReference.ulServiceHandle = pxCurrentService->uServiceReference.ulServiceHandle;
					pxEventConnection->pxCharacteristics[ucIndex].usCharacteristicHandle			= pxChar->handle_value;
					pxEventConnection->pxCharacteristics[ucIndex].usCCCDHandle						= 0;
					pxEventConnection->pxCharacteristics[ucIndex].eCharacteristicProperties			= prvCharacteristicPropertiesConversion( pxChar->char_props );
					pxEventConnection->pxCharacteristics[ucIndex].eCharacteristicProperties |= pxChar->char_ext_props ? BLE_CHARACTERISTIC_PROPERTY_EXTENDED : 0;
					if ( pxChar->uuid.type == BLE_UUID_TYPE_BLE ) {
						/* Normal 16-bit Bluetooth SIG UUID */
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.bBluetoothOfficialUUID = true;
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.usOfficialUUID   = pxChar->uuid.uuid;
					}
					else if ( pxChar->uuid.type == BLE_UUID_TYPE_UNKNOWN ) {
						/* We need to manually discover the 128bit UUID, silly Softdevice */
						pxEventConnection->pxServices[ucIndex].xUUID.bBluetoothOfficialUUID = false;
						/* We need to perform a secondary action (UUID discovery) on the characteristic declaration */
						pxEventConnection->xPrivate.ucIndex = ucIndex;
					}
					else {
						/* 128-bit UUID already exists in the stack, pull it out */
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.bBluetoothOfficialUUID			   = false;
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.xCustomUUID.ucStackReference = pxChar->uuid.type;
						vBluetoothStackUUIDResolve( &pxEventConnection->pxCharacteristics[ucIndex].xUUID );
						/* Set bytes 12 and 13 */
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128[12] = BYTE_READ( pxChar->uuid.uuid, 0 );
						pxEventConnection->pxCharacteristics[ucIndex].xUUID.uUUID.xCustomUUID.pucUUID128[13] = BYTE_READ( pxChar->uuid.uuid, 1 );
					}
					ucIndex = pxEventConnection->ucNumCharacteristics++;
				}
				else {
					eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Too many GATT Characteristics\r\n" );
				}
			}

			/* Next action to take */
			if ( pxEventConnection->xPrivate.ucIndex != 0 ) {
				/* Perform 128-bit UUID Discovery */
				prvBluetoothGattUUIDDiscover( pxEventConnection, pxEventConnection->pxCharacteristics[pxEventConnection->xPrivate.ucIndex].usCharacteristicHandle );
			}
			else {
				/* Continue GATT Discovery */
				prvBluetoothContinueGattDiscovery( pxEventConnection );
			}
			break;

		case BLE_GATTC_EVT_DESC_DISC_RSP:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT DESC DISC: %d %d\r\n", pxGattDesc->count, pxGattDesc->descs[0].handle );
			for ( uint8_t i = 0; i < pxGattDesc->count; i++ ) {
				const ble_gattc_desc_t *const pxDesc = &pxGattDesc->descs[i];
				eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "DESCRIPTOR: Handle: %d UUID: %d %02X\r\n", pxDesc->handle, pxDesc->uuid.type, pxDesc->uuid.uuid );
				/* Find the Client Characteristic Configuration Descriptor */
				if ( ( pxDesc->uuid.type == BLE_UUID_TYPE_BLE ) && ( pxDesc->uuid.uuid == BLE_ATTRIBUTE_TYPE_CLIENT_CHARACTERISTIC_CONFIGURATION ) ) {
					pxEventConnection->pxCharacteristics[pxEventConnection->xPrivate.ucIndex].usCCCDHandle = pxDesc->handle;
				}
			}
			prvBluetoothContinueGattDiscovery( pxEventConnection );
			break;

		case BLE_GATTC_EVT_ATTR_INFO_DISC_RSP:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "BT ATTR INFO: %d %d %d \r\n", pxGattInfo->count, pxGattInfo->format, pxGattInfo->info.attr_info128[0].uuid.uuid128[0] );
			/* Triggered because we needed to discover a 128bit UUID */
			for ( uint8_t i = 0; i < pxGattInfo->count; i++ ) {
				/* Add 128 bit UUID's to stack */
				if ( pxGattInfo->format == BLE_GATTC_ATTR_INFO_FORMAT_128BIT ) {
					uint8_t usReference;
					sd_ble_uuid_vs_add( &pxGattInfo->info.attr_info128[i].uuid, &usReference );
				}
			}
			break;

		case BLE_GATTC_EVT_READ_RSP:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "GATT READ: Handle %d, %d bytes\r\n", pxGattRead->handle, pxGattRead->len );
			if ( bDirectServiceRead ) {
				bDirectServiceRead = false;
				/* Add the UUID to the stack */
				ble_uuid128_t xUUIDBase;
				uint8_t		  ucReference;
				pvMemcpy( xUUIDBase.uuid128, pxGattRead->data, pxGattRead->len );
				configASSERT( sd_ble_uuid_vs_add( &xUUIDBase, &ucReference ) == NRF_SUCCESS );
				/* If we were discovering a service */
				xBluetoothUUID_t *pxUUID;
				if ( pxEventConnection->xPrivate.ucServicesQueried == 0 ) {
					pxUUID = &pxEventConnection->pxServices[pxEventConnection->xPrivate.ucIndex].xUUID;
				}
				/* If we were discovering a characteristic */
				else {
					pxUUID = &pxEventConnection->pxCharacteristics[pxEventConnection->xPrivate.ucIndex].xUUID;
				}
				/* Store the UUID */
				pvMemcpy( pxUUID->uUUID.xCustomUUID.pucUUID128, pxGattRead->data, pxGattRead->len );
				pxUUID->uUUID.xCustomUUID.ucStackReference = ucReference;
				/* We should now continue where we left off with GATT Discovery */
				prvBluetoothContinueGattDiscovery( pxEventConnection );
			}
			else {
				pxRemote = pxBluetoothSearchCharacteristicHandle( pxEventConnection, pxGattRead->handle );
				if ( pxRemote != NULL ) {
					pxRemote->pucData   = pxGattRead->data;
					pxRemote->usDataLen = pxGattRead->len;

					xCallback.eCallback		   = STACK_CALLBACK_REMOTE_READ;
					xCallback.pxConnection	 = pxEventConnection;
					xCallback.xParams.pxRemote = pxRemote;
					vBluetoothControllerCallbackRun( &xCallback );
				}
				else {
					eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "Couldn't find characteristic\r\n" );
				}
			}
			break;
		case BLE_GATTC_EVT_HVX:
			/* Confirm Indication */
			if ( pxGattHvx->type == BLE_GATT_HVX_INDICATION ) {
				sd_ble_gattc_hv_confirm( pxEventConnection->ucConnectionHandle, pxGattHvx->handle );
			}
			/* Find the characteristic that changed in the remote table */
			pxRemote = pxBluetoothSearchCharacteristicHandle( pxEventConnection, pxGattHvx->handle );
			if ( pxRemote != NULL ) {
				pxRemote->pucData   = pxGattHvx->data;
				pxRemote->usDataLen = pxGattHvx->len;

				xCallback.eCallback		   = STACK_CALLBACK_REMOTE_CHANGED;
				xCallback.pxConnection	 = pxEventConnection;
				xCallback.xParams.pxRemote = pxRemote;
				vBluetoothControllerCallbackRun( &xCallback );
			}
			else {
				eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "Couldn't find characteristic\r\n" );
			}
			break;

		case BLE_GATTC_EVT_WRITE_RSP:
			/* Remote GATT operation complete */
			eLog( LOG_BLUETOOTH_GATT, LOG_VERBOSE, "Write Operation %d on handle %d completed with code %d\r\n", pxWriteResp->write_op, pxWriteResp->handle, pxEvent->evt.gattc_evt.gatt_status );
			/* Write result back*/
			pxEventConnection->xPrivate.eError = pxEvent->evt.gattc_evt.gatt_status == BLE_GATT_STATUS_SUCCESS ? ERROR_NONE : ERROR_GENERIC;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			break;

		case BLE_GATTC_EVT_WRITE_CMD_TX_COMPLETE:
			/* Transmission complete, no possibility of error */
			pxEventConnection->xPrivate.eError = ERROR_NONE;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			break;

		case BLE_GATTS_EVT_HVN_TX_COMPLETE:
			/* Transmission complete, no possibility of error */
			pxEventConnection->xPrivate.eError = ERROR_NONE;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			break;

		case BLE_GATTS_EVT_HVC:
			/* Client acknowledged notification/indication */
			pxEventConnection->xPrivate.eError = ERROR_NONE;
			xEventGroupSetBits( pxEventConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE );
			break;

		case BLE_GATTS_EVT_WRITE:
			eLog( LOG_BLUETOOTH_GATT, LOG_DEBUG, "GATT Write: Handle %d, %d bytes\r\n", pxGattWrite->handle, pxGattWrite->len );

			xGattLocalCharacteristic_t xLocal = { 0 };
			/* Handle CCCD writes separately */
			if ( ( pxGattWrite->uuid.type == BLE_UUID_TYPE_BLE ) && ( pxGattWrite->uuid.uuid == BLE_ATTRIBUTE_TYPE_CLIENT_CHARACTERISTIC_CONFIGURATION ) ) {
				xCallback.eCallback = STACK_CALLBACK_LOCAL_SUBSCRIBED;
				xLocal.usCCCDValue  = LE_U16_EXTRACT( pxGattWrite->data );
				uint8_t i			= 0;
				/* Find the last handle before the CCCD handle */
				while ( ppusGattProfileHandles[i] != NULL ) {
					uint16_t *pusHandle		= ppusGattProfileHandles[i];
					uint16_t *pusNextHandle = ppusGattProfileHandles[i + 1];
					/* There are no more handles, or the next handle is larger than the attribute handle */
					if ( ( pusNextHandle == NULL ) || ( *pusNextHandle > pxGattWrite->handle ) ) {
						xLocal.usCharacteristicHandle = *pusHandle;
						break;
					}
					i++;
				}
			}
			else {
				xCallback.eCallback			  = STACK_CALLBACK_LOCAL_WRITTEN;
				xLocal.usCharacteristicHandle = pxGattWrite->handle;
				xLocal.pucData				  = pxGattWrite->data;
				xLocal.usDataLen			  = pxGattWrite->len;
			}
			/* Run the callback */
			xCallback.pxConnection	= pxEventConnection;
			xCallback.xParams.pxLocal = &xLocal;
			vBluetoothControllerCallbackRun( &xCallback );
			break;
		case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
			eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Check and Handle update\r\n" );
			sd_ble_gap_phy_update( pxEvent->evt.gap_evt.conn_handle, &pxEvent->evt.gap_evt.params.phy_update_request.peer_preferred_phys );
			break;
		case BLE_GAP_EVT_PHY_UPDATE:
			eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Update Checked and Completed\r\n" );
			break;
		default:
			eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT: Unhandled EVENT %d\r\n", ulEventId );
			break;
	}
}

/*-----------------------------------------------------------*/

void vBluetoothGattRegisterInitiatedConnection( xBluetoothConnection_t *pxContext )
{
	pxContextForInitiatedConnection = pxContext;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBluetoothContinueGattDiscovery( xBluetoothConnection_t *pxContext )
{
	xStackCallback_t xCallback;
	bool			 bDiscoveryComplete = true;
	/* If we are still discovering services */
	uint16_t usLastServiceHandle = pxContext->pxServices[pxContext->ucNumServices - 1].uServiceReference.xHandleRange.usRangeStop;
	if ( usLastServiceHandle < 0xFFFF ) {
		/* We still have services left to discover */
		prvBluetoothGattServiceDiscovery( pxContext, usLastServiceHandle + 1 );
	}
	/* Services are done, no characteristics to discover */
	else if ( pxContext->ucNumServices == 0 ) {
		/* We are done */
		xCallback.pxConnection = pxContext;
		xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
		vBluetoothControllerCallbackRun( &xCallback );
	}
	/* Service discovery is complete, characteristics not complete */
	else if ( pxContext->xPrivate.ucServicesQueried != pxContext->ucNumServices ) {
		uServiceReference_t xCharRange;
		/* First characteristic starts on first service, later characteristics start after previously discovered characteristic */
		xCharRange.xHandleRange.usRangeStart = ( pxContext->ucNumCharacteristics == 0 ) ? pxContext->pxServices[0].uServiceReference.xHandleRange.usRangeStart : pxContext->pxCharacteristics[pxContext->ucNumCharacteristics - 1].usCharacteristicHandle + 1;
		xCharRange.xHandleRange.usRangeStop  = UINT16_MAX;
		prvBluetoothGattCharacteristicDiscovery( pxContext, xCharRange );
	}
	/* Finally, ensure CCCD attributes are discovered for required characteristics */
	else {
		/* Scan through characteristics for undiscovered CCCD handles */
		for ( uint8_t i = 0; i < pxContext->ucNumCharacteristics; i++ ) {
			xGattRemoteCharacteristic_t *pxChar = &pxContext->pxCharacteristics[i];
			/* If we expect a CCCD handle, and its not found */
			if ( ( pxChar->eCharacteristicProperties & ( BLE_CHARACTERISTIC_PROPERTY_NOTIFY | BLE_CHARACTERISTIC_PROPERTY_INDICATE ) ) && ( pxChar->usCCCDHandle == 0 ) ) {
				prvBluetoothGattCCCDDiscovery( pxContext, i );
				bDiscoveryComplete = false;
				break;
			}
		}
		/* We have completed discovery */
		if ( bDiscoveryComplete ) {
			xCallback.pxConnection = pxContext;
			xCallback.eCallback	= STACK_CALLBACK_CONNECTED;
			vBluetoothControllerCallbackRun( &xCallback );
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBluetoothGattServiceDiscovery( xBluetoothConnection_t *pxContext, uint16_t usStartHandle )
{
	ret_code_t ulError;

	ulError = sd_ble_gattc_primary_services_discover( pxContext->ucConnectionHandle, usStartHandle, NULL );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Discover Services Error: 0x%X\r\n", ulError );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBluetoothGattCharacteristicDiscovery( xBluetoothConnection_t *pxContext, uServiceReference_t uService )
{
	ble_gattc_handle_range_t xHandles;
	ret_code_t				 ulError;

	xHandles.start_handle = uService.xHandleRange.usRangeStart;
	xHandles.end_handle   = uService.xHandleRange.usRangeStop;

	ulError = sd_ble_gattc_characteristics_discover( pxContext->ucConnectionHandle, &xHandles );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Discover Characteristics Error: 0x%X\r\n", ulError );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBluetoothGattCCCDDiscovery( xBluetoothConnection_t *pxContext, uint8_t ucIndex )
{
	xGattRemoteCharacteristic_t *const pxChar = &pxContext->pxCharacteristics[ucIndex];
	ble_gattc_handle_range_t		   xHandles;
	ret_code_t						   ulError;
	/* Discover all descriptors associated with this characteristic */
	xHandles.start_handle = pxChar->usCharacteristicHandle + 1;
	if ( ucIndex == ( pxContext->ucNumCharacteristics - 1 ) ) {
		/* Discover to the end of the range */
		xHandles.end_handle = UINT16_MAX;
	}
	else {
		/* Discover to the next characteristic */
		xHandles.end_handle = pxContext->pxCharacteristics[ucIndex + 1].usCharacteristicHandle;
	}
	pxContext->xPrivate.ucIndex = ucIndex;

	ulError = sd_ble_gattc_descriptors_discover( pxContext->ucConnectionHandle, &xHandles );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Discover Descriptors Error: 0x%X\r\n", ulError );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvBluetoothGattUUIDDiscover( xBluetoothConnection_t *pxContext, uint16_t usAttributeHandle )
{
	ret_code_t ulError;
	/* 
	 * Really should be using sd_ble_gattc_attr_info_discover, but for whatever reason the UUIDs returned are always 16bit 
	 * Instead we'll just read the service characteristic directly
	 */
	bDirectServiceRead = true;
	ulError			   = sd_ble_gattc_read( pxContext->ucConnectionHandle, usAttributeHandle, 0 );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Discover UUID Error: 0x%X\r\n", ulError );
		return ERROR_INVALID_STATE;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

int16_t sBluetoothGattConnectionRssi( xBluetoothConnection_t *pxContext )
{
	ret_code_t ulError;
	int8_t	 cRssi;
	uint8_t	ucChannel;
	ulError = sd_ble_gap_rssi_get( pxContext->ucConnectionHandle, &cRssi, &ucChannel );
	if ( ulError != NRF_SUCCESS ) {
		return INT16_MIN;
	}
	/* Convert to deci dBm */
	return 10 * ( (int16_t) cRssi );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattLocalWrite( xGattLocalCharacteristic_t *pxCharacteristic )
{
	ret_code_t		  ulError;
	ble_gatts_value_t xChar;

	xChar.len	= pxCharacteristic->usDataLen;
	xChar.offset = 0;

	/* Because we are writing the characteristic, pxCharacteristic->pucData wont be modified */
	COMPILER_WARNING_DISABLE( "-Wdiscarded-qualifiers" );
	xChar.p_value = pxCharacteristic->pucData;
	COMPILER_WARNING_ENABLE();

	ulError = sd_ble_gatts_value_set( BLE_CONN_HANDLE_INVALID, pxCharacteristic->usCharacteristicHandle, &xChar );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Gatts Write: 0x%X %d\r\n", ulError, xChar.len );
		return ERROR_INVALID_DATA;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattLocalDistribute( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic )
{
	ret_code_t ulError;

	ble_gatts_hvx_params_t xParams = {
		.handle = pxCharacteristic->usCharacteristicHandle,
		.type   = ( pxCharacteristic->usCCCDValue & BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_NOTIFICATION ) ? BLE_GATT_HVX_NOTIFICATION : BLE_GATT_HVX_INDICATION,
		.offset = 0,
		.p_len  = &pxCharacteristic->usDataLen,
		.p_data = pxCharacteristic->pucData
	};

	/* Start GATT procedure */
	ulError = sd_ble_gatts_hvx( pxConnection->ucConnectionHandle, &xParams );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Gatts HVX: 0x%X\r\n", ulError );
		return ERROR_INVALID_DATA;
	}
	/* Wait for the error code from the stack */
	xEventGroupWaitBits( pxConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE, pdTRUE, pdTRUE, portMAX_DELAY );
	return (eModuleError_t) pxConnection->xPrivate.eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattRemoteWrite( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, eGattWriteOptions_t eOptions )
{
	ret_code_t ulError;

	ble_gattc_write_params_t xParams = {
		.write_op = ( eOptions & GATT_WRITE_RESPONSE ) ? BLE_GATT_OP_WRITE_REQ : BLE_GATT_OP_WRITE_CMD,
		.flags	= BLE_GATT_EXEC_WRITE_FLAG_PREPARED_WRITE,
		.handle   = pxCharacteristic->usCharacteristicHandle,
		.offset   = 0,
		.len	  = pxCharacteristic->usDataLen,
		.p_value  = pxCharacteristic->pucData
	};
	/* Start GATT procedure */
	ulError = sd_ble_gattc_write( pxConnection->ucConnectionHandle, &xParams );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Gattc Write %d: 0x%X\r\n", xParams.len, ulError );
		return ERROR_INVALID_DATA;
	}
	/* Wait for the error code from the stack */
	xEventGroupWaitBits( pxConnection->xConnectionState, BT_CONNECTION_OPERATION_DONE, pdTRUE, pdTRUE, portMAX_DELAY );
	return (eModuleError_t) pxConnection->xPrivate.eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGattRemoteRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic )
{
	ret_code_t ulError;

	ulError = sd_ble_gattc_read( pxConnection->ucConnectionHandle, pxCharacteristic->usCharacteristicHandle, 0 );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GATT, LOG_ERROR, "BT Gattc READ: 0x%X\r\n", ulError );
		return ERROR_INVALID_DATA;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static inline uint8_t prvCharacteristicPropertiesConversion( ble_gatt_char_props_t xStackProperties )
{
	return ( xStackProperties.broadcast ? BLE_CHARACTERISTIC_PROPERTY_BROADCAST : 0 ) |
		   ( xStackProperties.read ? BLE_CHARACTERISTIC_PROPERTY_READ : 0 ) |
		   ( xStackProperties.write_wo_resp ? BLE_CHARACTERISTIC_PROPERTY_WRITE_NO_RESPONSE : 0 ) |
		   ( xStackProperties.write ? BLE_CHARACTERISTIC_PROPERTY_WRITE : 0 ) |
		   ( xStackProperties.notify ? BLE_CHARACTERISTIC_PROPERTY_NOTIFY : 0 ) |
		   ( xStackProperties.indicate ? BLE_CHARACTERISTIC_PROPERTY_INDICATE : 0 ) |
		   ( xStackProperties.auth_signed_wr ? BLE_CHARACTERISTIC_PROPERTY_AUTH_SIGNED_WRITE : 0 );
}

/*-----------------------------------------------------------*/
