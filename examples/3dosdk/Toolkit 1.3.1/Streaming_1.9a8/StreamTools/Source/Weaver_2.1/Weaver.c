/*******************************************************************************************
 *	File:			Weaver.c
 *
 *	Contains:		data stream merging application
 *
 *	Written by:		Joe Buczek
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	History:
 *	12/2/93		jb		Version 2.1
 *						Add command "numsubsmessages" to allow specification of the number
 *						of subscriber messages that should be allocated to run a given
 *						stream. Value gets written to the stream header.
 *						Made token recognizer case insensitive. All tokens may now be
 *						expressed in upper, lower, or mixed case.
 *	10/25/93	jb		Version 2.0
 *						Add support for variable sized marker table; boolean script command
 *						"writemarkertable" added to enable writing the marker table to the 
 *						output stream; Initialize the marker table to ones so that uninited
 *						entries can be detected by DataAcq.c; Add a default "markertime 0"
 *						behavior and always include a time zero marker in the output.
 *	10/20/93	jb		Version 1.9
 *						Add delta priority as a second argument to the "subscriber" comand.
 *						Add streamerpriority and dataacqpriority commands.
 *	10/19/93	jb		Version 1.8
 *						Change command line numeric parsing to accept either decimal
 *						or hexadecimal inputs.
 *	10/12/93	jb		Version 1.7
 *						Add comment "command" (lines starting with '#' are comments).
 *						Add parsing for commands to accept stream header values:
 *							streambuffers <suggested # of buffers to allocate for playback> 
 *							audioclockchan	<audio clock channel>
 *							enableaudiomask <bit mask of audio channels to ENABLE>
 *							preloadinstrument <name of audio data type>
 *							subscriber <subscriber ident (same as data type)> <delta priority>
 * 
 *	10/11/93	jb		Version 1.6
 *						Check chunk size <= stream block size in WeaveStreams.c
 *	9/12/93		rdg		Version 1.5
 *						Spin cursor.
 *	7/11/93		jb		Version 1.4
 *						Add feature to allow output of marker data to a binary output
 *						file so it can be post-processed with the Chunkify tool, and
 *						woven back into the stream for consumption by the data acquisition
 *						code. This will give us an automated way to get marker mapping
 *						info into the stream.
 *						Modified DumpMarkerTable() to write a binary version of the
 *						marker data table.
 *	6/28/93		jb		Version 1.3
 *						Put gIOBufferSize into the weaver param block so it will actually
 *						allocate the right sized buffers!
 *	6/27/93		jb		Add I/O buffer size specification
 *	6/21/93		jb		Version 1.2
 *						Make Control subscriber chunk data type agree with the actual
 *						Control subscriber's data type.
 *	6/17/93		jb		Version 1.1
 *						Output a 'SYNC' subtype chunk for the Control Subscriber at
 *						the start of every marker block. Add check to make sure that
 *						stream block size is an exact multiple of media block size.
 *	5/20/93		jb		Add program verson output in Usage(). Version 1.0
 *	5/18/93		jb		Add DumpMarkerTable()
 *	5/15/93		jb		New today.
 *
 *******************************************************************************************/

#define	PROGRAM_VERSION_STRING		"2.1"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Types.h>
#include <Memory.h>
#include <cursorctl.h>

#include "Weaver.h"
#include "WeaveStream.h"
#include "SATemplates.h"
#include "DSStreamHeader.h"

/*********************************************************************/
/* Parameters set by the ParseCommandLine() and/or ParseDirectives() */
/*********************************************************************/
long		gMediaBlockSize			= (2 * 1024L);
long		gStreamBlockSize		= (32 * 1024L);
long		gOutputBaseTime			= 0;
char*		gOutputStreamName		= NULL;
Boolean		gVerboseFlag			= false;
long		gIOBufferSize			= (32 * 1024L);
long		gMaxMarkers				= 1024;

/* Stream header parameters set only by ParseDirectives() */

