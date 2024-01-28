/*******************************************************************************************
 *	File:			ParseMF.h
 *
 *	Contains:		Interface to module which parses Standard
 *MIDIFiles and calls-back for events in the file.
 *
 *	Usage:			Err	ParseMidiFile( MIDIFileParserPtr mfp );
 *
 *  IMPORTANT: 		READ THE CALLBACK DOCUMENTATION AT THE END OF THIS
 *FILE!!!!!!!
 *
 *	Written by:		Software Attic
 *					 with a few clever ideas courtesy of Phil
 *Burk
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	06/02/94	rdg		Add callbacks for MIDI Real Time Msgs
 *						Buttloads of documentation
 *	05/31/94	rdg		Be more careful of signed vs unsigned
 *bytes. 05/23/94	rdg		NewToday
 *
 *******************************************************************************************/

#ifndef __PARSEMF_H__
#define __PARSEMF_H__

#include "types.h"

typedef long Err;
typedef unsigned char uchar;
typedef unsigned long ulong;

/* Needed these wierd casts for ThinkC */
#define CHKID(a, b, c, d)                                                     \
  ((((ulong)a) << 24) | (((ulong)b) << 16) | (((ulong)c) << 8) | ((ulong)d))

typedef enum PMFErrorCodes
{
  PMFErrNotInitalized = -1000,     /* The parser was not properly initalized */
  PMFErrNoReadfileCallback = -999, /* PMF() called with no way to get data */
  PMFErrEOF = -998,                /* Premature EOF */
  PMFErrWrongChunkID = -997,       /* Didn't find expected chunk ID */
  PMFErrBadHeaderSize = -996,      /* Header size was not = 6 bytes */
  PMFErrBadHeader = -995,          /* Aborted after trying to parse header */
  PMFErrNoTracks = -994,           /* File contains no tracks */
  PMFErrBadTrack = -993,           /* Aborted after trying to parse a track */
  PMFErrBadTrackHeader = -992,     /* Couldn't find a valid track header */
  PMFErrInsufficientMemory = -991  /* Unable to allocate RAM for operation */
};

/* A MIDI Channel Msg */
typedef struct ChanMsg
{
  uchar chan;   /* Channel number of current chan msg */
  uchar status; /* Status for this msg */
  uchar data1;  /* Data byte for this msg */
  uchar data2;  /* Data byte for this msg */
} ChanMsg, *ChanMsgPtr;

/* A generic MIDI (non-Channel) Msg */
typedef struct GenMsg
{
  uchar type;  /* The msg type */
  uchar *msg;  /* Pointer to a variable length msg */
  long length; /* Length of a the msg */
} GenMsg, *GenMsgPtr;

/* Callback context block for the MIDIFile parser */
typedef struct MIDIFileParser
{
  /* Parser context variables
   *
   * ¥¥THESE ARE PRIVATE!!!!¥¥
   */
  long bytesLeftInChunk; /* Bytes left in current chunk */

  short mfType;    /* Format type of MIDIFile */
  short numTracks; /* Number of tracks in MIDIFile */
  short division;  /* Time division */
  long tempo;      /* Tempo of MIDI data */
  short seqNum;    /* Number of current track/sequence */

  long currEvTime; /* Absolute time of current event */

  ChanMsg chanMsg; /* a MIDI Channel Msg */
  GenMsg genMsg;   /* a non-Channel Msg */

  /* Callback functions for event processing.
   *
   * ¥¥THESE ARE PUBLIC!!!!¥¥
   *
   * IMPORTANT: See the notes at the end of this
   * header file for information about each
   * of the callbacks
   */

  /* Read data from the MIDIFile */
  long (*cb_ReadFile) (void *buff, long count);

  /* MIDIFile header was encountered */
  void (*cb_MFHeader) (short mfType, short numTracks, short division);

  /* A track is about to be parsed */
  void (*cb_StartTrack) (long evTime);

  /* A track was parsed */
  void (*cb_EndTrack) (long evTime);

  /* Note On */
  void (*cb_NoteOn) (long evTime, ChanMsgPtr chanMsg);

  /* Note Off */
  void (*cb_NoteOff) (long evTime, ChanMsgPtr chanMsg);

  /* Pressure event */
  void (*cb_Pressure) (long evTime, ChanMsgPtr chanMsg);

  /* Controller event */
  void (*cb_Controller) (long evTime, ChanMsgPtr chanMsg);

  /* Pitchbend event */
  void (*cb_Pitchbend) (long evTime, ChanMsgPtr chanMsg);

  /* Program Change event */
  void (*cb_ProgramChange) (long evTime, ChanMsgPtr chanMsg);

  /* Channel Pressure event */
  void (*cb_ChanPressure) (long evTime, ChanMsgPtr chanMsg);

  /* Sysex event */
  void (*cb_Sysex) (long evTime, uchar code, GenMsgPtr genMsg);

  /* Misc/unrecognized Meta Event */
  void (*cb_MEv_Misc) (long evTime, GenMsgPtr genMsg);

  /* Sequencer Specific Meta Event */
  void (*cb_MEv_SeqSpecific) (long evTime, GenMsgPtr genMsg);

  /* Sequence Number Meta Event */
  void (*cb_MEv_SeqNum) (long evTime, long number);

  /* SMPTE Track Start Time Meta Event */
  void (*cb_MEv_SMPTE) (long evTime, GenMsgPtr genMsg);

  /* Time Signature Change Meta Event */
  void (*cb_MEv_TimeSig) (long evTime, GenMsgPtr genMsg);

  /* Key Signature Change Meta Event */
  void (*cb_MEv_KeySig) (long evTime, GenMsgPtr genMsg);

  /* Set Tempo Meta Event */
  void (*cb_MEv_SetTempo) (long evTime, long tempo);

  /* Some Text Meta Event */
  void (*cb_MEv_Text) (long evTime, GenMsgPtr genMsg);

  /* End Of Track Meta Event */
  void (*cb_MEv_EndOfTrack) (long evTime);

  /* Real Time Clock Event */
  void (*cb_RT_Clock) (long evTime);

  /* Real Time Start Event */
  void (*cb_RT_Start) (long evTime);

  /* Real Time Continue Event */
  void (*cb_RT_Continue) (long evTime);

  /* Real Time Stop Event */
  void (*cb_RT_Stop) (long evTime);

  /* Real Time Active Sensing Event */
  void (*cb_RT_ActiveSense) (long evTime);

  /* Handle and error condition */
  void (*cb_HandleError) (Err code, char *string);

} MIDIFileParser, *MIDIFileParserPtr;

