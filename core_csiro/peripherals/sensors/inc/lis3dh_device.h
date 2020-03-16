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
 * Filename: lis3dh_device.h
 * Creation_Date: 23/04/2019
 * Author: Rohan Malik <rohan.malik@data61.csiro.au>
 * 
 * A header file defining a bunch of important registers in the LIS3DH
 * accelerometer chip. This is a non-exhaustive list, but it should be
 * everything useful.
 */

#ifndef __CORE_CSIRO_SENSORS_LIS3DH_DEVICE
#define __CORE_CSIRO_SENSORS_LIS3DH_DEVICE

/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/
// clang-format off

/*
 * Used in an I2C command to automatically increment the 
 * register, used for writing or reading multiple registers
 */
#define LIS3DH_AUTO_INCREMENT 	0x80

/* Must be used when writing to the control 0 register */
#define CTRL_REG0_REQUIRED 		0x10

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Register Map for the LIS3DH. */
typedef enum eLis3dhRegisterMap_t {

	STATUS_REG_AUX = 0x07,
	OUT_ADC1_L	 = 0x08,
	OUT_ADC1_H	 = 0x09,
	OUT_ADC2_L	 = 0x0A,
	OUT_ADC2_H	 = 0x0B,
	OUT_ADC3_L	 = 0x0C,
	OUT_ADC3_H	 = 0x0D,
	WHO_AM_I	   = 0x0F,
	CTRL_REG0	  = 0x1E,
	TEMP_CFG_REG   = 0x1F,
	CTRL_REG1	  = 0x20,
	CTRL_REG2	  = 0x21,
	CTRL_REG3	  = 0x22,
	CTRL_REG4	  = 0x23,
	CTRL_REG5	  = 0x24,
	CTRL_REG6	  = 0x25,
	REFERENCE	  = 0x26,
	STATUS_REG	 = 0x27,
	OUT_X_L		   = 0x28,
	OUT_X_H		   = 0x29,
	OUT_Y_L		   = 0x2A,
	OUT_Y_H		   = 0x2B,
	OUT_Z_L		   = 0x2C,
	OUT_Z_H		   = 0x2D,
	FIFO_CTRL_REG  = 0x2E,
	FIFO_SRC_REG   = 0x2F,
	INT1_CFG	   = 0x30,
	INT1_SRC	   = 0x31,
	INT1_THS	   = 0x32,
	INT1_DURATION  = 0x33,
	INT2_CFG	   = 0x34,
	INT2_SRC	   = 0x35,
	INT2_THS	   = 0x36,
	INT2_DURATION  = 0x37,
	CLICK_CFG	  = 0x38,
	CLICK_SRC	  = 0x39,
	CLICK_THS	  = 0x3A,
	TIME_LIMIT	 = 0x3B,
	TIME_LATENCY   = 0x3C,
	TIME_WINDOW	= 0x3D,
	ACT_THS		   = 0x3E,
	ACT_DUR		   = 0x3F
} eLis3dhRegisterMap_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x07 - STATUS_REG_AUX
 * Check overruns and new data
 **/
typedef enum eLis3dhAuxStatus_t {
	AUX_321OR = ( 1 << 7 ),
	AUX_3OR   = ( 1 << 6 ),
	AUX_2OR   = ( 1 << 5 ),
	AUX_1OR   = ( 1 << 4 ),
	AUX_321DA = ( 1 << 3 ),
	AUX_3DA   = ( 1 << 2 ),
	AUX_2DA   = ( 1 << 1 ),
	AUX_1DA   = ( 1 << 0 )
} eLis3dhAuxStatus_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x1E - CTRL_REG0
 * SDO Pullup
 * '0' = inactive. '1' = active.
 * Default = 0x00
 **/
typedef enum eLis3dhCtrlReg0_t {
	SDO_PU_DISC = ( 1 << 7 ),
} eLis3dhCtrlReg0_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x1F - TEMP_CFG_REG
 * Temperature and ADC enable
 **/
typedef enum eLis3dhCfgTemp_t {
	ADC_EN  = ( 1 << 7 ),
	TEMP_EN = ( 1 << 6 ),
} eLis3dhCfgTemp_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x20 - CTRL_REG1
 * Data rate, low power mode and xyz axis enable
 **/
typedef enum eLis3dhCtrlEnable_t {
	CTRL_LOW_POWER_MODE_ENABLE = ( 1 << 3 ),
	CTRL_Z_ENABLE			   = ( 1 << 2 ),
	CTRL_Y_ENABLE			   = ( 1 << 1 ),
	CTRL_X_ENABLE			   = ( 1 << 0 )
} eLis3dhCtrlEnable_t;

