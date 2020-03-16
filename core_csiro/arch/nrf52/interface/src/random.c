/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "random.h"

#include "nrf_soc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t eRandomGenerate( uint8_t *pucRandomData, uint8_t ucRandomDataLen )
{
	uint32_t ulError;
	ulError = sd_rand_application_vector_get( pucRandomData, ucRandomDataLen );
	return ( ulError == NRF_SUCCESS ) ? ERROR_NONE : ERROR_INVALID_DATA;
}

/*-----------------------------------------------------------*/
