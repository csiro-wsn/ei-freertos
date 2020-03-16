/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "w25x.h"

#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define W25X_PAGE_SIZE			256

#define NO_ADDRESS 0xFFFFFFFFul

/* Part specific constants */
#define W25X_DEVICE_ID_W25X20CL			0x11
#define W25X_DEVICE_ID_W25X40CL			0x12
#define W25X_DEVICE_ID_W25Q64FV			0x16
#define W25X_DEVICE_ID_W25Q128JV		0x17

/* General size defines */
#define W25X_NUM_PAGES_2Mb		1024
#define W25X_NUM_PAGES_4Mb		2048
#define W25X_NUM_PAGES_64Mb		32768
#define W25X_NUM_PAGES_128Mb	65536
#define W25X_SECTOR_SIZE		4096

/* W25X SPI Commands */
#define W25X_CMD_MANUFACTURER_ID		0xEF	/* Manufacturer ID */
#define W25X_CMD_DEVICE_ID				0x90 	/* Device ID */
#define W25X_CMD_WRITE_ENABLE			0x06	/* Write Enable */
#define W25X_CMD_WRITE_VOLATILE_ENABLE	0x50	/* Write Enable for Volatile */
#define W25X_CMD_WRITE_DISABLE			0x04	/* Write Disable */
#define W25X_CMD_READ_STATUS_1			0x05	/* Read Status Register 1 */
#define W25X_CMD_WRITE_STATUS_1			0x01	/* Write Status Register 1 */
#define W25X_CMD_READ_STATUS_2			0x35	/* Read Status Register 2 */
#define W25X_CMD_WRITE_STATUS_2			0x31	/* Write Status Register 2 */
#define W25X_CMD_READ_STATUS_3			0x15	/* Read Status Register 3 */
#define W25X_CMD_WRITE_STATUS_4			0x11	/* Write Status Register 3 */
#define W25X_CMD_READ					0x03	/* Read Data */
#define W25X_CMD_FAST_READ				0x0B	/* Fast Read Data */
#define W25X_CMD_PAGE_PROGRAM			0x02	/* Page Program */
#define W25X_CMD_ERASE_4K				0x20	/* Erase a 4kB Sector */
#define W25X_CMD_ERASE_32K				0x52	/* Erase a 32kB Block */
#define W25X_CMD_ERASE_64K				0xD8	/* Erase a 64kB Block */
#define W25X_CMD_CHIP_ERASE				0xC7	/* Mass Erase */
#define W25X_CMD_DEEP_PWR_DOWN			0xB9	/* Power down */
#define W25X_CMD_RELEASE_PWR_DOWN		0xAB	/* Release Power-Down */
#define W25X_CMD_ENABLE_RESET			0x66	/* Enable Reset */
#define W25X_CMD_RESET_DEVICE			0x99	/* Reset Device */

/* Bitmasks of status register 1 */
#define W25X_MASK_STATUS1_STATUS_REGISTER_PROTECT	0x80
#define W25X_MASK_STATUS1_BLOCK_PROTECT				0x1C
#define W25X_MASK_STATUS1_WRITE_ENABLE_LATCH		0x02
#define W25X_MASK_STATUS1_BUSY						0x01

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eW25xMode_t {
	SEND_COMMAND_ONLY,
	READ_DATA,
	WRITE_DATA
} eW25xMode_t;

typedef struct xW25xGenericCommand_t
{
	eW25xMode_t eMode;
	uint8_t		ucCommand;
	uint32_t	ulPageNumber;
	uint8_t		ucByteOffset;
	uint8_t		ucDummyBytes;
	uint16_t	usNumDataBytes;
	uint8_t *   pucData;
} xW25xGenericCommand_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eW25xFlashInit( xFlashDevice_t *pxDevice );
eModuleError_t eW25xFlashSleep( xFlashDevice_t *pxDevice );
eModuleError_t eW25xFlashWake( xFlashDevice_t *pxDevice, bool bWasDepowered );
eModuleError_t eW25xFlashWriteSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen );
eModuleError_t eW25xFlashReadSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen );
eModuleError_t eW25xFlashReadStart( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset );
eModuleError_t eW25xFlashErasePages( xFlashDevice_t *pxDevice, uint32_t ulStartPage, uint32_t ulNumPages );
eModuleError_t eW25xFlashEraseAll( xFlashDevice_t *pxDevice );

