
#include "Portfolio.h"
#include "Init3DO.h"
#include "Utils3DO.h"
#include "UMemory.h"

#include "DataStreamDebug.h"
#include "DataStreamLib.h"
#include "DataAcq.h"

#include "SAnimSubscriber.h"
#include "SAudioSubscriber.h"
#include "ControlSubscriber.h"
#include "PrepareStream.h"
#include "DSStreamHeader.h"

#define	PROGRAM_VERSION_STRING		"2.0"

/* The following compile switches control various parameters for initializing
 * and creating the streaming environment.
 */
#define	NUMBER_OF_STREAMS			1
#define	NUM_DIRECTOR_CHANNELS		24

/* Setting the following compile switch will cause cels that would appear offscreen to
 * be centered on the screen if the X or Y position exceed the following maxima.
 */
#define FORCE_CELS_ONTO_SCREEN		0
#define	MAX_SAFE_X_POS				(320L << 16)
#define	MAX_SAFE_Y_POS				(240L << 16)
#define	MIN_SAFE_X_POS				(0L << 16)
#define	MIN_SAFE_Y_POS				(0L << 16)

/*************************************/
/* Handy macro for diagnostic output */
/*************************************/
#ifndef CHECKRESULT
#	define CHECKRESULT( name, result )			\
		if ( ((long) result) < 0 )				\
			{ 									\
			printf( "Failure in %s: %d ($%x)\n", name, ((long) result), ((long) result) ); \
			PrintfSysErr( result );				\
			exit( 0 );							\
			}
#endif

/* If true, this will cause a closechannel message to be sent to
   the audio subscriber when the stream is switched, or rewound
   with the "C" button.
 */
#define CLOSE_AUDIO_ON_SWITCH	1

/* Stream timer defines */
enum
{
	kInvalidClock = -1
};


/*********************************************************************************************
 * Library and system globals
 *********************************************************************************************/
ubyte*			gBackgroundPtr		= NULL;
long 			gScreenSelect		= 0;
 ScreenContext	gScreenContext;

/***********************************/
/* Stream related global variables */
/***********************************/
typedef struct SAnimPlayer {
	DSHeaderChunk		hdr;					/* Copy of stream header from stream file */

	DSDataBufPtr		bufferList;
	AcqContextPtr		acqContext;
	DSStreamCBPtr		streamCBPtr;
	
	ScreenContext		*screenContextPtr;
	Item				VBLIOReq;
	
	Item				messagePort;
	Item				messageItem;
	
	long				gAudioChannel;

	CtrlContextPtr		controlContextPtr;		/* control subscriber context ptr */
	SAudioContextPtr	audioContextPtr;		/* audio context */

	SAnimContextPtr		sanimContextPtr;		/* ANIM context ptr	*/
	SAnimRecPtr			sanimChannelPtr[ NUM_DIRECTOR_CHANNELS ];
	long 				*rawBufferList;
	} SAnimPlayer, *SAnimPlayerPtr;

extern	Boolean StartUp( void );
extern	void	EraseScreen( ScreenContext *sc, int32 screenNum );
static 	int32	InitSAnimPlayerFromStreamHeader( SAnimPlayerPtr ctx, char* streamFileName);
int32	PlaySAnimStream( ScreenContext *screenContextPtr, char* streamFileName);
void	DismantlePlayer( SAnimPlayerPtr ctx );
static DSDataBufPtr	CreateBufferList( long numBuffers, long bufferSize, SAnimPlayerPtr ctx  );
static int32	SetupSAnimSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr );
static int32	SetupAudioSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr );
static int32	SetupControlSubscriber( SAnimPlayerPtr ctx, DSHeaderSubsPtr subsPtr );
