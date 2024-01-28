
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: MapCelToPoint.c,v 1.2 1994/10/05 18:41:08 vertex Exp $
**
**  Lib3DO routines to perform SetXY ("Map To Point") operations.
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

#define COORDINATESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_XPos))
#define PERSPECTIVESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_HDX))

/*------------------------------------------------------------------------------
 * MapCelToIPoint()
 *	Simple set cel XY coords.
 *----------------------------------------------------------------------------*/

void
MapCelToIPoint (CCB *cel, IPoint *newPos)
{
  cel->ccb_XPos = Convert32_F16 (newPos->x);
  cel->ccb_YPos = Convert32_F16 (newPos->y);
}

/*------------------------------------------------------------------------------
 * MapCelToFPoint()
 *	Simple set cel XY coords.
 *----------------------------------------------------------------------------*/

void
MapCelToFPoint (CCB *cel, FPoint *newPos)
{
  cel->ccb_XPos = newPos->x;
  cel->ccb_YPos = newPos->y;
}

/*------------------------------------------------------------------------------
 * MapAACelToIPoint()
 *	Set the XY coordinates for an anti-aliased cel.
 *----------------------------------------------------------------------------*/

void
MapAACelToIPoint (CCB *cel1, IPoint *newPos)
{
  CCB *cel2;

  cel1->ccb_XPos = Convert32_F16 (newPos->x);
  cel1->ccb_YPos = Convert32_F16 (newPos->y);

  if (!IS_LASTCEL (cel1))
    {
      cel2 = CEL_NEXTPTR (cel1);
      memcpy (&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
    }
}

/*------------------------------------------------------------------------------
 * MapAACelToFPoint()
 *	Set the XY coordinates for an anti-aliased cel.
 *----------------------------------------------------------------------------*/

void
MapAACelToFPoint (CCB *cel1, FPoint *newPos)
{
  CCB *cel2;

  cel1->ccb_XPos = newPos->x;
  cel1->ccb_YPos = newPos->y;

  if (!IS_LASTCEL (cel1))
    {
      cel2 = CEL_NEXTPTR (cel1);
      memcpy (&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
    }
}

/*------------------------------------------------------------------------------
 * MapCelListToIPoint()
 *	Set the XY coordinates for first cel in a list, then modify the XY
 *	coordinates for all other cels in the list so that they maintain the
 *	same relative position they had to the first cel.  Optionally, the
 *	perspective (HDX thru HDDY) can be propagated to all cels in the list.
 *----------------------------------------------------------------------------*/

void
MapCelListToIPoint (CCB *list, IPoint *newPos, Boolean copyPerspective)
{
  FPoint delta;

  delta.x = SubF16 (Convert32_F16 (newPos->x), list->ccb_XPos);
  delta.y = SubF16 (Convert32_F16 (newPos->y), list->ccb_YPos);

  OffsetCelListByFDelta (list, &delta, copyPerspective);
}

/*------------------------------------------------------------------------------
 * MapCelListToFPoint()
 *	Set the XY coordinates for first cel in a list, then modify the XY
 *	coordinates for all other cels in the list so that they maintain the
 *	same relative position they had to the first cel.  Optionally, the
 *	perspective (HDX thru HDDY) can be propagated to all cels in the list.
 *----------------------------------------------------------------------------*/

void
MapCelListToFPoint (CCB *list, FPoint *newPos, Boolean copyPerspective)
{
  FPoint delta;

  delta.x = SubF16 (newPos->x, list->ccb_XPos);
  delta.y = SubF16 (newPos->y, list->ccb_YPos);

  OffsetCelListByFDelta (list, &delta, copyPerspective);
}
