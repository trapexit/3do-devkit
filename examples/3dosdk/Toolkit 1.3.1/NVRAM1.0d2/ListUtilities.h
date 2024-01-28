/*
        File:		ListUtilities.h

        Contains:	The list handler: utilitarian functions. The list
   handler code is NOT dependent on anything in this file. It is intended as
   programming aids only.

        Written by:	Edward Harp

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <1+>	  2/8/94	RLC		Add
   LHCreateCellFromCel() routine for building list cells from cels. 11/19/93
   EGH		Began implementation.

        To Do:
*/

#ifndef _ListUtilities_h
#define _ListUtilities_h

#ifndef _ListHandler_h
#include "ListHandler.h"
#endif

Err LHCreateTiledCell (ListHandler *listP, ListCell **cellP, int32 width,
                       int32 height);

Err LHCreateCellFromCel (ListHandler *listP, ListCell **cellP, CCB *ccb);
Err LHSetCellCCBs (ListCell *cellP, CCB *ccb, CCB *hiliteccb);

#endif
