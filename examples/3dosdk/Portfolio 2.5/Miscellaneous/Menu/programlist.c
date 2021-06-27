
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: programlist.c,v 1.1 1994/10/18 20:04:54 vertex Exp $
**
**  Utility routines to load in and manage a list of programs.
**
******************************************************************************/

#include "types.h"
#include "mem.h"
#include "stdio.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "filefunctions.h"
#include "string.h"
#include "ctype.h"
#include "operror.h"
#include "programlist.h"


/*****************************************************************************/


/* Given a line of text, add an entry to the program list */
static bool AddProgramEntry(ProgramList *programList, char *ptr)
{
ProgramNode *node;
bool         quoted;
char        *startLabel;
char        *startCmdLine;
uint32       labelLen;

    /* skip leading white space */
    while (isspace(*ptr))
        ptr++;

    /* isolate the label name */
    quoted = FALSE;
    if (*ptr == '"')
    {
        ptr++;
        quoted = TRUE;
    }

    startLabel = ptr;
    while (TRUE)
    {
        if (*ptr == 0)
        {
            break;
        }
        else if (quoted)
        {
            if (*ptr == '"')
            {
                *ptr = 0;
                ptr++;
                break;
            }
        }
        else if (*ptr == ';')   /* comment indicator */
        {
            *ptr = 0;
            break;
        }
        else if (*ptr == ' ')
        {
            *ptr = 0;
            ptr++;
            break;
        }

        ptr++;
    }

    /* skip white space between the label and the command-line */
    while (isspace(*ptr))
        ptr++;

    /* the rest of the entry line is used as the command-line */
    startCmdLine = ptr;

    labelLen = strlen(startLabel);
    if (labelLen == 0)
        return TRUE;

    if (strlen(startCmdLine) == 0)
        return TRUE;

    node = (ProgramNode *)AllocMem(sizeof(ProgramNode),MEMTYPE_ANY);
    if (node)
    {
        node->pn_Label    = startLabel;
        node->pn_CmdLine  = startCmdLine;
        node->pn_LabelLen = labelLen;

        AddTail(&programList->pl_Programs,(Node *)node);
        programList->pl_NumPrograms++;

        return TRUE;
    }

    return FALSE;
}


/*****************************************************************************/


/* Given a filename, parse the file and construct a usable ProgramList */
Err LoadProgramList(const char *scriptName, ProgramList *programList)
{
Stream   *stream;
void     *buffer;
char     *ptr;
char     *start;
int32     num;
Err       result;

    /* initialize the program list */
    InitList(&programList->pl_Programs,"Program List");
    programList->pl_Storage     = NULL;
    programList->pl_NumPrograms = 0;

    /* try to read the file */
    stream = OpenDiskStream((char *)scriptName,0);
    if (stream)
     {
        /* allocate enough memory to hold the file, plus one byte */
        buffer = malloc(stream->st_FileLength + 1);
        if (buffer)
        {
            /* read the whole file */
            num = ReadDiskStream(stream,(char *)buffer,stream->st_FileLength);
            if (num == stream->st_FileLength)
            {
                /* if everything was read in, start parsing the lines */
                ptr                     = (char *)buffer;
                ptr[num]                = 0;
                programList->pl_Storage = buffer;
                buffer                  = NULL;

                /* isolate lines of the file, and parse them */
                while (*ptr)
                {
                    start = ptr;
                    while (*ptr && (*ptr != '\r') && (*ptr != '\n'))
                        ptr++;

                    if (*ptr)
                    {
                        *ptr = 0;
                        ptr++;
                    }

                    AddProgramEntry(programList,start);
                }

                /* we succeeded */
                result = 0;
            }
            else
            {
                /* we failed to read the whole file */
                result = -1;  /* generic failure */
            }

            free(buffer);
        }
        else
        {
            /* no memory for the buffer */
            result = NOMEM;
        }
        CloseDiskStream(stream);
    }
    else
    {
        result = -1;  /* generic failure */
    }

    return result;
}


/*****************************************************************************/


/* Given a program list, free any resources allocated when the list was loaded */
void UnloadProgramList(ProgramList *programList)
{
ProgramNode *node;

    /* free all the program nodes */
    while (TRUE)
    {
        node = (ProgramNode *)RemHead(&programList->pl_Programs);
        if (!node)
            break;

        FreeMem(node,sizeof(ProgramNode));
    }

    /* free the text buffer */
    free(programList->pl_Storage);
}


/*****************************************************************************/


/* Run a given program from a program list */
Err RunProgram(ProgramList *programList, uint32 programNum)
{
Item         task;
ProgramNode *node;
Err          result;

    /* find the correct node within our list of programs */
    node = (ProgramNode *)FindNodeFromHead(&programList->pl_Programs,programNum);
    if (node)
    {
        /* load and start the program */
        task = LoadProgram(node->pn_CmdLine);
        if (task >= 0)
        {
            /* This waits for the task we created to terminate */
            while (LookupItem(task))
                WaitSignal(SIGF_DEADTASK);

            /* indicate success */
            result = 0;
        }
        else
        {
            result = task;
        }
    }
    else
    {
        result = -1;  /* generic failure */
    }

    return result;
}
