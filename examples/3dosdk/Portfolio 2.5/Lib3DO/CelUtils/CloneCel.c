
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: CloneCel.c,v 1.4 1994/11/02 00:25:05 vertex Exp $
**
**  Lib3DO routine to create a new cel that's a clone of an existing cel.
**
**  DeleteCel() compatible.
**
**  CloneCel() allocates a new CCB and copies the specified source CCB fields
**  into it.  It sets the CCB_LAST flag on and NULLs the ccb_NextPtr field
**  in the new CCB.  Other modifications to the new CCB depend on the options
**  specified in the call.
**
**  The CCB_ONLY option means that no other changes are made to the CCB,
**  resulting in the cloned CCB referring to the same pixel data and
**  PLUT as the original.
**
**  The COPY_PLUT option will allocate a new PLUT, store the new PLUT pointer
**  into the new CCB, and copy the contents of the source cel's PLUT into the
**  new PLUT.  It is safe to specify this option for an uncoded cel.  If the
**  source cel's PlutPtr field is NULL, no attempt is made to copy PLUT data.
**
**  The COPY_PIXELS option will allocate a new pixel data buffer, store that
**  buffer's pointer into the new CCB, and copy the contents of the source
**  cel's pixel data buffer into the new buffer.
**
**  The COPY_PLUT and COPY_PIXELS options can be ORed together.
**
******************************************************************************/


#include "types.h"
#include "mem.h"
#include "celutils.h"
#include "deletecelmagic.h"
#include "debug3do.h"
#include "string.h"

/*----------------------------------------------------------------------------
 * CloneCel()
 *	Create a new cel based on an existing cel.
 *--------------------------------------------------------------------------*/

CCB * CloneCel(CCB *src, int32 options)
{
	int32	allocExtra;
	void *	dataBuf;
	CCB *	cel	= NULL;

	allocExtra = ((options & CLONECEL_COPY_PLUT) ? sizeof(uint16[32]) : 0L);

	cel = AllocMagicCel_(allocExtra, DELETECELMAGIC_CCB_ONLY, NULL, NULL);
	if (cel == NULL) {
		DIAGNOSE_SYSERR(NOMEM, ("Can't allocate cel\n"));
		goto ERROR_EXIT;
	}

	memcpy(cel, src, sizeof(CCB));
	cel->ccb_Flags |= CCB_LAST;
	cel->ccb_NextPtr = NULL;

	if ((options & CLONECEL_COPY_PLUT) && src->ccb_PLUTPtr != NULL) {
		cel->ccb_PLUTPtr = cel+1;
		memcpy(cel->ccb_PLUTPtr, src->ccb_PLUTPtr, sizeof(uint16[32]));
	}

	if (options & CLONECEL_COPY_PIXELS) {
		allocExtra = GetCelDataBufferSize(src);
		if ((dataBuf = AllocMem(allocExtra, MEMTYPE_TRACKSIZE | MEMTYPE_CEL)) == NULL) {
			DIAGNOSE_SYSERR(NOMEM, ("Can't allocate cel data buffer\n"));
			goto ERROR_EXIT;
		}
		ModifyMagicCel_(cel, DELETECELMAGIC_CCB_AND_DATA, dataBuf, NULL);
		cel->ccb_SourcePtr = (CelData *)dataBuf;
		memcpy(cel->ccb_SourcePtr, src->ccb_SourcePtr, allocExtra);
	}

	return cel;

ERROR_EXIT:

	return DeleteCel(cel);
}

