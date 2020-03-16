/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "watchdog.h"

#include "memory_operations.h"
#include "tdf.h"

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

void vWatchdogRunInterrupt( uint32_t *pulStack )
{
	/** 
	 * Get the program counter and link register at the point the interrupt was called
     * How the register values are stored is detailed in the below documentation
     * Link register lives at ((uint32_t*)StackPointer + 5)
     * Program counter lives at ((uint32_t*)StackPointer + 6)
     * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0552a/Babefdjc.html
	 **/
	uint32_t ulLR = pulStack[5];
	uint32_t ulPC = pulStack[6];
	vWatchdogSetRebootReason( REBOOT_WATCHDOG, pcTaskGetName( NULL ), ulPC, ulLR );

#ifndef RELEASE_BUILD
	/* For debug builds on platforms that call this function, hang forever */
	configASSERT( 0 );
#endif /* RELEASE_BUILD */

	/* Reboot */
	vSystemReboot();
}

/*-----------------------------------------------------------*/

void vWatchdogPrintRebootReason( SerialLog_t eLogger, LogLevel_t eLevel, WatchdogReboot_t *pxReboot )
{
	const char *pcFormat;
	switch ( pxReboot->eRebootReason ) {
		case REBOOT_WATCHDOG:
			pcFormat = "Watchdog Reboot: %s\r\n";
			break;
		case REBOOT_ASSERTION:
			pcFormat = "Assertion Failed: %s PC: 0x%X LR: 0x%X\r\n";
			break;
		case REBOOT_RPC:
			pcFormat = "Rebooted From RPC: %s\r\n";
			break;
		default:
			pcFormat = "Unknown Reboot\r\n";
			break;
	}
	eLog( eLogger, eLevel, pcFormat, pxReboot->cTaskName, pxReboot->ulProgramCounter, pxReboot->ulLinkRegister );
}

/*-----------------------------------------------------------*/

void vWatchdogPopulateTdf( WatchdogReboot_t *pxReboot, tdf_watchdog_info_t *pxWatchdogTdf )
{
	uint8_t i = 0;
	pvMemset( pxWatchdogTdf->procName, ' ', sizeof( pxWatchdogTdf->procName ) );
	while ( ( pxReboot->cTaskName[i] != '\0' ) && ( i < sizeof( pxWatchdogTdf->procName ) ) ) {
		pxWatchdogTdf->procName[i] = pxReboot->cTaskName[i];
		i++;
	}
	pxWatchdogTdf->programCounter = pxReboot->ulProgramCounter;
	pxWatchdogTdf->linkRegister   = pxReboot->ulLinkRegister;
}

/*-----------------------------------------------------------*/

void vWatchdogPopulateTdfSmall( WatchdogReboot_t *pxReboot, tdf_watchdog_info_small_t *pxWatchdogTdf )
{
	uint8_t i = 0;
	pvMemset( pxWatchdogTdf->procName, ' ', sizeof( pxWatchdogTdf->procName ) );
	while ( ( pxReboot->cTaskName[i] != '\0' ) && ( i < sizeof( pxWatchdogTdf->procName ) ) ) {
		pxWatchdogTdf->procName[i] = pxReboot->cTaskName[i];
		i++;
	}
	pxWatchdogTdf->programCounter = pxReboot->ulProgramCounter;
	pxWatchdogTdf->linkRegister   = pxReboot->ulLinkRegister;
}

/*-----------------------------------------------------------*/
