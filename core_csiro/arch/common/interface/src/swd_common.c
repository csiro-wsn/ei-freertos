/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "swd.h"

#include "SEGGER_RTT.h"
#include "compiler_intrinsics.h"
#include "memory_pool.h"
#include "tiny_printf.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

void		   vSwdOn( void *pvContext );
void		   vSwdOff( void *pvContext );
eModuleError_t eSwdWrite( void *pvContext, const char *pcFormat, va_list pvArgs );
char *		   pcSwdClaimBuffer( void *pvContext, uint32_t *pulBufferLen );
void		   vSwdSendBuffer( void *pvContext, const char *pcBuffer, uint32_t ulBufferLen );
void		   vSwdReleaseBuffer( void *pvContext, char *pucBuffer );

/* Private Variables ----------------------------------------*/

xSerialBackend_t xSwdBackend = {
	.fnEnable		 = vSwdOn,
	.fnDisable		 = vSwdOff,
	.fnWrite		 = eSwdWrite,
	.fnClaimBuffer   = pcSwdClaimBuffer,
	.fnSendBuffer	= vSwdSendBuffer,
	.fnReleaseBuffer = vSwdReleaseBuffer
};

/* Static memory pool as there can only be one SWD instance in a system */
MEMORY_POOL_CREATE( SWD_POOL, 4, SERIAL_INTERFACE_DEFAULT_SIZE );
static xMemoryPool_t *const pxSwdPool = &MEMORY_POOL_GET( SWD_POOL );

/*-----------------------------------------------------------*/

void vSwdInit( void )
{
	vMemoryPoolInit( pxSwdPool );
}

/*-----------------------------------------------------------*/

void vSwdOn( void *pvContext )
{
	UNUSED( pvContext );
}

/*-----------------------------------------------------------*/

void vSwdOff( void *pvContext )
{
	UNUSED( pvContext );
}

/*-----------------------------------------------------------*/

eModuleError_t eSwdWrite( void *pvContext, const char *pcFormat, va_list pvArgs )
{
	UNUSED( pvContext );
	uint8_t  ucChannel	= 0;
	char *   pcBuffer	 = (char *) pcMemoryPoolClaim( pxSwdPool, portMAX_DELAY );
	uint32_t ulNumBytes   = tiny_vsnprintf( pcBuffer, pxSwdPool->ulBufferSize, pcFormat, pvArgs );
	int32_t  lBytesStored = SEGGER_RTT_Write( ucChannel, pcBuffer, ulNumBytes );
	vMemoryPoolRelease( pxSwdPool, (int8_t *) pcBuffer );
	return lBytesStored < 0 ? ERROR_GENERIC : ERROR_NONE;
}

/*-----------------------------------------------------------*/

char *pcSwdClaimBuffer( void *pvContext, uint32_t *pulBufferLen )
{
	UNUSED( pvContext );
	*pulBufferLen = pxSwdPool->ulBufferSize;
	return (char *) pcMemoryPoolClaim( pxSwdPool, portMAX_DELAY );
}

/*-----------------------------------------------------------*/

void vSwdSendBuffer( void *pvContext, const char *pcBuffer, uint32_t ulBufferLen )
{
	UNUSED( pvContext );
	uint8_t ucChannel = 0;
	SEGGER_RTT_Write( ucChannel, pcBuffer, ulBufferLen );
	vMemoryPoolRelease( pxSwdPool, (int8_t *) pcBuffer );
}

/*-----------------------------------------------------------*/

void vSwdReleaseBuffer( void *pvContext, char *pcBuffer )
{
	UNUSED( pvContext );
	vMemoryPoolRelease( pxSwdPool, (int8_t *) pcBuffer );
}

/*-----------------------------------------------------------*/
