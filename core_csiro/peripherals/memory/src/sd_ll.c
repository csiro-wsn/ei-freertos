/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "sd_ll.h"

#include "log.h"
#include "memory_operations.h"
#include "spi.h"

/* Private Defines ------------------------------------------*/
// clang-format off

/* Control Tokens */
#define SD_START_BLOCK          0xFE
#define SD_START_BLK_MULTI      0xFC
#define SD_STOP_TRANS           0xFD

#define SD_DATA_RESPONSE_MASK   0x1F
#define SD_DATA_ACCEPTED        0x05
#define SD_DATA_CRC_ERROR       0x0B
#define SD_DATA_WRITE_ERROR     0x0D

/* R1 Response Masks */
#define R1_NO_RESPONSE          0xFF
#define R1_RESPONSE_RECV        0x80
#define R1_IDLE_STATE           (1 << 0)
#define R1_ERASE_RESET          (1 << 1)
#define R1_ILLEGAL_COMMAND      (1 << 2)
#define R1_COM_CRC_ERROR        (1 << 3)
#define R1_ERASE_SEQUENCE_ERROR (1 << 4)
#define R1_ADDRESS_ERROR        (1 << 5)
#define R1_PARAMETER_ERROR      (1 << 6)

#define CSD_CSD_STRUCTURE( buf )            (( buf[0] & 0xC0000000) >> 30 )
#define CSD_TAAC( buf )                     (( buf[0] & 0x00FF0000) >> 16 )
#define CSD_NSAC( buf )                     (( buf[0] & 0x0000FF00) >> 8 )
#define CSD_TRAN_SPEED( buf )               (( buf[0] & 0x000000FF) >> 0 )

#define CSD_CCC( buf )                      (( buf[1] & 0xFFF00000) >> 20 )
#define CSD_READ_BL_LEN( buf )              (( buf[1] & 0x000F0000) >> 16 )
#define CSD_READ_BL_PARTIAL( buf )          (( buf[1] & 0x00008000) >> 15 )
#define CSD_WRITE_BLK_MISALIGN( buf )       (( buf[1] & 0x00004000) >> 14 )
#define CSD_READ_BLK_MISALIGN( buf )        (( buf[1] & 0x00002000) >> 13 )
#define CSD_DSR_IMP( buf )                  (( buf[1] & 0x00001000) >> 12 )

#define CSD_C_SIZE( buf )                   ((( buf[1] & 0x0000003F ) << 16 ) | (( buf[2] & 0xFFFF0000 ) >> 16))

#define CSD_ERASE_BLK_LEN( buf )            (( buf[2] & 0x00004000) >> 14 )
#define CSD_SECTOR_SIZE( buf )              (( buf[2] & 0x00003F80) >> 7 )
#define CSD_WP_GRP_SIZE( buf )              (( buf[2] & 0x0000007F) >> 0 )

#define CSD_WP_GRP_ENABLE( buf )            (( buf[3] & 0x80000000) >> 31 )
#define CSD_R2W_FACTOR( buf )               (( buf[3] & 0x1C000000) >> 26 )
#define CSD_WRITE_BL_LEN( buf )             (( buf[3] & 0x03C00000) >> 22 )
#define CSD_WRITE_BL_PARTIAL( buf )         (( buf[3] & 0x00200000) >> 21 )

#define CSD_FILE_FORMAT_GRP( buf )          (( buf[3] & 0x00008000) >> 15 )
#define CSD_COPY( buf )                     (( buf[3] & 0x00004000) >> 14 )
#define CSD_PERM_WRITE_PROTECT( buf )       (( buf[3] & 0x00002000) >> 13 )
#define CSD_TMP_WRITE_PROTECT( buf )        (( buf[3] & 0x00001000) >> 12 )
#define CSD_FILE_FORMAT( buf )              (( buf[3] & 0x00000C00) >> 10 )
#define CSD_CRC( buf )                      (( buf[3] & 0x000000FE) >> 1 )

// clang-format on

#define SD_COMMAND_TIMEOUT 1000 //ms
#define SD_COMMAND_SIZE 6
#define SD_COMMAND_RETRIES 4

/* SD Simplified Spec V6.0 pg80 */
#define SD_SDHC_TIMEOUT_READ 100
#define SD_SDHC_TIMEOUT_WRITE 250
#define SD_SDHC_TIMEOUT_ERASE 250

/* Type Definitions -----------------------------------------*/

