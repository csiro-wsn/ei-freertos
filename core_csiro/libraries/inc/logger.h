/*
 * Copyright (c) 2012, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file should be part of the FreeRTOS operating system.
 *
 * Filename: logger.h
 * Creation_Date: 04/07/2012
 * Description: Interface for creating a double buffered abstraction to the
 *   underlying storage device for saving a stream of bytes.
 * Author: Phil Valencia <Philip.Valencia@csiro.au>, Luke Hovington <Luke.Hovington@csiro.au>
 * 
 * Modified by John Scolaro <John.Scolaro@data61.csiro.au> on 16/8/2018 for FreeRTOS.
 *
 */

#ifndef __CORE_CSIRO_LOGGER_H__
#define __CORE_CSIRO_LOGGER_H__

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include "log.h"

/* Predeclarations ----------------------------------------------------------*/

struct xLogger_t;
struct xLoggerDevice_t;

/* Module Defines -------------------------------------------*/

/** 
 * These are all macros for creating loggers and the required variables for
 * using them.
 * 
 * LOGGER_DEVICE creates a constant struct containing function pointers to
 * functions implimenting basic logger functionalities like reading and
 * writing. It is implimented in each device_logger file, (EG: sd_logger.c)
 * and declared extern in their header files. (EG: sd_logger.h).
 * 
 * LOGGER creates a logical logger. Creates all the data structures required
 * for the logger and the double buffer to use for it.
 * 
 * LOGS creates a list of logical loggers.
 */
#define LOGGER_DEVICE( name, fnConfigure, fnStatus, fnReadBlock, fnWriteBlock, fnPrepareBlock ) \
	const xLoggerDevice_t name = { fnConfigure, fnStatus, fnReadBlock, fnWriteBlock, fnPrepareBlock }

