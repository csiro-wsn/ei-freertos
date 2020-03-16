/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: null_logger.h
 * Creation_Date: 15/05/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Empty logger for when a given logger doesn't exist
 * 
 */
#ifndef __CSIRO_CORE_NULL_LOGGER
#define __CSIRO_CORE_NULL_LOGGER
/* Includes -------------------------------------------------*/

#include "logger.h"

/* External Variables ----------------------------------------*/

extern const xLoggerDevice_t xNullLoggerDevice;

#endif /* __CSIRO_CORE_NULL_LOGGER */