typedef struct xCSD_t
{
	uint8_t  ucCSD_STRUCTURE;
	uint8_t  ucTAAC;
	uint8_t  ucNSAC;
	uint8_t  ucTRAN_SPEED;
	uint16_t usCCC;
	uint8_t  ucREAD_BL_LEN;
	uint8_t  ucDSR_IMP;
	uint32_t ulC_SIZE;
	uint8_t  ucSECTOR_SIZE;
	uint8_t  ucWRITE_BL_LEN;
	uint8_t  ucCOPY;
	uint8_t  ucPERM_WRITE_PROTECT;
	uint8_t  ucTEMP_WRITE_PROTECT;
} ATTR_PACKED xCSD_t;

/* Function Declarations ------------------------------------*/

static eModuleError_t prvWaitReady( uint16_t usWaitMs );

static eModuleError_t prvWaitToken( uint8_t ucToken );

static eModuleError_t prvCommandSpi( eSdCommand_t eCommand, uint32_t ulArgument, uint8_t *pucResponse );

/* Private Variables ----------------------------------------*/

/* 512 byte block together with 2 byte CRC */
static uint8_t puc00[SD_DEFAULT_BLOCK_SIZE + SD_DEFAULT_CRC_SIZE] = { [0 ... 513] = 0x00 };
static uint8_t pucFF[SD_DEFAULT_BLOCK_SIZE + SD_DEFAULT_CRC_SIZE] = { [0 ... 513] = 0xFF };

static xSpiConfig_t xConfig = {
	.ulMaxBitrate = 8000000,
	.ucDummyTx	= 0xFF,
	.ucMsbFirst   = true,
	.xCsGpio	  = { 0 },
	.eClockMode   = eSpiClockMode0
};
static xSpiModule_t *pxSpi;

static xCSD_t xCSD;

/*-----------------------------------------------------------*/

void vSdLLInit( xSpiModule_t *pxModule, xGpio_t xCs )
{
	configASSERT( pxModule != NULL );
	pxSpi			= pxModule;
	xConfig.xCsGpio = xCs;
}

/*-----------------------------------------------------------*/

void vSdLLWakeSequence( void )
{
	configASSERT( eSpiBusStart( pxSpi, &xConfig, portMAX_DELAY ) == ERROR_NONE );
	/* Send at least 74 Clocks before first command (9.25 bytes) with CS held high */
	pxSpi->bCsAsserted = true;
	vSpiTransmit( pxSpi, (uint8_t *) pucFF, 10 );
	pxSpi->bCsAsserted = false;
	vSpiBusEnd( pxSpi );
}

/*-----------------------------------------------------------*/

