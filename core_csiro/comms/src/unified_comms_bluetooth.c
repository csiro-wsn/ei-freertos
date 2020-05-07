/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "timers.h"

#include "unified_comms.h"
#include "unified_comms_bluetooth.h"

#include "bluetooth.h"

#include "board.h"
#include "crypto.h"
#include "csiro85_encode.h"
#include "log.h"
#include "memory_operations.h"
#include "rtc.h"
#include "unified_comms_serial.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define MULTI_PACKET_TIMEOUT		pdMS_TO_TICKS( 300 )

#define HEADER_ASCII_OFFSET			0x21

#define SEQUENCE_GENERAL_MASK			0b11110000
#define SEQUENCE_NUM_PACKETS_MASK		0b00001100
#define SEQUENCE_PACKET_INDEX_MASK		0b00000011

// clang-format on
/* Type Definitions -----------------------------------------*/

/* see Wiki: https://confluence.csiro.au/display/pacp/Packet+Framing
 * 

 * To mitigate the limitations of the small payload size (13 bytes), a multi-packet system is implemented.
 * Payloads up to 4x larger (52 bytes) can be handled via splitting data across multiple advertising packets.
 * The multi-packet functionality is encoded into the sequence number byte.
 * 
 * | MSB           ucSequenceNumber                	 LSB |
 * | ----------------------------------------------------|
 * | 4 bits          | 2 bits    	   | 2 bits          |
 * | Sequence Number | Num Packets - 1 | Subpacket Index |
 * 
 * The 4 bit sequence number is a normal incrementing counter for each unique payload that is sent.
 * All packets within a multi-packet have the same sequence number.
 * 
 * The second field encodes the number of packets, in a multi-packet, indexed at 0.
 * The last field encodes the index within the multi-packet that this packet is.
 * 
 */
typedef struct xBluetoothInterfaceHeader_t
{
	xPayloadType_t xPacketType;
	uint8_t		   ucSequence;
	uint8_t		   pucAddress[BLUETOOTH_MAC_ADDRESS_LENGTH];
} ATTR_PACKED xBluetoothInterfaceHeader_t;

CASSERT( sizeof( xBluetoothInterfaceHeader_t ) == 8, GapHeaderSize )

struct xBluetoothRxModule_t
{
	xBufferBuilder_t xBufferBuilder;
	TickType_t		 xPrevTime;
	xAddress_t		 xPrevSource;
	xAddress_t		 xPrevDestination;
	uint8_t			 ucPrevSequence;
	uint8_t			 ucPrevNumPackets;
	uint8_t			 ucNextIndex;
	uint8_t			 pucRxBuffer[CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH];
	bool			 bMsgInProgress;
} xBluetoothRxModule;

/* Function Declarations ------------------------------------*/

eModuleError_t eBluetoothCommsInit( void );
eModuleError_t eBluetoothCommsEnable( bool bEnable );
eModuleError_t eBluetoothCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage );

void vBluetoothReceived( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, int8_t cRssi, bool bConnectable, uint8_t *pucData, uint8_t ucDataLen );

/* Private Variables ----------------------------------------*/

xBluetoothScanParameters_t xBluetoothScan = {
	.ePhy			  = BLUETOOTH_PHY_1M,
	.usScanIntervalMs = 2000,
	.usScanWindowMs   = 2000,
	.fnCallback		  = vBluetoothReceived
};

static int8_t cLastBluetoothRssi;
static bool   bAdvertiseAsConnectable   = true;
static bool   bLastBluetoothConnectable = false;
static uint8_t pucInitialisationVectorTail[13] = {0};

static bool (*fnDecryptionChecker)(uint8_t *pucData, uint8_t ucDataLen) = NULL;
static vCustomPacketHandler_t fnCustomHandler = NULL;

