/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stddef.h>

#include "FreeRTOS.h"

#include "battery.h"

#include "csiro_math.h"
#include "misc_encodings.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Private Variables ----------------------------------------*/

static const uint32_t *pulBins = NULL;
static uint16_t		   usCapacityInMilliAmpHours;

/* Battery Declarations -------------------------------------*/
// clang-format off

/* Battery Charge Levels          ---         { 0%,   10%,  20%,  30%,  40%,  50%,  60%,  70%,  80%,  90%,  100% } */
static const uint32_t pulBatteryLiFePO4[11] = { 3001, 3162, 3194, 3223, 3239, 3248, 3255, 3267, 3284, 3288, 3306 };
static const uint32_t pulBatteryLiPo[11]	= { 2900, 3468, 3540, 3596, 3632, 3668, 3729, 3838, 3927, 4037, 4150 };
static const uint32_t pulBatteryLMO[11]     = { 2800, 2825, 2850, 2875, 2900, 2925, 2950, 2975, 2995, 3000, 3050 }; // Still need to be experimentally verified.

// clang-format on
/* Private Functions ----------------------------------------*/

uint8_t prvGetStateOfChargeFromBins( const uint32_t *pulBins, uint32_t ulBatteryVoltageMV, uint32_t ulChargeCurrentUA );

/* Private Functions ----------------------------------------*/

void vBatterySetType( eBatteryType_t eBatteryType, uint16_t usBatteryCapacityInMilliAmpHours )
{
	switch ( eBatteryType ) {
		case BATTERY_LIPO:
			pulBins = pulBatteryLiPo;
			break;
		case BATTERY_LIFEPO4:
			pulBins = pulBatteryLiFePO4;
			break;
		case BATTERY_LMO:
			pulBins = pulBatteryLMO;
			break;
		default:
			pulBins = pulBatteryLiPo;
			break;
	}

	usCapacityInMilliAmpHours = usBatteryCapacityInMilliAmpHours;
}

/*-----------------------------------------------------------*/

uint8_t ucBatteryVoltageToStateOfCharge( uint32_t ulBatteryVoltageMV, uint32_t ulChargeCurrentUA )
{
	return prvGetStateOfChargeFromBins( pulBins, ulBatteryVoltageMV, ulChargeCurrentUA );
}

/*-----------------------------------------------------------*/

/**
 * This function exists solely so I can unit test it with different sets of
 * bins, other than the ones defined above, and without unit testing bins
 * being compiled into release apps.
 **/
uint8_t prvGetStateOfChargeFromBins( const uint32_t *pulBins, uint32_t ulBatteryVoltageMV, uint32_t ulChargeCurrentUA )
{
	/**
	 * Note: Currently this function doesn't take into account the charging
	 * current, so if the battery voltage is low, but currently charging,
	 * it is going to look like it is sitting at 100%. This is wrong and needs
	 * to be changed eventually.
	 **/
	UNUSED( ulChargeCurrentUA );
	configASSERT( pulBins );

	uint8_t ucIndex = ucBinIndexLong( ulBatteryVoltageMV, pulBins, 11 );

	/* If below the 0% bin */
	if ( ucIndex == 0 ) {
		return 0;
	}

	/* If above the 100% bin */
	if ( ucIndex == 11 ) {
		return 100;
	}

	/* If between 0% and 100% */
	return ( ( ucIndex - 1 ) * 10 ) + ( ulValueBin( ulBatteryVoltageMV, pulBins[ucIndex - 1], pulBins[ucIndex], 100 ) / 10 );
}

/*-----------------------------------------------------------*/