eModuleError_t eSdCommand( eSdCommand_t eCommand, uint32_t ulArgument, uint8_t *pucResponse )
{
	static uint32_t ulLastCommandTimeout = SD_COMMAND_TIMEOUT;
	eModuleError_t  eError;
	uint8_t			ucDummy;
	bool			bIsAppCommand;

	/* Claim and assert the SPI bus */
	eError = eSpiBusStart( pxSpi, &xConfig, portMAX_DELAY );
	if ( eError != ERROR_NONE ) {
		eLog( LOG_SD_DRIVER, LOG_APOCALYPSE, "SD: Failed to claim SPI bus\r\n" );
		return eError;
	}
	vSpiCsAssert( pxSpi );

	/* Check if command requires CMD55 to be sent */
	switch ( eCommand ) {
		case SD_SEND_OP_COND:
		case SD_SEND_SCR:
		case SD_SEND_STATUS:
			bIsAppCommand = true;
			break;
		default:
			bIsAppCommand = false;
	}

	/* Wait until the SD card is ready to talk to us for all commands */
	if ( eCommand != SD_STOP_TRANSMISSION ) {
		eError = prvWaitReady( ulLastCommandTimeout );
		if ( eError != ERROR_NONE ) {
			eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Card was not ready\r\n" );
			goto command_error;
		}
	}

	/* Send the command until we get a response from the card */
	for ( uint8_t i = 0; i < SD_COMMAND_RETRIES; i++ ) {
		if ( bIsAppCommand ) {
			/* Send CMD55 for application commands */
			eError = prvCommandSpi( SD_APP_CMD, 0x00, &ucDummy );
			if ( eError != ERROR_NONE ) {
				eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Failed to send application command\r\n" );
				goto command_error;
			}
			if ( prvWaitReady( SD_COMMAND_TIMEOUT ) != ERROR_NONE ) {
				eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Card is not ready yet\r\n" );
			}
			eLog( LOG_SD_DRIVER, LOG_VERBOSE, "SD: CMD - %d RESP - 0x%02X\r\n", SD_APP_CMD, ucDummy );
		}

		eError = prvCommandSpi( eCommand, ulArgument, pucResponse );

		eLog( LOG_SD_DRIVER, LOG_VERBOSE, "SD: CMD - %d RESP - 0x%02X\r\n", eCommand, *pucResponse );

		if ( *pucResponse != SD_RESP_NO_RESPONSE ) {
			break;
		}
	}

	/* Handle Command errors */
	if ( *pucResponse == SD_RESP_NO_RESPONSE ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: No response to CMD %d\r\n", eCommand );
		eError = ERROR_TIMEOUT;
		goto command_error;
	}
	else if ( *pucResponse & SD_RESP_COM_CRC_ERROR ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: CRC error on CMD %d\r\n", eCommand );
		eError = ERROR_INVALID_CRC;
		goto command_error;
	}
	else if ( *pucResponse & SD_RESP_ILLEGAL_COMMAND ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Illegal CMD %d\r\n", eCommand );
		eError = ERROR_COMMAND_NOT_ACCEPTED;
		goto command_error;
	}
	if ( ( *pucResponse & SD_RESP_ERASE_RESET ) || ( *pucResponse & SD_RESP_ERASE_SEQ_ERROR ) ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Erase Error\r\n" );
		eError = ERROR_GENERIC;
	}
	if ( ( *pucResponse & SD_RESP_ADDRESS_ERROR ) || ( *pucResponse & SD_RESP_PARAMETER_ERROR ) ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Invalid parameter/address %d\r\n", ulArgument );
		eError = ERROR_INVALID_ADDRESS;
	}

	/* Read any remaining bytes afer the command response */
	const char *pcFormatStr;
	switch ( eCommand ) {
		case SD_SEND_IF_COND: /* R7 Response, 4 bytes remaining */
		case SD_READ_OCR:	 /* R3 Response, 4 bytes remaining */
			vSpiReceive( pxSpi, pucResponse + 1, 4 );
			pcFormatStr = "SD: CMD - %d RESP - %02X %02X %02X %02X %02X\r\n";
			break;
		case SD_STOP_TRANSMISSION: /* R1b Response, wait until ready */
		case SD_ERASE:			   /* R1b Response, wait until ready */
			prvWaitReady( SD_COMMAND_TIMEOUT );
			pcFormatStr = "SD: CMD - %d RESP - %02X\r\n";
			break;
		case SD_SEND_STATUS: /* R2 Response, 1 byte remaining */
			vSpiReceive( pxSpi, pucResponse + 1, 1 );
			pcFormatStr = "SD: CMD - %d RESP - %02X %02X\r\n";
			break;
		default: /* R1 Response, no bytes remaining */
			pcFormatStr = "SD: CMD - %d RESP - %02X\r\n";
			break;
	}
	eLog( LOG_SD_DRIVER, LOG_VERBOSE, pcFormatStr, eCommand, pucResponse[0], pucResponse[1], pucResponse[2], pucResponse[3], pucResponse[4] );

	/* For read and write commands, CS must be left low for following data */
	switch ( eCommand ) {
		case SD_SEND_CSD:
		case SD_SEND_CID:
		case SD_SEND_SCR:
		case SD_WRITE_BLOCK:
		case SD_WRITE_MULTIPLE_BLOCK:
		case SD_READ_SINGLE_BLOCK:
		case SD_READ_MULTIPLE_BLOCK:
			return ERROR_NONE;
		default:
			break;
	}

	/* Deassert and release the SPI bus */
command_error:
	vSpiCsRelease( pxSpi );
	vSpiBusEnd( pxSpi );
	return eError;
}

/*-----------------------------------------------------------*/

