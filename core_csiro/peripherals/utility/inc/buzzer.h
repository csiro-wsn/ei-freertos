/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: buzzer.h
 * Creation_Date: 07/02/2019
 * Author: Jonathan Chang <jonathan.chang@data61.csiro.au>
 *
 * 
 * Generic Buzzer implementation
 * 
 */

#ifndef __CSIRO_CORE_ARCH_BUZZER
#define __CSIRO_CORE_ARCH_BUZZER

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "gpio.h"
#include "pwm.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**
 * Setup the buzzer
 * \param pxBuzzer The pwm instance
 */
void vBuzzerInit( xPwmModule_t *pxBuzzerPwm, xGpio_t xEnGpio );

/**
 * Start the buzzer
 * \param ulFrequencyMilliHz The buzzer frequency
 */
void vBuzzerStart( uint32_t ulFrequencyMilliHz );

/**
 * Stop the buzzer
 */
void vBuzzerStop();

#endif /* __CSIRO_CORE_ARCH_BUZZER */
