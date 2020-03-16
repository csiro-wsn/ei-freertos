/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: sts31.h
 * Creation_Date: 13/8/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Driver for the STS-31-DIS Sensiron I2C Temperature Sensor.
 * 
 * Disclaimer: This chip has a couple of different features. This driver
 * doesn't care about repeated measurements, the heater, or any of that stuff,
 * so it has all just been ignored. This driver only gives the ability to use
 * the single shot measurements to measure the temperature.
 */

#ifndef __CORE_CSIRO_SENSORS_STS31
#define __CORE_CSIRO_SENSORS_STS31

/* Includes -------------------------------------------------*/

#include "i2c.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define STS31_STARTUP_TIME_MS 2
#define STS31_SOFT_RESET_TIME_MS 2

#define STS31_ADDRESS 0x4A

/**
 * Accuracy Levels
 * These are 3 commands for the chip which initiate a single temperature
 * measurement at differing accuracies. The time taken for each measurement is
 * shown next to the command. I2C stretching is disabled for these measurements
 * so a read command is sent before the data is ready, it won't be
 * acknowledged.
 */
typedef enum eSts31Accuracy_t {
	STS31_ACCURACY_HIGH,   // Timeout = 15.5ms
	STS31_ACCURACY_MEDIUM, // Timeout = 6.5ms
	STS31_ACCURACY_LOW	 // Timeout = 4.5ms
} eSts31Accuracy_t;

#define STS31_FETCH_DATA 0xE000
#define STS31_SOFT_RESET 0x30A2
#define STS31_READ_STATUS 0xF32D
#define STS31_CLEAR_STATUS 0x3041

#define STS31_HEATER_ENABLED 0x306D
#define STS31_HEATER_DISABLED 0x3066

/**
 * Status Register Mask
 * Tells us what bytes in the status register mean different things.
 */
typedef enum eSts31StatusMask_t {
	STS31_STATUS_ALERT_PENDING		   = ( 1 << 15 ),
	STS31_STATUS_HEATER				   = ( 1 << 13 ),
	STS31_STATUS_T_TRACKING			   = ( 1 << 10 ),
	STS31_STATUS_SYSTEM_RESET_DETECTED = ( 1 << 4 ),
	STS31_STATUS_COMMAND_STATUS		   = ( 1 << 1 ),
	STS31_STATUS_WRITE_CHECKSUM		   = ( 1 << 0 )
} eSts31StatusMask_t;

/* Type Definitions -----------------------------------------*/

typedef struct xSts31Init
{
	xI2CModule_t *pxModule;
} xSts31Init_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the STS31 temperature sensor
 */
eModuleError_t eSts31Init( xSts31Init_t *pxInit );

/**
 * Reads the temperature.
 */
eModuleError_t eSts31ReadRaw( uint16_t *pusRawReading, eSts31Accuracy_t eAccuracy, TickType_t xTimeout );

/**
 * Converts the temperature
 */
static inline int32_t lSts31ConvertRawToMillidegrees( uint16_t usRawData )
{
	/**
	 * 			T[Celcius]  = -45 + 175 * ( RAW / (2^16 - 1))
	 * 						= -45000 + 175000 *  ( RAW / (2^16 - 1))
	 **/

	return ( int32_t )( ( ( (int64_t) usRawData * 175000 ) / UINT16_MAX ) - 45000 );
}

static inline eModuleError_t eSts31ReadMilliDegrees( int32_t *plMilliDegrees, eSts31Accuracy_t eAccuracy, TickType_t xTimeout )
{
	eModuleError_t eError;
	uint16_t	   usRawData;

	eError			= eSts31ReadRaw( &usRawData, eAccuracy, xTimeout );
	*plMilliDegrees = lSts31ConvertRawToMillidegrees( usRawData );

	return eError;
}

#endif /* __CORE_CSIRO_SENSORS_STS31 */
