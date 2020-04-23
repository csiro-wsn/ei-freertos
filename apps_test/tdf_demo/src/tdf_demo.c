/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "board.h"

#include "gpio.h"
#include "leds.h"
#include "log.h"
#include "rtc.h"
#include "watchdog.h"


#include "tdf.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vApplicationSetLogLevels(void) 
{
	eLogSetLogLevel(LOG_RESULT, LOG_INFO);
	eLogSetLogLevel(LOG_APPLICATION, LOG_INFO);
}

/*-----------------------------------------------------------*/

void vApplicationStartupCallback(void) 
{
	WatchdogReboot_t *pxRebootData;

	vLedsOn(LEDS_ALL);

	/* Get Reboot reason */
	pxRebootData = xWatchdogRebootReason();
	if (pxRebootData != NULL) {
		vWatchdogPrintRebootReason(LOG_APPLICATION, LOG_INFO, pxRebootData);
	}

	vLedsOff(LEDS_ALL);
}

/*-----------------------------------------------------------*/

void vApplicationTickCallback(uint32_t ulUptime) 
{
	xTdfTime_t xTime;
	tdf_uptime_t xUptime;
	tdf_acc_xyz_4g_t xAcc4G;
	tdf_battery_stats_t xBattery;

	/* Print the current system time */
	xDateTime_t xDatetime;
	bRtcGetDatetime(&xDatetime);
	eRtcPrintDatetime(&xDatetime, LOG_APPLICATION, LOG_ERROR, "Time: ", "\r\n");

	/* After some time, update the RTC to a more reasonable value */
	if (ulUptime == 15) {
		xDateTime_t xNewDatetime = { .xDate = { .usYear = 2020, .eMonth = eApril, .ucDay = 23, .eDayOfWeek = eUnknownDay },
								   .xTime = { .ucHour = 10, .ucMinute = 58, .ucSecond = 57 } };
		eRtcSetDatetime(&xNewDatetime);
	}

	/* Get the current system tdf time (seconds since 01/01/2000) */
	bRtcGetTdfTime( &xTime );

	switch(ulUptime % 3) {
		case 0:
			/** 
			 * A single TDF, no timestamps.
			 * Data samples will be assigned a timestamp when decoded at the PC.
			 * The binary size of this sample will be:
			 * 		sizeof(TDF Header) + sizeof(tdf_uptime_t)
			 * 		2 + 4 = 6 bytes
			 */
			xUptime.uptime = ulUptime;
			eTdfAddMulti(BLE_LOG, TDF_UPTIME, TDF_TIMESTAMP_NONE, NULL, &xUptime);
			break;
		case 1:
			/** 
			 * A single TDF, with a global timestamp.
			 * The time provided here will be received at the decoder.
			 * The binary size of this sample will be:
			 * 		sizeof(TDF Header) + sizeof(Global Timestamp) + sizeof(tdf_battery_stats_t)
			 * 		2 + 6 + 4 = 12 bytes
			 */
			xBattery.battery_voltage = 3700;
			xBattery.charge_current = 100;
			eTdfAddMulti(BLE_LOG, TDF_BATTERY_STATS, TDF_TIMESTAMP_GLOBAL, &xTime, &xBattery);
			break;
		default:
			/** 
			 * Two TDFs, with relative timestamps.
			 * The first sample will automatically be promoted to a Global timestamp, to provide
			 * a time for further relative timestamps to be indexed against.
			 * The binary size of the first sample will be:
			 * 		sizeof(TDF Header) + sizeof(Global Timestamp) + sizeof(tdf_acc_xyz_4g_t)
			 * 		2 + 6 + 6 = 14 bytes
			 * The binary size of the second sample will be:
			 * 		sizeof(TDF Header) + sizeof(Relative Timestamp) + sizeof(tdf_acc_xyz_4g_t)
			 * 		2 + 2 + 6 = 10 bytes
			 */
			xAcc4G.x = 2 * ulUptime;
			xAcc4G.y = 20 - ulUptime;
			xAcc4G.z = ulUptime * ulUptime;
			eTdfAddMulti(BLE_LOG, TDF_ACC_XYZ_4G, TDF_TIMESTAMP_RELATIVE_OFFSET_MS, &xTime, &xAcc4G);
			xTime.ulSecondsSince2000++;
			eTdfAddMulti(BLE_LOG, TDF_ACC_XYZ_4G, TDF_TIMESTAMP_RELATIVE_OFFSET_MS, &xTime, &xAcc4G);
			break;
	}

	/** 
	 * Force the BLE_LOG to send all data.
	 * If this is not called, data will be buffered until the TDF Logger fills up
	 * 
	 * Note that the payload sizes received at baselisten will be larger than those specified
	 * above due to the packet headers and data padding on bluetooth.
	 */
	eTdfFlushMulti(BLE_LOG);
}

/*-----------------------------------------------------------*/
