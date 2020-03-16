/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * Adapted from serial byte handling in unifiedbase.c in Contiki3.x
 */
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "unified_comms.h"
#include "unified_comms_serial.h"

#include "address.h"
#include "board.h"
#include "compiler_intrinsics.h"
#include "crc.h"
#include "log.h"
#include "memory_operations.h"
#include "rtc.h"
#include "uart.h"
#include "unified_comms_bluetooth.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define MAX_PACKET_BUFFER       256

#define SERIAL_SYNC_A   		0xAA
#define SERIAL_SYNC_B   		0x55

// clang-format on

/* Type Definitions -----------------------------------------*/

/* see Wiki: https://confluence.csiro.au/display/pacp/On-Air+Packet+Format */
typedef struct xSerialInterfaceHeader_t
{
	uint8_t  ucSyncA;
	uint8_t  ucSyncB;
	uint16_t usPayloadLen;
	uint8_t  pucAddress[MAC_ADDRESS_LENGTH];
	uint8_t  ucSequence;
	uint8_t  ucPacketType;
} ATTR_PACKED xSerialInterfaceHeader_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eSerialCommsInit( void );
eModuleError_t eSerialCommsEnable( bool bEnable );
eModuleError_t eSerialCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage );

/* Private Variables ----------------------------------------*/

static const uint8_t pucSyncBytes[2] = { SERIAL_SYNC_A, SERIAL_SYNC_B };

static uint8_t pucRxBuffer[MAX_PACKET_BUFFER];

xCommsInterface_t xSerialComms = {
	.eInterface		  = COMMS_INTERFACE_SERIAL,
	.fnInit			  = eSerialCommsInit,
	.fnEnable		  = eSerialCommsEnable,
	.fnSend			  = eSerialCommsSend,
	.fnReceiveHandler = NULL
};

/*-----------------------------------------------------------*/

eModuleError_t eSerialCommsInit( void )
{
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSerialCommsEnable( bool bEnable )
{
	if ( bEnable ) {
		pxSerialOutput->pxImplementation->fnEnable( pxSerialOutput->pvContext );
	}
	else {
		pxSerialOutput->pxImplementation->fnDisable( pxSerialOutput->pvContext );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eSerialCommsSend( eCommsChannel_t eChannel, xUnifiedCommsMessage_t *pxMessage )
{
	static xSerialInterfaceHeader_t xInterfaceHeader = {
		.ucSyncA	  = SERIAL_SYNC_A,
		.ucSyncB	  = SERIAL_SYNC_B,
		.usPayloadLen = 0,
		.pucAddress   = { 0 },
		.ucSequence   = 0,
		.ucPacketType = 0
	};
	char *   pcBuffer;
	uint32_t ulBufferLength, ulTotalLen;
	UNUSED( eChannel );

	/* Desination address is ignored as serial is point-to-point */
	vAddressPack( xInterfaceHeader.pucAddress, pxMessage->xSource );
	xInterfaceHeader.ucPacketType = pxMessage->xPayloadType;
	xInterfaceHeader.usPayloadLen = pxMessage->usPayloadLen;
	ulTotalLen					  = sizeof( xSerialInterfaceHeader_t ) + xInterfaceHeader.usPayloadLen;

	/* Get the buffer to put formatted data into */
	pcBuffer = pxSerialOutput->pxImplementation->fnClaimBuffer( pxSerialOutput->pvContext, &ulBufferLength );
	/* Check the data we want to send fits in our UART buffer */
	if ( ulBufferLength < ulTotalLen ) {
		pvMemcpy( pcBuffer, (char *) "Serial Buffers not large enough to hold Serial Packet!!\r\n", 60 );
		pxSerialOutput->pxImplementation->fnSendBuffer( pxSerialOutput->pvContext, pcBuffer, 60 );
		return ERROR_GENERIC;
	}
	xBufferBuilder_t xPacketBuilder;
	vBufferBuilderStart( &xPacketBuilder, (uint8_t *) pcBuffer, ulBufferLength );
	/* Pack the data */
	vBufferBuilderPushData( &xPacketBuilder, &xInterfaceHeader, sizeof( xSerialInterfaceHeader_t ) );
	vBufferBuilderPushData( &xPacketBuilder, pxMessage->pucPayload, pxMessage->usPayloadLen );
	/* Push the message at the UART driver */
	pxSerialOutput->pxImplementation->fnSendBuffer( pxSerialOutput->pvContext, pcBuffer, xPacketBuilder.ulIndex );
	/* Increment the sequnce number for next message */
	xInterfaceHeader.ucSequence++;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vSerialPacketBuilder( char cByte )
{
	static xSerialInterfaceHeader_t xSerialHeader		 = { 0x00 };
	static uint16_t					usRxByteCount		 = 0;
	static uint16_t					usLastSequenceNumber = UINT16_MAX;

	//eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Serial Recv: %X\r\n", (uint8_t) cByte );

	configASSERT( usRxByteCount < MAX_PACKET_BUFFER );
	uint16_t usCurrentByte		 = usRxByteCount;
	pucRxBuffer[usRxByteCount++] = (uint8_t) cByte;

	if ( xSerialComms.fnReceiveHandler == NULL ) {
		return;
	}

	/* If we're looking for SYNC bytes */
	if ( usRxByteCount <= 2 ) {
		/* Check that sync bytes match */
		if ( pucRxBuffer[usCurrentByte] != pucSyncBytes[usCurrentByte] ) {
			usRxByteCount = 0;
		}
	}
	/* If the header is complete */
	else if ( usRxByteCount == sizeof( xSerialInterfaceHeader_t ) ) {
		pvMemcpy( &xSerialHeader, pucRxBuffer, sizeof( xSerialInterfaceHeader_t ) );
	}
	/* If the complete packet has arrived */
	else if ( usRxByteCount == ( sizeof( xSerialInterfaceHeader_t ) + xSerialHeader.usPayloadLen ) ) {
		/* Check if this is a duplicate packet */
		if ( xSerialHeader.ucSequence == usLastSequenceNumber ) {
			eLog( LOG_APPLICATION, LOG_ERROR, "Duplicate packet received, Sequence %d\r\n", xSerialHeader.ucSequence );
		}
		else {
			xUnifiedCommsIncomingRoute_t xRoute = {
				.xRoute = {
					.ucInterfaceAndChannel = MASK_WRITE( COMMS_INTERFACE_SERIAL, COMMS_INTERFACE_MASK ) | MASK_WRITE( COMMS_CHANNEL_DEFAULT, COMMS_CHANNEL_MASK ) },
				.xMetadata = { .usPacketAge = 0, .ucSequenceNumber = xSerialHeader.ucSequence, .ucRssi = 0 }
			};
			vAddressPack( xRoute.xRoute.pucHopAddress, BROADCAST_ADDRESS );

			xUnifiedCommsMessage_t xMessage = {
				.xSource	  = BROADCAST_ADDRESS,
				.xDestination = xAddressUnpack( xSerialHeader.pucAddress ),
				.xPayloadType = xSerialHeader.ucPacketType,
				.pucPayload   = pucRxBuffer + sizeof( xSerialInterfaceHeader_t ),
				.usPayloadLen = (uint16_t) xSerialHeader.usPayloadLen
			};
			xSerialComms.fnReceiveHandler( &xSerialComms, &xRoute, &xMessage );
		}
		usLastSequenceNumber = (uint16_t) xSerialHeader.ucSequence;
		usRxByteCount		 = 0;
	}
}

/*-----------------------------------------------------------*/
