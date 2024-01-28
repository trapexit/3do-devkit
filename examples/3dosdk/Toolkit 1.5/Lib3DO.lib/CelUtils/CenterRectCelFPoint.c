/*****************************************************************************
 *	File:			CenterRectCelFPoint.c
 *
 *	Contains:		Center a rectangular cel.
 *
 *	Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/27/94  Ian	Split into several modules to keep code size down.
 *	07/23/94  Ian	Performance tweaks: more inline calcs instead of
 *calling other library routines to convert-and-calc.
 *	03/02/94  Ian	Added AA and List versions of all CenterRectCel
 *functions. 02/27/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	These routines center a rectangular cel based on its current projection
 *	size, as opposed to the source data size.  For example, if you've got a
 *	1x1 pixel cel projected to display as a solid-color 20x20 rectangle,
 *then CenterRectCelOnScreen() will center the 20x20 projection on the screen.
 *	The old CenterCelOnScreen() routine would remap the cel to a 1x1 pixel
 *	projection, and center that on the screen.  (Not very useful, usually.)
 *
 *	These routines assume that the cel is rectangular.  If you pass in a
 *cel that's being projected to a non-rectangular quad, the results of the
 *	centering are quite likely to be bogus.
 *
 *	The list versions of these function center the first cel in the list,
 *	then offset the rest of the cels such that they maintain the same
 *	relationship they originally had to the first cel.  IE, they do not
 *	center each cel in the list.
 *
 *	Divide-by-two expressions herein are coded as ">> 1" expressions.  This
 *	is because the compiler generates painfully-correct code for signed
 *	divide-by-two via shifting, and we know that the rounding difference
 *	between signed divide and signed shift makes no difference in our
 *context.
 ****************************************************************************/

#include "CelUtils.h"
#include "string.h"

#define COORDINATESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_XPos))
#define PERSPECTIVESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_HDX))

/*----------------------------------------------------------------------------
 * CenterRectCelOverFPoint()
 *	Center a cel over the specified point.
 *--------------------------------------------------------------------------*/

void
CenterRectCelOverFPoint (CCB *cel, FPoint *point)
{
  frac16 width;
  frac16 height;

  width = MulSF16 ((cel->ccb_HDX >> 4), Convert32_F16 (cel->ccb_Width));
  height = MulSF16 ((cel->ccb_VDY), Convert32_F16 (cel->ccb_Height));

  cel->ccb_XPos = SubF16 (point->x, DivSF16 (width, Convert32_F16 (2)));
  cel->ccb_YPos = SubF16 (point->y, DivSF16 (height, Convert32_F16 (2)));
}

/*----------------------------------------------------------------------------
 * CenterRectAACelOverFPoint()
 *	Center an anti-aliased cel over the specified point.  The AA cel's mask
 *	and data CCBs are both set to the same location.
 *--------------------------------------------------------------------------*/

void
CenterRectAACelOverFPoint (CCB *cel1, FPoint *point)
{
  CCB *cel2;

  CenterRectCelOverFPoint (cel1, point);

  if (!IS_LASTCEL (cel1))
    {
      cel2 = CEL_NEXTPTR (cel1);
      memcpy (&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
    }
}

/*----------------------------------------------------------------------------
 * CenterRectCelListOverFPoint()
 *	Center a list of cels over the specified point.  The first cel is
 *	centered, then all other cels' locations are modified so that they
 *	maintain the same relative location to the first cel that they had
 *	on entry.
 *--------------------------------------------------------------------------*/

void
CenterRectCelListOverFPoint (CCB *list, FPoint *point)
{
  FPoint delta;

  delta.x = list->ccb_XPos; // capture starting position
  delta.y = list->ccb_YPos;

  CenterRectCelOverFPoint (list, point); // go center first cel

  delta.x = SubF16 (list->ccb_XPos, delta.x); // calc delta from captured
  delta.y = SubF16 (list->ccb_YPos, delta.y); // starting position

  if (!IS_LASTCEL (list))
    {
      OffsetCelListByFDelta (CEL_NEXTPTR (list), &delta, FALSE);
    }
}

/*----------------------------------------------------------------------------
 * CenterRectCelInDisplay()
 *	Center a cel at the center point of the display.
 *--------------------------------------------------------------------------*/

void
CenterRectCelInDisplay (CCB *cel)
{
  CenterRectCelOverFPoint (cel, CenterFPointInDisplay ());
}

/*----------------------------------------------------------------------------
 * CenterRectAACelInDisplay()
 *	Center an anti-aliased cel at the center point of the display.  The AA
 *	cel's mask and data CCBs are both set to the same location.
 *--------------------------------------------------------------------------*/

void
CenterRectAACelInDisplay (CCB *cel)
{
  CenterRectAACelOverFPoint (cel, CenterFPointInDisplay ());
}

/*----------------------------------------------------------------------------
 * CenterRectCelListInDisplay()
 *	Center a list of cels at the center point of the display.  The first
 *cel is centered, then all other cels' locations are modified so that they
 *	maintain the same relative location to the first cel that they had
 *	on entry.
 *--------------------------------------------------------------------------*/

void
CenterRectCelListInDisplay (CCB *cel)
{
  CenterRectCelListOverFPoint (cel, CenterFPointInDisplay ());
}
