
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: allocmem.c,v 1.8 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/allocmem
|||	allocmem - Demonstrates how to allocate a block of memory and return it.
|||
|||	  Synopsis
|||
|||	    allocmem
|||
|||	  Description
|||
|||	    Demonstrates how to use AllocMem() and FreeMem() to allocate and free
|||	    memory.
|||
|||	  Associated Files
|||
|||	    allocmem.c
|||
|||	  Location
|||
|||	    examples/Kernel
|||
**/

#include "types.h"
#include "mem.h"
#include "stdio.h"


int main(int32 argc, char **argv)
{
void *memBlock;

    memBlock = AllocMem(123,MEMTYPE_ANY);  /* allocate any type of memory */
    if (memBlock)
    {
        /* memBlock now contains the address of a block of memory
         * 123 bytes in size
         */

        FreeMem(memBlock,123); /* return the memory */
    }
    else
    {
        printf("Could not allocate memory!\n");
    }
}
