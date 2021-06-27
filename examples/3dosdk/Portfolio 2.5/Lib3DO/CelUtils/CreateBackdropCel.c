
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CreateBackdropCel.c,v 1.5 1994/11/08 06:24:13 ewhac Exp $
**
**  Lib3DO routine to creates a rectangular cel containing a single color.
**
**  DeleteCel() compatible.
**
**  It's often handy to create a rectangular cel to serve as a backdrop for
**  text, a dialog box, etc.  Sometimes you want it opaque, sometimes you
**  want it translucent.  This function creates such a cel for you, using
**  the color and opacity you specify.
**
**  The cel is created with a source data size of 1x1 pixel, and the HDX/VDY
**  fields in the CCB are used to project that one pixel to the required
**  size.  The one pixel is stashed in the (otherwise unused) ccb_PLUTPtr
**  field, so creating a backdrop cel uses up only sizeof(CCB) memory.  The
**  CreateCel() function wants to allocate a data buffer for us unless we
**  provide our own buffer pointer in the call.  Because we use the PLUTPtr
**  field as a data buffer, and that field isn't available until after the
**  cel is created, we pass a dummy non-NULL buffer parm to CreateCel() so
**  that it doesn't allocate a 1x1 pixel data buffer for us.
**
**  The cel can be MapCel'd, or manipulated in any other way you wish.
**
**  The color parameter is an RGB555 value.  There's currently no way to
**  change the color or opacity of the cel once it's created.  (No easy way
**  at least.  You can bash the PIXC and color pixel yourself if you want.)
**
**  Eight levels of opacity are available; the specified percentage is
**  rounded more or less to the nearest 1/8 increment available.  (EG, 53%
**  is rounded down to 4/8, 54% is rounded up to 5/8, which isn't exactly
**  'nearest' type rounding, but it works well enough and avoids the need for
**  lots of special-case logic for values less than 13% in the code below.)
**
******************************************************************************/


#include "types.h"
#include "mem.h"
#include "celutils.h"
#include "debug3do.h"

#define DUMMY_BUFFER	((void *)1)	/* a dummy non-NULL data buffer pointer */

#define	PIXCL_BLEND_8	(PPMPC_1S_CFBD | PPMPC_SF_8 | PPMPC_2S_PDC)

/*----------------------------------------------------------------------------
 * CreateBackdropCel()
 *	Create a cel suitable for use as a backdrop for other cels.  Handy for
 *	creating dialog boxes and such.
 *--------------------------------------------------------------------------*/

CCB * CreateBackdropCel(int32 width, int32 height, int32 color, int32 opacityPct)
{
	CCB *	pCel;
	int32	scaleMul;
	int32	r, g, b;
	int32	scaledColor;
	SRect	srect;

	/*------------------------------------------------------------------------
	 * validate parms.
	 *----------------------------------------------------------------------*/

#ifdef DEBUG
	if (opacityPct < 0 || opacityPct > 100) {
		DIAGNOSE(("Opacity of %ld%% is invalid, 100%% assumed\n", opacityPct));
		opacityPct = 100;
	}

	if (width <= 0) {
		width = 1;
	}

	if (height <= 0) {
		height = 1;
	}
#endif

	/*------------------------------------------------------------------------
	 * create a 16-bit uncoded cel at a 1x1 source data size, then map its
	 * projection to the caller-specified width and height.
	 *----------------------------------------------------------------------*/

	if ((pCel = CreateCel(1, 1, 16, CREATECEL_UNCODED, DUMMY_BUFFER)) == NULL) {
		return NULL;	/* error already reported by CreateCel(). */
	}

	MapCelToSRect(pCel, SRectFromIVal(&srect, 0, 0, width, height));

	/*------------------------------------------------------------------------
	 * Set up the PIXC word so that the primary source is the current frame
	 * buffer pixel, scaled according to the opacity percent, and the
	 * secondary source is a single pixel of the requested color, pre-scaled
	 * to the inverse of the primary source scaling factor.
	 *----------------------------------------------------------------------*/

	if (opacityPct == 0) {						/* 0% opacity means this is a  */
		scaledColor = 0;						/* 'virtual' cel that doesn't */
		pCel->ccb_Flags |= CCB_SKIP;			/* display anything. */
	} else {
		pCel->ccb_Flags |= CCB_BGND;			/* don't skip 0-valued pixels, */
		pCel->ccb_PRE0  |= PRE0_BGND;			/* really, trust me, don't skip them. */

		color &= 0x00007FFF;

		scaleMul = (opacityPct+12) / 13;		/* put 1-100% in range of 1-8 multiplier */

		if (scaleMul == 8) {
			scaledColor = color;
			pCel->ccb_PIXC = PIXC_OPAQUE; 			/* opaque PIXC for 100% opacity */
		} else {
			r = (color >> 10) & 0x1F;				/* isolate the color components. */
			g = (color >>  5) & 0x1F;
			b = (color >>  0) & 0x1F;

			r = (r * scaleMul) / 8;					/* scale color components to the inverse */
			g = (g * scaleMul) / 8;					/* of the scaling used for the current */
			b = (b * scaleMul) / 8;					/* frame buffer pixel. */

			scaledColor = (r<<10)|(g<<5)|(b<<0);	/* reassemble color components. */

			scaleMul = 8 - scaleMul;				/* inverse multiplier for CFDB scaling */

			pCel->ccb_PIXC = PIXCL_BLEND_8 | ((scaleMul - 1) << PPMPC_MF_SHIFT);
		}
	}

	/*------------------------------------------------------------------------
	 * Store the pre-scaled color pixel in the PLUT pointer in the CCB, and
	 * point the source data pointer to it.  This just saves memory; uncoded
	 * cels don't have a PLUT, so we use the PLUTPtr field as a little 4-byte
	 * pixel data buffer.  It doesn't matter that the pixel doesn't even
	 * slightly resemble a pointer, because the LDPLUT flag isn't set and
	 * thus the cel engine will never read the PLUTPtr field.  (Sneaky, huh?)
	 *----------------------------------------------------------------------*/

	pCel->ccb_PLUTPtr	= (void *)((scaledColor << 16));
	pCel->ccb_SourcePtr	= (CelData *)&pCel->ccb_PLUTPtr;

	return pCel;

}

