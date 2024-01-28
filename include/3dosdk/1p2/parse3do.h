/*****************************************************************************
 *	File:		Parse3DO.h
 *
 *	Contains:	Datatypes and prototypes for handling Cel, Image, Anim, and
 *				Sound data stored in standard 3DO file formats.
 *
 *	Written by: Neil Cormia
 *
 *	Copyright:	© 1992 by The 3DO Company. All rights reserved.
 *				This material constitutes confidential and proprietary
 *				information of the 3DO Company and shall not be used by
 *				any Person or for any purpose except as expressly
 *				authorized in writing by the 3DO Company.
 *
 *	Change History (most recent first):
 *
 *		 <9>	 8/6/93		JML 	Changed parameters to LoadAnim and ParseAnim.
 *		 <8>	 7/29/93	ian 	Added UnloadImage(), UnloadCel(), UnloadAnim().
 *									Changed LoadCel() and LoadAnim() to OldLoadCel()
 *									and OldLoadAnim(), then added new versions of them.
 *									Added ParseCel() and ParseAnim().
 *		 <7>	 1/28/93	dsm 	Dynamically allocate frame entries in ANIM structure.
 *		 <6>	  1/3/93	dsm 	Magneto5 Update
 *		 <5>	  1/2/93	pgb 	upgrade ifdefs to Rel_Magneto4. Also declared
 *									FreeSoundEffects(), and FindChunk(), GetChunk() as externals...
 *		 <4>	12/23/92	pgb 	tidier LoadSoundFX, LoadSoundEffect, FreeSoundEffect routines
 *		 <3>	12/10/92	JML 	Changed LoadImage to handle VDLs better.
 *		 <1>	 12/9/92	JML 	first checked in
 *
 *	To Do:
 ****************************************************************************/

#pragma force_top_level
#pragma include_only_once

#ifndef __Parse3DO_h
#define __Parse3DO_h

#include "operamath.h"
#include "init3do.h"
#include "form3do.h"

#define ANIM_MULTI_CCB	0
#define ANIM_SINGLE_CCB 1

/* The block size to use for file system reads	   */
/* used within ReadFIle 	*/

#define FS_CHUNKSIZE 65536

/*----------------------------------------------------------------------------
 * datatypes
 *--------------------------------------------------------------------------*/

typedef struct tag_AnimFrame {
	CCB 		*af_CCB;		/* Pointer to CCB for this frame */
	char		*af_PLUT;		/* Pointer to PLUT for this frame */
	char		*af_pix;		/* Pointer to pixels for this frame */
	int32		reserved;
} AnimFrame;

typedef struct tag_ANIM {
	long		num_Frames; /* greatest number of PDATs or CCBs in file */
	frac16		cur_Frame;	/* allows fractional values for smooth speed */
	long		num_Alloced_Frames;
	AnimFrame	*pentries;
	void		*dataBuffer; /* for internal use by LoadAnim/UnloadAnim only! */
} ANIM;

typedef struct tag_SoundInfo {
	Item		iSoundEffect;
	Item		iSoundData;
	Item		iSoundTemplate;
	Item		iSoundAttachment;
	struct tag_SoundInfo	*next;
} SoundInfo;


/* The Animframes within an anim are allocated incrementally */
/* Initially a block large enough to hold N_FRAMES_PER_CHUNK frames is */
/* allocated.  If this is exceeded while reading the ANIM then a block with space for */
/* N_FRAMES_PER_CHUNK additional AnimFrames is allocated, the previously computed Animframes */
/* are copied into this new space, and the old full AniFrame buffer space is deallocated */

#define N_FRAMES_PER_CHUNK 16

/*----------------------------------------------------------------------------
 * prototypes
 *--------------------------------------------------------------------------*/


#ifdef __cplusplus 
extern "C" {
#endif

extern int		GetFileSize (char *fn);
extern int		ReadFile (char *filename, int32 size, long *buffer, long offset);

extern ANIM *	LoadAnim(char *filename, uint32 memTypeBits);
extern void 	UnloadAnim(ANIM *anim);

extern CCB *	LoadCel (char *filename, uint32 memTypeBits);
extern void 	UnloadCel(CCB *cel);

extern void *	LoadImage( char *filename, ubyte* dest, VdlChunk **rawVDLPtr, ScreenContext *sc );
extern void 	UnloadImage(void *imagebuf);

extern CCB *	OldLoadCel (char *name, long **buffer);
extern int		OldLoadAnim (char *name, long **buffer, ANIM *pAnim);
extern void 	OldFreeAnimFrames(ANIM *pAnim);

extern Item 	LoadSoundEffect(char* sFilename, int nNumVoices);

extern Item 	LoadSoundFX(char* sFilename, int nNumVoices, SoundInfo *theSoundInfo);
extern void 	FreeSoundEffects(void);

// service routines used internally and also generally useful...

extern CCB *	ParseCel(void *inBuf, long inBufSize);
extern ANIM *	ParseAnim(void *inBuf, long inBufSize, uint32 memTypeBits);

extern char *	FindChunk( ulong chunk_ID, char **buffer, long *bufLen );
extern char *	GetChunk( ulong *chunk_ID, char **buffer, long *bufLen );

#ifdef __cplusplus
}
#endif



#endif
