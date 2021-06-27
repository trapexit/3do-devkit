	IF :DEF:__operamath_i
	ELSE
		GBLL	__operamath_i

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: operamath.i,v 1.2 1994/09/10 02:39:35 vertex Exp $
;*
;*****************************************************************************

		INCLUDE macros.i
		INCLUDE structs.i


;**************************************************************
;* Basic typedefs and structures			*
;**************************************************************

		TYPEDEF	INT32,frac16
		TYPEDEF	INT32,frac30
		TYPEDEF	INT32,frac14
		TYPEDEF	UINT32,ufrac16
		TYPEDEF	UINT32,ufrac30
		TYPEDEF	UINT32,ufrac14

		BEGINSTRUCT	int64
			STRUCT	INT32,HI
			STRUCT	INT32,LO
		ENDSTRUCT

		TYPEDEF	int64,uint64
		TYPEDEF int64,frac32
		TYPEDEF	int64,ufrac32
		TYPEDEF	int64,frac60
		TYPEDEF	int64,ufrac60

		BEGINSTRUCT	vec3f16
			ARRAY	frac16,,3
		ENDSTRUCT

		BEGINSTRUCT	vec4f16
			ARRAY	frac16,,4
		ENDSTRUCT

		BEGINSTRUCT	mat33f16
			ARRAY	vec3f16,,3
		ENDSTRUCT

		BEGINSTRUCT	mat44f16
			ARRAY	vec4f16,,4
		ENDSTRUCT

		BEGINSTRUCT	mat34f16
			ARRAY	vec3f16,,4
		ENDSTRUCT


;**************************************************************
;* Macros that look like subroutine calls			*
;**************************************************************

; These macros are available to C code, and NOT to assembly routines
;#define Convert32_F16(x) ((x)<<16)
;/* convert 32 bit integer to 16.16 fraction */

;#define ConvertF16_32(x) ((x)>>16)
;/* convert 16.16 fraction to 32 bit integer */

;#define Convert32_F32(d,x) {(d)->hi=(x);(d)->lo=0;}
;/* convert 32 bit integer to 32.32 fraction */

;#define ConvertUF16_F32(d,x) {(d)->hi=((uint32)(x))>>16;(d)->lo=(x)<<16;}
;#define ConvertSF16_F32(d,x) {(d)->hi=((int32)(x))>>16;(d)->lo=(x)<<16;}
;/* convert 16.16 fraction to 32.32 fraction */

;#define ConvertF32_F16(x) (((x)->hi<<16)+((x)->lo>>16))
;/* convert 32.32 fraction to 16.16 fraction */

;#define ConvertU32_64(d,x) {(d)->hi=0;(d)->lo=(x);}
;#define ConvertS32_64(d,x) {(d)->hi=((int32)(x)>>31);(d)->lo=(x);}
;/* convert 32 bit integer to 64 bit integer */

;#define Neg32(x) (-(x))
;#define NegF16(x) (-(x))
;/* return the two's complement of a 32 bit integer or a 16.16 fraction */

;#define Not32(x) (~(x))
;#define NotF16(x) (~(x))
;/* return the one's complement of a 32 bit integer or a 16.16 fraction */

;#define Add32(x,y) ((x)+(y))
;#define AddF16(x,y) ((x)+(y))
;/* return the sum of 32 bit integers or 16.16 fractions */

;#define Sub32(x,y) ((x)-(y))
;#define SubF16(x,y) ((x)-(y))
;/* return the difference of 32 bit integers or 16.16 fractions */

;#define Mul32(x,y) ((x)*(y))
;/* return the product of 32 bit integers */

;#define CosF16(x) (SinF16((x)+0x400000))
;/* return the 16.16 cosine of a 16.16 fraction (assuming 256.0 units in the circle) */

;#define CosF32(d,x) (SinF32((d),(x)+0x400000))
;/* return the 32.32 cosine of a 16.16 fraction (assuming 256.0 units in the circle) */


;***************************************************************
;* Folio routine number equates                                  *
;***************************************************************

