/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "rtc.h"
#include "cpu.h"

#include "em_device.h"

#include "em_cmu.h"
#include "em_rtcc.h"

#include "memory_operations.h"

/* Private Defines ------------------------------------------*/
// clang-format off

#define RTCC_SEC_ONES_OFFSET        		0
#define RTCC_SEC_TENS_OFFSET        		4
#define RTCC_MIN_ONES_OFFSET        		8
#define RTCC_MIN_TENS_OFFSET        		12
#define RTCC_HOUR_ONES_OFFSET       		16
#define RTCC_HOUR_TENS_OFFSET       		20

#define RTCC_SEC_ONES_MASK          		0x0000000F
#define RTCC_SEC_TENS_MASK         			0x00000070
#define RTCC_MIN_ONES_MASK          		0x00000F00
#define RTCC_MIN_TENS_MASK          		0x00007000
#define RTCC_HOUR_ONES_MASK         		0x000F0000
#define RTCC_HOUR_TENS_MASK         		0x00300000

#define RTCC_DAY_ONES_OFFSET        		0
#define RTCC_DAY_TENS_OFFSET        		4
#define RTCC_MONTH_ONES_OFFSET      		8
#define RTCC_MONTH_TENS_OFFSET      		12
#define RTCC_YEAR_ONES_OFFSET       		16
#define RTCC_YEAR_TENS_OFFSET       		20
#define RTCC_DAY_OF_WEEK_OFFSET     		24

#define RTCC_DAY_ONES_MASK          		0x0000000F
#define RTCC_DAY_TENS_MASK          		0x00000030
#define RTCC_MONTH_ONES_MASK        		0x00000F00
#define RTCC_MONTH_TENS_MASK        		0x00001000
#define RTCC_YEAR_ONES_MASK         		0x000F0000
#define RTCC_YEAR_TENS_MASK         		0x00F00000
#define RTCC_DAY_OF_WEEK_MASK       		0x07000000

#define RTCC_PRESCALER_MASK         		0x00007FFF

#define NUM_ALARMS	3

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xAlarmInfo_t
{
	SemaphoreHandle_t xAlarm;
	StaticSemaphore_t xAlarmStorage;
	uint32_t		  ulAlarmBit;
	fnAlarmCallback_t fnCallback;
	bool			  bInUse;
} xAlarmInfo_t;

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/* RTCC configuration structures. */
static const RTCC_Init_TypeDef xRTCInitStruct = {
	false,				/* Don't start counting immediately */
	true,				/* Run RTCC during debug halt. */
	false,				/* Don't care. */
	false,				/* Don't care. */
	rtccCntPresc_32768, /* NOTE:  Do not use a pre-scale if errata RTCC_E201 applies. */
	rtccCntTickPresc,   /* Count using the clock input directly. */
	false,				/* Oscillator fail detection disabled. */
	rtccCntModeNormal,  /* Use RTCC in counter mode */
	false				/* Don't disable Leap Year Correction */
};

static xDateTime_t xStoredCalendar = { { 0 }, { 0 } };
static uint64_t	ullTickCounter  = 0;
STATIC_SEMAPHORE_STRUCTURES( pxHeartbeat );

static xAlarmInfo_t xAlarms[NUM_ALARMS];

/*-----------------------------------------------------------*/

void vRtcInit( void )
{
	STATIC_SEMAPHORE_CREATE_BINARY( pxHeartbeat );

	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		xAlarms[i].xAlarm	 = xSemaphoreCreateBinaryStatic( &xAlarms[i].xAlarmStorage );
		xAlarms[i].ulAlarmBit = ( 0x01 << ( 1 + i ) );
		xAlarms[i].bInUse	 = false;
	}

	/* Ensure LE modules are accessible. */
	CMU_ClockEnable( cmuClock_CORELE, true );
	/* Use LFXO. */
	CMU_ClockSelectSet( cmuClock_LFE, cmuSelect_LFXO );
	/* Enable clock to the RTC module. */
	CMU_ClockEnable( cmuClock_RTCC, true );

	vInterruptDisable( RTCC_IRQn );
	vInterruptSetPriority( RTCC_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptClearPending( RTCC_IRQn );

	RTCC_Init( &xRTCInitStruct );

	RTCC_CounterSet( 0 );
	RTCC_PreCounterSet( 0 );
	RTCC_IntDisable( UINT32_MAX );
	RTCC_IntClear( UINT32_MAX );
	RTCC_IntEnable( RTCC_IEN_CNTTICK );

	RTCC_Enable( true );
	vInterruptEnable( RTCC_IRQn );

	/* Set the default system time to just before 2016 */
	xDateTime_t xValidDatetime = { .xDate = { .usYear = 2015, .eMonth = eDecember, .ucDay = 31, .eDayOfWeek = eUnknownDay },
								   .xTime = { .ucHour = 23, .ucMinute = 59, .ucSecond = 55 } };
	eRtcSetDatetime( &xValidDatetime );
}

