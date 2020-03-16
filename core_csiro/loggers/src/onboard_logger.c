/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "onboard_logger.h"

#include "board.h"
#include "flash_interface.h"

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
			*( (uint8_t *) pvParameters ) = pxOnboardFlash->xSettings.ucEraseByte;
			break;
		case LOGGER_CONFIG_GET_NUM_BLOCKS:
			*( (uint32_t *) pvParameters ) = pxOnboardFlash->xSettings.ulNumPages;
			break;
		case LOGGER_CONFIG_GET_ERASE_UNIT:
			*( (uint8_t *) pvParameters ) = pxOnboardFlash->xSettings.usErasePages;
			break;
		default:
			break;
	}
	return 0;
}

/*-----------------------------------------------------------*/

static eModuleError_t eStatus( uint16_t usType )
{
	UNUSED( usType );
	return 0;
}

/*-----------------------------------------------------------*/

static eModuleError_t eReadBlock( uint32_t ulBlockNum, uint16_t usOffset, void *pvBlockData, uint32_t ulBlockSize )
{
	uint64_t ullAddress = ( (uint64_t) ulBlockNum << pxOnboardFlash->xSettings.ucPageSizePower ) + usOffset;
	return eFlashRead( pxOnboardFlash, ullAddress, pvBlockData, ulBlockSize, pdMS_TO_TICKS( 1000 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteBlock( uint32_t ulBlockNum, void *pvBlockData, uint32_t ulBlockSize )
{
	uint64_t ullAddress = ( (uint64_t) ulBlockNum << pxOnboardFlash->xSettings.ucPageSizePower );
	return eFlashWrite( pxOnboardFlash, ullAddress, pvBlockData, ulBlockSize, pdMS_TO_TICKS( 1000 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t ePrepareBlock( uint32_t ulBlockNum )
{
	uint64_t ullAddress  = ( (uint64_t) ulBlockNum << pxOnboardFlash->xSettings.ucPageSizePower );
	uint32_t ulEraseSize = pxOnboardFlash->xSettings.usErasePages << pxOnboardFlash->xSettings.ucPageSizePower;

	/* If the block is the start of an erase boundary, erase if */
	if ( ulBlockNum % pxOnboardFlash->xSettings.usErasePages == 0 ) {
		return eFlashErase( pxOnboardFlash, ullAddress, ulEraseSize, pdMS_TO_TICKS( 1000 ) );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

LOGGER_DEVICE( xOnboardLoggerDevice, eConfigure, eStatus, eReadBlock, eWriteBlock, ePrepareBlock );

/*-----------------------------------------------------------*/
