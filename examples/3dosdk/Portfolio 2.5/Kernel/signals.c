
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: signals.c,v 1.5 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/signals
|||	signals - Demonstrates how to use signals.
|||
|||	  Synopsis
|||
|||	    signals
|||
|||	  Description
|||
|||	    Demonstrates the use of threads and signals.
|||
|||	    The main() routine launches two threads. These threads sit in a loop and
|||	    count. After a given number of iterations through their loop, they send a
|||	    signal to the parent task. When the parent task gets a signal, it wakes up
|||	    and prints the current counters of the threads to show how much they were
|||	    able to count.
|||
|||	  Associated Files
|||
|||	    signals.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include "types.h"
#include "task.h"
#include "kernel.h"
#include "stdio.h"
#include "operror.h"


/*****************************************************************************/


/* Global variables shared by all threads. */
static int32  thread1Sig;
static int32  thread2Sig;
static Item   parentItem;
static uint32 thread1Cnt;
static uint32 thread2Cnt;


/*****************************************************************************/


/* This routine shared by both threads */
static void DoThread(int32 signal, uint32 amount, uint32 *counter)
{
uint32 i;

    while (TRUE)
    {
        for (i = 0; i < amount; i++)
        {
            (*counter)++;
            SendSignal(parentItem,signal);
        }
    }
}


/*****************************************************************************/


static void Thread1Func(void)
{
    DoThread(thread1Sig, 100000, &thread1Cnt);
}


/*****************************************************************************/


static void Thread2Func(void)
{
    DoThread(thread2Sig, 200000,&thread2Cnt);
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
uint8  parentPri;
Item   thread1Item;
Item   thread2Item;
uint32 count;
int32  sigs;

    /* get the priority of the parent task */
    parentPri  = CURRENTTASK->t.n_Priority;

    /* get the item number of the parent task */
    parentItem = CURRENTTASK->t.n_Item;

    /* allocate one signal bits for each thread */
    thread1Sig = AllocSignal(0);
    thread2Sig = AllocSignal(0);

    /* spawn two threads that will run in parallel */
    thread1Item = CreateThread("Thread1", parentPri, Thread1Func, 2048);
    thread2Item = CreateThread("Thread2", parentPri, Thread2Func, 2048);

    /* enter a loop until we receive 10 signals */
    count = 0;
    while (count < 10)
    {
        sigs = WaitSignal(thread1Sig | thread2Sig);

        printf("Thread 1 at %d, thread 2 at %d\n",thread1Cnt,thread2Cnt);

        if (sigs & thread1Sig)
            printf("Signal from thread 1\n");

        if (sigs & thread2Sig)
            printf("Signal from thread 2\n");

        count++;
    }

    /* nuke both threads */
    DeleteThread(thread1Item);
    DeleteThread(thread2Item);
}
