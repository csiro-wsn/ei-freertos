/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: switch.h
 * Creation_Date: 12/04/2019
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Helper object for mediating access to a shared resource which is controlled by a switch.
 * 
 * Only supports a 2 pole-switch with a single control line.
 * Additional support for switch to be on a dedicated power line
 */
#ifndef __CSIRO_CORE_PERIPHERALS_UTILITY_SWITCH
#define __CSIRO_CORE_PERIPHERALS_UTILITY_SWITCH
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "gpio.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xSwitch_t
{
	xGpio_t			  xEnable;			/**< GPIO to enable the switch (Active-High) */
	xGpio_t			  xControl;			/**< GPIO to control output state of the switch  */
	SemaphoreHandle_t xAccess;			/**< Internal Use Only */
	StaticSemaphore_t xAccessStorage;   /**< Internal Use Only */
	bool			  bHardwareDisable; /**< True when external circuitry disables the switch when the enable pin is floating */
} xSwitch_t;

/* Function Declarations ------------------------------------*/

/**@brief Initialise the Switch Controller
 * 
 * @param[in]  pxSwitch			Switch to initialise
 */
void vSwitchInit( xSwitch_t *pxSwitch );

/**@brief Request control over the switch
 * 
 *  Upon a successful call, the peripheral can now be used as if it were always connected.
 * 	This function is intended to abstract away board specific circuitry not directly connected to the peripheral.
 * 
 * @param[in] pxSwitch			Switch to control
 * @param[in] bControlState		Desired output state of the control line
 * @param[in] xTimeout			How long to wait for control over switch
 * 
 * @retval::ERROR_NONE			Swtich was successfully enabled and control line is correctly driven
 * @retval::ERROR_TIMEOUT		Switch is in use by another peripheral
 */
eModuleError_t eSwitchRequest( xSwitch_t *pxSwitch, bool bControlState, TickType_t xTimeout );

/**@brief Switch to release control of
 * 
 * @param[in] pxSwitch			Switch to relinquish control over
 */
void vSwitchRelease( xSwitch_t *pxSwitch );

#endif /* __CSIRO_CORE_PERIPHERALS_UTILITY_SWITCH */
