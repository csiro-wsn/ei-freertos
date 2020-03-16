/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: nvm.h
 * Creation_Date: 18/08/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Non-Volatile memory for parameters which commonly change over a 
 * devices lifetime, e.g. Reset Count
 * This interface assumes an underlying key-value data storage mechanism
 */
#ifndef __CSIRO_CORE_ARCH_DEVICE_NVM
#define __CSIRO_CORE_ARCH_DEVICE_NVM
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "application.h"
#include "core_types.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define NVM_COUNTER_VARIABLE            UINT32_MAX
#define NVM_BOOLEAN_VARIABLE			0

// clang-format on
/* Type Definitions -----------------------------------------*/

#ifdef NVM_APP_STRUCT
typedef struct xNvmApplicationStruct_t NVM_APP_STRUCT xNvmApplicationStruct_t;
#else
typedef struct xNvmApplicationStruct_t
{
	uint8_t ucNull;
} xNvmApplicationStruct_t;
#endif /* NVM_APP_STRUCT */

typedef enum {
	NVM_KEY = 0x00,
	NVM_RESET_COUNT,
	NVM_GRENADE_COUNT,
	NVM_WATCHDOG_COUNT,
	NVM_APPLICATION_STRUCT,
	NVM_DEVICE_ACTIVATED,
	NVM_BLUETOOTH_TX_POWER_DBM,
	NVM_EXCEPTION_TIMESTAMP,
	NVM_CLIENT_KEY,
	NVM_XTID,
	NVM_DEVICE_END_TIME,
	NVM_SIGFOX_BLOCK,
	NVM_SCHEDULE_CRC,
	NVM_SCHEDULE_0,
	NVM_SCHEDULE_1,
	NVM_SCHEDULE_2,
	NVM_SCHEDULE_3,
	NVM_SCHEDULE_4,
	NVM_SCHEDULE_5,
	NVM_SCHEDULE_6,
	NVM_SCHEDULE_7,
	NVM_SCHEDULE_8,
	NVM_SCHEDULE_9,
	NVM_SCHEDULE_10,
	NVM_SCHEDULE_11,
	NVM_SCHEDULE_12,
	NVM_SCHEDULE_13,
	NVM_SCHEDULE_14,
	NVM_SCHEDULE_MAX = NVM_SCHEDULE_14 // Ensure this is equal to the largest NVM_SCHEDULE
} eNvmKey_t;

extern const uint32_t ulKeyLengthWords[];
extern const uint32_t ulApplicationNvmValidKey;

/* Function Declarations ------------------------------------*/

/**
 * Initialise NVM memory structures
 * \return Error State
 */
eModuleError_t eNvmInit( void );

/**
 * Erase all key-data pairs from NVM
 * \return Error State
 */
eModuleError_t eNvmEraseData( void );

/**
 * Erase a single key-data pair from NVM
 * \param eKey The data key
 * \return Error State
 */
eModuleError_t eNvmEraseKey( eNvmKey_t eKey );

/**
 * Write a data point to the provided key
 * Note that the implementation decides how large each key is
 * \param eKey The data key
 * \param pvData Pointer to the data to store
 * \return Error State
 */
eModuleError_t eNvmWriteData( eNvmKey_t eKey, void *pvData );

/**
 * Increments a given key value by 1
 * The implementation decides if this operation is valid for a given key
 * \param eKey The data key
 * \param pulNewData The new value of NVM[eKey] is stored here
 * \return Error State
 */
eModuleError_t eNvmIncrementData( eNvmKey_t eKey, uint32_t *pulNewData );

/**
 * Reads a stored key value
 * \param eKey The data key
 * \param pvData Location to store the key data in
 * \return Error State
 */
eModuleError_t eNvmReadData( eNvmKey_t eKey, void *pvData );

/**
 * Reads a stored key value, but if it doesn't exist, initialise it to a default value
 * \param eKey The data key
 * \param pvData Location to store the key data in
 * \param pvDefault Pointer to the default data value
 * \return Error State
 */
eModuleError_t eNvmReadDataDefault( eNvmKey_t eKey, void *pvData, void *pvDefault );

/*
 * Read a stored boolean flag
 * Flags default to false if they have never been set
 * 
 * \param eKey The flag key
 * \return Error State
 */
eModuleError_t eNvmReadFlag( eNvmKey_t eKey, bool *pbState );

/*
 * Read a stored boolean flag
 * Flags default to false if they have never been set
 * 
 * \param eKey The flag key
 * \param bSet True if flag should be enabled
 * \return Error State
 */
eModuleError_t eNvmWriteFlag( eNvmKey_t eKey, bool bSet );

#endif /* __CSIRO_CORE_ARCH_DEVICE_NVM */
