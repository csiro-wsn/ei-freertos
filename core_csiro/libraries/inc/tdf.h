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
 * DAMAGES (INCLUDING, BUT NOT LIM1ITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the FreeRTOS operating system.
 *
 * Filename: tdf.h
 * Description: Formats data into tdf3 packets and passes them to the relevent storage media.
 * Creation_Date: 05/06/12
 * Author: Phil Valencia <Philip.Valencia@csiro.au>, Luke Hovington <Luke.Hovington@csiro.au>
 *
 * Modifier: John Scolaro <John.Scolaro@data61.csiro.au>.
 * Modified for FreeRTOS on 17/08/2018.
 */

#ifndef __CORE_CSIRO_TDF_H__
#define __CORE_CSIRO_TDF_H__

#include "FreeRTOS.h"
#include "semphr.h"

#include "compiler_intrinsics.h"
#include "logger.h"

#include "null_logger.h"

#include "tdf_auto.h"
#include "tdf_struct.h"

/* Macros ---------------------------------------------------*/

#define TDF_LOGGER_STRUCTURES( mask, name, pucDescription, pxLoggerDevice, ulBlockSize, ulStartBlock, ulNumBlocks ) \
	LOGGER( mask, name##_log, pucDescription, pxLoggerDevice, ulBlockSize, ulStartBlock, ulNumBlocks );             \
	xTdfLogger_t name = { &name##_log, NULL, { TDF_INVALID_TIME, 0 }, { 0, 0 } }

/* Defines --------------------------------------------------*/
// clang-format off

#define TDF_LOGGER_CONFIG_MASK				0x40000000
#define TDF_LOGGER_CONFIG_CHECK_TIME_NOT_BEFORE (TDF_LOGGER_CONFIG_MASK | 0 ) /* Option to not add if time is old */

#define TDF_TIMESTAMP_MASK 						0xC000	// 11000000 00000000
#define TDF_ID_MASK 							0x0FFF	// 00001111 11111111

#define TDF_ID(x) 					((x) & TDF_ID_MASK)
#define TDF_TIMESTAMP(x) 			((x) & TDF_TIMESTAMP_MASK)

/* Logger bit definitions */
#define NONE_LOG				0x00
#define TDF_TIMESTAMP_AUTO  	0x01
#define LONG_RANGE_LOG      	0x02
#define NETSTACK_LOG        	0x04
#define ONBOARD_STORAGE_LOG 	0x08
#define EXTERNAL_STORAGE_LOG 	0x10
#define BLE_LOG             	0x20
#define SERIAL_LOG   			0x40
#define PLACEHOLDER_LOG_4   	0x80

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eTdfTimestampType_t {
	TDF_TIMESTAMP_NONE				 = 0x0000, // 0000xxxx xxxxxxxx
	TDF_TIMESTAMP_RELATIVE_OFFSET_MS = 0x8000, // 0100xxxx xxxxxxxx
	TDF_TIMESTAMP_RELATIVE_OFFSET_S  = 0x4000, // 1000xxxx xxxxxxxx
	TDF_TIMESTAMP_GLOBAL			 = 0xC000  // 1100xxxx xxxxxxxx
} eTdfTimestampType_t;

#define TDF_INVALID_TIME 0xFFFFFFFF // This will be reserved as an invalid (uninitialised) time (for the secondsSince2000 field)

/* This is the TDF3 format for global time. Needs to be packed to use 6 bytes, not 8. */
typedef struct xTdfTime_t
{
	uint32_t ulSecondsSince2000;
	uint16_t usSecondsFraction; // Units in 1/65536th of a second
} ATTR_PACKED xTdfTime_t;

typedef struct xTdfLogger_t
{
	xLogger_t *		  pxLog;
	SemaphoreHandle_t xTdfSemaphore;
	xTdfTime_t		  xBufferTime;
	xTdfTime_t		  xValidAfterTime;
} xTdfLogger_t;

/* More Macros ----------------------------------------------*/

// Enumerates a list of active TDF loggers
#define TDF_LOGS( ... )                                                                                  \
	TDF_LOGGER_STRUCTURES( 0x00, xNullLog, "NullLog", (xLoggerDevice_t *) &xNullLoggerDevice, 0, 0, 0 ); \
	const xTdfLogger_t *tdf_logs[]	 = { __VA_ARGS__, NULL };                                          \
	uint8_t				TDF_LOGGER_NUM = sizeof( tdf_logs ) / sizeof( xTdfLogger_t * )

/* Scheduler Related Definitions ----------------------------*/

typedef uint8_t xActivityTdfsMask_t;

typedef struct ATTR_PACKED xTdfLoggersMask_t
{
	uint8_t timestamp : 1;
	uint8_t loggers_mask : 7;
} xTdfLoggersMask_t;

#define NUM_LOGGERS_MASK 4

typedef struct ATTR_PACKED xTdfLoggerMapping_t
{
	/* TDF logging */
	xActivityTdfsMask_t xActivityTdfsMask;
	xTdfLoggersMask_t   pxTdfLoggersMask[NUM_LOGGERS_MASK];
} xTdfLoggerMapping_t;

/* Functions Declarations------------------------------------*/

/**@brief Returns a pointer to a TDF logger given the logger mask.
 *
 * @param[in] ucLoggerMask		A logger mask. Selected from one of the logger
 * 								bit definitions.
 * 
 * @retval pxTdfLogger:			A pointer to the referenced TDF logger
 */
xTdfLogger_t *pxTdfLoggerGet( uint8_t ucLoggerMask );

/**@brief Configure the TDF logger into a specific mode.
 *
 * @param[in] pxTDFlog			A pointer to the log we wish to configure.
 * @param[in] iSetting			The thing we wish to configure about that log.
 * @param[in] pvConfValue		Setting specific data.
 * 
 * @retval ::ERROR_NONE 		Logger configured successfully.
 */
eModuleError_t eTdfLoggerConfigure( xTdfLogger_t *pxTDFlog, int iSetting, void *pvConfValue );

/**@brief Adds TDF data to an arbitrary buffer.
 *
 * @param[in] pvBuffer			Arbitrary buffer to add data too.
 * @param[in] usTdfId			The timestamp type and TDF type being added.
 * @param[in] pxGlobalTime		A pointer to the timestamp. NULL is no timestamp being used.
 * @param[in] usTdfDataLength	The length of the data
 * @param[in] pucTdfData		A pointer to the data being added.
 * @param[in] ucBufferSize		The size of the profided buffer.
 * @param[in] pvBuffer			The provided buffer.
 * 
 * @retval 						Bytes added to buffer
 */
uint16_t usTdfAddToBuffer( eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxTimestamp, void *pucTdfData, uint8_t ucBufferSize, void *pvBuffer );

/**@brief Add data to a TDF log.
 *
 * @param[in] pxTDFlog			A pointer to the TDF log to add data to.
 * @param[in] usTdfId			The timestamp type and TDF type being added.
 * @param[in] pxGlobalTime		A pointer to the timestamp. NULL is no timestamp being used.
 * @param[in] usDataLength		The length of the data
 * @param[in] pucData			A pointer to the data being added.
 * 
 * @retval ::ERROR_NONE 		TDF added successfully.
 */
eModuleError_t eTdfAdd( xTdfLogger_t *pxTdfLog, eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, void *pucData );

/**@brief Add data to multiple TDF logs.
 *
 * @param[in] ucLoggerMask		The logger mask, containing one more more logs
 * 								from the logger bit definitions.
 * @param[in] usTdfId			The timestamp type and TDF type being added.
 * @param[in] pxGlobalTime		A pointer to the timestamp. NULL is no timestamp being used.
 * @param[in] usDataLength		The length of the data
 * @param[in] pucData			A pointer to the data being added. 
 * 
 * @retval ::ERROR_NONE 		TDF's all added succesfully.
 */
eModuleError_t eTdfAddMulti( uint8_t ucLoggerMask, eTdfIds_t eTdfId, eTdfTimestampType_t eTimestampType, xTdfTime_t *pxGlobalTime, void *pucData );

/**@brief Flush data to the log from a TDF log.
 *
 * @param[in] pxTDFlog			A pointer to the TDF log to flush.
 * 
 * @retval ::ERROR_NONE 		TDF log flushed successfully.
 * 
 * When eTdfAdd is called, data is added to a log in RAM, and only transferred
 * to the storage medium / send over some interface when the log is full.
 * Calling eTdfFlash flushes the log immediately.
 */
eModuleError_t eTdfFlush( xTdfLogger_t *pxTDFlog );

/**@brief Flush data from multiple TDF logs.
 *
 * @param[in] ucLoggerMask		A logger mask. Selected from one of the logger
 * 								bit definitions.
 * 
 * @retval ::ERROR_NONE 		All logs flushed successfully.
 */
eModuleError_t eTdfFlushMulti( uint8_t ucLoggerMask );

/**@brief Optionally add a TDF to a logger as specified by the schedule of an activity.
 *
 * @param[in] pxLoggerMapping	A pointer to the TdfLoggerMapping struct in an
 * 								activities schedule.
 * @param[in] ucTdfMask			A logger mask. Selected from one of the logger
 * 								bit definitions.
 * @param[in] usTdfId			The timestamp type and TDF type being added.
 * @param[in] pxGlobalTime		A pointer to the timestamp. NULL is no timestamp being used.
 * @param[in] usDataLen			The length of the data
 * @param[in] pucData			A pointer to the data being added. 
 * 
 * @retval ::ERROR_NONE 		Data added to TDF log successfully.
 */
eModuleError_t eTdfSchedulerArgsParse( xTdfLoggerMapping_t *pxLoggerMapping, uint8_t ucTdfMask, eTdfIds_t eTdfId, xTdfTime_t *pxGlobalTime, void *pucData );

#endif // __CORE_CSIRO_TDF_H__
