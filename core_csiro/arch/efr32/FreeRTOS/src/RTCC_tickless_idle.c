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
#include "em_rtcc.h"

#include "board.h"
#include "cpu.h"

/* The RTCC channel used to generate the tick interrupt. */
#define lpRTCC_CHANNEL ( 1 )

/* 32768 clock divided by 1.  Don't use a prescale if errata RTCC_E201
applies. */
#define mainTIMER_FREQUENCY_HZ ( 32768UL )

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

/*-----------------------------------------------------------*/

/* Calculate how many clock increments make up a single tick period. */
static const uint32_t ulReloadValueForOneTick = ( mainTIMER_FREQUENCY_HZ / configTICK_RATE_HZ );

/* Will hold the maximum number of ticks that can be suppressed. */
static uint32_t xMaximumPossibleSuppressedTicks = 0;

/* Flag set from the tick interrupt to allow the sleep processing to know if
sleep mode was exited because of a timer interrupt or a different interrupt. */
static volatile uint32_t ulTickFlag = pdFALSE;

/* As the clock is only 32KHz, it is likely a value of 1 will be enough. */
static const uint32_t ulStoppedTimerCompensation = 0UL;

/* RTCC configuration structures. */
static const RTCC_Init_TypeDef xRTCInitStruct = {
	false,			  /* Don't start counting when init complete. */
	false,			  /* Disable counter during debug halt. */
	false,			  /* Don't care. */
	true,			  /* Enable counter wrap on ch. 1 CCV value. */
	rtccCntPresc_1,   /* NOTE:  Do not use a pre-scale if errata RTCC_E201 applies. */
	rtccCntTickPresc, /* Count using the clock input directly. */
#if defined( _RTCC_CTRL_BUMODETSEN_MASK )
	false, /* Disable storing RTCC counter value in RTCC_CCV2 upon backup mode entry. */
#endif
	false,			   /* Oscillator fail detection disabled. */
	rtccCntModeNormal, /* Use RTCC in normal mode (increment by 1 on each tick) and not in calendar mode. */
	false			   /* Don't care. */
};

static const RTCC_CCChConf_TypeDef xRTCCChannel1InitStruct = {
	rtccCapComChModeCompare,	 /* Use Compare mode. */
	rtccCompMatchOutActionPulse, /* Don't care. */
	rtccPRSCh0,					 /* PRS not used. */
	rtccInEdgeNone,				 /* Capture Input not used. */
	rtccCompBaseCnt,			 /* Compare with Base CNT register. */
	0,							 /* Compare mask. */
	rtccDayCompareModeMonth		 /* Don't care. */
};

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
	/* Configure the RTCC to generate the RTOS tick interrupt. */

	/* The maximum number of ticks that can be suppressed depends on the clock
	frequency. */
	xMaximumPossibleSuppressedTicks = UINT32_MAX / ulReloadValueForOneTick;

	/* Ensure LE modules are accessible. */
	CMU_ClockEnable( cmuClock_CORELE, true );

	/* Use LFXO. */
	CMU_ClockSelectSet( cmuClock_LFE, cmuSelect_LFXO );

	/* Enable clock to the RTC module. */
	CMU_ClockEnable( cmuClock_RTCC, true );

	/* Use channel 1 to generate the RTOS tick interrupt. */
	RTCC_ChannelCCVSet( lpRTCC_CHANNEL, ulReloadValueForOneTick );

	RTCC_Init( &xRTCInitStruct );
	RTCC_ChannelInit( lpRTCC_CHANNEL, &xRTCCChannel1InitStruct );
	RTCC_EM4WakeupEnable( true );

	/* Disable RTCC interrupt. */
	RTCC_IntDisable( _RTCC_IF_MASK );
	RTCC_IntClear( _RTCC_IF_MASK );
	RTCC->CNT = _RTCC_CNT_RESETVALUE;

	/* The tick interrupt must be set to the lowest priority possible. */
	vInterruptSetPriority( RTCC_IRQn, configLIBRARY_LOWEST_INTERRUPT_PRIORITY );
	vInterruptClearPending( RTCC_IRQn );
	vInterruptEnable( RTCC_IRQn );
	RTCC_IntEnable( RTCC_IEN_CC1 );
	RTCC_Enable( true );

