/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_utility.h
 * Creation_Date: 11/07/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Common helper functions between stacks
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_UTILITY
#define __CSIRO_CORE_BLUETOOTH_UTILITY
/* Includes -------------------------------------------------*/

#include "log.h"

#include "bluetooth_types.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Search for a Service on a Bluetooth connection
 * 
 * @param[in] pxConnection	        Connection State
 * @param[in] pxUUID	            Service UUID to search for
 * 
 * @retval	::xGattService_t*	    The GATT service associated with pxUUID
 * @retval	::NULL	                No matching service was found
 */
xGattService_t *pxBluetoothSearchServiceUUID( xBluetoothConnection_t *pxConnection, xBluetoothUUID_t *pxUUID );

/**@brief Search for a Characteristic on a Bluetooth connection
 * 
 * @param[in] pxConnection	                    Connection State
 * @param[in] pxUUID	                        Characteristic UUID to search for
 * 
 * @retval	::xGattRemoteCharacteristic_t*      The GATT characteristic associated with pxUUID
 * @retval	::NULL	                            No matching characteristic was found
 */
xGattRemoteCharacteristic_t *pxBluetoothSearchCharacteristicUUID( xBluetoothConnection_t *pxConnection, xBluetoothUUID_t *pxUUID );

/**@brief Search for a Characteristic on a Bluetooth connection
 * 
 * @param[in] pxConnection	                    Connection State
 * @param[in] usHandle	                        Characteristic Handle to search for
 * 
 * @retval	::xGattRemoteCharacteristic_t*      The GATT characteristic associated with usHandle
 * @retval	::NULL	                            No matching characteristic was found
 */
xGattRemoteCharacteristic_t *pxBluetoothSearchCharacteristicHandle( xBluetoothConnection_t *pxConnection, uint16_t usHandle );

/**@brief Print the Gatt table associated with the remote device on a connection
 * 
 * @param[in] eLogger	            Logger to write to
 * @param[in] eLevel	            Level to write at
 * @param[in] pxConnection	        Connection State
 */
void vBluetoothPrintConnectionGattTable( SerialLog_t eLogger, LogLevel_t eLevel, xBluetoothConnection_t *pxConnection );

#endif /* __CSIRO_CORE_BLUETOOTH_UTILITY */
