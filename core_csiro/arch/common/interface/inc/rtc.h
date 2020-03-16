/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: template_header.h
 * Creation_Date: 2/6/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * CSIRO Real Time Counter Interface.
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_RTC
#define __CORE_CSIRO_INTERFACE_RTC

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "log.h"
#include "tdf.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define SECONDS_FROM_UNIX_EPOCH_TO_2000		946684800lu
#define SECONDS_FROM_UNIX_EPOCH_TO_2015		1420070400lu

#define SECONDS_IN_4_YEARS 					126230400lu
#define SECONDS_IN_1_YEAR  					31536000lu
#define SECONDS_IN_1_DAY   					86400lu
#define SECONDS_IN_1_HR    					3600lu
#define SECONDS_IN_1_MIN   					60lu

#define YEARS_TO_SECONDS( years )			( ( years ) * SECONDS_IN_1_YEAR )
#define DAYS_TO_SECONDS( days )				( ( days ) * SECONDS_IN_1_DAY )
#define HOURS_TO_SECONDS( hours )			( ( hours ) * SECONDS_IN_1_HR )
#define MINUTES_TO_SECONDS( mins )			( ( mins ) * SECONDS_IN_1_MIN )

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum {
	eUnixEpoch,
	e2000Epoch,
	e2015Epoch,
	eInvalidEpoch
} eTimeEpoch_t;

typedef enum {
	eJanuary   = 1,
	eFebruary  = 2,
	eMarch	 = 3,
	eApril	 = 4,
	eMay	   = 5,
	eJune	  = 6,
	eJuly	  = 7,
	eAugust	= 8,
	eSeptember = 9,
	eOctober   = 10,
	eNovember  = 11,
	eDecember  = 12
} eMonth_t;

typedef enum {
	eSunday		= 0,
	eMonday		= 1,
	eTuesday	= 2,
	eWednesday  = 3,
	eThursday   = 4,
	eFriday		= 5,
	eSaturday   = 6,
	eUnknownDay = 255
} eDay_t;

typedef enum {
	eSundayBits	= 0b0000001,
	eMondayBits	= 0b0000010,
	eTuesdayBits   = 0b0000100,
	eWednesdayBits = 0b0001000,
	eThursdayBits  = 0b0010000,
	eFridayBits	= 0b0100000,
	eSaturdayBits  = 0b1000000,
	eAllDayBits	= 0b1111111,
} eDayBits_t;

typedef struct
{
	uint16_t usYear;
	eMonth_t eMonth;
	uint8_t  ucDay; // 1 - 31
	eDay_t   eDayOfWeek;
} ATTR_PACKED xDate_t;

typedef struct
{
	uint8_t  ucHour;		   /**< Hour within the day [0,23] */
	uint8_t  ucMinute;		   /**< Minute within the hour [0,59] */
	uint8_t  ucSecond;		   /**< Second within the minute [0,59] */
	uint16_t usSecondFraction; /**< RTC clock tick with a second [0,32767] */
} ATTR_PACKED xTime_t;

typedef struct
{
	xDate_t xDate; /**< UTC Date */
	xTime_t xTime; /**< UTC Time */
} ATTR_PACKED xDateTime_t;

typedef void ( *fnAlarmCallback_t )( void );

/* Function Declarations ------------------------------------*/

/**
 * Initialise RTC module
 */
void vRtcInit( void );

/**
 * Retrieve the total internal tick count of the RTC
 * \return Ticks since RTC started
 */
uint64_t ullRtcTickCount( void );

/**
 * Retrieve the current subsecond count of the RTC
 * \return RTC Prescaler value [0 - 32767]
 */
uint16_t usRtcSubsecond( void );

/**
 * Wait until the next heartbeat tick (1 second period)
 * DO NOT CALL FROM APPLICATION CODE
 */
void vRtcHeartbeatWait( void );

/**
 * Start an alarm on the RTC
 * \param ulTicksUntil Alarm will be fired in this many ticks
 * \param fnCallback   Custom function called from alarm interrupt handler
 * \return Semaphore handle to wait on for alarm, NULL if alarm was not setup
 */
SemaphoreHandle_t xRtcAlarmSetup( uint32_t ulTicksUntil, fnAlarmCallback_t fnCallback );

/**
 * Prints the curent time on an arbitrary log with arbitrary importance
 * \param eLog The log this time is associated with
 * \param eLevel The log level for this time
 * \param pcPrefix String to print before the timestamp e.g. "Time: "
 * \param pcPostfix String to print after the timestamp e.g. "\r\n"
 * \return eModuleError specifying error, typically ERROR_TIMEOUT
 */
