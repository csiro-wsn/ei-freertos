/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: usb.h
 * Creation_Date: 13/2/2020
 * Author: Jordan Yates
 *
 * FreeRTOS abstraction for USB devices
 *
 */

#ifndef __CORE_CSIRO_LIBRARIES_USB
#define __CORE_CSIRO_LIBRARIES_USB

/* Includes -------------------------------------------------*/

#include "serial_interface.h"

/* Forward Declarations -------------------------------------*/

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vUsbInit(void);

void vUsbSetByteHandler(fnSerialByteHandler_t fnHandler);

/* External Variables ------------------------------------*/

extern xSerialBackend_t xUsbBackend;

#endif /* __CORE_CSIRO_LIBRARIES_USB */
