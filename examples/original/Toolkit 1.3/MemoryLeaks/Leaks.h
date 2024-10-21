/*
        File:		Leaks.h

        Contains:	track memory allocation and free, hopefully make it
                                 easier to track down leaks

        Written by:	Eric Carlson

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <2>	 10/12/93	EC		Added function
   Leaks_MemBlockSize <1>	 9/29/93	EC		first checked
   in 9/27/93	EC		New today

        To Do:
*/

#ifndef __LEAKS__
#define __LEAKS__

// convience macro so you don't have to include source file info in call to
//  dump
#define Leaks_Dump() _Leaks_Dump (__LINE__, __FILE__)

void Leaks_On (void);
void Leaks_Off (void);
void _Leaks_Dump (int32 lineNum, char *fileName);
uint32 Leaks_MemBlockSize (void *ptr, int32 lineNum, char *fileName);
void *Leaks_Malloc (uint32 bytesNeeded, uint32 memType, int32 lineNum,
                    char *fileName);
void *Leaks_Free (void *address, int32 lineNum, char *fileName);

#endif
