/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: pwm.h
 * Creation_Date: 24/01/2019
 * Author: Jonathan Chang <jonathan.chang@data61.csiro.au>
 *
 * 
 * Generic pwm implementation
 * 
 */

#ifndef __CSIRO_CORE_ARCH_PWM
#define __CSIRO_CORE_ARCH_PWM

/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

/* Forward Declarations -------------------------------------*/

typedef struct _xPwmModule_t   xPwmModule_t;
typedef struct _xPwmPlatform_t xPwmPlatform_t;

#include "pwm_arch.h"

/* Module Defines -------------------------------------------*/

#define PWM_MODULE_GET( NAME ) xModule_##NAME

#define PWM_MODULE_CREATE( NAME, HANDLE, IRQ )                    \
	static xPwmModule_t xModule_##NAME = {                        \
		.xPwmGpio  = UNUSED_GPIO,                                 \
		.bEnabled  = false,                                       \
		.xPlatform = PWM_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ), \
	};                                                            \
	PWM_MODULE_PLATFORM_SUFFIX( NAME, IRQ );

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

struct _xPwmModule_t
{
	xGpio_t			  xPwmGpio;
	bool			  bEnabled;
	xPwmPlatform_t	xPlatform;
	SemaphoreHandle_t xWait;
	StaticSemaphore_t xWaitStorage;
};

typedef struct xPwmSequence_t
{
	uint32_t  ulFrequencyMilliHz;
	uint16_t  usTopValue;
	uint16_t *pusBufferA;
	uint16_t *pusBufferB;
	uint16_t  usBufferLen;
} xPwmSequence_t;

/* Function Declarations ------------------------------------*/

/**
 * Setup underlying PWM hardware
 */
eModuleError_t vPwmInit( xPwmModule_t *pxModule );

/**
 * \param pxModule The PWM module
 * \param ulFrequency The frequency of the PWM
 * \param ulDutyCycle The duty cycle of the PWM
 */
eModuleError_t vPwmStart( xPwmModule_t *pxModule, uint32_t ulFrequencyMilliHz, uint8_t ucDutyCycle );

/**
 * \param pxModule The PWM module
 * Stop the PWM
 */
eModuleError_t vPwmStop( xPwmModule_t *pxModule );

void vPwmSequenceConfigure( xPwmModule_t *pxModule, xPwmSequence_t *pxSequence );

void vPwmSequenceStart( xPwmModule_t *pxModule );

uint16_t *pusPwmSequenceBufferRun( xPwmModule_t *pxModule );

void vPwmSequenceStop( xPwmModule_t *pxModule );

void vPwmInterrupt( xPwmModule_t *pxModule );

#endif /* __CSIRO_CORE_SOC_CRC */
