/*******************************************************************************************
 *	File:			ParseMF.c
 *
 *	Contains:		Interface to a module which parses Standard MIDIFiles and callsback
 *					for every event in the file.
 *
 *	Usage:			Err	ParseMIDIFile( MIDIFileParserPtr mfp );
 *
 *
 *	Written by:		Software Attic
 *					 with a few clever ideas courtesy of Phil Burk
 *
 *	Copyright © 1994 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *
 *	09/16/94	rdg		Add SpinCursor()
 *	06/02/94	rdg		Port back to MPW after debugging in ThinkC
 *						Tighten track parsing logic
 *						Replaced raw hex values with #defines whereever possible for 
 *						increased readability
 *	06/01/94	rdg		Make sure MetaEvent() doesn't try to allocate memory for
 *						a message of size zero.
 *	05/31/94	rdg		Fix logic in ParseTrack() to properly handle unrecognized bytes
 *	05/31/94	rdg		Be more careful of signed vs unsigned bytes
 *	05/23/94	rdg		NewToday
 *
 *******************************************************************************************/


#include	<stdlib.h>
#include	<StdIO.h>
#include	<string.h>
#include 	<cursorctl.h>

#include	"MIDI_Defs.h"
#include	"ParseMF.h"

#define EOF (-1)


/* --------- Local routine prototypes ---------- */

/* Error Reporting */
static void 	MFError( MIDIFileParserPtr mfp, char *string, Err code );

/* Conversion Utils */
static long 	To32Bit( uchar c1, uchar c2, uchar c3, uchar c4 );
static short 	To16Bit( uchar c1, uchar c2 );

/* File reading utils */
static long		ReadByte( MIDIFileParserPtr mfp, uchar* byte );
static long		Read16Bits( MIDIFileParserPtr mfp, short* val );
static long		Read32Bits( MIDIFileParserPtr mfp, long* val );
static long		ParseVLN( MIDIFileParserPtr mfp, long* val );

/* High Level Parsing Routines */
static Err		ReadTypeString( MIDIFileParserPtr mfp, char* str );
static Err 		ReadHeader( MIDIFileParserPtr mfp );
static void 	ChanMessage( MIDIFileParserPtr mfp );
static Err		HandleSysex( MIDIFileParserPtr mfp, uchar byteCode );
static Err	 	MetaEvent( MIDIFileParserPtr mfp );
static Err		HandleSpecialMsgs( MIDIFileParserPtr mfp, uchar aByte );

static Err 		ParseTrack( MIDIFileParserPtr mfp );

/* -------------------------------------------------------------
 * 		Error detection and reporting
 * -------------------------------------------------------------
 */

static void	MFError( MIDIFileParserPtr mfp, char *string, Err code )
{
	if ( mfp->cb_HandleError )
		mfp->cb_HandleError( code, string );
}

/* -------------------------------------------------------------
 * 			Routines for getting data from the MIDIFile 
 * -------------------------------------------------------------
 */
/* Read a single character and abort on EOF */
static long	ReadByte( MIDIFileParserPtr mfp, uchar* byte )
{
	long	result;
	
	/* Get a byte from the midifile */
	result = mfp->cb_ReadFile( byte, 1 );

	/* Check for end of file */
	if ( result != 1 )
		{
		MFError( mfp, "Unexpected EOF while reading midifile!", PMFErrEOF );
		return PMFErrEOF;
		}
		
	/* Update in case anyone is currently keeping count */
	mfp->bytesLeftInChunk--;
	
	/* No problems */
	return result;
}

/* Read n bytes from the MIDIFile */
static long	ReadData( MIDIFileParserPtr mfp, void* buff, long count )
{
	long	result;
	
	result = mfp->cb_ReadFile( buff, count );

	/* Check for end of file */
	if ( result != count )
		{
		MFError( mfp, "Unexpected EOF while reading midifile!", PMFErrEOF );
		return PMFErrEOF;
		}
	/* Update in case anyone is currently keeping count */
	mfp->bytesLeftInChunk -= count;
			
	/* No problems */
	return result;
}

