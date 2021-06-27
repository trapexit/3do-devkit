/*****************************************************************************
 *	File:			CreateCel.c
 *
 *	Contains:		Routine to create a cel using the specified attributes.
 *					The cel will have a data buffer and PLUT (if needed)
 *					attached to it.  If you don't specify a data buffer pointer,
 *					an appropriately-sized buffer is allocated for you.
 *
 *					If you need a simple solid-color cel and the pixels in it 
 *					will never change, the CreateBackdropCel() function will
 *					create a cel that uses less memory.
 *
 *	Copyright:	(c) 1994 The 3DO Company. 	All Rights Reserved.
 *
 *	History:
 *	07/23/94  Ian	Convert some mul/div ops to shifts for performance.
 *	03/03/94  Ian	Converted to the new DeleteCelMagic system.
 *	02/26/94  Ian	The preamble word 1 UNCLSB flag (PRE1_TLLSB_PDC0) is now
 *					set for all cels, coded and uncoded.  The manual turns out
 *					to be wrong on this: it definitely applies to both types
 *					of cels, not just uncoded.
 *	02/25/94  Ian	Cels only get a PLUT allocated now if they need it.  If
 *					one is needed, it is allocated along with the CCB 
 *					structure allocation instead of with a separate Malloc call.
 *	02/24/94  Ian 	New today.
 *
 *	Implementation notes:
 *
 *	DeleteCel() compatible.
 *
 *	After creating a cel, you can bash the CCB in any way you want.  Feel 
 *	free to change the flags, the contents of the PLUT and/or data buffer,
 *	attach a different PLUT and/or data buffer to the cel, etc.
 *
 ****************************************************************************/

#include "CelUtils.h"
#include "DeleteCelMagic.h"
#include "Umemory.h"
#include "Debug3DO.h"
#include "operror.h"

/*----------------------------------------------------------------------------
 * CreateCel()
 *	Create a basic cel of a pretty much standard type.  Useful as a starting
 *	point for creating any type of cel.
 *--------------------------------------------------------------------------*/

CCB * CreateCel(int32 w, int32 h, int32 bpp, int32 options, void *databuf)
{
	int32		allocExtra;
	int32		bufferBytes;
	CCB *		cel = NULL;
	
	/*------------------------------------------------------------------------
	 * validate parameters.
	 *----------------------------------------------------------------------*/

#if DEBUG
	if (w <= 0 || h <= 0) {
		DIAGNOSE(("Width (%ld) and Height (%ld) must be greater than zero\n", w, h));
		goto ERROR_EXIT;
	}
	
	if (w > 2047) {
		DIAGNOSE(("Width (%ld) exceeds cel engine limit of 2047\n", w));
		goto ERROR_EXIT;
	}
#endif

	/*------------------------------------------------------------------------
	 * create and init the cel.
	 *----------------------------------------------------------------------*/

	if (bpp < 8) {					// only 8 and 16 bit cels can be uncoded, for
		options = CREATECEL_CODED;	// anything less, force coded flag on.
	}

	allocExtra = (options & CREATECEL_CODED) ? sizeof(uint16[32]) : 0L;
		
	if ((cel = AllocMagicCel_(allocExtra, DELETECELMAGIC_CCB_ONLY, NULL, NULL)) == NULL) {
		DIAGNOSE_SYSERR(NOMEM, ("Can't allocate cel\n"));
		goto ERROR_EXIT;
	}
	
	bufferBytes = InitCel(cel, w, h, bpp, options);
	
	cel->ccb_PLUTPtr = (options & CREATECEL_CODED) ? (void *)(cel + 1) : NULL;

	/*------------------------------------------------------------------------
	 * create the cel data buffer, if the caller didn't supply one.
	 *----------------------------------------------------------------------*/

	if (databuf == NULL) {
		if ((databuf = Malloc(bufferBytes, MEMTYPE_CEL|MEMTYPE_FILL|0)) == NULL) {
			DIAGNOSE_SYSERR(NOMEM, ("Can't allocate cel data buffer\n"));
			goto ERROR_EXIT;
		}
		ModifyMagicCel_(cel, DELETECELMAGIC_CCB_AND_DATA, databuf, NULL);
	}
	
	cel->ccb_SourcePtr = (CelData *)databuf;
	
	return cel;

ERROR_EXIT:

	return DeleteCel(cel);
}
