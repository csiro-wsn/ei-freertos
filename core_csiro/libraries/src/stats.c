/*---------------------------------------------------------------------------*/
/*           Includes                                                        */
/*---------------------------------------------------------------------------*/

#include <stdbool.h>

#include "csiro_math.h"
#include "stats.h"

/*---------------------------------------------------------------------------*/
/*           Static misc functions                                           */
/*---------------------------------------------------------------------------*/

/*
 *  Clamps x between min and max (inclusive). Returns true if clamped. Value
 *  returned through pointer.
 */
static bool bClamp( int32_t *lResolution, int32_t lNumberToClamp, int32_t lMin, int32_t lMax )
{
	if ( lNumberToClamp > lMax ) {
		*lResolution = lMax;
		return true;
	}
	else if ( lNumberToClamp < lMin ) {
		*lResolution = lMin;
		return true;
	}
	else {
		*lResolution = lNumberToClamp;
		return false;
	}
}

/*---------------------------------------------------------------------------*/
/*           Static stats helper functions                                   */
/*---------------------------------------------------------------------------*/

static void vUpdateCount( xStats_t *pxStats )
{
	++pxStats->n;
}

static void vUpdateSum( xStats_t *pxStats, int32_t lSum )
{
	pxStats->sum += lSum;
}

static void vUpdateMax( xStats_t *pxStats, int32_t lMax )
{
	if ( lMax > pxStats->max ) {
		pxStats->max = lMax;
	}
}

static void vUpdateMin( xStats_t *pxStats, int32_t lMin )
{
	if ( lMin < pxStats->min ) {
		pxStats->min = lMin;
	}
}

static int32_t lUpdateInitialStats( xStats_t *pxStats, int32_t x )
{
	int32_t dm;
	int32_t p_dash;
	int32_t w_dash;
	int32_t w_double_dash;
	int32_t x_diff_from_mean;

	x_diff_from_mean = x - pxStats->m;

	// equation 4
	p_dash = pxStats->p + x_diff_from_mean;

	// equation 5
	dm = SIGNED_DIVISION_ROUNDED( p_dash, pxStats->n );

	// equation 6
	pxStats->m += dm;

	// equation 7
	pxStats->p = p_dash - pxStats->n * dm;

	// equation 11
	w_dash = pxStats->w + x_diff_from_mean * x_diff_from_mean - pxStats->v;

	// equation 12
	w_double_dash = w_dash - 2 * dm * p_dash + pxStats->n * dm * dm;

	return w_double_dash;
}

// adapted from http://seat.massey.ac.nz/research/centres/SPRG/pdfs/2013_IVCNZ_214.pdf
static void vUpdateAdvancedStats( xStats_t *pxStats, int32_t w_double_dash )
{
	int32_t dv;

	// equation 13
	dv = SIGNED_DIVISION_ROUNDED( w_double_dash, pxStats->n - 1 );

	// equation 14
	pxStats->v = pxStats->v + dv;
	pxStats->w = w_double_dash - ( pxStats->n - 1 ) * dv;
}

/*---------------------------------------------------------------------------*/
/*           Static stats logical functions                                  */
/*---------------------------------------------------------------------------*/
static bool bAdvancedStatsCalculable( xStats_t *pxStats )
{
	return pxStats->n > 1;
}

static bool bNoDataAdded( xStats_t *pxStats )
{
	return pxStats->n == 0;
}

/*---------------------------------------------------------------------------*/
/*           Static stats end calculation functions                          */
/*---------------------------------------------------------------------------*/

// adapted from http://seat.massey.ac.nz/research/centres/SPRG/pdfs/2013_IVCNZ_214.pdf
static int32_t lCalculateVariance( xStats_t *pxStats )
{
	// equation 9 (with last term neglected as explained below the equation in the paper)
	return pxStats->v + SIGNED_DIVISION_ROUNDED( pxStats->w, pxStats->n - 1 );
}

// adapted from http://seat.massey.ac.nz/research/centres/SPRG/pdfs/2013_IVCNZ_214.pdf
static int32_t lCalculateMean( xStats_t *pxStats )
{
	// equation 8
	return pxStats->m + SIGNED_DIVISION_ROUNDED( pxStats->p, pxStats->n );
}

