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
 * Filename: compiler_intrinsics.h
 * Creation_Date: 12/6/2018
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Defines function and value intrinsics for various compilers
 * Current supported compiler families:
 *  arm-none-eabi-gcc   Device cross compiler
 *  cc                  Cpputest on Ubuntu
 *  clang               Cpputest on MacOS
 * 
 */

#ifndef __CORE_CSIRO_UTIL_COMPILER_INTRINSICS
#define __CORE_CSIRO_UTIL_COMPILER_INTRINSICS

/* Includes -------------------------------------------------*/

#include "stdint.h"

#include "csiro_math.h"

/* Module Defines -------------------------------------------*/

#ifdef PYCPARSER
#define __attribute__( x )
#define asm
#define volatile( x )
#endif /* PYCPARSER */

// Common Compiler
#define UNUSED( x ) ( (void) ( x ) )

/* Counts leading zeros, single assembly instruction, undefined output for inputs of 0 */
#define COUNT_LEADING_ZEROS( x ) __builtin_clz( x )
/* Counts trailing zeros, unsure implementation, undefined output for inputs of 0 */
#define COUNT_TRAILING_ZEROS( x ) __builtin_ctz( x )
/* Returns one plus the index of the first set bit, unsure implementation */
#define FIND_FIRST_SET( x ) __builtin_ffs( x )
/* Counts number of bits set to '1' in an unsigned integer */
#define COUNT_ONES( unsigned_int ) __builtin_popcount( unsigned_int )

// Compiler time assertion errors
#define CASSERT( predicate, file ) _impl_CASSERT_LINE( predicate, __LINE__, file )

#define _impl_PASTE( a, b ) a##b
#define _impl_CASSERT_LINE( predicate, line, file ) \
	typedef char _impl_PASTE( assertion_failed_##file##_, line )[2 * !!(predicate) -1];

#define ABS( x ) __builtin_abs( x )

#ifndef STRINGIFY
#define _STRINGIFY( x ) #x
#define STRINGIFY( x ) _STRINGIFY( x )
#endif

#define ALIGNED_POINTER( x, n ) __builtin_assume_aligned( x, n )
#define IS_ALIGNED( p, n ) ( ( (uintptr_t) p % n ) == 0 )

#define SIZEOF_WORDS( t ) ( ROUND_UP( sizeof( t ), 4 ) / 4 )

#define ATTR_ALIGNED( x ) __attribute__( ( aligned( x ) ) )
#define ATTR_PACKED __attribute__( ( packed ) )
#define ATTR_NORETURN __attribute__( ( noreturn ) )
#define ATTR_UNUSED __attribute__( ( unused ) )
#define ATTR_SECTION( sec ) __attribute__( ( section( sec ) ) )
#define ATTR_NAKED __attribute__( ( naked ) )
#define ATTR_WEAK __attribute__( ( weak ) )
#define ATTR_FALLTHROUGH __attribute__( ( fallthrough ) )

#define ATTR_RAM_FUNCTION __attribute__( ( section( ".ram_function" ) ) )

#define DO_PRAGMA( x ) _Pragma( #x )

// arm-none-eabi-gcc
#if defined __arm__

#define __INLINE inline
#define __STATIC_INLINE static inline

#define WARNING_BUILD( a, b, c ) a##b##c

#define COMPILER_WARNING_DISABLE( warn ) \
	_Pragma( "GCC diagnostic push" )     \
		DO_PRAGMA( GCC diagnostic ignored warn )

#define COMPILER_WARNING_ENABLE() \
	_Pragma( "GCC diagnostic pop" )

__attribute__( ( always_inline ) ) static inline uint32_t __get_LR( void )
{
	register uint32_t result;

	asm volatile( "MOV %0, LR\n"
				  : "=r"( result ) );
	return ( result );
}

__attribute__( ( always_inline ) ) static inline uint32_t __get_PC( void )
{
	register uint32_t result;

	asm volatile( "MOV %0, PC\n"
				  : "=r"( result ) );
	return ( result );
}

// clang (c++)
#elif defined __clang__

#define __INLINE

#undef ATTR_FALLTHROUGH
#define ATTR_FALLTHROUGH

// Redefine ATTR_SECTION as clang doesn't like it
#undef ATTR_SECTION
#define ATTR_SECTION( sec )
#define ATTR_UNUSED __attribute__( ( unused ) )

#define COMPILER_WARNING_DISABLE( warn ) \
	_Pragma( "clang diagnostic push" )   \
		DO_PRAGMA( clang diagnostic ignored warn )

#define COMPILER_WARNING_ENABLE() \
	_Pragma( "clang diagnostic pop" )

__attribute__( ( always_inline ) ) static inline uint32_t __get_LR( void )
{
	return 0;
}

__attribute__( ( always_inline ) ) static inline uint32_t __get_PC( void )
{
	return 0;
}

// Redefine ATTR_NAKED as it is only supported on ARM, AVR, MCORE, RX and SPU compilers.
#undef ATTR_NAKED
#define ATTR_NAKED

// cc
#elif defined __unix__

#define __INLINE inline

#define COMPILER_WARNING_DISABLE( warn ) \
	_Pragma( "GCC diagnostic push" )     \
		DO_PRAGMA( GCC diagnostic ignored warn )

#define COMPILER_WARNING_ENABLE() \
	_Pragma( "GCC diagnostic pop" )

__attribute__( ( always_inline ) ) static inline uint32_t __get_LR( void )
{
	return 0;
}

__attribute__( ( always_inline ) ) static inline uint32_t __get_PC( void )
{
	return 0;
}

// Redefine ATTR_NAKED as it is only supported on ARM, AVR, MCORE, RX and SPU compilers.
#undef ATTR_NAKED
#define ATTR_NAKED

#endif

/* Type Definitions -----------------------------------------*/
/* Function Declarations ------------------------------------*/

#endif /* __CORE_CSIRO_UTIL_COMPILER_INTRINSICS */
