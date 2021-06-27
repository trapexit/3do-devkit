/**************************************************************************************
 **	Project:		SquashSnd - Read AIFF and AIFC files, compress 2:1 with 
 **							squareroot-delta-exactencoding and write an AIFC file.  
 **							Throw out all chunks but SSND FORM COMM MARK INST FVER
 **
 **	This File:		SquashSnd.c
 **
 ** Contains:		Main code -- AIFF and AIFF-C Form and chunk handling 
 **
 **	Copyright © 1993 The 3DO Company
 **
 **	All rights reserved. This material constitutes confidential and proprietary 
 **	information of the 3DO Company and shall not be used by any Person or for any 
 **	purpose except as expressly authorized in writing by the 3DO Company.
 **	                        
 **                                                                  
 **  History:                                                        
 **  -------                                                         
 **	 9/12/93	RDG		version 1.2
 **						Spin cursor.
 **						Change -if and -of to -i and -o.  Change -vb to -v.
 **	 6/18/93	RDG		version 1.1
 **						Added call to gestalt manager to check for the existance of an
 **						FPU now that twotoone.c uses inline FPU calls.  
 **	 6/17/93	RDG		version 1.0
 **	 6/15/93	RDG		version .12
 **  6/15/93	RDG		fixed the compression name string in the comm chunk.  I was 
 **						writing a Str255 instead of a C string.
 **  6/15/93	RDG		modify to use MacFileIO routines
 **	 3/10/93	RDG	    Version .11
 **						  -new  support for stereo samples
 **						  -fix  sets the creator and type of the new AIFC file	
 **	 3/10/93	RDG		made a global of the current commchunk so the compression
 **						  module can know about the ssnd chunk's format
 **	 3/9/93		RDG		Modified OpenFiles to set the Finder Creator and Type
 **						for output AIFC file
 **	 2/16/93	RDG		Version .10
 **	 2/11/93	RDG		Started re-write for final version
 **	 2/7/93		RDG		Converted for use with Phil Burk's aiff_tools
 **	 2/4/93		RDG		Converted to Think C for development
 **	 2/1/93		RDG		New Today                                  
 **                                                                  
 **************************************************************************************/

#include "SquashSnd.h"
#include <GestaltEqu.h>
#include <cursorctl.h>

#define	PROGRAM_VERSION_STRING	"1.2"
#define	PROGRAM_DATE_STRING		"09/12/93"

/*********************************************************************
 ** 		ANSI function prototypes                                
 *********************************************************************/

static 	void	usage( void );
static 	Boolean	ParseCommandLine ( int argc, char **argv );
static	long 	OpenFiles(iff_control_ptr in_ptr, iff_control_ptr out_ptr);
static	void 	CloseFiles(iff_control_ptr in_ptr, iff_control_ptr out_ptr);
static  long 	VerifyProcessability(iff_control_ptr iffc_in);
static  long 	CheckFormChunk(iff_control_ptr in_ptr, uint32 *FormType);
static  long 	CheckAIFCFormatVersion(iff_control_ptr in_ptr);
static  long 	CheckSampleFormat(iff_control_ptr in_ptr, uint32 FormType);
static  long 	BeginOutputForm(iff_control_ptr out_ptr);
static  long 	CompleteOutputForm(iff_control_ptr out_ptr);
static  long 	ChewChunks(iff_control_ptr in_ptr, iff_control_ptr out_ptr);

/*********************************************************************
 ** 	Globals                              
 *********************************************************************/
static char*	gProgramNameStringPtr	= NULL;		/* program name string ptr for usage() */
static char*	gOutputFileName			= NULL;		/* default output file name */
static char*	gInputFileName			= NULL;		/* default input file name */
static char *fpu[] = {			
	"<none>",
	"mc68881",
	"mc68882",
	"mc68040 built-in"
};													/* for gestalt manager call */

/*********************************************************************
 **      Begin Main Code                                           
 *********************************************************************/
static void usage(void)
{
	printf("\nVersion %s -- %s\n",PROGRAM_VERSION_STRING, PROGRAM_DATE_STRING);
	printf("Does 2:1 SDX2 compression on AIFF/AIFF-C files.\n");
	printf("Usage: %s [-v] -i <infile> -o <outfile>\n", gProgramNameStringPtr);
	printf( "\t\t-v\t\tverbose mode\n" );
	printf( "\t\t-i\t\tinput AIFF or AIFC filename\n" );
	printf( "\t\t-o\t\toutput compressed AIFC filename\n" );
}

