/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_sig.h
 * Creation_Date: 22/08/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Bluetooth Special Interest Group (SIG) definitions
 * 
 * Bluetooth SIG is the standards body responsible for the Bluetooth specification.
 * The Bluetooth SIG board consists of employees from the following organisations (as of 26/8/2019):
 * 			Microsoft
 * 			Intel
 * 			Apple
 * 			Bose
 * 			Ericsson AB
 * 			Motorola
 * 			Nokia
 * 			Telink Semiconductor
 * 			Toshiba
 */
#ifndef __CSIRO_CORE_BLUETOOTH_SIG
#define __CSIRO_CORE_BLUETOOTH_SIG
/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

#define BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH 31

/* Bluetoth SIG GAP Types ------------------------------------*/

/**@brief Bluetooth SIG Advertising Data Types (AD Types)
 * 
 * A single Bluetooth advertising packet is constructed from multiple AD structures
 * 
 * Populated from:
 *      https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile
 */
typedef enum eADTypes_t {
	BLE_AD_TYPE_FLAGS				   = 0x01, /**< Description of the advertising packet */
	BLE_AD_TYPE_INCOMPLETE_16BIT_UUID  = 0x02, /**< Incomplete list of 16bit Service UUID's present on the advertising device */
	BLE_AD_TYPE_COMPLETE_16BIT_UUID	= 0x03, /**< Complete list of 16bit Service UUID's present on the advertising device */
	BLE_AD_TYPE_INCOMPLETE_32BIT_UUID  = 0x04, /**< Incomplete list of Service 32bit UUID's present on the advertising device */
	BLE_AD_TYPE_COMPLETE_32BIT_UUID	= 0x05, /**< Complete list of 32bit Service UUID's present on the advertising device */
	BLE_AD_TYPE_INCOMPLETE_128BIT_UUID = 0x06, /**< Incomplete list of 128bit Service UUID's present on the advertising device */
	BLE_AD_TYPE_COMPLETE_128BIT_UUID   = 0x07, /**< Complete list of 128bit Service UUID's present on the advertising device */
	BLE_AD_TYPE_SHORTENED_LOCAL_NAME   = 0x08, /**< Truncated version of the complete local name of the advertising device */
	BLE_AD_TYPE_COMPLETE_LOCAL_NAME	= 0x09, /**< Complete local name of the advertising device */
	BLE_AD_TYPE_TX_POWER_LEVEL		   = 0x0A  /**< Transmit power of the advertisment data in dBm (Signed 8bit number) */
} ATTR_PACKED eADTypes_t;

/**@brief Bluetooth SIG Advertising Data Flags
 * 
 * Field description of the AD Type "Flags"
 * 
 * Bluetooth Core Specification Supplement Part A 1.3
 */
typedef enum eADFlags_t {
	BLE_ADV_FLAGS_LE_LIMITED_DISC_MODE = 0b00000001, /**< LE Limited Discoverable Mode. */
	BLE_ADV_FLAGS_LE_GENERAL_DISC_MODE = 0b00000010, /**< LE General Discoverable Mode. */
	BLE_ADV_FLAGS_BR_EDR_NOT_SUPPORTED = 0b00000100, /**< BR/EDR not supported. */
	BLE_ADV_FLAGS_LE_BR_EDR_CONTROLLER = 0b00001000, /**< Simultaneous LE and BR/EDR, Controller. */
	BLE_ADV_FLAGS_LE_BR_EDR_HOST	   = 0b00010000, /**< Simultaneous LE and BR/EDR, Host. */
	BLE_ADV_FLAGS_RESERVED			   = 0b11100000  /**< Reserved for future use */
} ATTR_PACKED eADFlags_t;

/**@brief Bluetooth SIG Advertising Data Structure
 * 
 * A single Bluetooth advertising packet is constructed from multiple AD structures
 * Each AD structure consists of a length field, the AD type contained, followed by the AD type data
 * 
 * |--------------- Advertising Data ------------|
 * |--- AD Structure 0 ---|--- AD Structure 1 ---|
 * | Length | Type | Data | Length | Type | Data |
 */
typedef struct xADHeader_t
{
	uint8_t ucLength;
	uint8_t ucType;
} ATTR_PACKED xADHeader_t;

/**@brief Bluetooth SIG Advertising Data Flags Structure
 * 
 * Complete Description of a "Flags" AD Structure
 */
typedef struct xADFlagsStructure_t
{
	xADHeader_t xHeader;
	uint8_t		ucFlags;
} ATTR_PACKED xADFlagsStructure_t;

typedef struct xADCompleteLocalNameStructure_t
{
	xADHeader_t xHeader;
	uint8_t		ucName[];
} ATTR_PACKED xADCompleteLocalNameStructure_t;

/* Bluetoth SIG GATT Attributes -----------------------------*/

typedef uint16_t xBleHandle_t;

/**@brief Bluetooth SIG Attribute Types
 * 
 * Bluetooth Core Specification 5.1, Vol 3, Part G, 3.4
 * 
 */
