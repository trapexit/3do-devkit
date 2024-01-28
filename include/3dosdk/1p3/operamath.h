/***************************************************************\
*								*
* Include file for Opera math routines				*
*								*
* By:  Stephen H. Landrum					*
*								*
* Last update:  17-Dec-93					*
*								*
* Copyright (c) 1992, 1993, The 3DO Company, Inc.               *
* All rights reserved						*
* This document is proprietary and confidential			*
*								*
\***************************************************************/

#pragma force_top_level
#pragma include_only_once


#ifndef __operamath_h
#define __operamath_h

#include "types.h"
#include "nodes.h"
#include "item.h"
#include "task.h"
#include "folio.h"

/***************************************************************\
* Basic typedefs and structures                                 *
\***************************************************************/

/*
 * frac16 is a 16.16 fraction
 * frac32 is a 32.32 fraction
 * frac30 is a 2.30 fraction
 * frac60 is a 4.60 fraction
 * frac14 is a 2.14 fraction (using the upper 16 bits of a 32 bit quantity)
 */

typedef int32 frac16, frac30, frac14;
typedef uint32 ufrac16, ufrac30, ufrac14;
typedef struct int64 { uint32 hi; uint32 lo; } int64, uint64, frac32, ufrac32, frac60, ufrac60;

typedef frac16 vec3f16[3], vec4f16[4], mat33f16[3][3], mat44f16[4][4], mat34f16[4][3];
typedef frac30 vec3f30[3], vec4f30[4], mat33f30[3][3], mat44f30[4][4], mat34f30[4][3];

typedef struct MathFolio {
  Folio mf;
} MathFolio;

extern MathFolio *_MathBase;


/***************************************************************\
* Useful constants                                              *
\***************************************************************/

#define FULLCIRCLE	Convert32_F16(256)
#define HALFCIRCLE	(FULLCIRCLE/2)
#define QUARTERCIRCLE	(FULLCIRCLE/4)


/***************************************************************\
* Macros that look like subroutine calls                        *
\***************************************************************/

#define Convert32_F16(x) ((x)<<16)
/* convert 32 bit integer to 16.16 fraction */

#define ConvertF16_32(x) ((x)>>16)
/* convert 16.16 fraction to 32 bit integer */

#define ConvertF16_F30(x) ((x)<<14)
/* convert 16.16 fraction to 2.30 fraction */

#define ConvertF30_F16(x) ((x)>>14)
/* convert 2.30 fraction to 16.16 fraction */

#define ConvertF16_F14(x) (((x)&0xfffffffc)<<14)
/* convert 16.16 fraction to 2.14 fraction */

#define ConvertF14_F16(x) ((x)>>14)
/* convert 2.14 fraction to 16.16 fraction */

#define Convert32_F32(d,x) {(d)->hi=(x);(d)->lo=0;}
/* convert 32 bit integer to 32.32 fraction */

#define ConvertUF16_F32(d,x) {(d)->hi=((uint32)(x))>>16;(d)->lo=(x)<<16;}
#define ConvertSF16_F32(d,x) {(d)->hi=((int32)(x))>>16;(d)->lo=(x)<<16;}
/* convert 16.16 fraction to 32.32 fraction */

#define ConvertF32_F16(x) (((x)->hi<<16)+((x)->lo>>16))
/* convert 32.32 fraction to 16.16 fraction */

#define ConvertU32_64(d,x) {(d)->hi=0;(d)->lo=(x);}
#define ConvertS32_64(d,x) {(d)->hi=((int32)(x)>>31);(d)->lo=(x);}
/* convert 32 bit integer to 64 bit integer */

#define Neg32(x) (-(x))
#define NegF16(x) (-(x))
#define NegF30(x) (-(x))
#define NegF14(x) (-(x))
/* return the two's complement of a 32 bit integer or fraction */

#define Not32(x) (~(x))
#define NotF16(x) (~(x))
#define NotF30(x) (~(x))
#define NotF14(x) (~(x))
/* return the one's complement of a 32 bit integer or fraction */

