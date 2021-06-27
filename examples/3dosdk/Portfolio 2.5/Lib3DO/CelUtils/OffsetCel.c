
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: OffsetCel.c,v 1.5 1994/10/05 18:48:14 vertex Exp $
**
**  Lib3DO routines to apply delta values to cel XY coordinates.
**
******************************************************************************/


#include "celutils.h"
#include "string.h"

#define COORDINATESIZE  (offsetof(CCB, ccb_PIXC) - offsetof(CCB, ccb_XPos))
#define PERSPECTIVESIZE (offsetof(CCB, ccb_PIXC) - offsetof(CCB, ccb_HDX))

static IPoint zero_delta;	/* let compiler init this to zeroes in bss seg */

/*------------------------------------------------------------------------------
 * OffsetCel()
 *	The original Lib3DO routine.
 *----------------------------------------------------------------------------*/

void OffsetCel(CCB *cel, int32 deltaX, int32 deltaY)
{
	cel->ccb_XPos = AddF16(cel->ccb_XPos, Convert32_F16(deltaX));
	cel->ccb_YPos = AddF16(cel->ccb_YPos, Convert32_F16(deltaY));
}

/*------------------------------------------------------------------------------
 * OffsetCelByFDelta()
 *----------------------------------------------------------------------------*/

void OffsetCelByFDelta(CCB *cel, FPoint *delta)
{
	cel->ccb_XPos = AddF16(cel->ccb_XPos, delta->x);
	cel->ccb_YPos = AddF16(cel->ccb_YPos, delta->y);
}

/*------------------------------------------------------------------------------
 * OffsetCelByIDelta()
 *----------------------------------------------------------------------------*/

void OffsetCelByIDelta(CCB *cel, IPoint *delta)
{
	cel->ccb_XPos = AddF16(cel->ccb_XPos, Convert32_F16(delta->x));
	cel->ccb_YPos = AddF16(cel->ccb_YPos, Convert32_F16(delta->y));
}

/*------------------------------------------------------------------------------
 * OffsetAACelByFDelta()
 *----------------------------------------------------------------------------*/

void OffsetAACelByFDelta(CCB *cel1, FPoint *delta)
{
	CCB * cel2;

	cel1->ccb_XPos = AddF16(cel1->ccb_XPos, delta->x);
	cel1->ccb_YPos = AddF16(cel1->ccb_YPos, delta->y);

	if (!IS_LASTCEL(cel1)) {
		cel2 = CEL_NEXTPTR(cel1);
		memcpy(&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
	}
}

/*------------------------------------------------------------------------------
 * OffsetAACelByIDelta()
 *----------------------------------------------------------------------------*/

void OffsetAACelByIDelta(CCB *cel1, IPoint *delta)
{
	CCB * cel2;

	cel1->ccb_XPos = AddF16(cel1->ccb_XPos, Convert32_F16(delta->x));
	cel1->ccb_YPos = AddF16(cel1->ccb_YPos, Convert32_F16(delta->y));

	if (!IS_LASTCEL(cel1)) {
		cel2 = CEL_NEXTPTR(cel1);
		memcpy(&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
	}
}

/*------------------------------------------------------------------------------
 * OffsetCelListByFDelta()
 *	Adjust the XY coords for each cel in a list by the given deltas.  If the
 *	copyPerspective flag is set, also copy the perspective fields from the
 *	first cel in the list to each of the other cels.
 *----------------------------------------------------------------------------*/

void OffsetCelListByFDelta(CCB *list, FPoint *delta, Boolean copyPerspective)
{
	CCB *	cur;

	if (delta == NULL) {
		delta = (FPoint *)&zero_delta;
	}

	for (cur = list; cur != NULL; cur = CEL_NEXTPTR(cur)) {
		cur->ccb_XPos = AddF16(cur->ccb_XPos, delta->x);
		cur->ccb_YPos = AddF16(cur->ccb_YPos, delta->y);
		if (copyPerspective) {
			memcpy(&cur->ccb_HDX, &list->ccb_HDX, PERSPECTIVESIZE);
		}
		if (cur->ccb_Flags & CCB_LAST) {
			break;
		}
	}
}

/*------------------------------------------------------------------------------
 * OffsetCelListByIDelta()
 *	Adjust the XY coords for each cel in a list by the given deltas.  If the
 *	copyPerspective flag is set, also copy the perspective fields from the
 *	first cel in the list to each of the other cels.
 *----------------------------------------------------------------------------*/

void OffsetCelListByIDelta(CCB *list, IPoint *delta, Boolean copyPerspective)
{
	CCB *	cur;

	if (delta == NULL) {
		delta = &zero_delta;
	}

	for (cur = list; cur != NULL; cur = CEL_NEXTPTR(cur)) {
		cur->ccb_XPos = AddF16(cur->ccb_XPos, Convert32_F16(delta->x));
		cur->ccb_YPos = AddF16(cur->ccb_YPos, Convert32_F16(delta->y));
		if (copyPerspective) {
			memcpy(&cur->ccb_HDX, &list->ccb_HDX, PERSPECTIVESIZE);
		}
		if (cur->ccb_Flags & CCB_LAST) {
			break;
		}
	}
}
