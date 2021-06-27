#ifndef __ACCESS_H
#define __ACCESS_H

#pragma force_top_level
#pragma include_only_once


/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: access.h,v 1.7 1994/09/10 02:02:11 vertex Exp $
**
**  Access simple UI services interface definitions
**
******************************************************************************/


#ifndef __OPERROR_H
#include "operror.h"
#endif


/****************************************************************************/


typedef enum AccessTags
{
    ACCTAG_REQUEST = 1,
    ACCTAG_SCREEN,
    ACCTAG_BUFFER,
    ACCTAG_BUFFERSIZE,
    ACCTAG_TITLE,
    ACCTAG_TEXT,
    ACCTAG_BUTTON_ONE,
    ACCTAG_BUTTON_TWO,
    ACCTAG_SAVE_BACK,
    ACCTAG_STRINGBUF,
    ACCTAG_STRINGBUF_SIZE,
    ACCTAG_FG_PEN,
    ACCTAG_BG_PEN,
    ACCTAG_HILITE_PEN,
    ACCTAG_SHADOW_PEN
} AccessTags;


/*****************************************************************************/


/* Type of services provided by Access. For use as an argument to the
 * ACCTAG_REQUEST tag.
 */
typedef enum AccessRequests
{
    ACCREQ_LOAD = 1,
    ACCREQ_SAVE,
    ACCREQ_DELETE,
    ACCREQ_OK,
    ACCREQ_OKCANCEL,
    ACCREQ_ONEBUTTON,
    ACCREQ_TWOBUTTON
} AccessRequests;


/*****************************************************************************/


/* return values for an ACCREQ_OKCANCEL request */
#define ACCRET_OK          0
#define ACCRET_CANCEL      1

/* return values for an ACCREQ_TWOBUTTON request */
#define ACCRET_BUTTON_ONE  0
#define ACCRET_BUTTON_TWO  1


/*****************************************************************************/


/* error return values, possible for all request types */

/* Unknown request argument */
#define ACCERR_BAD_REQUEST MakeTErr(ER_ACCESS,ER_SEVERE,ER_C_NSTND,1)

/* No screen specified */
#define ACCERR_NO_SCREEN   MakeTErr(ER_ACCESS,ER_SEVERE,ER_C_NSTND,2)

/* Something about the args was bad */
#define ACCERR_BAD_ARGS    MakeTErr(ER_ACCESS,ER_SEVERE,ER_C_NSTND,3)


/*****************************************************************************/


#endif /* __ACCESS_H */
