/**************************************************************************************
 *	Project:		DumpAIFF -	An MPW Tool to print formatted output for SSND FORM 
 *								COMM MARK INST and FVER chunks so it's easy to see 
 *								what an AIFF/AIFF-C file contains.
 *
 *	This File:		DumpAIFF.c
 *
 *
 *	Copyright © 1993 The 3DO Company
 *
 *	All rights reserved. This material constitutes confidential and proprietary 
 *	information of the 3DO Company and shall not be used by any Person or for any 
 *	purpose except as expressly authorized in writing by the 3DO Company.
 *
 *
 * History:
 * -------
 *	9/12/93		RDG		Version 1.2
 *						Add SpinCursor()
 *						Explicitly close input file.
 *						No longer return 0 on error... return 1.
 *						Make sure all chunk sizes are rounded up in file position calculations.
 *						Fix off by 4 problem when reading container chunk and calculating
 *						 bytesLeft.
 *						Check for unrecognized/unsupported chunkTypes.
 *						Check for corrupt/invalid FORM size.  This will detect the invalid
 *						 AIFC files that an early version of SoundHack was writing.
 *						Change command line flag for input file from -if to -i.
 *	8/6/93		RDG		Version 1.1
 *						Add "Ptr" typedefs to match definitions in AIFF.h
 *	7/23/93		RDG		Version 1.0
 *	7/23/93		RDG		New today
 *
 *******************************************************************************************/

#define	PROGRAM_VERSION_STRING	"1.2"
#define	PROGRAM_DATE_STRING		"9/12/93"

#include <StdLib.h>
#include <StdIO.h>
#include <Memory.h>
#include <String.h>
#include <Types.h>
#include <AIFF.h>
#include <cursorctl.h>

typedef unsigned long  ulong;

/******************************************************/
/* Define "Ptr" types to match the AIFF.h definitions */
/******************************************************/
typedef ContainerChunk *ContainerChunkPtr;
typedef FormatVersionChunk *FormatVersionChunkPtr;
typedef CommonChunk *CommonChunkPtr;
typedef ExtCommonChunk *ExtCommonChunkPtr;
typedef SoundDataChunk *SoundDataChunkPtr;
typedef MarkerChunk *MarkerChunkPtr;
typedef InstrumentChunk *InstrumentChunkPtr;

/* My format for the universal IFF identifiers. */
#define	ID_FORM		'FORM'		/* Form of file */
#define	ID_AIFF		'AIFF'		/* form type */
#define	ID_AIFC		'AIFC'		/* form type */
#define	ID_FVER		'FVER'		/* Format Version for AIFC */
#define	ID_SSND		'SSND'		/* sample data */
#define	ID_MARK		'MARK'		/* markers */
#define	ID_INST		'INST'		/* Instrument data */
#define	ID_COMM		'COMM'		/* Common */
#define ID_MIDI		'MIDI'		/* Midi data */
#define ID_APPL		'APPL'		/* Application specific */
#define ID_COMT		'COMT'		/* Comment */
#define ID_NAME		'NAME'		/* name */
#define ID_AUTH 	'AUTH'		/* author */
#define ID_COPY 	'(c) '		/* copyright */
#define ID_ANNO		'ANNO'		/* annotation

/* Macro for evening chunk size */
#define EVENUP(n) if (n & 1) n++


/*********************************************************************
 * 		ANSI function prototypes
 *********************************************************************/
static void		usage( void );
static Boolean	ParseCommandLine ( int argc, char **argv );

/*********************************************************************
 ** 	Globals
 *********************************************************************/
static char*	gProgramNameStringPtr	= NULL;		/* program name string ptr for usage() */
static char*	gInputFileName			= NULL;		/* default input file name */


/*********************************************************************************
 * Routine to output command line help
 *********************************************************************************/
static void 	usage( void )
	{
	printf("Summary: Prints formatted output of AIFF and AIFF-C chunk data.\n");
	printf("Usage: %s -i <filename>\n", gProgramNameStringPtr );
	printf("Version %s, %s\n",PROGRAM_VERSION_STRING, PROGRAM_DATE_STRING );
	printf( "\t\t-i\t\tinput file name\n" );
	}

/*********************************************************************************
 * Main code to parse AIFF/AIFC chunks and write stream data.
 *********************************************************************************/
