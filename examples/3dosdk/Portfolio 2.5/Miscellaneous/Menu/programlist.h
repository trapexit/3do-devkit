#ifndef __PROGRAMLIST_H
#define __PROGRAMLIST_H

/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: programlist.h,v 1.1 1994/10/18 20:04:54 vertex Exp $
**
**  Utility routines to load in and manage a list of programs.
**
******************************************************************************/

#ifndef __TYPES_H
#include "types.h"
#endif

#ifndef __LIST_H
#include "list.h"
#endif

/*****************************************************************************/

/* A list of programs */
typedef struct ProgramList
{
  List pl_Programs;
  uint32 pl_NumPrograms;
  void *pl_Storage;
} ProgramList;

/* A node as found in the ProgramList.pl_Programs list */
typedef struct ProgramNode
{
  MinNode pn_Link;
  char *pn_Label;
  uint32 pn_LabelLen;
  char *pn_CmdLine;
} ProgramNode;

/*****************************************************************************/

Err LoadProgramList (const char *scriptName, ProgramList *programList);
void UnloadProgramList (ProgramList *programList);
Err RunProgram (ProgramList *programList, uint32 programNum);

/*****************************************************************************/

#endif /* __PROGRAMLIST_H */
