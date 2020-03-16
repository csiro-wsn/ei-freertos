/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "application.h"
#include "board.h"
#include "thunderboard2.h"

#include "adc.h"
#include "crc.h"
#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "rtc.h"
#include "spi.h"
#include "temp.h"
#include "uart.h"
#include "watchdog.h"

#include "application_images.h"
#include "device_constants.h"
#include "device_nvm.h"
#include "flash_interface.h"
#include "log.h"

#include "bluetooth.h"

#include "unified_comms_bluetooth.h"
#include "unified_comms_gatt.h"
#include "unified_comms_serial.h"

#include "icm20648.h"
#include "mx25r.h"
#include "si1133.h"

#include "bluetooth_logger.h"
#include "onboard_logger.h"
#include "serial_logger.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// #define DEEP_SLEEP_LED

#define WATCHDOG_INSTANCE 			WDOG0
#define SERIAL_INSTANCE 			USART0
#define FLASH_SPI_INSTANCE 			USART2
#define IMU_SPI_INSTANCE 			USART3
#define EXTERNAL_I2C_INSTANCE		I2C0
#define ENVIRONMENTAL_I2C_INSTANCE	I2C1
#define ADC_INSTANCE				ADC0

// clang-format on
/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

void prvBoardLedsInit( void );
void prvBoardNvmInit( void );
void prvBoardSerialInit( void );
void prvBoardInterfaceInit( void );
void prvBoardPinsInit( void );
void prvBoardBluetoothInit( void );
void prvBoardPeripheralInit( void );
void prvBoardLoggersInit( void );

void prvBoardLowPowerInit( void );
void prvBoardServicesInit( void );
void prvBoardPrintIdentifiers( void );

/* Private Variables ----------------------------------------*/

/* Create Watchdog Driver */

/* Create UART Driver, 4 tx buffers of 128 bytes each, 64 byte receive stream */
UART_MODULE_CREATE( SERIAL_OUTPUT, SERIAL_INSTANCE, USART0_RX_IRQHandler, USART0_TX_IRQHandler, 4, SERIAL_INTERFACE_DEFAULT_SIZE, 64 );
SPI_MODULE_CREATE( FLASH_SPI, FLASH_SPI_INSTANCE, UNUSED );
SPI_MODULE_CREATE( IMU_SPI, IMU_SPI_INSTANCE, UNUSED );
I2C_MODULE_CREATE( EXTERNAL_I2C, EXTERNAL_I2C_INSTANCE );
I2C_MODULE_CREATE( ENVIRONMENTAL_I2C, ENVIRONMENTAL_I2C_INSTANCE );
WATCHDOG_MODULE_CREATE( WDOG0_IRQHandler, WDOG0_IRQn, WATCHDOG_INSTANCE );
ADC_MODULE_CREATE( ADC, ADC_INSTANCE, UNUSED );

xWatchdogModule_t *pxWatchdog		  = &WATCHDOG_MODULE_GET( WDOG0_IRQHandler );
xAdcModule_t *	 pxAdc			  = &ADC_MODULE_GET( ADC );
xUartModule_t *	pxUartOutput		  = &UART_MODULE_GET( SERIAL_OUTPUT );
xSpiModule_t *	 pxFlashSpi		  = &SPI_MODULE_GET( FLASH_SPI );
xSpiModule_t *	 pxImuSpi			  = &SPI_MODULE_GET( IMU_SPI );
xI2CModule_t *	 pxExternalI2C	  = &I2C_MODULE_GET( EXTERNAL_I2C );
xI2CModule_t *	 pxEnvironmentalI2C = &I2C_MODULE_GET( ENVIRONMENTAL_I2C );

xSerialModule_t xSerialOutput = {
	.pxImplementation = &xUartBackend,
	.pvContext		  = &UART_MODULE_GET( SERIAL_OUTPUT )
};

/* Board GPIOs */
xLEDConfig_t xLEDConfig = {
	.ePolarity = LED_ACTIVE_HIGH,
	.xRed	  = LED_RED_GPIO,
	.xGreen	= LED_GREEN_GPIO,
	.xBlue	 = LED_BLUE_GPIO,
	.xYellow   = LED_YELLOW_GPIO
};