Boolean		gWriteStreamHeader		= false;
Boolean		gWriteMarkerTable		= false;
long		gStreamBuffers			= 0;
long		gAudioClockChan			= 0;
long		gEnableAudioChan		= 0;
long		gStreamerDeltaPri		= 0;
long		gDataAcqDeltaPri		= 0;
long		gNextSubscriberIndex	= 0;
long		gNextPreloadInstIndex	= 0;
long		gNumSubsMsgs			= 256;

/*****************/
/* Other globals */
/*****************/
Boolean		gParseError				= false;
long		gParseLineNumber		= 1;

WeaveParams	gWSParamBlk;			/* Param block used by WeaveStreams() */


/****************************/
/* Local routine prototypes */
/****************************/
static Boolean		ParseCommandLine( int argc, char **argv );
static void			Usage( char* commandNamePtr );
static Boolean		ParseDirectives( char* pWord );
static void			GetTokensFromFile( FILE* fd, CallBackProcPtr callBackProc );
static void			AddPreloadInstrument( WeaveParamsPtr pb, char* pWord );
static void			AddSubscriber( WeaveParamsPtr pb, char* pWord );
static int			WordCmp( char *word, char *target );


/*******************************************************************************************
 * Main tool entrypoint. This may be called from a THINK C application shell, or directly
 * as an MPW tool (the former is used for debugging purposes only).
 *******************************************************************************************/
