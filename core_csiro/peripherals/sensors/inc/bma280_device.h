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
 * Filename: bma280_device.h
 * Creation_Date: 11/12/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 * 
 * A header file defining a bunch of important registers in the BMA280
 * accelerometer chip. This is a non-exhaustive list, but it should be
 * everything useful.
 */

#ifndef __CORE_CSIRO_SENSORS_BMA280_DEVICE
#define __CORE_CSIRO_SENSORS_BMA280_DEVICE

/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Alright this next section is going to get pretty dry folks.
 * I pretty much go through the chip and map out it's different registers and
 * all the settings that can possibly be inside them.
 */

/* Register Map for the BMA280. */
typedef enum eBma280RegisterMap_t {
	BGW_CHIP_ID   = 0x00,
	ACCD_X_LSB	= 0x02,
	ACCD_X_MSB	= 0x03,
	ACCD_Y_LSB	= 0x04,
	ACCD_Y_MSB	= 0x05,
	ACCD_Z_LSB	= 0x06,
	ACCD_Z_MSB	= 0x07,
	ACCD_TEMP	 = 0x08,
	INT_STATUS_0  = 0x09,
	INT_STATUS_1  = 0x0A,
	INT_STATUS_2  = 0x0B,
	INT_STATUS_3  = 0x0C,
	FIFO_STATUS   = 0x0E,
	PMU_RANGE	 = 0x0F,
	PMU_BW		  = 0x10,
	PMU_LPW		  = 0x11,
	PMU_LOW_POWER = 0x12,
	ACCD_HBW	  = 0x13,
	BGW_SOFTRESET = 0x14,
	INT_EN_0	  = 0x16,
	INT_EN_1	  = 0x17,
	INT_EN_2	  = 0x18,
	INT_MAP_0	 = 0x19,
	INT_MAP_1	 = 0x1A,
	INT_MAP_2	 = 0x1B,
	INT_SRC		  = 0x1E,
	INT_OUT_CTRL  = 0x20,
	INT_RST_LATCH = 0x21,
	INT_0		  = 0x22,
	INT_1		  = 0x23,
	INT_2		  = 0x24,
	INT_3		  = 0x25,
	INT_4		  = 0x26,
	INT_5		  = 0x27,
	INT_6		  = 0x28,
	INT_7		  = 0x29,
	INT_8		  = 0x2A,
	INT_9		  = 0x2B,
	INT_A		  = 0x2C,
	INT_B		  = 0x2D,
	INT_C		  = 0x2E,
	INT_D		  = 0x2F,
	FIFO_CONFIG_0 = 0x30,
	PMU_SELF_TEST = 0x32,
	TRIM_NVM_CTRL = 0x33,
	BGW_SPI3_WDT  = 0x34,
	OFC_CTRL	  = 0x36,
	OFC_SETTING   = 0x37,
	OFC_OFFSET_X  = 0x38,
	OFC_OFFSET_Y  = 0x39,
	OFC_OFFSET_Z  = 0x3A,
	TRIM_GP0	  = 0x3B,
	TRIM_GP1	  = 0x3C,
	FIFO_CONFIG_1 = 0x3E,
	FIFO_DATA	 = 0x3F
} eBma280RegisterMap_t;

/*-----------------------------------------------------------*/

/* Register 0x09 - INT_STATUS_0
 *
 * Each flag is associated with a specific interupt. It is set when the
 * interrupt is triggered.
 * '0' = inactive. '1' = active.
 * Default = 0x00
 */
typedef enum eBma280InterruptStatus0_t {
	FLAT_INT		= ( 1 << 7 ),
	ORIENT_INT		= ( 1 << 6 ),
	S_TAP_INT		= ( 1 << 5 ),
	D_TAP_INT		= ( 1 << 4 ),
	SLO_NOT_MOT_INT = ( 1 << 3 ),
	SLOPE_INT		= ( 1 << 2 ),
	HIGH_INT		= ( 1 << 1 ),
	LOW_INT			= ( 1 << 0 )
} eBma280InterruptStatus0_t;

/*-----------------------------------------------------------*/

/* Register 0x0A - INT_STATUS_1
 *
 * Each flag is associated with a specific interupt. It is set when the
 * interrupt is triggered.
 * '0' = inactive. '1' = active.
 * Default = 0x00
 */
typedef enum eBma280InterruptStatus1_t {
	DATA_INT	  = ( 1 << 7 ),
	FIFO_WM_INT   = ( 1 << 6 ),
	FIFO_FULL_INT = ( 1 << 5 )
} eBma280InterruptStatus1_t;

/*-----------------------------------------------------------*/

