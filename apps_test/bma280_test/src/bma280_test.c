/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "bluetooth.h"

#include "board.h"

#include "gpio.h"
#include "leds.h"
#include "log.h"
#include "rtc.h"
#include "watchdog.h"

#include "device_nvm.h"
#include "unified_comms_bluetooth.h"
#include "unified_comms_serial.h"

#include "bma280.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vTestBma280( void *pvParams );

/* Private Variables ----------------------------------------*/

STATIC_TASK_STRUCTURES( pxTestHandle, 2 * configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY );

/*-----------------------------------------------------------*/

void vApplicationSetLogLevels( void )
{
	eLogSetLogLevel( LOG_APPLICATION, LOG_DEBUG );
	eLogSetLogLevel( LOG_IMU_DRIVER, LOG_VERBOSE );
	eLogSetLogLevel( LOG_BLUETOOTH_GAP, LOG_ERROR );
}

/*-----------------------------------------------------------*/

void vApplicationStartupCallback( void )
{
	WatchdogReboot_t *pxRebootData;

	vLedsOn( LEDS_ALL );

	/* Set a valid time */
	xDateTime_t xValidDatetime = { .xDate = { .usYear = 2018, .eMonth = eJuly, .ucDay = 1, .eDayOfWeek = eUnknownDay },
								   .xTime = { .ucHour = 1, .ucMinute = 58, .ucSecond = 57 } };
	eRtcSetDatetime( &xValidDatetime );

	/* Get Reboot reasons and clear */
	pxRebootData = xWatchdogRebootReason();
	if ( pxRebootData != NULL ) {
		vWatchdogPrintRebootReason( LOG_APPLICATION, LOG_ERROR, pxRebootData );
	}

	// vUnifiedCommsListen( &xBluetoothComms, COMMS_LISTEN_ON_FOREVER );

	STATIC_TASK_CREATE( pxTestHandle, vTestBma280, "Test", NULL );

	vLedsOff( LEDS_ALL );
}

/*-----------------------------------------------------------*/

