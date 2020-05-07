/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: unified_comms_bluetooth.h
 * Creation_Date: 09/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 *  CSIRO Unified Packet Interface for Bluetooth Comms
 * 
 * 	USER DATA ENCODING 
 *  https://confluence.csiro.au/display/pacp/On-Air+Packet+Format
 * 
 *  Relevant Documentation
 *  Bluetooth Core Specification Version 5.1, Volume 3, Part C, 11
 *  
 *  Due to IOS Limitations, data must be transmitted as the 'Complete Local Name' AD Type.
 *  The name must be a valid UTF-8 string, requiring a binary<->text encoding scheme.
 *  From the 31 bytes raw advertising packet, we end up with a 26 byte UTF-8 name string.
 *  Most efficient binary packing for this length is base85, which packs 4 bytes of binary data to 5 bytes of ASCII (UTF-8 subset) data.
 *  26 characters can therefore encode 20 bytes of arbitrary data.
 *  The extra character can encode 6 bits, which we use for the payload type and metadata. 
 * 
 *  RAW PACKET:		[ 31 BYTES ADVERTISING DATA ]
 * 
 *  AD FIELDS:		[ 3 BYTE FLAGS ] [ 28 BYTE COMPLETE LOCAL NAME]
 * 
 *  AD EXPANDED:	[ 0x02 0x01 0x1A ] [ 27 0x09 LOCAL_NAME ]
 * 
 *  LOCAL_NAME:		[ PACKET_DESC ][ 25 BYTE BASE85 ENCODED STRING ]
 * 
 *  PACKET_DESC:    [ 0b00 ENCRYPTED:1 BROADCAST:1 RESERVED:1 PAYLOAD_TYPE:3 ]
 * 
 *  BINARY DATA:	[ 20 BYTE BINARY DATA ]
 * 					[ 1 BYTE SEQUENCE] [ 6 BYTE ADDRESS ] [ 13 BYTE PAYLOAD ]
 * 
 *  As IOS hashes MAC addresses, we duplicate the source address as destination address on broadcast packets (typically FF:FF:FF:FF:FF:FF).
 *  The broadcast bit in the packet descriptor is set to indicate this substitution.
 * 
 *  The encryption bit is used to indicate if encryption has been applied to the packet.
 *  Note that the encryption as applied to advertising packets is not cryptographically secure in any meaningfull sense.
 *  There is not enough space to transmit either a initialisation vector or message authentication code.
 *  As we don't respect any of the preconditions of AES encryption, we don't get any of its associated garauntees.
 *  Therefore the implementation should be thought of as a data randomisation scheme, not true encryption.
 */

#ifndef __CSIRO_CORE_COMMS_UNIFIED_BLUETOOTH
#define __CSIRO_CORE_COMMS_UNIFIED_BLUETOOTH

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

#include "unified_comms.h"

#include "bluetooth_types.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH             ( BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH - sizeof(xADFlagsStructure_t) - sizeof(xADHeader_t) )
#define BLE_UNIFIED_COMMS_LOCAL_NAME_BINARY_MAX_LENGTH		( BLE_UNIFIED_COMMS_LOCAL_NAME_MAX_LENGTH * 4 / 5 )

#define CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH                  ( BLE_UNIFIED_COMMS_LOCAL_NAME_BINARY_MAX_LENGTH - 7 )
#define CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH                  ( CSIRO_BLUETOOTH_PAYLOAD_MAX_LENGTH * 4 ) /* Max of 4 packets */

CASSERT( BLE_UNIFIED_COMMS_LOCAL_NAME_BINARY_MAX_LENGTH == 20, binary_max_len )

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eUnifiedCommsBluetoothChannel_t {
	COMMS_CHANNEL_BLUETOOTH_DEFAULT = 0,
	COMMS_CHANNEL_BLUETOOTH_PHONE   = 1
} eUnifiedCommsBluetoothChannel_t;

typedef void ( *vCustomPacketHandler_t )( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, 
										int8_t cRssi, bool bConnectable, 
										uint8_t *pucData, uint8_t ucDataLen );

/* Variable Declarations ------------------------------------*/

/*
 * Bluetooth Communication Interface
 */
extern xCommsInterface_t xBluetoothComms;

/* Function Declarations ------------------------------------*/

/**
 * @brief Sets the conectable flag in the Bluetooth advertiment flag
 * 
 * @param bStatus Set false to make advertiment not connectable
 */
void vUnifiedCommsBluetoothAdvertisingIsConnectable( bool bConnectable );

/**
 * @brief Get Gatt Connectability status of node
 * 
 * @return bool True if last packet was connectable
 */
bool bUnifiedCommsBluetoothWasConnectable( void );

/**
 * @brief Set initialisation vector used for encryption
 * 
 * 	Advertising interface does not have enough space to send a complete initialisation vector
 *  Therefore we set the last 13 bytes to be some deployment specific constant
 *  The first 3 bytes are the unencypted portion of the MAC address
 * 
 * @param[in] 	pucInitialisationTail	Last 13 bytes of the initialisation vector
 */
void vUnifiedCommsBluetoothSetInitialisationVector( uint8_t pucInitialisationTail[13]);

/**
 * @brief Set decryption validator for bluetooth advertising
 * 
 *  Bluetooth advertising cannot self validate the decrypted contents
 * 	fnChecker should return true if decryption results in a valid packet, false otherwise
 *  The first three bytes of pucData hold the 3 MSB's of the packet address 
 * 
 * @param[in] 	fnChecker		Validator function
 */
void vUnifiedCommsBluetoothDecryptionChecker( bool (*fnChecker)(uint8_t *pucData, uint8_t ucDataLen));

/**@brief Set handler function for unfiltered advertisement packets  
 * 
 * If set, the provided function will be called for ALL Bluetooth advertisement
 * packets observed.
 * 
 * @param[in] 	fnPacketHandler		Custom packet handler
 */
void vUnifiedCommsBluetoothCustomHandler( vCustomPacketHandler_t fnPacketHandler );

#endif /* __CSIRO_CORE_COMMS_UNIFIED_BLUETOOTH */
