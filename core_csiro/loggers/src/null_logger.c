/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "null_logger.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

static eModuleError_t eConfigure( uint16_t usSetting, void *pvParameters )
{
	switch ( usSetting ) {
		case LOGGER_CONFIG_GET_CLEAR_BYTE:
			*( (uint8_t *) pvParameters ) = 0x00;
			break;
		case LOGGER_CONFIG_GET_NUM_BLOCKS:
			*( (uint32_t *) pvParameters ) = 0;
			break;
		case LOGGER_CONFIG_GET_ERASE_UNIT:
			*( (uint8_t *) pvParameters ) = 0;
			break;
		default:
			break;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eStatus( uint16_t usType )
{
	UNUSED( usType );
	/* No status */
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eReadBlock( uint32_t ulBlockNum, uint16_t usOffset, void *pvBlockData, uint32_t ulBlockSize )
{
	UNUSED( ulBlockNum );
	UNUSED( usOffset );
	UNUSED( pvBlockData );
	UNUSED( ulBlockSize );
	return ERROR_INVALID_LOGGER;
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteBlock( uint32_t ulBlockNum, void *pvBlockData, uint32_t ulBlockSize )
{
	UNUSED( ulBlockNum );
	UNUSED( pvBlockData );
	UNUSED( ulBlockSize );
	return ERROR_INVALID_LOGGER;
}

/*-----------------------------------------------------------*/

static eModuleError_t ePrepareBlock( uint32_t ulBlockNum )
{
	UNUSED( ulBlockNum );
	return ERROR_INVALID_LOGGER;
}

/*-----------------------------------------------------------*/

LOGGER_DEVICE( xNullLoggerDevice, eConfigure, eStatus, eReadBlock, eWriteBlock, ePrepareBlock );

/*-----------------------------------------------------------*/
