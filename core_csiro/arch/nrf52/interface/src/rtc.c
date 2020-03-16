/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#include "cpu.h"
#include "memory_operations.h"
#include "rtc.h"

#include "nrf.h"
#include "nrfx.h"

/* Private Defines ------------------------------------------*/

#define UINT24_MAX 0x00FFFFFF

#define NUM_ALARMS 3

// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xAlarmInfo_t
{
	SemaphoreHandle_t xAlarm;
	StaticSemaphore_t xAlarmStorage;
	uint32_t		  ulAlarmBit;
	uint32_t		  ulIterations;
	fnAlarmCallback_t fnCallback;
	uint8_t			  ucCompareIndex;
} xAlarmInfo_t;

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

static uint32_t	ulStoredEpochTime = 0;
static xDateTime_t xStoredCalendar   = { 0 };
static uint64_t	ullTickCounter	= 0;
STATIC_SEMAPHORE_STRUCTURES( pxHeartbeat );

static xAlarmInfo_t xAlarms[NUM_ALARMS];

/*-----------------------------------------------------------*/

void vRtcInit( void )
{
	const IRQn_Type xIRQn = nrfx_get_irq_number( NRF_RTC2 );
	STATIC_SEMAPHORE_CREATE_BINARY( pxHeartbeat );

	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		xAlarms[i].xAlarm = xSemaphoreCreateBinaryStatic( &xAlarms[i].xAlarmStorage );
		/* Indexing starts at bit 17 as bit 16 is COMPARE0 which is reserved for Calendar use */
		xAlarms[i].ucCompareIndex = i + 1;
		xAlarms[i].ulAlarmBit	 = ( 0x01 << ( 16 + i + 1 ) );
		xAlarms[i].ulIterations   = 0;
	}

	/* Clear RTC */
	NRF_RTC2->TASKS_CLEAR = 0x01;
	/* Setup Events */
	NRF_RTC2->EVTENSET = RTC_EVTEN_COMPARE0_Msk | RTC_EVTEN_OVRFLW_Msk;
	NRF_RTC2->INTENSET = RTC_INTENSET_COMPARE0_Msk | RTC_INTENSET_OVRFLW_Msk;
	/* Initialise the Compare register */
	NRF_RTC2->CC[0] = 32768;
	/* Configure interrupts */
	vInterruptSetPriority( xIRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptEnable( xIRQn );
	/* Start the timer */
	NRF_RTC2->TASKS_START = 0x01;

	/* Set the default system time to just before 2016 */
	xDateTime_t xValidDatetime = { .xDate = { .usYear = 2015, .eMonth = eDecember, .ucDay = 31, .eDayOfWeek = eUnknownDay },
								   .xTime = { .ucHour = 23, .ucMinute = 59, .ucSecond = 55 } };
	eRtcSetDatetime( &xValidDatetime );
	return;
}

/*-----------------------------------------------------------*/

uint64_t ullRtcTickCount( void )
{
	return ullTickCounter + NRF_RTC2->COUNTER;
}

/*-----------------------------------------------------------*/

void vRtcHeartbeatWait( void )
{
	configASSERT( xSemaphoreTake( pxHeartbeat, portMAX_DELAY ) == pdPASS );
}

/*-----------------------------------------------------------*/

bool bRtcGetEpochTime( eTimeEpoch_t eEpoch, uint32_t *pulEpochTime )
{
	uint32_t ulTime = ulStoredEpochTime;
	bool	 bValid = ulTime > ( SECONDS_FROM_UNIX_EPOCH_TO_2015 + ( 3 * SECONDS_IN_1_YEAR ) );
	switch ( eEpoch ) {
		case eUnixEpoch:
			break;
		case e2000Epoch:
			ulTime -= SECONDS_FROM_UNIX_EPOCH_TO_2000;
			break;
		case e2015Epoch:
			ulTime -= SECONDS_FROM_UNIX_EPOCH_TO_2015;
			break;
		default:
			configASSERT( 0 );
	}
	*pulEpochTime = ulTime;
	return bValid;
}

/*-----------------------------------------------------------*/

bool bRtcGetDate( xDate_t *pxDate )
{
	pvMemcpy( pxDate, &xStoredCalendar.xDate, sizeof( xDate_t ) );
	return bRtcDateIsValid( pxDate );
}

