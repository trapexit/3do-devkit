#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

void
FreeBuffer (char *filename, long *fileBuffer)
{
int		buffSize;

	if ((buffSize = GetFileSize(filename)) <= 0)
	  return;

	FREEMEM (fileBuffer, buffSize);
}
