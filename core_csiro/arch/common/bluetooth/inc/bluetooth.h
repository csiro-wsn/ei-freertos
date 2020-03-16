/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth.h
 * Creation_Date: 06/06/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Core Bluetooth Driver API
 * 
 * Bluetooth Specifications
 * https://www.bluetooth.com/specifications/bluetooth-core-specification
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH
#define __CSIRO_CORE_BLUETOOTH
/* Includes -------------------------------------------------*/

#include <stdbool.h>

#include "FreeRTOS.h"

#include "bluetooth_types.h"

#include "bluetooth_gatt_arch.h"

/* Module Defines -------------------------------------------*/

// clang-format off
#define BLUETOOTH_MAX_QUEUED_ADV_PACKETS 			16	/**< Maximum number of advertising packets in a sequence */

// clang-format on

/* Type Definitions -----------------------------------------*/

/**@brief Channel Scanning Configuration */
typedef struct xBluetoothScanParameters_t
{
	enum eBluetoothPhy_t ePhy;			   /**< Physical Layer to scan on */
	uint16_t			 usScanIntervalMs; /**< Period at which to switch advertising channels */
	uint16_t			 usScanWindowMs;   /**< Duration to listen on advertising channel after switching */
	fnScanRecv_t		 fnCallback;	   /**< Function to call when advertising packets are observed */
} xBluetoothScanParameters_t;

/**@brief Packet Advertising Parameters */
typedef struct xBluetoothAdvertiseParameters_t
{
	enum eBluetoothPhy_t ePhy;											   /**< Physical Layer to advertise on */
	uint8_t				 ucAdvertiseCount;								   /**< Number of time this packet will be advertised */
	uint8_t				 pucData[BLUETOOTH_LEGACY_ADVERTISING_MAX_LENGTH]; /**< Data to advertise */
	uint8_t				 ucDataLen;										   /**< Length of valid data in pucData */
	bool				 bStartSequence;								   /**< Advertise now or wait for more packets */
	bool				 bAdvertiseConnectable;							   /**< Advertise as connectable */
} xBluetoothAdvertiseParameters_t;

/**@brief Connection Configuration */
typedef struct xBluetoothConnectionParameters_t
{
	uint16_t usConnectionInterval;  /**< Event timing desired for this connection, 1.25ms units */
	uint16_t usSlaveLatency;		/**< Number of events that can be skipped by the peripheral (GATT client) */
	uint16_t usSupervisorTimeoutMs; /**< Timeout for connection when not heard */
} xBluetoothConnectionParameters_t;

/* Function Declarations ------------------------------------*/

/**@brief Initialise the Bluetooth driver
 *
 * @note Depending on the underlying architecture, the bluetooth stack may have to be setup before operations on
 * 			Flash Memory and Interrupts can occur.
 *
 * @retval ::ERROR_NONE 					Driver was successfully initialised
 * @retval ::ERROR_INITIALISATION_FAILURE 	Initialisation failed, driver is not available for use
 */
eModuleError_t eBluetoothInit( void );

/**@brief Determine the type of a bluetooth address based on MAC
 *
 * 	The type of a bluetooth address is determined by its 2 highest bits, as per
 * 		Bluetoth Core Specification Volume 6 Part B 1.3
 * 
 * @param[in] pxAddress			Bluetooth Address
 * 
 * @retval						Type of the address
 */
eBluetoothAddressType_t eBluetoothAddressType( xBluetoothAddress_t *pxAddress );

/**@brief Set the local bluetooth address of this device
 *
 * @param[out] pxLocalAddress		New local address
 * 
 * @note 	Depending on stack implementation, this address may not be used until the next reboot
 * @note	Depending on stack implementation, this command may override the default factory address
 * 
 * @retval	::ERROR_NONE			Address set successfully
 * @retval	::ERROR_INVALID_DATA	Address provided was invalid
 */
eModuleError_t eBluetoothSetLocalAddress( xBluetoothAddress_t *pxAddress );

/**@brief Retrieve the local bluetooth address of this device
 *
 * @param[out] pxLocalAddress			Local address
 */
void vBluetoothGetLocalAddress( xBluetoothAddress_t *pxLocalAddress );

/**@brief Set the TX power used across the Bluetooth driver
 *
 * @note The power set here is used for all operations, advertising, connections, etc
 *
 * @param[in] cTxPowerDbm		Requested TX power in dBm ( decibel-milliwatts )
 * 
 * @retval						Actual TX Power in dBm ( decibel-milliwatts )
 */
int8_t cBluetoothSetTxPower( int8_t cTxPowerDbm );

/**@brief Retrieve the current TX power
 *
 * @note The power set here is used for all operations, advertising, connections, etc
 *
 * @retval		Current TX Power in dBm ( decibel-milliwatts )
 */
