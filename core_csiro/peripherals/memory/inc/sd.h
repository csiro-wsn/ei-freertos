/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: sd.h
 * Creation_Date: 6/9/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * SD card driver.
 * 
 * Author: Jordan Yates
 * 
 */

#ifndef __CORE_CSIRO_MEMORY_SD
#define __CORE_CSIRO_MEMORY_SD

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "spi.h"

#include "sd_ll.h"

/* Module Defines -------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* The initialisation struct for the SD card. */
typedef struct xSdInit_t
{
	xSpiModule_t *pxSpi;
	xGpio_t		  xChipSelect;
} xSdInit_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eSdInit( xSdInit_t *pxInit );
eModuleError_t eSdParameters( xSdParameters_t *pxParameters );
eModuleError_t eSdBlockRead( uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint16_t usBufferLen, TickType_t xTimeout );
eModuleError_t eSdBlockWrite( uint32_t ulBlockAddress, uint16_t usBlockOffset, uint8_t *pucBuffer, uint16_t usBufferLen, TickType_t xTimeout );
eModuleError_t eSdEraseBlocks( uint32_t ulFirstBlockAddress, uint32_t ulLastBlockAddress, TickType_t xTimeout );

#endif /* __CORE_CSIRO_MEMORY_SD */