MATHFOLIO		EQU	5
MATHSWI			EQU	(MATHFOLIO<<16)

MULMANYVEC3MAT33DIVZ_F16 EQU	(MATHSWI+18)
MULVEC3MAT33DIVZ_F16	EQU	(MATHSWI+17)
ABSVEC4_F16		EQU	(MATHSWI+16)
ABSVEC3_F16		EQU	(MATHSWI+15)
CROSS3_F16		EQU	(MATHSWI+14)
DOT4_F16		EQU	(MATHSWI+13)
DOT3_F16		EQU	(MATHSWI+12)
MULOBJECTMAT44_F16	EQU	(MATHSWI+11)
MULOBJECTVEC4MAT44_F16	EQU	(MATHSWI+10)
MULMANYVEC4MAT44_F16	EQU	(MATHSWI+9)
MULMAT44MAT44_F16	EQU	(MATHSWI+8)
MULVEC4MAT44_F16	EQU	(MATHSWI+7)
MULSCALARF16		EQU	(MATHSWI+6)
MULMANYF16		EQU	(MATHSWI+5)
MULOBJECTMAT33_F16	EQU	(MATHSWI+4)
MULOBJECTVEC3MAT33_F16	EQU	(MATHSWI+3)
MULMANYVEC3MAT33_F16	EQU	(MATHSWI+2)
MULMAT33MAT33_F16	EQU	(MATHSWI+1)
MULVEC3MAT33_F16	EQU	(MATHSWI+0)


;**************************************************************
;* math routine headers						*
;**************************************************************


;void Neg64 (int64 *dest, int64 *src);
;void NegF32 (frac32 *dest, frac32 *src);
;void NegF60 (frac60 *dest, frac60 *src);
;/* return the two's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
;/* (all names call the same routine) */

;void Not64 (int64 *dest, int64 *src);
;void NotF32 (frac32 *dest, frac32 *src);
;void NotF60 (frac60 *dest, frac60 *src);
;/* return the one's complement of a 64 bit integer (or 32.32 fraction or 4.60 fraction) */
;/* (all names call the same routine) */

;ufrac16 SqrtF16 (ufrac16 x);
;/* Return the positive square root of an unsigned 16.16 number */

;uint32 Sqrt32 (uint32 x);
;/* Return the positive square root of an unsigned 32 bit number */

;ufrac16 DivUF16 (ufrac16 d1, ufrac16 d2);
;ufrac16 DivRemUF16 (ufrac16 *rem, ufrac16 d1, ufrac16 d2);
;/* Divide an unsigned 16.16 fraction into another an return an unsigned */
;/* 16.16 result.  The remainder from DivUF16 will be returned in r1, which */
;/* is unavailable to C code, so DivRemUF16 stores the remainder at the */
;/* address pointed to by the first argument.  Overflow is signaled by */
;/* maximum return in both values. */
;/* Note:  The remainder is NOT in 16.16 format, but is in 0.32 format. */
;/* Note:  The divide will return a correct 16.16 result if the arguments */
;/* are uint32 or ufrac30, as long as both inputs are the same type. */

;frac16 DivSF16 (frac16 d1, frac16 d2);
;frac16 DivRemSF16 (frac16 *rem, frac16 d1, frac16 d2);
;/* Divide a signed 16.16 fraction into another an return a signed 16.16 */
;/* result.  The remainder from DivSF16 will be returned in r1, which is */
;/* unavailable to C code, so DivRemSF16 stores the remainder at the */
;/* address pointed to by the first argument.  Overflow is signaled by */
;/* maximum positive return in both values. */
;/* Note: the remainder is NOT in 16.16 format, but is in 0.32 format */
;/* Note: the MSB of the remainder is a sign bit, and must be extended if */
;/* remainder is to be used in subsequent calculations */
;/* Note:  The divide will return a correct 16.16 result if the arguments */
;/* are int32 or frac30, as long as both inputs are the same type. */