typedef enum eLis3dhSamplingRate_t {
	SAMPLING_RATE_OFF, //Power down
	SAMPLING_RATE_1HZ,
	SAMPLING_RATE_10HZ,
	SAMPLING_RATE_25HZ,
	SAMPLING_RATE_50HZ,
	SAMPLING_RATE_100HZ,
	SAMPLING_RATE_200HZ,
	SAMPLING_RATE_400HZ,
	SAMPLING_RATE_1600HZ, //Low power mode
	SAMPLING_RATE_MAX	 //1.344kHz in normal mode, 5Khz in low power mode
} eLis3dhSamplingRate_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x21 - CTRL_REG2
 * High pass filter
 **/
typedef enum eLis3dhCtrlFilter_t {
	CTRL_FILTER_HPCF2   = ( 1 << 5 ),
	CTRL_FILTER_HPCF1   = ( 1 << 4 ),
	CTRL_FILTER_FDS		= ( 1 << 3 ),
	CTRL_FILTER_HPCLICK = ( 1 << 2 ),
	CTRL_FILTER_HP_IA2  = ( 1 << 1 ),
	CTRL_FILTER_HP_IA1  = ( 1 << 0 )
} eLis3dhCtrlFilter_t;

typedef enum eLis3dhFilterMode_t {
	FILTER_REF_MODE			  = ( 1 << 6 ),
	FILTER_NORMAL_MODE		  = ( 1 << 7 ),
	FILTER_AUTORESET_INT_MODE = ( 1 << 7 ) | ( 1 << 6 )
} eLis3dhFilterMode_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x22 - CTRL_REG3
 * Interrupt 1 enable
 **/
typedef enum eLis3dhCtrlInt1_t {
	CTRL_INT1_CLICK   = ( 1 << 7 ),
	CTRL_INT1_IA1	 = ( 1 << 6 ),
	CTRL_INT1_IA2	 = ( 1 << 5 ),
	CTRL_INT1_ZYXDA   = ( 1 << 4 ),
	CTRL_INT1_321DA   = ( 1 << 3 ),
	CTRL_INT1_WTM	 = ( 1 << 2 ),
	CTRL_INT1_OVERRUN = ( 1 << 1 ),
} eLis3dhCtrlInt1_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x23 - CTRL_REG4
 * Data settings
 **/
typedef enum eLis3dhCtrlData_t {
	CTRL_DATA_BDU = ( 1 << 7 ),
	CTRL_DATA_BLE = ( 1 << 6 ),
	CTRL_DATA_HR  = ( 1 << 3 ),
	CTRL_DATA_SIM = ( 1 << 0 )
} eLis3dhCtrlData_t;

/* Full scale range */
typedef enum eLis3dhFullScaleRange_t {
	FS_RANGE_2G  = 0x00,
	FS_RANGE_4G  = ( 1 << 4 ),
	FS_RANGE_8G  = ( 1 << 5 ),
	FS_RANGE_16G = ( 1 << 4 ) | ( 1 << 5 )
} eLis3dhFullScaleRange_t;

/* Self test modes */
typedef enum eLis3dhSelfTestMode_t {
	ST_MODE_NORMAL = 0x00,
	ST_MODE_0	  = ( 1 << 1 ),
	ST_MODE_1	  = ( 1 << 2 )
} eLis3dhSelfTestMode_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x23 - CTRL_REG5
 * Other settings
 **/
typedef enum eLis3dhCtrlReg5_t {
	CTRL_BOOT	 = ( 1 << 7 ),
	CTRL_FIFO_EN  = ( 1 << 6 ),
	CTRL_LIR_INT1 = ( 1 << 3 ),
	CTRL_D4D_INT1 = ( 1 << 2 ),
	CTRL_LIR_INT2 = ( 1 << 1 ),
	CTRL_D4D_INT2 = ( 1 << 0 )
} eLis3dhCtrlReg5_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x25 - CTRL_REG6
 * Interrupt 2 and interrupt polarity
 **/
typedef enum eLis3dhCtrlInt2_t {
	CTRL_INT2_CLICK = ( 1 << 7 ),
	CTRL_INT2_IA1   = ( 1 << 6 ),
	CTRL_INT2_IA2   = ( 1 << 5 ),
	CTRL_INT2_BOOT  = ( 1 << 4 ),
	CTRL_INT2_ACT   = ( 1 << 3 ),
	CTRL_INT_POL	= ( 1 << 1 )
} eLis3dhCtrlInt2_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x27 - STATUS_REG
 * Check overruns and new data for x y and z axes
 **/
