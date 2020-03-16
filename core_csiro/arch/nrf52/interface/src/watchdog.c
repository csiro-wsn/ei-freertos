/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "log.h"
#include "memory_operations.h"
#include "nrfx_wdt.h"
#include "rtc.h"
#include "tdf.h"
#include "watchdog.h"

#include "nrf_soc.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

static WatchdogReboot_t xWatchdogRebootValues ATTR_SECTION( ".noinit" );

/*-----------------------------------------------------------*/

void vWatchdogInit( xWatchdogModule_t *pxWdog )
{
	UNUSED( pxWdog );
	/* We only want to run the watchdog on release builds */
#ifdef RELEASE_BUILD
	nrfx_wdt_config_t xConfig = NRFX_WDT_DEAFULT_CONFIG;
	nrfx_wdt_init( &xConfig, pxWdog->vIRQ );

	/* allocate a channel and store the channel ID in xHandle */
	nrfx_wdt_channel_alloc( pxWdog->xHandle );
	nrfx_wdt_enable();
#else
	pxWdog->ulWatchdogPeriodRtcTicks = ( 2000 * 32768 ) / 1000;
	pxWdog->ullSoftwareLastCount	 = ullRtcTickCount();
#endif /* RELEASE_BUILD */
}

/*-----------------------------------------------------------*/

void vWatchdogPeriodic( xWatchdogModule_t *pxWdog )
{
#ifdef RELEASE_BUILD
	/* Feed the hardware watchdog */
	nrfx_wdt_channel_feed( *( pxWdog->xHandle ) );
#else
	/* Check that the periodic call has been called quickly enough */
	uint64_t ullTickNow = ullRtcTickCount();
	uint32_t ulTickDiff = ullTickNow - pxWdog->ullSoftwareLastCount;
	configASSERT( ulTickDiff <= pxWdog->ulWatchdogPeriodRtcTicks );
	pxWdog->ullSoftwareLastCount = ullTickNow;
#endif /* RELEASE_BUILD */
}

/*-----------------------------------------------------------*/

WatchdogReboot_t *xWatchdogRebootReason( void )
{
	xDateTime_t xDatetime;
	uint32_t	ulRebootReason;
	/* Query Power Register for reset reason */
	sd_power_reset_reason_get( &ulRebootReason );
	/* GPREG's are reset on a watchdog reboot, so check for that condition first */
	if ( ulRebootReason & POWER_RESETREAS_DOG_Msk ) {
		xWatchdogRebootValues.eRebootReason = REBOOT_WATCHDOG;
		sd_power_reset_reason_clr( POWER_RESETREAS_DOG_Msk );
	}
	/* Software Reset */
	else if ( ulRebootReason & POWER_RESETREAS_SREQ_Msk ) {
		/* Query GPREG for more accurate reason */
		sd_power_gpregret_get( 0, &ulRebootReason );
		sd_power_gpregret_clr( 0, UINT8_MAX );
		xWatchdogRebootValues.eRebootReason = (eWatchdogRebootReason_t) ulRebootReason;
	}
	else {
		xWatchdogRebootValues.eRebootReason = REBOOT_UNKNOWN;
	}

	/* If we rebooted for a known reason, load the saved RTC time */
	if ( ( xWatchdogRebootValues.eRebootReason != REBOOT_UNKNOWN ) && ( xWatchdogRebootValues.ulWatchdogKey == WATCHDOG_KEY_VALUE ) ) {
		vRtcEpochToDateTime( e2000Epoch, xWatchdogRebootValues.xRebootTime.ulSecondsSince2000 + 1, &xDatetime );
		eRtcSetDatetime( &xDatetime );
	}

	/* Set the watchdog key to be invalid */
	xWatchdogRebootValues.ulWatchdogKey = 0;

	if ( xWatchdogRebootValues.eRebootReason == REBOOT_UNKNOWN ) {
		return NULL;
	}
	else {
		return &xWatchdogRebootValues;
	}
}

/*-----------------------------------------------------------*/

void vWatchdogSetRebootReason( eWatchdogRebootReason_t eReason, const char *pcTask, uint32_t ulProgramCounter, uint32_t ulLinkRegister )
{
	uint8_t i = 0;
	/* Store the task name in .noinit variable */
	while ( ( pcTask[i] != '\0' ) && ( i < configMAX_TASK_NAME_LEN ) ) {
		xWatchdogRebootValues.cTaskName[i] = pcTask[i];
		i++;
	}
	xWatchdogRebootValues.cTaskName[i] = '\0';
	/* Store PC and LR */
	xWatchdogRebootValues.ulProgramCounter = ulProgramCounter;
	xWatchdogRebootValues.ulLinkRegister   = ulLinkRegister;
	/* Store the current time */
	bRtcGetTdfTime( &xWatchdogRebootValues.xRebootTime );
	xWatchdogRebootValues.ulWatchdogKey = WATCHDOG_KEY_VALUE;
	/* Store the reboot reason */
	sd_power_gpregret_set( 0, (uint32_t) eReason );
}

/*-----------------------------------------------------------*/
