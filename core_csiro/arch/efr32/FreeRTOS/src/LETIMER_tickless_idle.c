/*
 * FreeRTOS Kernel V10.0.1
 * Copyright (C) 2017 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* SiLabs library includes. */
#include "em_cmu.h"
#include "em_core.h"
#include "em_emu.h"
#include "em_letimer.h"
#include "em_rtcc.h"

#include "board.h"
#include "cpu.h"
#include "leds.h"

/* The LETIMER channel used to generate the tick interrupt. */

/* 32768 clock divided by 1.  
 * Unscaled 16 bit timer can count 2 seconds before overflow
 */
#define LETIMER_PRESCALER ( 1 )
#define LETIMER_FREQUENCY ( 32768UL / LETIMER_PRESCALER )

// #define DEEP_SLEEP_LED

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
 * manages the LETIMER clock, as the tick is generated from the low power LETIMER
 * and not the SysTick as would normally be the case on a Cortex-M.
 */
void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );

/*-----------------------------------------------------------*/

/* Calculate how many clock increments make up a single tick period. */
static const uint32_t ulReloadValueForOneTick = ( LETIMER_FREQUENCY / configTICK_RATE_HZ ) - 1;

/* Will hold the maximum number of ticks that can be suppressed. */
static uint32_t xMaximumPossibleSuppressedTicks = 0;

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of a timer interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

/* As the clock is only 32KHz, it is likely a value of 1 will be enough. */
static const uint32_t ulStoppedTimerCompensation = 0UL;

/* LETIMER configuration structures */
static const LETIMER_Init_TypeDef xLETIMERInitStruct = {
	.enable   = false,
	.debugRun = false,
	.comp0Top = true, /**< Load COMP0 register into CNT when counter underflows. */
	.bufTop   = false,
	.out0Pol  = 0,
	.out1Pol  = 0,
	.ufoa0	= letimerUFOANone,
	.ufoa1	= letimerUFOANone,
	.repMode  = letimerRepeatFree,
	.topValue = ( LETIMER_FREQUENCY / configTICK_RATE_HZ ) - 1
};

/*-----------------------------------------------------------*/

void prvWaitForTimerStart( LETIMER_TypeDef *xTimer )
{
	while ( !( xTimer->STATUS & LETIMER_STATUS_RUNNING ) ) {
		continue;
	}
}

/*-----------------------------------------------------------*/

void prvWaitForTimerStop( LETIMER_TypeDef *xTimer )
{
	while ( xTimer->STATUS & LETIMER_STATUS_RUNNING ) {
		continue;
	}
}

/*-----------------------------------------------------------*/

