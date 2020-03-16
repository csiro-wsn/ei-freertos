/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: device_constants.h
 * Creation_Date: 17/08/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Non-Volatile storage for parameters which are constant across device lifetime
 * Currently there is no write interface as these parameters shouldn't change after programming
 */

#ifndef __CSIRO_CORE_ARCH_DEVICE_CONSTANTS
#define __CSIRO_CORE_ARCH_DEVICE_CONSTANTS

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"
#include "core_types.h"

#if __has_include( "device_constants_platform.h" )
#include "device_constants_platform.h"
#else
#include "device_constants_default.h"
#endif

/* Module Defines -------------------------------------------*/
// clang-format off

#define DEVICE_CONSTANTS_KEY    0x1337BEEF

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Read device constants
 *
 * @param[out] pxDeviceConstants	Struct to read constants into
 * 
 * @retval	::True		Constants are valid
 * @retval	::False	    Constants are invalid
 */
bool bDeviceConstantsRead( xDeviceConstants_t *pxDeviceConstants );

/**@brief One-Time Write to Device Constants 
 * 
 * Perform a One-Time-Programmable write to the device constants
 * This function will only perform the write if all of the memory
 * to be overwritten is clean (0xFF)
 * 
 * @note    Successful writes result in a device reboot
 * 
 * @param[in] ucOffset				Byte offset into the device constants to write
 * @param[in] pucData				Data to write
 * @param[in] ucDataLength			Length of data to write
 * 
 * @retval	::ERROR_NONE			Write succeeded, will never return due to reboot
 * @retval	::ERROR_INVALID_ADDRESS	Data already exists within the write range 
 * @retval	::ERROR_GENERIC	        Internal flash write failure
 */
eModuleError_t eDeviceConstantsOneTimeProgram( uint8_t ucOffset, uint8_t *pucData, uint8_t ucDataLength );

#endif /* __CSIRO_CORE_ARCH_DEVICE_CONSTANTS */
