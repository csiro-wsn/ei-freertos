/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "unified_comms.h"

#include "unified_comms_bluetooth.h"
#include "unified_comms_gatt.h"
#include "unified_comms_serial.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define MAX_ROUTING_PACKET_SIZE 256

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vCommsListenCallback( TimerHandle_t xTimer );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vUnifiedCommsInit( xCommsInterface_t *pxComms )
{
	pxComms->xListenTimer = xTimerCreate( NULL, portMAX_DELAY, false, pxComms, vCommsListenCallback );
	pxComms->fnInit();
}

/*-----------------------------------------------------------*/

void vUnifiedCommsListen( xCommsInterface_t *pxComms, eCommsListen_t eListenDurationMs )
{
	TimerHandle_t xTimer = pxComms->xListenTimer;
	TickType_t	xRemainingTime;
	/* Update our timeout timer */
	switch ( eListenDurationMs ) {
		case COMMS_LISTEN_OFF_IMMEDIATELY:
		case COMMS_LISTEN_ON_FOREVER:
			/* Stop currently running radio request timer */
			xTimerStop( xTimer, portMAX_DELAY );
			pxComms->eListenTime = eListenDurationMs;
			break;
		default:
			/* If the current request is ON_FOREVER, return immedietely */
			if ( pxComms->eListenTime == COMMS_LISTEN_ON_FOREVER ) {
				return;
			}
			/* If there is already a timed request executing */
			if ( xTimerIsTimerActive( xTimer ) ) {
				xRemainingTime = xTimerGetExpiryTime( xTimer ) - xTaskGetTickCount();
				/*  Only update the timeout if this request is longer than the time remaining */
				if ( eListenDurationMs < xRemainingTime ) {
					break;
				}
			}
			xTimerChangePeriod( xTimer, pdMS_TO_TICKS( eListenDurationMs ), portMAX_DELAY );
			pxComms->eListenTime = eListenDurationMs;
			break;
	}
	/* Turn the interface on or off */
	switch ( eListenDurationMs ) {
		case COMMS_LISTEN_OFF_IMMEDIATELY:
			pxComms->fnEnable( false );
			break;
		default:
			pxComms->fnEnable( true );
	}
	return;
}

/*-----------------------------------------------------------*/

/*
 * On listen timeout, turn off the communications interface
 * 
 * \param xTimer: 				Timer handle that expired
 */
void vCommsListenCallback( TimerHandle_t xTimer )
{
	xCommsInterface_t *pxContext = pvTimerGetTimerID( xTimer );
	pxContext->fnEnable( false );
}

/*-----------------------------------------------------------*/

