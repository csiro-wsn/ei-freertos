/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "rom.h"

#include "nrf52.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vRomConfigurationQuery( xDeviceRomConfiguration_t *pxConfiguration )
{
	pxConfiguration->ulRomPageSize = NRF_FICR->CODEPAGESIZE;
	pxConfiguration->ulRomPages	= NRF_FICR->CODESIZE;
	pxConfiguration->ucEraseByte   = 0xFF;
}

/*-----------------------------------------------------------*/
