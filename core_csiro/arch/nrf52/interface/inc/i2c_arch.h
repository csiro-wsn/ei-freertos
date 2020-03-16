/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: i2c_arch.h
 * Creation_Date: 02/08/2018
 * Author: Luke Swift <Luke.Swift@data61.csiro.au>
 * 		   John Scolaro <John.Scolaro@data61.csiro.au>
 *
 * Platform Specific I2C Implementation
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_I2C_ARCH
#define __CORE_CSIRO_INTERFACE_I2C_ARCH

/* Includes -------------------------------------------------*/

#include "nrfx_twim.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define I2C_GPIO_UNUSED \
	{                   \
		255             \
	}

/* Type Definitions -----------------------------------------*/

#define I2C_MODULE_PLATFORM_DEFAULT( NAME, PERIPHERAL ) \
	{                                                   \
		.xInstance = NRFX_TWIM_INSTANCE( PERIPHERAL ),  \
		.xSda	  = I2C_GPIO_UNUSED,                   \
		.xScl	  = I2C_GPIO_UNUSED,                   \
	}

struct _xI2CPlatform_t
{
	nrfx_twim_t xInstance;
	xGpio_t		xSda;
	xGpio_t		xScl;
};

/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_INTERFACE_I2C_ARCH */
