#ifndef __TIMER_H
#define __TIMER_H

/*****

$Id: timer.h,v 1.2 1994/01/21 02:31:59 limes Exp $

$Log: timer.h,v $
 * Revision 1.2  1994/01/21  02:31:59  limes
 * recover from rcs bobble
 *
 * Revision 1.3  1994/01/20  02:15:12  sdas
 * C++ compatibility - updated
 *
 * Revision 1.2  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.1  1993/03/16  06:42:46  dale
 * Initial revision
 *
 * Revision 1.7  1993/02/10  08:46:43  dale
 * massive changes to TAG architecture
 *
 * Revision 1.6  1993/01/29  05:08:58  dale
 * structures moved to clio.h
 *
 * Revision 1.5  1992/10/24  01:46:09  dale
 * rcsa
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

/* for controlling the hardware timers */

typedef struct Timer
{
	ItemNode	tm;
	uint8	tm_ID;	/* 0-15 */
	uint8	tm_Size;	/* how many 16 bit ctrs */
	uint8	tm_IntNum;	/* what firq int num */
	uint8	tm_r[1];
	/* n = 0,..,15 */
	void	(*tm_Control)(struct Timer *,int32 set,int32 clr);
	void	(*tm_Load)(int32 n,int32 value16, int32 reloadvalue16);
	int32	(*tm_Read)(int32 n);
} Timer;

#define TIMER_MUST_INTERRUPT	1

enum timer_tags
{
	CREATETIMER_TAG_NUM = TAG_ITEM_LAST+1,	/* How many contiguous timers needed? */
	CREATETIMER_TAG_FLAGS	/* MST needs to generate an interrupt */
};

#define TIMER_DECREMENT	1
#define TIMER_RELOAD	2
#define TIMER_CASCADE	4
#define TIMER_FLABLODE	8
#define TIMER_ALLBITS	0xf

#define TIMER_INTEN	0x10	/* enable interrupts */
#define TIMER_INTREQ	0x20	/* request interrupts */

#endif	/* __TIMER_H */
