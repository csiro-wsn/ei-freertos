/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "crc.h"

#include "em_cmu.h"
#include "em_gpcrc.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

STATIC_SEMAPHORE_STRUCTURES( pxCrcAccess );

/*-----------------------------------------------------------*/

void vCrcInit( void )
{
	CMU_ClockEnable( cmuClock_GPCRC, true );
	STATIC_SEMAPHORE_CREATE_MUTEX( pxCrcAccess );
}

/*-----------------------------------------------------------*/

void vCrcStart( eCrcPolynomial_t ePolynomial, uint32_t ulInitValue )
{
	GPCRC_Init_TypeDef xInit = {
		.crcPoly		  = 0x00,
		.initValue		  = ulInitValue,
		.reverseByteOrder = false,
		.reverseBits	  = true,
		.enableByteMode   = false,
		.autoInit		  = false,
		.enable			  = true,
	};
	/* Load the appropriate Polynomial */
	switch ( ePolynomial ) {
		case CRC32_IEEE_802_3:
			xInit.crcPoly = 0x04C11DB7;
			break;
		case CRC16_CCITT:
			xInit.crcPoly = 0x1021;
			break;
		case CRC16_IEC16_MBUS:
			xInit.crcPoly = 0x3D65;
			break;
		case CRC16_ZIGBEE:
		case CRC16_802_15_4:
		case CRC16_USB:
			xInit.crcPoly = 0x8005;
			break;
		default:
			configASSERT( 0 );
	}
	/* Claim the CRC hardware */
	xSemaphoreTake( pxCrcAccess, portMAX_DELAY );
	/* Setup the CRC hardware */
	GPCRC_Init( GPCRC, &xInit );
	GPCRC_Start( GPCRC );
}

/*-----------------------------------------------------------*/

uint32_t ulCrcCalculate( uint8_t *pucData, uint32_t ulDataLen, bool bTerminate )
{
	uint32_t ulCrc;

	/* Iterate through the provided data */
	for ( uint32_t i = 0; i < ulDataLen; i++ ) {
		GPCRC_InputU8( GPCRC, pucData[i] );
	}
	/* Get the current CRC value */
	ulCrc = GPCRC_DataReadBitReversed( GPCRC );
	/* If we're done, give the semaphore back */
	if ( bTerminate ) {
		xSemaphoreGive( pxCrcAccess );
	}
	return ulCrc;
}

/*-----------------------------------------------------------*/