/* Register 0x0B - INT_STATUS_2
 *
 * Each flag is associated with a specific interupt. It is set when the
 * interrupt is triggered.
 * '0' = inactive. '1' = active.
 * Default = 0x00
 */
typedef enum eBma280InterruptStatus2_t {
	TAP_SIGN	  = ( 1 << 7 ),
	TAP_FIRST_Z   = ( 1 << 6 ),
	TAP_FIRST_Y   = ( 1 << 5 ),
	TAP_FIRST_X   = ( 1 << 4 ),
	SLOPE_SIGN	= ( 1 << 3 ),
	SLIPE_FIRST_Z = ( 1 << 2 ),
	SLOPE_FIRST_Y = ( 1 << 1 ),
	SLOPE_FIRST_X = ( 1 << 0 )
} eBma280InterruptStatus2_t;

/*-----------------------------------------------------------*/

/* Register 0x0C - INT_STATUS_3
 *
 * Each flag is associated with a specific interupt. It is set when the
 * interrupt is triggered.
 * '0' = inactive. '1' = active.
 * Default = 0x00
 */
typedef enum eBma280InterruptStatus3_t {
	FLAT		  = ( 1 << 7 ),
	ORIENT_Z_AXIS = ( 1 << 6 ),
	ORIENT		  = ( 1 << 5 ) | ( 1 << 4 ),
	HIGH_SIGN	 = ( 1 << 3 ),
	HIGH_FIRST_Z  = ( 1 << 2 ),
	HIGH_FIRST_Y  = ( 1 << 1 ),
	HIGH_FIRST_X  = ( 1 << 0 )
} eBma280InterruptStatus3_t;

/* These are the values for each possible orientation above. */
typedef enum eBma280Orientation_t {
	PORTRAIT_UPRIGHT	 = 0,
	PORTRAIT_UPSIDE_DOWN = 1,
	LANDSCAPE_LEFT		 = 2,
	LANDSCAPE_RIGHT		 = 3
} eBma280Orientation_t;

/*-----------------------------------------------------------*/

/* Register 0x0E - FIFO_STATUS
 *
 * Contains the status of the FIFO buffer.
 * If 'FIFO_OVERRUN' = 1, an overrun has occurred.
 * Default = 0x00
 */
typedef enum eBma280FifoStatus_t {
	FIFO_OVERRUN	  = ( 0x80 ),
	FIFO_FRAMECOUNTER = ( 0x7F )
} eBma280FifoStatus_t;

/*-----------------------------------------------------------*/

/* Register 0x0F - PMU_RANGE
 *
 * Allows the selection of the accelerometer g-range.
 * Default = 0x03 (2G).
 * Note: 0x00 is actually invalid for this register.
 */
typedef enum eBma280Range_t {
	REGISTER_RANGE_2G  = ( 1 << 1 ) | ( 1 << 0 ),
	REGISTER_RANGE_4G  = ( 1 << 2 ) | ( 1 << 0 ),
	REGISTER_RANGE_8G  = ( 1 << 3 ),
	REGISTER_RANGE_16G = ( 1 << 3 ) | ( 1 << 2 )
} eBma280Range_t;

/*-----------------------------------------------------------*/

/** Register 0x10 - PMU_BW 
 * 
 * Bandwidth of the filtered accelerometer data
 * In normal mode, this also drives the Output Data Rate (ODR)
 * 			ODR = 2 * FILTER_BANDWIDTH
 */
typedef enum eBma280FilterBandwidth_t {
	FILTER_BANDWIDTH_7H81		= 0b01000, /**< ODR = 15.63Hz */
	FILTER_BANDWIDTH_15H63		= 0b01001, /**< ODR = 31.25Hz */
	FILTER_BANDWIDTH_31H25		= 0b01010, /**< ODR = 62.5Hz */
	FILTER_BANDWIDTH_62H5		= 0b01011, /**< ODR = 125Hz */
	FILTER_BANDWIDTH_125H		= 0b01100, /**< ODR = 250Hz */
	FILTER_BANDWIDTH_250H		= 0b01101, /**< ODR = 500Hz */
	FILTER_BANDWIDTH_500H		= 0b01110, /**< ODR = 1000Hz */
	FILTER_BANDWIDTH_UNFILTERED = 0b01111  /**< ODR = 2000Hz */
} eBma280FilterBandwidth_t;

/*-----------------------------------------------------------*/

/* Register 0x11 - PMU_LPW
 *
 * Allows the selection of power modes.
 * Note: Powers modes are mutually exclusive. DO NOT set the bits for two
 * modes simultaneously.
 * I have split this single register into two parts.
 * Default = 0x00 (Normal Operation)
 */
