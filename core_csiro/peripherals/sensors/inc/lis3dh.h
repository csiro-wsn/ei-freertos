/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Filename: lis3dh.h
 * Creation_Date: 26/7/2018
 * Author: Rohan Malik <rohan.malik@data61.csiro.au>
 * 
 * The header file defining the interface for the LIS3DH accelerometer.
 */

#ifndef __CORE_CSIRO_SENSORS_LIS3DH
#define __CORE_CSIRO_SENSORS_LIS3DH

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "i2c.h"

#include "lis3dh_device.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define LIS3DH_WHO_AM_I 0x33

#define LIS3DH_DEFAULT_CONFIG                     \
	{                                             \
		.bEnable		   = true,                \
		.eGRange		   = ( RANGE_2G ),        \
		.eSampleRate	   = ( SAMPLE_RATE_1HZ ), \
		.usInterruptEnable = ( INT_NEW_DATA )     \
	}

/* Type Definitions -----------------------------------------*/

typedef enum eLis3dhInterruptPin_t {
	INT1 = 1,
	INT2 = 2
} eLis3dhInterruptPin_t;

typedef enum eLis3dhSampleRate_t {
	SAMPLE_RATE_OFF,
	SAMPLE_RATE_1HZ,
	SAMPLE_RATE_10HZ,
	SAMPLE_RATE_25HZ,
	SAMPLE_RATE_50HZ,
	SAMPLE_RATE_100HZ,
	SAMPLE_RATE_200HZ,
	SAMPLE_RATE_400HZ,
	SAMPLE_RATE_1600HZ,
	SAMPLE_RATE_MAX
} eLis3dhSampleRate_t;

typedef enum eLis3dhGRange_t {
	RANGE_2G,
	RANGE_4G,
	RANGE_8G,
	RANGE_16G
} eLis3dhGRange_t;

typedef enum eLis3dhInterruptType_t {
	INT_NONE		 = 0,
	INT_NEW_DATA	 = 1 << 0,
	INT_LOW_X		 = 1 << 1,
	INT_HIGH_X		 = 1 << 2,
	INT_LOW_Y		 = 1 << 3,
	INT_HIGH_Y		 = 1 << 4,
	INT_LOW_Z		 = 1 << 5,
	INT_HIGH_Z		 = 1 << 6,
	INT_CLICK		 = 1 << 7,
	INT_DOUBLE_CLICK = 1 << 8,
	INT_WATERMARK	= 1 << 9,
	INT_OVERRUN		 = 1 << 10
} eLis3dhInterruptType_t;

/* Struct Declarations --------------------------------------*/
typedef struct xLis3dhInit_t
{
	xI2CModule_t *		  pxI2C;
	eLis3dhInterruptPin_t eLis3dhInterruptPin;
	xGpio_t				  xInterrupt;
} xLis3dhInit_t;

typedef struct xLis3dhConfig_t
{
	bool				bEnable; // Select whether the chip is on or off.
	eLis3dhGRange_t		eGRange;
	eLis3dhSampleRate_t eSampleRate;
	uint16_t			usInterruptEnable; // Select which interrupts to enable from eLis3dhInterruptType_t
} xLis3dhConfig_t;

typedef struct xLis3dhData_t
{
	int16_t sAccX;
	int16_t sAccY;
	int16_t sAccZ;
	int8_t  cTemp;
} xLis3dhData_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialise the LIS3DH accelerometer
 * \param pxInit - The Initialisation structure
 * \param xTimeout - Timeout period
 * \return Error State
 **/
eModuleError_t eLis3dhInit( xLis3dhInit_t *pxInit, TickType_t xTimeout );

/**
 * Returns Chip ID register value. 
 * \param pucWhoAmI - Where to store the register value
 * \param xTimeOut - Timeout period
 * \return Error state
 **/
eModuleError_t eLis3dhWhoAmI( uint8_t *pucWhoAmI, TickType_t xTimeOut );

/**
 * Configure accelerometer device
 * \param xConfig - The configuration
 * \param xTimeout - Timeout period
 * \return Error State
 **/
eModuleError_t eLis3dhConfigure( xLis3dhConfig_t *xConfig, TickType_t xTimeout );

/**
 * Wait for accelerometer interrupt
 * \param xTimeout - Timeout period
 * \return Error State
 **/
eModuleError_t eLis3dhWaitForInterrupt( TickType_t xTimeout );

/**
 * Populates type of most recent interrupt. 
 * \param pxInterruptType - Type of interrupt
 * \param xTimeout - Timeout period
 * \return Error State
 **/
eModuleError_t eLis3dhGetInterruptType( eLis3dhInterruptType_t *pxInterruptType, TickType_t xTimeout );

/**
 * Read x,y,z values and temperature
 * \param xData - Where to store the values
 * \param xTimeout - Timeout period
 * \return Error State
 **/
eModuleError_t eLis3dhRead( xLis3dhData_t *xData, TickType_t xTimeout );

/**
 * Get microsecond period from sample rate
 * \param eRate - The sample rate (enumeration)
 * \return Sample period in microseconds
 **/
uint32_t ulLis3dhGetPeriodUS( eLis3dhSampleRate_t eRate );

/*-----------------------------------------------------------*/

#endif /* __CORE_CSIRO_SENSORS_LIS3DH */
