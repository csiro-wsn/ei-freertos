/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: log.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Implements logging functionality over a serial backend
 *
 */

#ifndef __CORE_CSIRO_UTIL_SERIAL_LOGGER
#define __CORE_CSIRO_UTIL_SERIAL_LOGGER

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "serial_interface.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/

/**
 * This list of loggers should probably come from the pacp-server
 * in a similar fashion to TDF's
 **/
typedef enum SerialLog_t {
	LOG_APPLICATION = 0x00,
	/* System Interface Logs */
	LOG_UART,
	LOG_SPI,
	LOG_I2C,
	LOG_WATCHDOG,
	LOG_RTC,
	LOG_NVM,
	LOG_ADC,
	/* Library Logs */
	LOG_SCHEDULER,
	LOG_LOGGER,
	LOG_RPC,
	LOG_RESULT,
	/* Device Driver Logs */
	LOG_BLUETOOTH_GAP,
	LOG_BLUETOOTH_GATT,
	LOG_SIGFOX_DRIVER,
	LOG_GLOBALSTAR_DRIVER,
	LOG_FLASH_DRIVER,
	LOG_IMU_DRIVER,
	LOG_GPS_DRIVER,
	LOG_TEMPERATURE_DRIVER,
	LOG_SD_DRIVER,
	/* High Level Activity Logs */
	LOG_ACTIVITY_GPS,
	LOG_ACTIVITY_IMU,
	LOG_ACTIVITY_LED,
	LOG_ACTIVITY_LR_RADIO,
	LOG_ACTIVITY_SR_RADIO,
	LOG_ACTIVITY_POWER,
	LOG_ACTIVITY_ENVIRONMENTAL,
	LOG_ACTIVITY_INERTIAL_CLASSIFIER,
	LOG_ACTIVITY_PACKET_CONSTRUCTOR,
	LOG_MODULE_LAST
} SerialLog_t;

/**@brief Output logging levels */
typedef enum LogLevel_t {
	LOG_APOCALYPSE = 0x00, /**< Unmaskable output, will always be sent*/
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG,
	LOG_VERBOSE,
	LOG_LEVEL_LAST
} LogLevel_t;

typedef struct xLogBuilder_t
{
	SerialLog_t eLog;
	char *		pcString;
	uint32_t	ulIndex;
	uint32_t	ulMaxLen;
	bool		bValid;
} xLogBuilder_t;

/* Function Declarations ------------------------------------*/

/**@brief Reset all logs to their default levels
 *
 * @note	The default log level is LOG_ERROR
 */
void vLogResetLogLevels( void );

/**@brief Get the current log level associated with a log channel
 *
 * @param[in] eLog				Log channel to query
 * 
 * @retval	::LOG_LEVEL_LAST	eLog was invalid
 * @retval	::eLogLevel			Log level for eLog
 */
LogLevel_t xLogGetLogLevel( SerialLog_t eLog );

/**@brief Set the log level associated with a log channel
 *
 * @param[in] eLog				Log channel to update
 * @param[in] eLevel			New log level
 * 
 * @retval	::ERROR_INVALID_LOGGER		eLog was invalid
 * @retval	::ERROR_INVALID_LOG_LEVEL	eLevel was invalid
 * @retval	::ERROR_NONE				Log level set
 */
eModuleError_t eLogSetLogLevel( SerialLog_t eLog, LogLevel_t eLevel );

/**@brief Logger Output
 * 
 *		Outputs the provided message over the previously configured backend
 *		Message is only output if the log level for eLog is greater than the provided eLevel
 *
 * @param[in] eLog				Log channel to output on
 * @param[in] eLevel			Level of the message
 * @param[in] pcFormat			Format string of message
 * @param[in] ...				Variadic arguments containing format string parameters
 * 
 * @retval	::ERROR_INVALID_LOGGER		eLog was invalid
 * @retval	::ERROR_INVALID_LOG_LEVEL	eLevel was invalid
 * @retval	::ERROR_NONE				Message output
 */
eModuleError_t eLog( SerialLog_t eLog, LogLevel_t eLevel, const char *pcFormat, ... );

eModuleError_t eLogBuilderStart( xLogBuilder_t *pxBuilder, SerialLog_t eLog );

eModuleError_t eLogBuilderPush( xLogBuilder_t *pxBuilder, LogLevel_t eLevel, const char *pcFormat, ... );

eModuleError_t eLogBuilderFinish( xLogBuilder_t *pxBuilder );

#endif /*__CORE_CSIRO_UTIL_SERIAL_LOGGER */
