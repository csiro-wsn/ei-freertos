/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: mlx90614.h
 * Creation_Date: 23/11/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Header file for the MLX90614 IR thermometer.
 * 
 * Forward: This chip sucks and the people who wrote the datasheet deserve to
 * feel bad about it. If I am using seemingly random variable names for
 * variables, it's because that's what the datasheet uses.
 * 
 */

#ifndef __CORE_CSIRO_SENSORS_MLX90614_
#define __CORE_CSIRO_SENSORS_MLX90614_

/* Includes -------------------------------------------------*/

#include "FreeRTOSConfig.h"
#include "i2c.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define MLX90614_ADDRESS 0x5A

/* Type Definitions -----------------------------------------*/

typedef struct xMlx90614Init_t
{
	xI2CModule_t *pxModule;
} xMlx90614Init_t;

/**
 * This struct returns the temperature of the die, and the viewed object in
 * hundredths of a degree celcius.
 **/

typedef struct xMlx90614Data_t
{
	int16_t sAmbientTemperature; // Sensor Die Temperature
	int16_t sObjectTemperature;  // Measured IR Temperature
} xMlx90614Data_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the temperature chip.
 **/
eModuleError_t eMlx90614Init( xMlx90614Init_t *pxInit, TickType_t xTimeout );

/**
 * Reads the I2C address register from the chip. It should be 0x5A.
 * 
 * Note: Requires chip to have been turned on.
 **/
eModuleError_t eMlx90614WhoAmI( uint8_t *pucAddress, TickType_t xTimeout );

/**
 * Turns the temperature sensor on.
 **/
eModuleError_t eMlx90614TurnOn( void );

/**
 * Reads data from the sensor.
 * 
 * Note: Data only refreshes sub <10Hz.
 * Note: Only reads when sensor is on.
 **/
eModuleError_t eMlx90614Read( xMlx90614Data_t *pxData, TickType_t xTimeout );

/**
 * Turns the temperature sensor off.
 **/
eModuleError_t eMlx90614TurnOff( TickType_t xTimeout );

/**
 * Reads a single measurement from the sensor. Does not require the sensor to
 * be turned on.
 **/
eModuleError_t eMlx90614ReadSingle( xMlx90614Data_t *pxData, TickType_t xTimeout );

/*-----------------------------------------------------------*/

#endif /* __CORE_CSIRO_SENSORS_MLX90614_ */
