/*
 * Copyright (c) 2018, Commonwealth Scientific and Industrial Research
 * Organisation (CSIRO)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Filename: pwm_arch.h
 * Creation_Date: 5/2/2019
 * Author: Jesse Dennis <jesse.dennis@data61.csiro.au>
 *
 * nrf52 Specific PWM Functionality
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_PWM_PLATFORM
#define __CORE_CSIRO_INTERFACE_PWM_PLATFORM

/* Includes -------------------------------------------------*/

#include "gpio.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define PWM_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ) \
	{                                               \
		0                                           \
	}

#define PWM_MODULE_PLATFORM_SUFFIX( NAME )

/* Type Definitions -----------------------------------------*/

struct _xPwmPlatform_t
{
	uint16_t uTopValue;
};

/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_INTERFACE_PWM_PLATFORM */
