#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/* ReAllocateAnim      */
/* static routine used to dynamically allocate AnimFrames */
/* as an ANIM file is Parsed     */
static AnimFrame * ReAllocateAnim( ANIM *pAnim)
{
AnimFrame	*oldFrame, *newFrame,*xFrame;
long oldn, newn;
long i;
	oldn = pAnim->num_Alloced_Frames;
	oldFrame = pAnim->pentries;
	newn = oldn + N_FRAMES_PER_CHUNK;
	if ((newFrame = (AnimFrame *)ALLOCMEM(newn*sizeof(AnimFrame),MEMTYPE_CEL)) == NULL)
			return NULL;
    for(i=0;i<oldn;i++) newFrame[i] = oldFrame[i];
	/* memcpy( newFrame, oldFrame, oldn*sizeof(AnimFrame)); */
	FREEMEM(oldFrame,oldn*sizeof(AnimFrame));
	pAnim->pentries = newFrame;
	pAnim->num_Alloced_Frames = newn;
    xFrame = newFrame+oldn;
	for (i=0; i<N_FRAMES_PER_CHUNK; ++i) {
		xFrame->af_CCB		= NULL;
		xFrame->af_PLUT		= NULL;
		(xFrame++)->af_pix	= NULL;
	}
	return newFrame+oldn;
}
