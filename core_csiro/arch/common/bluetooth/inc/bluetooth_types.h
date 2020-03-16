/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_types.h
 * Creation_Date: 11/06/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Common Bluetooth GAP and GATT Structures 
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_TYPES
#define __CSIRO_CORE_BLUETOOTH_TYPES
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"

#include "address.h"
#include "compiler_intrinsics.h"

#include "bluetooth_sig.h"
#include "bluetooth_stack_defines.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define BLUETOOTH_GATT_MAX_SERVICES					5
#define BLUETOOTH_GATT_MAX_CHARACTERISTICS			16

/** 
 * Maximum supported MTU is 247.
 * This is not large enough to hit the next power of 2 (256),
 * we instead settle for 128 + some arbitrary overhead to save RAM
 **/
#define BLUETOOTH_GATT_MAX_MTU						220

#define BLUETOOTH_MAC_ADDRESS_LENGTH                MAC_ADDRESS_LENGTH

// clang-format on

/* Callback Definitions -------------------------------------*/

/* Predeclarations of the structs we define later for the function typedefs */
typedef struct xBluetoothConnection_t	  xBluetoothConnection_t;
typedef struct xGattRemoteCharacteristic_t xGattRemoteCharacteristic_t;
typedef struct xGattLocalCharacteristic_t  xGattLocalCharacteristic_t;

/**@brief Advertising Packet has been received
 *
 * 	This function is called by the bluetooth stack when it is scanning and an advertising packet has been received.
 * 
 * @note	This function is called directly by the stack event handler for efficiency reasons.
 *			Therefore the implementation of this function cannot initiate bluetooth commands that themselves block on an event from the stack.
 *			If such a situation occurs, event processing will stall, locking up the bluetooth controller.
 *
 * @note	The PHY address on packets broadcast from smartphones is typically not their true MAC address
 * 
 * @param[in] pucAddress			MAC Address of the device advertising the data as contained in the PHY packet
 * @param[in] eAddressType			Type of the received address as enumerated by the Bluetooth SIG
 * @param[in] cRssi					Signal Strength of the received packet in dBm
 * @param[in] pucData				The advertising data payload. For packets conforming to the bluetooth spec, this will contain AD data structures
 * @param[in] ucDataLen				Length of the advertising data payload
 */
typedef void ( *fnScanRecv_t )( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, int8_t cRssi, bool bConnectable, uint8_t *pucData, uint8_t ucDataLen );

/**@brief A Bluetooth GAP connection has been opened, configured and GATT attributes discovered
 *
 * For GATT discovery to respect desired behaviour, the discovery mode must be set before a connection has been initiated or advertising has begun
 * The data struct pointed to by pxConnection is valid for the lifetime of the connection ( until fnConnectionClosed_t callback is run )
 * 
 * @param[in] pxConnection			Reference handle for all further function calls regarding a connection
 */
typedef void ( *fnConnectionOpened_t )( xBluetoothConnection_t *pxConnection );

/**@brief A previously open GAP connection has closed
 * 
 * Further function calls referencing this connection will result in errors.
 *
 * @param[in] pxConnection			Closed connection
 */
typedef void ( *fnConnectionClosed_t )( xBluetoothConnection_t *pxConnection );

/**@brief A previously triggered RSSI measurement has completed
 * 
 * Not currently implemented on any stack.
 *
 * @param[in] pxConnection			Measured connection
 */
typedef void ( *fnConnectionRssi_t )( xBluetoothConnection_t *pxConnection );

/**@brief The connected remote device has written to a characteristic on the local GATT server
 * 
 * The pointer to the characteristic data is valid after this call returns, but may be updated at any time due to further writes
 *
 * @param[in] pxConnection			Connection Handle
 * @param[in] pxCharacteristic		Local characteristic that has been written to
 */