#define LOGGER( mask, name, pucDescription, pxLoggerDevice, ulBlockSize, ulStartBlock, ulNumBlocks ) \
	static uint8_t   name##_buffer[2 * ulBlockSize];                                                 \
	static xLogger_t name = { mask, pucDescription, pxLoggerDevice, ulBlockSize, 0, 0, 0, ulStartBlock, ulNumBlocks, 0x00, 0, 0, 0, name##_buffer }

#define LOGS( ... )                                      \
	xLogger_t *const logs[]		= { __VA_ARGS__, NULL }; \
	uint8_t			 LOGGER_NUM = ( sizeof( logs ) / sizeof( xLogger_t * ) );

#define LOGGER_LENGTH_REMAINING_BLOCKS UINT32_MAX

/** 
 * Conversions between number of completed wraps and the number stored on the first byte of the page
 * Physical wrap number cannot match the erase byte of the log
 **/
#define PHYSICAL_WRAP_NUMBER( log ) ( ( log )->ucWrapCounter + ( ( log )->ucClearByte == 0x00 ? 1 : 0 ) )
#define LOGICAL_WRAP_NUMBER( log, physical ) ( ( physical ) - ( ( log )->ucClearByte == 0x00 ? 1 : 0 ) )

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eLoggerConfigureOptions_t {
	LOGGER_CONFIG_INIT_DEVICE,
	LOGGER_CONFIG_CLEAR_UNUSED_BYTES,	 /* Clear the unused bytes in the buffer */
	LOGGER_CONFIG_COMMIT_ONLY_USED_BYTES, /* Only commit used bytes in the buffer */
	LOGGER_CONFIG_APPEND_MODE,			  /* Append data to log, default is to overwrite */
	LOGGER_CONFIG_WRAP_MODE,			  /* Wrap around to the start if the log is full, default is not to */
	LOGGER_CONFIG_GET_NUM_BLOCKS,		  /* Get the number of blocks the device can store */
	LOGGER_CONFIG_GET_CLEAR_BYTE,		  /* Get the byte that erase operations set to */
	LOGGER_CONFIG_GET_ERASE_UNIT,		  /* Get the byte that erase operations set to */
	LOGGER_CONFIG_END
} eLoggerConfigureOptions_t;

typedef enum eLoggerFlags_t {
	LOGGER_FLAG_DEVICE_INITIALISED	 = 0x01, /* Set if the underlying device hardware is initialised */
	LOGGER_FLAG_CLEAR_UNUSED_BYTES	 = 0x02, /* If set then unused buffer bytes are set to the clear_byte value */
	LOGGER_FLAG_COMMIT_ONLY_USED_BYTES = 0x04, /* If set then only used bytes are committed */
	LOGGER_FLAG_WRAPPING_ON			   = 0x08, /* If set then the logger will wrap around to the start page and continue writing */
} eLoggerFlags_t;

typedef enum eLoggerSearchOptions_t {
	LOGGER_SEARCH_NOT_MATCH		= 0x01,
	LOGGER_SEARCH_BINARY_SEARCH = 0x02,
} eLoggerSearchOptions_t;

typedef enum eLoggerSearchResults_t {
	LOGGER_SEARCH_NO_MATCH	= 1,
	LOGGER_SEARCH_FOUND_MATCH = 2,
} eLoggerSearchResults_t;

typedef enum eLoggerStatus_t {
	LOGGER_STATUS_BLOCKS_WRITTEN = 0, /* Used to request the number of blocks written to the device */
	LOGGER_STATUS_NUM_BLOCKS	 = 1,
	LOGGER_STATUS_WRAP_COUNT	 = 2,
	LOGGER_STATUS_DEVICE_STATUS /* Used to send the device a statuc pointer */
} eLoggerStatus_t;

/**
 * This struct represents a logical logger device.
 * 
 * There can be multiple logical loggers for each physical device.
 */
typedef struct xLogger_t
{
	uint8_t						  ucUniqueMask;			 /* TDF mask that is passed in to activate the loggers you want a tdf sent to */
	const char *				  pucDescription;		 /* A text description of the device */
	const struct xLoggerDevice_t *pxLoggerDevice;		 /* All the handles and information for the logger task which is controlling the physical device. */
	uint16_t					  usLogicalBlockSize;	/* Block size in bytes. Note: 2 buffers of this size need to be created */
	uint8_t						  ucCurrentBuffer;		 /* Used to keep track of which one is the current buffer (internal use only) */
	uint16_t					  usBufferByteOffset;	/* Byte offset within the current buffer */
	uint32_t					  ulCurrentBlockAddress; /* Offset in logical blocks for the next free block of this device. For SD cards this is the current page we are writing too. It counts upwards.*/
	uint32_t					  ulStartBlockAddress;   /* Offset in physical blocks for the first logical block of this device. */
	uint32_t					  ulNumBlocks;			 /* Number of logical blocks in this logger */
	uint8_t						  ucClearByte;			 /* The byte to fill unused bytes in a logical block if LOGGER_CLEAR_UNUSED_BYTES flag is set */
	uint8_t						  ucWrapCounter;		 /* Stores the number of wraps when LOGGER_FLAG_WRAPPING_ON flag is set */
	uint32_t					  ulPagesWritten;		 /* The number of pages written in this log */
	uint8_t						  ucFlags;				 /* Used to store various settings. Set using the eLoggerConfigure function */
	uint8_t *					  pucBuffer;			 /* This a pointer to an array of size 2 * usLogicalBlockSize */
} xLogger_t;

/** 
 * There will be only one log_device abstraction for each physical device.
 * A device needs to implement these functions and construct an xLoggerDevice_t
 * struct with pointers to the appropriate functions.
 */
typedef struct xLoggerDevice_t
{
	eModuleError_t ( *fnConfigure )( uint16_t usSetting, void *pvParameters );
	eModuleError_t ( *fnStatus )( uint16_t usType );																		 /*  A way to read some interesting stats and status of the underlying device hardware */
	eModuleError_t ( *fnReadBlock )( uint32_t ulBlockNum, uint16_t usBlockOffset, void *pvBlockData, uint32_t ulBlockSize ); /*  Copies a logical block of data out to the supplied pointer location */
	eModuleError_t ( *fnWriteBlock )( uint32_t ulBlockNum, void *pvBlockData, uint32_t ulBlockSize );						 /*  Copies a logical block of data onto the device a the specified block number */
	eModuleError_t ( *fnPrepareBlock )( uint32_t ulBlockNum );																 /*  Prepares the given block for writing (Erasing sectors etc) */
} xLoggerDevice_t;

/* Function Declarations ------------------------------------*/

// Adds data to buffer and flushes if full. Returns immediately.
eModuleError_t eLoggerLog( xLogger_t *pxLog, uint16_t usNumBytes, void *pvLogData );
// Reads data back from logger. Blocks until complete.
eModuleError_t eLoggerReadBlock( xLogger_t *pxLog, uint32_t ulBlockNum, uint16_t usBlockOffset, void *pvBlockData );
// Flushes buffer. Returns immediately.
eModuleError_t eLoggerCommit( xLogger_t *pxLog );
// Configures logger and/or device.
eModuleError_t eLoggerConfigure( xLogger_t *pxLog, uint16_t usSetting, void *pvConfValue );
// Gets logger and/or device status.
eModuleError_t eLoggerStatus( xLogger_t *pxLog, uint16_t usType, void *pvStatus );
// Used to find information within loggers.
eModuleError_t eLoggerSearch( xLogger_t *pxLog, uint16_t usNumBytes, uint8_t *pucMatchData, uint8_t ucSearchFlags, uint32_t *pulBlockNum );
// Output current logger info
void vLoggerPrint( xLogger_t *pxLog, SerialLog_t eLog, LogLevel_t eLevel );

#endif // __CORE_CSIRO_LOGGER_H__
