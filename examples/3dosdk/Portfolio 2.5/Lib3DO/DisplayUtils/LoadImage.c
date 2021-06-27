
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: LoadImage.c,v 1.15 1994/11/11 23:58:46 vertex Exp $
**
**  Lib3DO routine to allocate a buffer and load an image
**  into it, using fast block I/O.
**
******************************************************************************/


#include "types.h"
#include "mem.h"
#include "displayutils.h"
#include "blockfile.h"
#include "debug3do.h"
#include "string.h"

/*****************************************************************************
 * void *LoadImage(char *filename, void *dest,
 *				   VdlChunk **rawVDLPtr, ScreenContext *sc)
 *
 *	Loads an image from a 3DO file.  The pixels are loaded into a VRAM buffer
 *	which is allocated herein if not pre-allocated by the caller.  If the
 *	file contains a VDL chunk, and the rawVDLPtr parameter is non-NULL, the
 *	VDL chunk is loaded into an allocated DRAM buffer, and a pointer to that
 *	buffer is returned into *rawVDLPtr.  If the file contains an IMAG control
 *	chunk, the values in it are used, otherwise the pixel data is assumed to
 *	be a full screenful (320x240x16bit).  The pixel destination buffer is
 *	assumed to be the full size of a frame buffer as decscribed in the
 *	ScreenContext parm.  If the pixel data from the file isn't as large as
 *	the frame buffer, the frame buffer is zeroed first, then the pixels are
 *	loaded into it; no centering or other adjustments are made; the pixels
 *	are just loaded starting at the first line.
 *
 *	If you feed this routine a file that isn't an image file, it'll do its
 *	best to cope; if the file contains PDAT chunk(s), it'll try to
 *	load them using default IMAG control values.  If there are multiple
 *	pixel chunks, only the first one in the file is loaded.  If the data in
 *	the file is too far out of whack, the routine will fail, perhaps
 *	catastrophically.
 *
 *	The return value is a pointer to the destination pixel buffer, or NULL
 *	if the load failed.
 *
 *	NOTE:	If this routine allocates a VDL buffer for you and you want
 *			to free it later, use FreeMem(..., -1).
 *
 ****************************************************************************/


 void * LoadImage( char *name, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc )
{
	int32		filebufsize;
	uint32		chunk_ID;
	int32		tempSize;
	char*		tempBuf;
	char*		pChunk;
	Boolean		we_allocated_dest = FALSE;
	int32 		destbufsize = 0;
	int32 		vdlbufsize = 0;
	char*		vdlbuf = NULL;
	int32*		filebuf = NULL;
	ImageCC*	pIMAG = NULL;
	PixelChunk* pPDAT = NULL;
	VdlChunk*	pVDL = NULL;
	Item		VRAMIOReq = 0;

	destbufsize = sc->sc_NumBitmapBytes;

	if (dest == NULL) {
		dest = (ubyte *)AllocMem(destbufsize,MEMTYPE_TRACKSIZE | MEMTYPE_STARTPAGE | MEMTYPE_VRAM | sc->sc_BitmapBank);
		if (dest == NULL) {
			DIAGNOSE(("Can't allocate dest buffer for file %s\n", name));
			goto ERROR_EXIT;
		}
		we_allocated_dest = TRUE;
	}

	if (NULL == (filebuf = (int32 *)LoadFile(name, &filebufsize, MEMTYPE_ANY))) {
		DIAGNOSE(("Can't load file %s\n", name));
		goto ERROR_EXIT;
	}

	tempBuf  = (char *)filebuf;
	tempSize = filebufsize;

	while ( (pChunk = GetChunk( &chunk_ID, &tempBuf, &tempSize )) != NULL)	{
		switch (chunk_ID) {
			case CHUNK_IMAG:
				if (pIMAG == NULL) {
					pIMAG	= (ImageCC *)pChunk;
				}
				break;

			case CHUNK_PDAT:
				if (pPDAT == NULL) {
					pPDAT	= (PixelChunk *)pChunk;
				}
				break;

			case CHUNK_VDL:
				if (pVDL == NULL) {
					pVDL	= (VdlChunk *)pChunk;
				}
				break;

			case CHUNK_CPYR:
			case CHUNK_DESC:
			case CHUNK_KWRD:
			case CHUNK_CRDT:
			case CHUNK_XTRA:
				break;

			default:
				DIAGNOSE(("Unexpected chunk ID %.4s in file %s, ignored\n", pChunk, name));
				break;
		}
	}

	/* allocate some memory for the VDL and copy what we found in the file */

	if ( pVDL != NULL ) {
		if ( rawVDLPtr == NULL ) {
		   DIAGNOSE(("VDL chunk found in file %s, no way to return it, ignored\n", name));
		} else {
			vdlbufsize = pVDL->chunk_size;
			vdlbuf = (ubyte *)AllocMem(vdlbufsize, MEMTYPE_TRACKSIZE | MEMTYPE_ANY);
			if ( vdlbuf == NULL ) {
				DIAGNOSE(("Not enough memory to return copy of VDL for file %s\n", name));
				goto ERROR_EXIT;
			}
			memcpy(vdlbuf, pVDL, vdlbufsize);
			*rawVDLPtr = (VdlChunk *)vdlbuf;
		}
	 }

	/* figure out the size of the pixel data */

	if (pIMAG == NULL) {
		/*
		 * Correctly compute 320 * 240 buffer size.
		 */
		tempSize = ((240 + 1) >> 1) * 320 * sizeof (uint32);
	} else {
		tempSize = pIMAG->bytesperrow * pIMAG->h;
	}

	/* move or decompress the pixel data to the dest buffer */

	if (tempSize > destbufsize) {
		DIAGNOSE(("Image too big for destination buffer in file %s\n", name));
		goto ERROR_EXIT;
	} else if (tempSize < destbufsize) {
		VRAMIOReq = CreateVRAMIOReq();
		SetVRAMPages( VRAMIOReq, dest, 0, sc->sc_NumBitmapPages, ~0 );
		DeleteVRAMIOReq(VRAMIOReq);
	}

	if (pPDAT != NULL) {
		memcpy(dest, pPDAT->pixels, tempSize);
	} else {
		DIAGNOSE(("No pixel data chunk in image file %s\n", name));
		goto ERROR_EXIT;
	}

	UnloadFile(filebuf);

	return dest;

ERROR_EXIT:

	if (filebuf)
		UnloadFile(filebuf);

	if (vdlbuf && vdlbufsize)		/* vdlbufsize is non-zero if we allocated vdlbuf */
		FreeMem(vdlbuf,-1);

	if (dest && we_allocated_dest)		/* destbufsize is non-zero if we allocated dest */
		FreeMem(dest,-1);

	return NULL;
}

void UnloadImage(void *imagebuf)
{
    FreeMem(imagebuf,-1);
}
