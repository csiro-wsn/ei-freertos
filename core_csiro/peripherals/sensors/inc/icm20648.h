/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: icm20648.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <jordan.yates@csiro.au>
 *
 * Low Level Driver for ICM20648 6-axis IMU
 * Initialisation sequence based off the Silicon Labs driver which can be found here
 * https://siliconlabs.github.io/Gecko_SDK_Doc/efr32mg12/html/icm20648_8c.html
 * Bold is the programmer whom steps off the sacred initialisation path laid out above
 * Naive is the programmer whom expects reasoning behind the meanderings of said path
 * 
 * Things to try when verification fails:
 * 		1. Increase the delay between setting up gyroscope registers and enabling interrupts
 * 			in eIcm20648Configure()
 * 		2. Add in the prvIcmSensorEnable() call in prvIcmLowPowerMode()
 */

#ifndef __CORE_CSIRO_SENSORS_ICM20648
#define __CORE_CSIRO_SENSORS_ICM20648

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "spi.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define ICM_WHO_AM_I                0xE0

#define ICM_READ                    0x80
#define ICM_WRITE                   0x00

#define ICM_REG_BANK_0              ( 0 << 7 )   
#define ICM_REG_BANK_1              ( 1 << 7 )   
#define ICM_REG_BANK_2              ( 2 << 7 )   
#define ICM_REG_BANK_3              ( 3 << 7 )   

#define ICM_REG_BANK_MASK			( 3 << 7 )  
#define ICM_REGISTER( x )			( x & 0x7F )  

#define ICM_REG_BANK_SEL            ( 0x7F )

#define ICM_REG_WHO_AM_I            ( ICM_REG_BANK_0 | 0x00 )
#define ICM_REG_USR_CTRL            ( ICM_REG_BANK_0 | 0x03 )

#define ICM_REG_LP_CONFIG           ( ICM_REG_BANK_0 | 0x05 )
#define ICM_REG_PWR_MGMT_1          ( ICM_REG_BANK_0 | 0x06 )
#define ICM_REG_PWR_MGMT_2          ( ICM_REG_BANK_0 | 0x07 )

#define ICM_REG_INT_PIN_CFG         ( ICM_REG_BANK_0 | 0x0F )
#define ICM_REG_INT_ENABLE          ( ICM_REG_BANK_0 | 0x10 )
#define ICM_REG_INT_ENABLE_1        ( ICM_REG_BANK_0 | 0x11 )
#define ICM_REG_INT_ENABLE_2        ( ICM_REG_BANK_0 | 0x12 )
#define ICM_REG_INT_ENABLE_3        ( ICM_REG_BANK_0 | 0x13 )

#define ICM_REG_INT_STATUS			( ICM_REG_BANK_0 | 0x19 )
#define ICM_REG_INT_STATUS_1		( ICM_REG_BANK_0 | 0x1A )
#define ICM_REG_INT_STATUS_2		( ICM_REG_BANK_0 | 0x1B )
#define ICM_REG_INT_STATUS_3		( ICM_REG_BANK_0 | 0x1C )

#define ICM_REG_ACC_XOUT_H			( ICM_REG_BANK_0 | 0x2D )
#define ICM_REG_ACC_XOUT_L			( ICM_REG_BANK_0 | 0x2E )
#define ICM_REG_ACC_YOUT_H			( ICM_REG_BANK_0 | 0x2F )
#define ICM_REG_ACC_YOUT_L			( ICM_REG_BANK_0 | 0x30 )
#define ICM_REG_ACC_ZOUT_H			( ICM_REG_BANK_0 | 0x31 )
#define ICM_REG_ACC_ZOUT_L			( ICM_REG_BANK_0 | 0x32 )

#define ICM_REG_GYRO_SMPLRT_DIV     ( ICM_REG_BANK_2 | 0x00 )
#define ICM_REG_GYRO_CONFIG_1       ( ICM_REG_BANK_2 | 0x01 )
#define ICM_REG_GYRO_CONFIG_2       ( ICM_REG_BANK_2 | 0x02 )

#define ICM_REG_ACCEL_SMPLRT_DIV_1  ( ICM_REG_BANK_2 | 0x10 )
#define ICM_REG_ACCEL_SMPLRT_DIV_2  ( ICM_REG_BANK_2 | 0x11 )
#define ICM_REG_ACCEL_INTEL_CTRL    ( ICM_REG_BANK_2 | 0x12 )
#define ICM_REG_ACCEL_WOM_THR       ( ICM_REG_BANK_2 | 0x13 )
#define ICM_REG_ACCEL_CONFIG        ( ICM_REG_BANK_2 | 0x14 )
#define ICM_REG_ACCEL_CONFIG_2      ( ICM_REG_BANK_2 | 0x15 )

// ICM REG BANK SELECT MASKS
#define ICM_USER_BANK_0                 0x00
#define ICM_USER_BANK_1                 0x10
#define ICM_USER_BANK_2                 0x20
#define ICM_USER_BANK_3                 0x30

// ICM USER CONTROL MASKS
#define ICM_USER_CTRL_DMP_EN            0x80
#define ICM_USER_CTRL_FIFO_EN           0x40
#define ICM_USER_CTRL_I2C_MST_EN        0x20
#define ICM_USER_CTRL_I2C_DISABLE       0x10
#define ICM_USER_CTRL_DMP_RST           0x08
#define ICM_USER_CTRL_SRAM_RST          0x04
#define ICM_USER_CTRL_SRAM_MST_RST      0x02

