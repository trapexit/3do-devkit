/*******************************************************************************************
 *	File:			MIDI_Defs.h
 *
 *	Contains:		Defines for MIDI event parsing.
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
 *	06/02/94	rdg		Added additional #defines for MIDI Meta
 *Events 05/26/94	rdg		NewToday
 *
 *******************************************************************************************/

#ifndef __MIDI_DEFS_H__
#define __MIDI_DEFS_H__

/* Byte codes for MIDI Channel Messages */
#define NOTEOFF 0x80
#define NOTEON 0x90
#define PRESSURE 0xA0
#define CONTROLLER 0xB0
#define PROGRAM 0xC0
#define CHANPRESSURE 0xD0
#define PITCHBEND 0xE0

/* Sysex Events */
#define SYSEX_BEGIN 0xF0
#define SYSEX_CONTINUE 0xF7

/* Byte codes for MIDI Real Time Messages
 * These are unlikely to appear in a MIDIFile
 * but it is theoretically possible to capture
 * these events so I enclude them anyway.
 */
#define MRT_CLOCK 0xF8
#define MRT_START 0xFA
#define MRT_CONTINUE 0xFB
#define MRT_STOP 0xFC
#define MRT_ACTIVE_SENSE 0xFE

/* This is defined as a RealTime event
 * but it conflicts with the META event
 * code so I leave it commented out.
 * Who knows what it would do if it
 * were in a MIDIFile anyway?
 *
 * #define	MRT_SYSTEM_RESET	0xFF
 */

/* Byte codes for MIDI Meta Events */
#define META_EVENT 0xFF

/* Meta Event Types */
#define ME_SEQUENCE_NUM 0x00
#define ME_END_OF_TRACK 0x2F
#define ME_SET_TEMPO 0x51
#define ME_SMPTE_OFFSET 0x54
#define ME_TIME_SIGNATURE 0x58
#define ME_KEY_SIGNATURE 0x59
#define ME_SEQUENCER_SPECIFIC 0x7F

/* Meta Events Text Types */
#define METT_TEXT 0x01
#define METT_COPYRIGHT_NOTICE 0x02
#define METT_SEQUENCE_TRACK_NAME 0x03
#define METT_INSTRUMENT_NAME 0x04
#define METT_LYRIC 0x05
#define METT_MARKER 0x06
#define METT_CUE_POINT 0x07

/* These are the strings that identify Standard MIDI File
 *  Meta text messages.
 */
#define METATEXT "Text Event"
#define METACOPYRIGHT "Copyright Notice"
#define METASEQUENCE "Sequence/Track Name"
#define METAINSTRUMENT "Instrument Name"
#define METALYRIC "Lyric"
#define METAMARKER "Marker"
#define METACUE "Cue Point"
#define METAUNRECOGNIZED "Unrecognized"

#endif /* __MIDI_DEFS_H__ */
