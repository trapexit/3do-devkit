/*******************************************************************************************
 *	File:			MFToText.c
 *
 *	Contains:		This module sets up callback for the
 *MIDIFileParser which print out formatted text for each event that the parser
 *					supports.  It serves as an example of how to
 *use the parser.
 *
 *	Usage:			Err MIDIFileToText( FILE* fp );
 *
 *
 *	Written by:		Software Attic
 *					 with a few clever ideas courtesy of Phil
 *Burk
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	06/03/94	rdg		Fix sign extension problem in
 *KeySignature 06/02/94	rdg		Port back to MPW after debugging in
 *ThinkC Add callbacks for MIDI Real Time Msgs Re Name lots of things 06/01/94
 *rdg		re-format printf() statements for better readablilty; lots of
 *						gnarly re-casting, yuk.. isn't there a
 *better way? 06/01/94	rdg		fixed missing parameter to printf() in
 *txt_tempo() 05/23/94	rdg		new today
 *
 *******************************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "MFToText.h"
#include "MIDI_Defs.h"
#include "ParseMF.h"

/* Prototypes */
static void InitParserCallbacks (MIDIFileParserPtr mfp);

/* Callback functions to pass to the Midifile parser.
 *	See ParseMF.h for detail about each of the callbacks
 */
long ReadMIDIFile (void *buff, long count);
void HandleError (Err code, char *string);

void txt_Header (short mfType, short numTracks, short division);
void txt_StartTrack (long evTime);
void txt_EndTrack (long evTime);
void txt_NoteOn (long evTime, ChanMsgPtr chanMsg);
void txt_NoteOff (long evTime, ChanMsgPtr chanMsg);
void txt_Pressure (long evTime, ChanMsgPtr chanMsg);
void txt_Parameter (long evTime, ChanMsgPtr chanMsg);
void txt_Pitchbend (long evTime, ChanMsgPtr chanMsg);
void txt_Program (long evTime, ChanMsgPtr chanMsg);
void txt_ChanPressure (long evTime, ChanMsgPtr chanMsg);
void txt_Sysex (long evTime, uchar code, GenMsgPtr genMsg);
void txt_MEv_Misc (long evTime, GenMsgPtr genMsg);
void txt_MEv_SeqSpecific (long evTime, GenMsgPtr genMsg);
void txt_MEv_Text (long evTime, GenMsgPtr genMsg);
void txt_MEv_SeqNum (long evTime, long number);
void txt_MEv_EndOfTrack (long evTime);
void txt_MEv_KeySig (long evTime, GenMsgPtr genMsg);
void txt_MEv_SetTempo (long evTime, long tempo);
void txt_MEv_TimeSig (long evTime, GenMsgPtr genMsg);
void txt_MEv_SMPTE (long evTime, GenMsgPtr genMsg);
void txt_RT_Clock (long evTime);
void txt_RT_Start (long evTime);
void txt_RT_Continue (long evTime);
void txt_RT_Stop (long evTime);
void txt_RT_ActiveSense (long evTime);

/* The file we need to read from */
static FILE *myMidiFilePtr;

/* Main routine to parse the midifile with callbacks to simply dump
 * a text description of file events
 */
Err
MIDIFileToText (FILE *fp)
{
  Err result;

  MIDIFileParser mfp;

  /* Did we get something to work with? */
  if (fp == NULL)
    {
      result = MFTTErrNullFilePtr;
      goto ERR_EXIT;
    }

  /* Now we know where to get data from */
  myMidiFilePtr = fp;

  /* Initalize the parser context */
  InitParserCallbacks (&mfp);

  /* Initalize the parser */
  result = InitMIDIFileParser (&mfp);
  if (result < 0)
    goto ERR_EXIT;

  /* Parse the file */
  result = ParseMIDIFile (&mfp);
  if (result < 0)
    printf ("MIDIFile Parser Error #%ld: trying to clean up!\n", (long)result);

  /* Give the parser a chance to clean up after itself */
  TermMIDIFileParser (&mfp);

ERR_EXIT:
  return result;
}

/* IMPORTANT- You must set the callbacks you don't want to use
 * to NULL or bad things will happen.
 */
