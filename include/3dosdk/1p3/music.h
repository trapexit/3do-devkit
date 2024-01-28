/* $Id: music.h,v 1.11 1994/03/15 04:11:39 phil Exp $ */
#pragma include_only_once

#include "operror.h"
#include "midifile.h"
#include "juggler.h"
#include "score.h"
#include "soundspooler.h"
#include "soundfile.h"
#include "iff_fs_tools.h"
#include "handy_tools.h"
#include "flex_stream.h"
#include "sound3d.h"

/* Music Lib errors */

#define ER_LIBRARY Make6Bit('L')
#define ER_MUSIC ((Make6Bit('M')<<6)|Make6Bit('u'))
#define MAKEMLERR(svr,class,err) MakeErr(ER_LIBRARY,ER_MUSIC,svr,ER_E_SSTM,class,err)


#define ML_ERR_BADTAG     MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define ML_ERR_BADTAGVAL  MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define ML_ERR_BADPRIV    MAKEMLERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define ML_ERR_BADSUBTYPE MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define ML_ERR_BADITEM    MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define ML_ERR_NOMEM      MAKEMLERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define ML_ERR_BADPTR     MAKEMLERR(ER_SEVERE,ER_C_STND,ER_BadPtr)

/* Illegal IFF format. */
#define ML_ERR_NOT_IFF_FORM     MAKEMLERR(ER_SEVERE,ER_C_NSTND,1)
/* Could not open file. */
#define ML_ERR_BAD_FILE_NAME    MAKEMLERR(ER_SEVERE,ER_C_NSTND,2)
/* Stream or file not open. */
#define ML_ERR_NOT_OPEN         MAKEMLERR(ER_SEVERE,ER_C_NSTND,3)
/* Bad seek mode or offset. */
#define ML_ERR_BAD_SEEK         MAKEMLERR(ER_SEVERE,ER_C_NSTND,4)
/* Unexpected end of file. */
#define ML_ERR_END_OF_FILE      MAKEMLERR(ER_SEVERE,ER_C_NSTND,5)
/* Sample format has no instrument to play it. */
#define ML_ERR_UNSUPPORTED_SAMPLE   MAKEMLERR(ER_SEVERE,ER_C_NSTND,6)
/* File format is incorrect. */
#define ML_ERR_BAD_FORMAT       MAKEMLERR(ER_SEVERE,ER_C_NSTND,7)
/* UserData in MIDI Parser bad. */
#define ML_ERR_BAD_USERDATA     MAKEMLERR(ER_SEVERE,ER_C_NSTND,8)
/* Input parameter out of range. */
#define ML_ERR_OUT_OF_RANGE     MAKEMLERR(ER_SEVERE,ER_C_NSTND,9)
/* No Template mapped to that Program number. */
#define ML_ERR_NO_TEMPLATE      MAKEMLERR(ER_SEVERE,ER_C_NSTND,10)
/* No voices could be allocated. */
#define ML_ERR_NO_NOTES         MAKEMLERR(ER_SEVERE,ER_C_NSTND,11)

