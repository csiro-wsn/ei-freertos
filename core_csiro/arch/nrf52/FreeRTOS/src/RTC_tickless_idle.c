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

/* Includes -------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"

#include "app_util_platform.h"
#include "nrfx_clock.h"
#include "nrfx_rtc.h"

#include "board.h"
#include "cpu.h"

#ifdef USE_RTC_TICKLESS_IDLE

/* Private Defines ------------------------------------------*/
#define RTCTIMER_PRESCALER ( ( 32768UL / configTICK_RATE_HZ ) - 1 )
#define UINT24_MAX 0x00FFFFFF

/* Function Declarations ------------------------------------*/

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
 * manages the LETIMER clock, as the tick is generated from the low power RTCTIMER
 * and not the SysTick as would normally be the case on a Cortex-M.
 */
void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime );

/*-----------------------------------------------------------*/

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of a timer interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

/* Handler to the RTC timer: This is used for both the tick interrupt and the 
return from sleep interrupt */
static const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE( 1 );

/*-----------------------------------------------------------*/
static void prvRtcHandler( nrfx_rtc_int_type_t xInterruptType )
{
	/* If the interrupt was called because of an output compare, then disable the 
	interrupt to avoid repeat calls */
	if ( xInterruptType == NRFX_RTC_INT_COMPARE1 ) {
		nrfx_rtc_cc_disable( &rtc, 1 );
		return;
	}

	/* Critical section which protects incrementing the tick*/
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

static bool prvSleep( void )
{
	bool bEnterDeepSleep;

	/* Critical section to allow sleep blocks in ISRs. */
	bEnterDeepSleep = bBoardCanDeepSleep();
	if ( bEnterDeepSleep ) {
		vBoardDeepSleep();
		__WFI();
	}
	else {
		__WFI();
	}

	return bEnterDeepSleep;
}

/*-----------------------------------------------------------*/

void prvStopTickInterruptTimer( void )
{
	nrfx_rtc_tick_disable( &rtc );
}

/*-----------------------------------------------------------*/

void prvStartTickInterruptTimer( void )
{
	nrfx_rtc_tick_enable( &rtc, true );
}

/*-----------------------------------------------------------*/

uint32_t ulGetExternalTime( void )
{
	return nrfx_rtc_counter_get( &rtc );
}

/*-----------------------------------------------------------*/

void vPortSetupTimerInterrupt( void )
{
	/* Configure the RTC TIMER to generate the RTOS tick interrupt. */

	/* Start the low frequency clock */
	nrfx_clock_init( NULL );
	nrfx_clock_lfclk_start();

	/* Initialize RTC instance */
	nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
	config.prescaler		 = RTCTIMER_PRESCALER;
	nrfx_rtc_init( &rtc, &config, prvRtcHandler );

	/* Enable tick event & interrupt */
	nrfx_rtc_tick_enable( &rtc, true );

	/* Power on RTC instance */
	nrfx_rtc_enable( &rtc );
}

/*-----------------------------------------------------------*/

void vSetWakeTimeInterrupt( TickType_t xExpectedIdleTime )
{

	/* Calculate the compare value for the wakeup interrupt */
	uint32_t ulWakeUpCount = ulGetExternalTime() + xExpectedIdleTime;

	/* The counter is 24 bits, truncate the value to fit */
	ulWakeUpCount = ulWakeUpCount & UINT24_MAX;

	/* Set compare channel to trigger interrupt after COMPARE_COUNTERTIME seconds */
	nrfx_rtc_cc_set( &rtc, 1, ulWakeUpCount, true );
}

/*-----------------------------------------------------------*/

void portSUPPRESS_TICKS_AND_SLEEP( TickType_t xExpectedIdleTime )
{
	unsigned long	ulLowPowerTimeBeforeSleep, ulLowPowerTimeAfterSleep;
	eSleepModeStatus eSleepStatus;

	/* Ensure the expected idle time does not overflow the counter */
	if ( xExpectedIdleTime > UINT24_MAX ) {
		xExpectedIdleTime = UINT24_MAX;
	}

	/* Read the current time from a time source that will remain operational
    while the microcontroller is in a low power state. */
	ulLowPowerTimeBeforeSleep = ulGetExternalTime();

	/* Confirm the board can enter deep sleep before stopping the interrupt */
	if ( !bBoardCanDeepSleep() ) {
		return;
	}

	/* Stop the timer that is generating the tick interrupt. */
	prvStopTickInterruptTimer();

	/* Enter a critical section that will not effect interrupts bringing the MCU
    out of sleep mode. */
	CRITICAL_REGION_ENTER();

	/* Ensure it is still ok to enter the sleep mode. */
	eSleepStatus = eTaskConfirmSleepModeStatus();

	if ( eSleepStatus == eAbortSleep ) {
		/* A task has been moved out of the Blocked state since this macro was
        executed, or a context siwth is being held pending.  Do not enter a
        sleep state.  Restart the tick and exit the critical section. */
		prvStartTickInterruptTimer();
		CRITICAL_REGION_EXIT();
	}
	else {
		if ( eSleepStatus == eNoTasksWaitingTimeout ) {
			/* It is not necessary to configure an interrupt to bring the
            microcontroller out of its low power state at a fixed time in the
            future. */
			prvSleep();
		}
		else {
			/* Configure an interrupt to bring the microcontroller out of its low
            power state at the time the kernel next needs to execute.  The
            interrupt must be generated from a source that remains operational
            when the microcontroller is in a low power state. */
			vSetWakeTimeInterrupt( xExpectedIdleTime );

			/* Enter the low power state. */
			prvSleep();

			/* Determine how long the microcontroller was actually in a low power
            state for, which will be less than xExpectedIdleTime if the
            microcontroller was brought out of low power mode by an interrupt
            other than that configured by the vSetWakeTimeInterrupt() call.
            Note that the scheduler is suspended before
            portSUPPRESS_TICKS_AND_SLEEP() is called, and resumed when
            portSUPPRESS_TICKS_AND_SLEEP() returns.  Therefore no other tasks will
            execute until this function completes. */
			ulLowPowerTimeAfterSleep = ulGetExternalTime();

			/* Correct the kernels tick count to account for the time the
            microcontroller spent in its low power state. */
			vTaskStepTick( ulLowPowerTimeAfterSleep - ulLowPowerTimeBeforeSleep );
		}

		/* Exit the critical section - it might be possible to do this immediately
        after the prvSleep() calls. */
		CRITICAL_REGION_EXIT();

		/* Restart the timer that is generating the tick interrupt. */
		prvStartTickInterruptTimer();
	}
}
#endif
