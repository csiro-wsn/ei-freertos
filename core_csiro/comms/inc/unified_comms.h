/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: unified_comms.h
 * Creation_Date: 21/11/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 * 
 * Description of unified communications interfaces.
 * 
 * Allows underlying RF / Serial layers to be interchanged with no
 * function prototype changes
 */

#ifndef __CSIRO_CORE_COMMS_UNIFIED_COMMS
#define __CSIRO_CORE_COMMS_UNIFIED_COMMS

/* Includes -------------------------------------------------*/

#include <stdbool.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "address.h"
#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/
// clang-format off

/**
 * Number of independant packet types gauranteed to be supported by all interfaces
 */
#define NUM_PAYLOAD_TYPES 		8

#define COMMS_INTERFACE_MASK	0xF0
#define COMMS_CHANNEL_MASK		0x0F

// clang-format on

/* Packet Definitions ---------------------------------------*/

#define DESCRIPTOR_PACKET_TYPE_MASK		0b00000111
#define DESCRIPTOR_BROADCAST_MASK		0b00010000
#define DESCRIPTOR_ENCRYPTED_MASK		0b00100000

/** @brief CSIRO Packet types
 * 
 */
typedef enum eCsiroPayloadType_t {
	UNIFIED_MSG_PAYLOAD_TDF3	 = 0x00, /**< TDF3 packet type. */
	UNIFIED_MSG_PAYLOAD_OTI		 = 0x01, /**< OTI packet type. */
	UNIFIED_MSG_PAYLOAD_VTI		 = 0x02, /**< VTI packet type. */
	UNIFIED_MSG_PAYLOAD_RPC		 = 0x03, /**< RPC packet type. */
	UNIFIED_MSG_PAYLOAD_RPC_RESP = 0x04, /**< RPC response packet type. */
	UNIFIED_MSG_PAYLOAD_INCOMING = 0x05, /**< Outgoing payload */
	UNIFIED_MSG_PAYLOAD_OUTGOING = 0x06, /**< Incoming payload*/
} ATTR_PACKED eCsiroPayloadType_t;

/* Interface Definitions ------------------------------------*/

typedef enum eCommsInterface_t {
	COMMS_INTERFACE_SERIAL	= 0,
	COMMS_INTERFACE_BLUETOOTH = 1,
	COMMS_INTERFACE_GATT	  = 2,
	COMMS_INTERFACE_LORA	  = 3,
	COMMS_INTERFACE_LORAWAN   = 4
} ATTR_PACKED eCommsInterface_t;

typedef enum eCommsChannel_t {
	COMMS_CHANNEL_DEFAULT = 0
} ATTR_PACKED eCommsChannel_t;

typedef uint8_t xPayloadType_t; /**< 3 bits payload type (0b00000111), 5 bits configuration (0b11111000) */

/* Type Definitions -----------------------------------------*/

typedef enum eCommsListen_t {
	COMMS_LISTEN_OFF_IMMEDIATELY = 0,
	COMMS_LISTEN_ON_FOREVER		 = UINT32_MAX
} eCommsListen_t;

/**<@brief Base Message Structure & Metadata */
typedef struct xUnifiedCommsMessage_t
{
	xAddress_t	 xSource;		 /**< Message source */
	xAddress_t	 xDestination; /**< Intended message recipient */
	xPayloadType_t xPayloadType; /**< Message type */
	const uint8_t *pucPayload;   /**< Message payload */
	uint16_t	   usPayloadLen; /**< Message payload length */
} ATTR_PACKED xUnifiedCommsMessage_t;

/**<@brief Basic Route Information */
typedef struct xUnifiedCommsRoute_t
{
	uint8_t pucHopAddress[MAC_ADDRESS_LENGTH]; /**< Next or Previous Address */
	uint8_t ucInterfaceAndChannel;			   /**< Packet Interface (top 4 bits) and Channel (bottom 4 bits) */
} ATTR_PACKED xUnifiedCommsRoute_t;

/**<@brief Received Route Metadata */
typedef struct xUnifiedCommsRouteMetadata_t
{
	uint16_t usPacketAge;	  /**< Time between packet reception and packet forwarding */
	uint8_t  ucSequenceNumber; /**< Interface sequence number */
	uint8_t  ucRssi;		   /**< Received signal strength, (Actual = 30 - ucRssi) (dBm) */
} ATTR_PACKED xUnifiedCommsRouteMetadata_t;

/**<@brief Received Route Metadata */
typedef struct xUnifiedCommsIncomingRoute_t
{
	xUnifiedCommsRoute_t		 xRoute;
	xUnifiedCommsRouteMetadata_t xMetadata;
} ATTR_PACKED xUnifiedCommsIncomingRoute_t;

/**<@brief Additional Information surrounding the first hop of the packet */
typedef struct xUnifiedCommsIncomingFirstHop_t
{
	uint8_t						 ucTotalLength; /**< Total Length of this struct + the payload */
	xPayloadType_t				 xPayloadType;  /**< Original payload type */
	xUnifiedCommsIncomingRoute_t xFirstRoute;   /**< Route of the first hop */
	uint8_t						 pucPayload[0]; /**< Empty payload pointer */
} ATTR_PACKED xUnifiedCommsIncomingFirstHop_t;

/**<@brief Additional Information surrounding the last hop of the packet */
typedef struct xUnifiedCommsOutgoingLastHop_t
{
	uint8_t				 ucTotalLength; /**< Total Length of this struct + the payload */
	xPayloadType_t		 xPayloadType;  /**< Final payload type */
	xUnifiedCommsRoute_t xLastRoute;	/**< Route of the last hop */
	uint8_t				 pucPayload[0]; /**< Empty payload pointer */
} ATTR_PACKED xUnifiedCommsOutgoingLastHop_t;

