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

#include "rtos_gecko.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

typedef enum eBluetoothPhy_t {
	BLUETOOTH_PHY_1M	= 1,
	BLUETOOTH_PHY_2M	= 2,
	BLUETOOTH_PHY_CODED = 4
} eBluetoothPhy_t;

typedef enum eBluetoothAdvertisingType_t {
	BLUETOOTH_ADV_CONNECTABLE_SCANNABLE		  = le_gap_connectable_scannable,
	BLUETOOTH_ADV_NONCONNECTABLE_SCANNABLE	= le_gap_scannable_non_connectable,
	BLUETOOTH_ADV_NONCONNECTABLE_NONSCANNABLE = le_gap_non_connectable
} eBluetoothAdvertisingType_t;

typedef enum eBluetoothAddressType_t {
	BLUETOOTH_ADDR_TYPE_PUBLIC				   = le_gap_address_type_public,		  /**< Address registered with IEEE, 24bit company_id and 24bit company_assigned */
	BLUETOOTH_ADDR_TYPE_RANDOM_STATIC		   = le_gap_address_type_random,		  /**< Random address, generated on boot or constant for device lifetime */
	BLUETOOTH_ADDR_TYPE_PRIVATE_RESOLVABLE	 = le_gap_address_type_public_identity, /**< Constant address, can only be decoded by devices with corresponding IRK (identity resolving key) */
	BLUETOOTH_ADDR_TYPE_PRIVATE_NON_RESOLVABLE = le_gap_address_type_random_identity, /**< Random number that can change at any time */
	BLUETOOTH_ADDR_TYPE_UNKNOWN				   = 0xFF
} eBluetoothAddressType_t;

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

bool bBluetoothCanDeepSleep( void );

#endif /* __CSIRO_CORE_BLUETOOTH_STACK_DEFINES */
