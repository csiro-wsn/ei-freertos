/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: template_header.h
 * Creation_Date: 2/6/2018
 * Author: AddMe <add.me@data61.csiro.au>
 *
 * Template of a header file
 * 
 */

#ifndef __CORE_CSIRO_UTIL_WATCHDOG
#define __CORE_CSIRO_UTIL_WATCHDOG

/* Includes -------------------------------------------------*/

#include "log.h"
#include "rtc.h"

#include "watchdog_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define WATCHDOG_MODULE_GET( IRQ_NAME ) xModule_##IRQ_NAME

#define WATCHDOG_VARIABLE_BUILDER( IRQ_NAME, IRQN, HANDLE  ) \
	static xWatchdogModule_t xModule_##IRQ_NAME = { \
		.xHandle = HANDLE,                          \
		.vIRQ	 = IRQ_NAME,                        \
		.IRQn    = IRQN                             \
	};

#define WATCHDOG_MODULE_CREATE( IRQ_NAME, IRQN, HANDLE  ) \
	void IRQ_NAME( void );                      \
	WATCHDOG_VARIABLE_BUILDER( IRQ_NAME, IRQN, HANDLE )  \
	WATCHDOG_HANDLER_BUILD( IRQ_NAME )

#define WATCHDOG_KEY_VALUE 0x12345678

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef enum eWatchdogRebootReason_t {
	REBOOT_UNKNOWN = 0,
	REBOOT_WATCHDOG,
	REBOOT_ASSERTION,
	REBOOT_RPC
} eWatchdogRebootReason_t;

typedef struct xWatchdogModule_t
{
	xWatchdogHandle_t *xHandle;
	void ( *vIRQ )( void );
	int32_t  IRQn;
	uint32_t ulWatchdogPeriodRtcTicks;
	uint64_t ullSoftwareLastCount;
} xWatchdogModule_t;

typedef struct WatchdogReboot_t
{
	uint32_t				ulWatchdogKey;
	eWatchdogRebootReason_t eRebootReason;
	xTdfTime_t				xRebootTime;
	uint32_t				ulProgramCounter;
	uint32_t				ulLinkRegister;
	char					cTaskName[configMAX_TASK_NAME_LEN + 1];
} WatchdogReboot_t;

/* Function Declarations ------------------------------------*/

void			  vWatchdogInit( xWatchdogModule_t *pxWdog );
void			  vWatchdogPeriodic( xWatchdogModule_t *pxWdog );
void			  vWatchdogSetRebootReason( eWatchdogRebootReason_t eReason, const char *pcTask, uint32_t ulProgramCounter, uint32_t ulLinkRegister );
void			  vWatchdogPopulateTdf( WatchdogReboot_t *pxReboot, tdf_watchdog_info_t *pxWatchdogTdf );
void			  vWatchdogPopulateTdfSmall( WatchdogReboot_t *pxReboot, tdf_watchdog_info_small_t *pxWatchdogTdf );
void			  vWatchdogPrintRebootReason( SerialLog_t eLogger, LogLevel_t eLevel, WatchdogReboot_t *pxReboot );
void			  vWatchdogRunInterrupt( uint32_t *pulStack );
WatchdogReboot_t *xWatchdogRebootReason( void );

#endif /* __CORE_CSIRO_UTIL_WATCHDOG_COMMON */
