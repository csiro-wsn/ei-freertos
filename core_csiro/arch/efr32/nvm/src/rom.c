/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "rom.h"

#include "em_device.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vRomConfigurationQuery( xDeviceRomConfiguration_t *pxConfiguration )
{
	/* Page Size = 2 ^ ((MEMINFO_FLASH_PAGE_SIZE + 10) & 0xFF) */
	uint32_t ulMeminfoFlashPageSize = MASK_READ( DEVINFO->MEMINFO, _DEVINFO_MEMINFO_FLASH_PAGE_SIZE_MASK );
	/* Total Flash = MSIZE_FLASH * 1024 */
	uint32_t ulMsizeFlashSize = MASK_READ( DEVINFO->MSIZE, _DEVINFO_MSIZE_FLASH_MASK );
	/* Apply conversions */
	pxConfiguration->ulRomPageSize = 0x1 << ( ( ulMeminfoFlashPageSize + 10 ) & 0xFF );
	pxConfiguration->ulRomPages	= ( ulMsizeFlashSize * 1024 ) / pxConfiguration->ulRomPageSize;
	pxConfiguration->ucEraseByte   = 0xFF;
}

/*-----------------------------------------------------------*/
