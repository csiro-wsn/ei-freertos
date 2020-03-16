/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include <stdbool.h>

#include "device_constants.h"
#include "memory_operations.h"

#include "cpu.h"
#include "nrf_sdm.h"
#include "watchdog.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

bool bDeviceConstantsRead( xDeviceConstants_t *pxDeviceConstants )
{
	pvMemcpy( (void *) pxDeviceConstants, (void *) NRF_UICR->CUSTOMER, sizeof( xDeviceConstants_t ) );
	return ( pxDeviceConstants->ulKey == DEVICE_CONSTANTS_KEY );
}

/*-----------------------------------------------------------*/

eModuleError_t eDeviceConstantsOneTimeProgram( uint8_t ucOffset, uint8_t *pucData, uint8_t ucDataLength )
{
	volatile uint32_t *pulOutputAddress = NRF_UICR->CUSTOMER + ( ucOffset / 4 );
	/* Validate desired write bytes are 0xFF */
	uint8_t *pucUICRBytes = (uint8_t *) NRF_UICR->CUSTOMER;
	for ( uint8_t i = 0; i < ucDataLength; i++ ) {
		if ( pucUICRBytes[ucOffset + i] != 0xFF ) {
			return ERROR_INVALID_ADDRESS;
		}
	}
	/* Increase the 8 if larger writes are needed in the future */
	configASSERT( ucDataLength / 4 <= 8 );
	uint32_t pulTemporaryArray[8] = { [0 ... 7] = 0xFFFFFFFF };
	uint8_t *pucTemporaryBytes	= (uint8_t *) pulTemporaryArray;
	pvMemcpy( pucTemporaryBytes + ( ucOffset % 4 ), pucData, ucDataLength );

	/* Update our reboot reason here before we disable the softdevice */
	vWatchdogSetRebootReason( REBOOT_RPC, (char *) "OTP", __get_PC(), __get_LR() );

	/* Disable the softdevice before attempting to write to the UICR */
	if ( sd_softdevice_disable() != NRF_SUCCESS ) {
		return ERROR_GENERIC;
	}
	__disable_irq();

	/* Enable writing to flash */
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;

	/* Write the UICR values */
	uint8_t ucNumWords = ROUND_UP( ucDataLength, 4 ) / 4;
	for ( uint8_t i = 0; i < ucNumWords; i++ ) {
		pulOutputAddress[i] = pulTemporaryArray[i];

		/* Wait until the NVMC controller is ready*/
		while ( NRF_NVMC->READY == NVMC_READY_READY_Busy ) {
			;
		}
	}

	vSystemReboot();
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
