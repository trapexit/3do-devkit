#pragma force_top_level
#pragma once

/*
** WARNING!  Some mac folks (Bill Duvall, David Maynard, etc.) have
** locally cached versions of this file!
**
** WARNING2! Do not use ANSI-style comments (preceded by //)
** on the same line as a #define - cpp does not strip these!
** BAD:  #define MY_CONST 32	//bad comment that is now part of MY_CONST
*/

/*****
 * $Id: aif.h,v 1.21 1994/10/06 23:13:49 limes Exp $
 *
 * $Log: aif.h,v $
 * Revision 1.21  1994/10/06  23:13:49  limes
 * Add extern decls for __my_AIFHeader and __my_3DOBinHeader
 * so we don't have to spread them all over the rest of the source.
 *
 * Revision 1.20  1994/09/08  20:52:37  sdas
 * _3DO_Time field added to the 3dobinheader
 *
 * Revision 1.19  1994/05/12  02:10:09  sdas
 * FindImage() and nextImage() prototypes defined
 *
 * Revision 1.18  1994/02/02  21:51:22  deborah
 * fixed stupid early comment termination in last version
 *
 * Revision 1.17  1994/02/02  21:44:17  deborah
 * Modified #defines with ansi-style comments to k&r-style comments
 * otherwise #define value includes the comment string!
 *
 * Revision 1.16  1994/02/02  21:29:32  deborah
 * Added PORTFOLIO_DEBUG_CLEAR, PORTFOLIO_DEBUG_TASK and PORTFOLIO_DEBUG_UNSET
 * values used by modbin. This allows modbin to use this system version of
 * aif.h rather than having a local (and different) copy.
 *
 * Revision 1.15  1994/01/28  23:20:10  sdas
 * Include Files Self-consistency - includes dependant include files
 *
 * Revision 1.14  1994/01/21  02:01:20  limes
 * Recover from RCS bobble:
 *
 * +  * Revision 1.15  1994/01/20  01:49:27  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.14  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 * +  *
 *
 * Revision 1.15  1994/01/20  01:49:27  sdas
 * C++ compatibility - updated
 *
 * Revision 1.14  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.13  1993/08/19  23:42:57  bryce
 * Remove obsolete defines.  If they break, they should have!
 *
 * Revision 1.12  1993/07/29  04:45:13  bryce
 * Funny names for funny flags created under funny schedules.
 * We're burning them bits fast!
 *
 * Revision 1.10  1993/07/27  06:49:14  stan
 * dragon10
 *
 * Revision 1.10  1993/07/25  06:09:03  stan
 * Fix Bryce Bad checkin
 *
 * Revision 1.11  1993/07/25  04:28:28  bryce
 * This time for real...
 *
 * Revision 1.10  1993/07/25  02:23:21  bryce
 * Allow signed user uapps that don't get privs, without excluding
 * the possiblity of 3DO folios that don't need privs.
 *
 * Revision 1.9  1993/06/14  09:50:18  dale
 * FreeSpace
 *
 * Revision 1.8  1993/06/13  18:51:25  dale
 * define the 3do BinHeader
 *
 * Revision 1.7  1992/11/18  00:05:22  dale
 * rsa stuff
 *
 * Revision 1.6  1992/11/14  03:27:02  dale
 * cleaned up, added more flag definitions
 *
 * Revision 1.5  1992/10/24  01:10:34  dale
 * rcs again
 *****/

/*
  Copyright (C) 1993, The 3DO Company, Inc.
  All Rights Reserved
  Confidential and Proprietary.  DO NOT DISTRIBUTE EXTERNALLY.
*/

#include "types.h"
#include "nodes.h"

/* aif header structure */
/* Acorn Image Format */

/*
 * Note that the Portfolio Debugger uses a different SWI DDE_Debug
 * value, which is defined below as PORTFOLIO_DEBUG_TASK.
 */

#define AIF_NOOP	0xe1a00000	/* mov r0, r0 */
#define AIF_BLAL	0xeb000000
#define OS_EXIT		0xef000011	/* SWI OS_Exit */
#define OS_GETENV	0xef000010	/* SWI OS_GetEnv */
#define AIF_IMAGEBASE	0x00008000	/* Default load address */
#define AIF_BLZINIT	0xeb00000c
#define DEBUG_TASK	0xef041d41	/* RISC OS SWI DDE_Debug */
#define AIF_DBG_SRC	2
#define AIF_DBG_LL	1


