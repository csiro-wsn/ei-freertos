/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: i2c_arch.h
 * Creation_Date: 02/08/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Architecture Specific I2C Implementation
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_I2C_ARCH
#define __CORE_CSIRO_INTERFACE_I2C_ARCH

/* Includes -------------------------------------------------*/

#include "em_i2c.h"
#include "gpio.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

struct _xI2CPlatform_t
{
	I2C_TypeDef *pxI2C;
	xGpio_t		 xSda;
	xGpio_t		 xScl;
	uint32_t	 ulLocationSda;
	uint32_t	 ulLocationScl;
};

#define I2C_MODULE_PLATFORM_DEFAULT( NAME, PERIPHERAL ) \
	{                                                   \
		.pxI2C		   = PERIPHERAL,                    \
		.xSda		   = UNUSED_GPIO,                   \
		.xScl		   = UNUSED_GPIO,                   \
		.ulLocationSda = UNUSED_LOCATION,               \
		.ulLocationScl = UNUSED_LOCATION,               \
	}

/* Function Declarations ------------------------------------*/

/**
 * Returns true if I2C module can safely transition to deep sleep mode.
 *
 * When the I2C modules are being used, they require the high frequency clock
 * to be on. This stops us from enterring deep sleep, so this has to be
 * checked before entering deep sleep so I2C transmissions are not interrupted.
 **/
bool bI2CCanDeepSleep( xI2CModule_t *pxModule );

#endif /* __CORE_CSIRO_INTERFACE_I2C_ARCH */
