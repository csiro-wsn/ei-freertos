/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include <stdbool.h>

#include "device_constants.h"

#include "em_device.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

bool bDeviceConstantsRead( xDeviceConstants_t *pxDeviceConstants )
{
	pvMemcpy( (void *) pxDeviceConstants, (void *) USERDATA_BASE, sizeof( xDeviceConstants_t ) );
	return ( pxDeviceConstants->ulKey == DEVICE_CONSTANTS_KEY );
}

/*-----------------------------------------------------------*/

eModuleError_t eDeviceConstantsOneTimeProgram( uint8_t ucOffset, uint8_t *pucData, uint8_t ucDataLength )
{
	UNUSED( ucOffset );
	UNUSED( pucData );
	UNUSED( ucDataLength );
	return ERROR_GENERIC;
}
/*-----------------------------------------------------------*/
