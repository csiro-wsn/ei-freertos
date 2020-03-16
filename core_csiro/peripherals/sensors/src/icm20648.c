/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "icm20648.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "gpio.h"
#include "log.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

static void prvSetRegisterBank( uint16_t ucRegister );

static void prvWriteRegister( uint16_t ucRegister, uint8_t ucValue );
static void prvReadRegisterBurst( uint16_t ucRegister, uint8_t *pucData, uint8_t ucNumData );

static void prvIcmSensorEnable( bool bAccEn, bool bGyroEn, bool bTempEn );
static void prvIcmLowPowerMode( bool bAccEn, bool bGyroEn, bool bTempEn );
static void prvIcmInteruptEnable( bool bAccEn, bool bGyroEn );

static void prvAccelConfig( xIcmAccelConfiguration_t *pxAccConfig );
static void prvGyroConfig( xIcmGyroConfiguration_t *pxGyroConfig );

void prvIcmInterrupt( void );

/* Private Variables ----------------------------------------*/

static xSpiModule_t *pxModule;

static xSpiConfig_t xIcmBusConfig = {
	.ulMaxBitrate = 6000000,
	.ucDummyTx	= 0xFF,
	.ucMsbFirst   = true,
	.xCsGpio	  = { 0 },
	.eClockMode   = eSpiClockMode0
};

static xGpio_t xEnableGpio;
static xGpio_t xInterruptGpio;

