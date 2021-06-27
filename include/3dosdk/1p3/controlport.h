/*****

$Id: controlport.h,v 1.4 1994/03/19 01:11:30 dplatt Exp $

$Log: controlport.h,v $
 * Revision 1.4  1994/03/19  01:11:30  dplatt
 * Replace the old (unused) READWRITE and WAIT commands with a new
 * CONFIGURE command (which isn't used yet, but someday...)
 *
 * Revision 1.3  1994/01/21  02:53:20  limes
 * recover from rcs bobble
 *
 * Revision 1.3  1994/01/20  21:12:03  dplatt
 * Fix RCS stuff
 *

*****/

/*
  Copyright The 3DO Company Inc., 1993, 1992, 1991.
  All Rights Reserved Worldwide.
  Company confidential and proprietary.
  Contains unpublished technical data.
*/

/*
  controlport.h - definitions for the Control Port driver

*/

#define CONTROLPORTCMD_CONFIGURE        3       /* configure port */


