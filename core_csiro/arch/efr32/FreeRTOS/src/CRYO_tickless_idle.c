/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "task.h"

#include "cpu.h"

#include "em_cmu.h"
#include "em_core.h"
#include "em_cryotimer.h"
#include "em_emu.h"

#include "board.h"

/* Private Defines ------------------------------------------*/

#define mainTIMER_FREQUENCY_HZ ( 32768UL )

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

/**
 * Convert sleep time to a value which is valid for Cryotimer.
 * Specifically, Cryotimer can only wake up on a power of two
 * ticks
 * \param xSleepTicks The number of ticks requested to sleep
 * \return Actual sleep ticks we can represent, rounded down
 */
uint32_t ulCryotimerPeriodValidGet( TickType_t xSleepTicks );

/**
 * Convert sleep time from ticks to cryotimer period register
 * \param xSleepTicks The number of ticks to sleep for
 * \return Configuration value to pass to Cryotimer_PeriodSet()
 */
uint32_t ulCryotimerPeriodRegisterGet( TickType_t xSleepTicks );

/**
 * Put the processor into EM2 or EM3 dependant on peripheral usage
 */
bool vSleep( void );

/*
 * The low power demo does not use the SysTick, so override the
 * vPortSetupTickInterrupt() function with an implementation that configures
 * a low power clock source.  NOTE:  This function name must not be changed as
 * it is called from the RTOS portable layer.
 */
void vPortSetupTimerInterrupt( void );

/*
 * Override the default definition of vPortSuppressTicksAndSleep() that is
 * weakly defined in the FreeRTOS Cortex-M port layer with a version that
 * manages the RTC clock, as the tick is generated from the low power RTC
 * and not the SysTick as would normally be the case on a Cortex-M.
 */
void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );

/* Private Variables ----------------------------------------*/

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of a timer interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

/* Calculate how many clock increments make up a single tick period. */
static const uint32_t ulReloadValueForOneTick = ( mainTIMER_FREQUENCY_HZ / configTICK_RATE_HZ );

/* Will hold the maximum number of ticks that can be suppressed. */
static uint32_t xMaximumPossibleSuppressedTicks = 0;

/* Cryotimer period register typical 1 second value */
static uint32_t ulNormalPeriod = 0;

/*-----------------------------------------------------------*/

uint32_t ulCryotimerPeriodValidGet( TickType_t xSleepTicks )
{
	return 0x01 << ( 31 - __CLZ( xSleepTicks ) );
}

/*-----------------------------------------------------------*/

uint32_t ulCryotimerPeriodRegisterGet( TickType_t xSleepTicks )
{
	return 31 - __CLZ( xSleepTicks );
}

/*-----------------------------------------------------------*/

bool vSleep( void )
{
	CORE_DECLARE_IRQ_STATE;
	bool bEnterDeepSleep;

	/* Critical section to allow sleep blocks in ISRs. */
	CORE_ENTER_CRITICAL();
	bEnterDeepSleep = bBoardCanDeepSleep();
	if ( bEnterDeepSleep ) {
		vBoardDeepSleep();
		EMU_EnterEM2( true );
	}
	else {
		EMU_EnterEM1();
	}
	CORE_EXIT_CRITICAL();

	return bEnterDeepSleep;
}

/*-----------------------------------------------------------*/

void vPortSetupTimerInterrupt( void )
{
	/* The maximum number of ticks that can be suppressed depends on the clock frequency. */
	xMaximumPossibleSuppressedTicks = 0x100000000 / ulReloadValueForOneTick;

	ulNormalPeriod = ulCryotimerPeriodRegisterGet( ulReloadValueForOneTick );

	/* Ensure LE modules are accessible. */
	CMU_ClockEnable( cmuClock_CORELE, true );

	/* Use LFXO. */
	CMU_ClockSelectSet( cmuClock_LFE, cmuSelect_LFXO );

	/* Enable clock to the Cryotimer module. */
	CMU_ClockEnable( cmuClock_CRYOTIMER, true );

	/* Configure the CRYOTIMER to use the ULFRCO which is running at 1 KHz
	* and trigger an interrupt every 128/1000 = 0,128s using the period
	* interrupt. */
	CRYOTIMER_Init_TypeDef init = CRYOTIMER_INIT_DEFAULT;
	init.osc					= cryotimerOscLFXO;
	init.presc					= cryotimerPresc_1;
	init.period					= ulNormalPeriod;
	CRYOTIMER_Init( &init );
	/* Now we enable the period interrupt in the CRYOTIMER and we enable
	* the CRYOTIMER IRQ in the NVIC. */
	CRYOTIMER_IntEnable( CRYOTIMER_IEN_PERIOD );
	vInterruptSetPriority( CRYOTIMER_IRQn, 1 );
	vInterruptClearPending( CRYOTIMER_IRQn );
	vInterruptEnable( CRYOTIMER_IRQn );
}

/*-----------------------------------------------------------*/

