/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: temp.h
 * Creation_Date: 18/04/2019
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * Defines the interface for getting the CPU temperature.
 * 
 */

#ifndef __CSIRO_CORE_INTERFACE_TEMP
#define __CSIRO_CORE_INTERFACE_TEMP

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

/* Forward Declarations -------------------------------------*/

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**
 * A function to initialise the temperature module with everything it needs to
 * function correctly.
 **/
void vTempInit( void );

/**
 * A function returning the temperature in millidegrees celcius.
 **/
eModuleError_t eTempMeasureMilliDegrees( int32_t *plTemperature );

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_INTERFACE_TEMP */
