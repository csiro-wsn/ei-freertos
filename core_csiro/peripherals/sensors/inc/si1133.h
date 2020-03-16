/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: si1133.h
 * Creation_Date: 25/10/2019
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * A basic driver for the SI1133 ambient light sensor on the thunderboard sense 2.
 * This was used as a reference:
 * https://os.mbed.com/teams/SiliconLabs/code/Si1133/file/f780ca9105bb/Si1133.cpp/
 * 
 * TODO: Add conversions from the link above.
 * TODO: Add timeouts into driver.
 * TODO: Document what the chain of parameters added in the config function actually do.
 */

#ifndef __CSIRO_CORE_SI1133
#define __CSIRO_CORE_SI1133

/* Includes -------------------------------------------------*/

#include "i2c.h"

/* Module Defines -------------------------------------------*/

#define SI1133_ADDRESS 0x55

// clang-format off

typedef enum eSi1133Register_t {
	REG_PART_ID	   = 0x00, /**< Part ID                                                            */
	REG_HW_ID	   = 0x01, /**< Hardware ID                                                        */
	REG_REV_ID	   = 0x02, /**< Hardware revision                                                  */
	REG_HOSTIN0	   = 0x0A, /**< Data for parameter table on PARAM_SET write to COMMAND register    */
	REG_COMMAND	   = 0x0B, /**< Initiated action in Sensor when specific codes written here        */
	REG_IRQ_ENABLE = 0x0F, /**< Interrupt enable                                                   */
	REG_RESPONSE1  = 0x10, /**< Contains the readback value from a query or a set command          */
	REG_RESPONSE0  = 0x11, /**< Chip state and error status                                        */
	REG_IRQ_STATUS = 0x12, /**< Interrupt status                                                   */
	REG_HOSTOUT0   = 0x13, /**< Captured Sensor Data                                               */
	REG_HOSTOUT1   = 0x14, /**< Captured Sensor Data                                               */
	REG_HOSTOUT2   = 0x15, /**< Captured Sensor Data                                               */
	REG_HOSTOUT3   = 0x16, /**< Captured Sensor Data                                               */
	REG_HOSTOUT4   = 0x17, /**< Captured Sensor Data                                               */
	REG_HOSTOUT5   = 0x18, /**< Captured Sensor Data                                               */
	REG_HOSTOUT6   = 0x19, /**< Captured Sensor Data                                               */
	REG_HOSTOUT7   = 0x1A, /**< Captured Sensor Data                                               */
	REG_HOSTOUT8   = 0x1B, /**< Captured Sensor Data                                               */
	REG_HOSTOUT9   = 0x1C, /**< Captured Sensor Data                                               */
	REG_HOSTOUT10  = 0x1D, /**< Captured Sensor Data                                               */
	REG_HOSTOUT11  = 0x1E, /**< Captured Sensor Data                                               */
	REG_HOSTOUT12  = 0x1F, /**< Captured Sensor Data                                               */
	REG_HOSTOUT13  = 0x20, /**< Captured Sensor Data                                               */
	REG_HOSTOUT14  = 0x21, /**< Captured Sensor Data                                               */
	REG_HOSTOUT15  = 0x22, /**< Captured Sensor Data                                               */
	REG_HOSTOUT16  = 0x23, /**< Captured Sensor Data                                               */
	REG_HOSTOUT17  = 0x24, /**< Captured Sensor Data                                               */
	REG_HOSTOUT18  = 0x25, /**< Captured Sensor Data                                               */
	REG_HOSTOUT19  = 0x26, /**< Captured Sensor Data                                               */
	REG_HOSTOUT20  = 0x27, /**< Captured Sensor Data                                               */
	REG_HOSTOUT21  = 0x28, /**< Captured Sensor Data                                               */
	REG_HOSTOUT22  = 0x29, /**< Captured Sensor Data                                               */
	REG_HOSTOUT23  = 0x2A, /**< Captured Sensor Data                                               */
	REG_HOSTOUT24  = 0x2B, /**< Captured Sensor Data                                               */
	REG_HOSTOUT25  = 0x2C, /**< Captured Sensor Data                                               */
} eSi1133Register_t;