typedef void ( *fnLocalCharacteristicWritten_t )( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief The connected remote device has subscribed to a characteristic on the local GATT server
 * 
 * The mode the remote has subscribed with is contained in pxCharacteristic->usCCCDValue
 * This mode value must be preserved for future calls to eBluetoothDistributeLocalCharacteristic()
 *
 * @note	If multiple connections are present, each connection can subscribe to a single characteristic
 *
 * @param[in] pxConnection			Connection Handle
 * @param[in] pxCharacteristic		Local characteristic that has been subscribed to
 */
typedef void ( *fnLocalCharacteristicSubscribed_t )( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief A remote characteristic that the local device has previously subscribed to has changed
 *
 * @param[in] pxConnection			Connection Handle
 * @param[in] pxCharacteristic		Remote characteristic that has changed its value
 */
typedef void ( *fnRemoteCharacteristicChanged_t )( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic );

/**@brief A remote characteristic that the local device has ask to read has returned its value 
 *
 * @param[in] pxConnection			Connection Handle
 * @param[in] pxCharacteristic		Remote characteristic that has its value read
 */
typedef void ( *fnRemoteCharacteristicRead_t )( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic );

/* Type Definitions -----------------------------------------*/

/**@brief Asynchronous connection event callbacks */
typedef struct xBluetoothConnectionCallbacks_t
{
	fnConnectionOpened_t			  fnConnectionOpened;			   /**< A GATT connection has opened */
	fnConnectionClosed_t			  fnConnectionClosed;			   /**< The GATT connection has closed */
	fnConnectionRssi_t				  fnConnectionRssi;				   /**< RSSI of GATT connection was measured */
	fnLocalCharacteristicWritten_t	fnLocalCharacteristicWritten;	/**< Local GATT characteristic was updated by remote GATT client */
	fnLocalCharacteristicSubscribed_t fnLocalCharacteristicSubscribed; /**< Local GATT characteristic was subscribed to by remote GATT client */
	fnRemoteCharacteristicChanged_t   fnRemoteCharacteristicChanged;   /**< Remote GATT characteristic that we are subscribed to has changed value*/
	fnRemoteCharacteristicRead_t	  fnRemoteCharacteristicRead;	  /**< Remote GATT characteristic had its value read */
} xBluetoothConnectionCallbacks_t;

/**@brief  */
typedef struct xBluetoothAddress_t
{
	eBluetoothAddressType_t eAddressType;							  /**< Address type */
	uint8_t					pucAddress[BLUETOOTH_MAC_ADDRESS_LENGTH]; /**< Address bytes (LSB first) */
} xBluetoothAddress_t;

/**@brief Complete description of a GATT UUID */
typedef struct xBluetoothUUID_t
{
	bool bBluetoothOfficialUUID; /**< True when storage contains an official UUID */
	union
	{
		uint16_t usOfficialUUID; /**< 16-bit official Bluetooth SID UUID */
		struct
		{
			uint8_t pucUUID128[16];   /**< Complete 128-bit representation */
			uint8_t ucStackReference; /**< Implmentation specific reference to 128-bit UUID */
		} xCustomUUID;				  /**< 128-bit Custom UUID */
	} uUUID;
} xBluetoothUUID_t;

/**@brief Service Handle Formats 
 * 
 * Stacks refer to services either by the first and last attribute 
 * in the service or by a concatonation of the same.
 */
typedef union uServiceReference_t
{
	uint32_t ulServiceHandle; /**< Packed version of reference */
	struct ATTR_PACKED
	{
		uint16_t usRangeStop;  /**< Last attribute in the service */
		uint16_t usRangeStart; /**< First attribute in the service */
	} xHandleRange;			   /**< Range version of reference, element order is important  */
} uServiceReference_t;
CASSERT( sizeof( uServiceReference_t ) == 4, gatt_service_reference_size );

/**@brief GATT Service Description */
typedef struct xGattService_t
{
	xBluetoothUUID_t	xUUID;
	uServiceReference_t uServiceReference;
} xGattService_t;

/**@brief Remote GATT Characteristic Description */
struct xGattRemoteCharacteristic_t
{
	xBluetoothUUID_t			   xUUID;
	uServiceReference_t			   uServiceReference;
	uint16_t					   usCharacteristicHandle;
	uint16_t					   usCCCDHandle;
	eBleCharacteristicProperties_t eCharacteristicProperties;
	const uint8_t *				   pucData;
	uint16_t					   usDataLen;
};

/**@brief Local GATT Characteristic Description */
struct xGattLocalCharacteristic_t
{
	uint16_t	   usCharacteristicHandle; /**< Local Characteristic Handle */
	uint16_t	   usCCCDValue;			   /**< Value CCCD has been set to (Only valid on subscription calls) */
	const uint8_t *pucData;				   /**< Data written to local characteristic */
	uint16_t	   usDataLen;			   /**< Size of data written */
};

/**< Procedure to perform GATT Discovery */
enum eGattDiscoveryProcedure_t {
	GATT_DISCOVERY_NONE,	  /**< No GATT Discovery is done */
	GATT_DISCOVERY_AUTOMATIC, /**< The complete GATT table of the remote device is queried */
	GATT_DISCOVERY_MANUAL	 /**< Only the services and characteristics provided are discovered */
};

typedef enum eBluetoothConnectionState_t {
	BT_CONNECTION_IDLE			 = 0x01, /**< No connection present */
	BT_CONNECTION_PENDING		 = 0x02, /**< Connection has been requested, but is not yet established */
	BT_CONNECTION_CONNECTED		 = 0x04, /**< Connection is established and ready to use */
	BT_CONNECTION_OPERATION_DONE = 0x08  /**< Blocking operation is complete, result available in xPrivate.eError */
} eBluetoothConnectionState_t;

/**@brief Bluetooth Connection State 
 * 
 * Used for all application layer functions regarding GAP connections
 * GATT discovery behaviour is controlled via eGattDiscovery
 * 
 * If manual discovery is required, ucNumServices and ucNumCharacteristics must be initialised
 * 		* For each service and characteristic, the xUUID field must be correctly initialised
 * 
 * @note Each characteristic specified in xCharacteristics must be accompanied by its corresponding service in xServices
 */
struct xBluetoothConnection_t
{
	uint8_t							 ucConnectionHandle;									/**< Connection index for stack functions */
	xBluetoothAddress_t				 xRemoteAddress;										/**< Remote device we are connected to */
	xBluetoothConnectionCallbacks_t *pxCallbacks;											/**< Callbacks to run on asynchronous events */
	EventGroupHandle_t				 xConnectionState;										/**< Connection State, member of */
	bool							 bMaster;												/**< True if local device is the GAP Master */
	enum eGattDiscoveryProcedure_t   eGattDiscovery;										/**< Way to perform GATT discovery */
	uint8_t							 ucNumServices;											/**< Number of services on this connection */
	uint8_t							 ucNumCharacteristics;									/**< Number of characteristics on this connection */
	xGattService_t					 pxServices[BLUETOOTH_GATT_MAX_SERVICES];				/**< Service descriptions */
	xGattRemoteCharacteristic_t		 pxCharacteristics[BLUETOOTH_GATT_MAX_CHARACTERISTICS]; /**< Characteristic descriptions */
	struct xPrivateState_t
	{
		eModuleError_t eError;			  /**< Reserved */
		uint32_t	   ulGattOperation;   /**< Reserved */
		uint8_t		   ucServicesQueried; /**< Reserved */
		uint8_t		   ucIndex;			  /**< Reserved */
	} xPrivate;							  /**< Private State Information */
};

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_BLUETOOTH_TYPES */
