#ifndef __KERNELNODES_H
#define __KERNELNODES_H

/*****

$Id: kernelnodes.h,v 1.9 1994/01/21 02:17:30 limes Exp $

$Log: kernelnodes.h,v $
 * Revision 1.9  1994/01/21  02:17:30  limes
 * recover from rcs bobble:
 *
 * +  * Revision 1.10  1994/01/20  02:01:40  sdas
 * +  * C++ compatibility - updated
 * +  *
 * +  * Revision 1.9  1994/01/18  02:37:03  dale
 * +  * Corrected Copyright
 * +  * added #pragma include_only_once
 * +  * changed _<filename>_H to __<filename>_H
 * +  * misc cleanup
 *
 * Revision 1.10  1994/01/20  02:01:40  sdas
 * C++ compatibility - updated
 *
 * Revision 1.9  1994/01/18  02:37:03  dale
 * Corrected Copyright
 * added #pragma include_only_once
 * changed _<filename>_H to __<filename>_H
 * misc cleanup
 *
 * Revision 1.8  1993/06/23  18:41:40  dale
 * added SEMAPHORENODE as alias to SEMA4NODE
 *
 * Revision 1.7  1993/02/16  20:24:44  dale
 * docs
 *
 * Revision 1.6  1993/01/13  04:12:49  dale
 * new ErrorText kernelnode
 *
 * Revision 1.5  1992/12/16  21:55:50  dale
 * printf and syserr
 *
 * Revision 1.4  1992/10/24  01:37:21  dale
 * fix id
 *

 *****/

/*
    Copyright (C) 1993, The 3DO Company, Inc.
    All Rights Reserved
    Confidential and Proprietary
*/

#pragma force_top_level
#pragma include_only_once

#define KERNELNODE	1

#define MEMFREENODE	1
#define LISTNODE	2
#define MEMHDRNODE	3
#define FOLIONODE	4	/* see folio.h */
#define TASKNODE	5	/* see task.h */
#define FIRQNODE	6	/* see interrupts.h */
#define SEMA4NODE	7	/* see semaphores.h */
#define SEMAPHORENODE	SEMA4NODE	/* see semaphores.h */
#define SEMA4WAIT	8

#define MESSAGENODE	9	/* see msgports.h */
#define MSGPORTNODE	10	/* see msgports.h */

#define MEMLISTNODE	11
#define ROMTAGNODE	12

#define DRIVERNODE	13	/* see drivers.h */
#define IOREQNODE	14	/* see io.h */
#define	DEVICENODE	15	/* see devices.h */
#define TIMERNODE	16	/* see timer.h */
#define ERRORTEXTNODE	17	/* see operror.h */

/* nodes with 'see xxx.h' are also items */

#endif	/* __KERNELNODES_H */
