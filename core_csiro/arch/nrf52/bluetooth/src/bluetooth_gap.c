/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * Advertising can take place at the same time as scanning, however 
 * this comes at the cost of variable transmit timing.
 * 
 * Current behaviour appears to be that the first transmission of 
 * an advertising set is sent at the end of the current scan window.
 * 
 * Therefore for RPC's with a scan window of 2s, the turnaround time
 * if anywhere between 0 and 4 seconds.
 * 
 * Reducing the scan window reduces this variability, but increases
 * the amount of time the radio isn't listening, as the radio is off
 * while switching channels, and any partially received advertisments
 * are discarded.
 * 
 * Currently we are just forcing scan to be off while advertising,
 * which forces immediate transmission at the cost of being unable to
 * receive between transmissions in a set. This is negated by only
 * sending each set once.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "log.h"
#include "rtc.h"

#include "bluetooth.h"
#include "bluetooth_controller.h"
#include "bluetooth_gap.h"

#include "ble.h"
#include "ble_gap.h"
#include "ble_types.h"

/* Private Defines ------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static uint8_t pucBluetoothRawAdvertisingPacket[BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH];
static uint8_t pucScanningData[BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH];

static ble_gap_scan_params_t xScanParams = { 0 };

/* Default connection parameters */
static ble_gap_conn_params_t xConnParams = {
	.min_conn_interval = 35, /**< Minimum Connection Interval in 1.25 ms units, see @ref BLE_GAP_CP_LIMITS.*/
	.max_conn_interval = 45, /**< Maximum Connection Interval in 1.25 ms units, see @ref BLE_GAP_CP_LIMITS.*/
	.slave_latency	 = 0,  /**< Slave Latency in number of connection events, see @ref BLE_GAP_CP_LIMITS.*/
	.conn_sup_timeout  = 50  /**< Connection Supervision Timeout in 10 ms units, see @ref BLE_GAP_CP_LIMITS.*/
};

static fnScanRecv_t fnScanCallback = NULL;

/*-----------------------------------------------------------*/

