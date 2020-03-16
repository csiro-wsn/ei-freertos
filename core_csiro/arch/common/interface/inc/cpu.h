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
 * Filename: cpu.h
 * Creation_Date: 12/6/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Wrapper around CPU core functionality that is access directly from
 * the driver or application layer.
 * Primary purpose is to allow interception by unit testing framework
 * 
 */

#ifndef __CORE_CSIRO_INTERFACE_CPU
#define __CORE_CSIRO_INTERFACE_CPU

/* Includes -------------------------------------------------*/

#include <stdint.h>

#include "cpu_arch.h"

/* Module Defines -------------------------------------------*/

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

void vInterruptSetPriority( int32_t IRQn, uint32_t ulPriority );
void vInterruptClearPending( int32_t IRQn );
void vInterruptEnable( int32_t IRQn );
void vInterruptDisable( int32_t IRQn );

void vPendContextSwitch( void );

void vSystemReboot( void );

static inline uint32_t ulCpuClockFreq( void );

#endif /* __CORE_CSIRO_INTERFACE_CPU */
