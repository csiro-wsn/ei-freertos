/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */
/* Includes -------------------------------------------------*/

#include "device_nvm.h"

#include "compiler_intrinsics.h"

#if __has_include( "scheduler.h" )
/* CSIRO Scheduler */
#include "scheduler.h"
#else
/* Dummy Type */
typedef uint32_t xSchedule_t;
#endif

/* Private Defines ------------------------------------------*/
// clang-format off

// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/* Private Variables ----------------------------------------*/

#ifdef NVM_VALID_KEY
const uint32_t ulApplicationNvmValidKey = NVM_VALID_KEY;
#else
const uint32_t ulApplicationNvmValidKey = 0xDEADCAFE;
#endif

/* Counter variables may be handled more efficiently by the underlying driver */
/* A counter variable is a uint32_t that increments by less than 16 each time */
const uint32_t ulKeyLengthWords[] = {
	[NVM_KEY]					 = NVM_COUNTER_VARIABLE,
	[NVM_RESET_COUNT]			 = NVM_COUNTER_VARIABLE,
	[NVM_GRENADE_COUNT]			 = NVM_COUNTER_VARIABLE,
	[NVM_WATCHDOG_COUNT]		 = NVM_COUNTER_VARIABLE,
	[NVM_DEVICE_END_TIME]        = 1,
	[NVM_DEVICE_ACTIVATED]		 = NVM_BOOLEAN_VARIABLE,
	[NVM_APPLICATION_STRUCT]	 = SIZEOF_WORDS( xNvmApplicationStruct_t ),
	[NVM_EXCEPTION_TIMESTAMP]	 = 1,
	[NVM_BLUETOOTH_TX_POWER_DBM] = 1,
	[NVM_SIGFOX_BLOCK]			 = 2,
	[NVM_CLIENT_KEY]			 = 4,
	[NVM_XTID]					 = 2,
	[NVM_SCHEDULE_0]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_1]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_2]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_3]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_4]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_5]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_6]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_7]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_8]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_9]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_10]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_11]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_12]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_13]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_14]			 = SIZEOF_WORDS( xSchedule_t ),
	[NVM_SCHEDULE_CRC]			 = 1
};

/*-----------------------------------------------------------*/
