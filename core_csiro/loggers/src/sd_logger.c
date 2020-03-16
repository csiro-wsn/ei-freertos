/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "sd_logger.h"
#include "log.h"
#include "logger.h"
#include "sd.h"

#include "FreeRTOS.h"

#include "cpu_arch.h"
#include "leds.h"
#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

/**
 *  Default behaviour:
 *  	Log pages directly to SD upon receive
 *  Alternate behaviour:
 * 		Buffers groups of pages and writes them to flash all at once
 * 		Write is triggered when SD_RAM_BUFFER_FLUSH_PERCENTAGE of the RAM buffer is filled
 * 		eTdfLoggerConfigure( &xSdLog, LOGGER_CONFIG_SD_RAM_BUFFER, NULL ) should be added in vApplicationStartupCallback() to create dump task
 * 		The below defines should be set in "FreeRTOSConfigApp.h" if alternate functionality is desired
 **/
#ifndef SD_RAM_BUFFER_PAGES
#define SD_RAM_BUFFER_PAGES 1
#endif
#ifndef SD_RAM_BUFFER_FLUSH_PERCENTAGE
#define SD_RAM_BUFFER_FLUSH_PERCENTAGE 90
#endif

#define RAM_BUFFER_INDEX_INCREMENT(x) (((x) + 1) % SD_RAM_BUFFER_PAGES)

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvSdDumpTask( void *pvParameters );

static inline uint32_t ulBufferItems( void );

/* Private Variables ----------------------------------------*/

STATIC_TASK_STRUCTURES( pxSdDumpTask, configMINIMAL_STACK_SIZE, tskIDLE_PRIORITY + 1 );

static bool		bSdRamBuffer = false;
static bool		bDumpRunning = false;
static uint8_t  pucRamBuffer[SD_RAM_BUFFER_PAGES * 512];
static uint16_t usRamBufferHead;
static uint16_t usRamBufferTail;
static uint32_t ulRamBufferBasePage = UINT32_MAX;

/* Functions ------------------------------------------------*/

