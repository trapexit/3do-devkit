
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: ls.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/ls
|||	ls - Displays the contents of a directory.
|||
|||	  Synopsis
|||
|||	    ls [dir1] [dir2] [dir3]...
|||
|||	  Description
|||
|||	    Demonstrates how to scan a directory to determine its contents.
|||
|||	  Arguments
|||
|||	    dir1...                      Name of the directories to list. You can
|||	                                 specify an arbitrary number of directory
|||	                                 names. If no name is specified, the
|||	                                 directory in which the ls program is located
|||	                                 is displayed.
|||
|||	  Associated Files
|||
|||	    ls.c
|||
|||	  Location
|||
|||	    $c/ls
|||
|||	    examples/FileSystem
|||
**/

#include "types.h"
#include "stdio.h"
#include "string.h"
#include "io.h"
#include "operror.h"
#include "filesystem.h"
#include "filefunctions.h"
#include "directory.h"
#include "directoryfunctions.h"


/*****************************************************************************/


static void ListDirectory(const char *path)
{
Directory      *dir;
DirectoryEntry  de;
Item            ioReqItem;
int32           entry;
int32           err;
char            fullPath[FILESYSTEM_MAX_PATH_LEN];
IOInfo          ioInfo;
Item            dirItem;

    /* open the directory for access */
    dirItem = OpenDiskFile((char *)path);
    if (dirItem >= 0)
    {
        /* create an IOReq for the directory */
        ioReqItem = CreateIOReq(NULL, 0, dirItem, 0);
        if (ioReqItem >= 0)
        {
            /* Ask the directory its full name. This will expand any aliases
             * given on the command-line into fully qualified pathnames.
             */
            memset(&ioInfo, 0, sizeof(ioInfo));
            ioInfo.ioi_Command         = FILECMD_GETPATH;
            ioInfo.ioi_Recv.iob_Buffer = fullPath;
            ioInfo.ioi_Recv.iob_Len    = sizeof(fullPath);
            err = DoIO(ioReqItem, &ioInfo);
            if (err >= 0)
            {
                /* now open the directory for scanning */
                dir = OpenDirectoryPath((char *)path);
                if (dir)
                {
                    printf("\nContents of directory %s:\n\n", fullPath);
                    entry = 1;
                    while (ReadDirectory(dir, &de) >= 0)
                    {
                        printf("%5s", (char *) &de.de_Type);
                        printf(" %8d", de.de_ByteCount);
                        printf(" %4d*%4d", de.de_BlockCount, de.de_BlockSize);
                        printf(" %3d", de.de_AvatarCount);
                        printf(" %8x", de.de_Flags);
                        printf(" %s\n", de.de_FileName);
                        entry++;
                    }
                    CloseDirectory(dir);

                    printf("\nEnd of directory\n\n");
                }
                else
                {
                    printf("OpenDirectory(\"%s\") failed\n",fullPath);
                }
            }
            else
            {
                printf("Unable to get full path name for '%s': ",path);
                PrintfSysErr(err);
            }
            DeleteIOReq(ioReqItem);
        }
        else
        {
            printf("CreateIOReq() failed: ");
            PrintfSysErr(ioReqItem);
        }
        CloseDiskFile(dirItem);
    }
    else
    {
        printf("OpenDiskFile(\"%s\") failed: ",path);
        PrintfSysErr(dirItem);
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
int32 i;

    if (argc <= 1)
    {
        /* if no directory name was given, scan the current directory */
        ListDirectory(".");
    }
    else
    {
        /* go through all the arguments */
        for (i = 1; i < argc; i++)
            ListDirectory(argv[i]);
    }

    return 0;
}
