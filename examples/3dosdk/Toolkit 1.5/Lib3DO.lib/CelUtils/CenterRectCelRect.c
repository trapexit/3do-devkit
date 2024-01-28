/*****************************************************************************
 *	File:			CenterRectCelRect.c
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
 * CenterRectCelInSRect()
 *	Center a cel in/over the specified rect.
 *--------------------------------------------------------------------------*/

void
CenterRectCelInSRect (CCB *cel, SRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->pos.x + (rect->size.x >> 1));
  center.y = Convert32_F16 (rect->pos.y + (rect->size.y >> 1));

  CenterRectCelOverFPoint (cel, &center);
}

/*----------------------------------------------------------------------------
 * CenterRectCelInCRect()
 *	Center a cel in/over the specified rect.
 *--------------------------------------------------------------------------*/

void
CenterRectCelInCRect (CCB *cel, CRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->tl.x + (XSIZEFROMCRECT (rect) >> 1));
  center.y = Convert32_F16 (rect->tl.y + (YSIZEFROMCRECT (rect) >> 1));

  CenterRectCelOverFPoint (cel, &center);
}

/*----------------------------------------------------------------------------
 * CenterRectAACelInSRect()
 *	Center an anti-aliased cel in/over the specified rect.  The AA cel's
 *mask and data CCBs are both set to the same location.
 *--------------------------------------------------------------------------*/

void
CenterRectAACelInSRect (CCB *cel, SRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->pos.x + (rect->size.x >> 1));
  center.y = Convert32_F16 (rect->pos.y + (rect->size.y >> 1));

  CenterRectAACelOverFPoint (cel, &center);
}

/*----------------------------------------------------------------------------
 * CenterRectAACelInCRect()
 *	Center an anti-aliased cel in/over the specified rect.  The AA cel's
 *mask and data CCBs are both set to the same location.
 *--------------------------------------------------------------------------*/

void
CenterRectAACelInCRect (CCB *cel, CRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->tl.x + (XSIZEFROMCRECT (rect) >> 1));
  center.y = Convert32_F16 (rect->tl.y + (YSIZEFROMCRECT (rect) >> 1));

  CenterRectAACelOverFPoint (cel, &center);
}

/*----------------------------------------------------------------------------
 * CenterRectCelListInSRect()
 *	Center a list of cels in/over the specified rect.  The first cel is
 *	centered, then all other cels' locations are modified so that they
 *	maintain the same relative location to the first cel that they had
 *	on entry.
 *--------------------------------------------------------------------------*/

void
CenterRectCelListInSRect (CCB *cel, SRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->pos.x + (rect->size.x >> 1));
  center.y = Convert32_F16 (rect->pos.y + (rect->size.y >> 1));

  CenterRectCelListOverFPoint (cel, &center);
}

/*----------------------------------------------------------------------------
 * CenterRectCelListInCRect()
 *	Center a list of cels in/over the specified rect.  The first cel is
 *	centered, then all other cels' locations are modified so that they
 *	maintain the same relative location to the first cel that they had
 *	on entry.
 *--------------------------------------------------------------------------*/

void
CenterRectCelListInCRect (CCB *cel, CRect *rect)
{
  FPoint center;

  center.x = Convert32_F16 (rect->tl.x + (XSIZEFROMCRECT (rect) >> 1));
  center.y = Convert32_F16 (rect->tl.y + (YSIZEFROMCRECT (rect) >> 1));

  CenterRectCelListOverFPoint (cel, &center);
}