#define Add32(x,y) ((x)+(y))
#define AddF16(x,y) ((x)+(y))
#define AddF30(x,y) ((x)+(y))
#define AddF14(x,y) ((x)+(y))
/* return the sum of 32 bit integers or fractions */

#define Sub32(x,y) ((x)-(y))
#define SubF16(x,y) ((x)-(y))
#define SubF30(x,y) ((x)-(y))
#define SubF14(x,y) ((x)-(y))
/* return the difference of 32 bit integers or fractions */

#define Mul32(x,y) ((x)*(y))
/* return the product of 32 bit integers */

#define MulF14(x,y) (((((x)>>16)*((y)>>16))<<2)&0xffff0000)
/* return the product of 2.14 fractions */

#define CosF32(c,x) SinF32(c,(x)+0x00400000)
/* Return the 32.32 cosine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */


/***************************************************************\
* Folio routine number equates                                  *
\***************************************************************/

#define MATHFOLIO	5
#define MATHSWI		(MATHFOLIO<<16)

#define MULMANYVEC3MAT33DIVZ_F16 (MATHSWI+18)
#define MULVEC3MAT33DIVZ_F16 (MATHSWI+17)
#define ABSVEC4_F16 (MATHSWI+16)
#define ABSVEC3_F16 (MATHSWI+15)
#define CROSS3_F16 (MATHSWI+14)
#define DOT4_F16 (MATHSWI+13)
#define DOT3_F16 (MATHSWI+12)
#define MULOBJECTMAT44_F16 (MATHSWI+11)
#define MULOBJECTVEC4MAT44_F16 (MATHSWI+10)
#define MULMANYVEC4MAT44_F16 (MATHSWI+9)
#define MULMAT44MAT44_F16 (MATHSWI+8)
#define MULVEC4MAT44_F16 (MATHSWI+7)
#define MULSCALARF16 (MATHSWI+6)
#define MULMANYF16 (MATHSWI+5)
#define MULOBJECTMAT33_F16 (MATHSWI+4)
#define MULOBJECTVEC3MAT33_F16 (MATHSWI+3)
#define MULMANYVEC3MAT33_F16 (MATHSWI+2)
#define MULMAT33MAT33_F16 (MATHSWI+1)
#define MULVEC3MAT33_F16 (MATHSWI+0)


/***************************************************************\
* Math routine prototypes					*
\***************************************************************/

/* structure to pass arguments to MulManyVec3Mat33DivZ_F16 routine */
typedef struct mmv3m33d {
  vec3f16 *dest;
  vec3f16 *src;
  mat33f16 *mat;
  frac16 n;
  uint32 count;
} mmv3m33d;

/* structure to pass arguments to MULOBJECTVECxMATxx_F16 routines */
typedef struct ObjOffset1 {
  int32 oo1_DestArrayPtrOffset;
  int32 oo1_SrcArrayPtrOffset;
  int32 oo1_MatOffset;
  int32 oo1_CountOffset;
} ObjOffset1;

/* structure to pass arguments to MULOBJECTMATxx_F16 routines */
typedef struct ObjOffset2 {
  int32 oo2_DestMatOffset;
  int32 oo2_SrcMatOffset;
} ObjOffset2;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err OpenMathFolio (void);
/* Open the math folio and set up the global variable _MathBase. */
/* A negative result is returned on error */


void Neg64 (int64 *dest, int64 *src);
void NegF32 (frac32 *dest, frac32 *src);
void NegF60 (frac60 *dest, frac60 *src);
/* return the two's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */

void Not64 (int64 *dest, int64 *src);
void NotF32 (frac32 *dest, frac32 *src);
void NotF60 (frac60 *dest, frac60 *src);
/* return the one's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
/* (all names call the same routine) */

ufrac16 SqrtF16 (ufrac16 x);
/* Return the positive square root of an unsigned 16.16 number */

uint32 Sqrt32 (uint32 x);
/* Return the positive square root of an unsigned 32 bit number */