typedef enum eBma280PowerMode_t {
	POWER_MODE_NORMAL		= 0,
	POWER_MODE_SUSPEND		= ( 1 << 7 ),
	POWER_MODE_LOW_POWER	= ( 1 << 6 ),
	POWER_MODE_DEEP_SUSPEND = ( 1 << 5 )
} eBma280PowerMode_t;

typedef enum eBma280SleepDuration_t {
	SLEEP_DURATION_0MS5  = 0b00000,
	SLEEP_DURATION_1MS   = 0b01100,
	SLEEP_DURATION_2MS   = 0b01110,
	SLEEP_DURATION_4MS   = 0b10000,
	SLEEP_DURATION_6MS   = 0b10010,
	SLEEP_DURATION_10MS  = 0b10100,
	SLEEP_DURATION_25MS  = 0b10110,
	SLEEP_DURATION_50MS  = 0b11000,
	SLEEP_DURATION_100MS = 0b11010,
	SLEEP_DURATION_500MS = 0b11100,
	SLEEP_DURATION_1S	= 0b11110
} eBma280SleepDuration_t;

/*-----------------------------------------------------------*/

/* Register 0x12 - PMU_LOW_POWER
 *
 * Configuration settings for low power mode.
 * Default = 0x00 (LPM1, and Event Driven Timebase Mode)
 */
typedef enum eBma280LowPowerMode_t {
	LOW_POWER_MODE_1 = ( 0 << 6 ),
	LOW_POWER_MODE_2 = ( 1 << 6 ),
	SLEEPTIMER_MODE  = ( 1 << 5 )
} eBma280LowPowerMode_t;

/*-----------------------------------------------------------*/

/* Register 0x13 - ACCD_HBW
 *
 * Acceleration data aquisition and data output format.
 * Default = 0x00 (Filtering Enabled, and Data Shadowing Enabled)
 */
typedef struct xBma280Filtering
{
	uint8_t ucFiltering;
} xBma280Filtering_t;

#define BMA280_FILTERING_DISABLE ( 1 << 7 )
#define BMA280_FILTERING_ENABLE ( 0 << 7 )
#define BMA280_SHADOWING_DISABLE ( 1 << 6 )
#define BMA280_SHADOWING_ENABLE ( 0 << 6 )

/*-----------------------------------------------------------*/

/* Register 0x14 - BGW_SOFTRESET
 *
 * Register for soft-resetting the chip.
 * Unless this specific register value is written into this register, nothing
 * happens.
 */
typedef enum eBma280SoftReset_t {
	SOFT_RESET_VALUE = 0xB6
} eBma280SoftReset_t;

/*-----------------------------------------------------------*/

/* Register 0x16 - INT_EN_0 */
#define BMA280_INT_EN_0_EN_FLAT 0x80
#define BMA280_INT_EN_0_EN_ORIENT 0x40
#define BMA280_INT_EN_0_EN_S_TAP 0x20
#define BMA280_INT_EN_0_EN_D_TAP 0x10
#define BMA280_INT_EN_0_EN_SLOPE_Z 0x04
#define BMA280_INT_EN_0_EN_SLOPE_Y 0x02
#define BMA280_INT_EN_0_EN_SLOPE_X 0x01

/* Register 0x17 - INT_EN_1 */
#define BMA280_INT_EN_1_EN_FWM 0x40
#define BMA280_INT_EN_1_EN_FFULL 0x20
#define BMA280_INT_EN_1_EN_DATA 0x10
#define BMA280_INT_EN_1_EN_LOW_G 0x08
#define BMA280_INT_EN_1_EN_HIGH_Z 0x04
#define BMA280_INT_EN_1_EN_HIGH_Y 0x02
#define BMA280_INT_EN_1_EN_HIGH_X 0x01

/* Register 0x18 - INT_EN_2 */
#define BMA280_INT_EN_2_SLOW_NO_MOTION_SEL 0x08
#define BMA280_INT_EN_2_EN_SLOW_NO_MOTION_Z 0x04
#define BMA280_INT_EN_2_EN_SLOW_NO_MOTION_Y 0x02
#define BMA280_INT_EN_2_EN_SLOW_NO_MOTION_X 0x01

/*-----------------------------------------------------------*/

/* 
 * Register 0x19 - INT_MAP_0
 * Register 0x1A - INT_MAP_1
 * Register 0x1B - INT_MAP_2
 */

/* Applicable to either INT_MAP_0 (INT1 mapping) or INT_MAP_2 (INT2 mapping) */
#define BMA280_INT_MAP_FLAT 0x80
#define BMA280_INT_MAP_ORIENT 0x40
#define BMA280_INT_MAP_S_TAP 0x20
#define BMA280_INT_MAP_D_TAP 0x10
#define BMA280_INT_MAP_SLOW_NO_MOTION 0x08
#define BMA280_INT_MAP_SLOPE 0x04
#define BMA280_INT_MAP_HIGH_G 0x02
#define BMA280_INT_MAP_LOW_G 0x01