static void
InitParserCallbacks (MIDIFileParserPtr mfp)
{
  /* Callbacks */
  mfp->cb_ReadFile = ReadMIDIFile;
  mfp->cb_HandleError = HandleError;

  mfp->cb_MFHeader = txt_Header;
  mfp->cb_StartTrack = txt_StartTrack;
  mfp->cb_EndTrack = txt_EndTrack;
  mfp->cb_NoteOn = txt_NoteOn;
  mfp->cb_NoteOff = txt_NoteOff;
  mfp->cb_Pressure = txt_Pressure;
  mfp->cb_Controller = txt_Parameter;
  mfp->cb_Pitchbend = txt_Pitchbend;
  mfp->cb_ProgramChange = txt_Program;
  mfp->cb_ChanPressure = txt_ChanPressure;
  mfp->cb_Sysex = txt_Sysex;
  mfp->cb_MEv_Misc = txt_MEv_Misc;
  mfp->cb_MEv_SeqNum = txt_MEv_SeqNum;
  mfp->cb_MEv_EndOfTrack = txt_MEv_EndOfTrack;
  mfp->cb_MEv_TimeSig = txt_MEv_TimeSig;
  mfp->cb_MEv_SMPTE = txt_MEv_SMPTE;
  mfp->cb_MEv_SetTempo = txt_MEv_SetTempo;
  mfp->cb_MEv_KeySig = txt_MEv_KeySig;
  mfp->cb_MEv_SeqSpecific = txt_MEv_SeqSpecific;
  mfp->cb_MEv_Text = txt_MEv_Text;
  mfp->cb_RT_Clock = txt_RT_Clock;
  mfp->cb_RT_Start = txt_RT_Start;
  mfp->cb_RT_Continue = txt_RT_Continue;
  mfp->cb_RT_Stop = txt_RT_Stop;
  mfp->cb_RT_ActiveSense = txt_RT_ActiveSense;
}

long
ReadMIDIFile (void *buff, long count)
{
  /* Try to get the data from the file */
  return fread (buff, 1, count, myMidiFilePtr);
}

void
HandleError (Err code, char *string)
{
  fprintf (stdout, "\n\nError Code = %ld; Error Text: %s\n\n", code, string);
  fflush (stdout);
}

void
txt_Header (short mfType, short numTracks, short division)
{
  printf ("\nHeader: Format = %d; Number of Tracks = %d; Division = %d\n",
          mfType, numTracks, division);
  fflush (stdout);
}

