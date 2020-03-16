/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "bluetooth_utility.h"

#include "bluetooth.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvPrintService( SerialLog_t eLogger, LogLevel_t eLevel, xGattService_t *pxService );
void prvPrintCharacteristic( SerialLog_t eLogger, LogLevel_t eLevel, xGattRemoteCharacteristic_t *pxCharacteristic );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

xGattService_t *pxBluetoothSearchServiceUUID( xBluetoothConnection_t *pxConnection, xBluetoothUUID_t *pxUUID )
{
	for ( uint8_t i = 0; i < pxConnection->ucNumServices; i++ ) {
		if ( bBluetoothUUIDsEqual( pxUUID, &pxConnection->pxServices[i].xUUID ) ) {
			return &pxConnection->pxServices[i];
		}
	}
	return NULL;
}

/*-----------------------------------------------------------*/

xGattRemoteCharacteristic_t *pxBluetoothSearchCharacteristicUUID( xBluetoothConnection_t *pxConnection, xBluetoothUUID_t *pxUUID )
{
	for ( uint8_t i = 0; i < pxConnection->ucNumCharacteristics; i++ ) {
		if ( bBluetoothUUIDsEqual( pxUUID, &pxConnection->pxCharacteristics[i].xUUID ) ) {
			return &pxConnection->pxCharacteristics[i];
		}
	}
	return NULL;
}

/*-----------------------------------------------------------*/

xGattRemoteCharacteristic_t *pxBluetoothSearchCharacteristicHandle( xBluetoothConnection_t *pxConnection, uint16_t usHandle )
{
	for ( uint8_t i = 0; i < pxConnection->ucNumCharacteristics; i++ ) {
		if ( usHandle == pxConnection->pxCharacteristics[i].usCharacteristicHandle ) {
			return &pxConnection->pxCharacteristics[i];
		}
	}
	return NULL;
}

/*-----------------------------------------------------------*/

void vBluetoothPrintConnectionGattTable( SerialLog_t eLogger, LogLevel_t eLevel, xBluetoothConnection_t *pxConnection )
{
	xGattService_t *			 pxService		  = pxConnection->pxServices;
	xGattRemoteCharacteristic_t *pxCharacteristic = pxConnection->pxCharacteristics;

	if ( pxConnection->ucNumServices == 0 ) {
		eLog( eLogger, eLevel, "No discovered services on this GATT connection\r\n" );
	}

	/* For each service */
	for ( uint8_t ucService = 0; ucService < pxConnection->ucNumServices; ucService++ ) {
		/* Print the service */
		prvPrintService( eLogger, eLevel, pxService );
		/* Loop through all characteristics belonging to the service */
		while ( pxCharacteristic->uServiceReference.ulServiceHandle == pxService->uServiceReference.ulServiceHandle ) {
			/* Print the characteristic */
			prvPrintCharacteristic( eLogger, eLevel, pxCharacteristic );
			/* Move to the next characteristic */
			pxCharacteristic++;
		}
		/* Next Service */
		pxService++;
	}
}

/*-----------------------------------------------------------*/

void prvPrintService( SerialLog_t eLogger, LogLevel_t eLevel, xGattService_t *pxService )
{
	uint8_t *pucUUID;
	/* Bluetooth Organisation Services have a 16bit Service ID */
	if ( pxService->xUUID.bBluetoothOfficialUUID ) {
		eLog( eLogger, eLevel, "Service Handles %d-%d: 0x%04X\r\n",
			  pxService->uServiceReference.xHandleRange.usRangeStart, pxService->uServiceReference.xHandleRange.usRangeStop, pxService->xUUID.uUUID.usOfficialUUID );
	}
	/* Custom Services have a 128bit Service ID */
	else {
		pucUUID = pxService->xUUID.uUUID.xCustomUUID.pucUUID128;
		eLog( eLogger, eLevel, "Service Handles %d-%d: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n",
			  pxService->uServiceReference.xHandleRange.usRangeStart, pxService->uServiceReference.xHandleRange.usRangeStop,
			  pucUUID[15], pucUUID[14], pucUUID[13], pucUUID[12], pucUUID[11], pucUUID[10], pucUUID[9], pucUUID[8],
			  pucUUID[7], pucUUID[6], pucUUID[5], pucUUID[4], pucUUID[3], pucUUID[2], pucUUID[1], pucUUID[0] );
	}
}

/*-----------------------------------------------------------*/

void prvPrintCharacteristic( SerialLog_t eLogger, LogLevel_t eLevel, xGattRemoteCharacteristic_t *pxCharacteristic )
{
	uint8_t *pucUUID;
	/* Bluetooth Organisation Services have a 16bit Service ID */
	if ( pxCharacteristic->xUUID.bBluetoothOfficialUUID ) {
		eLog( eLogger, eLevel, "\tCharacteristic %2d: 0x%04X\r\n", pxCharacteristic->usCharacteristicHandle, pxCharacteristic->xUUID.uUUID.usOfficialUUID );
	}
	/* Custom Services have a 128bit Service ID */
	else {
		pucUUID = pxCharacteristic->xUUID.uUUID.xCustomUUID.pucUUID128;
		eLog( eLogger, eLevel, "\tCharacteristic %2d: %02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n",
			  pxCharacteristic->usCharacteristicHandle,
			  pucUUID[15], pucUUID[14], pucUUID[13], pucUUID[12], pucUUID[11], pucUUID[10], pucUUID[9], pucUUID[8],
			  pucUUID[7], pucUUID[6], pucUUID[5], pucUUID[4], pucUUID[3], pucUUID[2], pucUUID[1], pucUUID[0] );
	}
	/* If we expect a CCCD handle */
	if ( pxCharacteristic->eCharacteristicProperties & ( BLE_CHARACTERISTIC_PROPERTY_NOTIFY | BLE_CHARACTERISTIC_PROPERTY_INDICATE ) ) {
		eLog( eLogger, eLevel, "\t\tCCCD Handle: %2d\r\n", pxCharacteristic->usCCCDHandle );
	}
}

/*-----------------------------------------------------------*/
