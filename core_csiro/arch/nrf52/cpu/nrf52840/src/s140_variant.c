/*
 * Copyright (c) 2019, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 * 
 * The functions defined in this file are specific to the softdevice of the cpu 
 * that this file was created for. 
 * 
 * Currently, the only functionality is rounding a given transmit power to the 
 * nearest valid value supported by the softdevice in use.
 *
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Private Defines ------------------------------------------*/
// clang-format off

#define VALID_TX_POWER_NUMBER 14

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* Supported transmit power levels in dBm for s140 */
static int8_t  pcValidTxPower[VALID_TX_POWER_NUMBER] = { -40, -20, -16, -12, -8, -4, 0, 2, 3, 4, 5, 6, 7, 8 };
static uint8_t ucMaxValidTxPower					 = 8;
static int8_t  cMinvalidTxPower						 = -40;

/*-----------------------------------------------------------*/
// TODO: Replace this and s32_variant.c with a generic roundup function in misc_encoding

int8_t cBluetoothStackGetValidTxPower( int8_t cRequestedPowerDbm )
{
	int8_t cNearestValidTxPower = 0;

	/* Address 3 possible cases */
	if ( cRequestedPowerDbm >= ucMaxValidTxPower ) { /* new tx power is equal to or larger than the maximum allowable level */
		cNearestValidTxPower = ucMaxValidTxPower;
	}
	else if ( cRequestedPowerDbm <= cMinvalidTxPower ) { /* new tx power is equal to or smaller than the minimum allowable level */
		cNearestValidTxPower = cMinvalidTxPower;
	}
	else { /* new tx power is is a value between the minumum and maximum allowable levels */

		uint8_t ucNearestValidTxPowerIndex = 0;
		// largest possible distance between new tx value and the supported values
		uint8_t ucTxPowerDistance = ( cRequestedPowerDbm - cMinvalidTxPower ) < 0 ? ( cMinvalidTxPower - cRequestedPowerDbm ) : ( cRequestedPowerDbm - cMinvalidTxPower );

		for ( uint8_t i = 0; i < VALID_TX_POWER_NUMBER; i++ ) {
			uint8_t ucAbsDiff		   = ( cRequestedPowerDbm - pcValidTxPower[i] ) < 0 ? ( pcValidTxPower[i] - cRequestedPowerDbm ) : ( cRequestedPowerDbm - pcValidTxPower[i] );
			ucNearestValidTxPowerIndex = ucAbsDiff < ucTxPowerDistance ? i : ucNearestValidTxPowerIndex;
			ucTxPowerDistance		   = ucAbsDiff < ucTxPowerDistance ? ucAbsDiff : ucTxPowerDistance;
		}
		cNearestValidTxPower = pcValidTxPower[ucNearestValidTxPowerIndex];
	}

	return cNearestValidTxPower;
}
