#ifndef __ACCESS_H
#define __ACCESS_H

#pragma force_top_level
#pragma include_only_once


/****************************************************************************/


/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


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

/* error return values, possible for all request types */
#define ACCERR_BAD_REQUEST  -5001 /* Unknown request argument         */
#define ACCERR_NO_SCREEN    -5002 /* No screen specified              */
#define ACCERR_BAD_ARGS     -5003 /* Something abotu the args was bad */


/*****************************************************************************/


#endif /* __ACCESS_H */