typedef enum eSi1133Parameter_t {
	PARAM_I2C_ADDR	   = 0x00, /**< I2C address                                                        */
	PARAM_CH_LIST	   = 0x01, /**< Channel list                                                       */
	PARAM_ADCCONFIG0   = 0x02, /**< ADC config for Channel 0                                           */
	PARAM_ADCSENS0	   = 0x03, /**< ADC sensitivity setting for Channel 0                              */
	PARAM_ADCPOST0	   = 0x04, /**< ADC resolution, shift and threshold settings for Channel 0         */
	PARAM_MEASCONFIG0  = 0x05, /**< ADC measurement counter selection for Channel 0                    */
	PARAM_ADCCONFIG1   = 0x06, /**< ADC config for Channel 1                                           */
	PARAM_ADCSENS1	   = 0x07, /**< ADC sensitivity setting for Channel 1                              */
	PARAM_ADCPOST1	   = 0x08, /**< ADC resolution, shift and threshold settings for Channel 1         */
	PARAM_MEASCONFIG1  = 0x09, /**< ADC measurement counter selection for Channel 1                    */
	PARAM_ADCCONFIG2   = 0x0A, /**< ADC config for Channel 2                                           */
	PARAM_ADCSENS2	   = 0x0B, /**< ADC sensitivity setting for Channel 2                              */
	PARAM_ADCPOST2	   = 0x0C, /**< ADC resolution, shift and threshold settings for Channel 2         */
	PARAM_MEASCONFIG2  = 0x0D, /**< ADC measurement counter selection for Channel 2                    */
	PARAM_ADCCONFIG3   = 0x0E, /**< ADC config for Channel 3                                           */
	PARAM_ADCSENS3	   = 0x0F, /**< ADC sensitivity setting for Channel 3                              */
	PARAM_ADCPOST3	   = 0x10, /**< ADC resolution, shift and threshold settings for Channel 3         */
	PARAM_MEASCONFIG3  = 0x11, /**< ADC measurement counter selection for Channel 3                    */
	PARAM_ADCCONFIG4   = 0x12, /**< ADC config for Channel 4                                           */
	PARAM_ADCSENS4	   = 0x13, /**< ADC sensitivity setting for Channel 4                              */
	PARAM_ADCPOST4	   = 0x14, /**< ADC resolution, shift and threshold settings for Channel 4         */
	PARAM_MEASCONFIG4  = 0x15, /**< ADC measurement counter selection for Channel 4                    */
	PARAM_ADCCONFIG5   = 0x16, /**< ADC config for Channel 5                                           */
	PARAM_ADCSENS5	   = 0x17, /**< ADC sensitivity setting for Channel 5                              */
	PARAM_ADCPOST5	   = 0x18, /**< ADC resolution, shift and threshold settings for Channel 5         */
	PARAM_MEASCONFIG5  = 0x19, /**< ADC measurement counter selection for Channel 5                    */
	PARAM_MEASRATE_H   = 0x1A, /**< Main measurement rate counter MSB                                  */
	PARAM_MEASRATE_L   = 0x1B, /**< Main measurement rate counter LSB                                  */
	PARAM_MEASCOUNT0   = 0x1C, /**< Measurement rate extension counter 0                               */
	PARAM_MEASCOUNT1   = 0x1D, /**< Measurement rate extension counter 1                               */
	PARAM_MEASCOUNT2   = 0x1E, /**< Measurement rate extension counter 2                               */
	PARAM_THRESHOLD0_H = 0x25, /**< Threshold level 0 MSB                                              */
	PARAM_THRESHOLD0_L = 0x26, /**< Threshold level 0 LSB                                              */
	PARAM_THRESHOLD1_H = 0x27, /**< Threshold level 1 MSB                                              */
	PARAM_THRESHOLD1_L = 0x28, /**< Threshold level 1 LSB                                              */
	PARAM_THRESHOLD2_H = 0x29, /**< Threshold level 2 MSB                                              */
	PARAM_THRESHOLD2_L = 0x2A, /**< Threshold level 2 LSB                                              */
	PARAM_BURST		   = 0x2B, /**< Burst enable and burst count                                       */
} eSi1133Parameter_t;

typedef enum eSi1133Command_t {
	CMD_RESET_CMD_CTR = 0x00, /**< Resets the command counter                                         */
	CMD_RESET		  = 0x01, /**< Forces a Reset                                                     */
	CMD_NEW_ADDR	  = 0x02, /**< Stores the new I2C address                                         */
	CMD_FORCE_CH	  = 0x11, /**< Initiates a set of measurements specified in CHAN_LIST parameter   */
	CMD_PAUSE_CH	  = 0x12, /**< Pauses autonomous measurements                                     */
	CMD_START		  = 0x13, /**< Starts autonomous measurements                                     */
	CMD_PARAM_SET	  = 0x80, /**< Sets a parameter                                                   */
	CMD_PARAM_QUERY   = 0x40, /**< Reads a parameter                                                  */
} eSi1133Command_t;

typedef enum eSi1133Response_t {
	RSP0_CHIPSTAT_MASK = 0xE0, /**< Chip state mask in Response0 register                              */
	RSP0_COUNTER_MASK  = 0x1F, /**< Command counter and error indicator mask in Response0 register     */
	RSP0_SLEEP		   = 0x20, /**< Sleep state indicator bit mask in Response0 register               */
} eSi1133Response_t;

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xSi1133Init_t
{
	xI2CModule_t *pxModule;
} xSi1133Init_t;

typedef struct xSi1133Data_t
{
	int32_t lUltraVioletCh0;
	int32_t lAmbientCh1;
	int32_t lAmbientCh2;
	int32_t lAmbientCh3;
} xSi1133Data_t;

/* Function Declarations ------------------------------------*/

/**
 * Initialises the chip.
 **/
eModuleError_t eSi1133Init( xSi1133Init_t *pxInit );

/**
 * Configures the chip.
 **/
eModuleError_t eSi1133Config( void );

/**
 * Reads the ambient light.
 */
eModuleError_t eSi1133Read( xSi1133Data_t *pxSi1133Data, TickType_t xTimeout );

#endif /* __CSIRO_CORE_SI1133 */