long main(long argc, char *argv[])
{
	long 		Result = 0;
	long		max    = 255;
	long		length = 0;
	long 		gestaltAnswer;
	OSErr 		gestaltErr;
	iff_control iffc_in  = {0,0};
	iff_control iffc_out = {0,0};
	
	/* Copy program name string pointer for the usage() routine
	 */
	gProgramNameStringPtr = argv[0];

	if ( argc < 3 )
		{
		usage();
		return 0;
		}

	/* Parse command line and check results */

	if ( !ParseCommandLine(argc, argv))
		{
		usage();
		return 0;
		}

	if ( gInputFileName == NULL )
		{
		usage();
		printf( "error: input file name not specified\n" );
		return 0;
		}

	if ( gOutputFileName == NULL )
		{
		usage();
		printf( "error: output file name not specified\n" );
		return 0;
		}		
	
	gestaltErr = Gestalt(gestaltVersion,&gestaltAnswer);
	if(!gestaltErr)
		{
		if (gVerbose)
			printf("\nGestalt is available, version %ld...\n",gestaltAnswer);
		
		Gestalt(gestaltFPUType,&gestaltAnswer);
		if (gestaltAnswer == 0)
			{
			printf("\nThis Mac has no FPU!\n");
			printf("\nSquashSnd cannot run without one, sorry!\n");
			return 0;
			}
		else
			{
			if (gVerbose)
				printf("FPU type: %s\n", fpu[gestaltAnswer]);
			}
		}
	else
		{
		printf("Gestalt not available.\n");
		printf("If you don't have an FPU, SquashSnd is about to DIE a horrible death!\n");
		}
	
	/* so we can spin cursor and let the world know we're alive */
	InitCursorCtl(NULL);
		
	printf ("\n   Squashing... ");

	/* we're still here */
	SpinCursor(32);
	
	Result = OpenFiles( &iffc_in, &iffc_out);
	CHECKRESULT(Result,"_main","OpenFiles Failed!!");
	
	Result = VerifyProcessability(&iffc_in);
	CHECKRESULT(Result,"_main","VerifyProcessability Failed!");

	/* we're still here */
	SpinCursor(32);

	Result = BeginOutputForm(&iffc_out);
	CHECKRESULT(Result,"_main", "BeginOutputForm Failed!");

	Result = ChewChunks(&iffc_in, &iffc_out);
	CHECKRESULT(Result,"_main", "ChewChunks Failed!");

	Result = CompleteOutputForm(&iffc_out);
	CHECKRESULT(Result,"_main", "CompleteOutputForm Failed");
	
error:		
	if (Result) {
		printf("\n Compression Aborted!!\n\n");
	}
	else {
		printf(" Done!\n\n");
	}
	
	CloseFiles( &iffc_in, &iffc_out);
	return Result;
}
	
/*********************************************************************************
 * Routine to parse command line arguments
 *********************************************************************************/
#define	PARSE_FLAG_ARG( argString, argFormat, argVariable)	\
	if ( strcmp( *argv, argString ) == 0 )				\
		{												\
		argv++;											\
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

		if ( strcmp( *argv, "-v" ) == 0 )
			{
			argv++;
			if ((argc -= 1) < 0)
				return false;
			gVerbose = true;
			continue;
			}

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
		return false;
		}

	return true;
	}

/*********************************************************************
 ** 	Open input AIFF file and an output AIFC file using filenames 
 **		from argv... 
 **
 **		On error: prints abort message, Closes files and returns  
 *********************************************************************/
static long OpenFiles(iff_control_ptr in_ptr, iff_control_ptr out_ptr)
{
	long Result 	= 0;
	long Creator 	= 'SqSd';
	long Type 		= 'AIFC';

	if (gVerbose) printf("\n\nOpening %s...  ",gInputFileName);
	Result = iff_open_file(in_ptr,gInputFileName);
	CHECKRESULT(Result,"_OpenFiles","iff_open_file_read Failed!");
	if (gVerbose) printf("Done!\n");
			
	if (gVerbose) printf("Creating %s...  ",gOutputFileName);
	Result = iff_create_file(out_ptr, gOutputFileName, Creator, Type );
	CHECKRESULT(Result,"_OpenFiles","iff_open_file_write Failed!");

	if (gVerbose) printf("Done!\n");

	fflush(stdout);
	return Result;
	
error:
	iff_close_file(in_ptr);
	iff_close_file(out_ptr);
	return Result;
}

