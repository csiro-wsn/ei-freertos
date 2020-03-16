/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_stack.h
 * Creation_Date: 06/06/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Core Bluetooth Stack Operations
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_STACK
#define __CSIRO_CORE_BLUETOOTH_STACK
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "bluetooth_types.h"
#include "core_types.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Enable the bluetooth stack
 * 
 * @note    This may increase current consumption on some platforms
 *
 * @retval ::ERROR_NONE 				Stack successfully enabled
 * @retval ::ERROR_INVALID_STATE 		Stack was already enabled
 */
eModuleError_t eBluetoothStackOn( void );

/**@brief Disable the bluetooth stack
 * 
 * @note    This may decrease current consumption on some platforms
 *
 * @retval ::ERROR_NONE 				Stack successfully disabled
 * @retval ::ERROR_INVALID_STATE 		Stack was already disabled
 */
eModuleError_t eBluetoothStackOff( void );

/**@brief Convert a requested transmit power to a transmit power supported by the stack
 * 
 * This function rounds the requested power to the nearest supported transmit power
 * 
 * @param[in] cTxPowerDbm		Requested TX power in dBm ( decibel-milliwatts )
 * 
 * @retval                      Valid TX power in dBm
 */
int8_t cBluetoothStackGetValidTxPower( int8_t cRequestedPowerDbm );

#endif /* __CSIRO_CORE_BLUETOOTH_STACK */