static eModuleError_t eConfigure( uint16_t usSetting, void *pvParameters )
{
	xSdParameters_t xParameters;
	eSdParameters( &xParameters );

	switch ( usSetting ) {
		case LOGGER_CONFIG_GET_CLEAR_BYTE:
			*( (uint8_t *) pvParameters ) = xParameters.ucEraseByte;
			break;
		case LOGGER_CONFIG_GET_NUM_BLOCKS:
			*( (uint32_t *) pvParameters ) = xParameters.ulNumBlocks;
			break;
		case LOGGER_CONFIG_GET_ERASE_UNIT:
			*( (uint8_t *) pvParameters ) = 1;
			break;
		case LOGGER_CONFIG_SD_RAM_BUFFER:
			STATIC_TASK_CREATE( pxSdDumpTask, prvSdDumpTask, "SD Dump", NULL );
			bSdRamBuffer	= true;
			bDumpRunning	= false;
			usRamBufferHead = 0;
			usRamBufferTail = 0;
		default:
			break;
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eStatus( uint16_t usType )
{
	UNUSED( usType );
	// We don't really have a method of getting the SD card status.
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static eModuleError_t eReadBlock( uint32_t ulBlockNum, uint16_t usOffset, void *pvBlockData, uint32_t ulBlockSize )
{
	uint16_t usBlockSize = ulBlockSize;
	configASSERT( ulBlockSize != 0 );
	configASSERT( pvBlockData != NULL );

	return eSdBlockRead( ulBlockNum, usOffset, pvBlockData, usBlockSize, pdMS_TO_TICKS( 1000 ) );
}

/*-----------------------------------------------------------*/

static eModuleError_t eWriteBlock( uint32_t ulBlockNum, void *pvBlockData, uint32_t ulBlockSize )
{
	CRITICAL_SECTION_DECLARE;
	uint16_t usBlockSize = ulBlockSize;

	configASSERT( ulBlockSize != 0 );
	configASSERT( pvBlockData != NULL );

	/* If RAM buffering is enabled */
	if ( ( SD_RAM_BUFFER_PAGES > 1 ) && bSdRamBuffer ) {
		/* Initialise our base page on the first page we are saving */
		if ( ulRamBufferBasePage == UINT32_MAX ) {
			ulRamBufferBasePage = ulBlockNum;
		}
		eLog( LOG_LOGGER, LOG_INFO, "SD Log: Buffering in RAM Buffer index %d\r\n", usRamBufferHead );
		pvMemcpy( pucRamBuffer + usRamBufferHead * 512, pvBlockData, ulBlockSize );
		/* Critical section so that ( usRamBufferHead + 1 ) is never seen by the dump task before the modulo */
		CRITICAL_SECTION_START();
		usRamBufferHead = RAM_BUFFER_INDEX_INCREMENT( usRamBufferHead );
		CRITICAL_SECTION_STOP();
		COMPILER_WARNING_DISABLE( "-Wtype-limits" );
		if ( !bDumpRunning && ( ulBufferItems() >= ( ( SD_RAM_BUFFER_FLUSH_PERCENTAGE * SD_RAM_BUFFER_PAGES ) / 100 ) ) ) {
			/* Variable is set before the notify for the case where dump task is higher priority than the calling task */
			bDumpRunning = true;
			xTaskNotify( pxSdDumpTask, 0, eSetValueWithOverwrite );
		}
		COMPILER_WARNING_ENABLE();
	}
	/* If no RAM buffering, write it directly */
	else {
		return eSdBlockWrite( ulBlockNum, 0, pvBlockData, usBlockSize, pdMS_TO_TICKS( 1000 ) );
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

static inline uint32_t ulBufferItems( void )
{
	return ( ( usRamBufferHead > usRamBufferTail ) ? 0 : SD_RAM_BUFFER_PAGES ) + usRamBufferHead - usRamBufferTail;
}

/*-----------------------------------------------------------*/

ATTR_NORETURN void prvSdDumpTask( void *pvParameters )
{
	uint32_t ulPagesDumped;
	UNUSED( pvParameters );

	eLog( LOG_LOGGER, LOG_VERBOSE, "SD Log: Dump task created\r\n" );
	while ( 1 ) {
		/* Value is irrelvant, only used as a trigger to start dump */
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		ulPagesDumped = 0;
		/* Write pages until the tail matches the head */
		while ( usRamBufferTail != usRamBufferHead ) {
			eLog( LOG_LOGGER, LOG_VERBOSE, "SD Log:  Dump writing to page %d from buffer offset %d\r\n", ulRamBufferBasePage, ( 512 * usRamBufferTail ) );
			eSdBlockWrite( ulRamBufferBasePage, 0, pucRamBuffer + ( 512 * usRamBufferTail ), 512, pdMS_TO_TICKS( 1000 ) );
			ulRamBufferBasePage++;
			ulPagesDumped++;
			usRamBufferTail = RAM_BUFFER_INDEX_INCREMENT( usRamBufferTail );
		}
		/* TODO: A time taken in ms would be nice additional information for future */
		eLog( LOG_LOGGER, LOG_INFO, "SD Log: Wrote %d pages\r\n", ulPagesDumped );

		bDumpRunning = false;
	}
}

/*-----------------------------------------------------------*/

static eModuleError_t ePrepareBlock( uint32_t ulBlockNum )
{
	return eSdEraseBlocks( ulBlockNum, ulBlockNum, pdMS_TO_TICKS( 1000 ) );
}

/*-----------------------------------------------------------*/

LOGGER_DEVICE( xSdLoggerDevice, eConfigure, eStatus, eReadBlock, eWriteBlock, ePrepareBlock );

/*-----------------------------------------------------------*/