/*********************************************************************
 ** 	Closes files 
 **
 **		On error: ignores errors from close file  
 *********************************************************************/
static void CloseFiles(iff_control_ptr in_ptr, iff_control_ptr out_ptr)
{
	long Result = 0;
	
	Result = iff_close_file(in_ptr);
	Result = iff_close_file(out_ptr);
}


/*********************************************************************
 ** 	Checks FORM, FVER, and COMM to make sure we can process this
 **		file format.. 
 **
 **		On error: prints error message and returns result code 
 *********************************************************************/
static long VerifyProcessability(iff_control_ptr in_ptr)
{
	long Result = 0;
	uint32 FormType;
	
	Result = CheckFormChunk(in_ptr, &FormType);
	CHECKRESULT(Result,"_VerifyProcessability","CheckFormChunk Failed!");
	
	if (FormType == ID_AIFC) {
		Result = CheckAIFCFormatVersion(in_ptr);
		CHECKRESULT(Result,"_VerifyProcessability","CheckAIFCFormatVersion Failed!");
	}
	
	Result = CheckSampleFormat(in_ptr,FormType);
	CHECKRESULT(Result,"_VerifyProcessability","CheckSampleFormat Failed!");
	
error:
	return Result;	
}


/*********************************************************************
 **		Read the FORM chunk (should be the first header in the AIFF 
 **		file) and check for problems...
 **
 **		On error: prints detailed error messages and returns result code  
 *********************************************************************/
static long CheckFormChunk(iff_control_ptr in_ptr, uint32 *FormType)
{
	long Result = 0;

	Result = iff_read_form(in_ptr,FormType);
		
	if (Result == 3) 
		CHECKRESULT(Result,"_CheckFormChunk","iff_read_form Failed!");
	
	
	if (Result == 2) 
		CHECKRESULT(Result,"_CheckFormChunk","File damaged or not AIFF/AIFF-C!!!");
	

	if ((*FormType != AIFFID) && (*FormType != AIFCID)) 
		CHECKRESULT(true,"_CheckFormChunk","FormType not AIFF or AIFC!!");
	
														
	if (gVerbose) printf("\n\nForm chunk says: Type = %s, Size = %ld\n",TypeToString(*FormType),(in_ptr->iffc_length+4));
	fflush(stdout);
	
error:	
	return Result;
}

/*********************************************************************
 **		Find and read the FVER chunk, check for problems...
 **		Rewind to beginning of local chunks.
 **
 **		On error: prints error messages and returns result code  
 *********************************************************************/
static long CheckAIFCFormatVersion(iff_control_ptr in_ptr)
{
	long Result = 0;
	uint32 ChunkType, ChunkSize;
	FormatVersionChunk myFVC;	

	Result = iff_read_chunk_header(in_ptr, &ChunkType, &ChunkSize);
	CHECKRESULT(Result,"_CheckAIFCFormatVersion","Couldn't read a chunk header!!!");

	while ((ChunkType != FormatVersionID) && (in_ptr->iffc_length > 0))
	{
		Result = iff_skip_chunk_data(in_ptr, ChunkSize);
		CHECKRESULT(Result,"_CheckAIFCFormatVersion","Couldn't skip chunk data!!!");

		Result = iff_read_chunk_header(in_ptr, &ChunkType, &ChunkSize);
		CHECKRESULT(Result,"_CheckAIFCFormatVersion","Couldn't read a chunk header!!!");
	}
		
	if (ChunkType != FormatVersionID) {
		CHECKRESULT(true,"_CheckAIFCFormatVersion","AIFF-C file has no Format Version Chunk!");
	} else {
		Result = iff_read_chunk_data(in_ptr, &myFVC.timestamp, ChunkSize);
		CHECKRESULT(Result,"_CheckSampleFormat","Couldn't read chunk data!!!");

		if (myFVC.timestamp != AIFCVersion1) 
			CHECKRESULT(true,"_CheckAIFCFormatVersion","AIFF-C file version unsupported!");
	
	}
	
	Result = iff_rewind(in_ptr);
	CHECKRESULT(Result,"_CheckAIFCFormatVersion","Couldn't rewind input file!!");
	
error:
	return Result;
}

