/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: amg88xx.h
 * Creation_Date: 23/11/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Header file for the AMG88xx IR sensor. Otherwise known as the Grideye.
 * 
 */

#ifndef __CORE_CSIRO_SENSORS_AMG88XX_
#define __CORE_CSIRO_SENSORS_AMG88XX_

/* Includes -------------------------------------------------*/

#include "FreeRTOSConfig.h"
#include "i2c.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/
// clang-format off

/* I2C Address */
#define AMG88XX_ADDRESS_1	0x68	// Address pin is GND
#define AMG88XX_ADDRESS_2	0x69	// Address pin is VCC

// clang-format on

/*
 * If this is the grideye from the front:
 *      |-----------|
 *      |   -----   |
 *      |   |   |   |
 *      |   -----   |
 *      |           |
 *      |           |
 *      |           |
 *      |           |
 *      |-----------|
 *
 * And we were looking directly at the pixels, they would be ordered like this:
 *
 *      57 58 59 60 61 62 63 64
 *      49 50 51 52 53 54 55 56
 *      41 42 43 44 45 46 47 48
 *      33 34 35 36 37 38 39 40
 *      25 26 27 28 29 30 31 32
 *      17 18 19 20 21 22 23 24
 *      9  10 11 12 13 14 15 16
 *      1  2  3  4  5  6  7  8
 */

typedef enum eAmg88xxFrameRate_t {
	FRAME_RATE_1Hz  = 0x01,
	FRAME_RATE_10Hz = 0x00
} eAmg88xxFrameRate_t;

/*-----------------------------------------------------------*/

typedef struct xAmg88xxInit_t
{
	xI2CModule_t *pxModule;
} xAmg88xxInit_t;

/**
 * This is the struct that holds the data from each pixel.
 * The units are 'quarters of a degree'.
 * EG: -4 = -1 degrees C. 100 = 25 degrees C.
 **/
typedef struct xAmg88xxData_t
{
	int16_t psData[64];
} xAmg88xxData_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the Amg88xx.
 **/
eModuleError_t eAmg88xxInit( xAmg88xxInit_t *pxInit, TickType_t xTimeout );

/**
 * Turns the chip on and configures it so it can be read continuously at a set
 * frequency.
 * 
 * Returns ERROR_NONE if everything works. ERROR_GENERIC if a transmit fails,
 * and ERROR_PARTIAL_COMMAND if a transmit fails.
 **/
eModuleError_t eAmg88xxTurnOn( eAmg88xxFrameRate_t eFrameRate, TickType_t xTimeout );

/**
 * Reads all the temperature data from the grideye and stores it in the supplied
 * pointer.
 * 
 * NOTE: All communication with the GridEye must be sent at LEAST 50ms after
 * powerup of the chip. If you are calling this function directly after
 * initialising the board, delay this call by 50ms.
 **/
eModuleError_t eAmg88xxRead( xAmg88xxData_t *pxAmg88xxData, TickType_t xTimeout );

/**
 * Puts the chip into low power mode.
 **/
eModuleError_t eAmg88xxTurnOff( TickType_t xTimeout );

/**
 * Turns the chip on, reads an array of data from all the pixels, and then
 * turns it off again.
 */
eModuleError_t eAmg88xxReadSingle( xAmg88xxData_t *pxAmg88xxData, TickType_t xTimeout );

/*-----------------------------------------------------------*/

#endif /* __CORE_CSIRO_SENSORS_AMG88XX_ */
