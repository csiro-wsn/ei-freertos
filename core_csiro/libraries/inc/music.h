/*
 * Copyright (c) 2020, Commonwealth Scientific and Industrial Research 
 * Organisation (CSIRO) ABN 41 687 119 230.
 *
 * Filename: music.h
 * Creation_Date: 11/04/
 * Author: Jordan Yates <Jordan.Yates@data61.csiro.au>
 *
 * Helper functions for playing music on a buzzer
 *
 */
#ifndef __CSIRO_CORE_MUSIC
#define __CSIRO_CORE_MUSIC
/* Includes -------------------------------------------------*/

#include <stdint.h>

/* Module Defines -------------------------------------------*/

// clang-format off
// clang-format on

/* Type Definitions -----------------------------------------*/

typedef enum eNotes_t {
	C0		= 16350,
	Cp0_Db0 = 17320,
	D0		= 18350,
	Dp0_Eb0 = 19450,
	E0		= 20600,
	F0		= 21830,
	Fp0_Gb0 = 23120,
	G0		= 24500,
	Gp0_Ab0 = 25960,
	A0		= 27500,
	Ap0_Bb0 = 29140,
	B0		= 30870,
	C1		= 32700,
	Cp1_Db1 = 34650,
	D1		= 36710,
	Dp1_Eb1 = 38890,
	E1		= 41200,
	F1		= 43650,
	Fp1_Gb1 = 46250,
	G1		= 49000,
	Gp1_Ab1 = 51910,
	A1		= 55000,
	Ap1_Bb1 = 58270,
	B1		= 61740,
	C2		= 65410,
	Cp2_Db2 = 69300,
	D2		= 73420,
	Dp2_Eb2 = 77780,
	E2		= 82410,
	F2		= 87310,
	Fp2_Gb2 = 92500,
	G2		= 98000,
	Gp2_Ab2 = 103830,
	A2		= 110000,
	Ap2_Bb2 = 116540,
	B2		= 123470,
	C3		= 130810,
	Cp3_Db3 = 138590,
	D3		= 146830,
	Dp3_Eb3 = 155560,
	E3		= 164810,
	F3		= 174610,
	Fp3_Gb3 = 185000,
	G3		= 196000,
	Gp3_Ab3 = 207650,
	A3		= 220000,
	Ap3_Bb3 = 233080,
	B3		= 246940,
	C4		= 261630,
	Cp4_Db4 = 277180,
	D4		= 293660,
	Dp4_Eb4 = 311130,
	E4		= 329630,
	F4		= 349230,
	Fp4_Gb4 = 369990,
	G4		= 392000,
	Gp4_Ab4 = 415300,
	A4		= 440000,
	Ap4_Bb4 = 466160,
	B4		= 493880,
	C5		= 523250,
	Cp5_Db5 = 554370,
	D5		= 587330,
	Dp5_Eb5 = 622250,
	E5		= 659250,
	F5		= 698460,
	Fp5_Gb5 = 739990,
	G5		= 783990,
	Gp5_Ab5 = 830610,
	A5		= 880000,
	Ap5_Bb5 = 932330,
	B5		= 987770,
	C6		= 1046500,
	Cp6_Db6 = 1108730,
	D6		= 1174660,
	Dp6_Eb6 = 1244510,
	E6		= 1318510,
	F6		= 1396910,
	Fp6_Gb6 = 1479980,
	G6		= 1567980,
	Gp6_Ab6 = 1661220,
	A6		= 1760000,
	Ap6_Bb6 = 1864660,
	B6		= 1975530,
	C7		= 2093000,
	Cp7_Db7 = 2217460,
	D7		= 2349320,
	Dp7_Eb7 = 2489020,
	E7		= 2637020,
	F7		= 2793830,
	Fp7_Gb7 = 2959960,
	G7		= 3135960,
	Gp7_Ab7 = 3322440,
	A7		= 3520000,
	Ap7_Bb7 = 3729310,
	B7		= 3951070,
	C8		= 4186010,
	Cp8_Db8 = 4434920,
	D8		= 4698630,
	Dp8_Eb8 = 4978030,
	E8		= 5274040,
	F8		= 5587650,
	Fp8_Gb8 = 5919910,
	G8		= 6271930,
	Gp8_Ab8 = 6644880,
	A8		= 7040000,
	Ap8_Bb8 = 7458620,
	B8		= 7902130,
} eNotes_t;

typedef enum eSignature_t {
	SIGNATURE_2_4,
	SIGNATURE_3_4,
	SIGNATURE_4_4
} eSignature_t;

typedef enum eNoteTime_t {
	WHOLE_NOTE,
	HALF_NOTE,
	QUARTER_NOTE,
	EIGHT_NOTE,
	SIXTEENTH_NOTE
} eNoteTime_t;

typedef struct eNote_t
{
	eNotes_t eNode;
	uint8_t  ucDuration;
} eNote_t_t;

/* Function Declarations ------------------------------------*/

// void vInitialiseMusic( uint8_t ucBPM, eSignature_t eSignature );

// void vPlayBar();

#endif /* __CSIRO_CORE_MUSIC */