/* Memory Structures */
xMX25rHardware_t xMX25rHardware = {
	.pxInterface = &SPI_MODULE_GET( FLASH_SPI ),
	.xSpiConfig  = { .xCsGpio = FLASH_CS_GPIO, 0, 0, 0, 0 }
};
xFlashDevice_t xMX25rDevice = {
	.xSettings		  = { 0 },
	.pxImplementation = &xMX25rDriver,
	.xCommandQueue	= NULL,
	.pcName			  = "MX25R",
	.pxHardware		  = (xFlashDefaultHardware_t *) &xMX25rHardware
};
xFlashDevice_t *const  pxOnboardFlash = &xMX25rDevice;
xSerialModule_t *const pxSerialOutput = &xSerialOutput;

/* Sensor Initialisation Structures */
xSi1133Init_t xSi1133Init = {
	.pxModule = &I2C_MODULE_GET( ENVIRONMENTAL_I2C )
};

/* Serial Structures */
STATIC_TASK_STRUCTURES( xSerialReceiveTask, 2 * configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1 );

bool			   bDeepSleepEnabled = true;
xDeviceConstants_t xDeviceConstants;
xAddress_t		   xLocalAddress;

/* Logger Variables */
TDF_LOGGER_STRUCTURES( ONBOARD_STORAGE_LOG, xFlashLog, "FlashLog", (xLoggerDevice_t *) &xOnboardLoggerDevice, 256, 0, LOGGER_LENGTH_REMAINING_BLOCKS );
TDF_LOGGER_STRUCTURES( SERIAL_LOG, xSerialLog, "SerialLog", (xLoggerDevice_t *) &xSerialLoggerDevice, 100, 0, UINT32_MAX );
TDF_LOGGER_STRUCTURES( BLE_LOG, xBluetoothLog, "BtLog", (xLoggerDevice_t *) &xBluetoothLoggerDevice, CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH, 0, LOGGER_LENGTH_REMAINING_BLOCKS );

LOGS( &xFlashLog_log, &xSerialLog_log, &xBluetoothLog_log );

TDF_LOGS( &xFlashLog, &xSerialLog, &xBluetoothLog );

/*-----------------------------------------------------------*/

void vBoardSetupCore( void )
{
	EMU_DCDCInit_TypeDef xDCDCInit = EMU_DCDCINIT_DEFAULT;
	CMU_HFXOInit_TypeDef xHFXOInit = CMU_HFXOINIT_DEFAULT;

	/* Chip errata */
	CHIP_Init();

	/* Init DCDC regulator and HFXO with kit specific parameters */
	EMU_DCDCInit( &xDCDCInit );
	CMU_HFXOInit( &xHFXOInit );

	/* Switch HFCLK to HFXO and disable HFRCO */
	CMU_ClockSelectSet( cmuClock_HF, cmuSelect_HFXO );
	CMU_OscillatorEnable( cmuOsc_HFRCO, false, false );

	/* Set EM4 pin retention so pin configuration stays set if we enter EM4 */
	EMU->EM4CTRL |= EMU_EM4CTRL_EM4IORETMODE_EM4EXIT;
}

/*-----------------------------------------------------------*/

void vBoardInit( void )
{
	/* Let application define log levels */
	vApplicationSetLogLevels();
	/* Initialise board into low power state */
	prvBoardLowPowerInit();
	/* System services init */
	prvBoardServicesInit();
}

/*-----------------------------------------------------------*/

void prvBoardLowPowerInit( void )
{
	/* Initialise GPIO */
	prvBoardPinsInit();
	/* Initialise LEDs */
	prvBoardLedsInit();
	/* Initialise UART first so we have eLog Functionality */
	prvBoardSerialInit();
	/* Initialise Non-Volatile Memory */
	prvBoardNvmInit();
	/* Initialise Shared Interfaces */
	prvBoardInterfaceInit();
	/* Initialise Bluetooth */
	prvBoardBluetoothInit();
	/* Output board identifiers */
	prvBoardPrintIdentifiers();
	/* Wait a bit before initialising devices */
	vTaskDelay( pdMS_TO_TICKS( 200 ) );
	/* Sensor, Memory, Radio Initialisation */
	prvBoardPeripheralInit();
	/* Initialise Logger Structures */
	prvBoardLoggersInit();
}