/* Suck 32 bits from the MIDIFile */
static long	Read32Bits( MIDIFileParserPtr mfp, long* val )
{
	long	result;
	uchar	c[4]; 

	result = ReadData( mfp, (void*) c, 4 );
	
	*val = To32Bit( c[0], c[1], c[2], c[3] );
	
	return result;
}

/* Suck 32 bits from the MIDIFile */
static long	Read16Bits( MIDIFileParserPtr mfp, short* val )
{
	long	result;
	uchar 	c[2];
	
	result = ReadData( mfp, (void*) c, 2 );

	*val = To16Bit( c[0], c[1] );
	
	return result;
}

/* Parse a Variable Length Number, stuff the value in the parameter
 * and return number of bytes in the value.
 */
static long	ParseVLN( MIDIFileParserPtr mfp, long* val )
{
	long	result		= 0;
	uchar 	datum		= 0;
	long	byteCount	= 0;
	long 	accum		= 0;

	do
		{
		/* Get a byte from the MIDIFile */
		result = ReadByte( mfp, &datum );
		
		/* On error, return the error code back up */
		if ( result != 1 )
			return result;
		
		/* Keep track of number of bytes read */	
		byteCount++;

		/* Accumulate the result */
		accum = ((datum & 0x7F) | (accum << 7));

		/* See if we need more bytes */		
		} while ( datum & 0x80 );
	
	/* We got a good value */
	*val = accum;
	
	return byteCount;
}

static long	To32Bit( uchar c1, uchar c2, uchar  c3, uchar c4 )
{
	long value = 0;

	value = c1;
	value = (value<<8) + c2;
	value = (value<<8) + c3;
	value = (value<<8) + c4;
	
	return value;
}

static short	To16Bit( uchar c1, uchar c2 )
{
	return (short) ((c1 << 8) + c2);
}

/* Read and parse a 4 byte "type ID" string from the MIDIFile */
static Err	ReadTypeString( MIDIFileParserPtr mfp, char* str )
{
	long 	type, num;
	long	IDExpected;

	/* This is the chunk identifier we're looking for */
	IDExpected = CHKID(str[0], str[1], str[2], str[3]);
	
	/* Read in what we think should be a Chunk Type ID */
	num = Read32Bits( mfp, &type );

	/* Did we get the right number of bytes from the file? */
	if ( num != 4 )
		{
		MFError( mfp, "EOF trying to read a chunk ID!", PMFErrEOF );
		return PMFErrEOF;
		}
	
	/* Did we get the right thing from the file? */
	if ( type != IDExpected )
		{
		MFError( mfp, "Found wrong chunk ID!", PMFErrWrongChunkID );
		return PMFErrWrongChunkID;
		}
	
	return 0;
}

/* Read and parse the MIDIFile's header chunk */
static Err	ReadHeader( MIDIFileParserPtr mfp )
{
	long	result;
	long	size;
	char	pad[16];
	
	/* Try to find the four byte header type string from the Midifile */
 	result = ReadTypeString( mfp, "MThd" );
 
	/* Report any errors back up the chain */
	if ( result < 0 )
		return result;

	/* We expect a 4 byte header size, followed by 6 data bytes */
	result = ReadData( mfp, pad, 10 );

	/* Report any errors back up the chain */
	if ( result != 10 )
		return result;
	
	/* Find out how big the header is */
	size = To32Bit( pad[0], pad[1], pad[2], pad[3] );

	/* Header should be 6 bytes */
	if ( size != 6 )
		return PMFErrBadHeaderSize;

	/* Fill in the appropriate fields in the parser context */
	mfp->mfType 	= To16Bit( pad[4], pad[5] );
	mfp->numTracks 	= To16Bit( pad[6], pad[7] );
	mfp->division 	= To16Bit( pad[8], pad[9] );

	/* Call back now that we have read and parsed the header successfully*/
	if ( mfp->cb_MFHeader )
		mfp->cb_MFHeader( mfp->mfType, mfp->numTracks, mfp->division );

	/* No Problems */
	return 0;
}