xCommsInterface_t xBluetoothComms = {
	.eInterface		  = COMMS_INTERFACE_BLUETOOTH,
	.fnInit			  = eBluetoothCommsInit,
	.fnEnable		  = eBluetoothCommsEnable,
	.fnSend			  = eBluetoothCommsSend,
	.fnReceiveHandler = NULL
};

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothCommsInit( void )
{
	eBluetoothConfigureScanning( &xBluetoothScan );
	cLastBluetoothRssi = 0;
	pvMemset( &xBluetoothRxModule, 0x00, sizeof( struct xBluetoothRxModule_t ) );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothCommsEnable( bool bEnable )
{
	if ( bEnable ) {
		eBluetoothScanStart( BLUETOOTH_PHY_1M );
	}
	else {
		eBluetoothScanStop( NULL );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eBluetoothCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage )
{
	xBluetoothInterfaceHeader_t		xBluetoothHeader;
	xBluetoothAdvertiseParameters_t xBluetoothTransmit				= { 0 };
	xPayloadType_t					xPayloadType					= pxMessage->xPayloadType;
	uint16_t						usPayloadLen					= pxMessage->usPayloadLen;
	const uint8_t *					pucPayload						= pxMessage->pucPayload;
	static uint8_t					ucSequenceNumber				= 0;
	const uint8_t					ucPayloadOffset					= sizeof( xADFlagsStructure_t ) + sizeof( xADHeader_t );
	uint8_t							pucInitVector[AES128_IV_LENGTH] = { 0 };
	uint8_t *						pucEncryptionKey;

	/* Payload must fit in our max message length */
	configASSERT( pxMessage->usPayloadLen <= CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH );
	/* Bluetooth advertising doesn't support top two bits of payload type */
	configASSERT( ( pxMessage->xPayloadType & 0b11000000 ) == 0 );

	/* Basic Advertising Parameters */
	xBluetoothTransmit.ePhy		 = BLUETOOTH_PHY_1M;
	xBluetoothTransmit.ucDataLen = BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH;
	/* Based on eChannel, ucAdvertiseCount can be chosen */
	/* Bluetooth phone channel most likely indicates this payload is responding to a payload that originated from a phone */
	xBluetoothTransmit.ucAdvertiseCount		 = ( eChannel == (eCommsChannel_t) COMMS_CHANNEL_BLUETOOTH_PHONE ) ? 5 : 1;
	xBluetoothTransmit.bAdvertiseConnectable = bAdvertiseAsConnectable;

	/* Bluetooth header is a 2 byte long FLAGS AD Type */
	xADFlagsStructure_t xFlags = {
		.xHeader = {
			.ucLength = 0x02,
			.ucType   = BLE_AD_TYPE_FLAGS },
		.ucFlags = BLE_ADV_FLAGS_LE_GENERAL_DISC_MODE | BLE_ADV_FLAGS_BR_EDR_NOT_SUPPORTED
	};

	/* Bluetooth data is contained in a "Complete Local Name" AD type */
	xADHeader_t xNameHeader = {
		.ucLength = BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH + 1,
		.ucType   = BLE_AD_TYPE_COMPLETE_LOCAL_NAME
	};

	/* Number of sub-packets required */
	uint8_t ucPacketCapacity = CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH + ( ( xPayloadType & DESCRIPTOR_ENCRYPTED_MASK ) ? 3 : 0 ); /* Pre-encrypted packets have 3 bytes of address for each packet sitting in the payload */
	uint8_t ucNumPackets	 = ROUND_UP( usPayloadLen, ucPacketCapacity ) / ucPacketCapacity;

	/* Fill Header */
	uint8_t ucSequenceNumberBase = MASK_WRITE( ucSequenceNumber, SEQUENCE_GENERAL_MASK ) | MASK_WRITE( ucNumPackets - 1, SEQUENCE_NUM_PACKETS_MASK );
	xBluetoothHeader.xPacketType = xPayloadType;
	if ( bAddressesMatch( pxMessage->xDestination, BROADCAST_ADDRESS ) ) {
		/* If we are broadcasting data, instead use our own address with the broadcast bit  */
		vAddressPack( xBluetoothHeader.pucAddress, LOCAL_ADDRESS );
		xBluetoothHeader.xPacketType |= MASK_WRITE( 0b1, DESCRIPTOR_BROADCAST_MASK );
	}
	else {
		vAddressPack( xBluetoothHeader.pucAddress, pxMessage->xDestination );
	}

	uint8_t			 ucDataRemaining = usPayloadLen;
	xBufferBuilder_t xPacketBuilder;

	/* Build our packets */
	for ( uint8_t ucPacketNum = 0; ucPacketNum < ucNumPackets; ucPacketNum++ ) {
		bool bFinalPacket = ucPacketNum == ( ucNumPackets - 1 );
		/* Update sequence numbers */
		xBluetoothHeader.ucSequence = ucSequenceNumberBase | MASK_WRITE( ucPacketNum, SEQUENCE_PACKET_INDEX_MASK );
		/* Reset buffers */
		pvMemset( xBluetoothTransmit.pucData, 0x00, BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH );
		vBufferBuilderStart( &xPacketBuilder, xBluetoothTransmit.pucData, BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH );
		/* Required Bluetooth Headers */
		vBufferBuilderPushData( &xPacketBuilder, &xFlags, sizeof( xADFlagsStructure_t ) );
		vBufferBuilderPushData( &xPacketBuilder, &xNameHeader, sizeof( xADHeader_t ) );

		/* Handle different transmission modes */
		if ( xPayloadType & DESCRIPTOR_ENCRYPTED_MASK ) {
			/* Payload is already encrypted, transmit it as is */
			if (ucDataRemaining <= 3) {
				/* There must be at more than 3 bytes, as the first 3 bytes are just the encrypted address */
				return ERROR_INVALID_DATA;
			}
			/* Encryption on Advertising packets starts on the last 3 bytes of the destination address, overwrite existing address */
			pvMemcpy( xBluetoothHeader.pucAddress + 3, pucPayload, 3 );
			/* Push Header and Prencrypted data */
			uint8_t ucEncryptedPayloadLen = ucDataRemaining - 3;
			vBufferBuilderPushData( &xPacketBuilder, &xBluetoothHeader, sizeof( xBluetoothInterfaceHeader_t ) );
			vBufferBuilderPushData( &xPacketBuilder, pucPayload + 3, bFinalPacket ? ucEncryptedPayloadLen : CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH );
			/* Update remaining data */
			pucPayload += CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH + 3;
			ucDataRemaining -= CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH - 3;
		}
		else if ( bUnifiedCommsEncryptionKey( &xBluetoothComms, pxMessage->xPayloadType, pxMessage->xDestination, &pucEncryptionKey ) ) {
			/* Payload is not currently encrypted, but an encryption key was provided, therefore encrypt the data */

			/* Push the payload into the output buffer */
			vBufferBuilderPushData( &xPacketBuilder, &xBluetoothHeader, sizeof( xBluetoothInterfaceHeader_t ) );
			vBufferBuilderPushData( &xPacketBuilder, pucPayload, bFinalPacket ? ucDataRemaining : CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH );
			/* Setup initialisation vector */
			pvMemcpy( pucInitVector, xBluetoothHeader.pucAddress, 3 ); /* Set first 3 bytes of the MAC as the initialisation vector */
			pvMemcpy( pucInitVector + 3, pucInitialisationVectorTail, 13); /* Remaining 13 bytes are as provided by vUnifiedCommsBluetoothSetInitialisationVector */
			/* Apply the AES128 Encryption in-place, begins on last 3 bytes of the interface header */
			uint8_t *pucEncryptionStart = xBluetoothTransmit.pucData + ucPayloadOffset + sizeof( xBluetoothInterfaceHeader_t ) - 3;
			vAes128Crypt( ENCRYPT, pucEncryptionKey, pucInitVector, pucEncryptionStart, 1, pucEncryptionStart );
			/* Update remaining data */
			pucPayload += CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH;
			ucDataRemaining -= CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH;
		}
		else {
			/* Not encrypted and no encryption key, transmit data with no encryption */
			vBufferBuilderPushData( &xPacketBuilder, &xBluetoothHeader, sizeof( xBluetoothInterfaceHeader_t ) );
			vBufferBuilderPushData( &xPacketBuilder, pucPayload, bFinalPacket ? ucDataRemaining : CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH );
			/* Update remaining data */
			pucPayload += CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH;
			ucDataRemaining -= CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH;
		}
		configASSERT( bBufferBuilderValid( &xPacketBuilder ) );

		eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "UNIFIED BT TX RAW: %02X: % 31A\r\n", xBluetoothHeader.xPacketType, xBluetoothTransmit.pucData );
		/* First Payload byte is offset by ASCII_HEADER_OFFSET */
		*( xBluetoothTransmit.pucData + ucPayloadOffset ) += HEADER_ASCII_OFFSET;
		/* Remainder of the packet is BASE-85 encoded */
		ulCsiro85Encode( xBluetoothTransmit.pucData + ucPayloadOffset + 1, BLE_UNIFIED_COMMS_LOCAL_NAME_BINARY_MAX_LENGTH, xBluetoothTransmit.pucData + ucPayloadOffset + 1, BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH );
		/* We want to delay starting the advertising until the final packet */
		xBluetoothTransmit.bStartSequence = bFinalPacket;
		eBluetoothAdvertise( &xBluetoothTransmit );
	}
	ucSequenceNumber++;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vBluetoothReceived( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, int8_t cRssi, bool bConnectable, uint8_t *pucData, uint8_t ucDataLen )
{
	/* Data starts after the headers */
	uint8_t *  pucCsiroPayload							= pucData + sizeof( xADFlagsStructure_t ) + sizeof( xADHeader_t );
	uint8_t	pucInitVector[AES128_IV_LENGTH]			= { 0 };
	uint8_t	pucDecryptionBuffer[AES128_BLOCK_LENGTH] = { 0 };
	xAddress_t xSource									= xAddressUnpack( pucAddress );
	xAddress_t xDestination;
	UNUSED( eAddressType );

	/* Output all scan info on verbose */
	eLog( LOG_BLUETOOTH_GAP, LOG_VERBOSE, "BT: RSSI:%d ADDR: %:6R DATA: %*A\r\n", cRssi, pucAddress, ucDataLen, pucData );

	if (fnCustomHandler != NULL) {
		fnCustomHandler(pucAddress, eAddressType, cRssi, bConnectable, pucData, ucDataLen);
	}

	/* If there is no registered packet handler, there is nothing to do */
	if ( xBluetoothComms.fnReceiveHandler == NULL ) {
		return;
	}

	/* Our packets use the entire Bluetooth advertising size */
	if ( ucDataLen != BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH ) {
		return;
	}

	/* Our packets start with an AD FLAGS, data in COMPLETE_LOCAL_NAME */
	xADFlagsStructure_t *pxFlags	  = (xADFlagsStructure_t *) ( pucData + 0 );
	xADHeader_t *		 pxNameHeader = (xADHeader_t *) ( pucData + sizeof( xADFlagsStructure_t ) );
	if ( ( pxFlags->xHeader.ucType != 0x01 ) || ( pxNameHeader->ucType != 0x09 ) ) {
		return;
	}
	bLastBluetoothConnectable = bConnectable;

	/* Our Packets are encoded with a first byte offset and Base85  */
	if ( pucCsiroPayload[0] < HEADER_ASCII_OFFSET || !bCsiro85Valid( pucCsiroPayload + 1, BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH - 1 ) ) {
		return;
	}

	/* Revert UTF-8 Encoding ( HEADER_ASCII_OFFSET on byte 0, Base85 on remainder of packet ) */
	pucCsiroPayload[0] -= HEADER_ASCII_OFFSET;
	ulCsiro85Decode( pucCsiroPayload + 1, BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH - 1, pucCsiroPayload + 1, BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH - 1 );
	uint8_t ucPayloadLen = CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH;

	/* Cast back to useful structs */
	xBluetoothInterfaceHeader_t *pxHeader   = (xBluetoothInterfaceHeader_t *) pucCsiroPayload;
	uint8_t *					 pucPayload = pucCsiroPayload + sizeof( xBluetoothInterfaceHeader_t );

	/* Extract packet type flags and expected encryption keys */
	bool		   bIsEncrypted		= MASK_READ( pxHeader->xPacketType, DESCRIPTOR_ENCRYPTED_MASK );
	bool		   bIsBroadcast		= MASK_READ( pxHeader->xPacketType, DESCRIPTOR_BROADCAST_MASK );
	xPayloadType_t xPayloadType		= MASK_READ( pxHeader->xPacketType, DESCRIPTOR_PACKET_TYPE_MASK );
	uint8_t *	  pucEncryptionKey;

	/* Perform in-place decryption if required */
	if ( bIsEncrypted ) {

		if ( bUnifiedCommsDecryptionKey( &xBluetoothComms, xPayloadType, xAddressUnpack(pxHeader->pucAddress), &pucEncryptionKey ) ) {
			/* Setup initialisation vector */
			pvMemcpy( pucInitVector, pxHeader->pucAddress, 3 ); /* Set first 3 bytes of the MAC as the initialisation vector */
			pvMemcpy( pucInitVector + 3, pucInitialisationVectorTail, 13); /* Remaining 13 bytes are as provided by vUnifiedCommsBluetoothSetInitialisationVector */

			/* Apply decryption into scratch space, starting from 3 bytes before the payload ( Destination Address ) */
			vAes128Crypt( DECRYPT, pucEncryptionKey, pucInitVector, pucPayload - 3, 1, pucDecryptionBuffer );

			if ((fnDecryptionChecker == NULL) || fnDecryptionChecker(pucDecryptionBuffer, AES128_BLOCK_LENGTH)) {
				/* No checker was provided, or the check succeeded */
				pxHeader->xPacketType = MASK_CLEAR( pxHeader->xPacketType, DESCRIPTOR_ENCRYPTED_MASK );
				/* Decryption succeeded, copy back over to existing buffer */
				pvMemcpy( pucPayload - 3, pucDecryptionBuffer, AES128_BLOCK_LENGTH );
			}
			else {
				/* Decryption failed, leave original buffer encrypted, add encryption flag back to payload type */
				xPayloadType |= DESCRIPTOR_ENCRYPTED_MASK;
				/* Failed decryption needs to output the last three address bytes */
				pucPayload -= 3;
				ucPayloadLen += 3;
			}
		}
		else {
			/* Failed decryption needs to output the last three address bytes */
			pucPayload -= 3;
			ucPayloadLen += 3;
		}
	}

	/* Extract Addresses */
	xDestination = bIsBroadcast ? BROADCAST_ADDRESS : xAddressUnpack( pxHeader->pucAddress );
	xSource		 = bIsBroadcast ? xAddressUnpack( pxHeader->pucAddress ) : xSource;

	/* Extract sequence information */
	cLastBluetoothRssi	= cRssi;
	uint8_t ucShiftedRssi = 30 - (int16_t) cRssi;
	uint8_t ucSequence	= MASK_READ( pxHeader->ucSequence, SEQUENCE_GENERAL_MASK );
	uint8_t ucNumPackets  = MASK_READ( pxHeader->ucSequence, SEQUENCE_NUM_PACKETS_MASK ) + 1;
	uint8_t ucPacketNum   = MASK_READ( pxHeader->ucSequence, SEQUENCE_PACKET_INDEX_MASK );

	xUnifiedCommsIncomingRoute_t xRoute = {
		.xRoute = {
			.ucInterfaceAndChannel = MASK_WRITE( COMMS_INTERFACE_BLUETOOTH, COMMS_INTERFACE_MASK ) | MASK_WRITE( COMMS_CHANNEL_DEFAULT, COMMS_CHANNEL_MASK ) },
		.xMetadata = { .usPacketAge = 0, .ucSequenceNumber = ucSequence, .ucRssi = ucShiftedRssi }
	};
	vAddressPack( xRoute.xRoute.pucHopAddress, xSource );

	xDateTime_t xDatetime;
	bRtcGetDatetime( &xDatetime );
	eLog( LOG_BLUETOOTH_GAP, LOG_INFO, "RX %2d.%05d- SRC: " ADDRESS_FMT " DST: " ADDRESS_FMT " SEQ: 0x%02X TYPE: 0x%02X\r\n",
		  xDatetime.xTime.ucSecond, xDatetime.xTime.usSecondFraction, xSource, xDestination, pxHeader->ucSequence, pxHeader->xPacketType );

	/* If the message is not a multipacket we have everything we need to processes it */
	if ( ucNumPackets == 1 ) {
		xUnifiedCommsMessage_t xMessage = {
			.xSource	  = xSource,
			.xDestination = xDestination,
			.xPayloadType = xPayloadType,
			.pucPayload   = pucPayload,
			.usPayloadLen = (uint16_t) ucPayloadLen
		};
		xBluetoothComms.fnReceiveHandler( &xBluetoothComms, &xRoute, &xMessage );
		return;
	}

	TickType_t xPacketTime = xTaskGetTickCount();
	/* Check for valid continuation */
	if ( xBluetoothRxModule.bMsgInProgress ) {
		TickType_t xElapsedTime = xPacketTime - xBluetoothRxModule.xPrevTime;
		/* If the message has timed out, the addresses don't match or the sequence numbers aren't correct */
		if ( ( xElapsedTime > MULTI_PACKET_TIMEOUT ) ||
			 ( !bAddressesMatch( xSource, xBluetoothRxModule.xPrevSource ) ) ||
			 ( ucSequence != xBluetoothRxModule.ucPrevSequence ) ||
			 ( ucNumPackets != xBluetoothRxModule.ucPrevNumPackets ) ||
			 ( ucPacketNum != xBluetoothRxModule.ucNextIndex ) ||
			 ( !bAddressesU24Match( xDestination, xBluetoothRxModule.xPrevDestination ) ) ||
			 ( !bAddressesMatch( xDestination, xBluetoothRxModule.xPrevDestination ) && !bIsEncrypted ) ) {
			pvMemset( &xBluetoothRxModule, 0x00, sizeof( struct xBluetoothRxModule_t ) );
			xBluetoothRxModule.bMsgInProgress = false;
		}
	}

	/* Ignore messages if we don't have the first packet */
	if ( !xBluetoothRxModule.bMsgInProgress && ( ucPacketNum != 0 ) ) {
		return;
	}

	xBluetoothRxModule.xPrevTime		= xPacketTime;
	xBluetoothRxModule.xPrevSource		= xSource;
	xBluetoothRxModule.xPrevDestination = xDestination;
	xBluetoothRxModule.ucPrevSequence   = ucSequence;
	xBluetoothRxModule.ucPrevNumPackets = ucNumPackets;
	xBluetoothRxModule.ucNextIndex		= ucPacketNum + 1;

	/* Start of a multi-packet */
	if ( ucPacketNum == 0 ) {
		/* Reset the buffer builder for a new message */
		pvMemset( xBluetoothRxModule.pucRxBuffer, 0x00, CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH );
		vBufferBuilderStart( &xBluetoothRxModule.xBufferBuilder, xBluetoothRxModule.pucRxBuffer, CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH );
		xBluetoothRxModule.bMsgInProgress = true;
	}

	vBufferBuilderPushData( &xBluetoothRxModule.xBufferBuilder, pucPayload, ucPayloadLen );
	configASSERT( bBufferBuilderValid( &xBluetoothRxModule.xBufferBuilder ) );

	/* If this was the last packet, pass it to the receive handler*/
	if ( ucPacketNum == ( ucNumPackets - 1 ) ) {
		xUnifiedCommsMessage_t xMessage = {
			.xSource	  = xSource,
			.xDestination = xDestination,
			.xPayloadType = xPayloadType,
			.pucPayload   = xBluetoothRxModule.pucRxBuffer,
			.usPayloadLen = (uint16_t) xBluetoothRxModule.xBufferBuilder.ulIndex
		};
		xBluetoothComms.fnReceiveHandler( &xBluetoothComms, &xRoute, &xMessage );
	}
}

/*-----------------------------------------------------------*/

int16_t sBluetoothCommsRssi( void )
{
	return (int16_t) cLastBluetoothRssi;
}

/*-----------------------------------------------------------*/

void vUnifiedCommsBluetoothAdvertisingIsConnectable( bool bConnectable )
{
	bAdvertiseAsConnectable = bConnectable;
}

/*-----------------------------------------------------------*/

bool bUnifiedCommsBluetoothWasConnectable( void )
{
	return bLastBluetoothConnectable;
}

/*-----------------------------------------------------------*/

void vUnifiedCommsBluetoothSetInitialisationVector( uint8_t pucInitialisationTail[13])
{
	pvMemcpy(pucInitialisationVectorTail, pucInitialisationTail, 13);
}

/*-----------------------------------------------------------*/

void vUnifiedCommsBluetoothDecryptionChecker( bool (*fnChecker)(uint8_t *pucData, uint8_t ucDataLen))
{
	fnDecryptionChecker = fnChecker;
}

/*-----------------------------------------------------------*/

void vUnifiedCommsBluetoothCustomHandler( vCustomPacketHandler_t fnPacketHandler )
{
	fnCustomHandler = fnPacketHandler;
}

/*-----------------------------------------------------------*/