/* Main parser routines */
Err InitMIDIFileParser (MIDIFileParserPtr mfp);
Err ParseMIDIFile (MIDIFileParserPtr mfp);
Err TermMIDIFileParser (MIDIFileParserPtr mfp);

#endif /* __PARSEMF_H__ */

/* -----------------   Callback Documentation   ----------------
 *
 * OVERVIEW:
 *
 * The ParseMIDIFile() function expects that you will have initalized
 * the callback portion of the parser structure properly.
 * Any callback set to NULL will simply be skipped in the process of
 * of parsing the file.
 * If it is set to a non-NULL invalid address however, Bad Bad things
 * will happen.
 *
 * The callback routines (if non-NULL) are invoked as each event in
 * the MIDIFile is parsed.  The majority of the callbacks have data
 * associated with them and so the comments below explain some
 * of the funkyness of the MIDI data.
 *
 * ¥Callback: cb_Sysex()
 *
 *  Comments:
 *   This is called whenever a sysex event is parsed. Both F0 and F7
 *   messages are passed to this routine when they are encountered.  You
 *	 are passed the byte so you can do whatever you want with the
 *distinction.
 *
 * ¥Callback: cb_MEv_SeqNum()
 *
 *  Comments:
 *
 *   The value represents the number of the track which is
 *   about to follow.  Format 0 and 1 MIDIFiles only contain one sequence.
 *   This value only applies to format 2 MIDIFiles where it is used to
 *   uniquely identify each MIDI pattern in the file.
 *
 * ¥Callback: cb_MEv_SMPTE()
 *
 *  Track start time in SMPTE format
 *
 *  Relavant Data:
 *   5 bytes of data are passed in genMsg.msg
 *
 *   hours, minutes, seconds, frames, fractional-frames
 *
 * ¥Callback: cb_MEv_TimeSig()
 *
 *  Time signature change.
 *
 *  Relavant Data:
 *   4 bytes of data are passed in genMsg.msg
 *
 *   The four bytes of the msg are in the format:
 *   1) Numerator
 *   2) Denominator
 *   3) MIDI Clocks per metronome click
 *   4) number of notated 32nd notes in a MIDI quarter note
 *    (a MIDI Quarter Note is 24 MIDI Clocks)
 *
 *   The Denominator is for some reason expressed as "negative power
 *   of two".  In other words, 2 = a quarter note, 3 = an eighth note.
 *   Basically you take the number 2 and shift it left by
 *   the MIDIFile's denominator value - 1. Fun Fun...
 *
 * ¥Callback: cb_MEv_KeySig();
 *
 *  Key Signature change.
 *
 *   Relavant Data:
 *    The two bytes of the msg are in the format:
 *    1) Number of sharps/flats where:
 *       -5 = 5 flats
 *        0 = Key of C
 *        4 = 4 sharps
 *    2) Minor or major
 *       0 = Major Key
 *       1 = Minor Key
 *
 * ¥Callback: cb_MEv_SetTempo();
 *
 *  Set tempo event.
 *
 *  Relavant Data:
 *   mfp->tempo
 *
 *   Value is usecs per quarter note.
 *
 * ¥Callback: cb_MEv_Text();
 *
 *  Text Meta Event.
 *
 *  Relavant Data:
 *   message type, message length, and a pointer to the msg data are passed in
 *   genMsg
 *
 *  Comments:
 *   The string in the MIDIFile doesn't contain a null terminator
 *   so the parser adds one for you to make life a little easier.
 *
 * ¥Callback: cb_RT_Clock();
 * ¥Callback: cb_RT_Start();
 * ¥Callback: cb_RT_Continue();
 * ¥Callback: cb_RT_Stop();
 * ¥Callback: cb_RT_ActiveSense();
 *
 *  Comments:
 *   These are unlikeley to be found in MIDIFiles but it's possible.
 *
 *   None of the real time msgs have data bytes associated with them so
 *   the callbacks have only the event time updated.
 *
 */
