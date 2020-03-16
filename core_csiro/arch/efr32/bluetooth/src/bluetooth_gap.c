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

#include "bluetooth_gap.h"
#include "bluetooth_controller.h"

#include "rtos_gecko.h"

#include "log.h"
#include "memory_operations.h"
#include "rtc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static fnScanRecv_t fnScanCallback = NULL;

/*-----------------------------------------------------------*/

void vBluetoothGapEventHandler( struct gecko_cmd_packet *pvEventData )
{
	const uint32_t ulEventId = BGLIB_MSG_ID( pvEventData->header );

	struct gecko_msg_le_gap_scan_response_evt_t *pxScanResponse = &pvEventData->data.evt_le_gap_scan_response;
	struct gecko_msg_le_connection_opened_evt_t *pxConnOpened   = &pvEventData->data.evt_le_connection_opened;

	switch ( ulEventId ) {
		case gecko_evt_le_gap_scan_response_id:
			if ( fnScanCallback != NULL ) {
				fnScanCallback( pxScanResponse->address.addr, pxScanResponse->address_type,
								pxScanResponse->rssi, ( ( pxScanResponse->packet_type == 0x00 ) ? true : false ), pxScanResponse->data.data, pxScanResponse->data.len );
			}
			break;
		case gecko_evt_le_gap_adv_timeout_id:
			vBluetoothControllerAdvertisingComplete();
			break;
		case gecko_evt_le_connection_opened_id:
			/* If our advertising led to the connection, we need to manually run our advertising complete callback */
			if ( pxConnOpened->master == false ) {
				vBluetoothControllerAdvertisingComplete();
			}
			break;
		default:
			break;
	}
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothSetLocalAddress( xBluetoothAddress_t *pxLocalAddress )
{
	struct gecko_msg_system_set_bt_address_rsp_t *pxBtAddress;
	bd_addr										  xAddress;

	pvMemcpy( xAddress.addr, pxLocalAddress->pucAddress, BLUETOOTH_MAC_ADDRESS_LENGTH );
	/* New address won't be used until the next reboot */
	pxBtAddress = gecko_cmd_system_set_bt_address( xAddress );

	return ( pxBtAddress->result == 0 ) ? ERROR_NONE : ERROR_INVALID_ADDRESS;
}

/*-----------------------------------------------------------*/

void vBluetoothGetLocalAddress( xBluetoothAddress_t *pxLocalAddress )
{
	struct gecko_msg_system_get_bt_address_rsp_t *pxBtAddress;
	pxBtAddress = gecko_cmd_system_get_bt_address();

	pxLocalAddress->eAddressType = BLUETOOTH_ADDR_TYPE_PUBLIC;
	pvMemcpy( pxLocalAddress->pucAddress, pxBtAddress->address.addr, BLUETOOTH_MAC_ADDRESS_LENGTH );
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanConfigure( xGapScanParameters_t *pxScanParams )
{
	struct gecko_msg_le_gap_set_discovery_timing_rsp_t *pxTimingResponse;
	struct gecko_msg_le_gap_set_discovery_type_rsp_t *  pxTypeResponse;

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
	pxTimingResponse = gecko_cmd_le_gap_set_discovery_timing( pxScanParams->ePhy, usScanInterval, usScanWindow );
	if ( pxTimingResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set discovery timing 0x%X\r\n", pxTimingResponse->result );
		return ERROR_INVALID_DATA;
	}
	pxTypeResponse = gecko_cmd_le_gap_set_discovery_type( pxScanParams->ePhy, pxScanParams->bActiveScanning );
	if ( pxTypeResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set discovery type 0x%X\r\n", pxTypeResponse->result );
		return ERROR_INVALID_DATA;
	}

	fnScanCallback = pxScanParams->fnCallback;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapConnectionParameters( xGapConnectionParameters_t *pxConnectionParameters )
{
	struct gecko_msg_le_gap_set_conn_parameters_rsp_t *pxConnResponse;
	pxConnResponse = gecko_cmd_le_gap_set_conn_parameters( pxConnectionParameters->usConnectionInterval,
														   pxConnectionParameters->usConnectionInterval,
														   pxConnectionParameters->usSlaveLatency,
														   pxConnectionParameters->usSupervisorTimeoutMs / 10 );
	if ( pxConnResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set connection parameters 0x%X\r\n", pxConnResponse->result );
		return ERROR_INVALID_DATA;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanStart( eBluetoothPhy_t ePHY )
{
	struct gecko_msg_le_gap_start_discovery_rsp_t *pxStartResponse;

	/* Start the scan */
	pxStartResponse = gecko_cmd_le_gap_start_discovery( ePHY, le_gap_discover_observation );
	if ( pxStartResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to start scan 0x%X\r\n", pxStartResponse->result );
		return ERROR_INVALID_STATE;
	}
	xDateTime_t xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "BT %2d.%05d: Scanning started\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanResume( void )
{
	/* EFR32 Bluetooth stack doesn't pause scanning, resuming has no meaning */
	configASSERT( 0 );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapScanStop( void )
{
	struct gecko_msg_le_gap_end_procedure_rsp_t *xEndResp;
	xEndResp = gecko_cmd_le_gap_end_procedure();
	xDateTime_t xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_DEBUG, "BT %2d.%05d: Scan stopped\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	return ( xEndResp->result == 0 ) ? ERROR_NONE : ERROR_INVALID_STATE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapAdvertise( xGapAdvertiseParameters_t *pxParams )
{
	struct gecko_msg_le_gap_set_advertise_phy_rsp_t *	 pxPhyResponse;
	struct gecko_msg_le_gap_set_advertise_timing_rsp_t *  pxTimeResp;
	struct gecko_msg_le_gap_set_advertise_tx_power_rsp_t *pxPowerResp;
	struct gecko_msg_le_gap_bt5_set_adv_data_rsp_t *	  pxDataResp;
	struct gecko_msg_le_gap_start_advertising_rsp_t *	 pxStartResp;

	uint16_t usAdvertiseDuration;
	uint32_t ulAdvertisingInterval;
	/* Validate Data Length (Legacy Advertising only for now) */
	configASSERT( pxParams->ucDataLen <= BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH );

	/* Convert interval from ms to function args */
	/* 1 function unit is 0.625ms, which is 5/8, so multiply by 8/5 */
	ulAdvertisingInterval = ( ( (uint32_t) pxParams->usAdvertisePeriodMs ) * 8 ) / 5;
	/* Advertising duration is in multiples of 10ms  */
	usAdvertiseDuration = ( pxParams->usAdvertisePeriodMs * pxParams->ucAdvertiseCount ) / 10;

	/* Setup Advertising PHY */
	pxPhyResponse = gecko_cmd_le_gap_set_advertise_phy( 0, pxParams->ePhy, pxParams->ePhy );
	if ( pxPhyResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set advertising PHY %d with error 0x%X\r\n", pxParams->ePhy, pxPhyResponse->result );
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	/* Setup Advertising Parameters */
	pxTimeResp = gecko_cmd_le_gap_set_advertise_timing( 0, ulAdvertisingInterval - 5, ulAdvertisingInterval + 5, usAdvertiseDuration, pxParams->ucAdvertiseCount );
	if ( pxTimeResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set advertising timing %d with error 0x%X\r\n", ulAdvertisingInterval, pxTimeResp->result );
		return ERROR_INVALID_DATA;
	}
	/* Set advertising TX power, parameter is in 0.1dBm steps */
	pxPowerResp = gecko_cmd_le_gap_set_advertise_tx_power( 0, 10 * pxParams->cTransmitPowerDbm );
	if ( pxPowerResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set advertising power %d with error 0x%X\r\n", 10 * pxParams->cTransmitPowerDbm, pxPowerResp->result );
		return ERROR_INVALID_DATA;
	}
	/* Set the output data, data is copied to stack and does not need to be preserved after this function call */
	pxDataResp = gecko_cmd_le_gap_bt5_set_adv_data( 0, 0, pxParams->ucDataLen, pxParams->pucData );
	if ( pxDataResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to set advertising data 0x%X\r\n", pxDataResp->result );
		return ERROR_INVALID_DATA;
	}

	eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "BT ADV Data: % *A\r\n", pxParams->ucDataLen, pxParams->pucData );

	/* Start the advertising sequence */
	pxStartResp = gecko_cmd_le_gap_start_advertising( 0, le_gap_user_data, pxParams->eType );
	if ( pxStartResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT: Failed to start advertising 0x%X\r\n", pxStartResp->result );
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	xDateTime_t xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Advertising Started, Period %dms, Count %d\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, pxParams->usAdvertisePeriodMs, pxParams->ucAdvertiseCount );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapConnect( xBluetoothConnection_t *pxConnection )
{
	struct gecko_msg_le_gap_connect_rsp_t *pxConnResponse;
	bd_addr								   xRemote;
	xDateTime_t							   xDatetime;

	pvMemcpy( xRemote.addr, pxConnection->xRemoteAddress.pucAddress, BLUETOOTH_MAC_ADDRESS_LENGTH );
	bRtcGetDatetime( &xDatetime );

	pxConnResponse = gecko_cmd_le_gap_connect( xRemote, pxConnection->xRemoteAddress.eAddressType, BLUETOOTH_PHY_1M );
	if ( pxConnResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to initiate connection 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, pxConnResponse->result );
		return ERROR_INVALID_STATE;
	}
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Connection initiated to %:6R\r\n",
		  xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, xRemote.addr );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothGapDisconnect( xBluetoothConnection_t *pxConnection )
{
	struct gecko_msg_le_connection_close_rsp_t *pxDisconnResponse;
	xDateTime_t									xDatetime;

	pxDisconnResponse = gecko_cmd_le_connection_close( pxConnection->ucConnectionHandle );
	bRtcGetDatetime( &xDatetime );

	if ( pxDisconnResponse->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_ERROR, "BT %2d.%05d: Failed to initiate disconnection 0x%X\r\n", xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, pxDisconnResponse->result );
		return ERROR_INVALID_STATE;
	}
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "BT %2d.%05d: Disconnection initiated\r\n",
		  xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
