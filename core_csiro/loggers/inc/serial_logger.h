/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: serial_logger.h
 * Creation_Date: 17/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Serial Log interface for pushing TDF's over serial
 * 
 */
#ifndef __CSIRO_CORE_SERIAL_LOGGER
#define __CSIRO_CORE_SERIAL_LOGGER
/* Includes -------------------------------------------------*/

#include "logger.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Variable Declarations ------------------------------------*/

extern const xLoggerDevice_t xSerialLoggerDevice;

#endif /* __CSIRO_CORE_SERIAL_LOGGER */
