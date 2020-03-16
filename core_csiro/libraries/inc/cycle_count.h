/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: cycle_count.h
 * Creation_Date: 03/04/2019
 * Author: Lachlan Currie <Lachlan.Currie@csiro.au>
 * 
 * How to use:
 *  vInitCycleCount();
 *  volatile int32_t it1,it2;      // start and stop CYCCNT
 *  it1=ulGetCycleCount();
 *  SweetMathsFUNction(magic,numbers,here);
 *  it2 = ulGetCycleCount() - it1;    // calc the cycle-count difference
 *  eLog( LOG_APPLICATION, LOG_INFO,"cycles used by SweetMathsFUNction: %li \n",it2);
 */
#ifndef __CSIRO_CORE_CYCLE_COUNT
#define __CSIRO_CORE_CYCLE_COUNT
/* Includes -------------------------------------------------*/
#include "core_cm4.h"
/* Module Defines -------------------------------------------*/

#define vStartCycleCount() DWT->CTRL |= 1		 // Enable CYCCNT register
#define vStopCycleCount() DWT->CTRL = 0x40000000 // Disable CYCCNT register
#define ulGetCycleCount() DWT->CYCCNT			 // Get value from CYCCNT register
#define vClearCycleCount() DWT->CYCCNT = 0		 // Set the value of the  CYCCNT register
#define vInitCycleCount()                \
	do {                                 \
		ITM->LAR = 0xC5ACCE55;           \
		CoreDebug->DEMCR |= ( 1 << 24 ); \
	} while ( 0 )

#endif /* __CSIRO_CORE_CYCLE_COUNT */
