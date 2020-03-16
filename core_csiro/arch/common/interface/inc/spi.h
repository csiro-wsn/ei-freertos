/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: spi.h
 * Creation_Date: 17/6/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * FreeRTOS abstraction for DMA based SPI
 *
 */

#ifndef __LIBS_CSIRO_INTERFACE_SPI
#define __LIBS_CSIRO_INTERFACE_SPI

/* Includes -------------------------------------------------*/

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "gpio.h"

/* Forward Declarations -------------------------------------*/

typedef struct _xSpiModule_t   xSpiModule_t;
typedef struct _xSpiPlatform_t xSpiPlatform_t;

#include "spi_arch.h"

/* Module Defines -------------------------------------------*/
// clang-format off
// clang-format on

#define SPI_MODULE_GET( NAME ) xModule_##NAME

#define SPI_MODULE_CREATE( NAME, HANDLE, IRQ )                                  \
	SPI_MODULE_PLATFORM_PREFIX( NAME );                                         \
	static xSpiModule_t xModule_##NAME = {                                      \
		.xBusMutexHandle		 = NULL,                                        \
		.xBusMutexStorage		 = { { 0 } },                                   \
		.xTransactionDoneHandle  = NULL,                                        \
		.xTransactionDoneStorage = { { 0 } },                                   \
		.xPlatform				 = SPI_MODULE_PLATFORM_DEFAULT( NAME, HANDLE ), \
		.pxCurrentConfig		 = NULL,                                        \
		.bBusClaimed			 = false,                                       \
		.bCsAsserted			 = false                                        \
	};                                                                          \
	SPI_MODULE_PLATFORM_SUFFIX( NAME, IRQ );

/* Type Definitions -----------------------------------------*/

typedef enum _eSpiClockMode_t {
	eSpiClockMode0 = 0, // Clock Idle State = Low. Sample on Rising Edge.
	eSpiClockMode1 = 1, // Clock Idle State = Low. Sample on Falling Edge.
	eSpiClockMode2 = 2, // Clock Idle State = High. Sample on Rising Edge.
	eSpiClockMode3 = 3  // Clock Idle State = High. Sample on Falling Edge.
} eSpiClockMode_t;

typedef struct _xSpiConfig_t
{
	xGpio_t			xCsGpio;
	uint32_t		ulMaxBitrate;
	eSpiClockMode_t eClockMode;
	uint8_t			ucDummyTx;
	uint8_t			ucMsbFirst;
} xSpiConfig_t;

struct _xSpiModule_t
{
	SemaphoreHandle_t   xBusMutexHandle;
	StaticSemaphore_t   xBusMutexStorage;
	SemaphoreHandle_t   xTransactionDoneHandle;
	StaticSemaphore_t   xTransactionDoneStorage;
	xSpiPlatform_t		xPlatform;
	const xSpiConfig_t *pxCurrentConfig;
	bool				bBusClaimed;
	bool				bCsAsserted;
};

/* Function Declarations ------------------------------------*/

/**
 * Setting up FreeRTOS memory for the SPI driver
 * Should only be called once on program statup
 * \param xSpi The SPI module
 * \return Error State
 */
eModuleError_t eSpiInit( xSpiModule_t *pxSpi );

/**
 * Obtain exclusive access to xSpi bus until vSpiBusEnd is called
 * \param xSpi The SPI module
 * \param xConfig The SPI configuration instance
 * \param xTimeout Maximum time period to block while waiting for bus access
 * \return Error State
 */
eModuleError_t eSpiBusStart( xSpiModule_t *pxSpi, const xSpiConfig_t *xConfig, TickType_t xTimeout );

/**
 * Release exclusive access to xSpi bus
 * \param xSpi The SPI module
 */
void vSpiBusEnd( xSpiModule_t *pxSpi );

/**
 * Lockout other tasks from accessing the SPI bus
 * \param xSpi The SPI module
 * \param bEnableLockout Enable or Disable lockout
 */
eModuleError_t eSpiBusLockout( xSpiModule_t *pxSpi, bool bEnableLockout, TickType_t xTimeout );

/**
 * Assert CS line
 * \param xSpi The SPI module
 * \return Error State
 */
void vSpiCsAssert( xSpiModule_t *pxSpi );

/**
 * Release CS line
 * \param xSpi The SPI module
 * \return Error State
 */
void vSpiCsRelease( xSpiModule_t *pxSpi );

/**
 * Return if CS line is currently asserted
 * \param xSpi The SPI module
 * \return Chip select assertion state
 */
inline bool bSpiCsIsAsserted( xSpiModule_t *pxSpi )
{
	return pxSpi->bCsAsserted;
}

/**
 * Return if the bus is currently claimed
 * \param xSpi The SPI module
 * \return Bus claim state
 */
inline bool bSpiBusIsClaimed( xSpiModule_t *pxSpi )
{
	return pxSpi->bBusClaimed;
}

/**
 * DMA Driven SPI Transmission
 * Functions block until SPI transaction is complete.
 * \param xSpi The SPI module
 * \param ucBuffer A buffer of data
 * \param ulLength The length of the buffer of data
 */
void vSpiTransmit( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen );

void vSpiReceive( xSpiModule_t *pxSpi, void *pvBuffer, uint32_t ulBufferLen );

void vSpiTransfer( xSpiModule_t *pxSpi, void *pvTxBuffer, void *pvRxBuffer, uint32_t ulBufferLen );

#endif /* __LIBS_CSIRO_INTERFACE_SPI */
