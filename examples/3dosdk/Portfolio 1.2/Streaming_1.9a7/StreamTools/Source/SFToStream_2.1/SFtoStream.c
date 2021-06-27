/*******************************************************************************************
 *	File:			SFtoStream.c
 *
 *	Contains:		MPW tool to turn an AIFF or AIFC soundfile into a stream file.
 *
 *	Written by: 	Darren Gibbs
 *
 *	Copyright © 1993 The 3DO Company. All Rights Reserved.
 *
 *	Notes:			This tool will convert AIFF and AIFC soundfiles into streams
 *					Does NOT write blocks, it only writes SSND chunks.
 *
 *	History:
 *	10/31/93	rdg		Version 2.1
 *						Fix chunkTime to start at 1.  So if you weave multiple audio
 *						streams together which start at the same time, all of the headers
 *						will be processed together before any data starts playing.  This
 *						was fixed in an earlier version but the change got lost somehow!!!
 *	10/21/93	rdg		Version 2.0
 *						Fix sampleCount field in sampleDescriptor structure.  Was being 
 *						computed improperly.
 *	10/19/93	jb		Version 1.9
 *						Change command line numeric parsing to accept either decimal
 *						or hexadecimal inputs.
 *	9/12/93		rdg		version 1.8
 *						Spin cursor.
 *						Change input and output file command line flags to -i and -o.
 *	8/25/93		rdg		version 1.7
 *						Alter computation of chunk size to prevent the possibility of yielding
 *						a chunk size greater than the one desired.  Rounding is necessary, but
 *						a case appeared where the net rounding turned out to be up instead of down.
 *	8/19/93		rdg		version 1.6
 *						experimental changes slipped out with v1.5 which caused time stamps to be
 *						screwed up.  Reverted affected code to 1.4.
 *	8/07/93 	ian 	version 1.5
 *						program now exits with zero status for success, non-zero
 *						for failure.  on failure, it deletes any partial output file
 *						that may have been built.  it also spins the cursor now.
 *	7/16/93 	rdg 	version 1.4
 *						add support for larger I/O buffers to speed processing
 *	6/25/93 	rdg 	version 1.3
 *						fix aifc compression type check.
 *	6/25/93 	rdg 	version 1.2
 *						add checks for compression type, sample size, sampling frequency, and
 *						number of channels.  Exit with polite message if not supported.
 *	6/24/93 	rdg 	version 1.1
 *						fix default initial pan value
 *						check for proper AIFF format printed wrong filename
 *	6/17/93 	rdg 	version 1.0
 *	5/28/93 	njc 	BytesPTick was not being adjusted for compressionRatio.
 *	5/28/93 	rdg 	Version .13
 *						Not too pround of the hack to support AIFF and AIFC files directly.
 *						Ended up re-writing the main loop. BlockSize is no longer supported.
 *						SSND chunks are simply written to the output stream and not grouped
 *						into any higher level structure.  ChunkSize is still rounded to the
 *						nearest quadbyte aligned, integral of audio ticks.
 *						It also now supports specifing the logical channel number you want
 *						on the command line.
 *	5/25/93 	rdg 	Version .12
 *						Fixed compression type specification from command line
 *	5/25/93 	rdg 	Version .11
 *	5/25/93 	rdg 	Add actualSampleSize to the SSMPchunkHeader and longword align the chunk.
 *	5/20/93 	rdg 	Version .10
 *						Substantially altered data structures; now writes new timed
 *						chunks with header and data sub chunks.
 *	5/8/93		rdg 	Converted to read headerless sound files and parse
 *						into stream format.
 *	4/17/93 	jb		Add command line arg parsing and usage() routine.
 *	4/17/93 	jb		New today
 *
 *******************************************************************************************/

#define PROGRAM_VERSION_STRING	"2.1"

#include <StdLib.h>
#include <StdIO.h>
#include <Memory.h>
#include <String.h>
#include <Types.h>
#include <CursorCtl.h>
#include <AIFF.h>

/**********************************************/
/* Macro to even up AIF chunk sizes 		  */
/**********************************************/
#define EVENUP(n) if (n & 1) n++

/*****************************************************/
/* Format of a stream data chunk for this subscriber */
/*****************************************************/

#define SUBS_CHUNK_COMMON	\
	long		chunkType;		/*	Subscriber chunk type here	*/	\
	long		chunkSize;											\
	long		time;			/*	position in Audio Time		*/	\
	long		channel;		/*	which logical channel in the stream */	\
	long		subChunkType;	/*	'SHDR' or 'SSMP'    */