static SemaphoreHandle_t xIcmInterruptSemaphore = NULL;
static StaticSemaphore_t xIcmInterruptSemaphoreStorage;

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648Init( xIcmInit_t *pxIcmInit )
{
	uint8_t ucWhoAmI;
	pxModule = pxIcmInit->pxSpi;

	// Store Configuration
	xIcmBusConfig.xCsGpio = pxIcmInit->xCsGpio;
	xEnableGpio			  = pxIcmInit->xEnGpio;
	xInterruptGpio		  = pxIcmInit->xIntGpio;

	xIcmInterruptSemaphore = xSemaphoreCreateBinaryStatic( &xIcmInterruptSemaphoreStorage );

	/* Set interrupt pin to be disabled */
	vGpioSetup( xInterruptGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	/* Claim an interrupt line, no need to setup on each enable / disable of the sensor */
	if ( eGpioConfigureInterrupt( xInterruptGpio, true, GPIO_INTERRUPT_RISING_EDGE, prvIcmInterrupt ) != ERROR_NONE ) {
		return ERROR_UNAVAILABLE_RESOURCE;
	}
	/* Configure the enable port and turn sensor on */
	vGpioSetup( xEnableGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );

	/* Wait for sensor to power up */
	vTaskDelay( pdMS_TO_TICKS( 100 ) );

	/* Take control of the SPI Bus*/
	if ( eSpiBusStart( pxModule, &xIcmBusConfig, portMAX_DELAY ) != ERROR_NONE ) {
		return ERROR_TIMEOUT;
	}

	/* Validate that we are talking to the correct sensor */
	prvReadRegisterBurst( ICM_REG_WHO_AM_I, &ucWhoAmI, 1 );
	if ( ucWhoAmI != ICM_WHO_AM_I ) {
		eLog( LOG_IMU_DRIVER, LOG_ERROR, "ICM unexpected WHO_AM_I 0x%02X\r\n", ucWhoAmI );
		vSpiBusEnd( pxModule );
		return ERROR_INVALID_DATA;
	}

	/* Reset all register values to their default settings, wait for reset to complete */
	prvWriteRegister( ICM_REG_PWR_MGMT_1, ICM_PWR_MGMT_1_DEVICE_RESET );
	vTaskDelay( pdMS_TO_TICKS( 100 ) );

	/* Disable I2C Mode */
	prvWriteRegister( ICM_REG_USR_CTRL, ICM_USER_CTRL_I2C_DISABLE );

	/* Enable clock source, wait for it to come online */
	prvWriteRegister( ICM_REG_PWR_MGMT_1, ICM_PWR_MGMT_1_CLKSEL_BEST );
	vTaskDelay( pdMS_TO_TICKS( 30 ) );

	/* Setup Interrupt Pin */
	prvWriteRegister( ICM_REG_INT_PIN_CFG, ICM_INT_PIN_CFG_ACTIVE_LOW | ICM_INT_PIN_CFG_OPEN_DRAIN | ICM_INT_PIN_CFG_PULSE );

	/* Release control of the SPI Bus */
	vSpiBusEnd( pxModule );

	/* Configure sensor for low power */
	eIcm20648LowPower();

	/* Notification that initialisation has succeeded */
	eLog( LOG_IMU_DRIVER, LOG_INFO, "ICM Initialisation Complete\r\n" );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static void prvReadRegisterBurst( uint16_t ucRegister, uint8_t *pucData, uint8_t ucNumData )
{
	/* Note that the bank select and read must be seperate transactions (CS returning high between) */
	/* Not noted in the datasheet, but experimentally verified */
	uint8_t ucCommand = ICM_READ | ICM_REGISTER( ucRegister );
	/* Set register bank */
	prvSetRegisterBank( ucRegister );
	/* Read the register values */
	vSpiCsAssert( pxModule );
	vSpiTransmit( pxModule, &ucCommand, 1 );
	vSpiReceive( pxModule, pucData, ucNumData );
	vSpiCsRelease( pxModule );
}

/*-----------------------------------------------------------*/

static void prvWriteRegister( uint16_t ucRegister, uint8_t ucValue )
{
	/* Note that the bank select and read must be seperate transactions (CS returning high between) */
	/* Not noted in the datasheet, but experimentally verified */
	uint8_t ucCommand[2] = { ICM_WRITE | ICM_REGISTER( ucRegister ), ucValue };
	/* Set register bank */
	prvSetRegisterBank( ucRegister );
	/* Write to register */
	vSpiCsAssert( pxModule );
	vSpiTransmit( pxModule, ucCommand, 2 );
	vSpiCsRelease( pxModule );
}

/*-----------------------------------------------------------*/

static void prvSetRegisterBank( uint16_t ucRegister )
{
	uint8_t ucBank		 = ( ucRegister & ICM_REG_BANK_MASK ) >> 3;
	uint8_t ucBankSel[2] = { ICM_WRITE | ICM_REG_BANK_SEL, ucBank };
	/* Set the device to the correct register bank */
	vSpiCsAssert( pxModule );
	vSpiTransmit( pxModule, ucBankSel, 2 );
	vSpiCsRelease( pxModule );
}

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648WhoAmI( uint8_t *pucWhoAmI )
{
	/* Take control of the SPI Bus*/
	if ( eSpiBusStart( pxModule, &xIcmBusConfig, portMAX_DELAY ) != ERROR_NONE ) {
		return ERROR_TIMEOUT;
	}
	/* Read Register Value */
	prvReadRegisterBurst( ICM_REG_WHO_AM_I, pucWhoAmI, 1 );
	/* Release control of the SPI Bus */
	vSpiBusEnd( pxModule );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648Configure( xIcmConfiguration_t *pxConfig )
{
	/* Take control of the SPI Bus*/
	if ( eSpiBusStart( pxModule, &xIcmBusConfig, portMAX_DELAY ) != ERROR_NONE ) {
		return ERROR_TIMEOUT;
	}

	/* Disable Interrupts */
	vGpioSetup( xInterruptGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	/* Enable power to specified modules */
	prvIcmSensorEnable( pxConfig->xAccel.bEnabled, pxConfig->xGyro.bEnabled, pxConfig->bTemperatureEnabled );

	/* Setup Accelerometer */
	prvAccelConfig( &pxConfig->xAccel );
	/* Setup Gyroscope */
	prvGyroConfig( &pxConfig->xGyro );
	/* Delay from Silicon Labs driver was not enough, increased from 50ms */
	vTaskDelay( pdMS_TO_TICKS( 100 ) );

	/* Enable Interrupts */
	prvIcmInteruptEnable( pxConfig->xAccel.bEnabled, pxConfig->xGyro.bEnabled );

	/* Put sensor into low power operation mode */
	prvIcmLowPowerMode( pxConfig->xAccel.bEnabled, pxConfig->xGyro.bEnabled, pxConfig->bTemperatureEnabled );

	/* If either sensor is enabled, enable interrupts */
	if ( pxConfig->xAccel.bEnabled || pxConfig->xGyro.bEnabled ) {
		vGpioSetup( xInterruptGpio, GPIO_INPUT, GPIO_INPUT_NOFILTER );
	}

	/* Release control of the SPI Bus */
	vSpiBusEnd( pxModule );

	/* Clear any potentially pending interrupts from old configuration */
	eIcm20648WaitForInterrupt( 0 );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648LowPower( void )
{
	/* Because accelerometers are stupid, they use less power when actually running */
	/* Therefore configure with the lowest possible sample rate */
	eModuleError_t		eError;
	xIcmConfiguration_t xConfig = { { 0 }, { 0 }, false };
	xConfig.xAccel.bEnabled		= true;
	/* Rate less that 3 typically kills the verification task, TODO: Why? */
	xConfig.xAccel.usSampleRate = 3;
	xConfig.xAccel.eFullScale   = ACCEL_SCALE_2G;
	xConfig.xAccel.eFilter		= ACCEL_FILTER_NONE;
	eError						= eIcm20648Configure( &xConfig );
	/* Then disable the interrupt GPIO so we can stay in deep sleep */
	vGpioSetup( xInterruptGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	return eError;
}

/*-----------------------------------------------------------*/

static void prvIcmSensorEnable( bool bAccEn, bool bGyroEn, bool bTempEn )
{
	uint8_t ucPwrManagement1;
	uint8_t ucPwrManagement2 = ICM_PWR_MGMT_2_ACCEL_EN | ICM_PWR_MGMT_2_GYRO_EN;

	prvReadRegisterBurst( ICM_REG_PWR_MGMT_1, &ucPwrManagement1, 1 );

	/* Set the disabled bits for disabled sensors */
	if ( !bAccEn ) {
		ucPwrManagement2 |= ICM_PWR_MGMT_2_ACCEL_DIS;
	}
	if ( !bGyroEn ) {
		ucPwrManagement2 |= ICM_PWR_MGMT_2_GYRO_DIS;
	}

	if ( bTempEn ) {
		ucPwrManagement1 &= ~ICM_PWR_MGMT_1_TMP_DIS;
	}
	else {
		ucPwrManagement1 |= ICM_PWR_MGMT_1_TMP_DIS;
	}
	/* Write the config values to the registers */
	prvWriteRegister( ICM_REG_PWR_MGMT_1, ucPwrManagement1 );
	prvWriteRegister( ICM_REG_PWR_MGMT_2, ucPwrManagement2 );
}

/*-----------------------------------------------------------*/

static void prvIcmLowPowerMode( bool bAccEn, bool bGyroEn, bool bTempEn )
{
	uint8_t ucPwrManagement1;

	/* Perform the mystical incantation that the Silicon Labs driver does to make reconfiguring the sensor work
	 * Don't expect the reasoning to be documented anywhere, because why would it be
	 * https://siliconlabs.github.io/Gecko_SDK_Doc/efr32mg12/html/imu_8c_source.html#l00177
	 * https://siliconlabs.github.io/Gecko_SDK_Doc/efr32mg12/html/icm20648_8c_source.html#l00641
	 */

	prvReadRegisterBurst( ICM_REG_PWR_MGMT_1, &ucPwrManagement1, 1 );
	/* Enter Low Power Mode */
	if ( bAccEn || bGyroEn || bTempEn ) {
		// Clear Sleep Mode
		ucPwrManagement1 &= ~ICM_PWR_MGMT_1_SLEEP;
		prvWriteRegister( ICM_REG_PWR_MGMT_1, ucPwrManagement1 );
		// Disable cycle mode
		prvWriteRegister( ICM_REG_LP_CONFIG, 0x00 );

		/* Silicon Labs Driver calls prvIcmSensorEnable function here, but its already called previously
		 * Driver still works, so its probably safe to leave out
		 * Add back in if issues appear at a later time and 
		 * prvIcmSensorEnable(bAccEn, bGyroEn, bTempEn);
		 */

		vTaskDelay( pdMS_TO_TICKS( 50 ) );
		// Enable cycle mode
		prvWriteRegister( ICM_REG_LP_CONFIG, ICM_LP_CONFIG_ACCEL_CYCLE | ICM_LP_CONFIG_GYRO_CYCLE );
		// Set low power bit
		ucPwrManagement1 |= ICM_PWR_MGMT_1_LP_EN;
	}
	else {
		// Disable cycle mode
		prvWriteRegister( ICM_REG_LP_CONFIG, 0x00 );
		// Clear low power bit
		ucPwrManagement1 &= ~ICM_PWR_MGMT_1_LP_EN;
	}
	prvWriteRegister( ICM_REG_PWR_MGMT_1, ucPwrManagement1 );
}

/*-----------------------------------------------------------*/

static void prvIcmInteruptEnable( bool bAccEn, bool bGyroEn )
{
	/* WOM Interrupt */
	uint8_t ucInterruptEnable1 = 0x00;
	/* If a periodic data source is enabled, enable interrupts */
	if ( bAccEn || bGyroEn ) {
		ucInterruptEnable1 = ICM_INT_ENABLE_1_RAW_DATA_RDY;
	}
	/* Write the config values to the registers, order is important here (for whatever reason) */
	prvWriteRegister( ICM_REG_INT_ENABLE, 0x00 );
	prvWriteRegister( ICM_REG_INT_ENABLE_1, ucInterruptEnable1 );
}

/*-----------------------------------------------------------*/

static void prvAccelConfig( xIcmAccelConfiguration_t *pxAccConfig )
{
	uint16_t usSampleRateDiv = 0;
	uint8_t  ucAccelConfig   = 0x00;
	/* If it's not enabled, we have nothing to do */
	/* Power to accelerometer is disabled by sensor setup */
	if ( !pxAccConfig->bEnabled ) {
		return;
	}
	/* Setup full-scale limit */
	switch ( pxAccConfig->eFullScale ) {
		case ACCEL_SCALE_2G:
			ucAccelConfig |= ICM_ACCEL_CONFIG_2G;
			break;
		case ACCEL_SCALE_4G:
			ucAccelConfig |= ICM_ACCEL_CONFIG_4G;
			break;
		case ACCEL_SCALE_8G:
			ucAccelConfig |= ICM_ACCEL_CONFIG_8G;
			break;
		case ACCEL_SCALE_16G:
			ucAccelConfig |= ICM_ACCEL_CONFIG_16G;
			break;
		default:
			configASSERT( 0 );
	}
	/* Setup accelerometer filtering */
	if ( pxAccConfig->eFilter == ACCEL_FILTER_NONE ) {
		ucAccelConfig |= ICM_ACCEL_CONFIG_DLPF_DIS;
	}
	else {
		/* Refer to https://siliconlabs.github.io/Gecko_SDK_Doc/efr32mg12/html/imu_8c_source.html#l00202 
		 *  Experimentally validated as junk data
		 *  Note this doesn't mean the chip is broken, just that our happy initialisation path doesn't work for the filters
		 *  Good luck to the brave soul who tries to fix this
		 */
		eLog( LOG_IMU_DRIVER, LOG_ERROR, "ICM Configuration Error: Accelerometer filters are broken\r\n" );
		ucAccelConfig |= ICM_ACCEL_CONFIG_DLPF_EN;
		switch ( pxAccConfig->eFilter ) {
			case ACCEL_FILTER_473Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_473HZ;
				break;
			case ACCEL_FILTER_246Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_246HZ;
				break;
			case ACCEL_FILTER_111Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_111HZ;
				break;
			case ACCEL_FILTER_50Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_50HZ;
				break;
			case ACCEL_FILTER_24Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_24HZ;
				break;
			case ACCEL_FILTER_12Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_12HZ;
				break;
			case ACCEL_FILTER_6Hz:
				ucAccelConfig |= ICM_ACCEL_CONFIG_LPF_6HZ;
				break;
			case ACCEL_FILTER_NONE:
			default:
				configASSERT( 0 );
		}
	}

	/* Setup sample rate, capped to 12 bits */
	if ( pxAccConfig->usSampleRate == 0 ) {
		usSampleRateDiv = 4095;
	}
	else {
		usSampleRateDiv = ( 1125 / pxAccConfig->usSampleRate ) - 1;
	}
	/* Write Registers */
	prvWriteRegister( ICM_REG_ACCEL_SMPLRT_DIV_1, ( usSampleRateDiv & 0xF00 ) >> 8 );
	prvWriteRegister( ICM_REG_ACCEL_SMPLRT_DIV_2, usSampleRateDiv & 0xFF );
	prvWriteRegister( ICM_REG_ACCEL_CONFIG, ucAccelConfig );
}

/*-----------------------------------------------------------*/

static void prvGyroConfig( xIcmGyroConfiguration_t *pxGyroConfig )
{
	/* If its not enabled, we have nothing to do */
	/* Power to gyroscope is disabled by sensor setup */
	if ( !pxGyroConfig->bEnabled ) {
		return;
	}
}

/*-----------------------------------------------------------*/

void prvIcmInterrupt( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR( xIcmInterruptSemaphore, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648WaitForInterrupt( TickType_t xTimeout )
{
	if ( xSemaphoreTake( xIcmInterruptSemaphore, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eIcm20648ReadAcc( xIcmAccData_t *pxAccData, TickType_t xTimeout )
{
	/* Take control of the SPI Bus*/
	if ( eSpiBusStart( pxModule, &xIcmBusConfig, xTimeout ) != ERROR_NONE ) {
		return ERROR_TIMEOUT;
	}
	prvReadRegisterBurst( ICM_REG_ACC_XOUT_H, (uint8_t *) pxAccData, 6 );
	/* Release control of the SPI Bus */
	vSpiBusEnd( pxModule );

	/* Swap byte order */
	pxAccData->sAccX = BYTE_SWAP_16( pxAccData->sAccX );
	pxAccData->sAccY = BYTE_SWAP_16( pxAccData->sAccY );
	pxAccData->sAccZ = BYTE_SWAP_16( pxAccData->sAccZ );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
