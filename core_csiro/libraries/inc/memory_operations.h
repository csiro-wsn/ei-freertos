/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: memory_operations.h
 * Creation_Date: 15/07/2018
 * Author: Jordan Yates <jordan.yates@data61.csiro.au>
 *
 * Common memory operations
 * 
 */
#ifndef __CSIRO_CORE_UTIL_MEMORY_OPERATIONS
#define __CSIRO_CORE_UTIL_MEMORY_OPERATIONS
/* Includes -------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include "compiler_intrinsics.h"

/* Module Defines -------------------------------------------*/
// clang-format off

#define __MASK_OFFSET( ulMask ) 							COUNT_TRAILING_ZEROS( ulMask )

#define MASK_READ( ulExisting, ulMask ) 					((ulExisting & (ulMask)) >> __MASK_OFFSET( ulMask ))
#define MASK_WRITE( ulValue, ulMask)						(((ulValue) << __MASK_OFFSET( ulMask )) & (ulMask))
#define MASK_CLEAR( ulExisting, ulMask)						((ulExisting) & ~(ulMask))
#define MASK_OVERWRITE( ulExisting, ulMaskValue, ulMask )	( MASK_CLEAR( ulExisting, ulMask ) | MASK_WRITE( ulMaskValue, ulMask ) )

#define BYTE_READ( ulExisting, ucIndex )					(((ulExisting) >> (8 * ucIndex)) & 0xFF)
#define BYTE_WRITE( ucByte, ucIndex )						((ucByte) << (8 * ucIndex))
#define BYTE_CLEAR( ulExisting, ucIndex)					((ulExisting) & ~(0xFF << (8 * (ucIndex))))
#define BYTE_OVERWRITE( ulExisting, ucByte, ucIndex )		( BYTE_CLEAR( ulExisting, ucIndex ) | BYTE_WRITE( ucByte, ucIndex ) )

#define BIT_READ( ulExisting, ucOffset )					(((ulExisting) & (0x01 << (ucOffset))) >> (ucOffset))
#define BIT_WRITE( bValue, ucOffset )						(!!(bValue) << ucOffset)
#define BIT_CLEAR( ulExisting, ucOffset)					((ulExisting) & ~(0x01 << (ucOffset)))
#define BIT_OVERWRITE( ulExisting, bValue, ucOffset)		( BIT_CLEAR( ulExisting, ucOffset ) | BIT_WRITE( bValue, ucOffset ) )

#define LE_U48_EXTRACT( pucStart )      ((((uint64_t) (pucStart)[5] ) << 40 ) | (((uint64_t) (pucStart)[4] ) << 32 ) | (((uint64_t) (pucStart)[3] ) << 24 ) | (((uint64_t) (pucStart)[2] ) << 16 ) | (((uint64_t) (pucStart)[1] ) << 8 ) | ((uint64_t) (pucStart)[0] ))
#define LE_U32_EXTRACT( pucStart )      ((((uint32_t) (pucStart)[3] ) << 24 ) | (((uint32_t) (pucStart)[2] ) << 16 ) | (((uint32_t) (pucStart)[1] ) << 8 ) | ((uint32_t) (pucStart)[0] ))
#define LE_U24_EXTRACT( pucStart )      ((((uint32_t) (pucStart)[2] ) << 16 ) | (((uint32_t) (pucStart)[1] ) << 8 ) | ((uint32_t) (pucStart)[0] ))
#define LE_U16_EXTRACT( pucStart )      ((((uint16_t) (pucStart)[1] ) << 8 ) | ( (uint16_t) (pucStart)[0] ))
#define LE_U8_EXTRACT( pucStart )       ((uint8_t) (pucStart)[0] )

#define BE_U48_EXTRACT( pucStart )      ((((uint64_t) (pucStart)[0] ) << 40 ) | (((uint64_t) (pucStart)[1] ) << 32 ) | (((uint64_t) (pucStart)[2] ) << 24 ) | (((uint64_t) (pucStart)[3] ) << 16 )| (((uint64_t) (pucStart)[4] ) << 8 )| ((uint64_t) (pucStart)[5] ))
#define BE_U32_EXTRACT( pucStart )      ((((uint32_t) (pucStart)[0] ) << 24 ) | (((uint32_t) (pucStart)[1] ) << 16 ) | (((uint32_t) (pucStart)[2] ) << 8 ) | ((uint32_t) (pucStart)[3] ))
#define BE_U24_EXTRACT( pucStart )      ((((uint32_t) (pucStart)[0] ) << 16 ) | (((uint32_t) (pucStart)[1] ) << 8 ) | ((uint32_t) (pucStart)[2] ))
#define BE_U16_EXTRACT( pucStart )      ((((uint16_t) (pucStart)[0] ) << 8 ) | ((uint16_t) (pucStart)[1] ))
#define BE_U8_EXTRACT( pucStart )       ((uint8_t) (pucStart)[0] )

