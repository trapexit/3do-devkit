

/*****

$Id: scanf.h,v 1.2 1992/10/24 01:43:54 dale Exp $

$Log: scanf.h,v $
 * Revision 1.2  1992/10/24  01:43:54  dale
 * rcs
 *

 *****/

#define NOSTORE      01
#define LONG	     02
#define SHORT	     04
#define FIELDGIVEN  010
#define LONGDOUBLE  020
#define ALLOWSIGN   040    /* + or - acceptable to rd_int  */
#define DOTSEEN    0100    /* internal to rd_real */
#define NEGEXP	   0200    /* internal to rd_real */
#define NUMOK	   0400    /* ditto + rd_int */
#define NUMNEG	  01000    /* ditto + rd_int */
