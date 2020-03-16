/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_stack_defines.h
 * Creation_Date: 27/03/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Mappings from general bluetooth constants to stack specific constants
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_STACK_DEFINES
#define __CSIRO_CORE_BLUETOOTH_STACK_DEFINES
/* Includes -------------------------------------------------*/

#include "ble_gap.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define CSIRO_CONNECTION_TAG 0x1

typedef enum eBluetoothPhy_t {
	BLUETOOTH_PHY_1M	= BLE_GAP_PHY_1MBPS,
	BLUETOOTH_PHY_2M	= BLE_GAP_PHY_2MBPS,
	BLUETOOTH_PHY_CODED = BLE_GAP_PHY_CODED
} eBluetoothPhy_t;

typedef enum eBluetoothAdvertisingType_t {
	BLUETOOTH_ADV_CONNECTABLE_SCANNABLE		  = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED,
	BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE	= BLE_GAP_ADV_TYPE_NONCONNECTABLE_SCANNABLE_UNDIRECTED,
	BLUETOOTH_ADV_NONCONNECTABLE_NONSCANNABLE = BLE_GAP_ADV_TYPE_NONCONNECTABLE_NONSCANNABLE_UNDIRECTED
} eBluetoothAdvertisingType_t;

typedef enum eBluetoothAddressType_t {
	BLUETOOTH_ADDR_TYPE_PUBLIC				   = BLE_GAP_ADDR_TYPE_PUBLIC,						  /**< Address registered with IEEE, 24bit company_id and 24bit company_assigned  */
	BLUETOOTH_ADDR_TYPE_RANDOM_STATIC		   = BLE_GAP_ADDR_TYPE_RANDOM_STATIC,				  /**< Random address, generated on boot or constant for device lifetime */
	BLUETOOTH_ADDR_TYPE_PRIVATE_RESOLVABLE	 = BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_RESOLVABLE,	 /**< Constant address, can only be decoded by devices with corresponding IRK (identity resolving key) */
	BLUETOOTH_ADDR_TYPE_PRIVATE_NON_RESOLVABLE = BLE_GAP_ADDR_TYPE_RANDOM_PRIVATE_NON_RESOLVABLE, /**< Random number that can change at any time */
	BLUETOOTH_ADDR_TYPE_UNKNOWN				   = 0xFF
} eBluetoothAddressType_t;

/* Type Definitions -----------------------------------------*/

/**@brief Defined in bluetooth_types.h */
struct xBluetoothUUID_t;

/* Function Declarations ------------------------------------*/

/**@brief Resolve a stack relative UUID to the 128bit UUID value
 *
 * @param[out] pxUUID			UUID to resolve
 */
void vBluetoothStackUUIDResolve( struct xBluetoothUUID_t *pxUUID );

#endif /* __CSIRO_CORE_BLUETOOTH_STACK_DEFINES */