;ufrac16 RecipUF16 (ufrac16 d);
;/* Take the reciprocal of an unsigned 16.16 number and return the 16.16 */
;/* result.  The remainder will be returned in r1. */
;/* Overflow is signaled by all bits set in the return values. */

;frac16 RecipSF16 (frac16 d);
;/* Take the reciprocal of a signed 16.16 number and return the 16.16 */
;/* result.  The remainder will be returned in r1. */
;/* Overflow is signaled by all bits set in the return values. */

;frac16 MulSF16 (frac16 m1, frac16 m2);
;/* Multiply two signed 16.16 integers together, and get a 16.16 result. */
;/* Overflows are not detected.  Lower bits are truncated. */

;ufrac16 MulUF16 (ufrac16 m1, ufrac16 m2);
;/* Multiply two unsigned 16.16 integers together, and get a 16.16 result. */
;/* Overflows are not detected.  Lower bits are truncated. */

;frac30 MulSF30 (frac30 m1, frac30 m2);
;/* Multiply two 2.30 fractions together and get a 2.30 result. */
;/* Overflows are not detected.  Lower bits are truncated. */

;ufrac16 SquareSF16 (frac16 m);
;/* Return the square of a signed 16.16 integer. */
;/* Overflows are not detected.  Lower bits are truncated. */

;ufrac16 SquareUF16 (ufrac16 m);
;/* Return the square of an unsigned 16.16 integer. */
;/* Overflows are not detected.  Lower bits are truncated. */

;void MulS32_64 (int64 *prod, int32 m1, int32 m2);
;void MulSF16_F32 (frac32 *prod, frac16 m1, frac16 m2);
;void MulSF30_F60 (frac60 *prod, frac30 m1, frac30 m2);
;/* Multiply two signed 32 bit integers together, and get a 64 bit result. */
;/* Alternately multiply two signed 16.16 numbers and get a 32.32 result. */
;/* Alternately multiply two signed 2.30 numbers and get a 4.60 result. */
;/* (all names call the same routine) */

;void MulU32_64 (uint64 *prod, uint32 m1, uint32 m2);
;void MulUF16_F32 (ufrac32 *prod, ufrac16 m1, ufrac16 m2);
;void MulUF30_F60 (ufrac60 *prod, ufrac30 m1, ufrac30 m2);
;/* Multiply two unsigned 32 bit integers together, and get a 64 bit result. */
;/* Alternately multiply two unsigned 16.16 numbers and get a 32.32 result. */
;/* Alternately multiply two unsigned 2.30 numbers and get a 4.60 result. */
;/* (all names call the same routine) */

;void Add64 (int64 *r, int64 *a1, int64 *a2);
;void AddF32 (frac32 *r, frac32 *a1, frac32 *a2);
;void AddF60 (frac60 *r, frac60 *a1, frac60 *a2);
;/* Add two 64 bit integers together and return the 64 bit result */
;/* Alternately add two 32.32 fractions or two 4.60 fractions*/
;/* (all names call the same routine) */

;void Sub64 (int64 *r, int64 *s1, int64 *s2);
;void SubF32 (frac32 *r, frac32 *s1, frac32 *s2);
;void SubF60 (frac60 *r, frac60 *s1, frac60 *s2);
;/* Subtract two 64 bit integers and return the 64 bit result */
;/* Alternately subtract two 32.32 fractions or two 4.60 fractions*/
;/* (all names call the same routine) */

;long CompareU64 (uint64 *s1, uint64 *s2);
;long CompareUF32 (ufrac32 *s1, ufrac32 *s2);
;long CompareUF60 (ufrac60 *s1, ufrac60 *s2);
;/* Subtract two unsigned 64 bit integers and return 1 if s1>s2, 0 if s1==s2, */
;/* and -1 if s1<s2. */
;/* Alternately compare two unsigned 32.32 fractions or two 4.60 fractions */
;/* (all names call the same routine) */