/*-----------------------------------------------------------*/

void vRtcGetTime( xTime_t *pxTime )
{
	pvMemcpy( pxTime, &xStoredCalendar.xTime, sizeof( xTime_t ) );
}

/*-----------------------------------------------------------*/

bool bRtcGetDatetime( xDateTime_t *pxDatetime )
{
	bool bValidDate;

	pvMemcpy( pxDatetime, &xStoredCalendar, sizeof( xDateTime_t ) );
	/* ANDED with 0x7FFF to make it look like a typical 0-32767 RTC counter value */
	pxDatetime->xTime.usSecondFraction = 0x7FFF & NRF_RTC2->COUNTER;

	bValidDate = bRtcDateIsValid( &pxDatetime->xDate );

	return bValidDate;
}

/*-----------------------------------------------------------*/

eModuleError_t eRtcSetDatetime( xDateTime_t *pxDateTime )
{
	if ( eValidateDatetime( pxDateTime ) != ERROR_NONE ) {
		return ERROR_INVALID_DATA;
	}
	pvMemcpy( &xStoredCalendar, pxDateTime, sizeof( xDateTime_t ) );
	xStoredCalendar.xDate.eDayOfWeek = eRtcDayOfWeek( &xStoredCalendar.xDate );
	vRtcDateTimeToEpoch( pxDateTime, 0, &ulStoredEpochTime );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

SemaphoreHandle_t xRtcAlarmSetup( uint32_t ulTicksUntil, fnAlarmCallback_t fnCallback )
{
	CRITICAL_SECTION_DECLARE;
	uint32_t		  ulIterations = ( ulTicksUntil / UINT24_MAX ) + 1;
	SemaphoreHandle_t xAlarm	   = NULL;
	CRITICAL_SECTION_START();
	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		if ( xAlarms[i].ulIterations == 0 ) {
			xAlarms[i].ulIterations					= ulIterations;
			xAlarms[i].fnCallback					= fnCallback;
			xAlarm									= xAlarms[i].xAlarm;
			NRF_RTC2->CC[xAlarms[i].ucCompareIndex] = ( NRF_RTC2->COUNTER + ulTicksUntil ) & UINT24_MAX;
			NRF_RTC2->EVTENSET						= xAlarms[i].ulAlarmBit;
			NRF_RTC2->INTENSET						= xAlarms[i].ulAlarmBit;
			break;
		}
	}
	CRITICAL_SECTION_STOP();
	return xAlarm;
}

/*-----------------------------------------------------------*/

uint16_t usRtcSubsecond( void )
{
	return ( uint16_t )( NRF_RTC2->COUNTER & 0x7FFF );
}

/*-----------------------------------------------------------*/

void RTC2_IRQHandler( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	/* Handle our RTC compare event */
	if ( NRF_RTC2->EVENTS_COMPARE[0] ) {
		NRF_RTC2->EVENTS_COMPARE[0] = 0x00;
		NRF_RTC2->CC[0] += 32768;
		ulStoredEpochTime++;
		vRtcIncrementDateTime( &xStoredCalendar );
		xSemaphoreGiveFromISR( pxHeartbeat, &xHigherPriorityTaskWoken );
	}
	/* Handle our alarm events */
	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		if ( NRF_RTC2->EVENTS_COMPARE[xAlarms[i].ucCompareIndex] ) {
			NRF_RTC2->EVENTS_COMPARE[xAlarms[i].ucCompareIndex] = 0x00;
			if ( --xAlarms[i].ulIterations == 0 ) {
				NRF_RTC2->INTENCLR = xAlarms[i].ulAlarmBit;
				NRF_RTC2->EVTENCLR = xAlarms[i].ulAlarmBit;
				xSemaphoreGiveFromISR( xAlarms[i].xAlarm, &xHigherPriorityTaskWoken );
				if ( xAlarms[i].fnCallback != NULL ) {
					xAlarms[i].fnCallback();
				}
			}
		}
	}
	/* Handle overflows */
	if ( NRF_RTC2->EVENTS_OVRFLW ) {
		NRF_RTC2->EVENTS_OVRFLW = 0x00;
		ullTickCounter += UINT24_MAX;
	}
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
