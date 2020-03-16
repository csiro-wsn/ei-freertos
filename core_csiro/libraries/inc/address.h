/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: address.h
 * Creation_Date: 18/06/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Common Address and Address related functionality for core libraries
 * 
 */
#ifndef __CSIRO_CORE_ADDRESS
#define __CSIRO_CORE_ADDRESS
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "memory_operations.h"

/* Module Defines -------------------------------------------*/

// clang-format off

#define MAC_ADDRESS_LENGTH      6

#define BASE_ADDRESS			0x000000000000
#define BROADCAST_ADDRESS       0xFFFFFFFFFFFF
#define LOCAL_ADDRESS           xLocalAddress

#define ADDRESS_FMT				"%012llX"

// clang-format on

/* Type Definitions -----------------------------------------*/

typedef uint64_t xAddress_t;

extern xAddress_t xLocalAddress;

/* Function Declarations ------------------------------------*/

static inline void vAddressPack( uint8_t pucAddress[MAC_ADDRESS_LENGTH], xAddress_t xAddress )
{
	LE_U48_PACK( pucAddress, xAddress );
}

static inline xAddress_t xAddressUnpack( const uint8_t pucAddress[MAC_ADDRESS_LENGTH] )
{
	return (xAddress_t) LE_U48_EXTRACT( pucAddress );
}

static inline bool bIsLocalAddress( xAddress_t xAddress )
{
	return xAddress == LOCAL_ADDRESS;
}

static inline bool bIsBroadcastAddress( xAddress_t xAddress )
{
	return xAddress == BROADCAST_ADDRESS;
}

static inline bool bIsBaseAddress( xAddress_t xAddress )
{
	return xAddress == BASE_ADDRESS;
}

static inline bool bAddressesMatch( xAddress_t xAddressA, xAddress_t xAddressB )
{
	return xAddressA == xAddressB;
}

static inline bool bAddressesU24Match( xAddress_t xAddressA, xAddress_t xAddressB )
{
	return ( xAddressA & 0xFFFFFF000000 ) == ( xAddressB & 0xFFFFFF000000 );
}

#endif /* __CSIRO_CORE_ADDRESS */