static void	ChanMessage( MIDIFileParserPtr mfp )
{
	switch ( mfp->chanMsg.status & 0xf0 ) 
		{
		case NOTEON:
			/* chanMsg.data1 = pitch, data2 = velocity */
			if ( mfp->cb_NoteOn )
				mfp->cb_NoteOn( mfp->currEvTime, &mfp->chanMsg );
			break;

		case NOTEOFF:
			/* chanMsg.data1 = pitch, data2 = velocity */
			if ( mfp->cb_NoteOff )
				mfp->cb_NoteOff( mfp->currEvTime, &mfp->chanMsg );
			break;

		case PRESSURE:
			/* chanMsg.data1 = pitch, data2 = pressure */
			if ( mfp->cb_Pressure )
				mfp->cb_Pressure( mfp->currEvTime, &mfp->chanMsg );
			break;

		case CONTROLLER:
			/* chanMsg.data1 = control, data2 = value */
			if ( mfp->cb_Controller )
				mfp->cb_Controller( mfp->currEvTime, &mfp->chanMsg );
			break;

		case PITCHBEND:
			/* chanMsg.data1 = MSB, data2 = LSB */
			if ( mfp->cb_Pitchbend )
				mfp->cb_Pitchbend( mfp->currEvTime, &mfp->chanMsg );
			break;

		case PROGRAM:
			/* chanMsg.data1 = Program Number, data2 = 0 */
			if ( mfp->cb_ProgramChange )
				mfp->cb_ProgramChange( mfp->currEvTime, &mfp->chanMsg );
			break;

		case CHANPRESSURE:
			/* chanMsg.data1 = Pressure, data2 = 0 */
			if ( mfp->cb_ChanPressure )
				mfp->cb_ChanPressure( mfp->currEvTime, &mfp->chanMsg );
			break;
	}
}

static Err	HandleSysex ( MIDIFileParserPtr mfp, uchar byteCode )
{
	long	result;			/* Return values from called functions */
	long	msgLength;
	
	/* Buffer for constructing error msgs */
	char*	errBuff	= NULL;	

	/* Get length of Sysex msg from MIDIFile */
	result = ParseVLN( mfp, &mfp->genMsg.length );

	/* Report any errors back up the chain */
	if ( result < 0 )
		{
		MFError( mfp, "Error reading Sysex msg length!", result );
		return result;
		}

	/* Make the length easier to read */
	msgLength = (long)mfp->genMsg.length;
	
	/* Some MetaEvent msgs are of length zero and so don't need 
	 * to read any more data...
	 */
	if ( msgLength > 0 )
		{
		/* Allocate enough memory to collect the entire msg */
		mfp->genMsg.msg	 = (uchar*)malloc( msgLength );
	
		/* Report any errors back up the chain */
		if ( mfp->genMsg.msg == NULL )
			{
			sprintf( errBuff, "Not enough memory for Sysex msg!; size = %d.", msgLength );
			MFError( mfp, errBuff, PMFErrInsufficientMemory );
			return PMFErrInsufficientMemory;
			}
	
		/* Read the msg from the MIDIFile */
		result = ReadData( mfp, mfp->genMsg.msg, msgLength );
					
		/* Report any errors back up the chain */
		if ( result != msgLength )
			{
			MFError( mfp, "Error reading Sysex msg!", result );

			/* Free up the buffer if it was used */
			if ( msgLength > 0 )
				free( mfp->genMsg.msg );

			return result;
			}
		}

	/* Pass the message back via callback */
	if ( mfp->cb_Sysex )
		mfp->cb_Sysex( mfp->currEvTime, byteCode, &mfp->genMsg );

	/* Free up the buffer if it was used */
	if ( msgLength > 0 )
		free( mfp->genMsg.msg );
	
	/* Zero out the msg buffer */ 					
	mfp->genMsg.type	= 0;		
	mfp->genMsg.msg		= NULL;		
	mfp->genMsg.length	= 0;		
}

