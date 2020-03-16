/*
 * Copyright (c) 2012, Commonwealth Scientific and Industrial Research
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
 * This file should be part of the FreeRTOS operating system.
 *
 * Filename: stats.h
 * 
 * Modified on: 3/10/2012
 * Description: Maths library performing calculations for various statistics.
 * 
 * Author: Josh Riddell
 * 
 * Style modified by John Scolaro <John.Scolaro@data61.csiro.au> on 3/10/2018 for this repo.
 *
 * Adapted from http://seat.massey.ac.nz/research/centres/SPRG/pdfs/2013_IVCNZ_214.pdf
 * 
 */

#ifndef __CORE_CSIRO_STATS_H__
#define __CORE_CSIRO_STATS_H__
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

#include "tdf.h"

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

/**@brief Usable output of a @xStats_t object */
typedef struct xStatsSummary_t
{
	int32_t variance; /**< Statistical Variance, equal to the square of the Standard Deviation */
	int32_t last;	 /**< Previous sample */
	int32_t mean;	 /**< Average of all samples */
	int32_t max;	  /**< Maximum sample value */
	int32_t min;	  /**< Minimum sample value */
	int32_t sum;	  /**< Total of all samples */
	int32_t n;		  /**< Number of samples analyzed */
} ATTR_PACKED xStatsSummary_t;

/**@brief Internal state of the stats object 
 * 
 * Implemented as per #pdf_reference_here
*/
typedef struct xStats_t
{
	int32_t last; /**< Previous sample */
	int32_t max;  /**< Maximum sample value */
	int32_t min;  /**< Minimum sample value */
	int32_t sum;  /**< Total of all samples */
	int32_t p;	/**< Magic */
	int32_t w;	/**< Magic */
	int32_t v;	/**< Magic */
	int32_t m;	/**< Magic */
	int32_t n;	/**< Number of samples analyzed */
} ATTR_PACKED xStats_t;

/* Function Declarations ------------------------------------*/

// Stats object functions

/**@brief Erase all history of a xStats_t object
 *
 * @param[in] pxStats			Stats object to reset
 */
void vStatsReset( xStats_t *pxStats );

/**@brief Update a xStats_t object with a new sample
 *
 * @param[in] pxStats			Stats object to reset
 * @param[in] lNewSample		New sample to analyze
 */
void vStatsUpdate( xStats_t *pxStats, int32_t lNewSample );

/**@brief Extract statistical information from a xStats_t object
 * 
 * @param[in] pxStats			Stats object to reset
 * @param[out] pxSummary		Output summary structure
 */
void vStatsGetSummary( xStats_t *pxStats, xStatsSummary_t *pxSummary );

/**@brief Populate a stats tdf from a stats summary
 * 
 * @param[in] pxSummary			Stats summary
 * @param[out] pxTdf			Output TDF structure
 */
void vStatsSummaryToTdf( xStatsSummary_t *pxSummary, tdf_stats_summary_t *pxTdf );

// Clamping functions can be used to force the summary into smaller types
bool bStatsClampShortSigned( int16_t *psResolution, int32_t lNumberToClamp );
bool bStatsClampShortUnsigned( uint16_t *pusResolution, int32_t lNumberToClamp );
bool bStatsClampByteSigned( int8_t *pcResolution, int32_t lNumberToClamp );
bool bStatsClampByteUnsigned( uint8_t *pucResolution, int32_t lNumberToClamp );

#endif // __CORE_CSIRO_STATS_H__
