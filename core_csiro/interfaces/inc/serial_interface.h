/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: serial_interface.h
 * Creation_Date: 10/10/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Serial Interface
 * 
 */
#ifndef __CSIRO_CORE_SERIAL_INTERFACE
#define __CSIRO_CORE_SERIAL_INTERFACE
/* Includes -------------------------------------------------*/

#include <stdarg.h>
#include <stdint.h>

#include "core_types.h"

/* Module Defines -------------------------------------------*/
// clang-format off

/* Large enough for a 128 byte RPC + routing info */
#define SERIAL_INTERFACE_DEFAULT_SIZE	180

// clang-format on
/* Type Definitions -----------------------------------------*/

/* Function Definitions */
typedef void ( *fnSerialEnable_t )( void *pvContext );
typedef void ( *fnSerialDisable_t )( void *pvContext );
typedef eModuleError_t ( *fnSerialWriter_t )( void *pvContext, const char *pcFormat, va_list pvArgs );
typedef char *( *fnSerialClaimBuffer_t )( void *pvContext, uint32_t *pulBufferLen );
typedef void ( *fnSerialSendBuffer_t )( void *pvContext, const char *pcBuffer, uint32_t ulBufferLen );
typedef void ( *fnSerialReleaseBuffer_t )( void *pvContext, char *pucBuffer );

/**@brief Serial Backend Implementation */
typedef struct xSerialBackend_t
{
	fnSerialEnable_t		fnEnable;
	fnSerialDisable_t		fnDisable;
	fnSerialWriter_t		fnWrite;
	fnSerialClaimBuffer_t   fnClaimBuffer;
	fnSerialSendBuffer_t	fnSendBuffer;
	fnSerialReleaseBuffer_t fnReleaseBuffer;
} xSerialBackend_t;

typedef struct xSerialModule_t
{
	xSerialBackend_t *pxImplementation;
	void *			  pvContext;
} xSerialModule_t;

/* Function Declarations ------------------------------------*/

#endif /* __CSIRO_CORE_SERIAL_INTERFACE */