/*********************************************************************
 **		Find and read the COMM chunk, check for proper number of 
 **		channels, sample size, and make sure the file isn't already
 **		compressed... rewind to beginning of local chunks.
 **
 **		On error: prints error messages and returns result code  
 *********************************************************************/
static long CheckSampleFormat(iff_control_ptr in_ptr, uint32 FormType)
{
	long Result = 0;
	uint32 ChunkType, ChunkSize;

	Result = iff_read_chunk_header(in_ptr, &ChunkType, &ChunkSize);
	CHECKRESULT(Result,"_CheckSampleFormat","Couldn't read a chunk header!!!"); 

	while ((ChunkType != ID_COMM) && (in_ptr->iffc_length > 0))
	{
		Result = iff_skip_chunk_data(in_ptr, ChunkSize);
		CHECKRESULT(Result,"_CheckSampleFormat","Couldn't skip chunk data!!!");

		Result = iff_read_chunk_header(in_ptr, &ChunkType, &ChunkSize);
		CHECKRESULT(Result,"_CheckSampleFormat","Couldn't read a chunk header!!");
	}
								
	if (ChunkType != ID_COMM) 	/* check again cause we could have reached eof */
		CHECKRESULT(true,"_CheckSampleFormat","File has no Common Chunk!!!");

	/* make sure pad is big enough to hold compression description string*/
	if ((ChunkSize - 18) >= (sizeof(uint32)*20)) 	/* string size < pad size */
		CHECKRESULT(true,"_CheckSampleFormat","Compression descriptor in Common Chunk too long!!!");

	Result = iff_read_chunk_data(in_ptr, &theCommChunk.numChannels, ChunkSize);
	CHECKRESULT(Result,"_CheckSampleFormat","Couldn't read chunk data!!!");
	
	if ((theCommChunk.numChannels < 1) || (theCommChunk.numChannels > 2)) 
		CHECKRESULT(true,"_CheckSampleFormat","Only Mono and Stereo samples currently supported!!!");
		
	if (theCommChunk.sampleSize != 16) 
		CHECKRESULT(true,"_CheckSampleFormat","Only 16 bit samples currently supported!!!");
		
	if ((theCommChunk.compressionType != ID_NONE) && (FormType == ID_AIFC))
		CHECKRESULT(true,"_CheckSampleFormat","SSND Chunk already compressed!!!");
	
		
	Result = iff_rewind(in_ptr);
	CHECKRESULT(Result,"_CheckSampleFormat","Couldn't rewind input file!!!");

error:
	return Result;
}

/*********************************************************************
 **		Write a FORM chunk with size = 0 (don't know what it will be 
 **		till after the compression stage), and FORM = AIFF or AIFC... 
 **		The AIFF Read and Write routines keep track of the length
 **		of the file so end_form can write the proper count back to the
 **		FORM chunk.	
 **
 **		On error: prints error messages and returns result code  
 *********************************************************************/
static long BeginOutputForm(iff_control_ptr out_ptr)
{
	long Result = 0;
	
	Result = iff_begin_form(out_ptr,AIFCID);
	CHECKRESULT(Result,"_BeginOutputForm","Problem writing file!!!");

error:	
	return Result;
}

/*********************************************************************
 **		Write the updated size data to the FORM chunk. 
 **		The AIFF Read and Write routines keep track of the length
 **		of the file so end_form can write the proper count back to the
 **		FORM chunk.	
 **
 **		On error: prints error messages and returns result code  
 *********************************************************************/
static long CompleteOutputForm(iff_control_ptr out_ptr)
{
	long Result = 0;

	Result = iff_end_form(out_ptr);
	CHECKRESULT(Result,"_CompleteOutputForm","Problem writing new form length!!!");

error:	
	return Result;
}

/*********************************************************************
 **		Writes a FORM and Format Version chunk and then goes into a loop
 **		which grabs chunk headers and switches based on the chunk
 **		type.  All chunks are tossed out except the SSND chunk
 **		which is compressed, the COMM chunk which is re-written
 **		in AIFC extended format to describe the compression type, the
 **		Marker chunk which is preserved, and the instrument chunk which
 **		is also preserved.	
 **
 **		On error: prints error messages and returns result code  
 *********************************************************************/
