/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include <FreeRTOS.h>

#include "address.h"
#include "application_defaults.h"
#include "bluetooth.h"
#include "log.h"
#include "rtc.h"
#include "unified_comms.h"
#include "unified_comms_gatt.h"
#include "random.h"
#include "crypto.h"

#include "bluetooth_gatt_arch.h"
#include "bluetooth_utility.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xGattUnencryptedHeader_t {
	xPayloadType_t xPayloadType;
} ATTR_PACKED xGattUnencryptedHeader_t;

typedef struct xGattEncryptedHeader_t {
	xPayloadType_t xPayloadType;
	uint8_t ucPayloadLength;
	uint8_t pucInitVector[AES128_IV_LENGTH];
} ATTR_PACKED xGattEncryptedHeader_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eGattCommsInit( void );
eModuleError_t eGattCommsEnable( bool bEnable );
eModuleError_t eGattCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage );

void vGattConnected( xBluetoothConnection_t *pxConnection );
void vGattDisconnected( xBluetoothConnection_t *pxConnection );
void vGattLocalCharacterisiticWritten( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic );
void vGattLocalCharacterisiticSubscribed( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic );
void vGattRemoteCharacterisiticChanged( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic );
void vGattRemoteCharacterisiticRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic );

void vGattReceiveHandler( xBluetoothConnection_t *pxConnection, eCommsChannel_t eChannel, const uint8_t *pucData, uint8_t ucDataLen );

/* Private Variables ----------------------------------------*/

xCommsInterface_t xGattComms = {
	.eInterface		  = COMMS_INTERFACE_GATT,
	.fnInit			  = eGattCommsInit,
	.fnEnable		  = eGattCommsEnable,
	.fnSend			  = eGattCommsSend,
	.fnReceiveHandler = NULL
};

static struct xBluetoothConnectionCallbacks_t xGattCallbacks = {
	.fnConnectionOpened				 = vGattConnected,
	.fnConnectionRssi				 = NULL,
	.fnConnectionClosed				 = vGattDisconnected,
	.fnLocalCharacteristicWritten	= vGattLocalCharacterisiticWritten,
	.fnLocalCharacteristicSubscribed = vGattLocalCharacterisiticSubscribed,
	.fnRemoteCharacteristicChanged   = vGattRemoteCharacterisiticChanged,
	.fnRemoteCharacteristicRead		 = vGattRemoteCharacterisiticRead,
};

/* 9ac90002-c517-0d61-0c95-0d5593949597  */
static xBluetoothUUID_t xDataInUUID = {
	.bBluetoothOfficialUUID = false,
	.uUUID					= {
		 .xCustomUUID = {
			 .pucUUID128 = { 0x97, 0x95, 0x94, 0x93, 0x55, 0x0D, 0x95, 0x0C, 0x61, 0x0D, 0x17, 0xC5, 0x02, 0x00, 0xC9, 0x9A } } }
};

/* 9ac90003-c517-0d61-0c95-0d5593949597  */
static xBluetoothUUID_t xAckedOutUUID = {
	.bBluetoothOfficialUUID = false,
	.uUUID					= {
		 .xCustomUUID = {
			 .pucUUID128 = { 0x97, 0x95, 0x94, 0x93, 0x55, 0x0D, 0x95, 0x0C, 0x61, 0x0D, 0x17, 0xC5, 0x03, 0x00, 0xC9, 0x9A } } }
};

/* 9ac90004s-c517-0d61-0c95-0d5593949597  */
static xBluetoothUUID_t xNackedOutUUID = {
	.bBluetoothOfficialUUID = false,
	.uUUID					= {
		 .xCustomUUID = {
			 .pucUUID128 = { 0x97, 0x95, 0x94, 0x93, 0x55, 0x0D, 0x95, 0x0C, 0x61, 0x0D, 0x17, 0xC5, 0x04, 0x00, 0xC9, 0x9A } } }
};

static xBluetoothConnection_t *		pxCurrentConnection  = NULL;
static xGattRemoteCharacteristic_t *pxRemoteDataIn		 = NULL;
static xGattRemoteCharacteristic_t *pxRemoteAckedOutput  = NULL;
static xGattRemoteCharacteristic_t *pxRemoteNackedOutput = NULL;

static SemaphoreHandle_t xGattBuffer;
static uint8_t			 pucCharacteristicBuffer[BLUETOOTH_GATT_MAX_MTU];
static uint8_t			 pucReceiveBuffer[BLUETOOTH_GATT_MAX_MTU];