void
txt_StartTrack (long evTime)
{
  printf ("\nTrack start: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_EndTrack (long evTime)
{
  printf ("\nTrack end: evTime = %ld\n", evTime);
  fflush (stdout);
}

/* chanMsg->data1 = pitch, data2 = velocity */
void
txt_NoteOn (long evTime, ChanMsgPtr chanMsg)
{
  printf (
      "\nNote On: evTime = %ld; Channel = %lu; Pitch = %lu; Velocity = %lu\n",
      evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1,
      (ulong)chanMsg->data2);
  fflush (stdout);
}

/* chanMsg->data1 = pitch, data2 = velocity */
void
txt_NoteOff (long evTime, ChanMsgPtr chanMsg)
{
  printf (
      "\nNote Off: evTime = %ld; Channel = %lu; Pitch = %lu; Velocity = %lu\n",
      evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1,
      (ulong)chanMsg->data2);
  fflush (stdout);
}

/* chanMsg->data1 = pitch, data2 = pressure */
void
txt_Pressure (long evTime, ChanMsgPtr chanMsg)
{
  printf (
      "\nPressure: evTime = %ld; Channel = %lu; Pitch = %lu; Pressure = %lu\n",
      evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1,
      (ulong)chanMsg->data2);
  fflush (stdout);
}

/* chanMsg->data1 = control, data2 = value */
void
txt_Parameter (long evTime, ChanMsgPtr chanMsg)
{
  printf (
      "\nParameter: evTime = %ld; Channel = %lu; Control = %lu; Value = %lu\n",
      evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1,
      (ulong)chanMsg->data2);
  fflush (stdout);
}

/* chanMsg->data1 = MSB, data2 = LSB */
void
txt_Pitchbend (long evTime, ChanMsgPtr chanMsg)
{
  printf ("\nPitchbend: evTime = %ld; Channel = %lu; MSB = %lu; LSB = %lu\n",
          evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1,
          (ulong)chanMsg->data2);
  fflush (stdout);
}

/* chanMsg->data1 = Program Number, data2 = 0 */
void
txt_Program (long evTime, ChanMsgPtr chanMsg)
{
  printf ("\nProgram: evTime = %ld; Channel = %lu; Program = %lu\n", evTime,
          (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1);
  fflush (stdout);
}

/* chanMsg->data1 = Pressure, data2 = 0 */
void
txt_ChanPressure (long evTime, ChanMsgPtr chanMsg)
{
  printf ("\nChannel Pressure: evTime = %ld; Channel = %lu; Pressure = %lu\n",
          evTime, (ulong)(chanMsg->chan + 1), (ulong)chanMsg->data1);
  fflush (stdout);
}

void
txt_Sysex (long evTime, uchar code, GenMsgPtr genMsg)
{
  printf ("\nSysex: evTime = %ld; Type = %lx, Length = %ld\n", evTime,
          (ulong)code, genMsg->length);
  fflush (stdout);
}

void
txt_MEv_Misc (long evTime, GenMsgPtr genMsg)
{
  printf (
      "\nUnrecognized Meta event: evTime = %d; Type = 0x%02lx; Length = %ld\n",
      evTime, (ulong)genMsg->type, genMsg->length);
  fflush (stdout);
}

void
txt_MEv_SeqSpecific (long evTime, GenMsgPtr genMsg)
{
  printf ("\nMeta event - Sequencer Specific: evTime = %d; Type = 0x%02lx "
          "Length = %ld\n",
          evTime, (ulong)genMsg->type, genMsg->length);
  fflush (stdout);
}

void
txt_MEv_Text (long evTime, GenMsgPtr genMsg)
{
  static char *ttype[] = { NULL,
                           METATEXT,      /* type=0x01 */
                           METACOPYRIGHT, /* type=0x02 */
                           METASEQUENCE,  /* ...       */
                           METAINSTRUMENT,
                           METALYRIC,
                           METAMARKER,
                           METACUE, /* type=0x07 */
                           METAUNRECOGNIZED };

  /* Get index of unrecognized text event */
  short unrecognized = 8;

  /* For readability */
  long time = evTime;
  short type = genMsg->type;
  short length = genMsg->length;

  uchar *msg = genMsg->msg;

  if (type < 1 || type >= unrecognized)
    type = unrecognized;

  printf ("\nMeta Text Event: evTime = %ld; Type = 0x%02x (%s); Length = %d\n",
          time, type, ttype[type], length);

  printf ("  Text = < ");
  printf (" %s ", msg);
  printf (" >\n");

  fflush (stdout);
}

void
txt_MEv_SeqNum (long evTime, long number)
{
  printf ("\nMeta Event: evTime = %ld, Sequence Number = %ld\n", evTime,
          number);
  fflush (stdout);
}

void
txt_MEv_EndOfTrack (long evTime)
{
  printf ("\nMeta Event, End of Track: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_MEv_KeySig (long evTime, GenMsgPtr genMsg)
{
  long key = 0;

  /* For some wierd reason, you can't cast directly from a uchar to
   * a long and get the proper sign extension.  Thus this wierdness
   */
  key = (long)((char)genMsg->msg[0]);

  printf ("\nKey Signature: evTime = %ld; Sharp/Flats = %ld;  Minor = %ld\n",
          evTime, key, (long)genMsg->msg[1]);
  fflush (stdout);
}

void
txt_MEv_SetTempo (long evTime, long tempo)
{
  printf (
      "\nTempo: evTime = %ld; Tempo in usecs-Per-MIDI-Quarter-Note = %ld\n",
      evTime, tempo);
  fflush (stdout);
}

void
txt_MEv_TimeSig (long evTime, GenMsgPtr genMsg)
{
  ulong num = (ulong)genMsg->msg[0];
  ulong den = (ulong)genMsg->msg[1];
  ulong clk = (ulong)genMsg->msg[2];
  ulong bts = (ulong)genMsg->msg[3];

  /* Deal with that stupid negative power of two denominator */
  den = (2 << (den - 1));

  printf ("\nTime Signature: evTime = %ld; TS = %lu/%lu\n", evTime, num, den);

  printf ("    MIDI-clocks/click = %lu; 32nd-notes/24-MIDI-clocks = %lu\n",
          clk, bts);

  fflush (stdout);
}

void
txt_MEv_SMPTE (long evTime, GenMsgPtr genMsg)
{
  ulong hr = (ulong)genMsg->msg[0];
  ulong mn = (ulong)genMsg->msg[1];
  ulong se = (ulong)genMsg->msg[2];
  ulong fr = (ulong)genMsg->msg[3];
  ulong ff = (ulong)genMsg->msg[4];

  printf ("\nSMPTE: Hour = %lu; Minute = %lu; Second = %lu; Frame = %lu; "
          "Fract-Frame = %lu\n",
          hr, mn, se, fr, ff);
  fflush (stdout);
}

void
txt_RT_Clock (long evTime)
{
  printf ("\nReal Time Msg - Clock: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_RT_Start (long evTime)
{
  printf ("\nReal Time Msg - Start: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_RT_Continue (long evTime)
{
  printf ("\nReal Time Msg - Continue: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_RT_Stop (long evTime)
{
  printf ("\nReal Time Msg - Stop: evTime = %ld\n", evTime);
  fflush (stdout);
}

void
txt_RT_ActiveSense (long evTime)
{
  printf ("\nReal Time Msg - ActiveSense: evTime = %ld\n", evTime);
  fflush (stdout);
}
