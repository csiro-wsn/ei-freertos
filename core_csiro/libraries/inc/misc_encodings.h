/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: misc_encodings.h
 * Creation_Date: 10/05/2019
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Miscellaneous data encodings which are common between drivers
 * 
 */
#ifndef __CSIRO_CORE_MISC_ENCODINGS
#define __CSIRO_CORE_MISC_ENCODINGS
/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/* Function Declarations ------------------------------------*/

/**@brief Compress 32bit latitude to 24bit
 *
 *  Compresses the 32bit latitude format reported by Ublox GPS modules
 *      Valid input range (-900000000, 900000000)
 * 
 *  Output is compressed into an unsigned 24bit representation
 *      0x000000 => -90 degrees
 *      0x800000 => 0 degrees
 *      0xFFFFFF => +90 - delta degrees
 * 
 *  +90 degrees is not directly representable in this format
 * 
 *  Degrees associated with a single bit increment
 *      Delta = 180.0 / 0x1000000
 *            = 0.0000107288 degrees
 *  
 *  Approximate quantisation (Ignoring oblate spheroid)
 *      Quantization = (diameter_earth / 2) / 0x1000000
 *                   = (12'742'000m / 2) / 0x1000000
 *                   = 0.38m
 * 
 *  Rounding is applied per float rules, not integer rules
 *      0.0 - epsilon => 0x800000
 *      0.0           => 0x800000
 *      0.0 + epsilon => 0x800000
 * 
 * @param[in] lLatitude			Latitude scaled by 1e7
 * 
 * @retval						24bit compressed latitude
 */
uint32_t ulLatitudeTo24bit( int32_t lLatitude );

/**@brief Compress 32bit longitude to 24bit
 *
 *  Compresses the 32bit longitude format reported by Ublox GPS modules
 *      Valid input range (-1800000000, 1800000000)
 * 
 *  Output is compressed into an unsigned 24bit representation
 *      0x000000 => -180 degrees
 *      0x800000 => 0 degrees
 *      0xFFFFFF => +180 - delta degrees
 * 
 *  +180 degrees is implicitly representable, as it is equivalent to -180 degrees
 * 
 *  Degrees associated with a single bit increment
 *      Delta = 360.0 / 0x1000000
 *            = 0.0000214577 degrees
 *  
 *  Approximate quantisation
 *      Quantization = diameter_earth / 0x1000000
 *                   = 12'742'000m / 0x1000000
 *                   = 0.76m
 * 
 *  Rounding is applied per float rules, not integer rules
 *      0.0 - epsilon => 0x800000
 *      0.0           => 0x800000
 *      0.0 + epsilon => 0x800000
 *  
 * @param[in] lLongitude		Longitude scaled by 1e7
 * 
 * @retval						24bit compressed longitude
 */
uint32_t ulLongitudeTo24bit( int32_t lLongitude );

#endif /* __CSIRO_CORE_MISC_ENCODINGS */
