/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: sht31.h
 * Creation_Date: 30/1/2019
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Driver for the SHT-31-DIS Sensiron I2C Temperature and Humidity Sensor.
 * 
 */

#ifndef __CORE_CSIRO_SENSORS_SHT31
#define __CORE_CSIRO_SENSORS_SHT31

/* Includes -------------------------------------------------*/

#include "i2c.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define SHT31_STARTUP_TIME_MS 2
#define SHT31_SOFT_RESET_TIME_MS 3

#define SHT31_ADDRESS 0x44

/**
 * Accuracy Levels
 * These are 3 commands for the chip which initiate measurements at differing
 * accuracies. The time taken for each measurement is shown next to the
 * command. I2C stretching is disabled for these measurements so a read command
 * is sent before the data is ready, it won't be acknowledged.
 */
typedef enum eSht31Accuracy_t {
	SHT31_ACCURACY_HIGH,   // Timeout = 15ms
	SHT31_ACCURACY_MEDIUM, // Timeout = 6ms
	SHT31_ACCURACY_LOW	 // Timeout = 4ms
} eSht31Accuracy_t;

#define SHT31_FETCH_DATA 0xE000
#define SHT31_SOFT_RESET 0x30A2
#define SHT31_READ_STATUS 0xF32D
#define SHT31_CLEAR_STATUS 0x3041

#define SHT31_HEATER_ENABLED 0x306D
#define SHT31_HEATER_DISABLED 0x3066

/* Type Definitions -----------------------------------------*/

typedef struct xSht31Data
{
	uint16_t usTemp;
	uint16_t usHumidity;
} xSht31Data_t;

typedef struct xSht31Init
{
	xI2CModule_t *pxModule;
} xSht31Init_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the SHT31.
 */
eModuleError_t eSht31Init( xSht31Init_t *pxInit );

/**
 * Reads the temperature.
 */
eModuleError_t eSht31Read( xSht31Data_t *pxSht31Data, eSht31Accuracy_t eAccuracy, TickType_t xTimeout );

/**
 * Convert the temperature and humidity into human readable units.
 */
int32_t iSht31TemperatureConversion( uint16_t usTemp );
int32_t iSht31HumidityConversion( uint16_t usHumidity );

#endif /* __CORE_CSIRO_SENSORS_SHT31 */