// ICM INTERRUPT CONTROL MASKS
#define ICM_INT_PIN_CFG_ACTIVE_HIGH		0x00
#define ICM_INT_PIN_CFG_ACTIVE_LOW		0x80
#define ICM_INT_PIN_CFG_PUSH_PULL		0x00
#define ICM_INT_PIN_CFG_OPEN_DRAIN		0x40
#define ICM_INT_PIN_CFG_PULSE			0x00
#define ICM_INT_PIN_CFG_LEVEL_HELD		0x20

#define ICM_INT_ENABLE_1_RAW_DATA_RDY	0x01

// ICM POWER MANAGEMENT MASKS
#define ICM_PWR_MGMT_1_DEVICE_RESET     0x80
#define ICM_PWR_MGMT_1_SLEEP            0x40
#define ICM_PWR_MGMT_1_WAKE             0x00
#define ICM_PWR_MGMT_1_LP_EN            0x20
#define ICM_PWR_MGMT_1_TMP_DIS          0x08
#define ICM_PWR_MGMT_1_CLKSEL_STOP      0x07
#define ICM_PWR_MGMT_1_CLKSEL_INT       0x06
#define ICM_PWR_MGMT_1_CLKSEL_BEST      0x01

#define ICM_PWR_MGMT_2_ACCEL_EN         0x00
#define ICM_PWR_MGMT_2_ACCEL_DIS        0x38
#define ICM_PWR_MGMT_2_GYRO_EN          0x00
#define ICM_PWR_MGMT_2_GYRO_DIS         0x07

#define ICM_LP_CONFIG_I2C_MST_CYCLE		0x40
#define ICM_LP_CONFIG_ACCEL_CYCLE		0x20
#define ICM_LP_CONFIG_GYRO_CYCLE		0x10

// ICM ACCELEROMETER CONFIG MASKS
#define ICM_ACCEL_CONFIG_DLPF_DIS		0x00
#define ICM_ACCEL_CONFIG_DLPF_EN		0x01

#define ICM_ACCEL_CONFIG_2G				0x00
#define ICM_ACCEL_CONFIG_4G				0x02
#define ICM_ACCEL_CONFIG_8G				0x04
#define ICM_ACCEL_CONFIG_16G			0x06

#define ICM_ACCEL_CONFIG_LPF_473HZ		0x38 // Might look like the wrong order, it isn't
#define ICM_ACCEL_CONFIG_LPF_246HZ		0x08
#define ICM_ACCEL_CONFIG_LPF_111HZ		0x10
#define ICM_ACCEL_CONFIG_LPF_50HZ		0x18
#define ICM_ACCEL_CONFIG_LPF_24HZ		0x20
#define ICM_ACCEL_CONFIG_LPF_12HZ		0x28
#define ICM_ACCEL_CONFIG_LPF_6HZ		0x30

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum {
	ACCEL_SCALE_2G,
	ACCEL_SCALE_4G,
	ACCEL_SCALE_8G,
	ACCEL_SCALE_16G
} eAccelScale_t;

typedef enum {
	ACCEL_FILTER_NONE,
	ACCEL_FILTER_473Hz,
	ACCEL_FILTER_246Hz,
	ACCEL_FILTER_111Hz,
	ACCEL_FILTER_50Hz,
	ACCEL_FILTER_24Hz,
	ACCEL_FILTER_12Hz,
	ACCEL_FILTER_6Hz
} eAccelFilter_t;

typedef struct xIcmAccelConfiguration
{
	bool		   bEnabled;
	uint16_t	   usSampleRate;
	eAccelScale_t  eFullScale;
	eAccelFilter_t eFilter;
} xIcmAccelConfiguration_t;

typedef enum {
	GYRO_SCALE_250_DPS,
	GYRO_SCALE_500_DPS,
	GYRO_SCALE_1000_DPS,
	GYRO_SCALE_2000_DPS
} eGyroScale_t;

typedef enum {
	GYRO_FILTER_NONE,
	GYRO_FILTER_360Hz,
	GYRO_FILTER_197Hz,
	GYRO_FILTER_152Hz,
	GYRO_FILTER_120Hz,
	GYRO_FILTER_51Hz,
	GYRO_FILTER_24Hz,
	GYRO_FILTER_12Hz,
	GYRO_FILTER_6Hz
} eGyroFilter_t;

typedef struct xIcmGyroConfiguration
{
	bool		  bEnabled;
	uint8_t		  usSampleRate;
	eGyroScale_t  eFullScale;
	eGyroFilter_t eOutputFilter;
} xIcmGyroConfiguration_t;

typedef struct xIcmConfiguration
{
	xIcmAccelConfiguration_t xAccel;
	xIcmGyroConfiguration_t  xGyro;
	bool					 bTemperatureEnabled;
} xIcmConfiguration_t;

typedef struct xIcmAccData
{
	int16_t sAccX;
	int16_t sAccY;
	int16_t sAccZ;
} xIcmAccData_t;

typedef struct xIcmInit
{
	xSpiModule_t *pxSpi;
	xGpio_t		  xCsGpio;
	xGpio_t		  xEnGpio;
	xGpio_t		  xIntGpio;
} xIcmInit_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eIcm20648Init( xIcmInit_t *pxIcmInit );

eModuleError_t eIcm20648Configure( xIcmConfiguration_t *pxConfiguration );

eModuleError_t eIcm20648LowPower( void );

eModuleError_t eIcm20648WhoAmI( uint8_t *pucWhoAmI );

eModuleError_t eIcm20648WaitForInterrupt( TickType_t xTimeout );

eModuleError_t eIcm20648ReadAcc( xIcmAccData_t *pxAccData, TickType_t xTimeout );

#endif /* __CORE_CSIRO_SENSORS_ICM20648 */
