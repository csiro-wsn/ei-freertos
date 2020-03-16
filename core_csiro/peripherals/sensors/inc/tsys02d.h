/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: tsys02d.h
 * Creation_Date: 20/2/2019
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Driver for the TSYS02D I2C Temperature Sensor.
 */

#ifndef __CORE_CSIRO_SENSORS_TSYS02D
#define __CORE_CSIRO_SENSORS_TSYS02D

/* Includes -------------------------------------------------*/

#include "i2c.h"

/* Module Defines -------------------------------------------*/
// clang-format off


#define TSYS02D_ADDRESS                         0x40

#define TSYS02D_COMMAND_RESET                   0xFE
#define TSYS02D_COMMAND_READ_SERIAL_START       0xFA0F
#define TSYS02D_COMMAND_READ_SERIAL_END         0xFCC9
#define TSYS02D_COMMAND_READ_TEMPERATURE        0xF3

#define TSYS02D_CONVERSION_TIME_MS              100      // Typical conversion time is 43ms. 100 used to ensure success.

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xTsysInit_t
{
	xI2CModule_t *pxModule;
} xTsysInit_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the TSYS02D temperature sensor
 */
eModuleError_t eTsysInit( xTsysInit_t *pxInit );

/**
 * Reads the temperature
 */
eModuleError_t eTsysReadRaw( uint16_t *pusData, TickType_t xTimeout );

/**
 * Converts the temperature
 */
static inline int32_t lTsysConvertRawToMillidegrees( uint16_t usRawData )
{
	/**
	 * 			T[Celcius]  = -46.85 + 175.72 * ( RAW / 2^16)
	 * 						= -46850 + 175720 *  ( RAW / 2^16)
	 **/
	return ( int32_t )( ( ( (int64_t) usRawData * 175720 ) >> 16 ) - 46850 );
}

static inline eModuleError_t eTsysReadMilliDegrees( int32_t *plMilliDegrees, TickType_t xTimeout )
{
	eModuleError_t eError;
	uint16_t	   usRawData;

	eError			= eTsysReadRaw( &usRawData, xTimeout );
	*plMilliDegrees = lTsysConvertRawToMillidegrees( usRawData );

	return eError;
}

#endif /* __CORE_CSIRO_SENSORS_TSYS02D */
