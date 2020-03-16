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
 * Filename: i2c.h
 * Creation_Date: 9/08/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 *
 * I2C Interface
 * 
 */

#ifndef __CSIRO_CORE_INTERFACE_I2C
#define __CSIRO_CORE_INTERFACE_I2C

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"
#include "semphr.h"

#include "gpio.h"

/* Forward Declarations -------------------------------------*/

typedef struct _xI2CModule_t   xI2CModule_t;
typedef struct _xI2CPlatform_t xI2CPlatform_t;

#include "i2c_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define I2C_MODULE_GET( NAME ) xModule_##NAME

#define I2C_MODULE_CREATE( NAME, PERIPHERAL )                                \
	static xI2CModule_t xModule_##NAME = {                                   \
		.xPlatform		  = I2C_MODULE_PLATFORM_DEFAULT( NAME, PERIPHERAL ), \
		.pxCurrentConfig  = NULL,                                            \
		.xBusMutexHandle  = NULL,                                            \
		.xBusMutexStorage = { { 0 } },                                       \
		.bBusClaimed	  = false                                            \
	}

/* Type Definitions -----------------------------------------*/

/* Defines a set of configuration settings for operating the I2C interface at any given time */
typedef struct xI2CConfig
{
	uint32_t ulMaximumBusFrequency; // Defines the maximum bus frequency of the chip in Hz.
	uint8_t  ucAddress;				// Takes a 7 bit address. Enter as: AAAA AAAX.
} xI2CConfig_t;

/* Holds all the variables for the I2C Module */
struct _xI2CModule_t
{
	xI2CPlatform_t	xPlatform;
	xI2CConfig_t *	pxCurrentConfig;
	SemaphoreHandle_t xBusMutexHandle;
	StaticSemaphore_t xBusMutexStorage;
	bool			  bBusClaimed;
};

/* Function Declarations ------------------------------------*/

/**
 * Initialise the I2C module.
 * 
 * The idea is that this function is called once at board startup and that's
 * it. Allocates space on startup for all I2C configuration and initialisation
 * variables. Keeps the I2C interface in low power mode.
 **/
eModuleError_t eI2CInit( xI2CModule_t *pxModule );

/**
 * Requests exclusive access over the I2C bus.
 *
 * This sets up the I2C module to be used exclusively by a single peripheral
 * and turns on the high frequency clock. It configures communication for the
 * specific periferal which requested access.
 **/
eModuleError_t eI2CBusStart( xI2CModule_t *pxModule, xI2CConfig_t *pxConfig, TickType_t xTimeout );

/**
 * Relinquishes exclusive access over the I2C bus.
 *
 * Relinquishes control over the I2C lines. Turns off the high frequency clock
 * and allows the board to enter deep sleep again.
 **/
eModuleError_t eI2CBusEnd( xI2CModule_t *pxModule );

/**
 * Send data with the I2C module.
 *
 * Transmits the data buffer over I2C.
 **/
eModuleError_t eI2CTransmit( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout );

/**
 * Receive data with the I2C module.
 *
 * Stores received I2C data in data buffer.
 **/
eModuleError_t eI2CReceive( xI2CModule_t *pxModule, void *pvBuffer, uint16_t usLength, TickType_t xTimeout );

/**
 * A send and a receive in the same I2C command.
 * 
 * Sends one data buffer, then receives into the next.
 **/
eModuleError_t eI2CTransfer( xI2CModule_t *pxModule, void *pvSendBuffer, uint16_t usSendLength, void *pvReceiveBuffer, uint16_t usReceiveLength, TickType_t xTimeout );

#endif /* __CSIRO_CORE_INTERFACE_I2C */