/*-----------------------------------------------------------*/

void prvBoardPrintIdentifiers( void )
{
	xBluetoothAddress_t xLocalBtAddress;
	uint32_t			ulResetCount;

	eNvmReadData( NVM_RESET_COUNT, &ulResetCount );

	vBluetoothGetLocalAddress( &xLocalBtAddress );

	eLog( LOG_APPLICATION, LOG_APOCALYPSE, "\r\n\tApp        : %d.%d\r\n", APP_MAJOR, APP_MINOR );
	eLog( LOG_APPLICATION, LOG_APOCALYPSE, "\tMAC ADDR   : %:6R\r\n", xLocalBtAddress.pucAddress );
	eLog( LOG_APPLICATION, LOG_APOCALYPSE, "\tReset Count: %d\r\n", ulResetCount );
}

/*-----------------------------------------------------------*/

void prvBoardServicesInit( void )
{
	static xSerialReceiveArgs_t xArgs;
	/* Start our serial handler thread */
	xArgs.pxUart	= pxUartOutput;
	xArgs.fnHandler = fnBoardSerialHandler();
	STATIC_TASK_CREATE( xSerialReceiveTask, vSerialReceiveTask, "Ser Recv", &xArgs );
	/* Setup our Unified Comms interfaces */
	vUnifiedCommsInit( &xSerialComms );
	vUnifiedCommsInit( &xBluetoothComms );
	vUnifiedCommsInit( &xGattComms );
	/* Device by default are ordinary nodes */
	xSerialComms.fnReceiveHandler	= NULL;
	xBluetoothComms.fnReceiveHandler = NULL;
	xGattComms.fnReceiveHandler		 = NULL;
}

/*-----------------------------------------------------------*/

