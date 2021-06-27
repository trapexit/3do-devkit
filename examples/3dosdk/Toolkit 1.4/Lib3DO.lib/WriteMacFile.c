/*
	File:		WriteMacFile.c

	Contains:	xxx put contents here xxx

	Written by:	Jay Moreland

	Copyright:	© 1993 by The 3DO Company. All rights reserved.
				This material constitutes confidential and proprietary
				information of the 3DO Company and shall not be used by
				any Person or for any purpose except as expressly
				authorized in writing by the 3DO Company.

	Change History (most recent first):

		 <3>	 1/17/94	JAY		change calling parameters in memset call to match prototype.
		 <2>	 8/13/93	JAY		clean up for 1p01 release

	To Do:
*/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

long
WriteMacFile( char *filename, char *buf, long count )
/* Writes count bytes from the specified buffer into 
 * the file filename.  Returns the actual length of 
 * what was written. 
 */
{
	int32 j;
	long retvalue;
	IOInfo FileInfo;
	extern Item TheIOPort;
	extern IOReq* TheReplyPort;

	retvalue = -1;
	memset( &FileInfo, 0, sizeof(IOInfo) );
	FileInfo.ioi_Command = CMD_WRITE;
	FileInfo.ioi_Unit = 0;
	
	FileInfo.ioi_Send.iob_Buffer = buf;
	FileInfo.ioi_Send.iob_Len = (int)( count & ~3 );
	FileInfo.ioi_Recv.iob_Buffer = filename;
	FileInfo.ioi_Recv.iob_Len = strlen( filename ) + 1;

	FileInfo.ioi_Offset = 0;
	if ( (j = DoIO( TheIOPort, &FileInfo )) < 0 )
		{
		kprintf ("ERROR:  couldn't write Mac file %s : %d ",
			filename, j );
		kprintf ("($%x), %d ($%x)\n",
			j, TheReplyPort->io_Error, TheReplyPort->io_Error);
		kprintf( "SysErr() = " );
#ifndef CardinalChange
		PrintfSysErr( j );
#else
		SysErr( j );
#endif
		}
	else retvalue = TheReplyPort->io_Actual;

	return ( retvalue );
}