void vSdParseCSD( uint8_t *pucCSD, xSdParameters_t *pxParams )
{
	uint32_t pulCSD[4] = { BE_U32_EXTRACT( pucCSD ), BE_U32_EXTRACT( pucCSD + 4 ), BE_U32_EXTRACT( pucCSD + 8 ), BE_U32_EXTRACT( pucCSD + 12 ) };

	xCSD.ucCSD_STRUCTURE = CSD_CSD_STRUCTURE( pulCSD );

	/* CSD Version 1.0 - Standard Capacity Cards Only */
	if ( xCSD.ucCSD_STRUCTURE == 0 ) {
		eLog( LOG_SD_DRIVER, LOG_APOCALYPSE, "SD: Standard Capacity CSD register decoding not implemented\r\n" );
		pxParams->ulBlockSize = 0;
		pxParams->ulNumBlocks = 0;
	}
	/* CSD Version 2.0 - SDHC and SDXC Cards */
	else if ( xCSD.ucCSD_STRUCTURE == 1 ) {
		xCSD.ucTAAC				  = CSD_TAAC( pulCSD );
		xCSD.ucNSAC				  = CSD_NSAC( pulCSD );
		xCSD.ucTRAN_SPEED		  = CSD_TRAN_SPEED( pulCSD );
		xCSD.usCCC				  = CSD_CCC( pulCSD );
		xCSD.ucREAD_BL_LEN		  = CSD_READ_BL_LEN( pulCSD );
		xCSD.ucDSR_IMP			  = CSD_DSR_IMP( pulCSD );
		xCSD.ulC_SIZE			  = CSD_C_SIZE( pulCSD );
		xCSD.ucSECTOR_SIZE		  = CSD_SECTOR_SIZE( pulCSD );
		xCSD.ucWRITE_BL_LEN		  = CSD_WRITE_BL_LEN( pulCSD );
		xCSD.ucCOPY				  = CSD_COPY( pulCSD );
		xCSD.ucPERM_WRITE_PROTECT = CSD_PERM_WRITE_PROTECT( pulCSD );
		xCSD.ucTEMP_WRITE_PROTECT = CSD_TMP_WRITE_PROTECT( pulCSD );

		/* SD Size */
		pxParams->ulBlockSize	= ( 0x01 << xCSD.ucREAD_BL_LEN ); /* 2 ^ READ_BL_LEN */
		pxParams->ulNumBlocks	= ( xCSD.ulC_SIZE + 1 ) * 1024;
		pxParams->ulDeviceSizeMB = pxParams->ulNumBlocks / ( 2 * 1024 );

		/* TAAC and NSAC are not used for CSD V2.0 */
	}
}

/*-----------------------------------------------------------*/

void vSdParseSCR( uint8_t *pucCSR, xSdParameters_t *pxParams )
{
	uint8_t ucStructure  = ( BE_U8_EXTRACT( pucCSR + 0 ) & 0xF0 ) >> 4;
	uint8_t ucSDVersion  = ( BE_U8_EXTRACT( pucCSR + 0 ) & 0x0F ) >> 0;
	uint8_t ucEraseValue = ( BE_U8_EXTRACT( pucCSR + 1 ) & 0x80 ) >> 7 ? 0xFF : 0x00;
	uint8_t ucSdSpec3	= ( BE_U8_EXTRACT( pucCSR + 2 ) & 0x80 ) >> 7;
	uint8_t ucSdSpec4	= ( BE_U8_EXTRACT( pucCSR + 2 ) & 0x04 ) >> 2;
	uint8_t ucSdSpecX	= ( BE_U16_EXTRACT( pucCSR + 2 ) & 0x03C0 ) >> 6;

	if ( ucStructure != 0 ) {
		eLog( LOG_SD_DRIVER, LOG_APOCALYPSE, "SD: Unknown SCR Structure\r\n" );
		pxParams->ucEraseByte = 0x00;
		return;
	}
	const char *pcVersion;
	if ( ucSdSpecX == 2 ) {
		pcVersion = "6.XX";
	}
	else if ( ucSdSpecX == 1 ) {
		pcVersion = "5.XX";
	}
	else if ( ucSdSpec4 ) {
		pcVersion = "4.XX";
	}
	else if ( ucSdSpec3 ) {
		pcVersion = "3.0X";
	}
	else if ( ucSDVersion == 2 ) {
		pcVersion = "2.00";
	}
	else if ( ucSDVersion == 1 ) {
		pcVersion = "1.10";
	}
	else {
		pcVersion = "1.01";
	}
	pxParams->ucEraseByte = ucEraseValue;

	eLog( LOG_SD_DRIVER, LOG_ERROR, "SD Configuration Register\r\n"
									"\tVersion: %s\r\n"
									"\tErase V: 0x%02X\r\n",
		  pcVersion, ucEraseValue );
}

