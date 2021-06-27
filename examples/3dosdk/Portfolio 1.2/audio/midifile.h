#pragma include_only_once
#ifndef __midifile_h
#define __midifile_h

/*
** MIDI File Parser Includes
** By:  Phil Burk
*/

/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
*/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "io.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "semaphore.h"
#include "stdarg.h"
#include "string.h"
#include "operamath.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "flex_stream.h"

#define NUMMIDICHANNELS (16)
#define NUMMIDIPROGRAMS (128)

#define CHKID(a,b,c,d) ((a<<24)|(b<<16)|(c<<8)|d)

typedef struct MIDIFileParser
{
	FlexStream mfp_FlexStream;
	int32   mfp_Format;
	int32   mfp_NumTracks;
	int32   mfp_TrackIndex;
	int32   mfp_Division;
	int32   mfp_Shift;          /* Scale times by shifting if needed. */
	int32   mfp_Tempo;
	frac16  mfp_Rate;           /* TicksPerSecond */
	int32   mfp_Time;           /* Current Time as we scan track. */
	int32   mfp_RunningStatus;  /* Current running status command byte */
	int32   mfp_NumData;        /* Number of data bytes for above command. */
	int32   mfp_BytesLeft;      /* Bytes left in current track */
	int32 (*mfp_HandleTrack)();  /* User function to handle Track */
	int32 (*mfp_HandleEvent)();  /* User function to handle Event */
	void   *mfp_UserData;       /* For whatever user wants. */
} MIDIFileParser;

typedef struct MIDIEvent
{
	uint32	mev_Time;
	unsigned char mev_Command;
	unsigned char mev_Data1;
	unsigned char mev_Data2;
	unsigned char mev_Data3;
} MIDIEvent;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

Err ParseMFEvent( MIDIFileParser *mfpptr );
Err ParseMFHeader( MIDIFileParser *mfpptr );
Err ParseMFMetaEvent( MIDIFileParser *mfpptr );
Err ParseMFTrack( MIDIFileParser *mfpptr );
Err ScanMFTrack( MIDIFileParser *mfpptr, int32 Size );
int32 GetMFChar( MIDIFileParser *mfpptr );
int32 MIDIDataBytesPer( int32 command );
int32 ParseMFVLN( MIDIFileParser *mfpptr );
int32 ReadMFTrackHeader( MIDIFileParser *mfpptr );

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif

