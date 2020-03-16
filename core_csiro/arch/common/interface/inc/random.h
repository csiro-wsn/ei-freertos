/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: random.h
 * Creation_Date: 25/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Generating random numbers from TRNG sources
 * 
 */
#ifndef __CSIRO_CORE_RANDOM
#define __CSIRO_CORE_RANDOM
/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**
 * Generate random data from hardware sources
 * 
 * \param   pucRandomData     Buffer to store random data in
 * \param   ucRandomDataLen   Amount of random data to generate
 * \return  ERROR_INVALID_DATA if generation failed
 *          ERROR_NONE on success
 */
eModuleError_t eRandomGenerate( uint8_t *pucRandomData, uint8_t ucRandomDataLen );

#endif /* __CSIRO_CORE_RANDOM */
