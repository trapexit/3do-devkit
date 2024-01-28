/*****************************************************************************
 *	File:			MapCelToRect.c
 *
 *	Contains:		Map a cel to the given rectangle.
 *
 *	Copyright (c) 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	07/23/94  Ian	Performance tweaks: more inline calcs instead of
 *calling other library routines to convert-and-calc. 02/26/94  Ian 	New
 *today.
 *
 *	Implementation notes:
 *
 *	The MapCel() function uses a lot of CPU cycles to set a CCB's size and
 *	perspective fields to project into an arbitrary quad.  When you want to
 *	map a cel to rectangle, there's no need to do the expensive perspective
 *	calculations, you only need to set HDX and VDY; that's what these do.
 *
 *	Negative values for parameters are allowed.  Negative width and/or
 *height results in mirror-image projections of the cel.
 ****************************************************************************/

#include "CelUtils.h"
#include "string.h"

#define COORDINATESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_XPos))
#define PERSPECTIVESIZE (offsetof (CCB, ccb_PIXC) - offsetof (CCB, ccb_HDX))

/*----------------------------------------------------------------------------
 * MapCelToSRect()
 *	Map a cel to the given rectangle.
 *--------------------------------------------------------------------------*/

void
MapCelToSRect (CCB *cel, SRect *rect)
{
  cel->ccb_XPos = Convert32_F16 (rect->pos.x);
  cel->ccb_YPos = Convert32_F16 (rect->pos.y);
  cel->ccb_HDX
      = DivSF16 (Convert32_F16 (rect->size.x), Convert32_F16 (cel->ccb_Width))
        << 4;
  cel->ccb_VDY = DivSF16 (Convert32_F16 (rect->size.y),
                          Convert32_F16 (cel->ccb_Height));
}

/*----------------------------------------------------------------------------
 * MapCelToCRect()
 *	Map a cel to the given rectangle.
 *--------------------------------------------------------------------------*/

void
MapCelToCRect (CCB *cel, CRect *rect)
{
  cel->ccb_XPos = Convert32_F16 (rect->tl.x);
  cel->ccb_YPos = Convert32_F16 (rect->tl.y);
  cel->ccb_HDX = DivSF16 (Convert32_F16 (XSIZEFROMCRECT (rect)),
                          Convert32_F16 (cel->ccb_Width))
                 << 4;
  cel->ccb_VDY = DivSF16 (Convert32_F16 (YSIZEFROMCRECT (rect)),
                          Convert32_F16 (cel->ccb_Height));
}

/*------------------------------------------------------------------------------
 * MapAACelToSRect()
 *	Perform a MapCelToSRect() operation on an anti-aliased cel.  An AA cel
 *is presumed to be two CCBs linked together, both having the same XY
 *coordinates to begin with.  The perspective info is  copied from the first to
 *second cel. This will work if called on a single cel that has no links, so
 *that you can easily toggle between using AA and non-AA cels if you want.
 *----------------------------------------------------------------------------*/

void
MapAACelToSRect (CCB *cel1, SRect *rect)
{
  CCB *cel2;

  MapCelToSRect (cel1, rect);

  if (!IS_LASTCEL (cel1))
    {
      cel2 = CEL_NEXTPTR (cel1);
      memcpy (&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
    }
}

/*------------------------------------------------------------------------------
 * MapAACelToCRect()
 *	Perform a MapCelToCRect() operation on an anti-aliased cel.  An AA cel
 *is presumed to be two CCBs linked together, both having the same XY
 *coordinates to begin with.  The perspective info is copied from the first to
 *second cel. This will work if called on a single cel that has no links, so
 *that you can easily toggle between using AA and non-AA cels if you want.
 *----------------------------------------------------------------------------*/

void
MapAACelToCRect (CCB *cel1, CRect *rect)
{
  CCB *cel2;

  MapCelToCRect (cel1, rect);

  if (!IS_LASTCEL (cel1))
    {
      cel2 = CEL_NEXTPTR (cel1);
      memcpy (&cel2->ccb_XPos, &cel1->ccb_XPos, COORDINATESIZE);
    }
}

/*----------------------------------------------------------------------------
 * MapCelListToSRect()
 *	Perform a MapCelToSRect() operation for all cels in a list.  We
 *MapCelToSRect() the first cel.  Then for all other cels in the list, the CCB
 *perspective info fields (HDX thru HDDY) from the first cel are copied to each
 *cel in the list, and the XY coordinates of each cel in the list are modified
 *so that the cels maintain the same relative position as they had to the first
 *cel.
 *--------------------------------------------------------------------------*/

void
MapCelListToSRect (CCB *list, SRect *rect)
{
  FPoint delta;

  delta.x = SubF16 (Convert32_F16 (rect->pos.x), list->ccb_XPos);
  delta.y = SubF16 (Convert32_F16 (rect->pos.y), list->ccb_YPos);

  MapCelToSRect (list, rect);

  if (!IS_LASTCEL (list))
    {
      OffsetCelListByFDelta (CEL_NEXTPTR (list), &delta, TRUE);
    }
}

/*----------------------------------------------------------------------------
 * MapCelListToCRect()
 *	Perform a MapCelToCRect() operation for all cels in a list.  We
 *MapCelToCRect() the first cel.  Then for all other cels in the list, the CCB
 *perspective info fields (HDX thru HDDY) from the first cel are copied to each
 *cel in the list, and the XY coordinates of each cel in the list are modified
 *so that the cels maintain the same relative position as they had to the first
 *cel.
 *--------------------------------------------------------------------------*/

void
MapCelListToCRect (CCB *list, CRect *rect)
{
  FPoint delta;

  delta.x = SubF16 (Convert32_F16 (rect->tl.x), list->ccb_XPos);
  delta.y = SubF16 (Convert32_F16 (rect->tl.y), list->ccb_YPos);

  MapCelToCRect (list, rect);

  if (!IS_LASTCEL (list))
    {
      OffsetCelListByFDelta (CEL_NEXTPTR (list), &delta, TRUE);
    }
}