typedef enum eLis3dhStatus_t {
	STATUS_ZYXOR = ( 1 << 7 ),
	STATUS_ZOR   = ( 1 << 6 ),
	STATUS_YOR   = ( 1 << 5 ),
	STATUS_XOR   = ( 1 << 4 ),
	STATUS_ZYXDA = ( 1 << 3 ),
	STATUS_ZDA   = ( 1 << 2 ),
	STATUS_YDA   = ( 1 << 1 ),
	STATUS_XDA   = ( 1 << 0 )
} eLis3dhStatus_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x2E - FIFO_CTRL_REG
 * FIFO control settings.
 **/
typedef enum eLis3dhControlFifo_t {
	FIFO_TR = ( 1 << 5 )
} eLis3dhControlFifo_t;

typedef enum eLis3dhFifoMode_t {
	FIFO_MODE_BYPASS		 = 0x00,
	FIFO_MODE_FIFO			 = ( 1 << 6 ),
	FIFO_MODE_STREAM		 = ( 1 << 7 ),
	FIFO_MODE_STREAM_TO_FIFO = ( 1 << 7 ) | ( 1 << 6 )
} eLis3dhFifoMode_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x2F - FIFO_SRC_REG
 * Fifo status
 **/
typedef enum eLis3dhFifoSource_t {
	FIFO_OVERRUN = ( 1 << 6 ),
	FIFO_WTM	 = ( 1 << 7 ),
	FIFO_EMPTY   = ( 1 << 5 )
} eLis3dhFifoSource_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x30, 0x34 - INT1_CFG, INT2_CFG
 * Interrupt configuration
 **/
typedef enum eLis3dhInterruptConfig_t {
	INT_CFG_AOI  = ( 1 << 7 ),
	INT_CFG_6D   = ( 1 << 6 ),
	INT_CFG_ZHIE = ( 1 << 5 ),
	INT_CFG_ZLIE = ( 1 << 4 ),
	INT_CFG_YHIE = ( 1 << 3 ),
	INT_CFG_YLIE = ( 1 << 2 ),
	INT_CFG_XHIE = ( 1 << 1 ),
	INT_CFG_XLIE = ( 1 << 0 )
} eLis3dhInterruptConfig_t;

typedef enum eLis3dhInterruptMode_t {
	INT_MODE_OR		 = 0x00,
	INT_MODE_MOVE_6D = ( 1 << 6 ),
	INT_MODE_AND	 = ( 1 << 7 ),
	INT_MODE_POS_6D  = ( 1 << 7 ) | ( 1 << 6 )
} eLis3dhInterruptMode_t;

/*-----------------------------------------------------------*/

/** 
 * Register 0x31, 0x35 - Interrupt source
 *
 * Each flag is associated with a specific interupt. It is set when the
 * interrupt is triggered.
 * '0' = inactive. '1' = active.
 * Default = 0x00
 **/
typedef enum eLis3dhInterruptSource_t {
	INT_SRC_IA = ( 1 << 6 ),
	INT_SRC_ZH = ( 1 << 5 ),
	INT_SRC_ZL = ( 1 << 4 ),
	INT_SRC_YH = ( 1 << 3 ),
	INT_SRC_YL = ( 1 << 2 ),
	INT_SRC_XH = ( 1 << 1 ),
	INT_SRC_XL = ( 1 << 0 )
} eLis3dhInterruptSource_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x38 - CLICK_CFG
 * Interrupt click configuration
 **/
typedef enum eLis3dhInterruptClickConfig_t {
	CLICK_CFG_ZD = ( 1 << 5 ),
	CLICK_CFG_ZS = ( 1 << 4 ),
	CLICK_CFG_YD = ( 1 << 3 ),
	CLICK_CFG_YS = ( 1 << 2 ),
	CLICK_CFG_XD = ( 1 << 1 ),
	CLICK_CFG_XS = ( 1 << 0 )
} eLis3dhInterruptClickConfig_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x39 - CLICK_SRC
 * Interrupt click source
 **/
typedef enum eLis3dhInterruptClickSource_t {
	CLICK_SRC_IA	 = ( 1 << 6 ),
	CLICK_SRC_DCLICK = ( 1 << 5 ),
	CLICK_SRC_SCLICK = ( 1 << 4 ),
	CLICK_SRC_SIGN   = ( 1 << 3 ),
	CLICK_SRC_Z		 = ( 1 << 2 ),
	CLICK_SRC_Y		 = ( 1 << 1 ),
	CLICK_SRC_X		 = ( 1 << 0 )
} eLis3dhInterruptClickSource_t;

/*-----------------------------------------------------------*/

/**
 * Register 0x3A - CLICK_THS
 * Interrupt click threshold
 **/
typedef enum eLis3dhInterruptClickThreshold_t {
	LIR_CLICK = ( 1 << 7 )
} eLis3dhInterruptClickThreshold_t;

#endif /* __CORE_CSIRO_SENSORS_LIS3DH_DEVICE */