void prvBluetoothGapEventHandler( ble_evt_t const *pxEvent )
{
	const uint32_t ulEventId = pxEvent->header.evt_id;
	/* Pre cast all our events types for simplicity later */
	const ble_gap_evt_adv_report_t *pxAdvReport = &pxEvent->evt.gap_evt.params.adv_report;
	const ble_gap_evt_connected_t * pxConnected = &pxEvent->evt.gap_evt.params.connected;

	switch ( ulEventId ) {
		case BLE_GAP_EVT_ADV_REPORT:
			/* Process the Advertising report */
			if ( fnScanCallback != NULL ) {
				fnScanCallback( pxAdvReport->peer_addr.addr, pxAdvReport->peer_addr.addr_type,
								pxAdvReport->rssi, pxAdvReport->type.connectable, pxAdvReport->data.p_data, pxAdvReport->data.len );
			}
			/* Scanning is paused on receipt of advertising data, restart it here */
			if ( pxAdvReport->type.status != BLE_GAP_ADV_DATA_STATUS_INCOMPLETE_MORE_DATA ) {
				if ( eBluetoothGapScanResume() != ERROR_NONE ) {
					eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to resume scanning\r\n" );
				}
			}
			break;
		case BLE_GAP_EVT_ADV_SET_TERMINATED:
			vBluetoothControllerAdvertisingComplete();
			break;
		case BLE_GAP_EVT_CONNECTED:
			/* BLE_GAP_EVT_ADV_SET_TERMINATED is not generated when advertising leads to a connection, so if we were advertising manually run the callback */
			if ( pxConnected->role == BLE_GAP_ROLE_PERIPH ) {
				vBluetoothControllerAdvertisingComplete();
			}
			break;
		default:
			break;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothSetLocalAddress( xBluetoothAddress_t *pxAddress )
{
	ble_gap_addr_t xAddress;
	uint32_t	   ulError;

	xAddress.addr_id_peer = 0;
	xAddress.addr_type	= pxAddress->eAddressType;
	pvMemcpy( xAddress.addr, pxAddress->pucAddress, BLUETOOTH_MAC_ADDRESS_LENGTH );

	ulError = sd_ble_gap_addr_set( &xAddress );
	return ( ulError == NRF_SUCCESS ) ? ERROR_NONE : ERROR_INVALID_ADDRESS;
}

/*-----------------------------------------------------------*/

void vBluetoothGetLocalAddress( xBluetoothAddress_t *pxLocalAddress )
{
	ble_gap_addr_t xAddress;
	sd_ble_gap_addr_get( &xAddress );
	pxLocalAddress->eAddressType = xAddress.addr_type;
	pvMemcpy( pxLocalAddress->pucAddress, xAddress.addr, BLUETOOTH_MAC_ADDRESS_LENGTH );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanConfigure( xGapScanParameters_t *pxScanParams )
{
	uint16_t usScanInterval;
	uint16_t usScanWindow;
	/* Convert scan parameters from ms to function args */
	/* 1 function unit is 0.625ms, which is 5/8, so multiply by 8/5 */
	/* Also check that our ms value will fit in 16 bit number */
	configASSERT( pxScanParams->usScanIntervalMs < 40960 );
	configASSERT( pxScanParams->usScanWindowMs < 40960 );
	usScanInterval = ( uint16_t )( ( (uint32_t) pxScanParams->usScanIntervalMs * 8 ) / 5 );
	usScanWindow   = ( uint16_t )( ( (uint32_t) pxScanParams->usScanWindowMs * 8 ) / 5 );

	/* Setup Scan Parameters */
	xScanParams.extended			   = 0;
	xScanParams.report_incomplete_evts = 0;
	xScanParams.active				   = pxScanParams->bActiveScanning;
	xScanParams.filter_policy		   = BLE_GAP_SCAN_FP_ACCEPT_ALL;
	xScanParams.scan_phys			   = BLE_GAP_PHY_1MBPS;
	xScanParams.interval			   = usScanInterval;
	xScanParams.window				   = usScanWindow;
	xScanParams.timeout				   = 0;

	fnScanCallback = pxScanParams->fnCallback;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapConnectionParameters( xGapConnectionParameters_t *pxConnectionParameters )
{
	xConnParams.conn_sup_timeout  = pxConnectionParameters->usSupervisorTimeoutMs / 10;
	xConnParams.slave_latency	 = pxConnectionParameters->usSlaveLatency;
	xConnParams.max_conn_interval = pxConnectionParameters->usConnectionInterval;
	xConnParams.min_conn_interval = pxConnectionParameters->usConnectionInterval;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanStart( eBluetoothPhy_t ePHY )
{
	ble_data_t xScanningData;
	uint32_t   ulError;

	/* Saving configuration across multiple PHY's not implemented yet */
	configASSERT( ePHY == xScanParams.scan_phys );

	xScanningData.p_data = pucScanningData;
	xScanningData.len	= BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH;

	/* Start the Scan */
	ulError = sd_ble_gap_scan_start( &xScanParams, &xScanningData );
	if ( ulError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to start scan 0x%X\r\n", ulError );
		return ( ulError == NRF_ERROR_INVALID_STATE ) ? ERROR_INVALID_STATE : ERROR_INVALID_DATA;
	}

	xDateTime_t xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "BT %2d.%05d: Scan started\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanResume( void )
{
	ble_data_t xScanningData;
	xScanningData.p_data = pucScanningData;
	xScanningData.len	= BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH;
	return ( sd_ble_gap_scan_start( NULL, &xScanningData ) == NRF_SUCCESS ) ? ERROR_NONE : ERROR_INVALID_STATE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanStop( void )
{
	eModuleError_t eError = ( sd_ble_gap_scan_stop() == NRF_SUCCESS ) ? ERROR_NONE : ERROR_INVALID_STATE;
	if ( eError == ERROR_NONE ) {
		xDateTime_t xDatetime;
		bRtcGetDatetime( &xDatetime );
		eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "BT %2d.%05d: Scan stopped\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	}
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapAdvertise( xGapAdvertiseParameters_t *pxParams )
{
	static uint8_t ucAdvertisingHandle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;

	xDateTime_t xDatetime;
	ret_code_t  eError;

	ble_gap_adv_data_t   xAdvData   = { 0 };
	ble_gap_adv_params_t xAdvParams = { 0 };

	uint32_t ulAdvertisingInterval;

	/* Validate Data Length (Legacy Advertising only for now) */
	configASSERT( pxParams->ucDataLen <= BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH );

	pvMemcpy( pucBluetoothRawAdvertisingPacket, pxParams->pucData, pxParams->ucDataLen );

	/* Convert interval from ms to function args */
	/* 1 function unit is 0.625ms, which is 5/8, so multiply by 8/5 */
	ulAdvertisingInterval = ( ( (uint32_t) pxParams->usAdvertisePeriodMs ) * 8 ) / 5;

	/* Setup Advertising Parameters */
	xAdvParams.properties.type			   = pxParams->eType;
	xAdvParams.properties.anonymous		   = 0;
	xAdvParams.properties.include_tx_power = 0;
	xAdvParams.p_peer_addr				   = NULL;
	xAdvParams.interval					   = (uint16_t) ulAdvertisingInterval;
	xAdvParams.duration					   = 0; // Rely on advertise count to terminate
	xAdvParams.max_adv_evts				   = pxParams->ucAdvertiseCount;
	xAdvParams.filter_policy			   = BLE_GAP_ADV_FP_ANY;
	xAdvParams.primary_phy				   = BLE_GAP_PHY_1MBPS;

	xAdvData.adv_data.p_data = pucBluetoothRawAdvertisingPacket;
	xAdvData.adv_data.len	= pxParams->ucDataLen;

	/* Configure the Advertising Set */
	eError = sd_ble_gap_adv_set_configure( &ucAdvertisingHandle, &xAdvData, &xAdvParams );
	if ( eError != NRF_SUCCESS ) {
		bRtcGetDatetime( &xDatetime );
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to set configure advertising set %d with 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, ucAdvertisingHandle, eError );
		return ERROR_INVALID_DATA;
	}

	/* Set TX Power for the set */
	eError = sd_ble_gap_tx_power_set( BLE_GAP_TX_POWER_ROLE_ADV, ucAdvertisingHandle, pxParams->cTransmitPowerDbm );
	if ( eError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set TX power 0x%X\r\n", eError );
		return ERROR_INVALID_DATA;
	}

	/* Start the advertising set */
	eError = sd_ble_gap_adv_start( ucAdvertisingHandle, CSIRO_CONNECTION_TAG );
	if ( eError != NRF_SUCCESS ) {
		bRtcGetDatetime( &xDatetime );
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to start advertising 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, eError );
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Advertising Started, Period %dms, Count %d\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, pxParams->usAdvertisePeriodMs, pxParams->ucAdvertiseCount );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapConnect( xBluetoothConnection_t *pxConnection )
{
	ble_gap_addr_t xRemote;
	xDateTime_t	xDatetime;
	ret_code_t	 eError;

	xRemote.addr_id_peer = false;
	xRemote.addr_type	= pxConnection->xRemoteAddress.eAddressType;
	pvMemcpy( xRemote.addr, pxConnection->xRemoteAddress.pucAddress, BLUETOOTH_MAC_ADDRESS_LENGTH );

	pxConnection->ucConnectionHandle = UINT8_MAX;

	eError = sd_ble_gap_connect( &xRemote, &xScanParams, &xConnParams, CSIRO_CONNECTION_TAG );
	if ( eError != NRF_SUCCESS ) {
		bRtcGetDatetime( &xDatetime );
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to initiate connection 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, eError );
		return ERROR_UNAVAILABLE_RESOURCE;
	}

	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Connection initiated to %:6R\r\n",
		  xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction,
		  xRemote.addr );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapDisconnect( xBluetoothConnection_t *pxConnection )
{
	xDateTime_t xDatetime;
	ret_code_t  eError;

	if ( pxConnection->ucConnectionHandle == UINT8_MAX ) {
		/* Connection has not yet been established */
		eError = sd_ble_gap_connect_cancel();
	}
	else {
		/* Connection has been established */
		eError = sd_ble_gap_disconnect( pxConnection->ucConnectionHandle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION );
	}

	bRtcGetDatetime( &xDatetime );

	if ( eError != NRF_SUCCESS ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to initiate disconnection 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, eError );
		return ERROR_INVALID_STATE;
	}
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Disconnection initiated\r\n",
		  xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
