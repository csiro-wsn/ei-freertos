/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "board.h"

#include "application_defaults.h"
#include "bluetooth.h"
#include "device_nvm.h"
#include "uart.h"
#include "unified_comms_serial.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/
/*-----------------------------------------------------------*/

ATTR_WEAK fnSerialByteHandler_t fnBoardSerialHandler( void )
{
	return vSerialPacketBuilder;
}

/*-----------------------------------------------------------*/

ATTR_WEAK eModuleError_t eBoardEnablePeripheral( ePeripheral_t ePeripheral, bool *pbPowerApplied, TickType_t xTimeout )
{
	UNUSED( ePeripheral );
	UNUSED( xTimeout );
	/* Peripheral has not changed state */
	if ( pbPowerApplied != NULL ) {
		*pbPowerApplied = false;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vBoardDisablePeripheral( ePeripheral_t ePeripheral )
{
	UNUSED( ePeripheral );
}

/*-----------------------------------------------------------*/

ATTR_WEAK uint32_t ulBoardBatteryVoltageMV( void )
{
	/* Board does not have battery voltage sensing circuitry */
	return 0;
}

/*-----------------------------------------------------------*/

ATTR_WEAK uint32_t ulBoardBatteryChargeUA( void )
{
	/* Board does not have charge current sensing circuitry */
	return 0;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationWaitUntilValidVoltage( void )
{
	/* Voltage good by default */
	return;
}

/*-----------------------------------------------------------*/

ATTR_WEAK void vApplicationWaitUntilActivated( void )
{
	/* Activated by default */
	return;
}

/*-----------------------------------------------------------*/

ATTR_WEAK eModuleError_t eBoardReconfigureFromNvm( eNvmKey_t eKey )
{
	eModuleError_t eError = eApplicationReconfigureFromNvm( eKey );

	if ( eKey == NVM_BLUETOOTH_TX_POWER_DBM ) {
		int32_t lTxPower;
		eError = eNvmReadData( eKey, &lTxPower );
		if ( eError != ERROR_NONE ) {
			return eError;
		}
		cBluetoothSetTxPower( (int8_t) lTxPower );
	}

	return eError;
}

/*-----------------------------------------------------------*/
