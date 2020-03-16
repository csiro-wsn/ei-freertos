/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_controller.h
 * Creation_Date: 26/03/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Task which controls the bluetooth radio
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_CONTROLLER
#define __CSIRO_CORE_BLUETOOTH_CONTROLLER
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "event_groups.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eBluetoothState_t {
	BLUETOOTH_OFF			 = 0x01, /**< Bluetooth Stack is off, and must be enabled before operations can be applied */
	BLUETOOTH_SCANNING_1MBPS = 0x02, /**< Stack should scan on 1MBPS PHY when possible */
	BLUETOOTH_SCANNING_2MBPS = 0x04, /**< Stack should scan on 2MBPS PHY when possible */
	BLUETOOTH_SCANNING_CODED = 0x08, /**< Stack should scan on CODED PHY when possible */
	BLUETOOTH_SCANNING_ALL   = 0x0E, /**< Mask containing all scanning options */
	BLUETOOTH_ADVERTISING	= 0x10, /**< Stack is currently advertising */
	BLUETOOTH_CONNECTING	 = 0x20, /**< A connection has been initiated with a remote device */
	BLUETOOTH_CONNECTED		 = 0x40  /**< A connection has been established */
} eBluetoothState_t;

typedef enum eStackCallbackFunction_t {
	STACK_CALLBACK_CONNECTED,
	STACK_CALLBACK_DISCONNECTED,
	STACK_CALLBACK_LOCAL_WRITTEN,
	STACK_CALLBACK_LOCAL_SUBSCRIBED,
	STACK_CALLBACK_REMOTE_CHANGED,
	STACK_CALLBACK_REMOTE_READ
} eStackCallbackFunction_t;

typedef struct xStackCallback_t
{
	TaskHandle_t			 xTaskToResume;
	xBluetoothConnection_t * pxConnection;
	eStackCallbackFunction_t eCallback;
	union
	{
		xGattLocalCharacteristic_t * pxLocal;
		xGattRemoteCharacteristic_t *pxRemote;
	} xParams;
} xStackCallback_t;

/* External Variables ---------------------------------------*/

extern EventGroupHandle_t pxBluetoothState;

/* Function Declarations ------------------------------------*/

void vBluetoothControllerInit( void );

void vBluetoothControllerAdvertisingComplete( void );

void vBluetoothControllerCallbackRun( xStackCallback_t *pvCallback );

static inline eBluetoothState_t eBluetoothPhyToScanningState( eBluetoothPhy_t ePHY )
{
	return ( ePHY == BLUETOOTH_PHY_1M ) ? BLUETOOTH_SCANNING_1MBPS : ( ( ePHY == BLUETOOTH_PHY_2M ) ? BLUETOOTH_SCANNING_2MBPS : BLUETOOTH_SCANNING_CODED );
}

static inline eBluetoothPhy_t eBluetoothScanningStateToPhy( eBluetoothState_t eState )
{
	switch ( eState & BLUETOOTH_SCANNING_ALL ) {
		case BLUETOOTH_SCANNING_1MBPS:
			return BLUETOOTH_PHY_1M;
		case BLUETOOTH_SCANNING_2MBPS:
			return BLUETOOTH_PHY_2M;
		case BLUETOOTH_SCANNING_CODED:
			return BLUETOOTH_PHY_CODED;
		default:
			return 0x00;
	}
}

#endif /* __CSIRO_CORE_BLUETOOTH_CONTROLLER */