/*---------------------------------------------------------------------------*/
/*           External stats functions                                        */
/*---------------------------------------------------------------------------*/

void vStatsReset( xStats_t *pxStats )
{
	pxStats->p	= 0;
	pxStats->w	= 0;
	pxStats->v	= 0;
	pxStats->m	= 0;
	pxStats->n	= 0;
	pxStats->max  = INT32_MIN;
	pxStats->min  = INT32_MAX;
	pxStats->sum  = 0;
	pxStats->last = 0;
}

void vStatsUpdate( xStats_t *pxStats, int32_t lNewSample )
{
	int32_t w_double_dash;
	pxStats->last = lNewSample;

	/* Check overflow case */
	if ( pxStats->n == INT32_MAX ) {
		return;
	}

	vUpdateCount( pxStats );
	vUpdateSum( pxStats, lNewSample );
	vUpdateMax( pxStats, lNewSample );
	vUpdateMin( pxStats, lNewSample );

	w_double_dash = lUpdateInitialStats( pxStats, lNewSample );

	if ( bAdvancedStatsCalculable( pxStats ) ) {
		vUpdateAdvancedStats( pxStats, w_double_dash );
	}
}

void vStatsGetSummary( xStats_t *pxStats, xStatsSummary_t *pxSummary )
{
	if ( bAdvancedStatsCalculable( pxStats ) ) {
		pxSummary->variance = lCalculateVariance( pxStats );
		pxSummary->mean		= lCalculateMean( pxStats );
	}
	else {
		pxSummary->variance = 0;
		pxSummary->mean		= pxStats->sum; // only one element
	}

	if ( bNoDataAdded( pxStats ) ) {
		pxSummary->max = 0;
		pxSummary->min = 0;
	}
	else {
		pxSummary->max = pxStats->max;
		pxSummary->min = pxStats->min;
	}

	pxSummary->sum  = pxStats->sum;
	pxSummary->n	= pxStats->n;
	pxSummary->last = pxStats->last;
}

void vStatsSummaryToTdf( xStatsSummary_t *pxSummary, tdf_stats_summary_t *pxTdf )
{
	pxTdf->last		= pxSummary->last;
	pxTdf->max		= pxSummary->max;
	pxTdf->mean		= pxSummary->mean;
	pxTdf->min		= pxSummary->min;
	pxTdf->n		= (uint32_t) pxSummary->n;
	pxTdf->sum		= pxSummary->sum;
	pxTdf->variance = pxSummary->variance;
}

/*---------------------------------------------------------------------------*/
/*           External stats helper functions                                 */
/*---------------------------------------------------------------------------*/

bool bStatsClampShortSigned( int16_t *psResolution, int32_t lNumberToClamp )
{
	int32_t lTmpRes;

	uint8_t ucReturn = bClamp( &lTmpRes, lNumberToClamp, INT16_MIN, INT16_MAX );

	*psResolution = (int16_t) lTmpRes;
	return ucReturn;
}

bool bStatsClampShortUnsigned( uint16_t *pusResolution, int32_t lNumberToClamp )
{
	int32_t lTmpRes;

	uint8_t ucReturn = bClamp( &lTmpRes, lNumberToClamp, 0, UINT16_MAX );

	*pusResolution = (int16_t) lTmpRes;
	return ucReturn;
}

bool bStatsClampByteSigned( int8_t *pcResolution, int32_t lNumberToClamp )
{
	int32_t lTmpRes;

	uint8_t ucReturn = bClamp( &lTmpRes, lNumberToClamp, INT8_MIN, INT8_MAX );

	*pcResolution = (int8_t) lTmpRes;
	return ucReturn;
}

bool bStatsClampByteUnsigned( uint8_t *pucResolution, int32_t lNumberToClamp )
{
	int32_t lTmpRes;

	uint8_t ucReturn = bClamp( &lTmpRes, lNumberToClamp, 0, UINT8_MAX );

	*pucResolution = (uint8_t) lTmpRes;
	return ucReturn;
}
