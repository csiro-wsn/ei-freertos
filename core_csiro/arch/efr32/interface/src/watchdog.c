/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

#include "watchdog.h"

#include "compiler_intrinsics.h"
#include "cpu.h"
#include "csiro_math.h"
#include "em_rmu.h"
#include "em_wdog.h"
#include "log.h"
#include "memory_operations.h"
#include "tdf.h"

/* Private Defines ------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/
/* Private Variables ----------------------------------------*/

static WatchdogReboot_t xWatchdogRebootValues ATTR_SECTION( ".noinit" );

/*-----------------------------------------------------------*/

void vWatchdogInit( xWatchdogModule_t *pxWdog )
{
	/* Watchdog is always run for EFR32, as it doesn't automatically reboot */
	const WDOG_Init_TypeDef xWdogInit = {
		.enable		  = true, // Start on Init Call
		.debugRun	 = false,
		.em2Run		  = true,
		.em3Run		  = true,
		.em4Block	 = false,			  // Maybe we should enable this, means we cant enter EM4
		.swoscBlock   = false,			  // Maybe we should enable this, means External crystal cant be disabled
		.lock		  = false,			  // Maybe even enable this so no-one tries to disable it
		.clkSel		  = wdogClkSelULFRCO, // Ultra Low Frequency timer to increase potential periods (1000Hz)
		.perSel		  = wdogPeriod_2k,	// Should give us 2 seconds at 1kHz
		.resetDisable = true			  // Don't reboot straight away on watchdog timeout
	};
	WDOGn_IntEnable( pxWdog->xHandle, WDOG_IEN_TOUT );
	vInterruptEnable( pxWdog->IRQn );
	WDOGn_Init( pxWdog->xHandle, &xWdogInit );
}

/*-----------------------------------------------------------*/

void vWatchdogPeriodic( xWatchdogModule_t *pxWdog )
{
	WDOGn_Feed( pxWdog->xHandle );
}

/*-----------------------------------------------------------*/

WatchdogReboot_t *xWatchdogRebootReason( void )
{
	xDateTime_t xDatetime;
	uint32_t	eRebootReason = RMU_UserResetStateGet();
	RMU_ResetCauseClear();
	RMU_UserResetStateSet( REBOOT_UNKNOWN );
	xWatchdogRebootValues.eRebootReason = eRebootReason;
	/* If we rebooted for a known reason, load the saved RTC time */
	if ( ( eRebootReason != REBOOT_UNKNOWN ) && ( xWatchdogRebootValues.ulWatchdogKey == WATCHDOG_KEY_VALUE ) ) {
		vRtcEpochToDateTime( e2000Epoch, xWatchdogRebootValues.xRebootTime.ulSecondsSince2000 + 1, &xDatetime );
		eRtcSetDatetime( &xDatetime );
	}

	/* Set the watchdog key to be invalid */
	xWatchdogRebootValues.ulWatchdogKey = 0;

	if ( eRebootReason == REBOOT_UNKNOWN ) {
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
	/* Set a valid key */
	xWatchdogRebootValues.ulWatchdogKey = WATCHDOG_KEY_VALUE;
	/* Store the reboot reason */
	RMU_UserResetStateSet( (uint32_t) eReason );
}

/*-----------------------------------------------------------*/
