/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "mx25r.h"

#include "log.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define MX25R_PAGE_SIZE       256
#define MX25R_PAGE_COUNT      8192

#define NO_ADDRESS 0xFFFFFFFFul

#define MX25R_COMMAND_PAGE_PROGRAM          0x02
#define MX25R_COMMAND_PAGE_READ             0x03
#define MX25R_COMMAND_READ_STATUS           0x05

#define MX25R_COMMAND_WREN                  0x06

#define MX25R_COMMAND_SECTOR_ERASE          0x20 /* Erase a 4kB Sector */
#define MX25R_COMMAND_BLOCK_ERASE_32K       0x52 /* Erase a 32kB Block */
#define MX25R_COMMAND_BLOCK_ERASE_64K       0xD8 /* Erase a 64kB Block */
#define MX25R_COMMAND_CHIP_ERASE            0xC7 /* Mass Erase */

#define MX25R_COMMAND_READ_SECURITY         0x2B
#define MX25R_COMMAND_POWER_DOWN            0xB9

#define MX25R_COMMAND_READ_IDENTIFICATION   0x9F

// Status Register Masks
#define MX25R_STATUS_WIP                    0x01
#define MX25R_STATUS_WEL                    0x02

// Security Register Masks
#define MX25R_SECURITY_PROGRAM_FAIL         0x20
#define MX25R_SECURITY_ERASE_FAIL           0x40

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eMX25rMode_t {
	SEND_COMMAND_ONLY,
	READ_DATA,
	WRITE_DATA
} eMX25rMode_t;

typedef struct xMX25rGenericCommand_t
{
	eMX25rMode_t eMode;
	uint8_t		 ucCommand;
	uint32_t	 ulPageNumber;
	uint8_t		 ucByteOffset;
	uint8_t		 ucDummyBytes;
	uint16_t	 usNumDataBytes;
	uint8_t *	pucData;
} xMX25rGenericCommand_t;

/* Function Declarations ------------------------------------*/

eModuleError_t eMX25rFlashInit( xFlashDevice_t *pxDevice );
eModuleError_t eMX25rFlashSleep( xFlashDevice_t *pxDevice );
eModuleError_t eMX25rFlashWake( xFlashDevice_t *pxDevice, bool bWasDepowered );
eModuleError_t eMX25rFlashWriteSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen );
eModuleError_t eMX25rFlashReadSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen );
eModuleError_t eMX25rFlashReadStart( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset );
eModuleError_t eMX25rFlashErasePages( xFlashDevice_t *pxDevice, uint32_t ulStartPage, uint32_t ulNumPages );
eModuleError_t eMX25rFlashEraseAll( xFlashDevice_t *pxDevice );

static eModuleError_t prvMX25rGenericCommand( xFlashDevice_t *pxDevice, const xMX25rGenericCommand_t *pxCommand );
static eModuleError_t prvWaitWhileBusy( xFlashDevice_t *pxDevice, TickType_t xPollDelay, TickType_t xTimeout );

/* Private Variables ----------------------------------------*/

xFlashImplementation_t xMX25rDriver = {
	.fnInit			= eMX25rFlashInit,
	.fnWake			= eMX25rFlashWake,
	.fnSleep		= eMX25rFlashSleep,
	.fnReadSubpage  = eMX25rFlashReadSubpage,
	.fnWriteSubpage = eMX25rFlashWriteSubpage,
	.fnReadStart	= eMX25rFlashReadStart,
	.fnErasePages   = eMX25rFlashErasePages,
	.fnEraseAll		= eMX25rFlashEraseAll
};

static const xMX25rGenericCommand_t xCommandPowerDown = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= MX25R_COMMAND_POWER_DOWN,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

static const xMX25rGenericCommand_t xCommandWriteEnable = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= MX25R_COMMAND_WREN,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