void prvSetTimerCnt( LETIMER_TypeDef *xTimer, uint32_t ulCount )
{
	configASSERT( ulCount <= UINT16_MAX );
	xTimer->CNT = (uint16_t) ulCount;
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
	/* Configure the LETIMER to generate the RTOS tick interrupt. */

	/* The maximum number of ticks that can be suppressed depends on the clock
	frequency. */
	xMaximumPossibleSuppressedTicks = UINT16_MAX / ulReloadValueForOneTick;

	/* Use LFXO. */
	CMU_ClockSelectSet( cmuClock_LFA, cmuSelect_LFXO );
	/* Ensure LE modules are accessible. */
	CMU_ClockEnable( cmuClock_CORELE, true );
	/* Enable clock to the LETIMER module. */
	CMU_ClockDivSet( cmuClock_LETIMER0, LETIMER_PRESCALER );
	CMU_ClockEnable( cmuClock_LETIMER0, true );

	/* Initialise the timer */
	LETIMER_Init( LETIMER0, &xLETIMERInitStruct );

	/* Disable LETIMER interrupt. */
	LETIMER_IntDisable( LETIMER0, LETIMER_IF_UF );
	LETIMER_IntClear( LETIMER0, LETIMER_IF_UF );

	/* The tick interrupt must be set to the lowest priority possible. */
	vInterruptSetPriority( LETIMER0_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
	vInterruptClearPending( LETIMER0_IRQn );
	vInterruptEnable( LETIMER0_IRQn );

	LETIMER_IntEnable( LETIMER0, LETIMER_IF_UF );
	LETIMER_Enable( LETIMER0, true );
	prvWaitForTimerStart( LETIMER0 );
}
/*-----------------------------------------------------------*/

void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
	uint32_t		 ulReloadValue, ulCompletedTimerDecrements, ulCompleteTickPeriods;
	uint32_t		 ulCountBeforeSleep, ulCountAfterSleep;
	eSleepModeStatus eSleepAction;
	TickType_t		 xModifiableIdleTime;

	/* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */

	/* Make sure the LETIMER reload value does not overflow the counter. */
	if ( xExpectedIdleTime > xMaximumPossibleSuppressedTicks ) {
		xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
	}

	/* Stop the LETIMER momentarily.  The time the LETIMER is stopped for is accounted
	for as best it can be, but using the tickless mode will inevitably result
	in some tiny drift of the time maintained by the kernel with respect to
	calendar time. */
	LETIMER0->CMD = LETIMER_CMD_STOP;
	while ( LETIMER0->SYNCBUSY & LETIMER_SYNCBUSY_CMD )
		;

	ulCountBeforeSleep = LETIMER0->CNT;
	/* Calculate the reload value required to wait xExpectedIdleTime tick
	periods.  -1 is used as the current time slice will already be part way
	through, the part value coming from the current timer count value. */
	ulReloadValue = ulCountBeforeSleep + ( ulReloadValueForOneTick * ( xExpectedIdleTime - 1UL ) );

	if ( ulReloadValue > ulStoppedTimerCompensation ) {
		/* Compensate for the fact that the LETIMER is going to be stopped momentarily. */
		ulReloadValue -= ulStoppedTimerCompensation;
	}

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

		/* Restart the timer from whatever remains in the counter register,
		but 0 is not a valid value. */
		ulReloadValue = ulCountBeforeSleep - ulStoppedTimerCompensation;

		if ( ulReloadValue <= 1 ) {
			ulReloadValue		  = ulReloadValueForOneTick;
			ulCompleteTickPeriods = 1UL;
		}
		else {
			ulCompleteTickPeriods = 0UL;
		}

		/* Restart tick and continue counting to complete the current time slice. */
		LETIMER0->CNT = ulReloadValue;
		LETIMER0->CMD = LETIMER_CMD_START;

		/* Re-enable interrupts - see comments above the LETIMER_Enable() call above. */
		CORE_EXIT_ATOMIC();
	}
	else {
		/* Set current counter value and restart */
		LETIMER0->CNT = ulReloadValue;
		LETIMER0->CMD = LETIMER_CMD_START;

		/* Allow the application to define some pre-sleep processing. */
		xModifiableIdleTime = xExpectedIdleTime;
		configPRE_SLEEP_PROCESSING( xModifiableIdleTime );

		/* xExpectedIdleTime being set to 0 by configPRE_SLEEP_PROCESSING()
		means the application defined code has already executed the WAIT
		instruction. */
#ifdef DEEP_SLEEP_LED
		vLedsOff( LEDS_RED );
#endif
		if ( xModifiableIdleTime > 0 ) {
			__asm volatile( "dsb" );
			vSleep();
			__asm volatile( "isb" );
		}
#ifdef DEEP_SLEEP_LED
		vLedsOn( LEDS_RED );
#endif
		/* Allow the application to define some post sleep processing. */
		configPOST_SLEEP_PROCESSING( xModifiableIdleTime );

		/* Stop LETIMER.  Again, the time the SysTick is stopped for is accounted
		for as best it can be, but using the tickless mode will	inevitably
		result in some tiny drift of the time maintained by the	kernel with
		respect to calendar time. */
		LETIMER0->CMD = LETIMER_CMD_STOP;
		while ( LETIMER0->SYNCBUSY & LETIMER_SYNCBUSY_CMD )
			;
		ulCountAfterSleep = LETIMER0->CNT;
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

			/* Sanity check that the timer's reload value has indeed been reset. */
			configASSERT( LETIMER_CompareGet( LETIMER0, 0 ) == ulReloadValueForOneTick );
		}
		else {
			/** 
			 * Something other than the tick interrupt ended the sleep.  How
			 * many complete tick periods passed while the processor was sleeping? 
			 **/
			if ( ulReloadValue >= ulCountAfterSleep ) {
				ulCompletedTimerDecrements = ulReloadValue - ulCountAfterSleep;
			}
			else {
				ulCompletedTimerDecrements = 1;
			}

			/* Undo the adjustment that was made to the reload value to account
			for the fact that a time slice was part way through when this
			function was called before working out how many complete tick
			periods this represents.  (could have used [ulExpectedIdleTime *
			ulReloadValueForOneHighResolutionTick] instead of ulReloadValue on
			the previous line, but this way avoids the multiplication). */

			/** 
			 * If this condition is not true, ulCompletedTimerDecrements will explode
			 * Attempt to protect against this condition, with the side effect that 
			 * knowledge is lost about the partial tick which executed before the sleep
			 * request. This will result in FreeRTOS time slipping backwards against
			 * real time. But no explosions.
			 **/
			if ( ulReloadValueForOneTick >= ulCountBeforeSleep ) {
				ulCompletedTimerDecrements += ( ulReloadValueForOneTick - ulCountBeforeSleep );
			}
			ulCompleteTickPeriods = ulCompletedTimerDecrements / ulReloadValueForOneTick;

			/* The reload value is set to whatever fraction of a single tick period remains. */
			ulReloadValue = ( ( ulCompleteTickPeriods + 1UL ) * ulReloadValueForOneTick ) - ulCompletedTimerDecrements;

			/* Cannot use a reload value of 0 - it will not start the timer. */
			if ( ulReloadValue <= 1 ) {
				/* There is no fraction remaining. */
				ulReloadValue = ulReloadValueForOneTick;
				ulCompleteTickPeriods++;
			}

			/* Restart the LETIMER so it runs up to the alarm value.  The alarm value
			will get set to the value required to generate exactly one tick period
			the next time the LETIMER interrupt executes. */
			LETIMER0->CNT = ulReloadValue;
		}
		LETIMER0->CMD = LETIMER_CMD_START;
	}
	/* Wind the tick forward by the number of tick periods that the CPU remained in a low power state. */
	vTaskStepTick( ulCompleteTickPeriods );
}
/*-----------------------------------------------------------*/

void LETIMER0_IRQHandler( void )
{
	ulTickFlag = pdTRUE;

	LETIMER_IntClear( LETIMER0, LETIMER_IF_UF );

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
