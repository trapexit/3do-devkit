#ifndef __FOLIO_H
#define __FOLIO_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: folio.h,v 1.52 1994/11/23 21:23:41 vertex Exp $
**
**  Kernel folio management definitions
**
******************************************************************************/


#include "types.h"
#include "nodes.h"
#include "item.h"
#include "task.h"


typedef struct Folio
{
	ItemNode fn;
	int32 f_OpenCount;
	uint8  f_TaskDataIndex;
	uint8 f_MaxSwiFunctions;
	uint8 f_MaxUserFunctions;
	uint8 f_MaxNodeType;


        int32    f_Private[17];

} Folio, *FolioP;

enum folio_tags
{
	CREATEFOLIO_TAG_NUSERVECS = TAG_ITEM_LAST+1,	/* # of uservecs */
	CREATEFOLIO_TAG_USERFUNCS	/* ptr to uservec table */
};

/*
    A folio's function table is an array of function pointers that extends in
    the negative direction.  FolioFunc is a pointer to a folio function.
    GetFolioFunc() gets the function pointer for a particular folio function.
*/
typedef int32 (*FolioFunc)();
#define GetFolioFunc(folio,func) ((const FolioFunc *)(folio))[func]

/* To interpret this mismash of * and () */
/* _f is a pointer to table of function pointers */
/* This table extends in the negative direction */
/* These _could_ be updated to use GetFolioFunc() */
#define CALLFOLIO(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CALLFOLIORET(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}
#define CallFolio(folio,func,args) \
	{ \
		void (* *_f)() = (void (* *)())folio; \
		(*_f[func])args; \
	}
#define CallFolioRet(folio,func,args,ret,cast) \
	{ \
		int32 (* *_f)() = (int32 (* *)())folio; \
		ret = cast (*_f[func])args; \
	}

/* Kernel vectors */
#define KBV_REMHEAD		-1
#define KBV_ADDHEAD		-2
#define KBV_REMTAIL		-3
#define KBV_ADDTAIL		-4
#define KBV_INSERTTAIL	-5
#define KBV_REMOVENODE	-6
#define KBV_ALLOCMEM	-7
#define KBV_FREEMEM		-8
#define KBV_INITLIST	-9
#define KBV_FINDNAMEDNODE	-10
#define KBV_SCAVENGEMEM	-11
#define KBV_LOCATEITEM	-12
#define KBV_LOOKUPITEM	-12
#define KBV_MEMSET		-13
#define KBV_MEMCPY		-14
#define KBV_GETPAGESIZE	-15
#define KBV_CHECKITEM	-16
#define KBV_FINSERT		-17
#define KBV_USEC2TICKS	-18
#define KBV_SETNODEPRI	-19
#define KBV_INSERTHEAD	-20
#define KBV_VFPRINTF	-21
#define KBV_GETSYSERR	-22
#define KBV_TICKS2TIMEVAL	-23
#define KBV_FINDMH	-25
#define KBV_ALLOCMEMML	-26
#define KBV_FREEMEMML	-27
#define KBV_ALLOCMEMLIST	-28
#define KBV_FREEMEMLIST	-29
#define KBV_ITEMOPENED	-32
#define KBV_FINDIMAGE	-38
#define KBV_GETMEMALLOCALIGNMENT -39
#define KBV_GETMEMTRACKSIZE -40
#define KBV_CHECKIO       -41
#define KBV_ISMEMREADABLE -42
#define KBV_ISMEMWRITABLE -43
#define KBV_FINDTAGARG    -44
#define KBV_NEXTTAGARG    -45

#define FindFolio(n)        FindNamedItem(MKNODEID(KERNELNODE,FOLIONODE),(n))
#define FindAndOpenFolio(n) FindAndOpenNamedItem(MKNODEID(KERNELNODE,FOLIONODE),(n))


/*****************************************************************************/


#endif	/* __FOLIO_H */
