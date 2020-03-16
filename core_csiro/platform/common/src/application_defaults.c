/*
 * Copyright (c) 2019, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "application_defaults.h"

#include "compiler_intrinsics.h"
#include "unified_comms.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationSetLogLevels( void )
{
	return;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationStartupCallback( void )
{
	return;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationTickCallback( uint32_t ulUptime )
{
	UNUSED( ulUptime );
	return;
}

/*-----------------------------------------------------------*/

ATTR_WEAK eModuleError_t eApplicationReconfigureFromNvm( eNvmKey_t eKey )
{
	UNUSED( eKey );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattConnected( xBluetoothConnection_t *pxConnection )
{
	UNUSED( pxConnection );
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattDisconnected( xBluetoothConnection_t *pxConnection )
{
	UNUSED( pxConnection );
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattLocalCharacterisiticSubscribed( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic )
{
	UNUSED( pxConnection );
	UNUSED( pxUpdatedCharacteristic );
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattLocalCharacterisiticWritten( xBluetoothConnection_t *pxConnection, xGattLocalCharacteristic_t *pxUpdatedCharacteristic )
{
	UNUSED( pxConnection );
	UNUSED( pxUpdatedCharacteristic );
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattRemoteCharacterisiticChanged( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic )
{
	UNUSED( pxConnection );
	UNUSED( pxUpdatedCharacteristic );
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationGattRemoteCharacterisiticRead( xBluetoothConnection_t *pxConnection, xGattRemoteCharacteristic_t *pxUpdatedCharacteristic )
{
	UNUSED( pxConnection );
	UNUSED( pxUpdatedCharacteristic );
}

/*-----------------------------------------------------------*/

ATTR_WEAK bool bUnifiedCommsEncryptionKey( xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xDestination, uint8_t *ppucEncryptionKey[16] )
{
	UNUSED( pxInterface );
	UNUSED( eType );
	UNUSED( xDestination );
	UNUSED( ppucEncryptionKey );
	return false;
}

/*-----------------------------------------------------------*/

ATTR_WEAK bool bUnifiedCommsDecryptionKey( xCommsInterface_t *pxInterface, eCsiroPayloadType_t eType, xAddress_t xDestination, uint8_t *ppucDecryptionKey[16] )
{
	UNUSED( pxInterface );
	UNUSED( eType );
	UNUSED( xDestination );
	UNUSED( ppucDecryptionKey );
	return false;
}

/*-----------------------------------------------------------*/