;long CompareS64 (int64 *s1, int64 *s2);
;long CompareSF32 (frac32 *s1, frac32 *s2);
;long CompareSF60 (frac60 *s1, frac60 *s2);
;/* Subtract two signed 64 bit integers and return the high word of the result */
;/* (or 1 if the high word is zero, and the low word is non-zero). */
;/* Alternately compare two 32.32 fractions or two 4.60 fractions */
;/* The result of the comparison will be positive if s1>s2, zero if s1==s2, */
;/* and negative if s1<s2. */
;/* (all names call the same routine) */

;int64 *DivU64 (int64 *q, int64 *r, int64 *d1, int64 *d2);
;/* Divide one unsigned 64 bit integer into another, and return the quotient */
;/* and remainder.  On return, r0 will contain a pointer to the quotient. */

;int64 *DivS64 (int64 *q, int64 *r, int64 *d1, int64 *d2);
;/* Divide one signed 64 bit integer into another, and return the quotient */
;/* and remainder.  On return, r0 will contain a pointer to the quotient. */

;void Mul64 (int64 *p, int64 *m1, int64 *m2);
;/* Multiply one 64 bit integer by another and return the result.  Overflow */
;/* is not detected. */

;void Square64 (uint64 *p, int64 *m);
;/* Return the square of a 64 bit integer.  Overflow is not detected. */

;uint32 Sqrt64_32 (uint64 *x);
;ufrac16 SqrtF32_F16 (ufrac32 *x);
;ufrac16 SqrtF60_F30 (ufrac32 *x);
;/* Return the 32 bit square root of an unsigned 64 bit integer. */
;/* Alternatively, return the square root of 32.32 fraction as a 16.16 fraction. */
;/* Alternatively, return the square root of 4.60 fraction as a 2.30 fraction. */
;/* (all names call the same routine) */

;/* uint32 __rt_udiv (uint32 d1, uint32 d2); */
;/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
;/* The remainder is also returned in r1.  This routine takes arguments and returns */
;/* values in the same way that the compiler expects the internally supplied */
;/* library routines to.   Use DivRemU32() to get the remainder for C code. */

;/* int32 __rt_sdiv (int32 d1, int32 d2); */
;/* Divide a 32 bit integer d2 by 32 bit integer d1 and return a 32 bit quotient. */
;/* The remainder is also returned in r1.  This routine takes arguments and returns */
;/* values in the same way that the compiler expects the internally supplied */
;/* library routines to.  Use DivRemS32() to get the remainder for C code. */

;uint32 DivRemU32 (uint32 *rem, uint32 d1, uint32 d2);
;/* Calculate the quotient and remainder when dividing unsigned 32 bit integers d1/d2 */
;/* This routine calls __rt_udiv, but rearranges the arguments and return values */
;/* to return the remainder to C code. */

;int32 DivRemS32 (int32 *rem, int32 d1, int32 d2);
;/* Calculate the quotient and remainder when dividing signed 32 bit integers d1/d2 */
;/* This routine calls __rt_sdiv, but rearranges the arguments and return values */
;/* to return the remainder to C code. */

;frac16 SinF16 (frac16 x);
;/* Return the 16.16 sine of an angle (assume 256.0 units of angle available) */
;/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

;frac16 CosF16 (frac16 x);
;/* Return the 16.16 cosine of an angle (assume 256.0 units of angle available) */
;/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */

;frac16 Atan2F16 (frac16 x, frac16 y);
;/* Return the arctangent of the ratio y/x */
;/* The result assume 256.0 units in the circle (or 16,777,216 units if used as an integer) */
;/* Note:  The correct 16.16 result will be returned if the arguments are int32, frac30 or */
;/* frac14, assuming the both arguments are the same type. */

;frac30 SinF30 (frac16 x);
;/* Return the 2.30 sine of an angle (assume 256.0 units of angle available) */
;/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

;frac30 CosF30 (frac16 x);
;/* Return the 2.30 cosine of an angle (assume 256.0 units of angle available) */
;/* Rptionally, the angle can be an integer dividing the circle into 16,777,216 units */

