/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_interface.h
 * Creation_Date: 7/7/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Bluetooth stack agnostic implementation of GATT related funcionality
 * 
 * GATT covers the sevices and characteristics associated with a remote GATT device
 * 
 */

#ifndef __CORE_CSIRO_BLUETOOTH_GATT
#define __CORE_CSIRO_BLUETOOTH_GATT

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "bluetooth_types.h"

/* Module Defines -------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eGattWriteOptions_t {
	GATT_WRITE_CHARACTERISTIC = 0x01,
	GATT_WRITE_DESCRIPTOR	 = 0x02,
	GATT_WRITE_RESPONSE		  = 0x04,
	GATT_WRITE_NO_RESPONSE	= 0x08,
} eGattWriteOptions_t;

/* Function Declarations ------------------------------------*/

/**@brief Register the connection context to use for the next connection initated by this device
 *
 * @param[in] pxConnection				Connection context to register
 */
void vBluetoothGattRegisterInitiatedConnection( xBluetoothConnection_t *pxConnection );

/**@brief Query latest RSSI reading on the conenction
 *
 * @param[in] pxConnection				Connection context to register
 * 
 * @retval  ::INT16_MIN					No connection present
 * @retval	::rssi						Last event RSSI in deci-dBm ( -30 = -3.0dBm )
 */
int16_t sBluetoothGattConnectionRssi( xBluetoothConnection_t *pxConnection );

/**@brief Update a characteristic on the local GATT server
 * 
 * Intended for updating device information constants, not transporting dynamic data
 * 
 * @note Notifications are not triggered to connected remote devices
 *
 * @param[in] pxCharacteristic		Characteristic to update
 * 
 * @retval	::ERROR_NONE			Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA	Provided characteristic or data was invalid
 */
eModuleError_t eBluetoothGattLocalWrite( xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief Distribute an updated characteristic value to connected devices that have subscribed to
 * 
 * @param[in] pxConnection			Connection to distribute on
 * @param[in] pxCharacteristic		Characteristic to update
 * 
 * @retval	::ERROR_NONE			Characteristic successfully 
 */
eModuleError_t eBluetoothGattLocalDistribute( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxCharacteristic );

/**@brief Update a characteristic value on the connected remote GATT server
 * 
 * @param[in] xBluetoothConnection_t	Connection State
 * @param[in] pxCharacteristic			Characteristic to update
 * @param[in] eOptions                  Options configuring remote write (Remote attribute type, response/no response)
 * 
 * @retval	::ERROR_NONE			Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA	Provided characteristic or data was invalid
 */
eModuleError_t eBluetoothGattRemoteWrite( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic, eGattWriteOptions_t eOptions );

/**@brief Read a charicteristic value on a connected remote GATT server
 * 
 * @param[in] pxConnection Connection State
 * @param[in] pxCharacteristic Characteristic to read
 * 
 * @retval	::ERROR_NONE			Characteristic successfully updated
 * @retval	::ERROR_INVALID_DATA	Provided characteristic or data was invalid
 */
eModuleError_t eBluetoothGattRemoteRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxCharacteristic );

#endif /* __CORE_CSIRO_BLUETOOTH_GATT */
