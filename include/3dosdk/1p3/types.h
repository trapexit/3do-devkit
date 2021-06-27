#ifndef __TYPES_H
#define __TYPES_H

/*****

$Id: types.h,v 1.33 1994/04/08 22:25:42 sdas Exp $

$Log: types.h,v $
 * Revision 1.33  1994/04/08  22:25:42  sdas
 * offsetof() is now being defined the ANSI way
 *
 * Revision 1.32  1994/04/07  20:58:05  sdas
 * offsetof() definition added
 *
 * Revision 1.31  1994/04/07  18:32:40  sdas
 * Defined uifunc_t type
 *
 * Revision 1.30  1994/03/26  01:26:14  dplatt
 * Add CodeHandle typedef
 *
 * Revision 1.29  1994/03/23  23:06:54  sdas
 * vlong and vulong types defined; previously defined in sherrie.h
 *
 * Revision 1.28  1994/03/11  23:40:15  sdas
 * removed ITAG_ENTRYSIZE
 *
 * Revision 1.27  1994/03/11  18:50:23  sdas
 * Added unichar type.
 * Also added TAG_TO_FOLIO(), TAG_TO_INDEX() and ITAG_ENTRYSIZE
 *
 * Revision 1.26  1994/03/04  21:46:30  sdas
 * Clean-ups
 *
 * Revision 1.25  1994/03/03  01:41:41  sdas
 * Offset() macro definition added
 *
 * Revision 1.24  1994/01/21  02:32:17  limes
 * recover from rcs bobble
 *
 * Revision 1.25  1994/01/20  02:16:03  sdas
 * C++ compatibility - updated
 *
 * Revision 1.24  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.23  1993/10/26  01:18:06  limes
 * extend MakeFunc concept a little; we need compiler-silent ways
 * to change function pointers to generic pointers and ints.
 *
 * Revision 1.22  1993/08/09  18:37:34  andy
 * added TAG_JUMP
 *
 * Revision 1.21  1993/06/11  05:45:54  dale
 * make __swi better for non-norcroft system
 *
 * Revision 1.20  1993/03/16  06:44:09  dale
 * api
 *
 * Revision 1.19  1993/02/11  03:27:37  dale
 * new macro names
 *
 * Revision 1.18  1992/12/16  04:56:56  dale
 * changes for modified memory allocator
 *
 * Revision 1.17  1992/11/22  21:28:44  dale
 * fix warnings
 *
 * Revision 1.16  1992/11/18  00:06:12  dale
 * added TAG_NOP
 *
 * Revision 1.15  1992/10/30  01:29:02  dale
 * fixes to TagArg stuff
 *
 * Revision 1.14  1992/10/24  01:02:10  dale
 * rcs
 *
 * Revision 1.13  1992/10/24  00:54:32  dale
 * rcs
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

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
typedef struct TagArg
{
	uint32 ta_Tag;
	void *ta_Arg;
} TagArg, *TagArgP;

#define TAG_END		0
#define TAG_JUMP	254
#define TAG_NOP		255

#define TAG_TO_FOLIO(tag)	((uint32)(tag) >> 16)
#define TAG_TO_INDEX(tag)	((uint32)(tag) & 0xffff)

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

#endif /* __TYPES_H */
