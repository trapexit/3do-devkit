#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

/* MakeNewCel
	INPUTS: A Rect ptr
	OUTPUTS: A CCB ptr
			 
	DESC:	Creates a 16 pixel uncoded cel the size of the inputted
			rect and fills in the cel data with white.
			
*/

CCB *
MakeNewCel(Rectf16 *r)
{
int32		w, h, byteCount, longCount, scanlineSize;
CCB*		pCel = NULL;

	/* Determine memory size needed to allocate */
	w = ConvertF16_32(r->rectf16_XRight)-ConvertF16_32(r->rectf16_XLeft);
	h = ConvertF16_32(r->rectf16_YBottom)-ConvertF16_32(r->rectf16_YTop);
	byteCount = w*2;
	longCount = (byteCount+3)/4;
	if (longCount < 2) longCount = 2;
	byteCount = longCount*4*h;
	
	/* Allocate a block the size of a CCB and its cellData */
	if ((pCel = (CCB *)ALLOCMEM(sizeof(CCB),MEMTYPE_CEL | (MEMTYPE_FILL | 0x00))) == NULL)
		return NULL;
	if ((pCel->ccb_SourcePtr = (CelData *)ALLOCMEM(byteCount,MEMTYPE_CEL | (MEMTYPE_FILL | 0x00))) == NULL)
		return NULL;
  
	/* Set up CCB fields */
	pCel->ccb_HDX = (1 << 20);
	pCel->ccb_VDY = (1 << 16);
	pCel->ccb_XPos = r->rectf16_XLeft;
	pCel->ccb_YPos = r->rectf16_YTop;
	pCel->ccb_Flags = 	CCB_LAST | CCB_NPABS | CCB_SPABS | 
						CCB_LDSIZE | CCB_LDPRS | CCB_LDPPMP |
						CCB_CCBPRE | CCB_YOXY |
						CCB_ACW | CCB_ACCW;
	pCel->ccb_Width	 = w;
	pCel->ccb_Height = h;
	pCel->ccb_PLUTPtr = NULL;
	pCel->ccb_PIXC  = 0x1F001F00;
	pCel->ccb_PRE0	= ((h-1)<<6) +		/* number of vertical data lines in sprite data -1 */
					  PRE0_LINEAR +		/* use PIN for IPN (0x10) */ 
					  PRE0_BPP_16;		/* set bits/pixel to 16 (0x6) see HDWR spec 3.6.3 */

	scanlineSize = longCount - 2;
	pCel->ccb_PRE1	= (scanlineSize<<16) +	/* offset (in words) from line start to next line start. */ 
					  (w-1);			/* number of horizontal pixels to render -1 */
	return pCel;
}