int	main( int argc, char* argv[] )
	{
	long	result			=	0;

	FILE*	inputFile		=	NULL;

	char*	chunkPadPtr		=	NULL;
	long	chunkPadSize	=	1024;
	
	long	evenChunkSize	=	0;

	long	bytesRead		=	0;
	long	bytesLeft		=	0;
	long	currFilePos		=	0;

	ulong	formType 		= 	0;

	ChunkHeader				ch;
	ContainerChunkPtr		fcp;
	FormatVersionChunkPtr	fvcp;
	CommonChunkPtr			ccp;
	ExtCommonChunkPtr		eccp;
	SoundDataChunkPtr		sdcp;
	MarkerChunkPtr			mcp;
	InstrumentChunkPtr		icp;


	/* Copy program name string pointer for the usage() routine */
	gProgramNameStringPtr = argv[0];

	if ( argc < 3 )
		{
		usage();
		goto error;
		}

	/* Parse command line and check results */
	if ( !ParseCommandLine(argc, argv))
		{
		usage();
		goto error;
		}

	if ( gInputFileName == NULL )
		{
		usage();
		fprintf( stderr, "\nError: input file name not specified\n" );
		goto error;
		}

	/* We want to spin cursor later */
	InitCursorCtl(NULL);

	/* Open the input file */
	inputFile = fopen( gInputFileName, "rb" );
	if ( ! inputFile )
		{
		fprintf( stderr, "\nError: could not open the input file = %s\n\n", gInputFileName );
		goto error;
		}

	/* Allocate a buffer to use for chunk parsing */
	chunkPadPtr = malloc( chunkPadSize );
	if ( chunkPadPtr == 0 )
		{
		fprintf( stderr, "\nError: could not allocate a buffer of size = %ld\n\n", chunkPadSize );
		goto error;
		}

	/* Check the AIFF FORM header for the input file */
	bytesRead = fread ( chunkPadPtr, 1, 12, inputFile );
	if ( bytesRead != 12 )
		{
		fprintf( stderr, "\nError: Couldn't read a chunk header from %s.\n", gInputFileName );
		fprintf( stderr, "       Possibly due to an invalid FORM size or file/disk error.\n\n");
		goto error;
		}

	/* Check the AIFF FORM header (also called the Container chunk). */
	fcp = (ContainerChunkPtr) chunkPadPtr;
	if((fcp->formType == ID_AIFF) || (fcp->formType == ID_AIFC))
		{
		fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &fcp->ckID, fcp->ckSize);
		fprintf( stdout, "\t\tFormType = %.4s\n", &fcp->formType);
		fflush( stdout );
		bytesLeft = fcp->ckSize - 4;  /* the - 4 accounts for the FormType field */
		}
	else
		{
		fprintf( stderr, "\nError: %s is damaged or not an AIFF/AIFF-C file\n\n", gInputFileName );
		goto error;
		}

	/* Remember which form type this is so we can parse the rest of the chunks properly */
	formType = fcp->formType;

	while( bytesLeft > 0 )
		{
		SpinCursor(32);
	
		/* Get a chunk header from the input file */
		bytesRead = fread ( &ch, 1, 8, inputFile );
		if ( bytesRead != 8 )
			{
			fprintf( stderr, "\nError: Couldn't read a chunk header from %s.\n", gInputFileName );
			fprintf( stderr, "       Possibly due to an invalid FORM size or file/disk error.\n\n");
			goto error;
			}

		/* Get a copy of the chunk size and round it up for file positioning calculations */
		evenChunkSize = ch.ckSize;
		EVENUP(evenChunkSize);	

		/* If the number of bytes left in the file (calculated from the FORM size) is
		 * less than the size of the current chunk plus it's header, than the FORM size
		 * or the chunk size is screwed.  This is not trapped by default so this checks
		 * explicitly.
		 */
		if ( bytesLeft < (evenChunkSize + 8) )
			{
			fprintf( stderr, "\nError: FORM size or CHUNK size corrupt! \n");
			fprintf( stderr, "       Current chunk: %.4s chunk, Size (including header) = %ld\n", 
									&ch.ckID, (evenChunkSize + 8) );
			fprintf( stderr, "       FORM size says only %ld bytes left in file!\n\n", bytesLeft);
			goto error;
			}

		switch( ch.ckID )
			{
			case ID_FVER:
				if ( formType == ID_AIFF )
					{
					fprintf( stderr, "\nError: %s is supposed to be an AIFF file but contains an FVER chunk.\n\n", gInputFileName );
					goto error;
					}

				fvcp = (FormatVersionChunkPtr) chunkPadPtr;
				bytesRead = fread( &fvcp->timestamp, 1, ch.ckSize, inputFile);
				if ( bytesRead != ch.ckSize )
					{
					fprintf( stderr, "\nError: Couldn't read the Format Version Chunk from %s.\n\n", gInputFileName );
					goto error;
					}

				fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &ch.ckID, ch.ckSize);
				fprintf( stdout, "\t\tTimestamp = %lu\n", fvcp->timestamp);

				if ( fvcp->timestamp != AIFCVersion1 )
					fprintf( stdout, "\n\t\tWarning, %s is not a Version1 AIFF-C file!\n\n", gInputFileName );
			break;

			case ID_COMM:
				ccp = (CommonChunkPtr) chunkPadPtr;
				bytesRead = fread( &ccp->numChannels, 1, ch.ckSize, inputFile);
				if ( bytesRead != ch.ckSize )
					{
					fprintf( stderr, "\nError: Couldn't read the Common Chunk from %s.\n\n", gInputFileName );
					goto error;
					}

				fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &ch.ckID, ch.ckSize);
				fprintf( stdout, "\t\tNumber of Channels = %i\n", ccp->numChannels);
				fprintf( stdout, "\t\tNumber of Sample Frames = %lu\n", ccp->numSampleFrames);
				fprintf( stdout, "\t\tSample Size = %i\n", ccp->sampleSize);
				fprintf( stdout, "\t\tSample Rate = %f\n", ccp->sampleRate);

				if ( formType == ID_AIFC )
					{
					eccp = (ExtCommonChunkPtr) chunkPadPtr;
					fprintf( stdout, "\t\tCompression Type = %.4s\n", &eccp->compressionType);
					}

			break;

			case ID_SSND:
				sdcp = (SoundDataChunkPtr) chunkPadPtr;
				fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &ch.ckID, ch.ckSize);
				fseek( inputFile, evenChunkSize, SEEK_CUR );

			break;

			case ID_MARK:
				mcp = (MarkerChunkPtr) chunkPadPtr;
				bytesRead = fread( &mcp->numMarkers, 1, ch.ckSize, inputFile);
				if ( bytesRead != ch.ckSize )
					{
					fprintf( stderr, "\nError: Couldn't read the Marker Chunk from %s.\n\n", gInputFileName );
					goto error;
					}

				fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &ch.ckID, ch.ckSize);
				fprintf( stdout, "\t\tNumber of Markers = %i\n", mcp->numMarkers);

			break;

			case ID_INST:
				icp = (InstrumentChunkPtr) chunkPadPtr;
				bytesRead = fread( &icp->baseFrequency, 1, ch.ckSize, inputFile);
				if ( bytesRead != ch.ckSize )
					{
					fprintf( stderr, "\nError: Couldn't read the Instrument Chunk from %s.\n\n", gInputFileName );
					goto error;
					}

				fprintf( stdout, "\t%.4s chunk, Size = %ld\n", &ch.ckID, ch.ckSize);
				fprintf( stdout, "\t\tBase Frequency = %ld\n", (long)icp->baseFrequency);
				fprintf( stdout, "\t\tDetune = %ld\n", (long)icp->detune);
				fprintf( stdout, "\t\tLow Frequency = %ld\n", (long)icp->lowFrequency);
				fprintf( stdout, "\t\tHigh Frequency = %ld\n", (long)icp->highFrequency);
				fprintf( stdout, "\t\tLow Velocity = %ld\n", (long)icp->lowVelocity);
				fprintf( stdout, "\t\tHigh Velocity = %ld\n", (long)icp->highVelocity);
				fprintf( stdout, "\t\tGain = 0x%lx\n", (long)icp->gain);

			break;

			default:
			
				if ( (ch.ckID != ID_FORM) && 
					(ch.ckID != ID_AIFF) &&
					(ch.ckID != ID_AIFC) &&
					(ch.ckID != ID_FVER) &&
					(ch.ckID != ID_SSND) &&
					(ch.ckID != ID_MARK) &&
					(ch.ckID != ID_INST) &&
					(ch.ckID != ID_COMM) &&
					(ch.ckID != ID_MIDI) &&
					(ch.ckID != ID_APPL) &&
					(ch.ckID != ID_COMT) &&
					(ch.ckID != ID_NAME) &&
					(ch.ckID != ID_AUTH) &&
					(ch.ckID != ID_COPY) &&
					(ch.ckID != ID_ANNO) )
					{
					fprintf( stdout, "\nWarning: Unrecognized chunk type.\n" );
					fprintf( stdout, "\tApparent type: %.4s, Size = %ld.\n", &ch.ckID, ch.ckSize);
					fprintf( stdout, "\tAttempting to skip chunk...\n\n");
					fseek( inputFile, evenChunkSize, SEEK_CUR );
					}
				else
					{
					fprintf( stdout, "\tSkipping chunk... type: %.4s, Size = %ld.\n", &ch.ckID, ch.ckSize);
					fseek( inputFile, evenChunkSize, SEEK_CUR );
					}
					
			break;

			}

		bytesLeft = bytesLeft - ( evenChunkSize + 8 );  /* the + 8 accounts for each chunk header */

		fflush( stdout );
		fflush( stderr );
		}
		
	/* Successful completion */	
	fclose( inputFile );
	return 0;
		
error:
	if ( inputFile ) 
		fclose( inputFile );

	return 1;
}


/*******************************************************************************************
 * Routine to parse command line arguments.
 *******************************************************************************************/
static Boolean	ParseCommandLine ( int argc, char **argv )
	{
	/* Skip past the program name */
	argv++;
	argc--;

	while ( argc > 0 )
		{
		if ( strcmp( *argv, "-i" ) == 0 )
			{
			argv++;
			if ((argc -= 2) < 0)
				return false;
			gInputFileName = *argv++;
			continue;
			}

		/* Unknown flag encountered
		 */
		else
			return false;
		}
	return true;
	}