ufrac16 DivUF16 (ufrac16 d1, ufrac16 d2);
ufrac16 DivRemUF16 (uint32 *rem, ufrac16 d1, ufrac16 d2);
/* Divide an unsigned 16.16 fraction into another an return an unsigned */
/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum return in both values. */
/* Note:  The remainder is in 0.32 format. */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are uint32 or ufrac30, as long as both inputs are the same type. */

frac16 DivSF16 (frac16 d1, frac16 d2);
frac16 DivRemSF16 (int32 *rem, frac16 d1, frac16 d2);
/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
/* result.  The remainder from DivSF16 will be returned in r1, which is */
/* unavailable to C code, so DivRemSF16 stores the remainder at the */
/* address pointed to by the first argument.  Overflow is signaled by */
/* maximum positive return in both values. */
/* Note: the remainder is in 0.32 format */
/* Note: the MSB of the remainder is a sign bit, and must be extended if */
/* remainder is to be used in subsequent calculations */
/* Note:  The divide will return a correct 16.16 result if the arguments */
/* are int32 or frac30, as long as both inputs are the same type. */

ufrac16 RecipUF16 (ufrac16 d);
/* Take the reciprocal of an unsigned 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */

frac16 RecipSF16 (frac16 d);
/* Take the reciprocal of a signed 16.16 number and return the 16.16 */
/* result.  The remainder will be returned in r1. */
/* Overflow is signaled by all bits set in the return values. */

frac16 MulSF16 (frac16 m1, frac16 m2);
/* Multiply two signed 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */

ufrac16 MulUF16 (ufrac16 m1, ufrac16 m2);
/* Multiply two unsigned 16.16 integers together, and get a 16.16 result. */
/* Overflows are not detected.  Lower bits are truncated. */

frac30 MulSF30 (frac30 m1, frac30 m2);
/* Multiply two 2.30 fractions together and get a 2.30 result. */
/* Overflows are not detected.  Lower bits are truncated. */

ufrac16 SquareSF16 (frac16 m);
/* Return the square of a signed 16.16 integer. */
/* Overflows are not detected.  Lower bits are truncated. */

ufrac16 SquareUF16 (ufrac16 m);
/* Return the square of an unsigned 16.16 integer. */
/* Overflows are not detected.  Lower bits are truncated. */

void MulS32_64 (int64 *prod, int32 m1, int32 m2);
void MulSF16_F32 (frac32 *prod, frac16 m1, frac16 m2);
void MulSF30_F60 (frac60 *prod, frac30 m1, frac30 m2);
/* Multiply two signed 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two signed 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two signed 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */

void MulU32_64 (uint64 *prod, uint32 m1, uint32 m2);
void MulUF16_F32 (ufrac32 *prod, ufrac16 m1, ufrac16 m2);
void MulUF30_F60 (ufrac60 *prod, ufrac30 m1, ufrac30 m2);
/* Multiply two unsigned 32 bit integers together, and get a 64 bit result. */
/* Alternately multiply two unsigned 16.16 numbers and get a 32.32 result. */
/* Alternately multiply two unsigned 2.30 numbers and get a 4.60 result. */
/* (all names call the same routine) */

void Add64 (int64 *r, int64 *a1, int64 *a2);
void AddF32 (frac32 *r, frac32 *a1, frac32 *a2);
void AddF60 (frac60 *r, frac60 *a1, frac60 *a2);
/* Add two 64 bit integers together and return the 64 bit result */
/* Alternately add two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */

void Sub64 (int64 *r, int64 *s1, int64 *s2);
void SubF32 (frac32 *r, frac32 *s1, frac32 *s2);
void SubF60 (frac60 *r, frac60 *s1, frac60 *s2);
/* Subtract two 64 bit integers and return the 64 bit result */
/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
/* (all names call the same routine) */

int32 CompareU64 (uint64 *s1, uint64 *s2);
int32 CompareUF32 (ufrac32 *s1, ufrac32 *s2);
int32 CompareUF60 (ufrac60 *s1, ufrac60 *s2);
/* Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2, */
/* and -1 if s1<s2. */
/* Alternately compare two unsigned 32.32 fractions or two 4.60 fractions */
/* (all names call the same routine) */