void vTestBma280( void *pvParams )
{
	xAccelerometerConfiguration_t xConfig = {
		.bEnabled		   = true,
		.bLowPowerMode	 = false,
		.ucFIFOLimit	   = 2,
		.ucRangeG		   = 4,
		.usSampleRateHz	= 8,
		.xNoActivityConfig = {
			.bEnabled		   = false,
			.usThresholdMilliG = 100,
			.usDurationS	   = 2 }
	};
	xAccelerometerState_t	 xState;
	eAccelerometerInterrupt_t eInterruptType;
	uint32_t				  ulEventCounter = 0;
	eModuleError_t			  eError;
	uint16_t				  usTdfId;
	uint8_t					  ucTdfShift;
	xTdfTime_t				  xTime;
	uint32_t				  ulInterruptPeriod;
	bool					  bNoMotionActive = false;

	const bool	bLogTdf	 = true;
	const uint8_t ucTdfLogger = BLE_LOG;

	xAccelerometerSample_t pxData[32];
	UNUSED( pvParams );

	/** 
	 * Ceresat Current Numbers
	 * 
	 * Shutdown 			= 40uA
	 * 10Hz no Int no Read  = 43uA
	 * 10Hz no Read 		= 63uA
	 * 10Hz Reading 		= 82uA
	 * 
	 * 40Hz no Int no Read  = 82uA
	 * 40Hz no Read         = 98uA
	 * 40Hz Reading         = 164uA
	 **/

	/**
	 * BMA with application interrupts has weird behaviour
	 * Operates at low sample rates while interrupt not triggered, then reverts to NORMAL mode behaviour
	 * Multiple samples appear at INT1 per sample as well
	 * 
	 * Look further into this when we want to try the hardware implementations again
	 */

	eBoardEnablePeripheral( PERIPHERAL_IMU, NULL, portMAX_DELAY );

	eError = eBma280Configure( &xConfig, &xState, pdMS_TO_TICKS( 50 ) );
	if ( eError != ERROR_NONE ) {
		eLog( LOG_APPLICATION, LOG_ERROR, "Config Error: %d\r\n", eError );
	}

	const char *pcConfigString = "BMA Configuration:\r\n"
								 "\tEnabled     : %d\r\n"
								 "\tMax Range   : %dG\r\n"
								 "\tSample Rate : %d.%dHz\r\n"
								 "\tPeriod      : %dus\r\n";

	eLog( LOG_APPLICATION, LOG_INFO, pcConfigString, xState.bEnabled, xState.ucMaxG, xState.ulRateMilliHz / 1000, xState.ulRateMilliHz % 1000, xState.ulPeriodUs );

	switch ( xState.ucMaxG ) {
		case 2:
			usTdfId	= TDF_ACC_XYZ_2G;
			ucTdfShift = 0;
			break;
		case 4:
			usTdfId	= TDF_ACC_XYZ_4G;
			ucTdfShift = 1;
			break;
		case 8:
			usTdfId	= TDF_ACC_XYZ_8G;
			ucTdfShift = 2;
			break;
		default:
			usTdfId	= TDF_ACC_XYZ_16G;
			ucTdfShift = 3;
	}

	vTaskDelay( pdMS_TO_TICKS( 10 ) );
	uint32_t ulBufferIndex = 0;

	while ( 1 ) {
		vLedsSet( bNoMotionActive ? LEDS_RED : LEDS_NONE );

		if ( eBma280WaitForInterrupt( &eInterruptType, portMAX_DELAY ) != ERROR_NONE ) {
			eLog( LOG_APPLICATION, LOG_ERROR, "Timeout while waiting for interrupt from BMA280. Problem!!\r\n" );
			vTaskSuspend( NULL );
		}

		switch ( eInterruptType ) {
			case ACCELEROMETER_NEW_DATA:
				if ( eBma280ReadData( pxData, &xTime, &ulInterruptPeriod, xConfig.ucFIFOLimit, pdMS_TO_TICKS( 10 ) ) != ERROR_NONE ) {
					eLog( LOG_APPLICATION, LOG_ERROR, "Error while reading data from BMA280.\r\n" );
				}
				if ( bNoMotionActive ) {
					/* Query if condition has ended yet */
					eBma280ActiveInterrupts( &eInterruptType, portMAX_DELAY );
					bNoMotionActive = ( eInterruptType & ACCELEROMETER_NO_MOTION ) ? true : false;
				}

				xTdfTime_t xSampleDiff = { 0, 2 * ulInterruptPeriod / xConfig.ucFIFOLimit };

				eLog( LOG_APPLICATION, LOG_VERBOSE, "Buffer %5d: %5d ticks\r\n", ulBufferIndex++, ulInterruptPeriod );

				if ( bLogTdf ) {
					uint8_t ucNumTdfs = MAX( 1, xConfig.ucFIFOLimit );
					for ( uint8_t i = 0; i < ucNumTdfs; i++ ) {
						tdf_acc_xyz_signed_t xTdf = { .x = ( pxData[i].lX >> ucTdfShift ), .y = ( pxData[i].lY >> ucTdfShift ), .z = ( pxData[i].lZ >> ucTdfShift ) };
						eTdfAddMulti( ucTdfLogger, usTdfId, TDF_TIMESTAMP_RELATIVE_OFFSET_MS, &xTime, &xTdf );
						xTime = xRtcTdfTimeAdd( xTime, xSampleDiff );
					}
					eTdfFlushMulti( ucTdfLogger );
				}
				break;
			case ACCELEROMETER_NO_MOTION:
				eLog( LOG_APPLICATION, LOG_INFO, "BMA no motion interrupt %d!\r\n", ulEventCounter++ );
				bNoMotionActive = true;
				break;
			default:
				eLog( LOG_APPLICATION, LOG_INFO, "BMA unknown interrupt!\r\n" );
		}
	}
}

/*-----------------------------------------------------------*/
