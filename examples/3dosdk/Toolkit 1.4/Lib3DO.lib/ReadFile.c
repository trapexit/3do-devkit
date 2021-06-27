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
 *	ReadFile
 *	
 *	loads fileSize bytes of data starting at offset from the file
 *	identified by filename into buffer.
 *
 *	Errors:		returns gIOReqPtr->io_Error if an error occurs during the read
 *	
 */
int ReadFile(char *filename, Int32 size, long *buffer, long offset)
{
	long retval;
	Stream *fs;
	char * cbuf;
	long ntoread, nleft;	
	int retries = 4;
	fs = OpenDiskStream(filename, 0);
	if ( fs == NULL) return (-1);
	retval = SeekDiskStream(fs, offset, SEEK_SET);
	nleft = size;
	cbuf = (char *) buffer;
	if (retval >= 0)
	  while (nleft > 0)
		{
		ntoread = ((FS_CHUNKSIZE >= nleft) ? nleft: FS_CHUNKSIZE);
		retval = ReadDiskStream(fs, cbuf, ntoread);
		if(retval < 0) {
			retries--;
			if ( retries == 0) break;
			}
		else
			{
			nleft -= ntoread;
			cbuf += ntoread;
			retries = 4;
			}
		}
	CloseDiskStream(fs);

return (int)retval;
}
