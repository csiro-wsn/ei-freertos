/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: tdf-parse.h
 * Creation_Date: 21/12/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Basic C parser of TDF buffers
 * 
 */
#ifndef __CSIRO_CORE_TDF_PARSE
#define __CSIRO_CORE_TDF_PARSE
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "tdf.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef struct xTdfParser_t
{
	uint8_t *  pucBuffer;
	uint32_t   ulBufferLen;
	uint32_t   ulCurrentOffset;
	xTdfTime_t xBufferTime;
} xTdfParser_t;

typedef struct xTdf_t
{
	uint16_t   usId;
	xTdfTime_t xTime;
	uint8_t *  pucData;
	uint8_t	ucDataLen;
} xTdf_t;

/* Function Declarations ------------------------------------*/

void vTdfParseStart( xTdfParser_t *pxParser, uint8_t *pucBuffer, uint32_t ulBufferLen );

eModuleError_t eTdfParse( xTdfParser_t *pxParser, xTdf_t *pxTdf );

#endif /* __CSIRO_CORE_TDF_PARSE */