/**<@brief Routable packets header (INCOMING and OUTGOING) */
typedef struct xUnifiedCommsRoutableHeader_t
{
	uint8_t ucNumHops; /**< Number of hops this header contains information about */
} ATTR_PACKED xUnifiedCommsRoutableHeader_t;

typedef struct xCommsInterface_t xCommsInterface_t;

/**@brief Initialises any module specific requirements
 * 
 * @note  	Function will only be called once on startup
 * 
 * @retval ::ERROR_NONE 	Module successfully initialised
 */
typedef eModuleError_t ( *eCommsInit_t )( void );

/**@brief Turns the interface ON or OFF
 * 
 * @param[in] bEnable		True to enable interface, false to disable
 * 
 * @retval ::ERROR_NONE 	Module state successfully set
 */
typedef eModuleError_t ( *eCommsEnable_t )( bool bEnable );

/**@brief Send a payload over the interface
 * 
 * @param[in] eChannel		Interface channel to send message on
 * @param[in] pxMessage		Message to send
 * 
 * @retval ::ERROR_NONE 			Payload successfully sent, or queued to send automatically
 * @retval ::ERROR_INVALID_ADDRESS  Destination address is not available on this interface
 * @retval ::ERROR_INVALID_STATE  	Interface is not in the correct state to send this data
 * @retval ::ERROR_GENERIC  		Arbitrary error
 */
typedef eModuleError_t ( *eCommsSend_t )( eCommsChannel_t		  eChannel,
										  xUnifiedCommsMessage_t *pxMessage );

/**@brief Function to call when a packet is received
 * 
 * @param[in] pxComms				Pointer to the interface
 * @param[in] pxRouteInformation	Route Information of current hop
 * @param[in] pxMessage				Message Data
 */
typedef void ( *vCommsReceiveHandler_t )( xCommsInterface_t *			pxComms,
										  xUnifiedCommsIncomingRoute_t *pxCurrentRoute,
										  xUnifiedCommsMessage_t *		pxMessage );

/**@brief Implementation of a unified comms interface */
struct xCommsInterface_t
{
	eCommsInterface_t	  eInterface;					/**< Type of the underlying communication channel. */
	eCommsInit_t		   fnInit;						/**< Initialisation function. */
	eCommsEnable_t		   fnEnable;					/**< Enable/disable function. */
	eCommsSend_t		   fnSend;						/**< Send function. */
	vCommsReceiveHandler_t fnReceiveHandler;			/**< Function to call on packet reception. */
	TimerHandle_t		   xListenTimer;				/**< Listen duration timer */
	eCommsListen_t		   eListenTime;					/**< How long the interface is currently enabled for.  */
};

/* Function Declarations ------------------------------------*/

/**
 * @brief Initialise a Unified Comms interface
 * 
 * @param[in] pxComms	Interface to initialise
 */
void vUnifiedCommsInit( xCommsInterface_t *pxComms );

/**
 * @brief Turn a interface into receive mode for the specified duration
 * 
 * 	Timed durations are compared against the current remaining time, with the timer being updated
 * 	if the new duration is longer. COMMS_LISTEN_ON_FOREVER is treated as infinite time for this purpose.
 * 	COMMS_LISTEN_OFF_IMMEDIATELY is a special case which immediately overrides any remaining time.
 * 
 * @param[in] pxComms				Interface to enable
 * @param[in] eListenDurationMs		COMMS_LISTEN_ON_FOREVER, interface it enabled until a call to COMMS_LISTEN_OFF_IMMEDIATELY
 * 									COMMS_LISTEN_OFF_IMMEDIATELY, interface is disabled immediately
 * 									Other, receive duration in milliseconds
 */
void vUnifiedCommsListen( xCommsInterface_t *pxComms, eCommsListen_t eListenDurationMs );

/**
 * @brief Called by comms drivers to retrieve encryption key
 * 
 * @param[in] 	pxInterface 		Interface that is sending the packet
 * @param[in] 	eType 				Payload type to be encrypted
 * @param[in] 	xDestination 		Payload destination
 * @param[out] 	ppucEncryptionKey   If provided, *ppucEncryptionKey points to key
 * 
 * @return 		true 		Encryption Key was provided
 * @return 		false 		Encryption Key was NOT provided
 */
bool bUnifiedCommsEncryptionKey(xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xDestination, uint8_t **ppucEncryptionKey );

/**
 * @brief Called by comms drivers to retrieve decryption key
 * 
 * @param[in] 	pxInterface 		Interface that recevied the packet
 * @param[in] 	eType 				Payload type to be decrypted
 * @param[in] 	xDestination 		Payload destination
 * @param[out] 	ppucDecryptionKey   If provided, *ppucDecryptionKey points to key
 * 
 * @return 		true 		Decryption Key was provided
 * @return 		false 		Decryption Key was NOT provided
 */
bool bUnifiedCommsDecryptionKey(xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xSource, uint8_t *ppucDecryptionKey[16] );

/**
 * @brief Implmentation of vCommsReceiveHandler_t for a basic router
 *
 * 		Serial packets of type UNIFIED_MSG_PAYLOAD_OUTGOING are forwarded over specified RF interface
 * 		All other packets are forwarded up the serial interface
 */
void vUnifiedCommsBasicRouter( xCommsInterface_t *			 pxComms,
							   xUnifiedCommsIncomingRoute_t *pxCurrentRoute,
							   xUnifiedCommsMessage_t *	     pxMessage );

#endif /* __CSIRO_CORE_COMMS_UNIFIED_COMMS */
