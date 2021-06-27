#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	FindChunk
 *	
 *	Searches buffer until chunk_ID is found. On exit, the buffer
 *	ptr is updated to point to the next chunk and bufLen
 *	contains the number of bytes remaining in the buffer. 
 *	
 *	Errors:		returns NULL or Ptr to chunk
 *
 */
char *FindChunk( ulong chunk_ID, char **buffer, long *bufLen )
{
char	*retValue;
char	*tBuffer = *buffer;
long	tBufLen = *bufLen;
	
	while ( tBufLen > 0 )
	{
		if ( *(long *)tBuffer == chunk_ID ) {
			retValue = tBuffer;
			tBufLen -= ((PixelChunk *)tBuffer)->chunk_size;
			tBuffer += ((PixelChunk *)tBuffer)->chunk_size;
			*buffer = tBuffer;
			*bufLen = tBufLen;
			return retValue;
		}
		
		/* jump to next chunk */
		tBufLen -= ((PixelChunk *)tBuffer)->chunk_size;
		tBuffer += ((PixelChunk *)tBuffer)->chunk_size;
	}
	
return NULL;
}
