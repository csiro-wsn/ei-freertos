/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "switch.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vSwitchInit( xSwitch_t *pxSwitch )
{
	uint8_t ucEnableMode, ucEnableState;
	pxSwitch->xAccess = xSemaphoreCreateBinaryStatic( &pxSwitch->xAccessStorage );
	xSemaphoreGive( pxSwitch->xAccess );

	ucEnableMode  = pxSwitch->bHardwareDisable ? GPIO_DISABLED : GPIO_PUSHPULL;
	ucEnableState = pxSwitch->bHardwareDisable ? GPIO_DISABLED_NOPULL : GPIO_PUSHPULL_LOW;
	vGpioSetup( pxSwitch->xEnable, ucEnableMode, ucEnableState );
	vGpioSetup( pxSwitch->xControl, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
}

/*-----------------------------------------------------------*/

eModuleError_t eSwitchRequest( xSwitch_t *pxSwitch, bool bControlState, TickType_t xTimeout )
{
	if ( xSemaphoreTake( pxSwitch->xAccess, xTimeout ) != pdPASS ) {
		return ERROR_TIMEOUT;
	}
	vGpioSetup( pxSwitch->xEnable, GPIO_PUSHPULL, GPIO_PUSHPULL_HIGH );
	vGpioSetup( pxSwitch->xControl, GPIO_PUSHPULL, bControlState );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vSwitchRelease( xSwitch_t *pxSwitch )
{
	uint8_t ucEnableMode, ucEnableState;
	ucEnableMode  = pxSwitch->bHardwareDisable ? GPIO_DISABLED : GPIO_PUSHPULL;
	ucEnableState = pxSwitch->bHardwareDisable ? GPIO_DISABLED_NOPULL : GPIO_PUSHPULL_LOW;
	vGpioSetup( pxSwitch->xEnable, ucEnableMode, ucEnableState );
	vGpioSetup( pxSwitch->xControl, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	xSemaphoreGive( pxSwitch->xAccess );
}

/*-----------------------------------------------------------*/