int32 CompareS64 (int64 *s1, int64 *s2);
int32 CompareSF32 (frac32 *s1, frac32 *s2);
int32 CompareSF60 (frac60 *s1, frac60 *s2);
/* Subtract two signed 64 bit integers and return the high word of the result */
/* (or 1 if the high word is zero, and the low word is non-zero). */
/* Alternately compare two 32.32 fractions or two 4.60 fractions */
/* The result of the comparison will be positive if s1>s2, zero if s1==s2, */
/* and negative if s1<s2. */
/* (all names call the same routine) */

uint64 *DivU64 (uint64 *q, uint64 *r, uint64 *d1, uint64 *d2);
/* Divide one unsigned 64 bit integer into another, and return the quotient */
/* and remainder.  On return, r0 will contain a pointer to the quotient. */

int64 *DivS64 (int64 *q, int64 *r, int64 *d1, int64 *d2);
/* Divide one signed 64 bit integer into another, and return the quotient */
/* and remainder.  On return, r0 will contain a pointer to the quotient. */

void Mul64 (int64 *p, int64 *m1, int64 *m2);
/* Multiply one 64 bit integer by another and return the result.  Overflow */
/* is not detected. */

void Square64 (uint64 *p, int64 *m);
/* Return the square of a 64 bit integer.  Overflow is not detected. */

uint32 Sqrt64_32 (uint64 *x);
ufrac16 SqrtF32_F16 (ufrac32 *x);
ufrac30 SqrtF60_F30 (ufrac60 *x);
/* Return the 32 bit square root of an unsigned 64 bit integer. */
/* Alternatively, return the square root of 32.32 fraction as a 16.16 fraction. */
/* Alternatively, return the square root of 4.60 fraction as a 2.30 fraction. */
/* (all names call the same routine) */

/* uint32 __rt_udiv (uint32 d1, uint32 d2); */
/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
/* The remainder is also returned in r1.  This routine takes arguments and returns */
/* values in the same way that the compiler expects the internally supplied */
/* library routines to.   Use DivRemU32() to get the remainder for C code. */

/* int32 __rt_sdiv (int32 d1, int32 d2); */
/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
/* The remainder is also returned in r1.  This routine takes arguments and returns */
/* values in the same way that the compiler expects the internally supplied */
/* library routines to.  Use DivRemS32() to get the remainder for C code. */

uint32 DivRemU32 (uint32 *rem, uint32 d1, uint32 d2);
/* Calculate the quotient and remainder when dividing unsigned 32 bit integers d1/d2 */
/* This routine calls __rt_udiv, but rearranges the arguments and return values */
/* to return the remainder to C code. */

int32 DivRemS32 (int32 *rem, int32 d1, int32 d2);
/* Calculate the quotient and remainder when dividing signed 32 bit integers d1/d2 */
/* This routine calls __rt_sdiv, but rearranges the arguments and return values */
/* to return the remainder to C code. */

frac16 SinF16 (frac16 x);
/* Return the 16.16 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

frac16 CosF16 (frac16 x);
/* Return the 16.16 cosine of an angle (assume 256.0 units of angle available) */
/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */

frac16 Atan2F16 (frac16 x, frac16 y);
/* Return the arctangent of the ratio y/x */
/* The result assume 256.0 units in the circle (or 16,777,216 units if used as an integer) */
/* Note:  The correct 16.16 result will be returned if the arguments are int32, frac30 or */
/* frac14, assuming the both arguments are the same type. */

frac30 SinF30 (frac16 x);
/* Return the 2.30 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

frac30 CosF30 (frac16 x);
/* Return the 2.30 cosine of an angle (assume 256.0 units of angle available) */
/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */

void SinF32 (frac32 *dest, frac16 x);
/* Return the 32.32 sine of an angle (assume 256.0 units of angle available) */
/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

