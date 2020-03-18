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

#include "test_reporting.h"
#include "tiny_printf.h"

#include "device_nvm.h"

#include "unified_comms_serial.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vCustomSerialHandler(xCommsInterface_t *pxComms,
						  xUnifiedCommsIncomingRoute_t *pxCurrentRoute,
						  xUnifiedCommsMessage_t *pxMessage);

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

	/* Setup our serial receive handler */
	xSerialComms.fnReceiveHandler = vCustomSerialHandler;
	vUnifiedCommsListen(&xSerialComms, COMMS_LISTEN_ON_FOREVER);

	vLedsOff(LEDS_ALL);
}

/*-----------------------------------------------------------*/

void vApplicationTickCallback(uint32_t ulUptime)
{
	tdf_uptime_t xUptime = {ulUptime};
	eTdfAddMulti(BLE_LOG, TDF_UPTIME, TDF_TIMESTAMP_NONE, NULL, &xUptime);
	eTdfFlushMulti(BLE_LOG);

	xDateTime_t xDatetime;
	bRtcGetDatetime(&xDatetime);
	eRtcPrintDatetime(&xDatetime, LOG_APPLICATION, LOG_ERROR, "Time: ", "\r\n");
}

/*-----------------------------------------------------------*/

void vCustomSerialHandler(xCommsInterface_t *pxComms,
						  xUnifiedCommsIncomingRoute_t *pxCurrentRoute,
						  xUnifiedCommsMessage_t *pxMessage)
{
	char pcLocalString[60] = {0};
	UNUSED(pxCurrentRoute);
	UNUSED(pxComms);
	/* 
	 * Copy the string to a local buffer so it can be NULL terminated properly
	 * The %s format specifier does not respect provided lengths
	 */
	pvMemcpy(pcLocalString, pxMessage->pucPayload, pxMessage->usPayloadLen);

	eLog(LOG_APPLICATION, LOG_INFO, "\r\nReceived PKT:\r\n");
	eLog(LOG_APPLICATION, LOG_INFO, "\t  Type: %02X\r\n", pxMessage->xPayloadType);
	eLog(LOG_APPLICATION, LOG_INFO, "\tString: %s\r\n\r\n", pcLocalString);

	vLedsToggle(LEDS_BLUE);
}

/*-----------------------------------------------------------*/
