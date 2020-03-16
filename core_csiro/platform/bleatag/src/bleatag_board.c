/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

/* Board Layout */
#include "application.h"
#include "bleatag.h"
#include "board.h"
#include "nrf_soc.h"

/* System Services */
#include "adc.h"
#include "application_images.h"
#include "crc.h"
#include "device_constants.h"
#include "device_nvm.h"
#include "gpio.h"
#include "i2c.h"
#include "leds.h"
#include "rtc.h"
#include "spi.h"
#include "tdf.h"
#include "temp.h"
#include "uart.h"
#include "watchdog.h"

/* Board Peripherals */
#include "bma280.h"
#include "buzzer.h"
#include "mx25r.h"

/* Communication */
#include "bluetooth.h"

#include "log.h"
#include "unified_comms.h"
#include "unified_comms_bluetooth.h"
#include "unified_comms_gatt.h"
#include "unified_comms_serial.h"

/* Data Loggers */
#include "bluetooth_logger.h"
#include "onboard_logger.h"
#include "serial_logger.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define BATTERY_MEASURE_EN_COUNT	16

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvBoardLowPowerInit( void );
void prvBoardServicesInit( void );
void prvBoardPrintIdentifiers( void );
void prvBoardLedsInit( void );
void prvBoardNvmInit( void );
void prvBoardSerialInit( void );
void prvBoardInterfaceInit( void );
void prvBoardPinsInit( void );
void prvBoardBluetoothInit( void );
void prvBoardPeripheralInit( void );
void prvBoardLoggersInit( void );

/* Private Variables ----------------------------------------*/

/* Create UART Driver, 4 buffers of 128 bytes each, 64 byte receive stream */
UART_MODULE_CREATE( SERIAL_OUTPUT, NRF_UARTE0, UARTE0_UART0_IRQHandler, UNUSED_IRQ, 4, SERIAL_INTERFACE_DEFAULT_SIZE, 64 );

/* Create Watcdog Timer: The handler is assigned during initialisation, set to NULL as placeholder */
WATCHDOG_MODULE_CREATE( WDT_IRQHandler, WDT_IRQn, NULL );
ADC_MODULE_CREATE( ADC, ADC_INSTANCE, UNUSED );
SPI_MODULE_CREATE( FLASH_SPI, FLASH_SPI_INSTANCE, SPIM0_TWIM0_IRQHandler );
SPI_MODULE_CREATE( BMA280_SPI, BMA280_SPI_INSTANCE, SPIM1_TWIM1_IRQHandler );
PWM_MODULE_CREATE( BUZZER, NRF_PWM0, PWM0_IRQHandler );

xWatchdogModule_t *pxWatchdog   = &WATCHDOG_MODULE_GET( WDT_IRQHandler );
xAdcModule_t *	 pxAdc		= &ADC_MODULE_GET( ADC );
xUartModule_t *	pxUartOutput = &UART_MODULE_GET( SERIAL_OUTPUT );
xSpiModule_t *	 pxFlashSpi   = &SPI_MODULE_GET( FLASH_SPI );
xSpiModule_t *	 pxBma280Spi  = &SPI_MODULE_GET( BMA280_SPI );
xPwmModule_t *	 pxBuzzerPwm  = &PWM_MODULE_GET( BUZZER );

xSerialModule_t xSerialOutput = {
	.pxImplementation = &xUartBackend,
	.pvContext		  = &UART_MODULE_GET( SERIAL_OUTPUT )
};

/* LED GPIO Pins */
xLEDConfig_t xLEDConfig = {
	.ePolarity = LED_ACTIVE_HIGH,
	.xBlue	 = LED_1,
	.xRed	  = LED_2,
	.xGreen	= LED_3,
	.xYellow   = UNUSED_GPIO
};

/* Accelerometer Structures */
xBma280Init_t xBmaInit = {
	.pxSpi		 = &SPI_MODULE_GET( BMA280_SPI ),
	.xChipSelect = BMA280_CS_GPIO,
	.xInterrupt1 = BMA280_INT1_GPIO,
	.xInterrupt2 = BMA280_INT2_GPIO
};

/* Flash Memory */
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

xGpio_t xButtonGpio = BUTTON_1_GPIO;

/* Pin Control Variables */
STATIC_SEMAPHORE_STRUCTURES( xBatteryMeasureEnable );

