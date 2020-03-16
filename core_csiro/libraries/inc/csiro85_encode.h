/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: csiro85_encode.h
 * Creation_Date: 12/03/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Implementation of CSIRO's version of base85 encoding.
 * Use case is encoding binary data into a UTF-8 string
 * while avoiding code-points such as NULL which mobile
 * devices can treat specially.
 * 
 */
#ifndef __CSIRO_CORE_CSIRO85_ENCODE
#define __CSIRO_CORE_CSIRO85_ENCODE
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Encode a binary array using base-85
 *  
 *          The output of this function is a valid ascii string.
 *          This encoding scheme maps 4 input bytes to 5 output bytes
 * 
 * @note    The input length to this function must be a multiple of 4 currently
 * 
 * @retval  The length of the base-85 encoded buffer
 */
uint32_t ulCsiro85Encode( uint8_t *pucBinary, uint32_t ulBinaryLen, uint8_t *pucEncoded, uint32_t ulEncodedMaxLen );

/**@brief Decode a base-85 encoded array
 * 
 * @note    The input length to this function must be a multiple of 5 
 * 
 * @retval  The length of the decoded buffer
 */
uint32_t ulCsiro85Decode( uint8_t *pucEncoded, uint32_t ulEncodedLen, uint8_t *pucBinary, uint32_t ulBinaryMaxLen );

/**@brief Validate whether an array is valid base-85
 * 
 * @retval  True when the provided array is a valid target for decoding
 * @retval  False when not valid
 */
bool bCsiro85Valid( uint8_t *pucEncoded, uint32_t ulEncodedLen );

#endif /* __CSIRO_CORE_CSIRO85_ENCODE */