typedef struct AIFHeader AIFHeader;
struct AIFHeader
{
  uint32 aif_blDecompress;	/* NOP if image not compressed */
  uint32 aif_blSelfReloc;	/* NOP if image not self-relocating */
  uint32 aif_blZeroInit;	/* NOP if image has none */
  uint32 aif_blEntry;
  uint32 aif_SWIexit;		/* in case of return from main() */
  int32  aif_ImageROsize;	/* includes header */
  int32  aif_ImageRWsize;	/* exact size */
  int32  aif_DebugSize;		/* exact size */
  int32  aif_ZeroInitSize;	/* exact size, (right...) */
  int32  aif_ImageDebugType;	/* 0, 1, 2, or 3 */
  uint32 aif_ImageBase;		/* addr image linked at */
  int32  aif_WorkSpace;         /* initial stack size recommended */
  uint32 aif_AddressMode;
  uint32 aif_DataBaseAddr;	/* Addr image data linked at */
  uint32 aif_Reserved[2];
  uint32 aif_DebugInit;		/* NOP if unused */
  uint32 aif_ZeroInitCode[15];
};

extern AIFHeader __my_AIFHeader;

/* AddressMode flags */
#define AIF_DATABASAT	0x00000100	/* if has separate data base */
#define AIF_ADDR32	32
#define AIF_ADDR26	26

/* DebugInit Values */
/* Used by modbin in the 3DO header to communicate with debugger */
#define	PORTFOLIO_DEBUG_CLEAR	0xE1A02002	/* RISC OS NOP (MOV R1,R1) */
#define	PORTFOLIO_DEBUG_TASK	0xEF00010A	/* RICS OS SWI DDE_Debug */
#define	PORTFOLIO_DEBUG_UNSET	0xE1A00000	/* RISC OS NOP (MOV R0,R0) */

/* WorkSpace flags */
#define AIF_WS_MASK   0xf0000000
#define AIF_STCK_MASK 0x00ffffff	/* max 16M of stack */
#define AIF_3DOHEADER 0x40000000	/* There is a 128 byte header following this */

/* If the AIF_3DOHEADER is defined then the system will take all */
/* needed task parameters from the following structure */
/* above stack WS/aif encoded stack is ignored */

typedef struct _3DOBinHeader _3DOBinHeader;
struct _3DOBinHeader
{
  ItemNode _3DO_Item;
  uint8    _3DO_Flags;
  uint8    _3DO_OS_Version;	/* compiled for this OS release */
  uint8    _3DO_OS_Revision;
  uint8    _3DO_Reserved;
  uint32   _3DO_Stack;		/* stack requirements */
  uint32   _3DO_FreeSpace;	/* preallocate bytes for FreeList */
  uint32   _3DO_Signature;	/* if privileged, offset to beginning of sig */
  uint32   _3DO_SignatureLen;	/* length of signature */
  uint32   _3DO_MaxUSecs;	/* max usecs before task switch */
  uint32   _3DO_Reserved0;	/* must be zero */
  char     _3DO_Name[32];	/* optional task name on startup */
  uint32   _3DO_Time;		/* seconds since 1/1/93 00:00:00 GMT */
  uint32   _3DO_Reserved1[7];	/* must be zero */
};

extern _3DOBinHeader __my_3DOBinHeader;

/* _3DO_Flags */
#define	_3DO_DATADISCOK	32	/* App willing to accept data discs */
#define	_3DO_NORESETOK	16	/* App willing to keep running on empty drawer */
#define	_3DO_LUNCH	8	/* OS has been launched, chips may be lunched */

#define	_3DO_USERAPP	4	/* User app requiring authentication */
#define	_3DO_PRIVILEGE	2
#define _3DO_PERMSTACK	1

#define aif_MD4DataSize		aif_Reserved[0]

EXTERN_C_BEGIN

extern AIFHeader *FindImage(AIFHeader *aifp, uint32 pagemask, char *aifname);
  
EXTERN_C_END

#define	nextImage(aifp,pagemask) FindImage(aifp,pagemask,(char *)NULL)
