/* $Id: $ */

/****************************************************************************/


/* Copyright (C) 1993-1994, The 3DO Company.
 * All Rights Reserved
 * Confidential and Proprietary
 */


/*****************************************************************************/


#include "types.h"
#include "graphics.h"
#include "mem.h"
#include "debug.h"
#include "stdio.h"
#include "operror.h"
#include "storagetuner.h"


/*****************************************************************************/


/* This is a simple example program showing how to use the Storage Tuner link
 * library. This program creates a screen, and allocates a memory buffer
 * for use by the tuner. This lets the user of the library better control
 * memory allocations, and screen use.
 *
 * If you wish the tuner to open its own screen, provide -1 for the first
 * argument to StorageTunerRequest().
 *
 * If you wish the tuner to allocate its own memory, pass NULL for the
 * second parameter to the StorageTunerRequest() function.
 */


int main(uint32 argc, char **argv)
{
Err      err;
int32    i;
TagArg   tags[2];
Item     screenGroup;
Item     screens[2];
MemList *ml;
List     list;
void    *buffer;

    /* Allocate a work buffer, and create a MemList to go with it. Then
     * attach the MemList in a List. This List is what gets passed to the
     * tuner.
     */
    buffer = AllocMem(100*1024,MEMTYPE_ANY);
    ml = AllocMemList(buffer,NULL);
    FreeMemToMemList(ml,buffer,100*1024);
    InitList(&list,NULL);
    AddHead(&list,(Node *)ml);

    printf("Storage Tuner Example\n");

    err = OpenGraphicsFolio();
    if (err >= 0)
    {
        /* open a screen */

        tags[0].ta_Tag = CSG_TAG_SCREENCOUNT;
        tags[0].ta_Arg = (void *)2;
        tags[1].ta_Tag = CSG_TAG_DONE;
        screenGroup = CreateScreenGroup(screens, tags);
        AddScreenGroup(screenGroup, NULL);
        for (i = 0; i < 2; i++)
        {
            EnableHAVG(screens[i]);
            EnableVAVG(screens[i]);
        }

        /* call the tuner */
        err = StorageTunerRequest(screenGroup,&list,NULL);

        RemoveScreenGroup(screenGroup);
        DeleteScreenGroup(screenGroup);

        printf("StorageTunerRequest: %d, ",err);
        PrintfSysErr(err);

//      only available in Portfolio 2.0
//      CloseGraphicsFolio();
    }
    else
    {
        printf("Could not open graphics folio\n");
    }

    FreeMemList(ml);
    FreeMem(buffer,100*1024);

    return (0);
}
