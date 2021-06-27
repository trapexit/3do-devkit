#ifndef __SOUNDFILE_H
#define __SOUNDFILE_H

/****************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: soundfile.h,v 1.20 1994/10/24 21:21:44 peabody Exp $
**
**  Sound file player
**
**  By: Phil Burk
**
****************************************************************************/


#include "soundspooler.h"
#include "types.h"


/*
    SoundFilePlayer

    Unless otherwise noted, all fields are read-only.
*/

#define MAX_SOUNDFILE_BUFS (8)

typedef struct
{
	SoundSpooler *sfp_Spooler;
	char   *sfp_BufferAddrs[MAX_SOUNDFILE_BUFS];
	Item    sfp_FileItem;
	Item	sfp_IOReqItem;      /* For block reads. */

	Item    sfp_SamplerIns;     /* Instrument used to play samples.

	                               When SFP_NO_SAMPLER is not set, this item is set to
	                               an sample player instrument loaded by the sound file player.
	                               Client can access the sampler instrument to connect
	                               to it, twiddle knobs, etc.  Don't write to this
	                               field when SFP_NO_SAMPLER is not set.

	                               When SFP_NO_SAMPLER is set, the sound file player
	                               assumes that the client will write the item #
	                               of a sample player instrument here between calls
	                               to CreateSoundFilePlayer() and LoadSoundFile().
	                               If the sound spooler is responsible for the output
	                               instrument, it will connect the client-supplied sample
	                               instrument to the output instrument. */

	Item    sfp_OutputIns;      /* Instrument used to output sound.

	                               When SFP_NO_DIRECTOUT is not set, LoadSoundFile() loads
	                               directout.dsp and places its item number here. LoadSoundFile()
	                               also connects sfp_SamplerIns to sfp_OutputIns.  The
	                               sound file player is responsible for starting, stopping,
	                               and unloading the instrument in this case.  Don't touch it.

	                               When SFP_NO_DIRECTOUT is set, no output instrument is
	                               loaded, and sfp_SampleIns is not connected to anything.
	                               That becomes the client's responsibility. In this case,
	                               this field is set to 0. Don't touch it. */

	uint32  sfp_Cursor;         /* Position of file reader. */
	uint32  sfp_NumBuffers;     /* Number of Buffers in use. */
	uint32  sfp_BufIndex;       /* Next buffer to be loaded. */
	uint32  sfp_BufSize;        /* Size of the buffer in bytes. */
	int32   sfp_LastPos;        /* Byte position of DMA, updated periodically */
	uint32  sfp_NumToRead;      /* Number of bytes to read, total. */
	uint32  sfp_DataOffset;     /* Start of Data in file. */
	uint32  sfp_BuffersPlayed;  /* How many played so far. */
	uint32  sfp_BuffersToPlay;  /* How many total need to be played. */

	uint8	sfp_Flags;          /* User settable flags (SFP_). */

	uint8	sfp_PrivateFlags;   /* Private flags. Don't Touch! */
	uint32  sfp_BlockSize;      /* Block size of file. (V24) */
} SoundFilePlayer;


/* sfp_Flags - Flags used by SoundFile Player */
#define SFP_NO_SAMPLER   (0x01) /* If true, prevents LoadSoundFile() from loading an
                                   instrument to play sample into sfp_SamplerIns. */

#define SFP_NO_DIRECTOUT (0x02) /* If true, prevents LoadSoundFile() from loading directout.dsp
                                   into sfp_OutputIns and connecting sfp_SamplerIns to it. */

#define SFP_LOOP_FILE    (0x04) /* If true, loop entire file. (not implemented yet) */

/* sfp_PrivateFlags */
#define SFP_PRIV_OWN_BUFFERS  (0x01)    /* True if the Buffer memory is owned by this system. */
#define SFP_PRIV_IO_PENDING   (0x02)    /* Set if an IO request has been sent and not received. */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

SoundFilePlayer *CreateSoundFilePlayer ( int32 NumBuffers, int32 BufSize, void *Buffers[] );
SoundFilePlayer *OpenSoundFile (char *FileName, int32 NumBuffers, int32 BufSize );
int32 CloseSoundFile ( SoundFilePlayer *sfp );
int32 DeleteSoundFilePlayer ( SoundFilePlayer *sfp );
int32 LoadSoundFile (  SoundFilePlayer *sfp, char *FileName );
int32 ReadSoundFile( SoundFilePlayer *sfp, int32 Cursor, int32 BufIndex);
int32 RewindSoundFile ( SoundFilePlayer *sfp);
int32 ServiceSoundFile ( SoundFilePlayer *sfp, int32 SignalIn, int32 *SignalNeeded);
int32 StartSoundFile ( SoundFilePlayer *sfp , int32 Amplitude);
int32 StopSoundFile ( SoundFilePlayer *sfp );
int32 UnloadSoundFile ( SoundFilePlayer *sfp );

#ifdef __cplusplus
}
#endif /* __cplusplus */


/*****************************************************************************/


#endif /* __SOUNDFILE_H */