typedef struct StreamChunk {
	SUBS_CHUNK_COMMON
} StreamChunk, *StreamChunkPtr;

typedef struct SampleDescriptor
{
	long		dataFormat; 				/* ? from Neil */
	long		sampleSize; 				/* bits per sample, typically 16 */
	long		sampleRate; 				/* 44100, 22050, etc */
	long		numChannels;				/* channels per frame, 1 = mono, 2 = stereo */
	long		compressionType;			/* eg. SDX2 */
	long		compressionRatio;
	long		sampleCount;				/*	total number of samples in sound */
} SampleDescriptor, *SampleDescriptorPtr;

typedef struct SHDRChunk {
	SUBS_CHUNK_COMMON
	long		version;					/*	0 for this version */
	long		numBuffers; 				/*	max # of buffers that can be queued to the AudioFolio	*/
	long		initialAmplitude;			/*	initial volume of the stream */
	long		initialPan; 				/*	initial pan of the stream */
	SampleDescriptor sampleDesc;			/*	info about sound format */
	} SHDRChunk, *SHDRChunkPtr;

typedef struct SSMPChunk {
	SUBS_CHUNK_COMMON
	long		actualSampleSize;			/* actual number of bytes in the sample data */
	char		chunkData[0];				/* start of data */
	} SSMPChunk, *SSMPChunkPtr;


#define 	SAUDIO_STREAM_VERSION_0 	0			/* stream version */

#define 	SNDS_CHUNK_TYPE 			'SNDS'
#define 	SHDR_CHUNK_TYPE 			'SHDR'
#define 	SSMP_CHUNK_TYPE 			'SSMP'
#define 	FILLER_CHUNK_TYPE			'FILL'

#define 	SHDR_CHUNK_HDR_SIZE 		(sizeof(SHDRChunk))
#define 	SSMP_CHUNK_HDR_SIZE 		(sizeof(SSMPChunk))

#define 	COMPRESSION_TYPE_NONE		'NONE'
#define 	COMPRESSION_TYPE_SDX2		'SDX2'

#define 	AUDIO_TICK_RATE 			240
#define 	FRAMES_PER_TICK				184

/*********************************************/
/* Command line variables and their defaults */
/*********************************************/
long	gPropChunkSize			= (16 * 1024L); 		/* stream chunk size */
long	gLogicalChanNum 		= 0;					/* Which logical channel to use for this data */
long	gNumBuffers 			= 8;					/* max # of buffers that can be queued to the AudioFolio	*/
long	gInitialAmplitude		= 0x7500;				/* initial volume of the stream */
long	gInitialPan 			= 0x4000;				/* initial pan of the stream */
char*	gOutputFileName 		= NULL; 				/* no default output file name */
char*	gInputFileName			= NULL; 				/* no default input file name */
long	gIOBufferSize			= (32 * 1024L); 		/* bigger I/O buffers for MPW.	Speeds up processing */

char*	gProgramNameStringPtr	= NULL; 				/* program name string ptr for usage() */

/****************************/
/* Local routine prototypes */
/****************************/
static void 	usage( void );
static char*	GetIOBuffer( long bufferSize );
static Boolean	ParseCommandLine ( int argc, char **argv );


/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
void usage( void )
	{
	printf("Summary: Reads AIFF or AIFF-C soundfiles and writes audio streams\n");
	printf("Usage: %s -i <filename> -o <filename>\n", gProgramNameStringPtr );
	printf("Version %s\n",PROGRAM_VERSION_STRING);
	printf( "\t\t-cs\t\tsize of a stream data chunk in BYTES (16K)\n" );
	printf( "\t\t-lc\t\twhich logical channel to use for this stream (0)\n" );
	printf( "\t\t-nb\t\tnumber of AudioFolio buffers to use (8)\n" );
	printf( "\t\t-ia\t\tinitial amplitude (0x7500)\n" );
	printf( "\t\t-ip\t\tinitial pan (0x4000)\n" );
	printf( "\t\t-i\t\t\tinput file name\n" );
	printf( "\t\t-o\t\t\toutput file name\n" );
	}

/*********************************************************************************
 * Main code to parse AIFF/AIFC chunks and write stream data.
 *********************************************************************************/
