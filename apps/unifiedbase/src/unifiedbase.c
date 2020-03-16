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

#include "device_nvm.h"

#include "unified_comms_bluetooth.h"
#include "unified_comms_gatt.h"
#include "unified_comms_serial.h"

#include "bluetooth.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vApplicationSetLogLevels( void )
{
	eLogSetLogLevel( LOG_APPLICATION, LOG_INFO );
	eLogSetLogLevel( LOG_BLUETOOTH_GAP, LOG_ERROR );
	eLogSetLogLevel( LOG_BLUETOOTH_GATT, LOG_INFO );
}

/*-----------------------------------------------------------*/

void vApplicationStartupCallback( void )
{
	WatchdogReboot_t *  pxRebootData;
	tdf_watchdog_info_t xWatchdogInfo;
	vLedsOn( LEDS_ALL );
	/* Get Reboot reasons and clear */
	pxRebootData = xWatchdogRebootReason();
	if ( pxRebootData != NULL ) {
		vWatchdogPrintRebootReason( LOG_APPLICATION, LOG_INFO, pxRebootData );
		vWatchdogPopulateTdf( pxRebootData, &xWatchdogInfo );

		eTdfAddMulti( SERIAL_LOG, TDF_WATCHDOG_INFO_SMALL, TDF_TIMESTAMP_NONE, NULL, &xWatchdogInfo );
		eTdfFlushMulti( SERIAL_LOG );
	}

	/* Our received packets should use the Router handler */
	xSerialComms.fnReceiveHandler	= vUnifiedCommsBasicRouter;
	xBluetoothComms.fnReceiveHandler = vUnifiedCommsBasicRouter;
	xGattComms.fnReceiveHandler		 = vUnifiedCommsBasicRouter;
	/* UART always receiving */
	vUnifiedCommsListen( &xSerialComms, COMMS_LISTEN_ON_FOREVER );
	/* Start bluetooth scanning */
	vUnifiedCommsListen( &xBluetoothComms, COMMS_LISTEN_ON_FOREVER );

	vLedsOff( LEDS_ALL );
}

/*-----------------------------------------------------------*/

void vApplicationTickCallback( uint32_t ulUptime )
{
	UNUSED( ulUptime );

	xDateTime_t xDatetime;
	vLedsToggle( LEDS_BLUE );

	bRtcGetDatetime( &xDatetime );
	eRtcPrintDatetime( &xDatetime, LOG_APPLICATION, LOG_ERROR, "Time: ", "\r\n" );
}

/*-----------------------------------------------------------*/

bool bUnifiedCommsEncryptionKey( xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xDestination, uint8_t *ppucEncryptionKey[16] )
{
	return bUnifiedCommsDecryptionKey(pxInterface, eType, xDestination, ppucEncryptionKey);
}

/*-----------------------------------------------------------*/

bool bUnifiedCommsDecryptionKey( xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xDestination, uint8_t *ppucDecryptionKey[16] )
{
	UNUSED( pxInterface );
	UNUSED( eType );
	UNUSED( xDestination );
	UNUSED( ppucDecryptionKey );
	return false;
}

/*-----------------------------------------------------------*/