;void SinF32 (frac32 *dest, frac16 x);
;/* Return the 32.32 sine of an angle (assume 256.0 units of angle available) */
;/* Optionally, the angle can be an integer dividing the circle into 16,777,216 units */

;/* structure to pass arguments to MulManyVec3Mat33DivZ_F16 routine */
;typedef struct mmv3m33d {
;  vec3f16 *dest;
;  vec3f16 *src;
;  mat33f16 *mat;
;  frac16 n;
;  uint32 count;
;} mmv3m33d;
	BEGINSTRUCT	mmv3m33d
		PTR	mmv3m33d_dest
		PTR	mmv3m33d_src
		PTR	mmv3m33d_mat
		STRUCT	frac16,mmv3m33d_n
		STRUCT	UINT32,mmv3m33d_count
	ENDSTRUCT

;void __swi(MULMANYVEC3MAT33DIVZ_F16) MulManyVec3Mat33DivZ_F16 (mmv3m33d *s);
;/* Multiply a 3x3 matrix of 16.16 values by multiple vectors, then multiply x and y by n/z */
;/* Return the result vectors {x*n/z, y*n/z, z} */
		SETSWI MulManyVec3Mat33DivZ_F16,MULMANYVEC3MAT33DIVZ_F16

;void __swi(MULVEC3MAT33DIVZ_F16)
;     MulVec3Mat33DivZ_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat, frac16 n);
;/* Multiply a 3x3 matrix of 16.16 values by a vector, then multiply x and y by n/z */
;/* Return the result vector {x*n/z, y*n/z, z} */
		SETSWI MulVec3Mat33DivZ_F16,MULVEC3MAT33DIVZ_F16

;void __swi(MULVEC3MAT33_F16) MulVec3Mat33_F16 (vec3f16 dest, vec3f16 vec, mat33f16 mat);
;/* multiply a 3x3 matrix of 16.16 values by a vector of 16.16 values, return the result */
		SETSWI MulVec3Mat33_F16,MULVEC3MAT33_F16

;void __swi(MULVEC4MAT44_F16) MulVec4Mat44_F16 (vec4f16 dest, vec4f16 vec, mat44f16 mat);
;/* multiply a 4x4 matrix of 16.16 values by a vector of 16.16 values, return the result */
		SETSWI MulVec4Mat44_F16,MULVEC4MAT44_F16

;void __swi(MULMAT33MAT33_F16) MulMat33Mat33_F16 (mat33f16 dest, mat33f16 src1, mat33f16 src2);
;/* Multiply two 3x3 matrices of 16.16 values and return the result */
		SETSWI MulMat33Mat33_F16,MULMAT33MAT33_F16

;void __swi(MULMAT44MAT44_F16) MulMat44Mat44_F16 (mat44f16 dest, mat44f16 src1, mat44f16 src2);
;/* Multiply two 4x4 matrices of 16.16 values and return the result */
		SETSWI MulMat44Mat44_F16,MULMAT44MAT44_F16

;frac16 __swi(DOT3_F16) Dot3_F16 (vec3f16 v1, vec3f16 v2);
;/* Return the dot product of two vectors of 16.16 values */
		SETSWI Dot3_F16,DOT3_F16

;frac16 __swi(DOT4_F16) Dot4_F16 (vec4f16 v1, vec4f16 v2);
;/* Return the dot product of two vectors of 16.16 values */
		SETSWI Dot4_F16,DOT4_F16

;void __swi(CROSS3_F16) Cross3_F16 (vec3f16 dest, vec3f16 v1, vec3f16 v2);
;/* Return the cross product of two vectors of 16.16 values */
		SETSWI Cross3_F16,CROSS3_F16

;void Transpose33_F16 (mat33f16 dest, mat33f16 src);
;/* Return the transpose of a 3x3 matrix of 16.16 values */

;void Transpose44_F16 (mat44f16 dest, mat44f16 src);
;/* Return the transpose of a 4x4 matrix of 16.16 values */