int main( int argc, char* argv[] )
	{
	long				result;

	FILE*				inputFile			= NULL;
	FILE*				outputFile			= NULL;
	char*				bufferPtr;

	long				currFilePos;
	long				endSSNDChunkPos;

	ExtCommonChunk		myCommChunk;
	long				AIFFormHeader[3];
	long				AIFChunkHeader[2];
	long				SSNDChunkSize;

	SSMPChunkPtr		dcp;
	SHDRChunkPtr		hcp;

	long				ChunkSize			=	0;
	long				ChunkTime			=	1;		/* want headers at 0 data at 1 */

	long				BytesPFrame 		=	0;
	long				FramesPTick 		=	0;
	long				BytesPTick			=	0;
	long				TicksPChunk 		=	0;
	long				SampleDataSize		=	0;

	long				BytesRead			=	0;
	long				BytesLeft			=	0;

	long				CompressionType 	=	COMPRESSION_TYPE_NONE;
	long				CompressionRatio	=	1;

	InitCursorCtl(NULL);

	/* Copy program name string pointer for the usage() routine
	 */
	gProgramNameStringPtr = argv[0];

	if ( argc < 3 )
		{
		usage();
		goto ERROR_EXIT;
		}

	/* Parse command line and check results */

	if ( !ParseCommandLine(argc, argv))
		{
		usage();
		goto ERROR_EXIT;
		}

	if ( gInputFileName == NULL )
		{
		usage();
		printf( "error: input file name not specified\n" );
		goto ERROR_EXIT;
		}

	if ( gOutputFileName == NULL )
		{
		usage();
		printf( "error: output file name not specified\n" );
		goto ERROR_EXIT;
		}

	/* Open the input and output files */

	inputFile = fopen( gInputFileName, "rb" );
	if ( ! inputFile )
		{
		printf( "error: could not open the input file = %s\n", gInputFileName );
		goto ERROR_EXIT;
		}

	if ( setvbuf( inputFile, GetIOBuffer( gIOBufferSize ), _IOFBF, gIOBufferSize ) )
		{
		printf ( "setvbuf() failed for file: %s\n", gInputFileName );
		goto ERROR_EXIT;
		}

	outputFile = fopen( gOutputFileName, "wb" );
	if ( ! outputFile )
		{
		printf( "error: could not open the output file = %s\n", gOutputFileName );
		goto ERROR_EXIT;
		}

	if ( setvbuf( outputFile, GetIOBuffer( gIOBufferSize ), _IOFBF, gIOBufferSize ) )
		{
		printf ( "setvbuf() failed for file: %s\n", gOutputFileName );
		goto ERROR_EXIT;
		}


	/* Check the AIF form header for the input file */
	BytesRead = fread ((char *) &AIFFormHeader, 1, 12, inputFile);
	if((AIFFormHeader[2] != AIFFID) && (AIFFormHeader[2] != AIFCID))
		{
		printf( "error: %s is not an AIFF or AIFC file\n", gInputFileName );
		goto ERROR_EXIT;
		}

	/* Search for the COMM chunk */
	BytesLeft = AIFFormHeader[1] - 4;  /* subtract the length of the formType field */

	while (BytesLeft > 0)
		{
		SpinCursor(32);

		BytesRead = fread ((char *) &AIFChunkHeader, 1, 8, inputFile);
		BytesLeft -= BytesRead;
		if (AIFChunkHeader[0] != CommonID)
			{
			EVENUP(AIFChunkHeader[1]);									/* round up chunk size to and even number per aif spec */
			result = fseek(inputFile, AIFChunkHeader[1] , SEEK_CUR);	/* point to next chunk header */
			BytesLeft -= AIFChunkHeader[1]; 							/* update size */
			}
		else
			{
			/* read in the COMM chunk data and leave the loop */
			BytesRead = fread ((char *) &myCommChunk.numChannels, 1, sizeof(ExtCommonChunk) - 8, inputFile);
			break;
			}
		}

	/* Figure out compression type and ratio from AIF COMM chunk info */
	if ( (AIFFormHeader[2] == AIFCID) && (myCommChunk.compressionType != COMPRESSION_TYPE_SDX2)
			&& (myCommChunk.compressionType != COMPRESSION_TYPE_NONE) )
		{
		printf( "error: Unrecognized compression type or invalid AIFC header.\n");
		goto ERROR_EXIT;
		}
	else
		/* Figure out compression type and ratio from AIF COMM chunk info */
		if (myCommChunk.compressionType == COMPRESSION_TYPE_SDX2)
			{
			CompressionType = COMPRESSION_TYPE_SDX2;
			CompressionRatio = 2;
			}

	if ( (myCommChunk.sampleSize != 8) && (myCommChunk.sampleSize != 16) )
		{
		printf( "error: Sample size must be 8 or 16 bits.\n");
		goto ERROR_EXIT;
		}

	if ( ((long)myCommChunk.sampleRate != 44100) && ((long)myCommChunk.sampleRate != 22050) )
		{
		printf( "error: Sample rate must be 44100 or 22050.\n");
		goto ERROR_EXIT;
		}

	if ( ((long)myCommChunk.numChannels != 1) && ((long)myCommChunk.numChannels != 2) )
		{
		printf( "error: Sample must be mono or stereo.\n");
		goto ERROR_EXIT;
		}

	/* Figure out actual stream chunk size and audio ticks per chunk from AIF COMM chunk info */
 
	BytesPFrame 	=	((myCommChunk.sampleSize/8) * myCommChunk.numChannels);
	
	FramesPTick		=	(myCommChunk.sampleRate / AUDIO_TICK_RATE) + 1;   /* round up to match AudioFolio */

	BytesPTick		=	((BytesPFrame * FramesPTick) / CompressionRatio); 
	
	SampleDataSize	=	(gPropChunkSize - ( (gPropChunkSize - SSMP_CHUNK_HDR_SIZE) % BytesPTick) );
		
	TicksPChunk		=	SampleDataSize / BytesPTick;
	
	ChunkSize		=	(SSMP_CHUNK_HDR_SIZE + SampleDataSize + 3) & ~3;		/* round chunk size up to quad byte */
	
	/* Now Search for the SSND chunk */
	result = fseek(inputFile, 12, SEEK_SET);	/* point back to first chunk after FORM chunk */
	BytesLeft = AIFFormHeader[1] - 4;			/* reset size since we're pointing back to the beginning */

	while (BytesLeft > 0)
		{
		SpinCursor(32);

		BytesRead = fread ((char *) &AIFChunkHeader, 1, 8, inputFile);
		BytesLeft -= BytesRead;
		if (AIFChunkHeader[0] != SoundDataID)
			{
			EVENUP(AIFChunkHeader[1]);
			result = fseek(inputFile, AIFChunkHeader[1] , SEEK_CUR);
			BytesLeft -= AIFChunkHeader[1];
			}
		else
			{
			result = fseek(inputFile, 8 , SEEK_CUR);	/* skip to the beginning of the sample data */
			SSNDChunkSize = AIFChunkHeader[1] - 8;		/* account for offset and blocksize fields */
			BytesLeft = SSNDChunkSize;					/* set up BytesLeft for reading just the sample data */
			break;
			}
		}


	/* Allocate the I/O buffer */

	bufferPtr = malloc( ChunkSize );
	if ( bufferPtr == 0 )
		{
		printf( "error: could not allocate I/O block memory size = %ld\n", ChunkSize );
		goto ERROR_EXIT;
		}

	/*******************************************************************************************
	 * Write a header chunk into the output stream file using our AIFF/C header data
	 ******************************************************************************************/

	hcp = (SHDRChunkPtr) bufferPtr;

	hcp->chunkType						= SNDS_CHUNK_TYPE;
	hcp->chunkSize						= sizeof(SHDRChunk);
	hcp->time							= 0;							/* this is setup data */
	hcp->channel						= gLogicalChanNum;
	hcp->subChunkType					= SHDR_CHUNK_TYPE;
	hcp->version						= SAUDIO_STREAM_VERSION_0;
	hcp->numBuffers 					= gNumBuffers;					/* max # of buffers that can be queued to the AudioFolio	*/
	hcp->initialAmplitude				= gInitialAmplitude;			/* initial volume of the stream */
	hcp->initialPan 					= gInitialPan;					/* initial pan of the stream */
	hcp->sampleDesc.dataFormat			= 0;							/* currently unused */
	hcp->sampleDesc.sampleSize			= myCommChunk.sampleSize;		/* bits per sample, typically 16 */
	hcp->sampleDesc.sampleRate			= (long)myCommChunk.sampleRate; /* 44100 or 22050 */
	hcp->sampleDesc.numChannels 		= myCommChunk.numChannels;		/* channels per frame, 1 = mono, 2=stereo */
	hcp->sampleDesc.compressionType 	= CompressionType;				/* eg. SDX2 */
	hcp->sampleDesc.compressionRatio	= CompressionRatio;
	/* Total number of samples in sound file:
	 * Number of bytes in AIFF/AIFC SSND chunk i.e. (SSNDChunkSize)
	 * divided by number of bytes per sample i.e. ((myCommChunk.sampleSize / CompressionRatio) / 8)
	 */
	hcp->sampleDesc.sampleCount 		= ( SSNDChunkSize / ((myCommChunk.sampleSize / CompressionRatio) / 8) ); 
	
	if (1 != fwrite( bufferPtr, sizeof(SHDRChunk), 1, outputFile ) )
		{
		printf("error: header chunk write failed!\n");
		goto ERROR_EXIT;
		}

	/*******************************************************************************************
	 * Read in samps, format into stream chunks, and write into stream file.
	 ******************************************************************************************/
	currFilePos = ftell(inputFile); 				/* figure out where we are */
	endSSNDChunkPos = currFilePos + BytesLeft;		/* store end of sample data */

	while (currFilePos < endSSNDChunkPos)
		{
		SpinCursor(32);

		dcp = (SSMPChunkPtr) bufferPtr;

		/* Init the SSMP chunk header */
		dcp->chunkType			= SNDS_CHUNK_TYPE;
		dcp->chunkSize			= ChunkSize;
		dcp->time				= ChunkTime;
		dcp->channel			= gLogicalChanNum;
		dcp->subChunkType		= SSMP_CHUNK_TYPE;

		BytesRead = fread (&dcp->chunkData[0], 1, SampleDataSize, inputFile);
		dcp->actualSampleSize = BytesRead;
		currFilePos += BytesRead;

		/* Update the time */
		ChunkTime += TicksPChunk;

		if (currFilePos < endSSNDChunkPos)		/* normal chunk  */
			{
			if (1 != fwrite( bufferPtr, ChunkSize, 1, outputFile ) )
				{
				printf("error: data chunk write failed!\n");
				goto ERROR_EXIT;
				}
			}
		else
			{
			/* figure out how many bytes of samples we actually got */
			dcp->actualSampleSize = BytesRead - (currFilePos - endSSNDChunkPos);
			if (1 != fwrite( bufferPtr, ChunkSize, 1, outputFile ) ) /* out of data */
				{
				printf("error: data chunk write failed!\n");
				goto ERROR_EXIT;
				}
			}
		}

		fclose(inputFile);
		fclose(outputFile);

		return 0;	// successfull completion

ERROR_EXIT:

		if (inputFile) {
			fclose(inputFile);
		}

		if (outputFile) {
			fclose(outputFile);
		}

		remove(gOutputFileName);

		return 1;

	} /* main */