long main( int argc, char* argv[] )
	{
	long	status;
	long	index;
	long	markerTableSize;

	/* Collect command line switch values. If any error, output some
	 * helpful info and exit.
	 */
	if ( ! ParseCommandLine( argc, argv ) )
		{
		Usage(argv[0]);
		return EXIT_FAILURE;
		}

	/* So we can spin cursor */
	InitCursorCtl(NULL);

	/* Allocate the marker array. We fill this in with times that
	 * the user deems "interesting", and the weaver fills in the
	 * file positions associated with those times and takes care
	 * of "aligning" the data for branching to those times.
	 */
	markerTableSize = gMaxMarkers * sizeof( MarkerRec );
	gWSParamBlk.marker = (MarkerRecPtr) malloc( markerTableSize );
	if ( gWSParamBlk.marker == nil )
		{
		fprintf( stderr, "### Error allocating marker table\n" );
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	/* Initialize the marker table to all ones */
	memset( gWSParamBlk.marker, 0xff, markerTableSize );

	/* Initialize a couple of param block fields
	 */
	gWSParamBlk.numInputStreams		= 0;
	gWSParamBlk.numMarkers			= 0;
	gWSParamBlk.maxMarkers			= gMaxMarkers;

	/* Create a marker for the beginning of time */
	gWSParamBlk.marker[ gWSParamBlk.numMarkers++ ].markerTime = 0;

	/* Clear the instrument preload table */
	for ( index = 0; index < DS_HDR_MAX_PRELOADINST; index++ )
		gWSParamBlk.preloadInstList[ index ] = 0;
	
	/* Clear the subscriber tag table */
	for ( index = 0; index < DS_HDR_MAX_SUBSCRIBER; index++ )
		{
		gWSParamBlk.subscriberList[ index ].subscriberType = 0;
		gWSParamBlk.subscriberList[ index ].deltaPriority = 0;
		}

	/* Read and parse the input directives file. This will set up the
	 * parameter block we'll pass to WeaveStreams() below.
	 */
	GetTokensFromFile( stdin, (ProcPtr) ParseDirectives );
	if ( gParseError )
		{
		fprintf( stderr, "### Line %ld\n", gParseLineNumber );
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	/* Verify that the stream block size is a multiple of the media block size.
	 * This is to insure that we can do integral block I/O to the device where
	 * the stream data will ultimately original from.
	 */
	if ( (gStreamBlockSize % gMediaBlockSize) != 0 )
		{
		fprintf( stderr, "### Error: stream block size not exact multiple of media block size\n" );
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	SpinCursor(32);

	/* Set up the parameter block for the weaving routine by
	 * copying the values set by the command line switches and
	 * directive file parsing.
	 */
	gWSParamBlk.mediaBlockSize		= gMediaBlockSize;
	gWSParamBlk.streamBlockSize		= gStreamBlockSize;
	gWSParamBlk.outputBaseTime		= gOutputBaseTime;
	gWSParamBlk.outputStreamName	= gOutputStreamName;
	gWSParamBlk.ioBufferSize		= gIOBufferSize;
	gWSParamBlk.streamBuffers		= gStreamBuffers;
	gWSParamBlk.audioClockChan		= gAudioClockChan;
	gWSParamBlk.enableAudioChan		= gEnableAudioChan;
	gWSParamBlk.streamerDeltaPri	= gStreamerDeltaPri;
	gWSParamBlk.dataAcqDeltaPri		= gDataAcqDeltaPri;
	gWSParamBlk.fWriteStreamHeader	= gWriteStreamHeader;
	gWSParamBlk.fWriteMarkerTable	= gWriteMarkerTable;
	gWSParamBlk.numSubsMsgs			= gNumSubsMsgs;

	if ( ! OpenDataStreams( &gWSParamBlk ) )
		{
		status = EXIT_FAILURE;
		goto BAILOUT;
		}

	/* Call the weaving routine. The result is one output data stream
	 * file, sorted by time order.
	 */
	status = WeaveStreams( &gWSParamBlk );
	if ( status != 0 )
		{
		fprintf( stderr, "### Error WeaveStreams() returned: %ld\n", status );
		status = EXIT_FAILURE;
		}

BAILOUT:
	CloseDataStreams( &gWSParamBlk );
	return status;
	}


/*******************************************************************************************
 * Utilty routine to allow case insensitive comparison of keywords. Assumes second
 * string parameter is LOWER CASE. Returns zero if strings match exactly.
 *******************************************************************************************/
static int	WordCmp( char *word, char *target )
	{
	register char	c;
	register char*	cp = word;
	register char*	tp = target;

	/* Loop while neither string is at end */
	while ( (*cp != 0) && (*tp != 0) )
		{
		/* Get the next char. Convert it to lower case
		 * if it is an upper case alphabetic char.
		 */
		c = *cp;
		if ( (c >= 'A') && (c <= 'Z') )
			c += 'a' - 'A';

		/* If the current char does not match the corresponding
		 * target string character, the strings are unequal.
		 */
		if ( c != *tp )
			break;

		/* Advance both */
		++cp;
		++tp;
		}

	/* One or both strings are at end. Return the difference of
	 * the two. If both are at end, then zero will be the result.
	 */
	return *cp - *tp;
	}


/*******************************************************************************************
 * Token parser callback routine gets called for each non-whitespace word in the
 * input directive file. Parses commands that are used to program the actions of
 * the stream weaver.
 *******************************************************************************************/

/************************/
/* Parser "next" states */
/************************/
typedef enum {
	kNewCommand,		/* expecting new command */
	kMarkerTime,		/* expecting marker time */
	kFilename,			/* expecting file name string */
	kPriority,			/* expecting file priority number */
	kFileStartTime,		/* expecting file starting time (base time) */
	kCommentLine,		/* expecting comment characters until end of line */

	/* Stream header items */

	kMediaBlockSize,	/* expecting media block size */
	kStreamBlockSize,	/* expecting stream block size */
	kStreamStartTime,	/* expecting output stream start time */

	kStreamBuffers,		/* expecting number of buffers required to play the stream */
	kAudioClockChan,	/* expecting number of audio channel designated as clock source */
	kEnableAudioChan,	/* expecting number of secondary audio channel to enable */
	kPreloadInstrument,	/* expecting string specifying SAudioSubscriber tag name */
	kSubscriber,		/* expecting string specifying subcriber tag */
	kSubscriberPrio,	/* expecting string specifying subcriber delta priority */
	kStreamerDeltaPri,	/* expecting streamer thread delta priority */
	kDataAcqDeltaPri,	/* expecting data acquisition thread delta priority */
	kNumSubsMessages	/* expecting number of subscriber msgs to allocate for stream */
	} ParserState;

static ParserState gParseState = kNewCommand;

/* This routine gets called repeatedly with tokens from the input
 * directives file. We advance the parser state as we accept tokens
 * in the input stream. Pretty standard stuff. Hopefully extensible to
 * permit adding new directives.
 */
static Boolean ParseDirectives( char* pWord )
	{
	InStreamPtr		isp;

	/* Get a pointer to the input stream we are currently
	 * filling in.
	 */
	isp = gWSParamBlk.inStream + gWSParamBlk.numInputStreams - 1;

	/* If we get a NEWLINE at an unexpected place, we've 
	 * got a syntax error.
	 */
	if ( (strcmp( pWord, "\n" ) == 0) )
		{
		gParseLineNumber++;

		/* A newline at the end of a comment line causes us to begin
		 * a new command, and to signal successful completion parsing
		 * the comment "command".
		 */
		if ( gParseState == kCommentLine )
			{
			gParseState = kNewCommand;
			return true;
			}

		else if ( gParseState == kNewCommand )
			/* Discard the NEWLINE. It appeared
			 * where it was expected.
			 */
			return true;

		else
			/* Found a NEWLINE before it was expected.
			 * This means some command is missing some parameters
			 */
			{
			gParseError = true;
			return false;
			}
		}

	/* Pick up the parse where we left off
	 */
	switch ( gParseState )
		{
		case kNewCommand:
			{
			if ( WordCmp( pWord, "file" ) == 0 )
				{
				if ( gWSParamBlk.numInputStreams++ < MAX_INPUT_STREAMS )
					gParseState = kFilename;
				else
					{
					gParseError = true;
					return false;
					}
				}

			else if ( WordCmp( pWord, "mediablocksize" ) == 0 )
				gParseState = kMediaBlockSize;

			else if ( WordCmp( pWord, "streamblocksize" ) == 0 )
				gParseState = kStreamBlockSize;

			else if ( WordCmp( pWord, "streamstarttime" ) == 0 )
				gParseState = kStreamStartTime;

			else if ( WordCmp( pWord, "markertime" ) == 0 )
				{
				if ( gWSParamBlk.numMarkers < (gWSParamBlk.maxMarkers - 1) )
					gParseState = kMarkerTime;
				else
					{
					gParseError = true;
					return false;
					}
				}

			else if ( WordCmp( pWord, "streambuffers" ) == 0 )
				gParseState = kStreamBuffers;

			else if ( WordCmp( pWord, "audioclockchan" ) == 0 )
				gParseState = kAudioClockChan;

			else if ( WordCmp( pWord, "enableaudiomask" ) == 0 )
				gParseState = kEnableAudioChan;

			else if ( WordCmp( pWord, "#" ) == 0 )
				gParseState = kCommentLine;

			else if ( WordCmp( pWord, "preloadinstrument" ) == 0 )
				gParseState = kPreloadInstrument;

			else if ( WordCmp( pWord, "subscriber" ) == 0 )
				gParseState = kSubscriber;

			else if ( WordCmp( pWord, "streamerdeltapri" ) == 0 )
				gParseState = kStreamerDeltaPri;

			else if ( WordCmp( pWord, "dataacqdeltapri" ) == 0 )
				gParseState = kDataAcqDeltaPri;

			else if ( WordCmp( pWord, "writestreamheader" ) == 0 )
				gWriteStreamHeader = true;

			else if ( WordCmp( pWord, "writemarkertable" ) == 0 )
				gWriteMarkerTable = true;

			else if ( WordCmp( pWord, "numsubsmessages" ) == 0 )
				gParseState = kNumSubsMessages;

			else
				{
				/* unknown command */
				fprintf( stderr, "### Error: unknown directive: %s\n", pWord );
				gParseError = true;
				return false;
				}

			break;
			}

		case kMediaBlockSize:
			sscanf( pWord, "%li", &gMediaBlockSize);
			gParseState = kNewCommand;
			break;

		case kStreamBlockSize:
			sscanf( pWord, "%li", &gStreamBlockSize);
			gParseState = kNewCommand;
			break;

		case kStreamStartTime:
			sscanf( pWord, "%li", &gOutputBaseTime);
			gParseState = kNewCommand;
			break;

		case kMarkerTime:
			{
			sscanf( pWord, "%li", 
					&gWSParamBlk.marker[ gWSParamBlk.numMarkers++ ].markerTime );
			gParseState = kNewCommand;
			break;
			}

		case kFilename:
			{
			/* Allocate a place to store the file name */
			isp->fileName = (char*) malloc( strlen( pWord ) + 1 );
			if ( isp->fileName == nil )
				{
				gParseError = true;
				return false;
				}

			/* Copy the file name */
			strcpy( isp->fileName, pWord );

			/* Clear the EOF flag */
			isp->eof = false;

			gParseState = kPriority;
			break;
			}

		case kPriority:
			sscanf( pWord, "%li", &isp->priority );
			gParseState = kFileStartTime;
			break;

		case kFileStartTime:
			sscanf( pWord, "%li", &isp->startTime );
			gParseState = kNewCommand;
			break;

		case kCommentLine:
			/* Just eat comment tokens until end of line */
			break;

		case kStreamBuffers:
			sscanf( pWord, "%li", &gStreamBuffers );
			gParseState = kNewCommand;
			break;

		case kAudioClockChan:
			sscanf( pWord, "%li", &gAudioClockChan );
			gParseState = kNewCommand;
			break;

		case kEnableAudioChan:
			/* Note format specifier "i" allows hexadecimal input
			 * for this which is interpretted as a mask of channels
			 * to be enabled, where 2**n is set for each enabled channel.
			 */
			sscanf( pWord, "%li", &gEnableAudioChan );
			gParseState = kNewCommand;
			break;

		case kPreloadInstrument:
			AddPreloadInstrument( &gWSParamBlk, pWord );
			gParseState = kNewCommand;
			break;

		case kSubscriber:
			AddSubscriber( &gWSParamBlk, pWord );
			gParseState = kSubscriberPrio;
			break;

		case kSubscriberPrio:
			if ( gNextSubscriberIndex < (DS_HDR_MAX_SUBSCRIBER - 1) )
				{
				sscanf( pWord, "%li", 
					&gWSParamBlk.subscriberList[ gNextSubscriberIndex ].deltaPriority );

				/* One less free slot in the subscriber table */
				gNextSubscriberIndex++;
				}

			gParseState = kNewCommand;
			break;

		case kStreamerDeltaPri:
			sscanf( pWord, "%li", &gStreamerDeltaPri );
			gParseState = kNewCommand;
			break;

		case kDataAcqDeltaPri:
			sscanf( pWord, "%li", &gDataAcqDeltaPri );
			gParseState = kNewCommand;
			break;

		case kNumSubsMessages:
			sscanf( pWord, "%li", &gNumSubsMsgs );
			gParseState = kNewCommand;
			break;

		default:
			fprintf( stderr, "### Error: ParseDirectives() invalid parser state\n" );
			gParseError = true;
			return false;
		}

	return true;
	}


/*******************************************************************************************
 * Routine to add a preload instrument descriptor to the weave param block if there is 
 * space. Here we map the instrument name strings onto the values used by the 
 * SAudioSubscriber.
 *******************************************************************************************/
static void	AddPreloadInstrument( WeaveParamsPtr pb, char* pWord )
	{
	unsigned long	instrumentTag;

	/* Check for an empty slot. Note that we always leave one empty
	 * to guarantee that the list is left NULL terminated.
	 */
	if ( gNextPreloadInstIndex < (DS_HDR_MAX_PRELOADINST - 1) )
		{
		if ( WordCmp( pWord, "sa_44k_16b_s" ) == 0 )
			instrumentTag = SA_44K_16B_S;

		else if ( WordCmp( pWord, "sa_44k_16b_m" ) == 0 )
			instrumentTag = SA_44K_16B_M;

		else if ( WordCmp( pWord, "sa_44k_8b_s" ) == 0 )
			instrumentTag = SA_44K_8B_S;

		else if ( WordCmp( pWord, "sa_44k_8b_m" ) == 0 )
			instrumentTag = SA_44K_8B_M;

		else if ( WordCmp( pWord, "sa_22k_8b_s" ) == 0 )
			instrumentTag = SA_22K_8B_S;

		else if ( WordCmp( pWord, "sa_22k_8b_m" ) == 0 )
			instrumentTag = SA_22K_8B_M;

		else if ( WordCmp( pWord, "sa_44k_16b_s_sdx2" ) == 0 )
			instrumentTag = SA_44K_16B_S_SDX2;

		else if ( WordCmp( pWord, "sa_44k_16b_m_sdx2" ) == 0 )
			instrumentTag = SA_44K_16B_M_SDX2;

		else if ( WordCmp( pWord, "sa_22k_16b_s_sdx2" ) == 0 )
			instrumentTag = SA_22K_16B_S_SDX2;

		else if ( WordCmp( pWord, "sa_22k_16b_m_sdx2" ) == 0 )
			instrumentTag = SA_22K_16B_M_SDX2;

		else 
			instrumentTag = 0;

		/* If we actually found a valid instrument tag, then
		 * add it to the table of tags.
		 */
		if ( instrumentTag != 0 )
			{
			/* Set the new entry to the tag we just assembled */
			pb->preloadInstList[ gNextPreloadInstIndex ] = instrumentTag;

			/* One less free slot in the preloaded instruments table */
			gNextPreloadInstIndex++;
			}
		else
			{
			fprintf( stderr, "### Error: unknown instrument tag: %s\n", pWord );
			gParseError = true;
			}
		}
	}


/*******************************************************************************************
 * Routine to add a subscriber descriptor to the weave param block if there is space.
 * NOTE: we don't update the subscriber index here because the parser is expecting
 * an additional state, the delta priority value, and will update it then.
 *******************************************************************************************/
static void	AddSubscriber( WeaveParamsPtr pb, char* pWord )
	{
	unsigned long	subscriberTag;
	long			index;

	/* Check for an empty slot. Note that we always leave one empty
	 * to guarantee that the list is left NULL terminated.
	 */
	if ( gNextSubscriberIndex < (DS_HDR_MAX_SUBSCRIBER - 1) )
		{
		/* Copy up to 4 characters of subscriber tag string.
		 * Space fill the tag if there are less than 4 characters specified.
		 */
		for ( index = 0; index < 4; index++ )
			if ( *pWord != 0 )
				subscriberTag = (subscriberTag << 8) | *pWord++;
			else
				subscriberTag = (subscriberTag << 8) | ' ';

		/* Set the new entry to the tag we just assembled */
		pb->subscriberList[ gNextSubscriberIndex ].subscriberType = subscriberTag;
		}
	}


/*******************************************************************************************
 * Read a text file and parse word tokens from a file stream.
 *******************************************************************************************/
static void GetTokensFromFile( FILE* fd, CallBackProcPtr callBackProc )
	{
	int			wordBufIndex = 0;
	char		c;
	char		wordBuf[256];

	/****************************************
	 * Loop to read and parse the input text 
	 ****************************************/
	while ((c = getc(fd)) > 0)
		{
		if ( IsWhiteSpace(c) )
			{
			if ( wordBufIndex == 0 )	/* skip consecutive white space */
				continue;

			/* This is a word break call the callback proc and then
			 * Then reset the index to accumulate a new word.
			 */
			wordBuf[wordBufIndex] = 0;	/* terminate string with a NULL */

			/* Call the proc with the token, and exit if
			 * the proc returns FALSE.
			 */
			if ((*callBackProc)(wordBuf) == false)
				return;

			/* Start a new word */
			wordBufIndex = 0;
			
			/* If the terminating white space character is
			 * a newline, then call the callback proc, again,
			 * with just a newline character as a string "\n".
			 */
			if ( c == '\n' )
				{
				wordBuf[0] = '\n';
				wordBuf[1] = 0;
				if ((*callBackProc)(wordBuf) == false)
					return;
				}
			}
		else
			/* Accumulate a non-whitespace character into the
			 * word buffer.
			 */
			{
			wordBuf[wordBufIndex++] = c;
			}
		}

	/* If there's a word accumulated when we reach EOF,
	 * call the callback routine one last time.
	 */
	if ( wordBufIndex != 0 )
		{
		wordBuf[wordBufIndex] = 0;	/* terminate string with a NULL */
		(*callBackProc)(wordBuf);
		}
	}


/*******************************************************************************************
 * Command line parser. Accepts the usual -x unix style switches. Puts parsing
 * results into application global variables which can be pre-initialized to
 * default values. Returns TRUE if command line parsed OK, FALSE if any error.
 *******************************************************************************************/
static Boolean ParseCommandLine ( int argc, char **argv )
	{
	argv++;
	argc--;

	while ( argc > 0 )
		{
		/* Check for the flag that specifies the base (start) time
		 * value for the output stream.
		 */
		if ( strcmp( *argv, BASETIME_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gOutputBaseTime);
			}

		/* Check for the flag that specifies the physical media
		 * data block size. For CDROM, this will be 2048.
		 */
		if ( strcmp( *argv, MEDIABLK_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gMediaBlockSize);
			}

		/* Check for the flag that specifies the stream data
		 * block size. Default for this is 32k bytes.
		 */
		else if ( strcmp( *argv, STREAMBLK_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gStreamBlockSize);
			}


		/* Check for the flag that specifies the stream 
		 * I/O buffer size. Default is 32k bytes.
		 */
		else if ( strcmp( *argv, IOBUFSIZE_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gIOBufferSize);
			}

		/* Check for the flag that specifies the output data file name
		 */
		else if ( strcmp( *argv, OUTDATA_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;

			gOutputStreamName = *argv++;	/* save pointer to file name */
			}

		/* Check for the flag that specifies the max number of markers to allow
		 */
		else if ( strcmp( *argv, MAX_MARKERS_FLAG_STRING ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			sscanf( *argv++, "%li", &gMaxMarkers);
			}

		/* Check for the flag that specifies verbose output
		 */
		else if ( strcmp( *argv, VERBOSE_FLAG_STRING ) == 0 )
			{
			argv++;
			argc--;
			gVerboseFlag = ~gVerboseFlag;
			}

		/* Unknown flag encountered
		 */
		else
			return false;
		}

	return true;
	}


/*******************************************************************************************
 * Routine to display command usage instructions.
 *******************************************************************************************/
static void Usage( char* commandNamePtr )
	{
	fprintf( stderr, "%s version %s\n", commandNamePtr, PROGRAM_VERSION_STRING);
	fprintf( stderr, "usage: %s flags < <scriptfile> \n", commandNamePtr);
	fprintf( stderr, "		%s <size>	I/O buffer size (32k)\n", IOBUFSIZE_FLAG_STRING );
	fprintf( stderr, "		%s <time>		output stream start time\n", BASETIME_FLAG_STRING );
	fprintf( stderr, "		%s <size>		output stream block size\n", STREAMBLK_FLAG_STRING );
	fprintf( stderr, "		%s <size>		output stream media block size\n", MEDIABLK_FLAG_STRING );
	fprintf( stderr, "		%s <file>		output stream file name\n", OUTDATA_FLAG_STRING );
	fprintf( stderr, "		%s <num>		max markers to allow\n", MAX_MARKERS_FLAG_STRING );
	fprintf( stderr, "		%s				toggle verbose diagnostic output\n", VERBOSE_FLAG_STRING );
	}