eModuleError_t eRtcPrintDatetime( xDateTime_t *pxDatetime, SerialLog_t eLog, LogLevel_t eLevel, const char *pcPrefix, const char *pcPostfix );

/**
 * Sets the RTC to equal the datetime object passed
 * Basic sanity checking of input ranges is performed
 * No checking for bRtcDateIsValid is performed
 * \param xDatetime The datetime to set the RTC to
 * \return eModuleError specifying error, typically ERROR_INVALID_DATA
 */
eModuleError_t eRtcSetDatetime( xDateTime_t *pxDatetime );

/**
 * Basic sanity check to see if the RTC time could be the actual time
 * Currently checks to see if the year is greater than 2018
 * \param xDate The date to check validity of
 * \return True if date could be valid
 */
bool bRtcDateIsValid( xDate_t *pxDate );

/**
 * Get the current date
 * \param xDate Pointer to location to store date in
 * \return True if date could be valid
 */
bool bRtcGetDate( xDate_t *pxDate );

/**
 * Get the current time
 * \param xTime Pointer to location to store time in
 */
void vRtcGetTime( xTime_t *pxTime );

/**
 * Get the current date and time
 * \param xDatetime Pointer to location to store datetime in
 * \return True if date could be valid
 */
bool bRtcGetDatetime( xDateTime_t *pxDatetime );

/**
 * Gets the current epoch time
 * \param eEpoch 		Start time to reference the time against
 * \param ulEpochTime 	Pointer to location to store timestamp in
 * \return True if timestamp could be valid
 */
bool bRtcGetEpochTime( eTimeEpoch_t eEpoch, uint32_t *pulEpochTime );

/**
 * Get the day of week for a given time
 * \param xDate Date to convert
 * \return Day of week
 */
eDay_t eRtcDayOfWeek( xDate_t *pxDate );

/**
 * Get the bitfield corresponding to the current day of the week
 * \param xDate Date to convert
 * \return Day of week bitfield
 */
eDayBits_t eRtcDayOfWeekBits( xDate_t *pxDate );

/**
 * Convert a datetime to a given epoch
 * \param pxDateTime	Calendar time to convert
 * \param eEpoch 		Start time to reference our seconds offset against
 * \param ulEpochTime 	Pointer to location to store timestamp in
 * \return True if date could be valid
 */
void vRtcDateTimeToEpoch( xDateTime_t *pxDatetime, eTimeEpoch_t eEpoch, uint32_t *pulEpochTime );

/**
 * Convert an Epoch time to a Calendar struct
 * \param eEpoch 		Start time to reference our seconds offset against
 * \param ulEpochTime 	Seconds offset to convert
 * \param pxDatetime 	Pointer to datetime struct to store result
 */
void vRtcEpochToDateTime( eTimeEpoch_t eEpoch, uint32_t ulEpochTime, xDateTime_t *pxDatetime );

/**
 * Get the current TDF time (Epoch2000 with seconds fraction)
 * \param pxTdfTime Pointer to location to store TDF time
 * \return True if date could be valid
 */
bool bRtcGetTdfTime( xTdfTime_t *pxTdfTime );

xTdfTime_t xRtcTdfTimeAdd( xTdfTime_t xBase, xTdfTime_t xAddition );

xTdfTime_t xRtcTdfTimeSub( xTdfTime_t xBase, xTdfTime_t xSubtraction );

/**
 * Increment a Datetime struct by 1 second
 * \param pxDateTime The struct to increment
 */
void vRtcIncrementDateTime( xDateTime_t *pxDateTime );

/**
 * Check if the provided year is a Leap Year
 * \param usYear Complete year, e.g. 2018
 * \returns True if leap year, false otherwise
 */
bool bRtcIsLeapYear( uint16_t usYear );

/**
 * Get the elapsed number of days in this year to date.
 * \param pxDate: Pointer to the date to use to calculate this.
 * \param pusDays: Pointer to data location to store answer.
 * \return ERROR_NONE on success, ERROR_DEFAULT_VALUE on error.
 */
eModuleError_t eElapsedDaysInYear( xDate_t *pxDate, uint16_t *pusDays );

/**
 * Determine if a datetime is a valid time and data
 * \param pxDatetime: Datetime to evaluate.
 * \return ERROR_NONE if valid, ERROR_INVALID_DATA on error.
 */
eModuleError_t eValidateDatetime( xDateTime_t *pxDatetime );

#endif /* __CORE_CSIRO_INTERFACE_RTC */