/*******************************************************************************************
 * Routine to allocate alternate I/O buffers for the weaving process. Bigger is better!
 *******************************************************************************************/
static char*	GetIOBuffer( long bufferSize )
	{
	char*	ioBuf;

	ioBuf = (char*) NewPtr( bufferSize );
	if ( ioBuf == NULL )
		{
		fprintf( stderr, "GetIOBuffer() - failed. Use larger MPW memory partition!\n" );
		fprintf( stderr, "                Use Get Info... from the Finder to set it.\n" );
		}

	return ioBuf;
	}

/*********************************************************************************
 * Routine to parse command line arguments
 *********************************************************************************/
#define PARSE_FLAG_ARG( argString, argFormat, argVariable)	\
	if ( strcmp( *argv, argString ) == 0 )				\
		{												\
		argv++; 										\
		if ((argc -= 2) < 0)							\
			return false;								\
		sscanf( *argv++, argFormat, &argVariable);		\
		continue;										\
		}

static Boolean	ParseCommandLine ( int argc, char **argv )
	{
	/* Skip past the program name */
	argv++;
	argc--;

	while ( argc > 0 )
		{
		PARSE_FLAG_ARG( "-cs", "%li", gPropChunkSize );

		PARSE_FLAG_ARG( "-lc", "%li", gLogicalChanNum );

		PARSE_FLAG_ARG( "-nb", "%li", gNumBuffers );

		PARSE_FLAG_ARG( "-ia", "%li", gInitialAmplitude );

		PARSE_FLAG_ARG( "-ip", "%li", gInitialPan );

		PARSE_FLAG_ARG( "-iobs", "%li", gIOBufferSize );

		if ( strcmp( *argv, "-i" ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gInputFileName = *argv++;
			continue;
			}

		if ( strcmp( *argv, "-o" ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gOutputFileName = *argv++;
			continue;
			}

		/* Unknown flag encountered
		 */
		else
			return false;
		}

	return true;
	}