#define	ID_SDX2	 MAKE_ID('S','D','X','2')	/* ID for SHayes' Squareroot-Delta-Exact 2:1 compression */
#define AIF_PAD_SIZE 200

static long ChewChunks( iff_control_ptr in_ptr, iff_control_ptr out_ptr)
{
	long Result = 0;
	uint32 ChunkType, ChunkSize;
	uint32 pad[AIF_PAD_SIZE];  		
	ExtCommonChunk myExtCommonChunk;
	const char CompNameString[] = "2:1 Squareroot-Delta-Exact compression.";
	long StrLen = 0;
	
	StrLen = strlen(CompNameString);
	
	/* First, write an AIFC format version chunk */
	pad[0] = AIFCVersion1;
	Result = iff_write_chunk(out_ptr,ID_FVER,pad,4);
	CHECKRESULT(Result,"_ChewChunks","Couldn't write format version chunk!!!");
		 
	while(in_ptr->iffc_length > 0)
	{
		/* we're still here */
		SpinCursor(32);

		Result = iff_read_chunk_header(in_ptr, &ChunkType, &ChunkSize);
		CHECKRESULT(Result,"_ChewChunks","Couldn't read chunk header!!!");
		
		if (gVerbose) printf("\nChewChunks: %s, %lu bytes...\n", TypeToString(ChunkType), ChunkSize);
		fflush(stdout);

		if ((ChunkType != ID_SSND) && (ChunkSize > 0))
		{
			if (ChunkSize > (AIF_PAD_SIZE * sizeof(uint32)))
				CHECKRESULT(true,"_ChewChunks","Some HUGE chunk (not SSND) is to large to chew!!!");
		}
		
		switch(ChunkType)
		{
			case ID_SSND:    /* Does not currently handle non-zero offsets %Q */
				Result = CompressSSND(in_ptr,out_ptr,theCommChunk); 
				CHECKRESULT(Result,"_ChewChunks","CompressSSND Failed!!!!");
		
			break;
				
			case ID_FVER:
				Result = iff_read_chunk_data(in_ptr, pad, ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not read Format Version Data!!!!");
				
				if (pad[0] != AIFCVersion1)
				CHECKRESULT(true,"_ChewChunks","Not a Version1 AIFF-C file!!!");
			break;
			
			case ID_COMM:
				/* pass address of field in COMM chunk to load the rest of the chunk */
				Result = iff_read_chunk_data(in_ptr, &myExtCommonChunk.numChannels, ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not read COMM chunk data!!!");
				
				if (gVerbose) printf("  Updating Common chunk for AIFF-C compatibility....\n");
				fflush(stdout);
				
				myExtCommonChunk.compressionType = ID_SDX2;

				strcpy(myExtCommonChunk.compressionName,CompNameString);
							
				Result = iff_write_chunk(out_ptr,ID_COMM,&myExtCommonChunk.numChannels,
													(ChunkSize+4+(StrLen+1)));
												/* this accounts for the difference between
												    the AIFF input chunk size and the output size
												    - an AIFF-C ExtendedCOMM Chunk...
												 */
				CHECKRESULT(Result,"_ChewChunks","Could not write new COMM new chunk data!!!");
			break;
				
			case ID_MARK:
				Result = iff_read_chunk_data(in_ptr, pad, ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not read Marker chunk data!!!");

				if (gVerbose) printf("  Copying Marker chunk to AIFF-C....\n");
				fflush(stdout);

				Result = iff_write_chunk(out_ptr,ChunkType,pad,ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not write Marker chunk data!!!");
				
			break;

			case ID_INST:
				Result = iff_read_chunk_data(in_ptr, pad, ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not read Instrument chunk data!!!");

				if (gVerbose) printf("  Copying Instrument Chunk to AIFF-C....\n");
				fflush(stdout);

				Result = iff_write_chunk(out_ptr,ChunkType,pad,ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not write Instrument chunk data!!!");
				
			break;
			
			default:
				if (gVerbose) printf("  Tossing out unnecessary chunk....\n");
				fflush(stdout);

				Result = iff_skip_chunk_data(in_ptr, ChunkSize);
				CHECKRESULT(Result,"_ChewChunks","Could not skip some unnecessary chunk!!!");
			break;
		} 
	}
	
error:
	return Result;
}