#if ( lpUSE_TEST_TIMER == 1 )
	{
		void prvSetupTestTimer( void );

		/* A second timer is used to test the path where the MCU is brought out
		of a low power state by a timer other than the tick timer. */
		prvSetupTestTimer();
	}
#endif
}
/*-----------------------------------------------------------*/

void vPortSuppressTicksAndSleep( TickType_t xExpectedIdleTime )
{
	uint32_t		 ulReloadValue, ulCompleteTickPeriods, ulCountAfterSleep;
	eSleepModeStatus eSleepAction;
	TickType_t		 xModifiableIdleTime;

	/* THIS FUNCTION IS CALLED WITH THE SCHEDULER SUSPENDED. */

	/* Make sure the RTC reload value does not overflow the counter. */
	if ( xExpectedIdleTime > xMaximumPossibleSuppressedTicks ) {
		xExpectedIdleTime = xMaximumPossibleSuppressedTicks;
	}

	/* Calculate the reload value required to wait xExpectedIdleTime tick
	periods. */
	ulReloadValue = ulReloadValueForOneTick * xExpectedIdleTime;
	if ( ulReloadValue > ulStoppedTimerCompensation ) {
		/* Compensate for the fact that the RTC is going to be stopped
		momentarily. */
		ulReloadValue -= ulStoppedTimerCompensation;
	}

	/* Stop the RTC momentarily.  The time the RTC is stopped for is accounted
	for as best it can be, but using the tickless mode will inevitably result
	in some tiny drift of the time maintained by the kernel with respect to
	calendar time. */
	RTCC_Enable( false );

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
		RTCC_Enable( true );

		/* Re-enable interrupts - see comments above the RTCC_Enable() call
		above. */
		CORE_EXIT_ATOMIC();
	}
	else {
		RTCC_ChannelCCVSet( lpRTCC_CHANNEL, ulReloadValue );

		/* Restart the RTC. */
		RTCC_Enable( true );

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

		/* Stop RTC.  Again, the time the SysTick is stopped for is accounted
		for as best it can be, but using the tickless mode will	inevitably
		result in some tiny drift of the time maintained by the	kernel with
		respect to calendar time. */
		RTCC_Enable( false );
		ulCountAfterSleep = RTCC_CounterGet();

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

			/* The interrupt should have reset the CCV value. */
			configASSERT( RTCC_ChannelCCVGet( lpRTCC_CHANNEL ) == ulReloadValueForOneTick );
		}
		else {
			/* Something other than the tick interrupt ended the sleep.  How
			many complete tick periods passed while the processor was
			sleeping? */
			ulCompleteTickPeriods = ulCountAfterSleep / ulReloadValueForOneTick;

			/* The next interrupt is configured to occur at whatever fraction of
			the current tick period remains by setting the reload value back to
			that required for one tick, and truncating the count to remove the
			counts that are greater than the reload value. */
			RTCC_ChannelCCVSet( lpRTCC_CHANNEL, ulReloadValueForOneTick );
			ulCountAfterSleep %= ulReloadValueForOneTick;
			RTCC_CounterSet( ulCountAfterSleep );
		}

		/* Restart the RTC so it runs up to the alarm value.  The alarm value
		will get set to the value required to generate exactly one tick period
		the next time the RTC interrupt executes. */
		RTCC_Enable( true );

		/* Wind the tick forward by the number of tick periods that the CPU
		remained in a low power state. */
		vTaskStepTick( ulCompleteTickPeriods );
	}
}
/*-----------------------------------------------------------*/

void RTCC_IRQHandler( void )
{
	ulTickFlag = pdTRUE;

	if ( RTCC_ChannelCCVGet( lpRTCC_CHANNEL ) != ulReloadValueForOneTick ) {
		/* Set RTC interrupt to one RTOS tick period. */
		RTCC_Enable( false );
		RTCC_ChannelCCVSet( lpRTCC_CHANNEL, ulReloadValueForOneTick );
		RTCC_Enable( true );
	}

	RTCC_IntClear( _RTCC_IF_MASK );

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
