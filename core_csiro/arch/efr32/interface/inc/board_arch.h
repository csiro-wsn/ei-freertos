/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: board_arch.h
 * Creation_Date: 24/02/2019
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * EFR32 Architecture specific functions which platforms must implement
 * 
 */
#ifndef __CSIRO_CORE_BOARD_ARCH
#define __CSIRO_CORE_BOARD_ARCH
/* Includes -------------------------------------------------*/

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Check if all peripherals are able to enter deep sleep mode */
bool bBoardCanDeepSleep( void );

/* Application level deep-sleep overide to disallow deep sleep */
void vBoardDeepSleepEnabled( bool bEnable );

/* Deinitialise peripherals that dont run in deep sleep mode */
void vBoardDeepSleep( void );

#endif /* __CSIRO_CORE_BOARD_ARCH */
