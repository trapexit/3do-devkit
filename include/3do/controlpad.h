#pragma once

/******************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: controlpad.h,v 1.9 1995/01/30 21:52:28 gyld Exp $
 **
 ******************************************************************************/

#include "extern_c.h"

#include "types.h"
#include "operror.h"

#ifdef DEBUG
#define	CPERR(errNum)                           \
  {                                             \
    PRT (("%s (%ld)", __FILE__, __LINE__));     \
    PrintfSysErr((errNum));                     \
  }
#else
#define CPERR(errNum)
#endif

/*
  This is stuff for making our errors be opera compatible.
*/

#define ER_CPAD	MakeErrId('C','p')

#define MAKECPADERR( class, errNum )MakeErr( ER_USER, ER_CPAD, ER_SEVERE, ER_E_USER, class, errNum )

#define INITCONTROLPAD_ERR MAKECPADERR ( ER_C_NSTND, 1 )
#define PADNUMBER_ERR	   MAKECPADERR ( ER_C_NSTND, 2 )
#define INITEDUTIL_ERR	   MAKECPADERR ( ER_C_NSTND, 3 )
#define ALLOCMEM_ERR	   MAKECPADERR ( ER_C_NSTND, 4 )
#define ALREADYALLOC_ERR   MAKECPADERR ( ER_C_NSTND, 5 )

/*
  function prototypes for functions found in controlpad.c
*/
EXTERN_C_BEGIN

extern s32 InitControlPad ( s32 nPads );
extern s32 KillControlPad ( void );
extern s32 DoControlPad ( s32 whichPad, u32 *pButton, s32 continuousBits );
extern s32 ReturnPreviousControlPad ( s32 whichPad, u32 *pButton );

EXTERN_C_END
