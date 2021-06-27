#ifndef __TYPES_H
#define __TYPES_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: types.h,v 1.38 1994/11/01 02:32:33 vertex Exp $
**
**  Standard Portfolio types and constants
**
******************************************************************************/


#ifdef __cplusplus
//	*****************************************************************
//	*	C++ is currently incapable of handling volatile.	*
//	*	Remove the following line as soon as this bug is fixed.	*
//	*****************************************************************
#define	volatile
#endif /* __cplusplus */

#pragma force_top_level
#pragma include_only_once

/* On the arm, char is unsigned */

/* compatibility typedefs */
typedef signed char	int8;
typedef signed short	int16;		/* Avoid this type - unreliable */
typedef signed long	int32;

typedef char	uchar;			/* should be unsigned char */
typedef uchar	ubyte;
typedef uchar	uint8;

typedef unsigned short	ushort;		/* Avoid this type - unreliable */
typedef ushort		uint16;		/* Avoid this type - unreliable */

typedef unsigned long	ulong;
typedef ulong		uint32;

typedef volatile long	vlong;
typedef	vlong		vint32;

typedef volatile unsigned long	vulong;
typedef	vulong			vuint32;

typedef	uint8	Boolean;
typedef Boolean	bool;

typedef uint16	unichar;	/* a UniCode character */

typedef	uint32	size_t;

#define NULL	((void *)0)
#define null	NULL
#define NIL	NULL
#define nil	NULL

#define TRUE	((Boolean)1)
#define FALSE	((Boolean)0)

#define false	FALSE
#define true	TRUE

typedef int32	(*func_t)();	/* pointer to function returning integer */
typedef uint32	(*uifunc_t)();	/* pointer to function returning uint32 */
typedef void	(*vfunc_t)();	/* pointer to function returning void */
typedef void	*(*vpfunc_t)();	/* pointer to function returning pointer */

typedef int32	Item;
typedef	int32	Err;

/*  Definitions for code loading and management */

typedef void *CodeHandle;

/* Offset of a field in a structure */
#define Offset(struct_type,field)	((uint32)&(((struct_type)NULL)->field))
#define offsetof(type,memb)		((size_t)&(((type *)NULL)->memb))

/* TagArgs - use to pass a list of arguments to functions */
typedef	void *TagData;

typedef struct TagArg
{
	uint32	ta_Tag;
	TagData	ta_Arg;
} TagArg, *TagArgP;

#define TAG_END		0
#define TAG_JUMP	254
#define TAG_NOP		255

/* non portable inline __swi stuff */
#ifndef __CC_NORCROFT
#define __swi(x)		/* nothing */
#endif

/* Bit manipulation macros */
#define NBBY	8	/* bits per byte */

/* page descriptor bits */
/*#define PD_SETSIZE	128*/	/* 128 bits */
typedef uint32		pd_mask;
#define NPDBITS	(sizeof(pd_mask) * NBBY)	/* bits per mask  = 32 */

#define howmany(x,y)	(((x)+((y)-1))/(y))

typedef struct pd_set
{
	pd_mask	pds_bits[1];	/* actually variable size */
} pd_set;

#define PD_Set(n, p)	((p)->pds_bits[(n)/NPDBITS] |= ((uint32)1 << ((n) % NPDBITS)))
#define PD_Clr(n, p)	((p)->pds_bits[(n)/NPDBITS] &= ~((uint32)1 << ((n) % NPDBITS)))
#define PD_IsSet(n, p)	((p)->pds_bits[(n)/NPDBITS] & ((uint32)1 << ((n) % NPDBITS)))
#define PD_Zero(p)	bzero((int8 *)(p), sizeof (*(p)))

#define Make_Func(x,y) (x (*)())make_func((int32)(y))
#define Make_Ptr(x,y) (x *)make_int((func_t)(y))
#define Make_Int(x,y) (x)make_int((func_t)(y))

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

func_t	make_func(int32);
int32	make_int(func_t);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#define KERNELSWI	0x10000


/*****************************************************************************/


#endif /* __TYPES_H */