/*-----------------------------------------------------------*/

void vSdPrintCID( uint8_t *pucCID )
{
	uint8_t  ucManufacturerId  = BE_U8_EXTRACT( pucCID + 0 );
	char *   pcApplicationId   = (char *) pucCID + 1;
	char *   pcProductName	 = (char *) pucCID + 3;
	uint8_t  ucProductRevision = BE_U8_EXTRACT( pucCID + 8 );
	uint32_t ulSerialNumber	= BE_U16_EXTRACT( pucCID + 9 );
	uint16_t usManufactureDate = 0xFFF & BE_U16_EXTRACT( pucCID + 13 );

	const char *pcManufacturer = "Unknown";
	switch ( ucManufacturerId ) {
		case 0x01:
			pcManufacturer = "Panasonic";
			break;
		case 0x02:
			pcManufacturer = "Toshiba";
			break;
		case 0x03:
			pcManufacturer = "SanDisk";
			break;
		case 0x1B:
			pcManufacturer = "Samsung";
			break;
		case 0x1D:
			pcManufacturer = "AData";
			break;
		case 0x27:
			pcManufacturer = "Phison";
			break;
		case 0x28:
			pcManufacturer = "Lexar";
			break;
		case 0x31:
			pcManufacturer = "Silicon Power";
			break;
		case 0x41:
			pcManufacturer = "Kingston";
			break;
		case 0x74:
			pcManufacturer = "Transcend";
			break;
		case 0x76:
			pcManufacturer = "Patriot";
			break;
		case 0x82:
		case 0x9C:
			pcManufacturer = "Sony";
			break;
		default:
			break;
	}

	eLog( LOG_SD_DRIVER, LOG_ERROR, "SD Card Identification\r\n"
									"\tManu : %s\r\n"
									"\tApp  : %.2s\r\n"
									"\tName : %.5s\r\n"
									"\tRev  : %d.%d\r\n"
									"\tSer  : %08X\r\n"
									"\tMDT  : %d/%d\r\n",
		  pcManufacturer, pcApplicationId, pcProductName, ( ucProductRevision & 0xF0 ) >> 4, ucProductRevision & 0x0F,
		  ulSerialNumber, usManufactureDate & 0xF, 2000 + ( ( usManufactureDate & 0xFF0 ) >> 4 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t prvWaitReady( uint16_t usWaitMs )
{
	uint8_t	ucResponse;
	TickType_t xEndTime = xTaskGetTickCount() + pdMS_TO_TICKS( usWaitMs );
	do {
		vSpiReceive( pxSpi, &ucResponse, 1 );
		if ( ucResponse == 0xFF ) {
			return ERROR_NONE;
		}
		eLog( LOG_SD_DRIVER, LOG_VERBOSE, "SD: Card still busy\r\n" );
		/* Release the SPI bus while we the SD card is busy*/
		vSpiCsRelease( pxSpi );
		vSpiBusEnd( pxSpi );
		vTaskDelay( 1 );
		eSpiBusStart( pxSpi, &xConfig, xEndTime - xTaskGetTickCount() );
		vSpiCsAssert( pxSpi );
	} while ( xTaskGetTickCount() < xEndTime );
	return ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvWaitToken( uint8_t ucToken )
{
	uint8_t	ucResponse;
	TickType_t xEndTime = xTaskGetTickCount() + pdMS_TO_TICKS( SD_SDHC_TIMEOUT_READ );
	do {
		vSpiReceive( pxSpi, &ucResponse, 1 );
		if ( ucResponse == ucToken ) {
			return ERROR_NONE;
		}
		/* Release the SPI bus while we the SD card is busy*/
		vSpiCsRelease( pxSpi );
		vSpiBusEnd( pxSpi );
		vTaskDelay( 1 );
		eSpiBusStart( pxSpi, &xConfig, xEndTime - xTaskGetTickCount() );
		vSpiCsAssert( pxSpi );
	} while ( xTaskGetTickCount() < xEndTime );
	return ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

static eModuleError_t prvCommandSpi( eSdCommand_t eCommand, uint32_t ulArgument, uint8_t *pucResponse )
{
	uint8_t pucCommand[SD_COMMAND_SIZE + 1];

	pucCommand[0] = 0x40 | ( (uint8_t) eCommand & 0x3F );
	pucCommand[1] = ( uint8_t )( ulArgument >> 24 ) & 0xFF;
	pucCommand[2] = ( uint8_t )( ulArgument >> 16 ) & 0xFF;
	pucCommand[3] = ( uint8_t )( ulArgument >> 8 ) & 0xFF;
	pucCommand[4] = ( uint8_t )( ulArgument >> 0 ) & 0xFF;
	/* CRC: 0x95 on CMD0, 0x87 on CMD8, otherwise 0xFF */
	pucCommand[5] = ( eCommand == SD_GO_IDLE_STATE ) ? 0x95 : ( eCommand == SD_SEND_IF_COND ) ? 0x87 : 0xFF;
	pucCommand[6] = 0xFF;

	/* SD_STOP_TRANSMISSION requires an additional byte of dummy data to be sent before the response can be read */
	uint32_t ulDataLen = ( eCommand == SD_STOP_TRANSMISSION ) ? SD_COMMAND_SIZE + 1 : SD_COMMAND_SIZE;

	/* Send the 6 byte command sequence */
	vSpiTransmit( pxSpi, pucCommand, ulDataLen );

	TickType_t xEndTime = xTaskGetTickCount() + pdMS_TO_TICKS( SD_SDHC_TIMEOUT_READ );
	do {
		vSpiReceive( pxSpi, pucResponse, 1 );
		if ( !( *pucResponse & SD_RESP_START_BIT_0 ) ) {
			return ERROR_NONE;
		}
		vTaskDelay( 1 );
	} while ( xTaskGetTickCount() < xEndTime );

	return ERROR_TIMEOUT;
}

/*-----------------------------------------------------------*/

eModuleError_t eSdReadRegister( uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	return eSdReadBytes( 0, pucBuffer, ulBufferLen );
}

/*-----------------------------------------------------------*/

eModuleError_t eSdReadBytes( uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	eModuleError_t eError = ERROR_NONE;
	configASSERT( ( usBlockOffset + ulBufferLen ) <= 512 );

	eError = prvWaitToken( SD_START_BLOCK );
	if ( eError != ERROR_NONE ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Failed to receive start token 0x%02X\r\n", SD_START_BLOCK );
	}
	else {
		/* Receive the bytes we need, then discard the remainder and the CRC */
		if ( usBlockOffset > 0 ) {
			vSpiTransmit( pxSpi, (uint8_t *) pucFF, usBlockOffset );
		}
		vSpiReceive( pxSpi, pucBuffer, ulBufferLen );
		vSpiTransmit( pxSpi, (uint8_t *) pucFF, 514 - usBlockOffset - ulBufferLen );
	}
	/* This completes the SPI command */
	vSpiCsRelease( pxSpi );
	vSpiBusEnd( pxSpi );
	return eError;
}

/*-----------------------------------------------------------*/

eModuleError_t eSdWriteBytes( xSdParameters_t *pxParams, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen )
{
	eModuleError_t eError		= ERROR_NONE;
	uint8_t		   ucStartToken = SD_START_BLOCK;
	uint8_t		   ucResponse;
	uint8_t *	  pucClearBytes = (uint8_t *) ( ( pxParams->ucEraseByte ) ? pucFF : puc00 );
	configASSERT( ( usBlockOffset + ulBufferLen ) <= 512 );

	vSpiTransmit( pxSpi, &ucStartToken, 1 ); /* Write the Start Block Token */

	/* Send our buffer, remainder set to 0xFF with a 2 byte CRC */
	if ( usBlockOffset > 0 ) {
		vSpiTransmit( pxSpi, (uint8_t *) pucClearBytes, usBlockOffset );
	}
	vSpiTransmit( pxSpi, pucBuffer, ulBufferLen );
	vSpiTransmit( pxSpi, (uint8_t *) pucClearBytes, 514 - usBlockOffset - ulBufferLen );

	/* Receive the Data Response */
	vSpiReceive( pxSpi, &ucResponse, 1 );
	ucResponse &= SD_DATA_RESPONSE_MASK;
	if ( ucResponse != SD_DATA_ACCEPTED ) {
		eLog( LOG_SD_DRIVER, LOG_ERROR, "SD: Write failed with error 0x%02X\r\n", ucResponse );
		eError = ERROR_FLASH_OPERATION_FAIL;
	}

	prvWaitReady( SD_SDHC_TIMEOUT_WRITE );

	vSpiCsRelease( pxSpi );
	vSpiBusEnd( pxSpi );
	return eError;
}

/*-----------------------------------------------------------*/