/* Read and parse a meta event from the MIDIFile */
static Err	MetaEvent( MIDIFileParserPtr mfp )
{
	long	result;
	long	msgLength;

	/* Buffer for constructing error msgs */
	char*	errBuff	= NULL;	

	/* Get the Meta event type */
	result = ReadByte( mfp, &mfp->genMsg.type );

	/* Report any errors back up the chain */
	if ( result != 1 )
		{
		MFError( mfp, "Error reading Meta event type!", result );
		return result;
		}
	
	/* Get length of Meta event from MIDIFile */
	result = ParseVLN( mfp, &mfp->genMsg.length );

	/* Report any errors back up the chain */
	if ( result < 0 )
		{
		MFError( mfp, "Error reading Meta Event message length!", result );
		return result;
		}

	/* Make the length easier to read */
	msgLength = (long)mfp->genMsg.length;
	
	/* Some MetaEvent msgs are of length zero and so don't need 
	 * to read any more data...
	 */
	if ( msgLength > 0 )
		{
		/* Allocate enough memory to collect the entire msg plus room for
		 * a zero string termination byte in case this is a text event
		 */
		mfp->genMsg.msg	 = (uchar*)malloc( msgLength + 1 );
	
		/* Report any errors back up the chain */
		if ( mfp->genMsg.msg == NULL )
			{
			sprintf( errBuff, "Not enough memory for Meta event msg!; size = %d.", msgLength );
			MFError( mfp, errBuff, PMFErrInsufficientMemory );
			return PMFErrInsufficientMemory;
			}
	
		/* Read the msg from the MIDIFile */
		result = ReadData( mfp, mfp->genMsg.msg, msgLength );
					
		/* Report any errors back up the chain */
		if ( result != msgLength )
			{
			MFError( mfp, "Error reading Meta event msg!", result );

			/* Free up the buffer if it was used */
			if ( msgLength > 0 )
				free( mfp->genMsg.msg );

			return result;
			}
		}
									
	switch  ( mfp->genMsg.type ) 
		{
		case ME_SEQUENCE_NUM:
			/* Sequence number event.  The two bytes are a 16bit value representing
			 * the number of the track which is about to follow.  Format 0 and 1
			 * MIDIFiles only contain one sequence. This value only applies to
			 * format 2 MIDIFiles where it is used to uniquely identify each pattern
			 * in the file.
			 */
			if ( mfp->cb_MEv_SeqNum )
				{
				mfp->seqNum = To16Bit( mfp->genMsg.msg[0], mfp->genMsg.msg[1] );
				mfp->cb_MEv_SeqNum( mfp->currEvTime, mfp->seqNum );
				}
			break;

		case METT_TEXT:	
		case METT_COPYRIGHT_NOTICE:
		case METT_SEQUENCE_TRACK_NAME:	
		case METT_INSTRUMENT_NAME:	
		case METT_LYRIC:	
		case METT_MARKER:
		case METT_CUE_POINT:	
		case 0x08:	/* The rest of these are Undefined Text Events */
		case 0x09:
		case 0x0a:
		case 0x0b:
		case 0x0c:
		case 0x0d:
		case 0x0e:
		case 0x0f:
			/* All of the above are Text Meta Events.  The msg is formatted as
			 * a length byte and a variable number of (hopefully!) ASCII
			 * characters.  The string doesn't contain a null terminator
			 * so we have to add one ourselves.
			 */
			if ( mfp->cb_MEv_Text )
				{
				/* Put in the null termination byte for text events 
				 * and add 1 to the length of the msg
				 */
				mfp->genMsg.msg[msgLength] = 0;
				mfp->genMsg.length++;
				mfp->cb_MEv_Text( mfp->currEvTime, &mfp->genMsg );
				}
			break;

		case ME_END_OF_TRACK:	
			/* End of Track Event*/
			if ( mfp->cb_MEv_EndOfTrack )
				mfp->cb_MEv_EndOfTrack( mfp->currEvTime );
			break;

		case ME_SET_TEMPO:	
			/* Set tempo event. Value is usecs per quarter note */
			if ( mfp->cb_MEv_SetTempo )
				{
				mfp->tempo = To32Bit(0, mfp->genMsg.msg[0], mfp->genMsg.msg[1], 
										mfp->genMsg.msg[2]);
				mfp->cb_MEv_SetTempo( mfp->currEvTime, mfp->tempo );
				}
			break;

		case ME_SMPTE_OFFSET:
			/* SMPTE time at which the track should start 
			 * The five bytes of the msg are in the format:
			 * 1) Hours
			 * 2) Minutes
			 * 3) Seconds
			 * 4) Frames
			 * 5) Fractional-Frames
			 */
			if ( mfp->cb_MEv_SMPTE )
				mfp->cb_MEv_SMPTE( mfp->currEvTime, &mfp->genMsg );
			break;

		case ME_TIME_SIGNATURE:
			/* Time signature change.  The four bytes of the msg are
			 * in the format:
			 * 1) Numerator
			 * 2) Denominator
			 * 3) MIDI Clocks per metronome click
			 * 4) number of notated 32nd notes in a MIDI quarter note 
			 *    (a MIDI Quarter Note is 24 MIDI Clocks)
			 *
			 * The Denominator is for some reason expressed as "negative power 
			 * of two".  In other words, 2 = a quarter note, 3 = an eighth note.
			 * Basically you take the number 2 and shift it left by
			 * the MIDIFile's denominator value - 1. Fun Fun...
			 */
			if ( mfp->cb_MEv_TimeSig )
				mfp->cb_MEv_TimeSig( mfp->currEvTime, &mfp->genMsg );
			break;

		case ME_KEY_SIGNATURE:
			/* Key Signature change. The two bytes of the msg are in 
			 * the format:
			 * 1) Number of sharps/flats where:
			 *    -5 = 5 flats
			 *     4 = 4 sharps
			 *     0 = Key of C
			 * 2) Minor or major
			 *    0 = Major Key
			 *    1 = Minor Key
			 */
			if ( mfp->cb_MEv_KeySig )
				mfp->cb_MEv_KeySig( mfp->currEvTime, &mfp->genMsg );
			break;

		case ME_SEQUENCER_SPECIFIC:
			/* A sequencer specific Meta Event.  The msg is formatted
			 * as a length byte and a variable number of data bytes
			 */
			if ( mfp->cb_MEv_SeqSpecific )
					mfp->cb_MEv_SeqSpecific( mfp->currEvTime, &mfp->genMsg );
			break;

		default:
			/* An unrecognized/misc Meta Event.  The msg is most likely
			 * formatted as a length byte and a variable number of data bytes
			 */
			if ( mfp->cb_MEv_Misc )
				mfp->cb_MEv_Misc( mfp->currEvTime, &mfp->genMsg );
		}

	/* Free up the buffer if it was used */
	if ( msgLength > 0 )
		free( mfp->genMsg.msg );

	/* Zero out the msg buffer */ 					
	mfp->genMsg.type	= 0;		
	mfp->genMsg.msg		= NULL;		
	mfp->genMsg.length	= 0;		
}


