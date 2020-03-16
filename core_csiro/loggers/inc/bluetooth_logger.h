/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: bluetooth_logger.h
 * Creation_Date: 24/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Bluetooth Log interface for TDF's
 * 
 */
#ifndef __CSIRO_CORE_BLUETOOTH_LOGGER
#define __CSIRO_CORE_BLUETOOTH_LOGGER
/* Includes -------------------------------------------------*/

#include "logger.h"

#include "unified_comms_bluetooth.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

extern const xLoggerDevice_t xBluetoothLoggerDevice;

#endif /* __CSIRO_CORE_BLUETOOTH_LOGGER */
