/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "memory_operations.h"
#include "rtc.h"

/* Private Defines ------------------------------------------*/
// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static const char *pucDaysOfWeek[] = {
	[eSunday]	= "Sun",
	[eMonday]	= "Mon",
	[eTuesday]   = "Tue",
	[eWednesday] = "Wed",
	[eThursday]  = "Thu",
	[eFriday]	= "Fri",
	[eSaturday]  = "Sat"
};

static const uint8_t pucMonthLengths[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*-----------------------------------------------------------*/

eModuleError_t eValidateDatetime( xDateTime_t *xDatetime )
{
	if ( !VALUE_IN_RANGE( 2000, xDatetime->xDate.usYear, 2099 ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( !VALUE_IN_RANGE( eJanuary, xDatetime->xDate.eMonth, eDecember ) ) {
		return ERROR_INVALID_DATA;
	}
	const uint8_t ucMonthLength = pucMonthLengths[xDatetime->xDate.eMonth - 1];
	if ( !VALUE_IN_RANGE( 1, xDatetime->xDate.ucDay, ucMonthLength ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( !VALUE_IN_RANGE( 0, xDatetime->xTime.ucHour, 23 ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( !VALUE_IN_RANGE( 0, xDatetime->xTime.ucMinute, 59 ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( !VALUE_IN_RANGE( 0, xDatetime->xTime.ucSecond, 59 ) ) {
		return ERROR_INVALID_DATA;
	}
	if ( !VALUE_IN_RANGE( 0, xDatetime->xTime.usSecondFraction, 32767 ) ) {
		return ERROR_INVALID_DATA;
	}

	/* Special validity check on February for non leap years */
	if ( !bRtcIsLeapYear( xDatetime->xDate.usYear ) ) {
		if ( xDatetime->xDate.eMonth == 2 ) {
			if ( xDatetime->xDate.ucDay >= 29 ) {
				return ERROR_INVALID_DATA;
			}
		}
	}
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

bool bRtcDateIsValid( xDate_t *pxDate )
{
	/* Time is valid if year is greater than or equal to 2019 */
	return ( pxDate->usYear >= 2019 );
}

/*-----------------------------------------------------------*/

bool bRtcGetTdfTime( xTdfTime_t *pxTdfTime )
{
	uint32_t ulSeconds;
	/* Seconds count is not loaded into struct directly due to alignemnt issues */
	bool bValid					  = bRtcGetEpochTime( e2000Epoch, &ulSeconds );
	pxTdfTime->ulSecondsSince2000 = ulSeconds;
	pxTdfTime->usSecondsFraction  = ( usRtcSubsecond() << 1 );
	return bValid;
}

/*-----------------------------------------------------------*/

xTdfTime_t xRtcTdfTimeAdd( xTdfTime_t xBase, xTdfTime_t xAddition )
{
	xTdfTime_t xResult;
	if ( xAddition.usSecondsFraction >= ( UINT16_MAX - xBase.usSecondsFraction ) ) {
		/* Addition would overflow */
		xResult.ulSecondsSince2000 = xBase.ulSecondsSince2000 + xAddition.ulSecondsSince2000 + 1;
		xResult.usSecondsFraction  = xBase.usSecondsFraction - ( UINT16_MAX - xAddition.usSecondsFraction );
	}
	else {
		/* No overflow */
		xResult.ulSecondsSince2000 = xBase.ulSecondsSince2000 + xAddition.ulSecondsSince2000;
		xResult.usSecondsFraction  = xBase.usSecondsFraction + xAddition.usSecondsFraction;
	}
	return xResult;
}

/*-----------------------------------------------------------*/

xTdfTime_t xRtcTdfTimeSub( xTdfTime_t xBase, xTdfTime_t xSubtraction )
{
	xTdfTime_t xResult;
	if ( xSubtraction.usSecondsFraction > xBase.usSecondsFraction ) {
		/* Subtraction would underflow */
		xResult.ulSecondsSince2000 = xBase.ulSecondsSince2000 - xSubtraction.ulSecondsSince2000 - 1;
		xResult.usSecondsFraction  = UINT16_MAX - ( xSubtraction.usSecondsFraction - xBase.usSecondsFraction );
	}
	else {
		/* No underflow */
		xResult.ulSecondsSince2000 = xBase.ulSecondsSince2000 - xSubtraction.ulSecondsSince2000;
		xResult.usSecondsFraction  = xBase.usSecondsFraction - xSubtraction.usSecondsFraction;
	}
	return xResult;
}

/*-----------------------------------------------------------*/

void vRtcDateTimeToEpoch( xDateTime_t *pxDatetime, eTimeEpoch_t eEpoch, uint32_t *pulEpochTime )
{
	uint16_t usDays;
	/**
	 * Using the calculation from here:
	 * http://pubs.opengroup.org/onlinepubs/009695399/basedefs/xbd_chap04.html#tag_04_14
	 * to find the number of seconds since epoch.
	 **/

	eElapsedDaysInYear( &( pxDatetime->xDate ), &usDays );

	*pulEpochTime = pxDatetime->xTime.ucSecond;
	*pulEpochTime += pxDatetime->xTime.ucMinute * 60;
	*pulEpochTime += pxDatetime->xTime.ucHour * 3600;
	*pulEpochTime += usDays * 86400;
	*pulEpochTime += ( pxDatetime->xDate.usYear - 1900 - 70 ) * 31536000;
	*pulEpochTime += ( ( pxDatetime->xDate.usYear - 1900 - 69 ) / 4 ) * 86400;
	*pulEpochTime -= ( ( pxDatetime->xDate.usYear - 1900 - 1 ) / 100 ) * 86400;
	*pulEpochTime += ( ( pxDatetime->xDate.usYear - 1900 + 299 ) / 400 ) * 86400;

	switch ( eEpoch ) {
		case eUnixEpoch:
			break;
		case e2000Epoch:
			*pulEpochTime -= SECONDS_FROM_UNIX_EPOCH_TO_2000;
			break;
		case e2015Epoch:
			*pulEpochTime -= SECONDS_FROM_UNIX_EPOCH_TO_2015;
			break;
		default:
			configASSERT( 0 );
	}
}

/*-----------------------------------------------------------*/

void vRtcEpochToDateTime( eTimeEpoch_t eEpoch, uint32_t ulEpochTime, xDateTime_t *pxDatetime )
{
	uint8_t  ucEpochYearsToLeapYear = 0;
	bool	 bIsLeapYear			= false;
	uint32_t ulBlocks;
	pvMemset( pxDatetime, 0, sizeof( xDateTime_t ) );

	/* Calculate how many years into the Epoch the leap year occurs */
	/* Also apply the year offset while we're at it */
	switch ( eEpoch ) {
		case eUnixEpoch:
			pxDatetime->xDate.usYear = 1970;
			ucEpochYearsToLeapYear   = 2;
			break;
		case e2000Epoch:
			pxDatetime->xDate.usYear = 2000;
			ucEpochYearsToLeapYear   = 0;
			break;
		case e2015Epoch:
			pxDatetime->xDate.usYear = 2015;
			ucEpochYearsToLeapYear   = 1;
			break;
		default:
			configASSERT( 0 );
	}

	/* 4 Year Blocks*/
	ulBlocks = ( ulEpochTime / SECONDS_IN_4_YEARS );
	ulEpochTime -= ulBlocks * SECONDS_IN_4_YEARS;
	pxDatetime->xDate.usYear += 4 * ulBlocks;
	/* 1 Year Blocks */
	ulBlocks = ( ulEpochTime / SECONDS_IN_1_YEAR );
	ulEpochTime -= ulBlocks * SECONDS_IN_1_YEAR;
	pxDatetime->xDate.usYear += ulBlocks;
	/* Take a day off our seconds count if we hit the Epoch cycles leap year and its not this year */
	if ( ulBlocks > ucEpochYearsToLeapYear ) {
		ulEpochTime -= SECONDS_IN_1_DAY;
	}
	/* Store if this year is a leap year */
	else if ( ulBlocks == ucEpochYearsToLeapYear ) {
		bIsLeapYear = true;
	}
	eMonth_t eMonth					 = 0;
	uint8_t  ucLocalMonthLengths[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	ucLocalMonthLengths[1] += bIsLeapYear ? 1 : 0;
	/* Loop over months because its easier */
	while ( ulEpochTime >= SECONDS_IN_1_DAY * ucLocalMonthLengths[eMonth] ) {
		ulEpochTime -= SECONDS_IN_1_DAY * ucLocalMonthLengths[eMonth];
		eMonth++;
	}
	pxDatetime->xDate.eMonth = eMonth + 1;
	/* Days in the month */
	ulBlocks = ( ulEpochTime / SECONDS_IN_1_DAY );
	ulEpochTime -= ulBlocks * SECONDS_IN_1_DAY;
	pxDatetime->xDate.ucDay = ulBlocks + 1;
	/* Hour of the day */
	ulBlocks = ( ulEpochTime / SECONDS_IN_1_HR );
	ulEpochTime -= ulBlocks * SECONDS_IN_1_HR;
	pxDatetime->xTime.ucHour = ulBlocks;
	/* Minute of the hour */
	ulBlocks = ( ulEpochTime / SECONDS_IN_1_MIN );
	ulEpochTime -= ulBlocks * SECONDS_IN_1_MIN;
	pxDatetime->xTime.ucMinute = ulBlocks;
	/* Second of the minute */
	pxDatetime->xTime.ucSecond = ulEpochTime;
}

/*-----------------------------------------------------------*/

bool bRtcIsLeapYear( uint16_t usYear )
{
	/* Leap Years are evenly divisable by 4, and not 100, except for when they are divisible by 400 */
	return ( ( usYear % 400 ) == 0 ) || ( ( ( usYear % 4 ) == 0 ) && ( ( usYear % 100 ) != 0 ) );
}

/*-----------------------------------------------------------*/

void vRtcIncrementDateTime( xDateTime_t *pxDateTime )
{
	if ( ++pxDateTime->xTime.ucSecond < 60 )
		return;
	pxDateTime->xTime.ucSecond = 0;
	if ( ++pxDateTime->xTime.ucMinute < 60 )
		return;
	pxDateTime->xTime.ucMinute = 0;
	if ( ++pxDateTime->xTime.ucHour < 24 )
		return;
	pxDateTime->xTime.ucHour = 0;
	if ( ++pxDateTime->xDate.eDayOfWeek > eSaturday )
		pxDateTime->xDate.eDayOfWeek = eSunday;
	uint8_t ucLeapYearOffset = ( ( pxDateTime->xDate.eMonth == 2 ) && bRtcIsLeapYear( pxDateTime->xDate.usYear ) ) ? 0 : 1;
	uint8_t ucMonthLen		 = pucMonthLengths[pxDateTime->xDate.eMonth - 1] - ucLeapYearOffset;
	if ( ++pxDateTime->xDate.ucDay <= ucMonthLen )
		return;
	pxDateTime->xDate.ucDay = 1;
	if ( ++pxDateTime->xDate.eMonth <= 12 )
		return;
	pxDateTime->xDate.eMonth = eJanuary;
	++pxDateTime->xDate.usYear;
}

/*-----------------------------------------------------------*/

// Sakamoto's method from https://en.wikipedia.org/wiki/Determination_of_the_day_of_the_week
// Sunday = 0, Monday = 1 etc...
// 1 <= m <= 12,  y > 1752 (in the U.K.)
eDay_t eRtcDayOfWeek( xDate_t *xDate )
{
	uint16_t	   usYear = xDate->usYear;
	static uint8_t t[]	= { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
	usYear -= xDate->eMonth < 3;
	/* Handily this algorithm has the representation as eDay_t*/
	return ( ( usYear + usYear / 4 - usYear / 100 + usYear / 400 + t[xDate->eMonth - 1] + xDate->ucDay ) % 7 );
}

/*-----------------------------------------------------------*/

eDayBits_t eRtcDayOfWeekBits( xDate_t *pxDate )
{
	eDay_t eDay = eRtcDayOfWeek( pxDate );
	return ( 0b1 << (uint8_t) eDay );
}

/*-----------------------------------------------------------*/

/**
 * Calculates the number of elapsed days in the current year using the calendar
 * date. Takes into account leap years.
 **/
eModuleError_t eElapsedDaysInYear( xDate_t *pxDate, uint16_t *pusDays )
{
	uint16_t usDays = 0;

	switch ( pxDate->eMonth ) {
		case 12:
			usDays += 30; // Number of days in November
			ATTR_FALLTHROUGH;
		case 11:
			usDays += 31; // Number of days in October
			ATTR_FALLTHROUGH;
		case 10:
			usDays += 30; // Number of days in September
			ATTR_FALLTHROUGH;
		case 9:
			usDays += 31; // Number of days in August
			ATTR_FALLTHROUGH;
		case 8:
			usDays += 31; // Number of days in July
			ATTR_FALLTHROUGH;
		case 7:
			usDays += 30; // Number of days in June
			ATTR_FALLTHROUGH;
		case 6:
			usDays += 31; // Number of days in May
			ATTR_FALLTHROUGH;
		case 5:
			usDays += 30; // Number of days in April
			ATTR_FALLTHROUGH;
		case 4:
			usDays += 31; // Number of days in March
			ATTR_FALLTHROUGH;
		case 3:
			if ( pxDate->usYear % 100 == 0 ) {	 // Every 100th year actually isnt a leap year.
				if ( pxDate->usYear % 400 == 0 ) { // Unless it's every 400th year.
					usDays += 29;
				}
				else {
					usDays += 28;
				}
			}
			else if ( pxDate->usYear % 4 == 0 ) { // If it's a regular leap year.
				usDays += 29;
			}
			else { // If it's just a normal year.
				usDays += 28;
			}
			ATTR_FALLTHROUGH;
		case 2:
			usDays += 31; // Number of days in January
			ATTR_FALLTHROUGH;
		case 1:
			usDays += pxDate->ucDay;
			break;
		default:
			return ERROR_DEFAULT_CASE;
	}

	*pusDays = usDays - 1;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t eRtcPrintDatetime( xDateTime_t *xDatetime, SerialLog_t eLogger, LogLevel_t eLevel, const char *pcPrefix, const char *pcPostfix )
{
	const eDay_t eDayOfWeek  = xDatetime->xDate.eDayOfWeek;
	uint32_t	 usSubSecond = xDatetime->xTime.usSecondFraction;
	/* While still developing, print out the actual prescalar counter */

	// usSubSecond				  = ( usSubSecond * 1000 ) / 32768;

	eLog( eLogger, eLevel, "%s%s %02d/%02d/%02d %02d:%02d:%02d.%05d%s",
		  pcPrefix,
		  pucDaysOfWeek[eDayOfWeek],
		  xDatetime->xDate.ucDay,
		  xDatetime->xDate.eMonth,
		  xDatetime->xDate.usYear,
		  xDatetime->xTime.ucHour,
		  xDatetime->xTime.ucMinute,
		  xDatetime->xTime.ucSecond,
		  usSubSecond,
		  pcPostfix );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/
