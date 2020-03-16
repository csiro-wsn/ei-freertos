/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: sd_ll.h
 * Creation_Date: 05/12/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Low level implementation of SD cards
 * 
 */
#ifndef __CSIRO_CORE_SD_LL
#define __CSIRO_CORE_SD_LL
/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "gpio.h"
#include "spi.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define SD_DEFAULT_BLOCK_SIZE 		512
#define SD_DEFAULT_CRC_SIZE			2

#define SD_CHECK_PATTERN    		0xAA

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Different types of SD cards. This driver only supports SD and SDHC cards. */
typedef enum eSdCardType_t {
	SDCARD_TYPE_NONE, /* No card detected */
	SDCARD_TYPE_MMC,  /* MultiMedia Card */
	SDCARD_TYPE_SD,   /* Secure Digital (SD) Card */
	SDCARD_TYPE_SDHC, /* Secure Digital (SD) High Capacity Card */
} eSdCardType_t;

typedef enum eSdCommand_t {
	SD_GO_IDLE_STATE		   = 0,  // CMD0,  Reset into SPI mode
	SD_SEND_IF_COND			   = 8,  // CMD8,  Voltage check
	SD_SEND_CSD				   = 9,  // CMD9,  Send Card Specific Data
	SD_SEND_CID				   = 10, // CMD10,  Send Card Identification
	SD_STOP_TRANSMISSION	   = 12, // CMD12,  Stop Transmission
	SD_SEND_STATUS			   = 13, // ACMD13, Send status
	SD_SET_BLOCKLEN			   = 16, // CMD16, Set block length
	SD_READ_SINGLE_BLOCK	   = 17, // CMD17, Block read
	SD_READ_MULTIPLE_BLOCK	 = 18, // CMD18, Multiple block read
	SD_WRITE_BLOCK			   = 24, // CMD24, Block write
	SD_WRITE_MULTIPLE_BLOCK	= 25, // CMD25,  Multiple block write
	SD_ERASE_WR_BLK_START_ADDR = 32, // CMD32, Erase block start
	SD_ERASE_WR_BLK_END_ADDR   = 33, // CMD33, Erase block end
	SD_ERASE				   = 38, // CMD38, Erase
	SD_SEND_OP_COND			   = 41, // ACMD41, Initialization
	SD_SEND_SCR				   = 51, // ACMD51, Send SD Configuration Register
	SD_APP_CMD				   = 55, // CMD55, Application-Specific
	SD_READ_OCR				   = 58  // CMD58, Read OCR
} eSdCommand_t;

typedef enum eSdCommandResponse_t {
	SD_RESP_IN_IDLE			= 0x01,
	SD_RESP_ERASE_RESET		= 0x02,
	SD_RESP_ILLEGAL_COMMAND = 0x04,
	SD_RESP_COM_CRC_ERROR   = 0x08,
	SD_RESP_ERASE_SEQ_ERROR = 0x10,
	SD_RESP_ADDRESS_ERROR   = 0x20,
	SD_RESP_PARAMETER_ERROR = 0x40,
	SD_RESP_START_BIT_0		= 0x80,
	SD_RESP_NO_RESPONSE		= 0xFF
} ATTR_PACKED eSdCommandResponse_t;

typedef enum eSdIfCondVoltageSupplied_t {
	IF_COND_VOLTAGE_27V_36V		= 0x01,
	IF_COND_VOLTAGE_LOW_VOLTAGE = 0x02
} ATTR_PACKED eSdIfCondVoltageSupplied_t;

typedef enum eSdOpCond_t {
	OP_COND_HIGH_CAPACITY_SUPPORT = 0x40000000,
} ATTR_PACKED eSdOpCond_t;

typedef enum eSdOCRRegister_t {
	SD_OCR_27_28		   = 0x00008000,
	SD_OCR_28_29		   = 0x00010000,
	SD_OCR_29_30		   = 0x00020000,
	SD_OCR_30_31		   = 0x00040000,
	SD_OCR_31_32		   = 0x00080000,
	SD_OCR_32_33		   = 0x00100000,
	SD_OCR_33_34		   = 0x00200000,
	SD_OCR_34_35		   = 0x00400000,
	SD_OCR_35_36		   = 0x00800000,
	SD_OCR_27_36		   = 0x00FF8000,
	SD_OCR_UHS_STATUS	  = 0x20000000,
	SD_OCR_CCS			   = 0x40000000,
	SD_OCR_POWER_UP_STATUS = 0x80000000
} ATTR_PACKED eSdOCRRegister_t;

typedef struct xSdParameters_t
{
	eSdCardType_t eCardType;
	uint32_t	  ulDeviceSizeMB;
	uint32_t	  ulBlockSize;
	uint32_t	  ulNumBlocks;
	uint8_t		  ucEraseByte;
} xSdParameters_t;

/* Function Declarations ------------------------------------*/

void vSdLLInit( xSpiModule_t *pxModule, xGpio_t xCs );

void vSdLLWakeSequence( void );

eModuleError_t eSdCommand( eSdCommand_t eCommand, uint32_t ulArgument, uint8_t *pucResponse );

eModuleError_t eSdReadRegister( uint8_t *pucBuffer, uint32_t ulBufferLen );

eModuleError_t eSdReadBytes( uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen );

eModuleError_t eSdWriteBytes( xSdParameters_t *pxParams, uint16_t usBlockOffset, uint8_t *pucBuffer, uint32_t ulBufferLen );

void vSdParseCSD( uint8_t *pucCSD, xSdParameters_t *pxParams );

void vSdParseSCR( uint8_t *pucCSR, xSdParameters_t *pxParams );

void vSdPrintCID( uint8_t *pucCID );

#endif /* __CSIRO_CORE_SD_LL */
