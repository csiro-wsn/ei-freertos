/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 */

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "cpu.h"
#include "gpio.h"
#include "pwm.h"

#include "nrf_pwm.h"

/* Private Defines ------------------------------------------*/

// clang-format off
// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

void prvPwmBase( uint32_t ulFrequencyMilliHz, uint16_t *pusCounterTop, nrf_pwm_clk_t *pxClock );

/* Private Variables ----------------------------------------*/

/*-----------------------------------------------------------*/

eModuleError_t vPwmInit( xPwmModule_t *pxModule )
{
	const IRQn_Type xIRQn = nrfx_get_irq_number( pxModule->xPlatform.pxInstance );

	pxModule->xWait = xSemaphoreCreateBinaryStatic( &pxModule->xWaitStorage );

	/* Set priority of PWM interrupt */
	vInterruptSetPriority( xIRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY );
	vInterruptEnable( xIRQn );

	pxModule->bEnabled = false;
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t vPwmStart( xPwmModule_t *pxModule, uint32_t ulFrequencyMilliHz, uint8_t ucDutyCycle )
{
	NRF_PWM_Type *const pxInst = pxModule->xPlatform.pxInstance;
	nrf_pwm_clk_t		xBaseClock;
	uint16_t			usTop;

	vGpioSetup( pxModule->xPwmGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	prvPwmBase( ulFrequencyMilliHz, &usTop, &xBaseClock );

	pxInst->PRESCALER		= xBaseClock;
	pxInst->MODE			= NRF_PWM_MODE_UP;
	pxInst->DECODER			= 0;
	pxInst->COUNTERTOP		= usTop;
	pxInst->LOOP			= 0;
	pxInst->SEQ[0].REFRESH  = 0;
	pxInst->SEQ[0].ENDDELAY = 0;
	pxInst->SHORTS			= NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
	pxInst->PSEL.OUT[0]		= pxModule->xPwmGpio.ucPin;

	pxModule->xPlatform.usCompareValue = ( ucDutyCycle * usTop ) / 100;

	pxInst->SEQ[0].PTR = (uint32_t) &pxModule->xPlatform.usCompareValue;
	pxInst->SEQ[0].CNT = 1;

	pxInst->ENABLE			  = 0x01;
	pxInst->TASKS_SEQSTART[0] = 0x01;

	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

eModuleError_t vPwmStop( xPwmModule_t *pxModule )
{
	NRF_PWM_Type *const pxInst = pxModule->xPlatform.pxInstance;

	pxInst->TASKS_STOP = 0x01;
	pxInst->ENABLE	 = 0x00;
	vGpioSetup( pxModule->xPwmGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
	return ERROR_NONE;
}

/*-----------------------------------------------------------*/

void vPwmSequenceConfigure( xPwmModule_t *pxModule, xPwmSequence_t *pxSequence )
{
	NRF_PWM_Type *const pxInst = pxModule->xPlatform.pxInstance;
	nrf_pwm_clk_t		xBaseClock;

	vGpioSetup( pxModule->xPwmGpio, GPIO_PUSHPULL, GPIO_PUSHPULL_LOW );
	prvPwmBase( pxSequence->ulFrequencyMilliHz, &pxSequence->usTopValue, &xBaseClock );

	pxInst->PRESCALER		= xBaseClock;
	pxInst->MODE			= NRF_PWM_MODE_UP;
	pxInst->DECODER			= 0;
	pxInst->COUNTERTOP		= pxSequence->usTopValue;
	pxInst->LOOP			= 1;
	pxInst->SEQ[0].REFRESH  = 0;
	pxInst->SEQ[0].ENDDELAY = 0;
	pxInst->SEQ[1].REFRESH  = 0;
	pxInst->SEQ[1].ENDDELAY = 0;
	pxInst->SHORTS			= NRF_PWM_SHORT_LOOPSDONE_SEQSTART0_MASK;
	pxInst->PSEL.OUT[0]		= pxModule->xPwmGpio.ucPin;

	pxInst->INTENCLR = UINT32_MAX;
	pxInst->INTENSET = PWM_INTENSET_SEQEND0_Msk | PWM_INTENSET_SEQEND1_Msk;

	pxInst->SEQ[0].PTR = (uint32_t) pxSequence->pusBufferA;
	pxInst->SEQ[0].CNT = pxSequence->usBufferLen;
	pxInst->SEQ[1].PTR = (uint32_t) pxSequence->pusBufferB;
	pxInst->SEQ[1].CNT = pxSequence->usBufferLen;
}

/*-----------------------------------------------------------*/

void vPwmSequenceStart( xPwmModule_t *pxModule )
{
	NRF_PWM_Type *const pxInst = pxModule->xPlatform.pxInstance;

	pxInst->ENABLE			  = 0x01;
	pxInst->TASKS_SEQSTART[0] = 0x01;
}

/*-----------------------------------------------------------*/

uint16_t *pusPwmSequenceBufferRun( xPwmModule_t *pxModule )
{
	configASSERT( xSemaphoreTake( pxModule->xWait, portMAX_DELAY ) == pdPASS );
	return pxModule->xPlatform.pusFinishedBuffer;
}

/*-----------------------------------------------------------*/

void vPwmSequenceStop( xPwmModule_t *pxModule )
{
	NRF_PWM_Type *const pxInst = pxModule->xPlatform.pxInstance;

	pxInst->TASKS_STOP = 0x01;
	pxInst->ENABLE	 = 0x00;
	vGpioSetup( pxModule->xPwmGpio, GPIO_DISABLED, GPIO_DISABLED_NOPULL );
}

/*-----------------------------------------------------------*/

void vPwmInterrupt( xPwmModule_t *pxModule )
{
	NRF_PWM_Type *const pxInst					 = pxModule->xPlatform.pxInstance;
	BaseType_t			xHigherPriorityTaskWoken = pdFALSE;

	if ( pxInst->EVENTS_SEQEND[0] ) {
		pxInst->EVENTS_SEQEND[0]			  = 0x00;
		pxModule->xPlatform.pusFinishedBuffer = (uint16_t *) pxInst->SEQ[0].PTR;
	}
	if ( pxInst->EVENTS_SEQEND[1] ) {
		pxInst->EVENTS_SEQEND[1]			  = 0x00;
		pxModule->xPlatform.pusFinishedBuffer = (uint16_t *) pxInst->SEQ[1].PTR;
	}
	xSemaphoreGiveFromISR( pxModule->xWait, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
}

/*-----------------------------------------------------------*/

void prvPwmBase( uint32_t ulFrequencyMilliHz, uint16_t *pusCounterTop, nrf_pwm_clk_t *pxClock )
{
	/**
	 * Get the prescaler and clock rate that allows for the most duty cycle resolution
	 * Maximum value of COUNTER_TOP is (2 ^ 15) - 1
	 * Comparison values are generated by CLOCK_FREQUENCY / ( 2^15 )
	*/
	uint64_t ullTimerFreq;
	if ( ulFrequencyMilliHz >= ( 490 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_16MHz;
		ullTimerFreq = 16000000;
	}
	else if ( ulFrequencyMilliHz >= ( 245 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_8MHz;
		ullTimerFreq = 8000000;
	}
	else if ( ulFrequencyMilliHz >= ( 123 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_4MHz;
		ullTimerFreq = 4000000;
	}
	else if ( ulFrequencyMilliHz >= ( 62 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_2MHz;
		ullTimerFreq = 2000000;
	}
	else if ( ulFrequencyMilliHz >= ( 31 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_1MHz;
		ullTimerFreq = 1000000;
	}
	else if ( ulFrequencyMilliHz >= ( 16 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_500kHz;
		ullTimerFreq = 500000;
	}
	else if ( ulFrequencyMilliHz >= ( 8 * 1000 ) ) {
		*pxClock	 = NRF_PWM_CLK_250kHz;
		ullTimerFreq = 250000;
	}
	else {
		*pxClock	 = NRF_PWM_CLK_125kHz;
		ullTimerFreq = 125000;
	}
	*pusCounterTop = ( uint16_t )( 1000 * ullTimerFreq / ulFrequencyMilliHz );
}

/*-----------------------------------------------------------*/