static bool bAckedSubscribed  = false;
static bool bNackedSubscribed = false;

/*-----------------------------------------------------------*/

/**
 * @brief Initialise GATT Comms
 *
 * We are primarily concerned with the highest possible throughput on our GATT connections
 * As such we set the connection interval to be as short as possible
 *
 * If this is not the desired behaviour for a particular application, eBluetoothConfigureConnections can be called again in application code
 *
 * By default, we expect the initiating device to be the GATT Client. 
 * As a result, the initiating device discovers the remote GATT table, while the GATT server is passive.
 * Again this can be updated in application code, both sides of the connection are capable of discovery on the same connection.
 */
eModuleError_t eGattCommsInit( void )
{
	xGattBuffer = xSemaphoreCreateMutex();
	xSemaphoreGive( xGattBuffer );

	/* By default we want the highest throughput connection possible for RPC's */
	xBluetoothConnectionParameters_t xConnectionParams = {
		.usConnectionInterval  = 8, /* Units of 1.25ms, 8 = 10ms */
		.usSlaveLatency		   = 0,
		.usSupervisorTimeoutMs = 250
	};
	eBluetoothConfigureConnections( &xConnectionParams );

	/* Configure Master Connection */
	xBluetoothConnection_t *pxMaster = pxBluetoothMasterConfiguration();
	pxMaster->eGattDiscovery		 = GATT_DISCOVERY_AUTOMATIC;
	pxMaster->pxCallbacks			 = &xGattCallbacks;

	/* Configure Slave Connection */
	xBluetoothConnection_t *pxSlave = pxBluetoothSlaveConfiguration();
	pxSlave->eGattDiscovery			= GATT_DISCOVERY_NONE;
	pxSlave->pxCallbacks			= &xGattCallbacks;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * @brief GATT State Control
 *
 * eCommsListen has little meaning for GATT due to its directed nature
 *
 * As such, enabling the GATT interface has no meaning.
 * 
 * Disabling triggers a disconnection, but is not expected to be called
 */
eModuleError_t eGattCommsEnable( bool bEnable )
{
	if ( bEnable ) {
		;
	}
	else {
		/* Trigger a disconnection if we are connected */
		if ( pxCurrentConnection != NULL ) {
			eBluetoothDisconnect( pxCurrentConnection );
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

/**
 * @brief Transfering data to a remote device
 *
 * The default behaviour is if the remote device has a data_in characteristic we have found, send it there.
 * This will be the typical mode for bases that have connected to deployed devices 
 *
 * Otherwise we check if the appropriate local channel (ACKED / NACKED) has been subscribed to as dictated by eChannel
 * If yes, we make the new data available on that characteristic, otherwise we drop the packet as there is nothing to send it to.
 *
 * A local buffer is used so we can append the payload to our header rather than some unholy ritual of sending data in two parts.
 */
eModuleError_t eGattCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage )
{
	eModuleError_t			   eError;
	xGattLocalCharacteristic_t xLocalChar;
	bool					   bAcked = ( eChannel == (eCommsChannel_t) COMMS_CHANNEL_GATT_ACKED );
	uint8_t *pucEncryptionKey;
	uint8_t pucInitVector[AES128_IV_LENGTH];

	/* Only a chance to send if the connection is up */
	if ( pxCurrentConnection == NULL ) {
		/* Remote address is not available */
		return ERROR_INVALID_ADDRESS;
	}
	xAddress_t xRemote = xAddressUnpack( pxCurrentConnection->xRemoteAddress.pucAddress );
	if ( pxMessage->xDestination != xRemote ) {
		/* We are not connected to desired destination */
		return ERROR_INVALID_ADDRESS;
	}
	if (pxMessage->usPayloadLen > (BLUETOOTH_GATT_MAX_MTU - sizeof(xGattEncryptedHeader_t))) {
		/* Packet too large for GATT */
		return ERROR_INVALID_DATA;
	}

	xSemaphoreTake( xGattBuffer, portMAX_DELAY );

	if ( pxMessage->xPayloadType & DESCRIPTOR_ENCRYPTED_MASK ) {
		/* Payload is already encrypted, transmit it as is */
		xGattUnencryptedHeader_t *pxHeader = (xGattUnencryptedHeader_t *)pucCharacteristicBuffer;
		uint8_t *pucData = pucCharacteristicBuffer + sizeof(xGattEncryptedHeader_t);

		pxHeader->xPayloadType = pxMessage->xPayloadType;
		pvMemcpy( pucData, pxMessage->pucPayload, pxMessage->usPayloadLen );
	}
	else if ( bUnifiedCommsEncryptionKey( &xGattComms, pxMessage->xPayloadType, pxMessage->xDestination, &pucEncryptionKey ) ) {
		/* Payload is not currently encrypted, but an encryption key was provided, therefore encrypt the data */
		xGattEncryptedHeader_t *pxHeader = (xGattEncryptedHeader_t *)pucCharacteristicBuffer;
		uint8_t *pucData = pucCharacteristicBuffer + sizeof(xGattEncryptedHeader_t);
		/* Populate header and payload */ 
		pxHeader->xPayloadType = pxMessage->xPayloadType;
		pxHeader->ucPayloadLength = pxMessage->usPayloadLen;
		eRandomGenerate(pxHeader->pucInitVector, AES128_IV_LENGTH);
		pvMemcpy( pucData, pxMessage->pucPayload, pxMessage->usPayloadLen );
		/* Pad data with 0's to a multiple of the block size */
		uint8_t ucEncryptLength = ROUND_UP(pxMessage->usPayloadLen, AES128_BLOCK_LENGTH);
		pvMemset(pucData + pxMessage->usPayloadLen, 0x00, ucEncryptLength - pxMessage->usPayloadLen);
		/* Copy IV into scratch space */
		pvMemcpy(pucInitVector, pxHeader->pucInitVector, AES128_IV_LENGTH );
		/* Apply encryption in place */
		vAes128Crypt( ENCRYPT, pucEncryptionKey, pucInitVector, pucData, ucEncryptLength / AES128_IV_LENGTH, pucData );
	} else {
		/* Transmit unencrypted */
		xGattUnencryptedHeader_t *pxHeader = (xGattUnencryptedHeader_t *)pucCharacteristicBuffer;
		uint8_t *pucData = pucCharacteristicBuffer + sizeof(xGattEncryptedHeader_t);

		pxHeader->xPayloadType = pxMessage->xPayloadType;
		pvMemcpy( pucData, pxMessage->pucPayload, pxMessage->usPayloadLen );
	}

	/* If we know the other side has a data_in characteristic */
	if ( pxRemoteDataIn != NULL ) {
		pxRemoteDataIn->pucData   = pucCharacteristicBuffer;
		pxRemoteDataIn->usDataLen = pxMessage->usPayloadLen + 1;
		eError					  = eBluetoothWriteRemoteCharacteristic( pxCurrentConnection, pxRemoteDataIn, bAcked );
	}
	/* Otherwise, make the data available on our own characteristic */
	else {
		xLocalChar.pucData				  = pucCharacteristicBuffer;
		xLocalChar.usDataLen			  = pxMessage->usPayloadLen + 1;
		xLocalChar.usCharacteristicHandle = UINT8_MAX;
		if ( bAcked && bAckedSubscribed ) {
			xLocalChar.usCharacteristicHandle = gattdb_csiro_out_acked;
			xLocalChar.usCCCDValue			  = BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_INDICATION;
		}
		if ( !bAcked && bNackedSubscribed ) {
			xLocalChar.usCharacteristicHandle = gattdb_csiro_out_nacked;
			xLocalChar.usCCCDValue			  = BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_NOTIFICATION;
		}
		if ( xLocalChar.usCharacteristicHandle == UINT8_MAX ) {
			/* Remote device has not subscribed to the characteristic on which the data will be served */
			xSemaphoreGive( xGattBuffer );
			return ERROR_INVALID_STATE;
		}
		eError = eBluetoothDistributeLocalCharacteristic( pxCurrentConnection, &xLocalChar );
	}
	xSemaphoreGive( xGattBuffer );
	return eError;
}

/*-----------------------------------------------------------*/

/**
 * @brief Remote device has connected
 *
 *  We use this function to search through the remote devices GATT table for characteristics we are interested in
 *		xDataInUUID: 	Characteristic that we can send commands to
 *      xAckedOutUUID:  Characteristic for data that needs to be ACKED (reliability important, throughput less important)
 *		xNackedOutUUID: Characteristic for data that doesn't need to be ACKED (throughput important, reliability less important)
 *
 *  The handles to these characteristics (if they exist) are stored for future use in sending and receiving data.
 *  The global storage is due to the fact that we currently only support a single connection.
 *  The Acked/Nacked characteristics are subscribed to so that we are notified about new data that is available
 *
 *  Notification of the connection is also provided to the application
 *
 * @note	Implementation of fnConnectionOpened_t
 */
void vGattConnected( xBluetoothConnection_t *pxConnection )
{
	pxCurrentConnection = pxConnection;

	vBluetoothPrintConnectionGattTable( LOG_APPLICATION, LOG_DEBUG, pxConnection );

	/* Scan through the provided characteristics for our UUID if we performed discovery */
	if ( pxConnection->eGattDiscovery != GATT_DISCOVERY_NONE ) {
		pxRemoteDataIn		 = pxBluetoothSearchCharacteristicUUID( pxConnection, &xDataInUUID );
		pxRemoteAckedOutput  = pxBluetoothSearchCharacteristicUUID( pxConnection, &xAckedOutUUID );
		pxRemoteNackedOutput = pxBluetoothSearchCharacteristicUUID( pxConnection, &xNackedOutUUID );

		/* Subscribe to data changes on the Remote */
		if ( pxRemoteAckedOutput ) {
			/* Indication is the ACKED GATT operation */
			if ( eBluetoothSubscribeRemoteCharacteristic( pxConnection, pxRemoteAckedOutput, BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_INDICATION ) != ERROR_NONE ) {
				pxRemoteAckedOutput = NULL;
			}
		}
		if ( pxRemoteNackedOutput ) {
			/* Notification is the NACKED GATT operation */
			if ( eBluetoothSubscribeRemoteCharacteristic( pxConnection, pxRemoteNackedOutput, BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_NOTIFICATION ) != ERROR_NONE ) {
				pxRemoteNackedOutput = NULL;
			}
		}
	}
	vApplicationGattConnected( pxConnection );
}

/*-----------------------------------------------------------*/

/**
 * @brief Remote device has disconnected 
 * 
 *  All stored handles are cleared so that appropriate error paths are taken if further operations are attempted
 *
 *  Notification of the disconnection is also provided to the application
 *
 * @note	Implementation of fnConnectionClosed_t
 */
void vGattDisconnected( xBluetoothConnection_t *pxConnection )
{
	pxCurrentConnection  = NULL;
	pxRemoteDataIn		 = NULL;
	pxRemoteAckedOutput  = NULL;
	pxRemoteNackedOutput = NULL;
	bAckedSubscribed	 = false;
	bNackedSubscribed	= false;
	vApplicationGattDisconnected( pxConnection );
}

/*-----------------------------------------------------------*/

/**
 * @brief Remote device has written to one of our characteristics
 * 
 * Here we are only checking if the remote device has written to our own local data input characteristic
 * All devices are expected to contain this characteristic as it is part of gatt_base.xml
 * Written data on this characteristic is passed to the appropriate unified_comms handler
 * 
 * Characteristics that are not related to unified_comms are passed to an application handler
 *
 * @note	Implementation of fnLocalCharacteristicWritten_t
 */
void vGattLocalCharacterisiticWritten( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic )
{
	if ( pxUpdatedCharacteristic->usCharacteristicHandle == gattdb_csiro_in ) {
		vGattReceiveHandler(pxConnection, COMMS_CHANNEL_GATT_NACKED, pxUpdatedCharacteristic->pucData, pxUpdatedCharacteristic->usDataLen );
	}
	else {
		/* Not a CSIRO characteristic */
		vApplicationGattLocalCharacterisiticWritten( pxConnection, pxUpdatedCharacteristic );
	}
}

/*-----------------------------------------------------------*/

/**
 * @brief Remote device has subscribed to one of our characteristics, implementation of fnLocalCharacteristicSubscribed_t
 * 
 * Here we check if the remote subscribed to one of our two output characteristics
 * All devices are expected to contain these characteristics as they are part of gatt_base.xml
 * Subscribed characteristics are stored for later notification use
 * 
 * Characteristics that are not related to unified_comms are passed to an application handler
 *
 * @note	Implementation of fnLocalCharacteristicSubscribed_t
 */
void vGattLocalCharacterisiticSubscribed( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic )
{
	UNUSED( pxConnection );
	if ( pxUpdatedCharacteristic->usCharacteristicHandle == gattdb_csiro_out_acked ) {
		bAckedSubscribed = true;
	}
	else if ( pxUpdatedCharacteristic->usCharacteristicHandle == gattdb_csiro_out_nacked ) {
		bNackedSubscribed = true;
	}
	else {
		/* Not a CSIRO characteristic */
		vApplicationGattLocalCharacterisiticSubscribed( pxConnection, pxUpdatedCharacteristic );
	}
}

/*-----------------------------------------------------------*/

/**
 * @brief A previously subscribed characteristic on the remote device has changed value, 
 *
 * Here we check if the updated remote characteristic is a unified_comms characteristic we previously subscribed to
 * If so, it contains normal packet data, and is passed to the appropriate receive handler
 * 
 * Characteristics that are not related to unified_comms are passed to an application handler
 *
 * @note	Implementation of fnRemoteCharacteristicChanged_t
 */
void vGattRemoteCharacterisiticChanged( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic )
{
	eCommsChannel_t eChannel;
	if ( pxUpdatedCharacteristic == pxRemoteAckedOutput ) {
		eChannel = COMMS_CHANNEL_GATT_ACKED;
	}
	else if ( pxUpdatedCharacteristic == pxRemoteNackedOutput ) {
		eChannel = COMMS_CHANNEL_GATT_NACKED;
	}
	else {
		/* Not a CSIRO characteristic */
		vApplicationGattRemoteCharacterisiticChanged( pxConnection, pxUpdatedCharacteristic );
		return;
	}
	vGattReceiveHandler(pxConnection, eChannel, pxUpdatedCharacteristic->pucData, pxUpdatedCharacteristic->usDataLen );
}

/*-----------------------------------------------------------*/

void vGattReceiveHandler( xBluetoothConnection_t *pxConnection, eCommsChannel_t eChannel, const uint8_t *pucData, uint8_t ucDataLen )
{
	uint8_t *pucEncryptionKey;
	uint8_t pucInitVector[AES128_IV_LENGTH];
	/* Pretend the RSSI is 0dBm for now */
	xUnifiedCommsIncomingRoute_t xRoute = {
		.xRoute = {
			.ucInterfaceAndChannel = MASK_WRITE( COMMS_INTERFACE_GATT, COMMS_INTERFACE_MASK ) | MASK_WRITE( eChannel, COMMS_CHANNEL_MASK ) },
		.xMetadata = { .usPacketAge = 0, .ucSequenceNumber = 0, .ucRssi = 30 }
	};
	pvMemcpy( xRoute.xRoute.pucHopAddress, pxConnection->xRemoteAddress.pucAddress, MAC_ADDRESS_LENGTH );

	xPayloadType_t xPayloadType = (xPayloadType_t)pucData[0];
	xPayloadType_t xBaseType		= MASK_READ( xPayloadType, DESCRIPTOR_PACKET_TYPE_MASK );

	xUnifiedCommsMessage_t xMessage = {
		.xSource	  = xAddressUnpack( pxConnection->xRemoteAddress.pucAddress ),
		.xDestination = LOCAL_ADDRESS,
		.xPayloadType = xPayloadType
	};

	if ( xPayloadType & DESCRIPTOR_ENCRYPTED_MASK ) {
		/* Payload is encrypted */
		xGattEncryptedHeader_t *pxHeader = (xGattEncryptedHeader_t *)pucData;
		/* Validate length */
		if ((ucDataLen - sizeof(xGattEncryptedHeader_t)) % AES128_BLOCK_LENGTH != 0) {
			/* Data payload is not a multiple of the block length */
			return;
		}
		uint8_t ucEncryptedBlocks = (ucDataLen - sizeof(xGattEncryptedHeader_t)) / AES128_BLOCK_LENGTH;

		if ( bUnifiedCommsDecryptionKey( &xGattComms, xBaseType, xMessage.xSource, &pucEncryptionKey ) ) {
			/* Decryption key was provided */

			/* Decrypt data, after locading initialisation vector into scratch memory */
			pvMemcpy(pucInitVector, pxHeader->pucInitVector, AES128_IV_LENGTH);
			vAes128Crypt( DECRYPT, pucEncryptionKey, pucInitVector, pucData + sizeof(xGattEncryptedHeader_t), ucEncryptedBlocks, pucReceiveBuffer );

			xMessage.pucPayload = pucReceiveBuffer;
			xMessage.usPayloadLen = pxHeader->ucPayloadLength;
		} else {
			/* No chance of decrypting data, pass it up to the handler as is, minus the payload type */
			xMessage.pucPayload = pucData + 1;
			xMessage.usPayloadLen = ucDataLen - 1;
		}
	}
	else {
		xMessage.pucPayload = pucData + sizeof(xGattUnencryptedHeader_t);
		xMessage.usPayloadLen = ucDataLen - 1;
	}
	xGattComms.fnReceiveHandler( &xGattComms, &xRoute, &xMessage );
}

/*-----------------------------------------------------------*/

void vGattRemoteCharacterisiticRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic )
{
	vApplicationGattRemoteCharacterisiticRead( pxConnection, pxUpdatedCharacteristic );
}

/*-----------------------------------------------------------*/