/*-----------------------------------------------------------*/

uint64_t ullRtcTickCount( void )
{
	return ullTickCounter + RTCC_PreCounterGet();
}

/*-----------------------------------------------------------*/

void vRtcHeartbeatWait( void )
{
	configASSERT( xSemaphoreTake( pxHeartbeat, portMAX_DELAY ) == pdPASS );
}

/*-----------------------------------------------------------*/

bool bRtcGetEpochTime( eTimeEpoch_t eEpoch, uint32_t *pulEpochTime )
{
	uint32_t ulTime = RTCC_CounterGet();
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
	pxTime->usSecondFraction = (uint16_t) RTCC_PreCounterGet();
}

/*-----------------------------------------------------------*/

bool bRtcGetDatetime( xDateTime_t *pxDatetime )
{
	pvMemcpy( pxDatetime, &xStoredCalendar, sizeof( xDateTime_t ) );
	pxDatetime->xTime.usSecondFraction = (uint16_t) RTCC_PreCounterGet();
	return bRtcDateIsValid( &pxDatetime->xDate );
}

/*-----------------------------------------------------------*/

eModuleError_t eRtcSetDatetime( xDateTime_t *pxDateTime )
{
	if ( eValidateDatetime( pxDateTime ) != ERROR_NONE ) {
		return ERROR_INVALID_DATA;
	}

	RTCC_Enable( false );

	pvMemcpy( &xStoredCalendar, pxDateTime, sizeof( xDateTime_t ) );
	xStoredCalendar.xDate.eDayOfWeek = eRtcDayOfWeek( &xStoredCalendar.xDate );
	/* Get the UNIX time corresponding to the new time */
	uint32_t ulEpochTime;
	vRtcDateTimeToEpoch( pxDateTime, eUnixEpoch, &ulEpochTime );
	/* Use the UNIX time as out counter value */
	RTCC_CounterSet( ulEpochTime );
	RTCC_PreCounterSet( pxDateTime->xTime.usSecondFraction );

	RTCC_Enable( true );

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

SemaphoreHandle_t xRtcAlarmSetup( uint32_t ulTicksUntil, fnAlarmCallback_t fnCallback )
{
	CRITICAL_SECTION_DECLARE;
	const uint32_t ulCCControl  = RTCC_CC_CTRL_COMPBASE_PRECNT | RTCC_CC_CTRL_MODE_OUTPUTCOMPARE;
	const uint32_t ulExpiryTime = RTCC_CombinedCounterGet() + ulTicksUntil;

	SemaphoreHandle_t xAlarm = NULL;
	CRITICAL_SECTION_START();
	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		if ( !xAlarms[i].bInUse ) {
			xAlarms[i].bInUse	 = true;
			xAlarm				  = xAlarms[i].xAlarm;
			xAlarms[i].fnCallback = fnCallback;
			RTCC_IntEnable( xAlarms[i].ulAlarmBit );
			RTCC->CC[i].CCV  = ulExpiryTime;
			RTCC->CC[i].CTRL = ulCCControl;
			break;
		}
	}
	CRITICAL_SECTION_STOP();
	return xAlarm;
}

/*-----------------------------------------------------------*/

uint16_t usRtcSubsecond( void )
{
	return (uint16_t) RTCC_PreCounterGet();
}

/*-----------------------------------------------------------*/

void RTCC_IRQHandler( void )
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	uint32_t   ulInterrupts				= RTCC_IntGet();
	if ( ulInterrupts & RTCC_IF_CNTTICK ) {
		ullTickCounter += 32768;
		vRtcIncrementDateTime( &xStoredCalendar );
		xSemaphoreGiveFromISR( pxHeartbeat, &xHigherPriorityTaskWoken );
	}
	for ( uint8_t i = 0; i < NUM_ALARMS; i++ ) {
		if ( ulInterrupts & xAlarms[i].ulAlarmBit ) {
			RTCC_IntDisable( xAlarms[i].ulAlarmBit );
			RTCC->CC[i].CTRL  = 0x00;
			xAlarms[i].bInUse = false;
			xSemaphoreGiveFromISR( xAlarms[i].xAlarm, &xHigherPriorityTaskWoken );
			if ( xAlarms[i].fnCallback != NULL ) {
				xAlarms[i].fnCallback();
			}
		}
	}
	RTCC_IntClear( ulInterrupts );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/
