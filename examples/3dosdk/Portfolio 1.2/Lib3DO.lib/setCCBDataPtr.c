#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*	setCCBDataPtr
 *
 *	Set the most recently loaded Cel to use this Cel data if
 *	1) there is PDAT to use (lastPDAT)
 *	2) Cel expects data address to be absolute (CCB_SPABS)
 */
void setCCBDataPtr(CCB *lastCCB, char *lastPDAT)
{
  if ((lastCCB->ccb_Flags & CCB_SPABS) && (lastPDAT != NULL))
	lastCCB->ccb_SourcePtr = (CelData *)&(((PixelChunk *)lastPDAT)->pixels[0]);
}