static const xMX25rGenericCommand_t xCommandChipErase = {
	.eMode			= SEND_COMMAND_ONLY,
	.ucCommand		= MX25R_COMMAND_CHIP_ERASE,
	.ulPageNumber   = NO_ADDRESS,
	.ucByteOffset   = 0,
	.ucDummyBytes   = 0,
	.usNumDataBytes = 0,
	.pucData		= NULL
};

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashInit( xFlashDevice_t *pxDevice )
{
	xFlashSettings_t *pxSettings = &pxDevice->xSettings;
	xMX25rHardware_t *pxHardware = (xMX25rHardware_t *) pxDevice->pxHardware;
	uint8_t			  pucDeviceId[3];

	/* SPI Configuration */
	pxHardware->xSpiConfig.ulMaxBitrate = 8000000;
	pxHardware->xSpiConfig.ucDummyTx	= 0xFF;
	pxHardware->xSpiConfig.ucMsbFirst   = true;
	pxHardware->xSpiConfig.eClockMode   = eSpiClockMode0;

	/* Flash Settings */
	pxSettings->ucEraseByte		 = 0xFF;
	pxSettings->ulNumPages		 = MX25R_PAGE_COUNT;
	pxSettings->usPageSize		 = MX25R_PAGE_SIZE;
	pxSettings->ucPageSizePower  = 8; /* 2 ^ 8 == 256 == W25X_PAGE_SIZE */
	pxSettings->usErasePages	 = 16;
	pxSettings->usPageOffsetMask = ( MX25R_PAGE_SIZE - 1 );

	/* Allocate space for reading a page internally */
	pxSettings->pucPage = pvPortMalloc( MX25R_PAGE_SIZE );

	/* Wait for boot sequence */
	vTaskDelay( pdMS_TO_TICKS( 10 ) );

	/* Wake the device */
	eMX25rFlashWake( pxDevice, true );

	/* Get the manufacturer / device ID */
	xMX25rGenericCommand_t xDeviceIdCommand = {
		.eMode			= READ_DATA,
		.ucCommand		= MX25R_COMMAND_READ_IDENTIFICATION,
		.ulPageNumber   = NO_ADDRESS,
		.ucByteOffset   = 0,
		.ucDummyBytes   = 0,
		.usNumDataBytes = 3,
		.pucData		= pucDeviceId
	};
	/* Not sure why I have to read twice here, first time always results in FF FF FF, additional delays don't solve it */
	prvMX25rGenericCommand( pxDevice, &xDeviceIdCommand );
	prvMX25rGenericCommand( pxDevice, &xDeviceIdCommand );
	eLog( LOG_FLASH_DRIVER, LOG_INFO, "%s Initialisation Complete, ID: % 3A Blocks: %d\r\n", pxDevice->pcName, pucDeviceId, pxSettings->ulNumPages );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashSleep( xFlashDevice_t *pxDevice )
{
	eModuleError_t eError;
	/* Send sleep command */
	eError = prvMX25rGenericCommand( pxDevice, &xCommandPowerDown );
	/* Device cannot be woken for 30us after entering deep sleep, delay here to ensure that */
	vTaskDelay( 1 );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashWake( xFlashDevice_t *pxDevice, bool bWasDepowered )
{
	xGpio_t xCS = ( (xMX25rHardware_t *) pxDevice->pxHardware )->xSpiConfig.xCsGpio;
	/* No additional action needs to be taken if power was removed from the device */
	UNUSED( bWasDepowered );
	/* Waking device is achieved by holding CS pin low for 20ns ( Same options as vSpiCsAssert/Release ) */
	vGpioSetup( xCS, GPIO_OPENDRAIN, GPIO_OPENDRAIN_LOW );
	vTaskDelay( 1 );
	vGpioSetup( xCS, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	/* 25uS recovery time after waking from deep sleep */
	vTaskDelay( 1 );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashWriteSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen )
{
	/* Page Program takes a maximum of 10ms */
	xMX25rGenericCommand_t xWriteCommand = {
		.eMode			= WRITE_DATA,
		.ucCommand		= MX25R_COMMAND_PAGE_PROGRAM,
		.ulPageNumber   = ulPage,
		.ucByteOffset   = (uint8_t) usPageOffset,
		.ucDummyBytes   = 0,
		.usNumDataBytes = usDataLen,
		.pucData		= pucData
	};
	prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
	prvMX25rGenericCommand( pxDevice, &xWriteCommand );
	/* Wait for the page write to complete, checking every tick for a maximum of 20ms */
	return prvWaitWhileBusy( pxDevice, 1, pdMS_TO_TICKS( 20 ) );
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashReadSubpage( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset, uint8_t *pucData, uint16_t usDataLen )
{
	xMX25rGenericCommand_t xReadCommand = {
		.eMode			= READ_DATA,
		.ucCommand		= MX25R_COMMAND_PAGE_READ,
		.ulPageNumber   = ulPage,
		.ucByteOffset   = (uint8_t) usPageOffset,
		.ucDummyBytes   = 0,
		.usNumDataBytes = usDataLen,
		.pucData		= pucData
	};
	return prvMX25rGenericCommand( pxDevice, &xReadCommand );
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashReadStart( xFlashDevice_t *pxDevice, uint32_t ulPage, uint16_t usPageOffset )
{
	const xMX25rHardware_t *pxHardware = (xMX25rHardware_t *) pxDevice->pxHardware;
	uint8_t					pucCommand[4];

	/* Start the SPI transaction */
	eSpiBusStart( pxHardware->pxInterface, &pxHardware->xSpiConfig, portMAX_DELAY );
	vSpiCsAssert( pxHardware->pxInterface );

	/* Send the command */
	pucCommand[0] = MX25R_COMMAND_PAGE_READ;
	pucCommand[1] = ( ulPage >> 8 ) & 0xFF;
	pucCommand[2] = ( ulPage >> 0 ) & 0xFF;
	pucCommand[3] = usPageOffset & 0xFF;
	vSpiTransmit( pxHardware->pxInterface, pucCommand, 4 );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashErasePages( xFlashDevice_t *pxDevice, uint32_t ulStartPage, uint32_t ulNumPages )
{
	const uint16_t usPagesPerSector   = 16;  /**< Sector Erase Operation, T_Max = 240ms */
	const uint16_t usPagesPer32kBlock = 128; /**< 32k Block Erase Operation, T_Max = 3.0s */
	const uint16_t usPagesPer64kBlock = 256; /**< 64k Block Erase Operation, T_Max = 3.5s */

	xMX25rGenericCommand_t xErase = {
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
		xErase.ucCommand	= MX25R_COMMAND_SECTOR_ERASE;
		xErase.ulPageNumber = ulStartPage;
		prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
		prvMX25rGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
		ulStartPage += usPagesPerSector;
		ulNumPages -= usPagesPerSector;
	}
	/* Run 32k erases to the first 64k block */
	while ( ( ulNumPages > 0 ) && ( ulStartPage % usPagesPer64kBlock != 0 ) ) {
		xErase.ucCommand	= MX25R_COMMAND_BLOCK_ERASE_32K;
		xErase.ulPageNumber = ulStartPage;
		prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
		prvMX25rGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 4000 ) );
		ulStartPage += usPagesPer32kBlock;
		ulNumPages -= usPagesPer32kBlock;
	}
	/* Run 64k erases until less than 64k remains */
	while ( ulNumPages >= usPagesPer64kBlock ) {
		xErase.ucCommand	= MX25R_COMMAND_BLOCK_ERASE_64K;
		xErase.ulPageNumber = ulStartPage;
		prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
		prvMX25rGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 5000 ) );
		ulStartPage += usPagesPer64kBlock;
		ulNumPages -= usPagesPer64kBlock;
	}
	/* Run 32k erases until less than 32k remains */
	while ( ulNumPages >= usPagesPer32kBlock ) {
		xErase.ucCommand	= MX25R_COMMAND_BLOCK_ERASE_32K;
		xErase.ulPageNumber = ulStartPage;
		prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
		prvMX25rGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 250 ), pdMS_TO_TICKS( 4000 ) );
		ulStartPage += usPagesPer32kBlock;
		ulNumPages -= usPagesPer32kBlock;
	}
	/* Run sector erases until done */
	while ( ulNumPages > 0 ) {
		xErase.ucCommand	= MX25R_COMMAND_SECTOR_ERASE;
		xErase.ulPageNumber = ulStartPage;
		prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
		prvMX25rGenericCommand( pxDevice, &xErase );
		prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 50 ), pdMS_TO_TICKS( 500 ) );
		ulStartPage += usPagesPerSector;
		ulNumPages -= usPagesPerSector;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eMX25rFlashEraseAll( xFlashDevice_t *pxDevice )
{
	/* Enable the Erase */
	prvMX25rGenericCommand( pxDevice, &xCommandWriteEnable );
	/* Send the erase command */
	prvMX25rGenericCommand( pxDevice, &xCommandChipErase );
	/* Wait for the erase to complete, checking once a second for a maximum of 100 seconds */
	return prvWaitWhileBusy( pxDevice, pdMS_TO_TICKS( 1000 ), pdMS_TO_TICKS( 100000 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvMX25rGenericCommand( xFlashDevice_t *pxDevice, const xMX25rGenericCommand_t *pxCommand )
{
	const xMX25rHardware_t *pxHardware = (xMX25rHardware_t *) pxDevice->pxHardware;
	uint8_t					ucTmpData;
	uint8_t					pucCommand[4];

	eLog( LOG_FLASH_DRIVER, LOG_VERBOSE, "%s Command - Mode: %d Comm: 0x%02X Page: %d Len %d\r\n", pxDevice->pcName, pxCommand->eMode, pxCommand->ucCommand, pxCommand->ulPageNumber, pxCommand->usNumDataBytes );

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

	xMX25rGenericCommand_t xReadStatus = {
		.eMode			= READ_DATA,
		.ucCommand		= MX25R_COMMAND_READ_STATUS,
		.ulPageNumber   = NO_ADDRESS,
		.ucByteOffset   = 0,
		.ucDummyBytes   = 0,
		.usNumDataBytes = 1,
		.pucData		= &ucStatus
	};

	while ( true ) {
		/* Read status register into ucStatus */
		eError = prvMX25rGenericCommand( pxDevice, &xReadStatus );
		if ( eError != ERROR_NONE ) {
			eLog( LOG_FLASH_DRIVER, LOG_ERROR, "%s Failed to read status\r\n", pxDevice->pcName );
			return eError;
		}

		/* Break if we are no longer busy */
		if ( !( ucStatus & MX25R_STATUS_WIP ) ) {
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
