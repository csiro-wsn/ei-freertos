/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
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

#include "unified_comms_bluetooth.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vCustomBluetoothHandler( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, 
							int8_t cRssi, bool bConnectable, 
							uint8_t *pucData, uint8_t ucDataLen );

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
	if (pxRebootData != NULL)
	{
		vWatchdogPrintRebootReason(LOG_APPLICATION, LOG_INFO, pxRebootData);
	}

	/* Setup our Bluetooth receive handler */
	vUnifiedCommsBluetoothCustomHandler(vCustomBluetoothHandler);
	vUnifiedCommsListen(&xBluetoothComms, COMMS_LISTEN_ON_FOREVER);

	vLedsOff(LEDS_ALL);
}

/*-----------------------------------------------------------*/

void vApplicationTickCallback(uint32_t ulUptime)
{
	UNUSED(ulUptime);
}

/*-----------------------------------------------------------*/

void vCustomBluetoothHandler( const uint8_t *pucAddress, eBluetoothAddressType_t eAddressType, 
							int8_t cRssi, bool bConnectable, 
							uint8_t *pucData, uint8_t ucDataLen )
{
	UNUSED(eAddressType);
	UNUSED(bConnectable);

	/* Limit printed devices based on RSSI */
	if (cRssi < -60) {
		return;
	}

	eLog(LOG_APPLICATION, LOG_INFO, "%:6R %3d dBm = % *A\r\n", pucAddress, cRssi, ucDataLen, pucData );
}

/*-----------------------------------------------------------*/
