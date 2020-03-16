/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: fpu_handler.h
 * Creation_Date: 29/11/2019
 * Author: Anton Schieber <anton.schieber@data61.csiro.au>
 *
 * Handler to clear FPSCR to allow overflow that was occuring in prvCalulateLocation (Bleat-Localisation-API)
 * https://devzone.nordicsemi.com/f/nordic-q-a/12433/fpu-divide-by-0-and-high-current-consumption/47063#47063
 *
 */
#ifndef __CSIRO_CORE_FPU_HANDLER
#define __CSIRO_CORE_FPU_HANDLER
/* Includes -------------------------------------------------*/

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* External Variables ---------------------------------------*/

/* Function Declarations ------------------------------------*/
void vInitFPU( void );

#endif /* __CSIRO_CORE_FPU_HANDLER */
