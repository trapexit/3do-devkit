#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/*	FreeAnimFrames - Free the memory allocated to hold the array of 
 *	
 *	
 *	Errors:	none
 *
 */
void FreeAnimFrames( ANIM * pAnim)
{
long nFrames;
AnimFrame *frames;
	frames = pAnim->pentries;
	nFrames = pAnim->num_Alloced_Frames;
	FREEMEM(frames,nFrames*sizeof(AnimFrame));
	pAnim->pentries = NULL;
	pAnim->num_Alloced_Frames = 0;
	}
