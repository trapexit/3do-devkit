/* EZQ_Defs.h */

/* Contains global definitions used for EZQ compression

	Copyright 1994 The 3DO Company, All Rights Reserved
	
	*/
	
	
// Version Info

#define CodecID_2_Bits (('E' << 8) + 'Z' + 2)	/* EZ2 is 2 bits/pixel */
#define CodecID_4_Bits (('E' << 8) + 'Z' + 1)	/* EZ1 is 4 bits/pixel */
#define Version_2_Bits (8114) /* 8/11/94 */
#define Version_4_Bits (8114) /* 8/11/94 */

// Compilation Controls

	// If defined, compress RGB.  Otherwise, compress Yuv
		#define UseRGBSpace
		#define NoOverflow	
		//#define TranslateFirstFrameOnly
		
typedef union RGBTriple
	{
	unsigned long whole;
	struct
		{
		unsigned char accum;
		unsigned char red;
		unsigned char green;
		unsigned char blue;
		} part;
	} RGBTriple;
		
typedef struct PPMHeader
		{
		unsigned short PPMWidth;	/* width in file */
		unsigned short PPMHeight;	/* height in file */
		
		unsigned short width;
		unsigned short height;
		unsigned short depth;
		RGBTriple *pixels;
		} PPMHeader;
		
// Misc Defines

	#define FirstRunPrint	0
	#define LastRunPrint	4
	#define PrintRuns ((0) && (runCount >= FirstRunPrint) && (runCount <= LastRunPrint))
	
	#define LiteralEscape 0xFF
	#define RunEscape 0xFE
	#define RunLitThreshold	254 /* This long & longer will get a literal pixel */
	
	#define ColorBias	(15)
	#define ColorValue(v) (((v) == 0)?0:((((unsigned char)(v))>(255-ColorBias))?255:(((v)+ColorBias)) & 0xFF))
	
	#define MaxDisplayHeight	216
	#define MaxDisplayWidth		288
	
	#define WidthModulus		8
	
	#ifdef UseRGBSpace
		
		#define PixelMin		0	
		#define PixelRange		32
		#define PixelMidValue	(PixelRange/2)
		#define PixelMax		(PixelRange-1)
		#define PixelMask		0xF800	/* Keep only desired bits of 16 bit RGB value */
		
		#define MinRedValue	PixelMin
		#define MaxRedValue	PixelMax
		
		#define MinGreenValue	PixelMin
		#define MaxGreenValue	PixelMax
		
		#define MinBlueValue	PixelMin
		#define MaxBlueValue	PixelMax
		
		#define MinYValue	PixelMin
		#define MaxYValue	PixelMax
		
		#define MinuValue	PixelMin
		#define MaxuValue	PixelMax
		
		#define MinvValue	PixelMin
		#define MaxvValue	PixelMax
		
		//#define Max_Red_Q_Count	6
		//#define Max_Green_Q_Count	7
		//#define Max_Blue_Q_Count	6
		
		#define Max_Q_Count	8
		
		#define _3D0toColor(v) (CLUT[clamp((v), PixelMin, PixelMax)] + dither())
		#define _3D0toPixel(v) (((CLUT[clamp((v), PixelMin, PixelMax)] + dither()) << 8) & PixelMask)
		
		#define field1	color.red
		#define field2	color.green
		#define field3	color.blue
		#define DCPMPixel(v) RevCLUT[clamp((v) >> 8, 0, 0xFF)]
	#else
	
		#define PixelMin		0	
		#define PixelRange		256
		#define PixelMidValue	(PixelRange/2)
		#define PixelMax		(PixelRange-1)
		#define PixelMask		0xFF00	/* Keep only desired bits of 16 bit RGB value */
		
		#define MinRedValue	PixelMin
		#define MaxRedValue	PixelMax
		
		#define MinGreenValue	PixelMin
		#define MaxGreenValue	PixelMax
		
		#define MinBlueValue	PixelMin
		#define MaxBlueValue	PixelMax
		
		#define MinYValue	PixelMin
		#define MaxYValue	PixelMax
		
		#define MinuValue	PixelMin
		#define MaxuValue	PixelMax
		
		#define MinvValue	PixelMin
		#define MaxvValue	PixelMax
		
		//#define Max_Red_Q_Count	8
		//#define Max_Green_Q_Count	8
		//#define Max_Blue_Q_Count	8
		
		#define _3D0toColor(v) (clamp((v), PixelMin, PixelMax))
		#define _3D0toPixel(v) (((clamp((v), PixelMin, PixelMax)) << 8) & PixelMask)
		
		#define field1	Yuv.Y
		#define field2	Yuv.u
		#define field3	Yuv.v
		#define DCPMPixel(v) clamp(FloatToUChar(v), 0, 0xFF)
		
	#endif
	
// Define Predictor

	#define _3DOPredictor
	//#define LinearPredictor
	//#define AveragePredictor
	#ifdef AveragePredictor
		#define Predictor(x,y,field) (pixelAverage = (x == 1)?(sourceDisplay[(y)][0].field):(pixelAverage + sourceDisplay[(y)][(x)-1].field)/2)
		extern unsigned short pixelAverage;
	#else
		#ifdef LinearPredictor
			#define Predictor(x,y,field) sourceDisplay[(y)][(x)-1].field
		#else
			#ifdef _3DOPredictor
				//#define Predictor(x,y,field) clamp((sourceDisplay[(y)][(x)-1].field+sourceDisplay[(y)-1][(x)].field)/2, PixelMin, PixelMax)
				#define Predictor(x,y,field) ((sourceDisplay[(y)][(x)-1].field+sourceDisplay[(y)-1][(x)].field)/2)
			#else
				#define Predictor(x,y,field) clamp(sourceDisplay[(y)][(x)-1].field+sourceDisplay[(y)-1][(x)].field\
					-sourceDisplay[(y)-1][(x)-1].field, PixelMin, PixelMax)
			#endif
		#endif
	#endif
	
	//#define MaxvalidPatterns (Max_Red_Q_Count*Max_Green_Q_Count*Max_Blue_Q_Count)
	#define PatMapSize 256	/* Should be first power of 2 greater than validPatterns */
	
// Extern Definitions

	extern char CLUT[32];
	extern char red_Error_Table[Max_Q_Count];
	extern char green_Error_Table[Max_Q_Count];
	extern char blue_Error_Table[Max_Q_Count];
	
	extern unsigned short red_Q_Count;
	extern unsigned short green_Q_Count;
	extern unsigned short blue_Q_Count;
	extern unsigned short validPatterns;
	
	extern char unsigned red_Q_Table[PixelRange]; /* Index by value + PixelMidValue */
	extern char unsigned green_Q_Table[PixelRange]; /* Index by value + PixelMidValue */
	extern char unsigned blue_Q_Table[PixelRange]; /* Index by value + PixelMidValue */

	extern unsigned char accumPat[PatMapSize];	// index by accumerror to get packed error index
	extern unsigned char accumMap[PatMapSize];	// index by accumerror to get packed error index
	extern int accumFrequency[PatMapSize];			// use count -- sorted in reverse order by buildErrorCommon

	extern unsigned short clamp(int v, int min, int max);