void prvBoardPinsInit( void )
{
	/* Enable GPIO */
	vGpioInit();

	vGpioSetup( FLASH_CS_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( ACC_CS_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );

	vGpioSetup( HALL_EFFECT_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( ENVIRONMENTAL_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( MICROPHONE_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( AIR_QUALITY_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( ACC_CS_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
}

/*-----------------------------------------------------------*/

void prvBoardLedsInit( void )
{
	vLedsInit( &xLEDConfig );
	/* Enabling these all the time isn't the best for power, but this platform is never being deployed */
	vGpioSetup( LED_ENABLE_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( LED_ENABLE_R_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( LED_ENABLE_G_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( LED_ENABLE_B_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
}

/*-----------------------------------------------------------*/

void prvBoardSerialInit( void )
{
	/**
	 *  Don't change from 115200, this is a static baudrate between the chip
	 *  and the debug microcontroller
	 */
	pxUartOutput->ulBaud				 = 115200;
	pxUartOutput->xPlatform.ucTxLocation = USART_ROUTELOC0_TXLOC_LOC0;
	pxUartOutput->xPlatform.ucRxLocation = USART_ROUTELOC0_RXLOC_LOC0;

	eUartInit( pxUartOutput, false );
}

/*-----------------------------------------------------------*/

void prvBoardNvmInit( void )
{
	eModuleError_t eResult;
	uint32_t	   ulResetCount;

	/* Load Device Constants */
	bDeviceConstantsRead( &xDeviceConstants );

	/* Initialise NVM */
	eResult = eNvmInit();
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise NVM\r\n" );
	}
	/* Increment reset count */
	eResult = eNvmIncrementData( NVM_RESET_COUNT, &ulResetCount );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_ERROR, "Failed to increment reset count\r\n" );
	}
}

/*-----------------------------------------------------------*/

void prvBoardInterfaceInit( void )
{
	pxFlashSpi->xPlatform.ucPortLocationMosi = FLASH_MOSI_LOC;
	pxFlashSpi->xPlatform.ucPortLocationMiso = FLASH_MISO_LOC;
	pxFlashSpi->xPlatform.ucPortLocationSclk = FLASH_SCLK_LOC;

	pxImuSpi->xPlatform.ucPortLocationMosi = ACC_MOSI_LOC;
	pxImuSpi->xPlatform.ucPortLocationMiso = ACC_MISO_LOC;
	pxImuSpi->xPlatform.ucPortLocationSclk = ACC_SCLK_LOC;

	pxExternalI2C->xPlatform.ulLocationSda = EXTERNAL_I2C_SDA_LOC;
	pxExternalI2C->xPlatform.ulLocationScl = EXTERNAL_I2C_SCL_LOC;
	pxExternalI2C->xPlatform.xSda		   = EXTERNAL_I2C_SDA_GPIO;
	pxExternalI2C->xPlatform.xScl		   = EXTERNAL_I2C_SCL_GPIO;

	pxEnvironmentalI2C->xPlatform.ulLocationSda = ENVIRONMENTAL_I2C_SDA_LOC;
	pxEnvironmentalI2C->xPlatform.ulLocationScl = ENVIRONMENTAL_I2C_SCL_LOC;
	pxEnvironmentalI2C->xPlatform.xSda			= ENVIRONMENTAL_I2C_SDA_GPIO;
	pxEnvironmentalI2C->xPlatform.xScl			= ENVIRONMENTAL_I2C_SCL_GPIO;

	/* Initialise Interfaces */
	vCrcInit();
	vRtcInit();
	vWatchdogInit( pxWatchdog );
	eSpiInit( pxFlashSpi );
	eSpiInit( pxImuSpi );
	eI2CInit( pxExternalI2C );
	eI2CInit( pxEnvironmentalI2C );
	vAdcInit( pxAdc );
	vTempInit();
}

/*-----------------------------------------------------------*/

void prvBoardBluetoothInit( void )
{
	/* Setup Bluetooth with a maximum transmit power of 10 dBm */
	eBluetoothInit();

	xGattLocalCharacteristic_t xPlatform = {
		.usCharacteristicHandle = gattdb_model_number_string,
		.pucData				= (uint8_t *) "Thunderboard2",
	};
	xGattLocalCharacteristic_t xApplication = {
		.usCharacteristicHandle = gattdb_firmware_revision_string,
		.pucData				= (uint8_t *) STRINGIFY( APP_MAJOR ) "." STRINGIFY( APP_MINOR ),
	};
	xPlatform.usDataLen	= ulStrLen( xPlatform.pucData );
	xApplication.usDataLen = ulStrLen( xApplication.pucData );
	eBluetoothWriteLocalCharacteristic( &xPlatform );
	eBluetoothWriteLocalCharacteristic( &xApplication );

	/* Set bluetooth transmit power. */
	int32_t lTxPower;
	int32_t lTxPowerDefault = 10;
	configASSERT( eNvmReadDataDefault( NVM_BLUETOOTH_TX_POWER_DBM, &lTxPower, &lTxPowerDefault ) == ERROR_NONE );
	lTxPower = cBluetoothSetTxPower( (int8_t) lTxPower );
	eLog( LOG_APPLICATION, LOG_VERBOSE, "Bluetooth TX Power set to %ddBm\r\n", lTxPower );
}

/*-----------------------------------------------------------*/

void prvBoardPeripheralInit( void )
{
	eModuleError_t eResult;
	xIcmInit_t	 xIcmInit = { pxImuSpi, ACC_CS_GPIO, ACC_EN_GPIO, ACC_INT_GPIO };

	/* Initialise Flash Chip */
	eBoardEnablePeripheral( PERIPHERAL_ONBOARD_FLASH, NULL, portMAX_DELAY );
	eResult = eFlashInit( &xMX25rDevice );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise Flash with error code %d\r\n", eResult );
	}
	vBoardDisablePeripheral( PERIPHERAL_ONBOARD_FLASH );

	/* Initialise accelerometer */
	eResult = eIcm20648Init( &xIcmInit );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise ICM20648 with error code %d\r\n", eResult );
	}

	/* Initialise environmental sensors */
	eBoardEnablePeripheral( PERIPHERAL_ENVIRONMENTAL_SENSOR, NULL, portMAX_DELAY );
	eResult = eSi1133Init( &xSi1133Init );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise SI1133 with error code %d\r\n", eResult );
	}
	vBoardDisablePeripheral( PERIPHERAL_ENVIRONMENTAL_SENSOR );
}

/*-----------------------------------------------------------*/

void prvBoardLoggersInit( void )
{
	uint32_t ulReservedSpace			 = ulNumApplicationImages() * ulApplicationImageSize();
	xFlashLog.pxLog->ulStartBlockAddress = ulReservedSpace / xFlashLog.pxLog->usLogicalBlockSize;

	eTdfLoggerConfigure( &xNullLog, LOGGER_CONFIG_INIT_DEVICE, NULL );

	eTdfLoggerConfigure( &xSerialLog, LOGGER_CONFIG_INIT_DEVICE, NULL );
	eTdfLoggerConfigure( &xSerialLog, LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, NULL );

	eTdfLoggerConfigure( &xBluetoothLog, LOGGER_CONFIG_INIT_DEVICE, NULL );
	eTdfLoggerConfigure( &xBluetoothLog, LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, NULL );
}

/*-----------------------------------------------------------*/

void vBoardDeepSleepEnabled( bool bEnable )
{
	bDeepSleepEnabled = bEnable;
}

/*-----------------------------------------------------------*/

bool bBoardCanDeepSleep( void )
{
	bool bUartDeepSleep		 = bUartCanDeepSleep( pxUartOutput );
	bool bSpiADeepSleep		 = bSpiCanDeepSleep( pxFlashSpi );
	bool bSpiBDeepSleep		 = bSpiCanDeepSleep( pxImuSpi );
	bool bBluetoothDeepSleep = bBluetoothCanDeepSleep();

	bool bCanDeepSleep = ( bDeepSleepEnabled && bUartDeepSleep && bSpiADeepSleep && bSpiBDeepSleep && bBluetoothDeepSleep );

#ifdef DEEP_SLEEP_LED
	if ( bCanDeepSleep ) {
		vLedsOff( LEDS_RED );
	}
	else {
		vLedsOn( LEDS_RED );
	}
#endif
	return bCanDeepSleep;
}

/*-----------------------------------------------------------*/

eModuleError_t eBoardEnablePeripheral( ePeripheral_t ePeripheral, bool *pbPowerApplied, TickType_t xTimeout )
{
	UNUSED( xTimeout );
	bool		   bFallback;
	eModuleError_t eError = ERROR_NONE;

	if ( pbPowerApplied == NULL ) {
		pbPowerApplied = &bFallback;
	}
	*pbPowerApplied = false;

	switch ( ePeripheral ) {
		/* Apply power to all environmental sensors */
		case PERIPHERAL_ENVIRONMENTAL_SENSOR:
			vGpioSetup( ENVIRONMENTAL_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
			*pbPowerApplied = true;
			break;
		default:
			break;
	}
	return eError;
}

/*-----------------------------------------------------------*/

void vBoardDisablePeripheral( ePeripheral_t ePeripheral )
{
	switch ( ePeripheral ) {
		/* Remove power from all environmental sensors */
		case PERIPHERAL_ENVIRONMENTAL_SENSOR:
			vGpioSetup( ENVIRONMENTAL_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
			break;
		default:
			break;
	}
}

/*-----------------------------------------------------------*/

void vBoardDeepSleep( void )
{
	vUartDeepSleep( pxUartOutput );
}

/*-----------------------------------------------------------*/

void vBoardWatchdogPeriodic( void )
{
	vWatchdogPeriodic( pxWatchdog );
}

/*-----------------------------------------------------------*/

uint32_t ulBoardAdcSample( xGpio_t xGpio, eAdcResolution_t eResolution, eAdcReferenceVoltage_t eReferenceVoltage )
{
	return ulAdcSample( pxAdc, xGpio, eResolution, eReferenceVoltage );
}

/*-----------------------------------------------------------*/

eModuleError_t eBoardAdcRecalibrate( void )
{
	return eAdcRecalibrate( pxAdc );
}

/*-----------------------------------------------------------*/