int8_t cBluetoothGetTxPower( void );

/**@brief Setup Bluetooth scanning parameters
 *
 * @param[in] pxScanParameters		Channel configuration
 * 
 * @retval	::ERROR_NONE			Scanning configured successfully
 * @retval	::ERROR_INVALID_DATA	Provided parameters were invalid
 */
eModuleError_t eBluetoothConfigureScanning( xBluetoothScanParameters_t *pxScanParameters );

/**@brief Setup Bluetooth GAP connection parameters
 *
 * @param[in] pxConnectionsParameters		Connection configuration
 * 
 * @retval	::ERROR_NONE			Connection configured successfully
 * @retval	::ERROR_INVALID_DATA	Provided parameters were invalid
 */
eModuleError_t eBluetoothConfigureConnections( xBluetoothConnectionParameters_t *pxConnectionsParameters );

/**@brief Start scanning the Bluetooth advertising channels
 *
 * Advertising packets that are observed while scanning will run the fnScanRecv() callback.
 * 
 * @note 	Physical hardware can only scan on one of channels at once, underlying stacks
 * 			therefore iterate over the advertising channels repeatedly with durations 
 * 			specified in the function arguments.
 * 
 * @note 	Advertising packets that are partway through transmission when a channel swap
 * 			is scheduled will typically be discarded and lost
 *
 * @param[in] ePHY					Physical layer to scan on
 * 
 * @retval	::ERROR_NONE			Scanning started successfully
 * @retval	::ERROR_INVALID_DATA	Provided parameters were invalid
 * @retval	::ERROR_INVALID_STATE	Driver was in an incorrect state to start scanning
 */
eModuleError_t eBluetoothScanStart( eBluetoothPhy_t ePHY );

/**@brief Stops the currently active scanning configuration
 *
 * @param[out] pePHY				Physical layer that was scanning, can be NULL
 * 
 * @retval	::ERROR_NONE			Scanning stopped successfully
 * @retval	::ERROR_INVALID_STATE	Driver was not previously scanning
 */
eModuleError_t eBluetoothScanStop( eBluetoothPhy_t *pePHY );

/**@brief Advertise the smallest possible data packet as a potential connection event
 *
 * @retval	::ERROR_NONE					Advertising started successfully
 * @retval 	::ERROR_UNAVAILABLE_RESOURCE	No free buffers to queue packet in
 * @retval	::ERROR_INVALID_DATA			Provided parameters were invalid
 * @retval	::ERROR_INVALID_STATE			Driver was in an incorrect state to start advertising
 */
eModuleError_t eBluetoothAdvertisePing( void );

/**@brief Start advertising the provided packet.
 *
 * 	Advertising packets are advertised on all 3 Bluetooth advertising channels sequentially from a single call.
 * 
 * 	Advertising packets can be 'chained' together by using the xBluetoothAdvertiseParameters_t::bStartSequence
 * 	parameter. Packets will be queued until a call is made with the value set to true.
 *  Packets in a sequence are advertised in the order they were provided to eBluetoothAdvertise.
 * 	Packets in a sequence can have unique ucAdvertiseCount values, with packets being dropped as the count expires.
 * 
 *	In order to ensure prompt packet transmission, the current scanning configuration
 *	is stopped until the complete advertising chain is complete, at which point it is automatically resumed
 * 
 * @note	Up to BLUETOOTH_MAX_QUEUED_ADV_PACKETS packets can be queued for transmission at once
 * 
 * @note	The underlying Bluetooth stack may impose limits on the data present in xBluetoothAdvertiseParameters_t::pucData
 * 
 * @param[in] pxAdvertiseParameters			Advertising packet configuration
 *
 * @retval	::ERROR_NONE					Advertising started successfully
 * @retval 	::ERROR_UNAVAILABLE_RESOURCE	No free buffers to queue packet in
 * @retval	::ERROR_INVALID_DATA			Provided parameters were invalid
 * @retval	::ERROR_INVALID_STATE			Driver was in an incorrect state to start advertising
 */
eModuleError_t eBluetoothAdvertise( xBluetoothAdvertiseParameters_t *pxAdvertiseParameters );

/**@brief Retrieve the master connection handler for configuration
 *
 * Discovery behaviour is described in @ref xBluetoothConnection_t
 * 
 * @retval 	Pointer to the master connection handler
 */
xBluetoothConnection_t *pxBluetoothMasterConfiguration( void );

/**@brief Retrieve the slave connection handler for configuration
 *
 * Discovery behaviour is described in @ref xBluetoothConnection_t
 * 
 * @retval 	Pointer to the slave connection handler
 */
xBluetoothConnection_t *pxBluetoothSlaveConfiguration( void );

