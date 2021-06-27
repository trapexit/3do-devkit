/*****************************************************************************
 *	File:			LoadImage.c
 *
 *	Contains:		Routine to allocate a buffer and load an image
 *					into it, using fast block I/O.
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *  05/12/94  ian	Removed reference to GrafBase->gf_ZeroPage.  We now use
 *					sc->sc_Bitmaps[0].bm_Buffer in the GETBANKBITS() call.
 *	08/06/93  jml	Updated LoadImage with new SetVRamPages call for new OS
 *	07/25/93  ian	Now uses new faster LoadFile() block I/O instead of
 *					filestream I/O. Also, it now makes fewer assumptions
 *					about the validity and content of the data.
 *
 *	Implementation notes:
 *
 *	While it certainly appears on the face of it that copying the data
 *	from the input buffer to another allocated buffer is not efficient,
 *	that method is used (instead of parsing and returning the I/O buffer
 *	itself) in preparation for implementing data compression in the future,
 *	wherein it will be mandatory to decompress data into a new buffer.
 ****************************************************************************/

#include "Portfolio.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "BlockFile.h"
#include "UMemory.h"

#include "Debug3DO.h"

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
 *			to free it later, use Free().
 *
 ****************************************************************************/


 void * LoadImage( char *name, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc )
	{
	long		filebufsize;
	ulong		chunk_ID;
	long		tempSize;
	char*		tempBuf;
	char*		pChunk;
	Boolean		we_allocated_dest = FALSE;
	int 		destbufsize = 0;
	int 		vdlbufsize = 0;
	char*		vdlbuf = NULL;
	long*		filebuf = NULL;
	ImageCC*	pIMAG = NULL;
	PixelChunk* pPDAT = NULL;
	VdlChunk*	pVDL = NULL;
	static	Item	gVRAMIOItem = 0;

	destbufsize = (int)(sc->sc_nFrameBufferPages*GrafBase->gf_VRAMPageSize);

	if (dest == NULL) {
		dest = (ubyte *)Malloc(destbufsize,
			  GETBANKBITS( sc->sc_Bitmaps[0]->bm_Buffer )
			| MEMTYPE_STARTPAGE
			| MEMTYPE_VRAM
			| MEMTYPE_CEL);

		if (dest == NULL) {
			DIAGNOSE(("Can't allocate dest buffer for file %s\n", name));
			goto ERROR_EXIT;
		}
		we_allocated_dest = TRUE;
	}

	if (NULL == (filebuf = (long *)LoadFile(name, &filebufsize, MEMTYPE_ANY))) {
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

	// allocate some memory for the VDL and copy what we found in the file

	if ( pVDL != NULL ) {
		if ( rawVDLPtr == NULL ) {
		   DIAGNOSE(("VDL chunk found in file %s, no way to return it, ignored\n", name));
		} else {
			vdlbufsize = (int)pVDL->chunk_size;
			vdlbuf = (ubyte *)Malloc(vdlbufsize, MEMTYPE_ANY);
			if ( vdlbuf == NULL ) {
				DIAGNOSE(("Not enough memory to return copy of VDL for file %s\n", name));
				goto ERROR_EXIT;
			}
			memcpy(vdlbuf, pVDL, vdlbufsize);
			*rawVDLPtr = (VdlChunk *)vdlbuf;
		}
	 }

	// figure out the size of the pixel data

	if (pIMAG == NULL) {
		tempSize = 640 * 240;	// default image size
	} else {
		tempSize = pIMAG->bytesperrow * pIMAG->h;
	}

	// move or decompress the pixel data to the dest buffer

	if (tempSize > destbufsize) {
		DIAGNOSE(("Image too big for destination buffer in file %s\n", name));
		goto ERROR_EXIT;
	} else if (tempSize < destbufsize) {
		if (gVRAMIOItem == 0)
			gVRAMIOItem = GetVRAMIOReq();
		SetVRAMPages( gVRAMIOItem, dest, 0, sc->sc_nFrameBufferPages, -1 );
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

	if (vdlbuf && vdlbufsize)		// vdlbufsize is non-zero if we allocated vdlbuf
		Free(vdlbuf);

	if (dest && we_allocated_dest)		// destbufsize is non-zero if we allocated dest
		Free(dest);

	return NULL;
}

void UnloadImage(void *imagebuf)
{
	if (imagebuf)
		Free(imagebuf);
}