void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
	uint32_t		 ulReloadValue, ulCompleteTickPeriods, ulCountAfterSleep;
	uint32_t		 ulSleepPeriod, ulSubTickRemaining;
	eSleepModeStatus eSleepAction;
	TickType_t		 xModifiableIdleTime;
	/* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */

	/* Make sure the RTC reload value is not larger than maximum representable */
	if ( xExpectedIdleTime > xMaximumPossibleSuppressedTicks ) {
		xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
	}

	/* Calculate the reload value required to wait xExpectedIdleTime tick periods. */
	ulReloadValue = ulReloadValueForOneTick * xExpectedIdleTime;
	/* Update the reload value with what we can actually represent */
	ulReloadValue = ulCryotimerPeriodValidGet( ulReloadValue );
	/* Update the expected idle time based on the new reload value */
	xExpectedIdleTime = ulReloadValue / ulReloadValueForOneTick;
	/* Get the configuration register setting */
	ulSleepPeriod = ulCryotimerPeriodRegisterGet( ulReloadValue );

	/* Timer compensation could be added here, see RTCC_tickless_idle */
	/* Currently not used as period is not an actual count, but a 2**x number of ticks */

	CRYOTIMER_Enable( false );

	/* Enter a critical section but don't use the taskENTER_CRITICAL() method as
	that will mask interrupts that should exit sleep mode. */
	CORE_DECLARE_IRQ_STATE;
	CORE_ENTER_ATOMIC();
	__asm volatile( "dsb" );
	__asm volatile( "isb" );

	/* The tick flag is set to false before sleeping.  If it is true when sleep
	mode is exited then sleep mode was probably exited because the tick was
	suppressed for the entire xExpectedIdleTime period. */
	ulTickFlag = pdFALSE;

	/* If a context switch is pending then abandon the low power entry as the
	context switch might have been pended by an external interrupt that	requires
	processing. */
	eSleepAction = eTaskConfirmSleepModeStatus();
	if ( eSleepAction == eAbortSleep ) {
		/* Restart tick and continue counting to complete the current time
		slice. */
		CRYOTIMER_Enable( true );

		/* Re-enable interrupts - see comments above the RTCC_Enable() call
		above. */
		CORE_EXIT_ATOMIC();
	}
	else {
		CRYOTIMER_PeriodSet( ulSleepPeriod );

		/* Restart the RTC. */
		CRYOTIMER_Enable( true );

		/* Allow the application to define some pre-sleep processing. */
		xModifiableIdleTime = xExpectedIdleTime;
		configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

		/* xExpectedIdleTime being set to 0 by configPRE_SLEEP_PROCESSING()
		means the application defined code has already executed the WAIT
		instruction. */
		if ( xModifiableIdleTime > 0 ) {
			__asm volatile( "dsb" );
			vSleep();
			__asm volatile( "isb" );
		}

		/* Allow the application to define some post sleep processing. */
		configPOST_SLEEP_PROCESSING( xModifiableIdleTime );

		/* Stop Cryotimer.  Again, the time the SysTick is stopped for is accounted
		for as best it can be, but using the tickless mode will	inevitably
		result in some tiny drift of the time maintained by the	kernel with
		respect to calendar time. */
		CRYOTIMER_Enable( false );
		ulCountAfterSleep = CRYOTIMER_CounterGet();

		/* Re-enable interrupts - see comments above the INT_Enable() call
		above. */
		CORE_EXIT_ATOMIC();
		__asm volatile( "dsb" );
		__asm volatile( "isb" );

		if ( ulTickFlag != pdFALSE ) {
			/* The tick interrupt has already executed, although because this
			function is called with the scheduler suspended the actual tick
			processing will not occur until after this function has exited.
			The tick interrupt handler will already have pended the tick
			processing in the kernel.  As the pending tick will be processed as
			soon as this function exits, the tick value	maintained by the tick
			is stepped forward by one less than the	time spent sleeping.  The
			actual stepping of the tick appears later in this function. */
			ulCompleteTickPeriods = xExpectedIdleTime - 1UL;

			/* The interrupt should have reset the period. */
			configASSERT( CRYOTIMER_PeriodGet() == ulNormalPeriod );
		}
		else {
			/* Something other than the tick interrupt ended the sleep.  How
			many complete tick periods passed while the processor was
			sleeping? */
			ulCompleteTickPeriods = ulCountAfterSleep / ulReloadValueForOneTick;

			/* The next interrupt is configured to occur at whatever fraction of
			the current tick period remains. As we cannot directly set the timer 
			count, set the period itself to be lower for the next tick. */

			/* TODO: Quantify the drift that could occur due to the fact that
			 we can only set the period to a multiple of 2 for this subtick period*/
			ulSubTickRemaining = ulReloadValueForOneTick - ( ulCountAfterSleep % ulReloadValueForOneTick );

			CRYOTIMER_PeriodSet( ulCryotimerPeriodRegisterGet( ulSubTickRemaining ) );
		}

		/* Restart the RTC so it runs up to the alarm value.  The alarm value
		will get set to the value required to generate exactly one tick period
		the next time the RTC interrupt executes. */
		CRYOTIMER_Enable( true );

		/* Wind the tick forward by the number of tick periods that the CPU
		remained in a low power state. */
		vTaskStepTick( ulCompleteTickPeriods );
	}
}

/*-----------------------------------------------------------*/

void CRYOTIMER_IRQHandler( void )
{
	ulTickFlag = pdTRUE;

	if ( CRYOTIMER_PeriodGet() != ulNormalPeriod ) {
		CRYOTIMER_Enable( false );
		CRYOTIMER_PeriodSet( ulNormalPeriod );
		CRYOTIMER_Enable( true );
	}

	CRYOTIMER_IntClear( _CRYOTIMER_IF_MASK );

	/* Critical section which protect incrementing the tick*/
	portDISABLE_INTERRUPTS();
	{
		if ( xTaskIncrementTick() != pdFALSE ) {
			/* Pend a context switch. */
			vPendContextSwitch();
		}
	}
	portENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/
