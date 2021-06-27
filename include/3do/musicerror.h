#ifndef __MUSICERROR_H
#define __MUSICERROR_H

/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: musicerror.h,v 1.11 1994/10/07 21:09:39 peabody Exp $
**
**  Music library error codes.
**
****************************************************************************/


#include "operror.h"


/* -------------------- Error codes */

#ifndef ER_LINKLIB
  #define ER_LINKLIB               Make6Bit('L')
#endif

#ifndef ER_MUSICLIB
  #define ER_MUSICLIB              MakeErrId('M','u')
#endif

#ifdef MakeLErr
  #define MAKEMLERR(svr,class,err) MakeLErr(ER_MUSICLIB,svr,class,err)
#else
  #define MAKEMLERR(svr,class,err) MakeErr(ER_LINKLIB,ER_MUSICLIB,svr,ER_E_SSTM,class,err)
#endif

    /* Standard errors */
#define ML_ERR_BADTAG               MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define ML_ERR_BADTAGVAL            MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define ML_ERR_BADPRIV              MAKEMLERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define ML_ERR_BADSUBTYPE           MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define ML_ERR_BADITEM              MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define ML_ERR_NOMEM                MAKEMLERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define ML_ERR_BADPTR               MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define ML_ERR_NOT_SUPPORTED        MAKEMLERR(ER_SEVERE,ER_C_STND,ER_NotSupported)

    /* Non-standard errors */
#define ML_ERR_NOT_IFF_FORM         MAKEMLERR(ER_SEVERE,ER_C_NSTND,1)   /* Illegal IFF format. */
#define ML_ERR_BAD_FILE_NAME        MAKEMLERR(ER_SEVERE,ER_C_NSTND,2)   /* Could not open file. */
#define ML_ERR_NOT_OPEN             MAKEMLERR(ER_SEVERE,ER_C_NSTND,3)   /* Stream or file not open. */
#define ML_ERR_BAD_SEEK             MAKEMLERR(ER_SEVERE,ER_C_NSTND,4)   /* Bad seek mode or offset. */
#define ML_ERR_END_OF_FILE          MAKEMLERR(ER_SEVERE,ER_C_NSTND,5)   /* Unexpected end of file. */
#define ML_ERR_UNSUPPORTED_SAMPLE   MAKEMLERR(ER_SEVERE,ER_C_NSTND,6)   /* Sample format has no instrument to play it. */
#define ML_ERR_BAD_FORMAT           MAKEMLERR(ER_SEVERE,ER_C_NSTND,7)   /* File format is incorrect. */
#define ML_ERR_BAD_USERDATA         MAKEMLERR(ER_SEVERE,ER_C_NSTND,8)   /* UserData in MIDI Parser bad. */
#define ML_ERR_OUT_OF_RANGE         MAKEMLERR(ER_SEVERE,ER_C_NSTND,9)   /* Input parameter out of range. */
#define ML_ERR_NO_TEMPLATE          MAKEMLERR(ER_SEVERE,ER_C_NSTND,10)  /* No Template mapped to that Program number. */
#define ML_ERR_NO_NOTES             MAKEMLERR(ER_SEVERE,ER_C_NSTND,11)  /* No voices could be allocated. */
#define ML_ERR_BAD_ARG              MAKEMLERR(ER_SEVERE,ER_C_NSTND,12)  /* Invalid or conflicting argument(s) supplied to function. */
#define ML_ERR_BAD_SAMPLE_ALIGNMENT MAKEMLERR(ER_SEVERE,ER_C_NSTND,13)  /* Sample not properly DMA aligned */
#define ML_ERR_BAD_SAMPLE_ADDRESS   MAKEMLERR(ER_SEVERE,ER_C_NSTND,14)  /* Invalid sample address/length (e.g. NULL address) */
#define ML_ERR_INCOMPATIBLE_SOUND   MAKEMLERR(ER_SEVERE,ER_C_NSTND,15)  /* sound is incompatible with player */
#define ML_ERR_CORRUPT_DATA         MAKEMLERR(ER_SEVERE,ER_C_NSTND,16)  /* some music.lib data has become corrupt */
#define ML_ERR_INVALID_MARKER       MAKEMLERR(ER_SEVERE,ER_C_NSTND,17)  /* inappropriate marker (e.g. not a "to" marker) (soundplayer) */
#define ML_ERR_INVALID_FILE_TYPE    MAKEMLERR(ER_SEVERE,ER_C_NSTND,18)  /* inappropriate file type (e.g. not an AIFF) */
#define ML_ERR_DUPLICATE_NAME       MAKEMLERR(ER_SEVERE,ER_C_NSTND,19)  /* named thing already exists */
#define ML_ERR_NAME_NOT_FOUND       MAKEMLERR(ER_SEVERE,ER_C_NSTND,20)  /* named thing not found */
#define ML_ERR_BUFFER_TOO_SMALL     MAKEMLERR(ER_SEVERE,ER_C_NSTND,21)  /* A (sound) buffer is too small */


/* -------------------- Functions */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err InstallMusicLibErrors (void);

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif  /* __MUSICERROR_H */
