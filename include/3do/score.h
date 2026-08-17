#pragma once

/****************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: score.h,v 1.33 1994/10/25 00:15:04 phil Exp $
 **
 **  Score player
 **
 ****************************************************************************/

#include "extern_c.h"

#include "juggler.h"        /* Sequence, Collection */
#include "list.h"
#include "midifile.h"       /* NUMMIDICHANNELS, MIDIEvent, MIDIFileParser */
#include "operamath.h"      /* frac16 */
#include "types.h"

#define SCORE_MAX_VOICES    16
#define MAXSCOREVOICES      SCORE_MAX_VOICES
#define SCORE_MIN_PRIORITY  0       /* min/max for pimap entry priorities */
#define SCORE_MAX_PRIORITY  200

typedef struct PIMapEntry
{
  Item  pimp_InsTemplate;
  u8 pimp_Priority;
  u8 pimp_MaxVoices;
  u8 pimp_RateDivide;
  u8 pimp_Reserved3;
} PIMapEntry;

#define NTTR_FLAG_INUSE    (0x01)  /* In channel list */
#define NTTR_FLAG_NOTEON   (0x02)
#define NTTR_FLAG_FREE     (0x04)  /* In free list */

typedef struct NoteTracker
{
  Node  nttr_Node;
  s8  nttr_Note;
  s8  nttr_MixerChannel;
  u8 nttr_Flags;
  s8  nttr_Channel;           /* MIDI */
  Item  nttr_Instrument;
} NoteTracker;

/* One for each of the 16 MIDI Channels. */
typedef struct ScoreChannel
{
  s8   schn_DefaultProgram;   /* MIDI Programs */
  s8   schn_CurrentProgram;
  u8  schn_Priority;
  u8  schn_NumVoices;
  s32  schn_Volume;           /* 14 bit volume set by control 7 */
  s32  schn_Pan;              /* 14 bit pan set by control 10 */
  s32  schn_LeftVolume;       /* calculated from Volume, Pan, Max */
  s32  schn_RightVolume;      /* calculated from Volume, Pan, Max */
  List   schn_NoteList;
  frac16 schn_PitchBend;        /* in MIDI units, 0x2000 is no bend. */
} ScoreChannel;

#define SCON_FLAG_VERBOSE    (0x01)   /* If set, warn when no program defined. */
#define SCON_FLAG_USE_INSPRI (0x02)   /* Use priority from PIMap. */
#define SCON_FLAG_NO_TEMPLATE_OK (0x04) /* Continue execution if no template match. */

/* Global context for score to be played in. */
typedef struct ScoreContext
{
  u8         scon_PIMapSize; /* Number of Entries in PIMap */
  u8         scon_MaxVoices;
  u8         scon_Flags;
  u8         scon_Reserved3;
  PIMapEntry   *scon_PIMap;
  s32 	scon_MaxVolume; /* Per Voice */
  Item          scon_MixerIns;
  Item          scon_LeftGains[MAXSCOREVOICES]; /* Gain Knobs */
  Item          scon_RightGains[MAXSCOREVOICES];
  NoteTracker  *scon_NoteTrackers;
  List          scon_FreeNoteTrackers;
  ScoreChannel  scon_Channels[NUMMIDICHANNELS];
  s32         scon_BendRange; /* in semitones, used to interpret MIDI bend */
  s32	(*scon_PurgeHook)(u8 Priority, s32 MaxActivity); /* Called when desparate. */
} ScoreContext;

EXTERN_C_BEGIN

ScoreContext *CreateScoreContext(s32 MaxNumPrograms);
char *SelectSamplePlayer(Item Sample , s32 IfVariable);
Err ChangeScoreControl(ScoreContext *scon, s32 Channel, s32 Index, s32 Value);
Err ChangeScoreProgram(ScoreContext *ScoreCon, s32 Channel, s32 ProgramNum);
Err ChangeScorePitchBend(ScoreContext *scon, s32 Channel, s32 Bend);
Err ConvertPitchBend(s32 Bend, s32 SemitoneRange, frac16 *BendFractionPtr);
Err DeleteScoreContext(ScoreContext *scon);
Err DisableScoreMessages(s32 Flag);
Err InitScoreDynamics ( ScoreContext *scon, s32 MaxScoreVoices);
Err InitScoreMixer(ScoreContext *scon, char *MixerName, s32 MaxNumVoices, s32 Amplitude);
Err InterpretMIDIEvent(Sequence *SeqPtr, MIDIEvent *MEvCur, ScoreContext *scon);
Err InterpretMIDIMessage(ScoreContext *ScoreCon, char *MIDIMsg, s32 IfMute);
Err LoadPIMap(ScoreContext *scon, char *FileName);
Err MFDefineCollection (MIDIFileParser *mfpptr, char *Image, s32 NumBytes, Collection *ColPtr);
Err MFLoadCollection(MIDIFileParser *mfpptr, char *filename, Collection *ColPtr);
Err MFLoadSequence(MIDIFileParser *mfpptr, char *filename, Sequence *SeqPtr);
Err MFUnloadCollection(Collection *ColPtr);
Err NoteOffIns(Item Instrument, s32 Note, s32 Velocity);
Err NoteOnIns(Item Instrument, s32 Note, s32 Velocity);
Err ReleaseScoreNote(ScoreContext *scon, s32 Channel, s32 Note, s32 Velocity);
Err SetPIMapEntry(ScoreContext *scon, s32 ProgramNum, Item InsTemplate, s32 MaxVoices, s32 Priority);
Err StartScoreNote(ScoreContext *scon, s32 Channel, s32 Note, s32 Velocity);
Err StopScoreNote(ScoreContext *scon, s32 Channel, s32 Note);
Err TermScoreMixer(ScoreContext *ScoreCon);
Err UnloadPIMap(ScoreContext *scon);
Err FreeChannelInstruments(ScoreContext *scon, s32 Channel);

s32 PurgeScoreInstrument(ScoreContext *scon, u8 Priority, s32 MaxLevel);

Err SetScoreBendRange(ScoreContext *scon, s32 BendRange);
s32 GetScoreBendRange(ScoreContext *scon);

EXTERN_C_END