#define LE_U48_PACK( pucDst, ulSrc )	do {(pucDst)[0] = BYTE_READ(ulSrc, 0); (pucDst)[1] = BYTE_READ(ulSrc, 1); (pucDst)[2] = BYTE_READ(ulSrc, 2); (pucDst)[3] = BYTE_READ(ulSrc, 3); (pucDst)[4] = BYTE_READ(ulSrc, 4); (pucDst)[5] = BYTE_READ(ulSrc, 5);} while (0)
#define LE_U32_PACK( pucDst, ulSrc )	do {(pucDst)[0] = BYTE_READ(ulSrc, 0); (pucDst)[1] = BYTE_READ(ulSrc, 1); (pucDst)[2] = BYTE_READ(ulSrc, 2); (pucDst)[3] = BYTE_READ(ulSrc, 3);} while (0)
#define LE_U24_PACK( pucDst, ulSrc )	do {(pucDst)[0] = BYTE_READ(ulSrc, 0); (pucDst)[1] = BYTE_READ(ulSrc, 1); (pucDst)[2] = BYTE_READ(ulSrc, 2);} while (0)
#define LE_U16_PACK( pucDst, ulSrc )	do {(pucDst)[0] = BYTE_READ(ulSrc, 0); (pucDst)[1] = BYTE_READ(ulSrc, 1);} while (0)

#define BE_U48_PACK( pucDst, ulSrc )	do {(pucDst)[5] = BYTE_READ(ulSrc, 0); (pucDst)[4] = BYTE_READ(ulSrc, 1); (pucDst)[3] = BYTE_READ(ulSrc, 2); (pucDst)[2] = BYTE_READ(ulSrc, 3); (pucDst)[1] = BYTE_READ(ulSrc, 4); (pucDst)[0] = BYTE_READ(ulSrc, 5);} while (0)
#define BE_U32_PACK( pucDst, ulSrc )	do {(pucDst)[3] = BYTE_READ(ulSrc, 0); (pucDst)[2] = BYTE_READ(ulSrc, 1); (pucDst)[1] = BYTE_READ(ulSrc, 2); (pucDst)[0] = BYTE_READ(ulSrc, 3);} while (0)
#define BE_U24_PACK( pucDst, ulSrc )	do {(pucDst)[2] = BYTE_READ(ulSrc, 0); (pucDst)[1] = BYTE_READ(ulSrc, 1); (pucDst)[0] = BYTE_READ(ulSrc, 2);} while (0)
#define BE_U16_PACK( pucDst, ulSrc )	do {(pucDst)[1] = BYTE_READ(ulSrc, 0); (pucDst)[0] = BYTE_READ(ulSrc, 1);} while (0)

#define BYTE_SWAP_32( x ) __builtin_bswap32( x )
#define BYTE_SWAP_16( x ) __builtin_bswap16( x )

// clang-format on
/* Type Definitions -----------------------------------------*/

typedef struct xBufferBuilder_t
{
	uint8_t *pucBuffer;
	uint32_t ulIndex;
	uint32_t ulMaxLen;
} xBufferBuilder_t;

/* Function Declarations ------------------------------------*/

void *  pvMemset( void *pvPtr, uint8_t ucValue, uint32_t ulLen );
void *  pvMemcpy( void *pvDestination, const void *pvSource, uint32_t ulLen );
int32_t lMemcmp( const void *pvPtr1, const void *pvPtr2, uint32_t ulLen );

uint32_t ulStrLen( const void *pvPtr );

/**@brief Search through an array for the first occurance of a byte
 *
 * @param[in] pucArray			Array to search in
 * @param[in] ucArrayValue		Value to search for
 * @param[in] ulMaximumLength	Maximum length to search over
 *
 * @retval ::UINT32_MAX			Value was not found in array 		
 * @retval ::uint32_t 			Array index that value was found at
 */
uint32_t ulArraySearchByte( const uint8_t *pucArray, uint8_t ucArrayValue, uint32_t ulMaximumLength );

/**@brief Search through an array for the first occurance of a word
 *
 * @param[in] pulArray			Array to search in
 * @param[in] ulArrayValue		Value to search for
 * @param[in] ulMaximumLength	Maximum length to search over
 *
 * @retval ::UINT32_MAX			Value was not found in array 		
 * @retval ::uint32_t 			Array index that value was found at
 */
uint32_t ulArraySearchWord( const uint32_t *pulArray, uint32_t ulArrayValue, uint32_t ulMaximumLength );

/*-----------------------------------------------------------*/

static inline void vBufferBuilderStart( xBufferBuilder_t *pxBuilder, uint8_t *pxBuffer, uint32_t ulBufferMax )
{
	pxBuilder->ulIndex   = 0;
	pxBuilder->pucBuffer = pxBuffer;
	pxBuilder->ulMaxLen  = ulBufferMax;
}

/*-----------------------------------------------------------*/

static inline void vBufferBuilderPushData( xBufferBuilder_t *pxBuilder, const void *pucData, uint32_t ulDataLen )
{
	if ( pxBuilder->ulIndex + ulDataLen <= pxBuilder->ulMaxLen ) {
		pvMemcpy( pxBuilder->pucBuffer + pxBuilder->ulIndex, pucData, ulDataLen );
	}
	pxBuilder->ulIndex += ulDataLen;
}

/*-----------------------------------------------------------*/

static inline void vBufferBuilderPushByte( xBufferBuilder_t *pxBuilder, uint8_t ucByte )
{
	if ( pxBuilder->ulIndex < pxBuilder->ulMaxLen ) {
		pxBuilder->pucBuffer[pxBuilder->ulIndex] = ucByte;
	}
	pxBuilder->ulIndex++;
}

/*-----------------------------------------------------------*/

static inline bool bBufferBuilderValid( xBufferBuilder_t *pxBuilder )
{
	return pxBuilder->ulIndex <= pxBuilder->ulMaxLen;
}

/*-----------------------------------------------------------*/

#endif /* __CSIRO_CORE_UTIL_MEMORY_OPERATIONS */