void __swi(MULMANYVEC3MAT33DIVZ_F16) MulManyVec3Mat33DivZ_F16 (mmv3m33d *s);
/* Multiply a 3x3 matrix of 16.16 values by multiple vectors, then multiply x and y by n/z */
/* Return the result vectors {x*n/z, y*n/z, z} */

void __swi(MULVEC3MAT33DIVZ_F16)
     MulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n);
/* Multiply a 3x3 matrix of 16.16 values by a vector, then multiply x and y by n/z */
/* Return the result vector {x*n/z, y*n/z, z} */

void __swi(MULVEC3MAT33_F16) MulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat);
/* multiply a 3x3 matrix of 16.16 values by a vector of 16.16 values, return the result */

void __swi(MULVEC4MAT44_F16) MulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat);
/* multiply a 4x4 matrix of 16.16 values by a vector of 16.16 values, return the result */

void __swi(MULMAT33MAT33_F16) MulMat33Mat33_F16 (mat33f16 dest, mat33f16 src1, mat33f16 src2);
/* Multiply two 3x3 matrices of 16.16 values and return the result */

void __swi(MULMAT44MAT44_F16) MulMat44Mat44_F16 (mat44f16 dest, mat44f16 src1, mat44f16 src2);
/* Multiply two 4x4 matrices of 16.16 values and return the result */

frac16 __swi(DOT3_F16) Dot3_F16 (vec3f16 v1, vec3f16 v2);
/* Return the dot product of two vectors of 16.16 values */

frac16 __swi(DOT4_F16) Dot4_F16 (vec4f16 v1, vec4f16 v2);
/* Return the dot product of two vectors of 16.16 values */

void __swi(CROSS3_F16) Cross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2);
/* Return the cross product of two vectors of 16.16 values */

void Transpose33_F16 (mat33f16 dest, mat33f16 src);
/* Return the transpose of a 3x3 matrix of 16.16 values */

void Transpose44_F16 (mat44f16 dest, mat44f16 src);
/* Return the transpose of a 4x4 matrix of 16.16 values */

void __swi(MULMANYVEC3MAT33_F16) MulManyVec3Mat33_F16
       (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count);
/* Multiply many vectors by a matrix */

void __swi(MULMANYVEC4MAT44_F16) MulManyVec4Mat44_F16
       (vec4f16 *dest, vec4f16 *src, mat44f16 mat, int32 count);
/* Multiply many vectors by a matrix */

void __swi(MULOBJECTVEC3MAT33_F16) MulObjectVec3Mat33_F16
       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */

void __swi(MULOBJECTVEC4MAT44_F16) MulObjectVec4Mat44_F16
       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
/* Multiply many vectors within object structures by a matrix within that object */
/* structure, and repeat over a number of objects */

void __swi(MULOBJECTMAT33_F16) MulObjectMat33_F16
       (void *objectlist[], ObjOffset2 *offsetstruct, mat33f16 mat, int32 count);
/* Multiply a matrix within an object structure by an external matrix, and repeat */
/* over a number of objects */

void __swi(MULOBJECTMAT44_F16) MulObjectMat44_F16
       (void *objectlist[], ObjOffset2 *offsetstruct, mat44f16 mat, int32 count);
/* Multiply a matrix within an object structure by an external matrix, and repeat */
/* over a number of objects */

void __swi(MULMANYF16) MulManyF16 (frac16 *dest, frac16 *src1, frac16 *src2, int32 count);
/* Multiply an array of 16.16 fractions by another array of fractions */

void __swi(MULSCALARF16) MulScalarF16 (frac16 *dest, frac16 *src, frac16 scalar, int32 count);
/* Multiply a 16.16 scalar by an array of 16.16 fractions */

frac16 __swi(ABSVEC3_F16) AbsVec3_F16 (vec3f16 vec);
/* Return the absolute value (length) of a vector of 16.16 values */

frac16 __swi(ABSVEC4_F16) AbsVec4_F16 (vec4f16 vec);
/* Return the absolute value (length) of a vector of 16.16 values */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

