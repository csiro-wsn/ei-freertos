/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: crc.h
 * Creation_Date: 10/10/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * 
 * Generic CRC implementation
 * 
 * Best CRC Reference for named parameters
 * 	http://reveng.sourceforge.net/crc-catalogue/
 * 
 * Best explaination of calculations
 * 	http://www.ross.net/crc/download/crc_v3.txt
 */
#ifndef __CSIRO_CORE_ARCH_CRC
#define __CSIRO_CORE_ARCH_CRC
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eCrcPolynomial_t {
	CRC32_IEEE_802_3, // 0x04C11DB7
	CRC16_CCITT,	  // 0x1021
	CRC16_IBM_SDLC,   // 0x1021
	CRC16_IEC16_MBUS, // 0x3D65
	CRC16_ZIGBEE,	 // 0x8005
	CRC16_802_15_4,   // 0x8005
	CRC16_USB		  // 0x8005
} eCrcPolynomial_t;

/* Function Declarations ------------------------------------*/

/**
 * Setup underlying CRC hardware
 */
void vCrcInit( void );

/**
 * Setup the CRC hardware for a calculation
 * \param ePolynomial The CRC Polynomial to be used
 * \param ulInitValue The initial value of the CRC
 */
void vCrcStart( eCrcPolynomial_t ePolynomial, uint32_t ulInitValue );

/**
 * Setup the CRC hardware for a calculation
 * \param pucData 		New data to add to the CRC
 * \param ulDataLen 	Length of data to add
 * \param bTerminate	True to release CRC hardware for other users
 * \return 				The current value of the CRC
 */
uint32_t ulCrcCalculate( uint8_t *pucData, uint32_t ulDataLen, bool bTerminate );

#endif /* __CSIRO_CORE_ARCH_CRC */