void vUnifiedCommsBasicRouter( xCommsInterface_t *			 pxComms,
							   xUnifiedCommsIncomingRoute_t *pxCurrentRoute,
							   xUnifiedCommsMessage_t *	     pxMessage )
{
	uint8_t			 pucRoutingPacket[MAX_ROUTING_PACKET_SIZE];
	xPayloadType_t   xCompleteType = pxMessage->xPayloadType;
	xPayloadType_t   xPayloadType  = MASK_READ( xCompleteType, DESCRIPTOR_PACKET_TYPE_MASK );
	xBufferBuilder_t xBuilder;

	if (xPayloadType == UNIFIED_MSG_PAYLOAD_OUTGOING) {
		/* We only make the next hop if the packet came over serial, or if it was explicitly addressed to us */
		bool bSerialForward = ( pxComms->eInterface == COMMS_INTERFACE_SERIAL ) && bIsBaseAddress( pxMessage->xDestination );
		if ( !bIsLocalAddress( pxMessage->xDestination ) && !bSerialForward ) {
			return;
		}
		xUnifiedCommsMessage_t xUpdatedMessage = {
			.xSource = LOCAL_ADDRESS
		};
		/* Peel off the next route information from the packet and handle appropriately */
		const xUnifiedCommsRoutableHeader_t *pxHeader = (const xUnifiedCommsRoutableHeader_t *) pxMessage->pucPayload;
		uint8_t								 ucNextInterface;
		uint8_t								 ucNextChannel;
		/* Depending on number of hops remaining... */
		if ( pxHeader->ucNumHops == 1 ) {
			/* The next hop is the last hop */
			const xUnifiedCommsOutgoingLastHop_t *pxNextRoute = (const xUnifiedCommsOutgoingLastHop_t *) ( pxMessage->pucPayload + sizeof( xUnifiedCommsRoutableHeader_t ) );
			ucNextInterface									  = MASK_READ( pxNextRoute->xLastRoute.ucInterfaceAndChannel, COMMS_INTERFACE_MASK );
			ucNextChannel									  = MASK_READ( pxNextRoute->xLastRoute.ucInterfaceAndChannel, COMMS_CHANNEL_MASK );
			/* Set next message info */
			xUpdatedMessage.xDestination = xAddressUnpack( pxNextRoute->xLastRoute.pucHopAddress );
			xUpdatedMessage.xPayloadType = pxNextRoute->xPayloadType;
			xUpdatedMessage.pucPayload   = pxNextRoute->pucPayload;
			xUpdatedMessage.usPayloadLen = pxNextRoute->ucTotalLength - sizeof( xUnifiedCommsOutgoingLastHop_t );
		}
		else {
			/* The next hop is another boring hop, extract the next route */
			const xUnifiedCommsRoute_t *pxNextRoute = (const xUnifiedCommsRoute_t *) ( pxMessage->pucPayload + sizeof( xUnifiedCommsRoutableHeader_t ) );
			ucNextInterface							= MASK_READ( pxNextRoute->ucInterfaceAndChannel, COMMS_INTERFACE_MASK );
			ucNextChannel							= MASK_READ( pxNextRoute->ucInterfaceAndChannel, COMMS_CHANNEL_MASK );
			/* Repack the payload */
			const uint8_t *pucRemainingPayload   = pxMessage->pucPayload + sizeof( xUnifiedCommsRoutableHeader_t ) + sizeof( xUnifiedCommsRoute_t );
			const uint16_t usRemainingPayloadLen = pxMessage->usPayloadLen - sizeof( xUnifiedCommsRoutableHeader_t ) + sizeof( xUnifiedCommsRoute_t );
			vBufferBuilderStart( &xBuilder, pucRoutingPacket, MAX_ROUTING_PACKET_SIZE );
			/* New number of hops will be previous number - 1 */
			vBufferBuilderPushByte( &xBuilder, pxHeader->ucNumHops - 1 );
			/* Then the remaining payload */
			vBufferBuilderPushData( &xBuilder, pucRemainingPayload, usRemainingPayloadLen );
			/* Set next message info */
			xUpdatedMessage.xDestination = xAddressUnpack( pxNextRoute->pucHopAddress );
			xUpdatedMessage.xPayloadType = UNIFIED_MSG_PAYLOAD_OUTGOING;
			xUpdatedMessage.pucPayload   = xBuilder.pucBuffer;
			xUpdatedMessage.usPayloadLen = xBuilder.ulIndex;
		}
		/* Forward the outgoing packet via its desired route */
		switch ( ucNextInterface ) {
			case COMMS_INTERFACE_BLUETOOTH:
				xBluetoothComms.fnSend( ucNextChannel, &xUpdatedMessage );
				break;
			case COMMS_INTERFACE_GATT:
				xGattComms.fnSend( ucNextChannel, &xUpdatedMessage );
				break;
			case COMMS_INTERFACE_SERIAL:
			case COMMS_INTERFACE_LORA:
			case COMMS_INTERFACE_LORAWAN:
			default:
				/* Unimplemented interfaces */
				break;
		}
	} else {
		if (pxComms->eInterface == COMMS_INTERFACE_SERIAL) {
			/* Nothing to do for serial packets that aren't outgoing */
			return;
		}
		/* RF packet, forward it up serial */
		if (xPayloadType == UNIFIED_MSG_PAYLOAD_INCOMING) {
			/* All incoming messages are prepended with an additional route and forwarded up serial */
			const xUnifiedCommsRoutableHeader_t *pxHeader = (const xUnifiedCommsRoutableHeader_t *) pxMessage->pucPayload;
			vBufferBuilderStart( &xBuilder, pucRoutingPacket, MAX_ROUTING_PACKET_SIZE );
			/* New number of hops will be previous number + 1 */
			vBufferBuilderPushByte( &xBuilder, pxHeader->ucNumHops + 1 );
			/* Push the current route information onto the packet */
			vBufferBuilderPushData( &xBuilder, pxCurrentRoute, sizeof( xUnifiedCommsIncomingRoute_t ) );
			/* Push the remainder of the old payload */
			vBufferBuilderPushData( &xBuilder, pxMessage->pucPayload + 1, pxMessage->usPayloadLen - 1 );
		}
		else {
			/* Basic packets are prepended with first hop information */
			xUnifiedCommsRoutableHeader_t xHeader = {
				.ucNumHops = 1
			};
			xUnifiedCommsIncomingFirstHop_t xFirstHop = {
				.ucTotalLength = sizeof( xUnifiedCommsIncomingFirstHop_t ) + pxMessage->usPayloadLen,
				.xPayloadType  = xCompleteType
			};
			pvMemcpy( &xFirstHop.xFirstRoute, pxCurrentRoute, sizeof( xUnifiedCommsIncomingRoute_t ) );
			/* Everything else is to be forwarded up serial after adding the hop information */
			vBufferBuilderStart( &xBuilder, pucRoutingPacket, MAX_ROUTING_PACKET_SIZE );
			/* Push route information */
			vBufferBuilderPushData( &xBuilder, &xHeader, sizeof( xUnifiedCommsRoutableHeader_t ) );
			vBufferBuilderPushData( &xBuilder, &xFirstHop, sizeof( xUnifiedCommsIncomingFirstHop_t ) );
			/* Push payload */
			vBufferBuilderPushData( &xBuilder, pxMessage->pucPayload, pxMessage->usPayloadLen );
		}
		/* Update message information */
		xUnifiedCommsMessage_t xUpdatedMessage = {
			.xSource	  = LOCAL_ADDRESS,
			.xDestination = BROADCAST_ADDRESS,
			.xPayloadType = UNIFIED_MSG_PAYLOAD_INCOMING,
			.pucPayload   = xBuilder.pucBuffer,
			.usPayloadLen = xBuilder.ulIndex
		};
		/* Forward the packet */
		xSerialComms.fnSend( COMMS_CHANNEL_DEFAULT, &xUpdatedMessage );
	}
}

/*-----------------------------------------------------------*/
