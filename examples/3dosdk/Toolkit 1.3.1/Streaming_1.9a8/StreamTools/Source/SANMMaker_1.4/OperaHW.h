/*
 *
 */

typedef struct {
	unsigned int skipThisSprite				: 1;	// 31
	unsigned int endOfList					: 1;	// 30
	unsigned int nextPtrIsAbsolute			: 1;	// 29
	unsigned int dataPtrIsAbsolute			: 1;	// 28
	unsigned int palettePtrIsAbsolute		: 1;	// 27
	unsigned int loadDX_DY					: 1;	// 26
	unsigned int loadDDX_DDY				: 1;	// 25
	unsigned int loadPPMPControlWord		: 1;	// 24
	unsigned int loadPalette				: 1;	// 23
	unsigned int preambleWithSpriteCB		: 1;	// 22
	unsigned int writeXY					: 1;	// 21
	unsigned int ACSC						: 1;	// 20
	unsigned int ALSC						: 1;	// 19
	unsigned int acw						: 1;	// 18
	unsigned int accw						: 1;	// 17
	unsigned int terminateIfWrongDirection	: 1;	// 16
	unsigned int lockstepCornerEngines		: 1;	// 15
	unsigned int enable2ndCornerEngine		: 1;	// 14
	unsigned int allowSuperClipping			: 1;	// 13
	unsigned int maria						: 1;	// 12
	unsigned int enableXOR					: 1;	// 11
	unsigned int useav						: 1;	// 10
	unsigned int packed						: 1;	//  9
	unsigned int overrideDMode				: 2;	//  7-8
	unsigned int subposFromPaletteEntry		: 1;	//  6
	unsigned int background					: 1;	//  5
	unsigned int noBlack					: 1;	//  4
	unsigned int paletteAddressBits			: 4;	//  0-3
} SpriteFlags;

typedef struct {
	unsigned int pixelSource			: 1;	// 15
	unsigned int multiplierSelect		: 2;	// 13-14
	unsigned int multiplyConst			: 3;	// 10-12
	unsigned int divideConst			: 2;	//  8-9
	unsigned int adderSelect			: 2;	//  6-7
	union {
		unsigned int addConst			: 5;	//  1-5
		struct {
			unsigned int divideConst2	: 2;	//  4-5
			unsigned int allowWrap		: 1;	//  3
			unsigned int enableSignx	: 1;	//  2
			unsigned int subtract2nd	: 1;	//  1
		} f;
	} u;
	unsigned int secondDivideConst		: 1;	//  0
} aPPMPControlWordHalf;

typedef struct {
	unsigned int pixelSource			: 1;	// 15
	unsigned int multiplierSelect		: 2;	// 13-14
	unsigned int multiplyConst			: 3;	// 10-12
	unsigned int divideConst			: 2;	//  8-9
	unsigned int adderSelect			: 2;	//  6-7
	unsigned int divideConst2			: 2;	//  4-5
	unsigned int allowWrap				: 1;	//  3
	unsigned int enableSignx			: 1;	//  2
	unsigned int subtract2nd			: 1;	//  1
	unsigned int secondDivideConst		: 1;	//  0
} AV_PPMPControlWordHalf;

typedef struct {
	unsigned int pixelSource			: 1;	// 15
	unsigned int multiplierSelect		: 2;	// 13-14
	unsigned int multiplyConst			: 3;	// 10-12
	unsigned int divideConst			: 2;	//  8-9
	unsigned int adderSelect			: 2;	//  6-7
	unsigned int addConst				: 5;	//  1-5
	unsigned int secondDivideConst		: 1;	//  0
} PPMPControlWordHalf;

typedef struct {
	PPMPControlWordHalf	dModeOn;
	PPMPControlWordHalf dModeOff;
} PPMPControlWord;

typedef enum {
	kDivideBy16 = 0,
	kDivideBy2 = 1,
	kDivideBy4 = 2,
	kDivideBy8 = 3
} DivideCode;

typedef enum {
	kMultBySpriteControlBlockValue = 0,
	kMultByPenInputNumber = 1,
	kMultByInputPenNumber = 2
} MultiplierSelectorCode;

typedef enum {
	kInputPenNumberData = 0,
	kCurrentFrameBufferData = 1
} DataSelectorCode;

typedef enum {
	kAddZero = 0,
	kAddSpriteControlBlockValue = 1,
	kAddCurrentFrameBufferData = 2,
	kAddInputPenNumber = 3
} AdderSelectorCode;

typedef enum {
	kFinalDivideBy2 = 0,
	kFinalDivideBy4 = 1,
	kFinalDivideBy8 = 2,
	kFinalDivideBy16 = 3
} FinalDivideSelector;

typedef struct SpriteControlBlock* SpritePtr;

typedef struct {
	SpriteFlags		flags;			// control flags
	SpritePtr		nextSpritePtr;	// ptr to next sprite in list
	void*			imageDataPtr;	// ptr to image data for this sprite
	void*			palettePtr;		// ptr to palette for this sprite
	unsigned long	xPos;			// x-coordinate of this sprite
	unsigned long	yPos;			// y-coordinate of this sprite
	unsigned long	dx;				// horizontal increment for first line
	unsigned long	dy;				// vertical increment for first line
	unsigned long	linedx;			// horizontal increment from line to line
	unsigned long	linedy;			// vertical increment from line to line
	unsigned long	ddx;			// per-line dx increment
	unsigned long	ddy;			// per-line dy increment
	unsigned long	PPMPControl;	// PPMP control word
} SpriteControlBlock;

typedef struct {
	unsigned int 					: 16;	// 16-31
	unsigned int scanLineCount		: 10;	//  6-15
	unsigned int 					: 1;	//  5
	unsigned int directPixels		: 1;	//  4
	unsigned int replicateDirect8	: 1;	//  3
	unsigned int bitsPerPixel		: 3;	//  0-2
} SpritePreambleWord0;

typedef enum {	k1BitPixels = 1,
				k2BitPixels = 2,
				k4BitPixels = 3,
				k6BitPixels = 4,
				k8BitPixels = 5,
				k16BitPixels = 6
} BitsPerPixelCode;

typedef struct {
	unsigned int scanLineSizeInWords	: 8;	// 24-31
	unsigned int						: 8;	// 16-25 Thanks, hardware guys!
	unsigned int						: 1;	// 15
	unsigned int disableSwapHV			: 1;	// 14
	unsigned int blueLSBSource			: 2;	// 12-13
	unsigned int leftRightFormat		: 1;	// 11
	unsigned int pixelsPerLine			: 11;	//  0-10
} SpritePreambleWord1a;

typedef struct {
	unsigned int						: 6;	// 26-31
	unsigned int scanLineSizeInWords	: 10;	// 16-25 Thanks, hardware guys!
	unsigned int						: 1;	// 15
	unsigned int disableSwapHV			: 1;	// 14
	unsigned int blueLSBSource			: 2;	// 12-13
	unsigned int leftRightFormat		: 1;	// 11
	unsigned int pixelsPerLine			: 11;	//  0-10
} SpritePreambleWord1b;

typedef enum {
	kConstZero = 0,
	kInputPenNumberBit0 = 1,
	kInputPenNumberBit4 = 2,
	kInputPenNumberBit5 = 3
} BlueLSBSourceCode;

