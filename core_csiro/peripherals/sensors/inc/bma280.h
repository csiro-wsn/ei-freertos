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
 * Filename: bma280.h
 * Creation_Date: 26/7/2018
 * Author: John Scolaro <john.scolaro@data61.csiro.au>
 * 
 * The header file defining the interface for the BMA280 accelerometer.
 * 
 * Driver only supports a single connected BMA280
 */

#ifndef __CORE_CSIRO_SENSORS_BMA280
#define __CORE_CSIRO_SENSORS_BMA280

/* Includes -------------------------------------------------*/

#include "FreeRTOS.h"

#include "accelerometer_interface.h"
#include "bma280_device.h"
#include "spi.h"
#include "tdf.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define BMA280_WHO_AM_I 0xFB

/* Type Definitions -----------------------------------------*/

/* Struct Declarations --------------------------------------*/

typedef struct xBma280Init_t
{
	xSpiModule_t *pxSpi;
	xGpio_t		  xChipSelect;
	xGpio_t		  xInterrupt1;
	xGpio_t		  xInterrupt2;
} xBma280Init_t;

/* Function Declarations ------------------------------------*/

/**@brief Initialise the BMA280 into low power mode
 *
 * @param[in] pxInit			Hardware Mappings
 * @param[in] xTimeout			Initialisation timeout
 *
 * @retval ::ERROR_NONE			Initialised successfully 	
 * @retval ::ERROR_TIMEOUT		Unable to claim provided SPI bus	
 */
eModuleError_t eBma280Init( xBma280Init_t *pxInit, TickType_t xTimeout );

/**@brief Retrieve the Chip ID
 *
 * @param[out] pucWhoAmI		Storage location for ID (1 byte)
 * @param[in]  xTimeout			Retrieval timeout
 *
 * @retval ::ERROR_NONE			ID retrieved 	
 * @retval ::ERROR_TIMEOUT		Unable to claim provided SPI bus	
 */
eModuleError_t eBma280WhoAmI( uint8_t *pucWhoAmI, TickType_t xTimeout );

/**@brief Configure the BMA280 according to pxConfig
 *
 * @param[in]  pxConfig			Desired configuration
 * @param[out] pxState			Applied configuration
 * @param[in]  xTimeout			Configuration timeout
 * 
 * @retval ::ERROR_NONE			Device configured	
 * @retval ::ERROR_TIMEOUT		Unable to claim provided SPI bus	
 */
eModuleError_t eBma280Configure( xAccelerometerConfiguration_t *pxConfig, xAccelerometerState_t *pxState, TickType_t xTimeout );

/**@brief Wait for an interrupt from the BMA280
 *
 * @param[out] pxInterruptType	Type of interrupt/event that occured
 * @param[in]  xTimeout			Wait timeout
 * 
 * @retval ::ERROR_NONE			Interrupt received	
 * @retval ::ERROR_TIMEOUT		No interrupt occured	
 */
eModuleError_t eBma280WaitForInterrupt( eAccelerometerInterrupt_t *peInterruptType, TickType_t xTimeout );

/**@brief Read accelerometer samples
 *
 * @param[out] pxData				Array of samples of length MAX(1, ucNumFIFO)
 * @param[out] pxFirstSample		Timestamp of the first sample
 * @param[out] pulGenerationTime	RTC ticks that passed between previous data interrupt and current interrupt
 * @param[in]  ucNumFIFO			Number of bytes to read from the FIFO, must equal ucFIFOLimit from the configuration struct
 * 									A value of 0 reads from the data registers instead of the FIFO
 * @param[in]  xTimeout				Wait timeout
 * 
 * @retval ::ERROR_NONE				Data retrieved
 * @retval ::ERROR_TIMEOUT			SPI Bus was busy	
 */
eModuleError_t eBma280ReadData( xAccelerometerSample_t *pxData, xTdfTime_t *pxFirstSample, uint32_t *pulGenerationTime, uint8_t ucNumFIFO, TickType_t xTimeout );

/**@brief Query the currently active hardware interrupts
 *
 * 	This function only needs to be called to determine end times of interrupts
 *  Start times are implied by the original interrupts
 * 
 * @param[out] pxInterruptType	Bitmask of active interrupts
 * @param[in]  xTimeout			Wait timeout
 * 
 * @retval ::ERROR_NONE			Interrupts queried	
 * @retval ::ERROR_TIMEOUT		SPI Bus was busy	
 */
eModuleError_t eBma280ActiveInterrupts( eAccelerometerInterrupt_t *peInterrupts, TickType_t xTimeout );

/*-----------------------------------------------------------*/

#endif /* __CORE_CSIRO_SENSORS_BMA280 */