/* Deal with Meta and Real Time Messages */
static Err	HandleSpecialMsgs( MIDIFileParserPtr mfp, uchar aByte )
{
	Err	result;
	
	/* MPW won't work unless I do this...  */
	long wierd	=	(long) aByte;
	
	switch ( wierd ) 
		{
		/* Meta event */
		case META_EVENT:
			result = MetaEvent( mfp );

			/* Report any errors back up the chain */
			if ( result < 0 )
				return result;
			break;

		/* Start of system exclusive msg */
		case SYSEX_BEGIN:
			
		/* Sysex continuation or arbitrary events */
		case SYSEX_CONTINUE:
			result = HandleSysex( mfp, aByte );

			/* Report any errors back up the chain */
			if ( result < 0 )
				return result;
			break;
		
		/* All of these Real Time Msgs have no data 
		 * bytes so just callback 
		 */
		case MRT_CLOCK:
			if ( mfp->cb_RT_Clock )
				mfp->cb_RT_Clock( mfp->currEvTime );
			break;
			
		case MRT_START:
			if ( mfp->cb_RT_Start )
				mfp->cb_RT_Start( mfp->currEvTime );
			break;
			
		case MRT_CONTINUE:
			if ( mfp->cb_RT_Continue )
				mfp->cb_RT_Continue( mfp->currEvTime );
			break;
			
		case MRT_STOP:
			if ( mfp->cb_RT_Stop )
				mfp->cb_RT_Stop( mfp->currEvTime );
			break;
			
		case MRT_ACTIVE_SENSE:
			if ( mfp->cb_RT_ActiveSense )
				mfp->cb_RT_ActiveSense( mfp->currEvTime );
			break;
			
		default:
			break;
		}
}