#define BMA280_INT_MAP_1_INT2_DATA 0x80
#define BMA280_INT_MAP_1_INT2_FWM 0x40
#define BMA280_INT_MAP_1_INT2_FFULL 0x10
#define BMA280_INT_MAP_1_INT1_FFULL 0x04
#define BMA280_INT_MAP_1_INT1_FWM 0x02
#define BMA280_INT_MAP_1_INT1_DATA 0x01

/*-----------------------------------------------------------*/

/* Register 0x1E - INT_SRC
 *
 * Controls the data source for interrupts with selectable data source.
 * '0' = filtered. '1' = unfiltered.
 * Pins not specifed are reserved and should be written to '0'.
 * Default = 0x00.
 */
#define BMA280_INT_SRC_DATA ( 1 << 5 )
#define BMA280_INT_SRC_TAP ( 1 << 4 )
#define BMA280_INT_SRC_SLO_NO_MOT ( 1 << 3 )
#define BMA280_INT_SRC_SLOPE ( 1 << 2 )
#define BMA280_INT_SRC_HIGH ( 1 << 1 )
#define BMA280_INT_SRC_LOW ( 1 << 0 )

/*-----------------------------------------------------------*/

/* Register 0x20 - INT_OUT_CTRL
 *
 * Contains electrical behaviour for interrupt pins.
 * Pins not specified are reserved and should be written to '0'.
 * Default = 0x05 (Push-Pull interrupt pins + INT1 and INT2 active high)
 */
#define BMA280_INT2_OUT_CTRL_PUSH_PULL ( 0 << 3 )
#define BMA280_INT2_OUT_CTRL_OPEN_DRAIN ( 1 << 3 )
#define BMA280_INT2_OUT_CTRL_ACTIVE_LOW ( 0 << 2 )
#define BMA280_INT2_OUT_CTRL_ACTIVE_HIGH ( 1 << 2 )
#define BMA280_INT1_OUT_CTRL_PUSH_PULL ( 0 << 1 )
#define BMA280_INT1_OUT_CTRL_OPEN_DRAIN ( 1 << 1 )
#define BMA280_INT1_OUT_CTRL_ACTIVE_LOW ( 0 << 0 )
#define BMA280_INT1_OUT_CTRL_ACTIVE_HIGH ( 1 << 0 )

/*-----------------------------------------------------------*/

/* Register 0x21 - INT_RST_LATCH
 *
 * Contains the interrupt reset bit and mode selection.
 * Pins not specified are reserved and should be written to '0'.
 * Default = 0x00 (non-latched interrupts)
 */
typedef enum eBma280InterruptLatch_t {
	BMA280_LATCH_UNLATCHED = 0b0000,
	BMA280_LATCH_250MS	 = 0b0001,
	BMA280_LATCH_500MS	 = 0b0010,
	BMA280_LATCH_1S		   = 0b0011,
	BMA280_LATCH_2S		   = 0b0100,
	BMA280_LATCH_4S		   = 0b0101,
	BMA280_LATCH_8S		   = 0b0110,
	BMA280_LATCH_250US	 = 0b1001,
	BMA280_LATCH_500US	 = 0b1010,
	BMA280_LATCH_1MS	   = 0b1011,
	BMA280_LATCH_12MS	  = 0b1100,
	BMA280_LATCH_25MS	  = 0b1101,
	BMA280_LATCH_50MS	  = 0b1110,
	BMA280_LATCH_LATCHED   = 0b1111,
} eBma280InterruptLatch_t;

/*-----------------------------------------------------------*/

/* Register 0x34 - MBG_SPI3_WDT
 *
 * Contains settings for the digital interfaces.
 * Reserved pins should be set to '0'.
 * Default = 0x00.
 */
#define BMA280_I2C_WATCHDOG_ENABLE ( 1 << 2 )
#define BMA280_I2C_WATCHDOG_DISABLE ( 0 << 2 )
#define BMA280_I2C_WATCHDOG_PERIOD_1MS ( 1 << 1 )
#define BMA280_I2C_WATCHDOG_PERIOD_50MS ( 0 << 1 )
#define BMA280_SPI_FOUR_WIRE ( 1 << 0 )
#define BMA280_SPI_THREE_WIRE ( 0 << 0 )

/*-----------------------------------------------------------*/

#endif /* __CORE_CSIRO_SENSORS_BMA280_DEVICE */