/**@brief UUID equality comparison
 *
 * @param[in] pxUUID_A					First UUID
 * @param[in] pxUUID_B					Second UUID
 * 
 * @retval 	True if the two UUID's are equal
 */
bool bBluetoothUUIDsEqual( xBluetoothUUID_t *pxUUID_A, xBluetoothUUID_t *pxUUID_B );

/**@brief Initiate a connection to a remote device
 *
 * pxConnection::xRemoteDevice must be a valid remote address
 * 
 * Discovery behaviour is described in @ref xBluetoothConnection_t
 * 
 * @param[in] pxConnection				Connection State
 * 
 * @retval	::ERROR_NONE				Connection process has been initiated
 * @retval	::ERROR_INVALID_STATE		A connection is already present
 */
eModuleError_t eBluetoothConnect( xBluetoothConnection_t *pxConnection );

/**@brief Wait for a connection to be established
 * 
 * @param[in] pxConnection				Connection State
 * @param[in] xTimeout					Duration to wait
 * 
 * @retval	::ERROR_NONE				Connection was successfully established
 * @retval	::ERROR_TIMEOUT				Connection has not yet been established
 */
eModuleError_t eBluetoothConnectWait( xBluetoothConnection_t *pxConnection, TickType_t xTimeout );

/**@brief Disconnect from the currently connected remote device
 *
 * @param[in] pxConnection				Connection State
 * 
 * @retval	::ERROR_NONE				Disconnection process has been initiated
 * @retval	::ERROR_INVALID_STATE		No connection was present
 */
eModuleError_t eBluetoothDisconnect( xBluetoothConnection_t *pxConnection );

/**@brief Retrieve RSSI of the latest connection event on GATT
 *
 * @param[in]  pxConnection				Connection State
 * 
 * @retval  ::INT16_MIN					No connection present
 * @retval	::rssi						Last event RSSI in deci-dBm ( -30 = -3.0dBm )
 */
int16_t sBluetoothRssi( xBluetoothConnection_t *pxConnection );

/**@brief Update a characteristic value on the local device GATT server
 * 
 * Intended for updating device information constants, not transporting dynamic data
 * 
 * @note Notifications are not triggered to connected remote devices
 *
 * @param[in] pxCharacteristic			Characteristic to update
 * 
 * @retval	::ERROR_NONE				Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA		Provided characteristic or data was invalid
 */
eModuleError_t eBluetoothWriteLocalCharacteristic( xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief Distribute an updated characteristic value to connected devices that have subscribed to
 * 
 * @note	xGattLocalCharacteristic_t::eSubscriptionMode must be set to desired output mode
 * @note	Should only be called on a local characteristic that has received a subscription callback
 * 
 * @param[in] pxConnection				Connection to distribute on
 * @param[in] pxCharacteristic			Characteristic to update
 * 
 * @retval	::ERROR_NONE				Characteristic successfully distributed
 * @retval	::ERROR_INVALID_STATE		Connection dropped in the middle of procedure 
 */
eModuleError_t eBluetoothDistributeLocalCharacteristic( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief Update a characteristic value on the connected remote GATT server
 * 
 * @param[in] xBluetoothConnection_t	Connection State
 * @param[in] pxCharacteristic			Characteristic to update
 * @param[in] bAcknowledged				True if write should be acknowledged
 * 
 * @retval	::ERROR_NONE				Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA		Provided characteristic or data was invalid
 * @retval	::ERROR_INVALID_STATE		Connection dropped in the middle of procedure 
 */
eModuleError_t eBluetoothWriteRemoteCharacteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, bool bAcknowledged );

/**@brief Subscribe to changes to a characteristic on the connected remote GATT server
 * 
 * @note	NOTIFY and INDICATE are mutually exclusive modes
 * 
 * @param[in] xBluetoothConnection_t	Connection State
 * @param[in] pxCharacteristic			Characteristic to subscribe to
 * @param[in] eSubscriptionMode			Mode we will subscribe in
 * 
 * @retval	::ERROR_NONE				Successfully subscribed to characteristic
 * @retval	::ERROR_INVALID_DATA		Provided characteristic not available for requested subscription
 * @retval	::ERROR_INVALID_STATE		Connection dropped in the middle of procedure 
 */
eModuleError_t eBluetoothSubscribeRemoteCharacteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, eBleClientCharacteristicConfiguration_t eSubscriptionMode );

/**@brief Read a charicteristic value on a connected remote GATT server
 * 
 * @param[in] pxConnection Connection State
 * @param[in] pxCharacteristic Characteristic to read
 * 
 * @retval	::ERROR_NONE			Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA	Provided characteristic or data was invalid
 */
eModuleError_t eBluetoothReadRemoteCharicteristic( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic );

#endif /* __CSIRO_CORE_BLUETOOTH */
