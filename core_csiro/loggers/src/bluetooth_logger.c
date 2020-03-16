/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "bluetooth_logger.h"

#include "board.h"

#include "unified_comms.h"
#include "unified_comms_bluetooth.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* Functions ------------------------------------------------*/

static eModuleError_t eConfigure( uint16_t usSetting, void *pvParameters )
{
	UNUSED( pvParameters );
	switch ( usSetting ) {
		case LOGGER_CONFIG_GET_CLEAR_BYTE:
			*( (uint8_t *) pvParameters ) = 0x00;
			break;
		case LOGGER_CONFIG_GET_NUM_BLOCKS:
			*( (uint32_t *) pvParameters ) = UINT32_MAX;
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
	/* Output logger only */
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteBlock( uint32_t ulBlockNum, void *pvBlockData, uint32_t ulBlockSize )
{
	UNUSED( ulBlockNum );
	configASSERT( ulBlockSize != 0 );
	configASSERT( pvBlockData != NULL );

	xUnifiedCommsMessage_t xMessage = {
		.xSource	  = LOCAL_ADDRESS,
		.xDestination = BROADCAST_ADDRESS,
		.xPayloadType = UNIFIED_MSG_PAYLOAD_TDF3,
		.pucPayload   = pvBlockData,
		.usPayloadLen = (uint16_t) ulBlockSize
	};
	return xBluetoothComms.fnSend( COMMS_CHANNEL_DEFAULT, &xMessage );
}

/*-----------------------------------------------------------*/

static eModuleError_t ePrepareBlock( uint32_t ulBlockNum )
{
	UNUSED( ulBlockNum );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

LOGGER_DEVICE( xBluetoothLoggerDevice, eConfigure, eStatus, eReadBlock, eWriteBlock, ePrepareBlock );

/*-----------------------------------------------------------*/
