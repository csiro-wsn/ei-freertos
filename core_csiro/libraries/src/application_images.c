/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "application_images.h"

#include "board.h"
#include "device_constants.h"
#include "log.h"
#include "rom.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

uint32_t ulNumApplicationImages( void )
{
	return xDeviceConstants.ucApplicationImageSlots;
}

/*-----------------------------------------------------------*/

uint32_t ulApplicationImageSize( void )
{
	xDeviceRomConfiguration_t xDeviceRom;
	vRomConfigurationQuery( &xDeviceRom );
	return xDeviceRom.ulRomPageSize * xDeviceRom.ulRomPages;
}

/*-----------------------------------------------------------*/

void *pvApplicationBuildInfoOffset( void )
{
	/**
	 * NRF52:
	 * 		__buildinfo_start exists behind the softdevice and and our ISR vectors
	 * 		It will be consistent across all application where the softdevice has the same size
	 * 		Provided data will be junk otherwise ( validate retrieved ulKey )
	 * 
	 * EFR32:
	 * 		__buildinfo_start exists behind the ISR vectors
	 * 		It will be consistent across all applications
	 */
	extern unsigned char __buildinfo_start[];
	return (void *) __buildinfo_start;
}

/*-----------------------------------------------------------*/

xBuildInfo_t *pxLocalBuildInfo( void )
{
	return (xBuildInfo_t *) pvApplicationBuildInfoOffset();
}

/*-----------------------------------------------------------*/
