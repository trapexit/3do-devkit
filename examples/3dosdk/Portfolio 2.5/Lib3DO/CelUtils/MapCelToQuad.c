
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: MapCelToQuad.c,v 1.2 1994/10/05 18:42:06 vertex Exp $
**
**  Lib3DO routines to perform MapCel() operations.
**
**  In these routines, an AA cel is presumed to be two CCBs linked together,
**  both having the same XY coordinates to begin with.  For operations on AA
**  cels, the perspective info (HDX thru HDDY) is also copied from the first
**  to the second cel, if a second cel exists. The AA calls will work if called
**  on a single cel that has no links, allowing you to easily toggle between
**  using AA and non-AA cels if you want.
**
**  The list-oriented functions will also work on both AA and non-AA cels.
**  They even work on lists containing a mixture of both types of cels.  The
**  main difference between the list-oriented calls and AA-oriented calls is
**  that the AA calls will only affect the immediately-following CCB (if it
**  exists) but won't walk any further down the list.
**
******************************************************************************/


#include "celutils.h"
#include "string.h"

#define COORDINATESIZE  (offsetof(CCB, ccb_PIXC) - offsetof(CCB, ccb_XPos))
#define PERSPECTIVESIZE (offsetof(CCB, ccb_PIXC) - offsetof(CCB, ccb_HDX))

/*------------------------------------------------------------------------------
 * MapCelToCQuad()
 *	A standard MapCel() call, but it takes a CQuad* instead of Point[] parm.
 *----------------------------------------------------------------------------*/

void MapCelToCQuad(CCB *cel, CQuad *quad)
{
	MapCel(cel, (Point *)quad);
}

/*------------------------------------------------------------------------------
 * MapAACelTo CQuad()
 *	Perform a MapCel() operation on an anti-aliased cel.
 *----------------------------------------------------------------------------*/

void MapAACelToCQuad(CCB *cel1, CQuad *quad)
{
	CCB *	cel2;

	MapCel(cel1, (Point *)quad);

	if (!IS_LASTCEL(cel1)) {
		cel2 = CEL_NEXTPTR(cel1);
		memcpy(&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
	}
}

/*------------------------------------------------------------------------------
 * MapCelListToCQuad()
 *	Perform a MapCel() operation for all cels in a list.  The MapCel() is done
 *	on the first cel.  Then for all other cels in the list, the CCB perspective
 *	fields (HDX thru HDDY) from the first cel are copied to each cel in the
 *	list, and the XY coordinates of each cel in the list are modified so that
 *	the cels maintain the same relative position as they had to the first cel.
 *----------------------------------------------------------------------------*/

void MapCelListToCQuad(CCB *list, CQuad *quad)
{
	FPoint	delta;

	delta.x = SubF16(Convert32_F16(quad->tl.x), list->ccb_XPos);
	delta.y = SubF16(Convert32_F16(quad->tl.y), list->ccb_YPos);

	MapCel(list, (Point *)quad);

	if (!IS_LASTCEL(list)) {
		OffsetCelListByFDelta(CEL_NEXTPTR(list), &delta, TRUE);
	}
}