/* An array for looking up how many data bytes are needed for a given MIDI msg.
 * The array is indexed using the byte code of the msg iteslf.  Slick.
 */
static char DataBytesPer[] = 
{
	2,  2,  2,  2,  1,  1,  2,  0,   /* 0x80, 0x90, ..., 0xE0, unused */
	1,  1,  2,  1,  0,  0,  0,  0,   /* 0xF0, 0xF1, ..., 0xF7 */
	0,  0,  0,  0,  0,  0,  0,  0    /* 0xF8, 0xF9, ..., 0xFF */
};

static long MIDIDataBytesPer( long command )
{
	long indx;
	
	if ( (command & 0xF0) == 0xF0 )
		indx = ((command >> 4) & 0x0F) + 8;
	else
		indx = (command >> 4) & 0x07;

	return DataBytesPer[indx];
}

/* Parse a track chunk and call appropriate callback when an event is
 * encountered
 */
static Err	ParseTrack( MIDIFileParserPtr mfp )
{
	long	result			= 0;		/* Return values from called functions */
	
	uchar	aByte 			= 0;		/* Raw data byte from MIDI file */
	uchar	runStatus		= 0;		/* Running status byte */
	char	dataBytesNeeded	= 0;		/* num data bytes needed for a given channel msg */
	char	bytesToRead		= 0;		/* how many bytes to read given running status etc. */

	long	i;							/* For counting accumulated MIDI msg bytes */

	uchar	pad[5];						/* Scratch pad */
	long	deltaTime;					/* Delta time stamp from MIDIFile */

	/* Look for track header chunk type */
 	result = ReadTypeString( mfp, "MTrk" );
 
	/* Report any errors back up the chain */
	if ( result < 0 )
		{
		MFError( mfp, "Couldn't find a track header!", PMFErrBadTrackHeader );
		return PMFErrBadTrackHeader;
		}
		
	/* FInd out how many bytes in the track chunk */
	result = Read32Bits( mfp, &mfp->bytesLeftInChunk );

	/* Report any errors back up the chain */
	if ( result != 4 )
		{
		MFError( mfp, "Error reading track size!", result );
		return result;
		}
	
	/* Initalize track time */
	mfp->currEvTime = 0;

	/* Call the track start callback function */
	if ( mfp->cb_StartTrack )
		mfp->cb_StartTrack( mfp->currEvTime );
	
	/* Parse the track one event at a time */
	while ( mfp->bytesLeftInChunk > 0 ) 
		{
		/* Spin cursor during processing, and allow Cmd-. */
		SpinCursor(32);

		/* Read delta time stamp from MIDIFile */
		result = ParseVLN( mfp, &deltaTime );

		/* Report any errors back up the chain */
		if ( result < 0 )
			{
			MFError( mfp, "Error reading event timestamp!", result );
			return result;
			}

		/* Update current time in parser context */
		mfp->currEvTime += deltaTime;

		/* Get a data byte */
		result = ReadByte( mfp, &aByte );

		/* Report any errors back up the chain */
		if ( result != 1 )
			{
			MFError( mfp, "Error reading a data byte!", result );
			return result;
			}

		/* Is this a special non-channel msg?  Since all these events
		 * are special and are numerically contiguous I test for them
		 * in this slightly dodgy manner
		 */
		if ( (aByte >= SYSEX_BEGIN) && (aByte <= META_EVENT) )	
			{
			result = HandleSpecialMsgs( mfp, aByte );

			/* Report any errors back up the chain */
			if ( result < 0 )
				{
				MFError( mfp, "Error parsing a non-channel msg!", result );
				return result;
				}
			}
		else 
			{
			/* Reset msg accumulator count */
			i = 0;
			
			/* Is this a new MIDI channel msg? */
			if ( aByte & 0x80 ) 
				{				
				/* Save new status byte */
				pad[i++] = aByte;
	
				/* Keep for running status */
				runStatus = aByte;
	
				/* How many data bytes needed for this msg */
				dataBytesNeeded = MIDIDataBytesPer( aByte );
				bytesToRead = dataBytesNeeded;
				}
			else 
				{
				/* Must be a data byte so we have running status */
				pad[i++] = runStatus;
				pad[i++] = aByte;
				
				/* How many data bytes needed for this msg */
				dataBytesNeeded = MIDIDataBytesPer( runStatus );
				
				/* We already have one because of running status */
				bytesToRead = dataBytesNeeded - 1;
				}

			if ( (bytesToRead > 0) && (bytesToRead < 3) )
				{
				/* Get data byte(s) from the file */
				result = ReadData( mfp, &pad[i], bytesToRead );
		
				/* Report any errors back up the chain */
				if ( result != bytesToRead )
					{
					MFError( mfp, "Error reading a data byte!", result );
					return result;
					}
				}

			/* Build channel msg from data accumulated into pad */
			mfp->chanMsg.status	= pad[0];
			mfp->chanMsg.chan	= (mfp->chanMsg.status & 0xf);
			
			mfp->chanMsg.data1	= pad[1];

			/* Zero out second data byte if not used. Not strictly
			 * necessary, but polite.
			 */
			if ( dataBytesNeeded == 2 )
				mfp->chanMsg.data2	= pad[2];
			else
				mfp->chanMsg.data2	= 0;
				
			/* Process the channel msg */
			ChanMessage( mfp );
			
		} /* else not Meta Event */
		
	}  /* while (bytesleft) */
	
	/* Finished parsing this track */
	if ( mfp->cb_EndTrack )
		mfp->cb_EndTrack( mfp->currEvTime );
		
	/* Success */
	return 0;
}

