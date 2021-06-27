#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "filestream.h"
#include "filestreamfunctions.h"

/*==============================================================
	INTERNAL GLOBAL VARIABLES
---------------------------------------------------------------*/

/*=============================================================*/

/*==============================================================
	EXTERNAL GLOBAL VARIABLES
	These variables are exported from Init3DO.lib.c.
---------------------------------------------------------------*/

	extern Item TheIOPort;
/*=============================================================*/
/*
 *	GetFileSize
 *	
 *	returns the size in bytes of the file fn.
 *	
 *	Errors:		returns gIOReqPtr->io_Error if no file
 *	
 */
int GetFileSize(char *fn)
{
	int filesize;
	Stream *fs;
	
	fs = OpenDiskStream(fn, 0);
	if ( fs == NULL) return (-1);
	filesize = (int)fs->st_FileLength;
	CloseDiskStream(fs);
	return filesize;
}
