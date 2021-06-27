#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*
 *	GetChunk
 *	
 *	Returns the next chunk_ID in this file. On exit, chunk_ID contains
 *	the chucnk_ID found in the chucnk pointed to by buffer, the buffer
 *	ptr is updated to point to the next chunk and the bufLen param
 *	has the number of bytes remaining in the buffer. 
 *	
 *	Errors:		returns NULL or Ptr to chunk
 *
 */
char *GetChunk( ulong *chunk_ID, char **buffer, long *bufLen )
{
char *buf;

	if ( *bufLen > 0 ) {

		buf = *buffer;
		*chunk_ID = ((PixelChunk *)buf)->chunk_ID;
		
		/* jump to next chunk */
		*bufLen -= ((PixelChunk *)buf)->chunk_size;
		*buffer += ((PixelChunk *)buf)->chunk_size;
		
		return buf;
	}
	
return NULL;
}