typedef enum eBleAttributeType_t {
	BLE_ATTRIBUTE_TYPE_PRIMARY_SERVICE_DECLARATION		   = 0x2800, /**< Primary Service Declaration */
	BLE_ATTRIBUTE_TYPE_SECONDARY_SERVICE_DECLARATION	   = 0x2801, /**< Secondary Service Declaration, Secondary services are only included in Primary Services */
	BLE_ATTRIBUTE_TYPE_INCLUDE_DECLARATION				   = 0x2802, /**< Include Declaration */
	BLE_ATTRIBUTE_TYPE_CHARACTERISTIC_DECLARATION		   = 0x2803, /**< Characteristic Declaration */
	BLE_ATTRIBUTE_TYPE_CHARACTERISTIC_EXTENDED_PROPERTIES  = 0x2900, /**< Characteristic Extended Properties */
	BLE_ATTRIBUTE_TYPE_CHARACTERISTIC_USER_DESCRIPTION	 = 0x2901, /**< Characteristic User Description, UTF-8 String */
	BLE_ATTRIBUTE_TYPE_CLIENT_CHARACTERISTIC_CONFIGURATION = 0x2902, /**< Client Characteristic Configuration */
	BLE_ATTRIBUTE_TYPE_SERVER_CHARACTERISTIC_CONFIGURATION = 0x2903, /**< Service Characteristic Configuration */
	BLE_ATTRIBUTE_TYPE_CHARACTERISTIC_PRESENTATION_FORMAT  = 0x2904, /**< Characteristic Presentation Format */
	BLE_ATTRIBUTE_TYPE_CHARACTERISTIC_AGGREGATE_FORMAT	 = 0x2905  /**< Characteristic Aggregate Format */
} ATTR_PACKED eBleAttributeType_t;

/**@brief Bluetooth SIG "Include Declaration"
 * 
 * Bluetooth Core Specification 5.1, Vol 3, Part G, 3.2
 * 
 * Used to nest a secondary service within the service containing this attribute
 * Circular include declarations are invalid as per the Bluetooth Spec
 */
typedef struct xBleIncludeDeclaration_t
{
	xBleHandle_t xIncludedServiceAttributeHandle; /**< Service that is being included in this service */
	xBleHandle_t xEndGroupHandle;				  /**< Service that is being included in this service */
	uint16_t	 usServiceUUID;					  /**< Only included when UUID is a 16-bit Official UUID*/
} ATTR_PACKED xBleIncludeDeclaration_t;

/**@brief Bluetooth SIG "Characteristic Properties"
 * 
 * Bluetooth Core Specification 5.1, Vol 3, Part G, 3.3.1.1
 * 
 * Specifies how a characteristics can be used. If the appropriate bit is set, that action is permitted.
 * These bits are set without regatds for security requirements.
 */
typedef enum eBleCharacteristicProperties_t {
	BLE_CHARACTERISTIC_PROPERTY_BROADCAST		  = 0x01, /**< Permits broadcasts of the characteristic value */
	BLE_CHARACTERISTIC_PROPERTY_READ			  = 0x02, /**< Permits reads of the characteristic value */
	BLE_CHARACTERISTIC_PROPERTY_WRITE_NO_RESPONSE = 0x04, /**< Permits writing to the characteristic value without a response */
	BLE_CHARACTERISTIC_PROPERTY_WRITE			  = 0x08, /**< Permits writing to the characteristic value with a response */
	BLE_CHARACTERISTIC_PROPERTY_NOTIFY			  = 0x10, /**< Permits notification of changes to the characteristic value without acknowledgement */
	BLE_CHARACTERISTIC_PROPERTY_INDICATE		  = 0x20, /**< Permits indication of changes to the characteristic value with acknowledgement */
	BLE_CHARACTERISTIC_PROPERTY_AUTH_SIGNED_WRITE = 0x40, /**< Permits signed writes to the characteristic value */
	BLE_CHARACTERISTIC_PROPERTY_EXTENDED		  = 0x80  /**< Additional characteristic properties are defined in an Extended Properties Descriptor */
} ATTR_PACKED eBleCharacteristicProperties_t;

/**@brief Bluetooth SIG "Client Characteristic Configuration Descriptor"
 * 
 * Bluetooth Core Specification 5.1, Vol 3, Part G, 3.3.3.3
 * 
 * Unlike other characteristics, each connected client to the GATT server has a unique instance of the characteristic
 * Each bit can only be set if the corresponding bit in the "Characteristic Properties" is set
 * 
 * All other bits are reserved for future use
 */
typedef enum eBleClientCharacteristicConfiguration_t {
	BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_NOTIFICATION = 0x01, /**< Characteristic value shall be notified on change */
	BLE_CLIENT_CHARACTERISTIC_CONFIGURATION_INDICATION   = 0x02  /**< Characteristic value shall be indicated on change */
} ATTR_PACKED eBleClientCharacteristicConfiguration_t;

#endif /* __CSIRO_CORE_BLUETOOTH_SIG */
