/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: accelerometer_interface.h
 * Creation_Date: 11/10/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Accelerometer Common Interface
 * 
 */
#ifndef __CSIRO_CORE_ACCELEROMETER_INTERFACE
#define __CSIRO_CORE_ACCELEROMETER_INTERFACE
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eAccelerometerInterrupt_t {
	ACCELEROMETER_NEW_DATA   = 0x01,
	ACCELEROMETER_NO_MOTION  = 0x02,
	ACCELEROMETER_SINGLE_TAP = 0x04,
	ACCELEROMETER_DOUBLE_TAP = 0x08,
	ACCELEROMETER_OTHER		 = 0xFF /**< Type has not yet been determined */
} ATTR_PACKED eAccelerometerInterrupt_t;

typedef struct xAccelerometerConfiguration_t
{
	bool	 bEnabled;		 /**< True when accelerometer should be running */
	bool	 bLowPowerMode;  /**< Run in lowest power mode, will result in less accurate sampling */
	uint8_t  ucFIFOLimit;	/**< Receive a data interrupt every N samples, unavailable in Low Power Modes */
	uint8_t  ucRangeG;		 /**< Maximum G range, must be an element of [2, 4, 8, 16] */
	uint16_t usSampleRateHz; /**< Sampling Rate in Hz */
	struct
	{
		bool	 bEnabled;			/**< True if no activity sensing should be enabled */
		uint16_t usThresholdMilliG; /**< Resets detections timers if acceleration on any axis changes by more than this threshold */
		uint16_t usDurationS;		/**< Time that device needs to be still to trigger the interrupt */
	} xNoActivityConfig;
} xAccelerometerConfiguration_t;

typedef struct xAccelerometerState_t
{
	bool	 bEnabled;		   /**< True when running */
	uint32_t ulRateMilliHz;	/**< Configured sample rate in milliHertz */
	uint32_t ulPeriodUs;	   /**< Expected period of individual samples in microSeconds */
	uint8_t  ucSampleGrouping; /**< Number of samples the accelerometer will generate per data interrupt */
	uint8_t  ucMaxG;		   /**< Configured maximum G range*/
} xAccelerometerState_t;

/**<@brief Accelerometer sample
 * 
 * The physical G reading represented by this struct is constant between bit depths and maximum ranges
 * Maximum ranges constrain the maximum value of the fields, not the physical meaning of an LSB
 * 
 * The highest resolution represented by this struct is a 16bit sample at 2G resolution
 * The resolution of a single bit is therefore (2 / 2^15) = 0.061 milliG
 * A reading of +1G is consequently 2^14 (16384)
 * 
 * Accelerometers with less than 16bit resolution must shift sample such that they appear to have 16 bits of resolution
 * The lowest 2 bits for a 14bit accelerometer will therefore be 0
 * 
 * Higher ranges, such as 4G, 8G, and 16G are represented by shifting the samples by one bit per power of 2
 * A 16G reading therefore takes (16 + 3) bits, regardless of the underlying accelerometer resolution
 **/
typedef struct xAccelerometerSample_t
{
	int32_t  lX;
	int32_t  lY;
	int32_t  lZ;
	uint32_t ulMagnitude;
} xAccelerometerSample_t;

typedef struct xAccelerometerSampleBuffer_t
{
	uint8_t					ucNumSamples;
	xAccelerometerSample_t *pxSamples;
} xAccelerometerSampleBuffer_t;

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_ACCELEROMETER_INTERFACE */