static eModuleError_t prvW25xGenericCommand( xFlashDevice_t *pxDevice, const xW25xGenericCommand_t *pxCommand );
static eModuleError_t prvWaitWhileBusy( xFlashDevice_t *pxDevice, TickType_t xPollDelay, TickType_t xTimeout );

/* Private Variables ----------------------------------------*/

xFlashImplementation_t xW25xDriver = {
	.fnInit			= eW25xFlashInit,
	.fnWake			= eW25xFlashWake,
	.fnSleep		= eW25xFlashSleep,
	.fnReadSubpage  = eW25xFlashReadSubpage,
	.fnWriteSubpage = eW25xFlashWriteSubpage,
	.fnReadStart	= eW25xFlashReadStart,
	.fnErasePages   = eW25xFlashErasePages,
	.fnEraseAll		= eW25xFlashEraseAll
};

static const xW25xGenericCommand_t xCommandReleasePowerDown = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= W25X_CMD_RELEASE_PWR_DOWN,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

static const xW25xGenericCommand_t xCommandPowerDown = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= W25X_CMD_DEEP_PWR_DOWN,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

static const xW25xGenericCommand_t xCommandWriteEnable = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= W25X_CMD_WRITE_ENABLE,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

static const xW25xGenericCommand_t xCommandChipErase = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= W25X_CMD_CHIP_ERASE,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashInit( xFlashDevice_t *pxDevice )
{
	xFlashSettings_t *pxSettings = &pxDevice->xSettings;
	xW25xHardware_t * pxHardware = (xW25xHardware_t *) pxDevice->pxHardware;
	uint8_t			  pucDeviceId[2];

	/* SPI Configuration */
	pxHardware->xSpiConfig.ulMaxBitrate = 8000000; /* Maximum clock speed of device = 133MHz */
	pxHardware->xSpiConfig.ucDummyTx	= 0xFF;
	pxHardware->xSpiConfig.ucMsbFirst   = true;
	pxHardware->xSpiConfig.eClockMode   = eSpiClockMode0;

	/* Flash Settings */
	pxSettings->ucEraseByte		 = 0xFF;
	pxSettings->usPageSize		 = W25X_PAGE_SIZE;
	pxSettings->ucPageSizePower  = 8; /* 2 ^ 8 == 256 == W25X_PAGE_SIZE */
	pxSettings->usErasePages	 = 16;
	pxSettings->usPageOffsetMask = ( W25X_PAGE_SIZE - 1 );

	/* Allocate space for reading a page internally */
	pxSettings->pucPage = pvPortMalloc( W25X_PAGE_SIZE );

	/* Release the device from Power Down */
	prvW25xGenericCommand( pxDevice, &xCommandReleasePowerDown );
	vTaskDelay( 1 );

	/* Wait until chip not busy */
	if ( prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 10 ), pdMS_TO_TICKS( 2000 ) ) != ERROR_NONE ) {
		eLog( LOG_FLASH_DRIVER, LOG_ERROR, "Flash: Failed to boot...\r\n" );
		return ERROR_INITIALISATION_FAILURE;
	}

	/* Get the manufacturer / device ID */
	xW25xGenericCommand_t xDeviceIdCommand = {
		.eMode			= READ_DATA,
		.ucCommand		= W25X_CMD_DEVICE_ID,
		.ulPageNumber   = 0x0000,
		.ucByteOffset   = 0,
		.ucDummyBytes   = 0,
		.usNumDataBytes = 2,
		.pucData		= pucDeviceId
	};
	prvW25xGenericCommand( pxDevice, &xDeviceIdCommand );

	/* Determine how many pages are present */
	switch ( pucDeviceId[1] ) {
		case W25X_DEVICE_ID_W25X20CL:
			pxSettings->ulNumPages = W25X_NUM_PAGES_2Mb;
			break;
		case W25X_DEVICE_ID_W25X40CL:
			pxSettings->ulNumPages = W25X_NUM_PAGES_4Mb;
			break;
		case W25X_DEVICE_ID_W25Q64FV:
			pxSettings->ulNumPages = W25X_NUM_PAGES_64Mb;
			break;
		case W25X_DEVICE_ID_W25Q128JV:
			pxSettings->ulNumPages = W25X_NUM_PAGES_128Mb;
			break;
		default:
			return ERROR_INITIALISATION_FAILURE;
	}
	eLog( LOG_FLASH_DRIVER, LOG_INFO, "%s Initialisation Complete, ID: % 2A Blocks: %d\r\n", pxDevice->pcName, pucDeviceId, pxSettings->ulNumPages );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashSleep( xFlashDevice_t *pxDevice )
{
	return prvW25xGenericCommand( pxDevice, &xCommandPowerDown );
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashWake( xFlashDevice_t *pxDevice, bool bWasDepowered )
{
	eModuleError_t eError;
	/* No additional action needs to be taken if power was removed from the device */
	UNUSED( bWasDepowered );
	eError = prvW25xGenericCommand( pxDevice, &xCommandReleasePowerDown );
	vTaskDelay( 1 );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashWriteSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen )
{
	/* Page Program takes a maximum of 3ms */
	xW25xGenericCommand_t xWriteCommand = {
		.eMode			= WRITE_DATA,
		.ucCommand		= W25X_CMD_PAGE_PROGRAM,
		.ulPageNumber   = ulPage,
		.ucByteOffset   = (uint8_t) usPageOffset,
		.ucDummyBytes   = 0,
		.usNumDataBytes = usDataLen,
		.pucData		= pucData
	};
	prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
	prvW25xGenericCommand( pxDevice, &xWriteCommand );
	/* Wait for the page write to complete, checking every tick for a maximum of 5ms */
	return prvWaitWhileBusy( pxDevice, 1, pdMS_TO_TICKS( 5 ) );
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashReadSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen )
{
	/* Consider using W25X_CMD_FAST_READ if we can temporarily boost the clock frequency */
	xW25xGenericCommand_t xReadCommand = {
		.eMode			= READ_DATA,
		.ucCommand		= W25X_CMD_READ,
		.ulPageNumber   = ulPage,
		.ucByteOffset   = (uint8_t) usPageOffset,
		.ucDummyBytes   = 0,
		.usNumDataBytes = usDataLen,
		.pucData		= pucData
	};
	return prvW25xGenericCommand( pxDevice, &xReadCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashReadStart( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset )
{
	const xW25xHardware_t *pxHardware = (xW25xHardware_t *) pxDevice->pxHardware;
	uint8_t				   pucCommand[4];

	/* Start the SPI transaction */
	eSpiBusStart( pxHardware->pxInterface, &pxHardware->xSpiConfig, portMAX_DELAY );
	vSpiCsAssert( pxHardware->pxInterface );

	/* Send the command */
	pucCommand[0] = W25X_CMD_READ;
	pucCommand[1] = ( ulPage >> 8 ) & 0xFF;
	pucCommand[2] = ( ulPage >> 0 ) & 0xFF;
	pucCommand[3] = usPageOffset & 0xFF;
	vSpiTransmit( pxHardware->pxInterface, pucCommand, 4 );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashErasePages( xFlashDevice_t *pxDevice, uint32_t ulStartPage, uint32_t ulNumPages )
{
	const uint16_t usPagesPerSector   = 16;  /**< Sector Erase Operation, T_Max = 400ms */
	const uint16_t usPagesPer32kBlock = 128; /**< 32k Block Erase Operation, T_Max = 1.6s */
	const uint16_t usPagesPer64kBlock = 256; /**< 64k Block Erase Operation, T_Max = 2s */

	xW25xGenericCommand_t xErase = {
		.eMode			= SEND_COMMAND_ONLY,
		.ucCommand		= 0,
		.ulPageNumber   = 0,
		.ucByteOffset   = 0,
		.ucDummyBytes   = 0,
		.usNumDataBytes = 0,
		.pucData		= NULL
	};

	/* Run sector erases to the first 32k block */
	while ( ( ulNumPages > 0 ) && ( ulStartPage % usPagesPer32kBlock != 0 ) ) {
		xErase.ucCommand	= W25X_CMD_ERASE_4K;
		xErase.ulPageNumber = ulStartPage;
		prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
		prvW25xGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
		ulStartPage += usPagesPerSector;
		ulNumPages -= usPagesPerSector;
	}
	/* Run 32k erases to the first 64k block */
	while ( ( ulNumPages > 0 ) && ( ulStartPage % usPagesPer64kBlock != 0 ) ) {
		xErase.ucCommand	= W25X_CMD_ERASE_32K;
		xErase.ulPageNumber = ulStartPage;
		prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
		prvW25xGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 2000 ) );
		ulStartPage += usPagesPer32kBlock;
		ulNumPages -= usPagesPer32kBlock;
	}
	/* Run 64k erases until less than 64k remains */
	while ( ulNumPages >= usPagesPer64kBlock ) {
		xErase.ucCommand	= W25X_CMD_ERASE_64K;
		xErase.ulPageNumber = ulStartPage;
		prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
		prvW25xGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 2500 ) );
		ulStartPage += usPagesPer64kBlock;
		ulNumPages -= usPagesPer64kBlock;
	}
	/* Run 32k erases until less than 32k remains */
	while ( ulNumPages >= usPagesPer32kBlock ) {
		xErase.ucCommand	= W25X_CMD_ERASE_32K;
		xErase.ulPageNumber = ulStartPage;
		prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
		prvW25xGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 2000 ) );
		ulStartPage += usPagesPer32kBlock;
		ulNumPages -= usPagesPer32kBlock;
	}
	/* Run sector erases until done */
	while ( ulNumPages > 0 ) {
		xErase.ucCommand	= W25X_CMD_ERASE_4K;
		xErase.ulPageNumber = ulStartPage;
		prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
		prvW25xGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
		ulStartPage += usPagesPerSector;
		ulNumPages -= usPagesPerSector;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eW25xFlashEraseAll( xFlashDevice_t *pxDevice )
{
	/* Enable the Erase */
	prvW25xGenericCommand( pxDevice, &xCommandWriteEnable );
	/* Send the erase command */
	prvW25xGenericCommand( pxDevice, &xCommandChipErase );
	/* Wait for the erase to complete, checking once a second for a maximum of 200 seconds */
	return prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 1000 ), pdMS_TO_TICKS( 200000 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvW25xGenericCommand( xFlashDevice_t *pxDevice, const xW25xGenericCommand_t *pxCommand )
{
	const xW25xHardware_t *pxHardware = (xW25xHardware_t *) pxDevice->pxHardware;
	uint8_t				   ucTmpData;
	uint8_t				   pucCommand[4];

	eLog( LOG_FLASH_DRIVER, LOG_VERBOSE, "%s Command - Mode: %d Comm: 0x%02X Page: %d\r\n", pxDevice->pcName, pxCommand->eMode, pxCommand->ucCommand, pxCommand->ulPageNumber );

	/* Start the SPI transaction */
	eSpiBusStart( pxHardware->pxInterface, &pxHardware->xSpiConfig, portMAX_DELAY );
	vSpiCsAssert( pxHardware->pxInterface );

	/* Send the command byte */
	pucCommand[0] = pxCommand->ucCommand;
	pucCommand[1] = ( pxCommand->ulPageNumber >> 8 ) & 0xFF;
	pucCommand[2] = ( pxCommand->ulPageNumber >> 0 ) & 0xFF;
	pucCommand[3] = pxCommand->ucByteOffset;

	/* Send address if it is passed in */
	uint32_t ulNumAddressBytes = ( pxCommand->ulPageNumber == NO_ADDRESS ) ? 1 : 4;
	vSpiTransmit( pxHardware->pxInterface, pucCommand, ulNumAddressBytes );

	/* Write dummy bytes */
	ucTmpData		   = 0xFF;
	uint8_t ucNumDummy = pxCommand->ucDummyBytes;
	while ( ucNumDummy-- ) {
		vSpiTransmit( pxHardware->pxInterface, &ucTmpData, 1 );
	}

	/* If reading: read. If writing: write. If sending command only: exit */
	switch ( pxCommand->eMode ) {
		case SEND_COMMAND_ONLY:
			break;
		case READ_DATA:
			vSpiReceive( pxHardware->pxInterface, pxCommand->pucData, pxCommand->usNumDataBytes );
			break;
		case WRITE_DATA:
			vSpiTransmit( pxHardware->pxInterface, pxCommand->pucData, pxCommand->usNumDataBytes );
			break;
		default:
			configASSERT( 0 );
	}
	/* Finish the transaction */
	vSpiCsRelease( pxHardware->pxInterface );
	vSpiBusEnd( pxHardware->pxInterface );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvWaitWhileBusy( xFlashDevice_t *pxDevice, TickType_t xPollDelay, TickType_t xTimeout )
{
	// This will cause overflow so enter something reasonable.
	configASSERT( xTimeout != portMAX_DELAY );

	uint8_t		   ucStatus = 0xFF;
	eModuleError_t eError;
	TickType_t	 xEndTime = xTaskGetTickCount() + xTimeout;

	xW25xGenericCommand_t xReadStatus = {
		.eMode			= READ_DATA,
		.ucCommand		= W25X_CMD_READ_STATUS_1,
		.ulPageNumber   = NO_ADDRESS,
		.ucByteOffset   = 0,
		.ucDummyBytes   = 0,
		.usNumDataBytes = 1,
		.pucData		= &ucStatus
	};

	while ( true ) {
		/* Read status register into ucStatus */
		eError = prvW25xGenericCommand( pxDevice, &xReadStatus );
		if ( eError != ERROR_NONE ) {
			eLog( LOG_FLASH_DRIVER, LOG_ERROR, "%s Failed to read status\r\n", pxDevice->pcName );
			return eError;
		}

		/* Break if we are no longer busy */
		if ( !( ucStatus & W25X_MASK_STATUS1_BUSY ) ) {
			eLog( LOG_FLASH_DRIVER, LOG_DEBUG, "%s WWB done\r\n", pxDevice->pcName );
			return ERROR_NONE;
		}

		/* Break if we have timed out */
		if ( xTaskGetTickCount() > xEndTime ) {
			eLog( LOG_FLASH_DRIVER, LOG_ERROR, "%s WWB timeout\r\n", pxDevice->pcName );
			return ERROR_TIMEOUT;
		}

		/* Otherwise wait and try again */
		else {
			if ( ( xEndTime - xTaskGetTickCount() ) < xPollDelay ) {
				eLog( LOG_FLASH_DRIVER, LOG_VERBOSE, "%s WWB wait small\r\n", pxDevice->pcName );
				vTaskDelay( xEndTime - xTaskGetTickCount() );
			}
			else {
				eLog( LOG_FLASH_DRIVER, LOG_VERBOSE, "%s WWB waiting %d ticks\r\n", pxDevice->pcName, xPollDelay );
				vTaskDelay( xPollDelay );
			}
		}
	}
}

/*-----------------------------------------------------------*/