;void __swi(MULMANYVEC3MAT33_F16) MulManyVec3Mat33_F16
;       (vec3f16 *dest, vec3f16 *src, mat33f16 mat, int32 count);
;/* Multiply many vectors by a matrix */
		SETSWI MulManyVec3Mat33_F16,MULMANYVEC3MAT33_F16

;void __swi(MULMANYVEC4MAT44_F16) MulManyVec4Mat44_F16
;       (vec4f16 *dest, vec4f16 *src, mat44f16 mat, int32 count);
;/* Multiply many vectors by a matrix */
		SETSWI MulManyVec4Mat44_F16,MULMANYVEC4MAT44_F16

;typedef struct ObjOffset1 {
;  int32 oo1_DestArrayPtrOffset;
;  int32 oo1_SrcArrayPtrOffset;
;  int32 oo1_MatOffset;
;  int32 oo1_CountOffset;
;} ObjOffset1;
	BEGINSTRUCT	ObjOffset1
		STRUCT	INT32,oo1_DestArrayPtrOffset
		STRUCT	INT32,oo1_SrcArrayPtrOffset
		STRUCT	INT32,oo1_MatOffset
		STRUCT	INT32,oo1_CountOffset
	ENDSTRUCT

;void __swi(MULOBJECTVEC3MAT33_F16) MulObjectVec3Mat33_F16
;       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
;/* Multiply many vectors within object structures by a matrix within that object */
;/* structure, and repeat over a number of objects */
		SETSWI MulObjectVec3Mat33_F16,MULOBJECTVEC3MAT33_F16

;void __swi(MULOBJECTVEC4MAT44_F16) MulObjectVec4Mat44_F16
;       (void *objectlist[], ObjOffset1 *offsetstruct, int32 count);
;/* Multiply many vectors within object structures by a matrix within that object */
;/* structure, and repeat over a number of objects */
		SETSWI MulObjectVec4Mat44_F16,MULOBJECTVEC4MAT44_F16

;typedef struct ObjOffset2 {
;  int32 oo2_DestMatOffset;
;  int32 oo2_SrcMatOffset;
;} ObjOffset2;
	BEGINSTRUCT	ObjOffset2
		STRUCT	INT32,oo2_DestMatOffset
		STRUCT	INT32,oo2_SrcMatOffset
	ENDSTRUCT

;void __swi(MULOBJECTMAT33_F16) MulObjectMat33_F16
;       (void *objectlist[], ObjOffset2 *offsetstruct, mat33f16 mat, int32 count);
;/* Multiply a matrix within an object structure by an external matrix, and repeat */
;/* over a number of objects */
		SETSWI MulObjectMat33_F16,MULOBJECTMAT33_F16

;void __swi(MULOBJECTMAT44_F16) MulObjectMat44_F16
;       (void *objectlist[], ObjOffset2 *offsetstruct, mat44f16 mat, int32 count);
;/* Multiply a matrix within an object structure by an external matrix, and repeat */
;/* over a number of objects */
		SETSWI MulObjectMat44_F16,MULOBJECTMAT44_F16

;void __swi(MULMANYF16) MulManyF16 (frac16 *dest, frac16 *src1, frac16 *src2, int32 count);
;/* Multiply an array of 16.16 fractions by another array of fractions */
		SETSWI MulManyF16,MULMANYF16

;void __swi(MULSCALARF16) MulScalarF16 (frac16 *dest, frac16 *src, frac16 scalar, int32 count);
;/* Multiply a 16.16 scalar by an array of 16.16 fractions */
		SETSWI MulScalarF16,MULSCALARF16

;frac16 __swi(ABSVEC3_F16) AbsVec3_F16 (vec3f16 vec);
;/* Return the absolute value (length) of a vector of 16.16 values */
		SETSWI AbsVec3_F16,ABSVEC3_F16

;frac16 __swi(ABSVEC4_F16) AbsVec4_F16 (vec3f16 vec);
;/* Return the absolute value (length) of a vector of 16.16 values */
		SETSWI AbsVec4_F16,ABSVEC4_F16


;#endif



	ENDIF

		END
