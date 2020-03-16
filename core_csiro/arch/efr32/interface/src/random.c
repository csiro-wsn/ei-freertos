/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "random.h"

#include "log.h"
#include "memory_operations.h"
#include "rtos_gecko.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t eRandomGenerate( uint8_t *pucRandomData, uint8_t ucRandomDataLen )
{
	struct gecko_msg_system_get_random_data_rsp_t *pxResp;
	pxResp = gecko_cmd_system_get_random_data( ucRandomDataLen );

	if ( pxResp->result != 0 ) {
		eLog( LOG_BLUETOOTH_GAP, LOG_APOCALYPSE, "BT: Failed to get random 0x%X\r\n", pxResp->result );
		return ERROR_INVALID_DATA;
	}
	pvMemcpy( pucRandomData, pxResp->data.data, ucRandomDataLen );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