/* The only non-static functions in this file. */
Err	InitMIDIFileParser( MIDIFileParserPtr mfp )
{
	Err	result	= 0;
	
	/* A way to keep track of data left in a track chunk */
	mfp->bytesLeftInChunk	= 0;	

	/* Global characteristics of the MIDIFile */
	mfp->mfType				= 0;
	mfp->numTracks			= 0;
	mfp->division			= 0;
	mfp->tempo				= 0;
	mfp->seqNum				= 0;

	/* Absolute time of current event */	
	mfp->currEvTime			= 0;	

	/* A MIDI channel msg */
	mfp->chanMsg.chan		= 0;
	mfp->chanMsg.status		= 0;
	mfp->chanMsg.data1		= 0;
	mfp->chanMsg.data2		= 0;
	
	/* A generic MIDI msg, text, sysex, etc. */
	mfp->genMsg.type		= 0;
	mfp->genMsg.msg			= NULL;
	mfp->genMsg.length		= 0;

	/* Make sure we have a way to get data from the MIDIFile before we start */
	if ( mfp->cb_ReadFile == NULL )
		{
		printf( "InitMidiFileParser() called without providing a ReadFile callback"); 
		result = PMFErrNoReadfileCallback;
		}
		
	return result;
}

Err	ParseMIDIFile( MIDIFileParserPtr mfp )	 	
{
	Err		result;
	long	trackCount;
	
	/* Allow spin cursor */
	InitCursorCtl(NULL);

	/* Read and parse the MIDIFile's header */
	result = ReadHeader( mfp );

	/* Report any errors */
	if ( result < 0 )
		{
		MFError( mfp, "Read Header failed!", PMFErrBadHeader );
		return PMFErrBadHeader;
		}

	/* Make sure that the MIDIFile has at least one track to parse */
	if ( mfp->numTracks <= 0 )
		{
		MFError( mfp, "File contains no tracks!", PMFErrNoTracks );
		return PMFErrNoTracks;
		}
		
	trackCount = mfp->numTracks;
	
	/* Parse each track */
	for ( trackCount = 0; trackCount < mfp->numTracks; trackCount++ )
		{
		result = ParseTrack( mfp );

		if ( result < 0 )
			{
			MFError( mfp, "ParseTrack() failed!", PMFErrBadTrack );
			return PMFErrBadTrack;
			}
		}
		
	/* Total success dude! */
	return 0;
}

Err	TermMIDIFileParser( MIDIFileParserPtr mfp ) 	
{
	return 0;
}

