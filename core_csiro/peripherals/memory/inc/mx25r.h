/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: mx25r.h
 * Creation_Date: 09/09/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * MX25R Implementation of flash_interface.h
 * 
 */
#ifndef __CSIRO_CORE_W25X_GENERIC
#define __CSIRO_CORE_W25X_GENERIC
/* Includes -------------------------------------------------*/

#include "flash_interface.h"

#include "gpio.h"
#include "spi.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xMX25rHardware_t
{
	xSpiModule_t *pxInterface;
	xSpiConfig_t  xSpiConfig;
} xMX25rHardware_t;

/* Function Declarations ------------------------------------*/

extern xFlashImplementation_t xMX25rDriver;

#endif /* __CSIRO_CORE_W25X_GENERIC */
