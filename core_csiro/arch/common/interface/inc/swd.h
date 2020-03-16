/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: rtt.h
 * Creation_Date: 17/4/2019
 * Author: Mia Balogh
 *
 * FreeRTOS abstraction for SEGGER_RTT_vprintf 
 *
 */

#ifndef __CORE_CSIRO_LIBRARIES_SWD_PRINTF
#define __CORE_CSIRO_LIBRARIES_SWD_PRINTF

/* Includes -------------------------------------------------*/

#include "serial_interface.h"

/* Forward Declarations -------------------------------------*/

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void vSwdInit( void );

/* External Variables ------------------------------------*/

extern xSerialBackend_t xSwdBackend;

#endif /* __CORE_CSIRO_LIBRARIES_SWD_PRINTF */