/* System Structures */
xDeviceConstants_t xDeviceConstants;
xAddress_t		   xLocalAddress;

/* Logger Variables */
TDF_LOGGER_STRUCTURES( SERIAL_LOG, xSerialLog, "SerialLog", (xLoggerDevice_t *) &xSerialLoggerDevice, 100, 0, UINT32_MAX );
TDF_LOGGER_STRUCTURES( BLE_LOG, xBluetoothLog, "BtLog", (xLoggerDevice_t *) &xBluetoothLoggerDevice, CSIRO_BLUETOOTH_MESSAGE_MAX_LENGTH, 0, UINT32_MAX );
TDF_LOGGER_STRUCTURES( ONBOARD_STORAGE_LOG, xFlashLog, "FlashLog", (xLoggerDevice_t *) &xOnboardLoggerDevice, 256, 0, LOGGER_LENGTH_REMAINING_BLOCKS );

LOGS( &xSerialLog_log, &xBluetoothLog_log, &xFlashLog_log );
TDF_LOGS( &xSerialLog, &xBluetoothLog, &xFlashLog );

/*-----------------------------------------------------------*/

void vBoardSetupCore( void )
{
	;
}

/*-----------------------------------------------------------*/

void vBoardInit( void )
{
	/* Initialise the bluetooth stack as the first action */
	eBluetoothInit();
	sd_power_dcdc_mode_set( NRF_POWER_DCDC_ENABLE );
	/* Let application define log levels */
	vApplicationSetLogLevels();
	/* Initialise board into low power state */
	prvBoardLowPowerInit();
	/* Output board identifiers */
	prvBoardPrintIdentifiers();
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
	xLocalAddress = xAddressUnpack( xLocalBtAddress.pucAddress );

	/* Output Identifiers */
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
	configASSERT( xTaskCreate( vSerialReceiveTask, "Ser Recv", configMINIMAL_STACK_SIZE, &xArgs, tskIDLE_PRIORITY + 1, NULL ) == pdPASS );
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
	vGpioInit();

	STATIC_SEMAPHORE_CREATE_COUNTING( xBatteryMeasureEnable, BATTERY_MEASURE_EN_COUNT, BATTERY_MEASURE_EN_COUNT );

	/* Turn off voltage sensing circuitry */
	vGpioSetup( BATTERY_VOLTAGE_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( BATTERY_MEAS_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );

	/* Enable the button */
	vGpioSetup( BUTTON_1_GPIO, GPIO_INPUT, GPIO_INPUT_NOFILTER );

	/* Initialise Flash SPI Pins */
	vGpioSetup( FLASH_CS_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( FLASH_MISO_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( FLASH_MOSI_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( FLASH_SCK_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );

	/* Initialise IMU SPI Pins */
	vGpioSetup( BMA280_ENABLE_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	vGpioSetup( BMA280_CS_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( BMA280_MISO_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( BMA280_MOSI_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	vGpioSetup( BMA280_SCK_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );

	/* Initialise buzzer pins */
	vGpioSetup( BUZZER_ENABLE_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( BUZZER_PWM_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
}

/*-----------------------------------------------------------*/

void prvBoardLedsInit( void )
{
	vLedsInit( &xLEDConfig );
}

/*-----------------------------------------------------------*/

void prvBoardSerialInit( void )
{
	/* TODO: Figure out how to increase while retaining reliable serial recv */
	pxUartOutput->xPlatform.pxTimer = NRF_TIMER1;
	pxUartOutput->ulBaud			= 115200;
	pxUartOutput->xPlatform.xRx		= UART_RX_GPIO;
	pxUartOutput->xPlatform.xTx		= UART_TX_GPIO;
	pxUartOutput->xPlatform.xRts	= UART_RTS_GPIO;
	pxUartOutput->xPlatform.xCts	= UNUSED_GPIO;

	eUartInit( pxUartOutput, true );
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
	/* Setup the MX25 external flash interface channel */
	pxFlashSpi->xPlatform.xMosi = FLASH_MOSI_GPIO;
	pxFlashSpi->xPlatform.xMiso = FLASH_MISO_GPIO;
	pxFlashSpi->xPlatform.xSclk = FLASH_SCK_GPIO;

	/* Setup the BMA280 accelerometer interface channel */
	pxBma280Spi->xPlatform.xMosi = BMA280_MOSI_GPIO;
	pxBma280Spi->xPlatform.xMiso = BMA280_MISO_GPIO;
	pxBma280Spi->xPlatform.xSclk = BMA280_SCK_GPIO;

	/* Initialise Interfaces */
	vCrcInit();
	vRtcInit();
	eSpiInit( pxBma280Spi );
	eSpiInit( pxFlashSpi );
	vWatchdogInit( pxWatchdog );
	vAdcInit( pxAdc );
	vTempInit();
}

/*-----------------------------------------------------------*/

void prvBoardLoggersInit( void )
{
	uint32_t ulReservedSpace			 = ulNumApplicationImages() * ulApplicationImageSize();
	xFlashLog.pxLog->ulStartBlockAddress = ulReservedSpace / xFlashLog.pxLog->usLogicalBlockSize;

	eTdfLoggerConfigure( &xNullLog, LOGGER_CONFIG_INIT_DEVICE, NULL );

	eTdfLoggerConfigure( &xSerialLog, LOGGER_CONFIG_INIT_DEVICE, 0 );
	eTdfLoggerConfigure( &xSerialLog, LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, 0 );

	eTdfLoggerConfigure( &xBluetoothLog, LOGGER_CONFIG_INIT_DEVICE, 0 );
	eTdfLoggerConfigure( &xBluetoothLog, LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, 0 );

	eTdfLoggerConfigure( &xFlashLog, LOGGER_CONFIG_INIT_DEVICE, NULL );
	eTdfLoggerConfigure( &xFlashLog, LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, 0 );
	eTdfLoggerConfigure( &xFlashLog, LOGGER_CONFIG_APPEND_MODE, 0 );
}

/*-----------------------------------------------------------*/

void prvBoardBluetoothInit( void )
{
	eModuleError_t eGattInit( void );
	/* Initialise GATT Table */
	eGattInit();

	xGattLocalCharacteristic_t xPlatform = {
		.usCharacteristicHandle = gattdb_model_number_string,
		.pucData				= (uint8_t *) "BLEATag",
	};
	xGattLocalCharacteristic_t xApplication = {
		.usCharacteristicHandle = gattdb_firmware_revision_string,
		.pucData				= (uint8_t *) STRINGIFY( APP_MAJOR ) "." STRINGIFY( APP_MINOR ),
	};
	xPlatform.usDataLen	= ulStrLen( xPlatform.pucData );
	xApplication.usDataLen = ulStrLen( xApplication.pucData );
	eBluetoothWriteLocalCharacteristic( &xPlatform );
	eBluetoothWriteLocalCharacteristic( &xApplication );

	/* Set Local Address if constants are valid */
	if ( xDeviceConstants.ulKey == DEVICE_CONSTANTS_KEY ) {
		xBluetoothAddress_t xLocalBtAddress;
		xLocalBtAddress.eAddressType = BLUETOOTH_ADDR_TYPE_PUBLIC;
		pvMemcpy( xLocalBtAddress.pucAddress, xDeviceConstants.pucIEEEMac, BLUETOOTH_MAC_ADDRESS_LENGTH );
		eBluetoothSetLocalAddress( &xLocalBtAddress );
	}

	/* Setup Bluetooth with TX power value in NVM, or -4 dBm if it does not yet exist in NVM */
	int32_t lTxPower;
	int32_t lTxPowerDefault = -4;
	configASSERT( eNvmReadDataDefault( NVM_BLUETOOTH_TX_POWER_DBM, &lTxPower, &lTxPowerDefault ) == ERROR_NONE );
	lTxPower = cBluetoothSetTxPower( (int8_t) lTxPower );
	eLog( LOG_APPLICATION, LOG_VERBOSE, "Bluetooth TX Power set to %ddBm\r\n", lTxPower );
}

/*-----------------------------------------------------------*/

void prvBoardPeripheralInit( void )
{
	eModuleError_t eResult;

	/* Initialise flash chip */
	eBoardEnablePeripheral( PERIPHERAL_ONBOARD_FLASH, NULL, portMAX_DELAY );
	eResult = eFlashInit( &xMX25rDevice );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise Flash with error code %d\r\n", eResult );
	}
	vBoardDisablePeripheral( PERIPHERAL_ONBOARD_FLASH );

	/* Initialise Accelerometer */
	configASSERT( eBoardEnablePeripheral( PERIPHERAL_IMU, NULL, portMAX_DELAY ) == ERROR_NONE );
	eResult = eBma280Init( &xBmaInit, pdMS_TO_TICKS( 250 ) );
	if ( eResult != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_APOCALYPSE, "Failed to initialise BMA280 with error code %d\r\n", eResult );
	}
	vBoardDisablePeripheral( PERIPHERAL_IMU );

	pxBuzzerPwm->xPwmGpio = BUZZER_PWM_GPIO;
	vBuzzerInit( pxBuzzerPwm, BUZZER_ENABLE_GPIO );
}

/*-----------------------------------------------------------*/

void vBoardWatchdogPeriodic( void )
{
	vWatchdogPeriodic( pxWatchdog );
}

/*-----------------------------------------------------------*/

eModuleError_t eBoardEnablePeripheral( ePeripheral_t ePeripheral, bool *pbPowerApplied, TickType_t xTimeout )
{
	bool		   bFallback;
	BaseType_t	 uxCount;
	eModuleError_t eError = ERROR_NONE;

	if ( pbPowerApplied == NULL ) {
		pbPowerApplied = &bFallback;
	}
	*pbPowerApplied = false;

	switch ( ePeripheral ) {
		case PERIPHERAL_BATTERY_MONITORING:
			uxCount = uxSemaphoreGetCount( xBatteryMeasureEnable );
			eError  = ( xSemaphoreTake( xBatteryMeasureEnable, xTimeout ) == pdPASS ) ? ERROR_NONE : ERROR_TIMEOUT;
			/* Only actually enabled on the first take */
			if ( uxCount == BATTERY_MEASURE_EN_COUNT ) {
				vGpioSetup( BATTERY_MEAS_EN_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
				vTaskDelay( 1 );
				/* Power was applied */
				*pbPowerApplied = true;
			}
			break;
		case PERIPHERAL_IMU:
			vGpioSetup( BMA280_CS_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
			vGpioSetup( BMA280_MISO_GPIO, GPIO_INPUTPULL, GPIO_INPUTPULL_PULLUP );
			vGpioSetup( BMA280_MOSI_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
			vGpioSetup( BMA280_SCK_GPIO, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
			vGpioSet( BMA280_ENABLE_GPIO );
			/* Power was applied to the peripheral */
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
		case PERIPHERAL_BATTERY_MONITORING:
			xSemaphoreGive( xBatteryMeasureEnable );
			if ( uxSemaphoreGetCount( xBatteryMeasureEnable ) == BATTERY_MEASURE_EN_COUNT ) {
				vGpioClear( BATTERY_MEAS_EN_GPIO );
				vGpioSetup( BATTERY_VOLTAGE_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
			}
			break;
		case PERIPHERAL_IMU:
			vGpioClear( BMA280_ENABLE_GPIO );
			vGpioSetup( BMA280_CS_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
			vGpioSetup( BMA280_MISO_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
			vGpioSetup( BMA280_MOSI_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
			vGpioSetup( BMA280_SCK_GPIO, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
			break;
		default:
			break;
	}
}

/*-----------------------------------------------------------*/

uint32_t ulBoardBatteryVoltageMV( void )
{
	uint32_t ulAdc = ulAdcSample( pxAdc, BATTERY_VOLTAGE_GPIO, ADC_RESOLUTION_12BIT, ADC_REFERENCE_VOLTAGE_0V6 );
	/** 
	 * Pin mV = 1000 * Reference * ADC_Reading / Full_Scale
	 * 		  = 1000 * 0.6 * ADC_Reading / (2**12 - 1)
	 * 		  = 600 * ADC_READING / 4095
	 */
	uint32_t ulPinVoltage = ( 600 * ulAdc ) / 4095;

	/**
	 * Convert the pin reading to battery voltage.
	 * Pin voltage comes from a voltage divider of 49.9k over 10k
	 * 
	 * Pin = ( Battery * 10k ) / ( 49.9k + 10k )
	 * Battery = Pin * ( 49.9k + 10k ) / ( 10k )
	 *         = ( Pin * 599 ) / 100
	 */
	return ( ulPinVoltage * 599 ) / 100;
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
