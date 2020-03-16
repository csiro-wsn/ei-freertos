/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: battery.h
 * Creation_Date: 20/05/2019
 * Author: John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * A file containing all scheduler functions for working with the battery
 * voltage and the conversions to state of charge.
 * 
 */

#ifndef __CSIRO_CORE_BATTERY
#define __CSIRO_CORE_BATTERY

/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/
// clang-format off

/** @brief Enum value for battery types. */
typedef enum eBatteryType_t {
	BATTERY_LIPO,		/**< Lithium Polymer. */
	BATTERY_LMO,		/**< Lithium Manganese Dioxide. */
	BATTERY_LIFEPO4,	/**< Lithium Iron Phosphate. */
} eBatteryType_t;

// clang-format on
/* Function Declarations ------------------------------------*/

/** @brief Sets the battery type and capacity used in the conversion formula.
 * 
 * Sets the battery type of the voltage to state of charge conversion formula.
 * Examples of bins to parse into the arguements can be found in this folder.
 * 
 * @param[in]   eBatteryType    				A battery chemistry.
 * @param[in]	usCapacityInMilliAmpHours		A Battery capacity in mAh (milliamp hour)
 */
void vBatterySetType( eBatteryType_t eBatteryType, uint16_t usCapacityInMilliAmpHours );

/**@brief Gets the current state of charge of the battery.
 * 
 * Takes the current status of the battery, and the bins corresponding to the
 * voltage curve of a specific type of battery, and return the current state of
 * charge of the battery. Returns it as a percentage from 0% to 100% where 0%
 * represents an empty battery, and 100% represents a fully charged battery.
 * 
 * @param[in]   ulBatteryVoltageMV  Battery voltage in millivolts.
 * @param[in]   ulChargeCurrentUA   Charge current in microamps.
 * @param[in]   pulBins             Array of voltage bins. Battery specific. Must have length 11.
 * 
 * @retval 		uint8_t					State of charge in percent from 0 to 100.
 */
uint8_t ucBatteryVoltageToStateOfCharge( uint32_t ulBatteryVoltageMV, uint32_t ulChargeCurrentUA );

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_SCHEDULER_BATTERY */
