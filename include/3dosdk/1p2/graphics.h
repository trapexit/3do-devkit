#pragma force_top_level
#pragma include_only_once

/* *************************************************************************
 *
 * Graphics Include File
 *
 * Copyright (C) 1992, New Technologies Group, Inc.
 * NTG Trade Secrets  -  Confidential and Proprietary
 *
 * The contents of this file were designed with any tab stops from 1 to 8
 *
 * DATE   NAME             DESCRIPTION
 * ------ ---------------- -------------------------------------------------
 * 940106 SHL		   Added Greg Omi's fast mapcel routines
 * 931025 SHL              Added DisplayInfo structure and calls
 * 930830 SHL              Split CreateBitmap out of CreateScreenGroup
 * 930729 SHL              Removed all reference to file base font stuff
 * 930726 SHL              Made graphics more paranoid about Items
 *                         not owned by the current task
 * 930708 SHL              Commented out stale elements of GrafFolio struct
 * 930630 SHL              Changed all SWI calls to use in-line SWIs
 * 930421 JCR              Changed Screen struct.
 * 930315 -RJ              Macro name changes
 * 930210 -RJ              Merged vdl.h into this file
 * 930102 -RJ              Changed DEFAULT_DISPCTRL to set HSUB and VSUB 
 *                         flags to default to zero
 * 921212 -RJ              Added font data structures
 * 921104 -RJ              Added scr_BitmapCount, started toying with idea
 *                         of BitmapInfo and ScreenInfo structures
 * 921031 -RJ              Changed rect_YBot to rect_YBottom
 * 921028 -RJ              Changed some CCB fields to follow name convention
 * 921016 -RJ              Ongoing massive overhauls for graphics 
 *                         restructuring  -  everything is different
 * 921015 -RJ              __SHERRIE must be defined for sherrie.h to be 
 *                         included, else hardware.h will be included. 
 * 920724 -RJ Mical        Start overhaul
 * 920717 Stephen Landrum  Last edits before July handoff
 *
 * ********************************************************************** */


#ifndef __GRAPHICS_H
#define __GRAPHICS_H


#include "types.h"
#include "nodes.h"
#include "folio.h"
#include "item.h"
#include "list.h"
#include "operror.h"
#include "timer.h"

#include "filesystem.h"
#include "filesystemdefs.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "hardware.h"




/* ===  =========  ====================================================== */
/* ===             ====================================================== */
/* ===  Constants  ====================================================== */
/* ===             ====================================================== */
/* ===  =========  ====================================================== */


/* Hard coded numbers for the graphics folio SWI functions */
#define GRAPHICSFOLIO	2
#define GRAFSWI		(GRAPHICSFOLIO<<16)



/* These represent the value one (1) in various number formats.  
 * For example, ONE_12_20 is the value of 1 in fixed decimal format 
 * of 12 bits integer, 20 bits fraction
 */
#define ONE_12_20  (1<<20)
#define ONE_16_16  (1<<16)


/* === Some typical PPMP modes === */
#define PPMP_MODE_NORMAL   0x01F40L
#define PPMP_MODE_AVERAGE  0x01F81L


/* When setting up your own VDL, this constant defines a reasonable 
 * starting value for your display control word
 * The display control word in the system's pre-display VDL uses this 
 * constant, so the values here are what your screen will inherit 
 * unless (until) you specify your own display control word.
 */
#define DEFAULT_DISPCTRL \
  ( VDL_DISPCTRL|VDL_HINTEN|VDL_VINTEN \
   |VDL_BLSB_BLUE|VDL_HSUB_ZERO|VDL_VSUB_ZERO \
   |VDL_WINBLSB_BLUE|VDL_WINHSUB_ZERO|VDL_WINVSUB_ZERO )

/* These are the types of VDL's that can exist in the system */
/* VDLTYPE_SYSTEM is reserved for system use */
#define VDLTYPE_SYSTEM	0
#define VDLTYPE_FULL    1
#define VDLTYPE_COLOR   2
#define VDLTYPE_ADDRESS 3
#define VDLTYPE_SIMPLE  4
#define VDLTYPE_DYNAMIC 5


/* These are the type arguments for the tag args that can be used to create 
 * a screen group.
 * Note that these tags do NOT conform to the normal tag rules.
 * This method of creating screen groups will be highly disrecommended
 * starting with the next release.
 */
#define CSG_TAG_DONE			0
#define CSG_TAG_DISPLAYHEIGHT		1
#define CSG_TAG_SCREENCOUNT		2
#define CSG_TAG_SCREENHEIGHT		3
#define CSG_TAG_BITMAPCOUNT		4
#define CSG_TAG_BITMAPWIDTH_ARRAY	5
#define CSG_TAG_BITMAPHEIGHT_ARRAY	6
#define CSG_TAG_BITMAPBUF_ARRAY		7
#define CSG_TAG_VDLTYPE			8
#define CSG_TAG_VDLPTR_ARRAY		9
#define CSG_TAG_VDLLENGTH_ARRAY		10
#define CSG_TAG_SPORTBITS		11
#define CSG_TAG_DISPLAYTYPE		12

/* These are the type arguments for the tag args that can be used to create 
 * a bitmap.
 */
#define CBM_TAG_DONE		0
#define CBM_TAG_WIDTH		11
#define CBM_TAG_HEIGHT		12
#define CBM_TAG_BUFFER		13
#define CBM_TAG_CLIPWIDTH	14
#define CBM_TAG_CLIPHEIGHT	15
#define CBM_TAG_CLIPX		16
#define CBM_TAG_CLIPY		17
#define CBM_TAG_WATCHDOGCTR	18
#define CBM_TAG_CECONTROL	19


/* These are the type arguments for the tag args that can be used to create or
 * to modify an existing VDL.
 */
#define CREATEVDL_TAG_DONE		0
#define CREATEVDL_TAG_SLIPSTREAM	11
#define CREATEVDL_TAG_HAVG		12
#define CREATEVDL_TAG_VAVG		13


/* NOTE: THESE OFFSETS MUST CORROSPOND TO SPORTCmdTable IN sportdev.c */
#define SPORTCMD_CLONE  4
#define SPORTCMD_COPY   5
#define FLASHWRITE_CMD  6


/* === Node and Item type numbers for graphics folio == */
#define NODE_GRAPHICS	2

/* These are the graphics folio's item and node types */
#define TYPE_SCREENGROUP  1
#define TYPE_SCREEN       2
#define TYPE_BITMAP       3
#define TYPE_VDL          4
#define TYPE_DISPLAYINFO  5

/* The default CE watch dog time out vertical blank counter */
#define WATCHDOG_DEFAULT	1000000

/* The default value in the Bitmap structure CEControl register */
#define CECONTROL_DEFAULT	(B15POS_PDC|B0POS_PPMP|CFBDSUB|CFBDLSB_CFBD0|PDCLSB_PDC0)




/* ===  ======  ========================================================= */
/* ===          ========================================================= */
/* ===  Macros  ========================================================= */
/* ===          ========================================================= */
/* ===  ======  ========================================================= */

/* This macro allows you to turn the absolute address of an object into 
 * the sort of relative address needed by the cel engine.  The first 
 * argument is the absolute address of the field to receive the relative 
 * address, and the second argument is the absolute address of the object 
 * to be referenced.  
 *   For instance, to create a relative pointer to a "next cel" you 
 * would use these arguments:
 *      MakeCCBRelative( &cel->ccb_NextPtr, &NextCel );
 * To make sure your cel indicates it has a relative pointer to the next 
 * cel, you might want to explicitly clear the control flag:
 *      ClearFlag( cel->ccb_Flags, CCB_NPABS );
 */
#define MakeCCBRelative(field,linkobject) ((int32)(linkobject)-(int32)(field)-4)

#define MakeRGB15(r,g,b) (((uint32)(r)<<10)|((uint32)(g)<<5)|(uint32)(b))
#define MakeRGB15Pair(r,g,b) (MakeRGB15(r,g,b)*0x00010001)

#define MakeCLUTColorEntry(index,r,g,b) ((((uint32)(index)<<24)|VDL_FULLRGB\
                         |((uint32)(r)<<16)|((uint32)(g)<<8)|((uint32)(b))))
#define MakeCLUTRedEntry(index,r) ((((uint32)(index)<<24)|VDL_REDONLY\
                         |((uint32)(r)<<16)))
#define MakeCLUTGreenEntry(index,g) ((((uint32)(index)<<24)|VDL_GREENONLY\
                         |((uint32)(g)<<8)))
#define MakeCLUTBlueEntry(index,b) ((((uint32)(index)<<24)|VDL_BLUEONLY\
                         |((uint32)(b)<<0)))
#define MakeCLUTBackgroundEntry(r,g,b) ((VDL_DISPCTRL|VDL_BACKGROUND\
                         |((uint32)(r)<<16)|((uint32)(g)<<8)|((uint32)(b))))


/* === RJ's Idiosyncracies === */
#define NOT !
#define FOREVER for(;;)
#define SetFlag(v,f) ((v)|=(f))
#define ClearFlag(v,f) ((v)&=~(f))
#define FlagIsSet(v,f) ((bool)(((v)&(f))!=0))
#define FlagIsClear(v,f) ((bool)(((v)&(f))==0))



/* ===  ===============  ================================================ */
/* ===                   ================================================ */
/* ===  Data Structures  ================================================ */
/* ===                   ================================================ */
/* ===  ===============  ================================================ */

typedef uint32 VDLEntry;

typedef uint32  Color;
typedef int32  Coord;
typedef uint32  RGB888;
typedef uint8 CharMap;


/* temporary definition of cel data structure */
typedef uint32 CelData[];



/* Here's the new font data structures */
typedef struct FontEntry
	{
	Node     ft;
	int32   ft_CharValue;
	int32   ft_Width;
	CelData *ft_Image;
	int32    ft_ImageByteCount;

	struct FontEntry *ft_LesserBranch;
	struct FontEntry *ft_GreaterBranch;
	} FontEntry;



typedef struct ScreenGroup {
  ItemNode sg;
  
  /* display location, 0 == top of screen */
  int32 sg_Y;
  
  /* total height of each screen */
  int32 sg_ScreenHeight;
  
  /* display height of each screen (can be less than the screen's 
   * actual height)
   */
  int32 sg_DisplayHeight; 
  
  /* list of tasks that have shared access to this ScreenGroup */
  List sg_SharedList;
  
  /* Flag verifying that user has called AddScreenGroup() */
  /* Just a temp solution for now (4-21-93) */
  int32 sg_Add_SG_Called;

  List sg_ScreenList;
} ScreenGroup;


typedef struct DisplayInfo {
  Node di;

  TagArg di_Tags[1];
} DisplayInfo;

/* DisplayInfo tags */

/* end of displayinfo tag list */
#define DI_END		0
/* uint32 - numeric type of display for passing into Screen and VDL creation calls */
#define DI_TYPE		1
/* uint32 - width of display in pixels */
#define DI_WIDTH	2
/* uint32 - height of display in pixels */
#define DI_HEIGHT	3
/* uint32 - number of usec/field */
#define DI_FIELDTIME	4
/* uint32 - frequency (Hz) of field */
#define DI_FIELDFREQ	5
/* ufrac16 - aspect ratio (width/height) of a pixel */
#define DI_ASPECT	6
/* uint32 - width component of aspect ratio */
#define DI_ASPECTW	7
/* uint32 - height component of aspect ratio */
#define DI_ASPECTH	8
/* uint32 - flag word - non-zero if display does not support interlace */
#define DI_NOINTERLACE	9
/* uint32 - flag word - non-zero if display does not support stereographics */
#define DI_NOSTEREO	10
/* char * - text string - name of display standard supported */
#define DI_NAME		11

/* number at which system internal private tags start */
#define DI_PRIVATE	0x00008000

/* Additional tag values above DI_PRIVATE may appear but are reserved for system use */

/* Currently available values for DI_TYPE */
/* This will never be found in the structure, but is a dummy value that can be */
/* passed into Screen and VDL creation routines to get the default display type */
#define DI_TYPE_DEFAULT	0
/* Standard 320x240 NTSC display */
#define DI_TYPE_NTSC	1
/* Narrow PAL display (320x288) */
#define DI_TYPE_PAL1	2
/* Normal PAL display (384x288) */
#define DI_TYPE_PAL2	3



typedef struct Bitmap {
  ItemNode bm;
  
  ubyte *bm_Buffer; 
  
  int32 bm_Width;
  int32 bm_Height;
  int32 bm_VerticalOffset;
  int32 bm_Flags;
  
  int32 bm_ClipWidth;
  int32 bm_ClipHeight;
  int32 bm_ClipX;
  int32 bm_ClipY;
  int32 bm_WatchDogCtr;  /* JCR */
  int32 bm_SysMalloc;  /* If set, CreateScreenGroup MALLOCED for bm. JCR */
  
  /* List of tasks that have share access to this Bitmap */
  List bm_SharedList;
  
  int32 bm_CEControl;
  int32 bm_REGCTL0;
  int32 bm_REGCTL1;
  int32 bm_REGCTL2;
  int32 bm_REGCTL3;
} Bitmap;


/* The VDL is a system private structure.  Future releases of the graphics
 * folio will NOT include the description of the VDL structure.  The VDL
 * structure WILL change in the future.
 */
/* VDL */
typedef struct VDL {
  ItemNode vdl;  /* link VDL's in screen lists */
  struct Screen* vdl_ScreenPtr;
  VDLEntry* vdl_DataPtr; /* addr of concatenation of VDLEntries*/
  int32    vdl_Type;
  int32    vdl_DataSize;  /* length of concat */
  
  int32	vdl_Flags;	/* element added 24-Sep-93 - SHL */
  int32 vdl_DisplayType;	/* The type of display that this VDL was created for */
  int32 vdl_Height;	/* number of lines this VDL encompasses */
  int32 vdl_Offset;	/* number of blank lines to pad preceding this VDL */
} VDL;
			  


/* JCR */
typedef struct Screen
	{
	ItemNode scr;

	ScreenGroup *scr_ScreenGroupPtr;

	VDL       *scr_VDLPtr;
	Item      scr_VDLItem; /* Item # for above VDL */
	int32     scr_VDLType;

	int32 scr_BitmapCount;
	List  scr_BitmapList;

	List scr_SharedList;
Bitmap *scr_TempBitmap;
	} Screen;


/* ??? The BitmapInfo and ScreenInfo stuff is under construction.  
 * ??? I'm thinking about it ... I'm workin' on it, I'm workin' on it! 
 */
typedef struct BitmapInfo
	{
	Item    bi_Item;
	Bitmap *bi_Bitmap;
	ubyte  *bi_Buffer;
	} BitmapInfo;

/* The ScreenInfo structure contains critical information about a 
 * screen and all its associated data structures.  
 * 
 * The ScreenInfo ends with an instance of the BitmapInfo structure.  
 * In actuality, there can be any number of BitmapInfo structures at the 
 * end of the ScreenInfo structure.  In the simple case, which almost 
 * everyone will use, a screen will be comprised of a single bitmap.  
 * To simplify references to the ScreenInfo fields, the ScreenInfo 
 * structure is defined as having a single instance of a BitmapInfo 
 * structure.  Furthermore, to simplify allocation of and referencing to 
 * ScreenInfo structures with more than a single bitmap, the ScreenInfo2 
 * and ScreenInfo3 structures are defined to describe screens that have 
 * two and three bitmaps.  These are defined for your convenience.  
 * 
 * The InitScreenInfo() call presumes that your ScreenInfo argument 
 * points to a ScreenInfo structure with the correct number of BitmapInfo 
 * fields at the end of it.  
 * 
 * Hmm:
 *   ScreenInfo ScreenInfos[2];
 *   ScreenInfo *ScreenInfoPtrs[2] = {&ScreenInfos[0], &ScreenInfos[1]};
 *   CreateScreenGroup( ScreenInfoPtrs, TagArgs );
 *   DrawCels( ScreenInfos[ScreenSelect].si_BitmapInfo.bi_Item, &Cel );
 *   DisplayScreen( ScreenInfos[ScreenSelect].si_Item, 0 );
 *   ScreenSelect = 1 - ScreenSelect;
 */
typedef struct ScreenInfo
	{
	Item       si_Item;
	Screen    *si_Screen;
	BitmapInfo si_BitmapInfo;
	} ScreenInfo;

typedef struct Point
	{
	Coord pt_X;
	Coord pt_Y;
	} Point;


typedef struct Rect
	{
	Coord rect_XLeft;
	Coord rect_YTop;
	Coord rect_XRight;
	Coord rect_YBottom;
	} Rect;


/* Graphics Context structure */
typedef struct GrafCon
	{
	Node  gc;
	Color gc_FGPen;
	Color gc_BGPen;
	Coord gc_PenX;
	Coord gc_PenY;
	uint32 gc_Flags;
	} GrafCon;


/* temporary definition of cel control block */
typedef struct CCB
	{
	uint32 ccb_Flags;

	struct CCB *ccb_NextPtr;
	CelData    *ccb_SourcePtr;
	void       *ccb_PLUTPtr;

	Coord ccb_XPos;
	Coord ccb_YPos;
	int32  ccb_HDX;
	int32  ccb_HDY;
	int32  ccb_VDX;
	int32  ccb_VDY;
	int32  ccb_HDDX;
	int32  ccb_HDDY;
	uint32 ccb_PIXC;
	uint32 ccb_PRE0;
	uint32 ccb_PRE1;

	/* These are special fields, tacked on to support some of the 
	 * rendering functions.  
	 */
	int32  ccb_Width;
	int32  ccb_Height;
	} CCB;


/* These are temporary definitions of the data structures the text 
 * rendering routines will require.  All of this is probably going to 
 * change dramatically when the real stuff comes online
 */

/* The FontChar structure defines the image for a single character 
 * The text value of the character is defined with an int32 to allow 
 * either 8-bit or 16-bit text character definitions.  
 */
typedef struct FontChar
	{
	uint32   fc_CharValue;
	uint8    fc_Width;
	CelData *fc_Image;
	} FontChar;

/* The Font definition provides a font to be used with the text rendering 
 * routines.  It defines a mapping from text characters to their images 
 * by pointing to an array of FontChar definitions.  It also allows 
 * the programmer to control the appearance of the rendered text imagery 
 * by providing for a CCB to be used when printing the characters, 
 * allowing the programmer to control both the CCB's Flags field and the 
 * PPMP value.  
 * 
 * The PPMP value will come from the GrafCon supplied to the DrawChar() 
 * call, as soon as I define a PPMP field in the GrafCon.
 */
typedef struct Font
	{
	uint8      font_Height;
	uint8      font_Flags;
	CCB       *font_CCB;
	
	/* The font_FontEntries field is significant only with RAM-resident fonts */
	FontEntry *font_FontEntries;
	} Font;
/* === font_Flags definitions === */
#define FONT_ASCII            0x01 /* This is an ASCII font */
#define FONT_ASCII_UPPERCASE  0x02 /* Lowercase will be translated to upper */
#define FONT_FILEBASED        0x04 /* Font is file-based (not RAM-resident */
#define FONT_VERTICAL         0x08 /* Font rendered vertically */



/* Applications should not retrieve numbers directly from GrafBase, but should
 * instead call utility routines to obtain the numbers.  In future releases,
 * the GrafFolio structure will not be published (except for a few important
 * offsets that old programs refer to).
 */
typedef struct GrafFolio
	{
	Folio gf;

	uint32	gf_Flags;

#ifdef __cplusplus
	uint32 gf_VBLNumber;
#else
	volatile uint32 gf_VBLNumber;
#endif /* __cplusplus */

	void*	gf_ZeroPage;
	void*	gf_VIRSPage;

	uint32	gf_VRAMPageSize;
	int32	gf_DefaultDisplayWidth;
	int32	gf_DefaultDisplayHeight;

	Timer*	gf_TimeoutTimer;

	int32	gf_Reserved5;
	int32	gf_Reserved6;
	int32	gf_Reserved7;

	VDLEntry* gf_VDLForcedFirst;
	VDLEntry* gf_VDLPreDisplay;
	VDLEntry* gf_VDLPostDisplay;
	VDLEntry* gf_VDLBlank;
	VDL*	gf_CurrentVDLEven;
	VDL*	gf_CurrentVDLOdd;
	VDLEntry* gf_VDLDisplayLink;

	int32	gf_Reserved1;
	int32	gf_Reserved3;

	Item	gf_CelSemaphore;	/* who has the Cel Engine? */

	int32	gf_VBLTime;		/* number of usec between VBLs */
	int32	gf_VBLFreq;		/* approximate VBL frequency in Hz */

	int32   gf_Reserved2;

	Stream*	gf_CurrentFontStream;	/* Much of this block is unused */
	int32   gf_FileFontCacheSize;
	int32   gf_FileFontCacheAlloc;
	ubyte*	gf_FileFontCache;
	FontEntry* gf_FontEntryHead;
	FontEntry* gf_FontEntryButt;
	List    gf_FontLRUList;
	int32   gf_FileFontFlags;
	int32   gf_FontBaseChar;
	int32   gf_FontMaxChar;
	Font*	gf_CurrentFont;
	int32   gf_CharArrayOffset;
	int32   gf_fileFontCacheUsed;

	List	gf_DisplayInfoList;

	uint32	gf_DefaultDisplayType;
	uint32	gf_DisplayTypeMask;
	VDL*	gf_BlankVDL;

	} GrafFolio;


/* === gf_Flags bits === */
/* none defined just now */



/* ===  =================  ============================================= */
/* ===                     ============================================= */
/* ===  Error Definitions  ============================================= */
/* ===                     ============================================= */
/* ===  =================  ============================================= */

#define GRAFERR_BADTAG		MAKEGERR(ER_SEVERE,ER_C_STND,ER_BadTagArg)
#define GRAFERR_BADTAGVAL	MAKEGERR(ER_SEVERE,ER_C_STND,ER_BadTagArgVal)
#define GRAFERR_BADPRIV		MAKEGERR(ER_SEVERE,ER_C_STND,ER_NotPrivileged)
#define GRAFERR_BADSUBTYPE	MAKEGERR(ER_SEVERE,ER_C_STND,ER_BadSubType)
#define GRAFERR_BADITEM		MAKEGERR(ER_SEVERE,ER_C_STND,ER_BadItem)
#define GRAFERR_NOMEM		MAKEGERR(ER_SEVERE,ER_C_STND,ER_NoMem)
#define GRAFERR_BADPTR		MAKEGERR(ER_SEVERE,ER_C_STND,ER_BadPtr)
#define GRAFERR_NOTOWNER	MAKEGERR(ER_SEVERE,ER_C_STND,ER_NotOwner)

#define GRAFERR_BASE (20)
#define GRAFERR_CELTIMEOUT	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+0)
#define GRAFERR_BADCLIP		MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+1)
#define GRAFERR_BADVDLTYPE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+2)
#define GRAFERR_INDEXRANGE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+3)
#define GRAFERR_BUFWIDTH	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+4)
#define GRAFERR_COORDRANGE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+5)
#define GRAFERR_VDLWIDTH	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+6)
#define GRAFERR_NOTYET		MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+7)
#define GRAFERR_MIXEDSCREENS	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+8)
#define GRAFERR_BADFONTFILE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+9)
#define GRAFERR_BADDEADBOLT	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+10)
#define GRAFERR_VDLINUSE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+11)
#define GRAFERR_PROOF_ERR	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+12)
#define GRAFERR_VDL_LENGTH	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+13)
#define GRAFERR_NO_FONT		MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+14)
#define GRAFERR_BADDISPDIMS	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+15)
#define GRAFERR_BADBITMAPSPEC	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+16)
#define GRAFERR_INTERNALERROR	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+17)
#define GRAFERR_SGINUSE		MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+18)
#define GRAFERR_SGNOTINUSE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+19)
#define GRAFERR_GRAFNOTOPEN	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+20)
#define GRAFERR_NOWRITEACCESS	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+21)
#define GRAFERR_BADDISPLAYTYPE	MAKEGERR(ER_SEVERE,ER_C_NSTND,GRAFERR_BASE+22)



/* ===  ===============================  ================================ */
/* ===                                   ================================ */
/* ===  Externs and Function Prototypes  ================================ */
/* ===                                   ================================ */
/* ===  ===============================  ================================ */

extern GrafFolio *GrafBase;
extern Item GrafFolioNum;

/* routine numbers for user mode folio calls */
#define _MODIFYVDL_	-47
#define _GETFIRSTDISPLAYINFO_	-46
#define _SETCEWATCHDOG_	-45
#define _DRAWSCREENCELS_	-44
#define _DRAWCELS_	-43
#define _SUBMITVDL_	-42
#define _SETVDL_	-41
#define _DISPLAYSCREEN_	-40
/* */
#define _SETCECONTROL_	-38
#define _DRAWTEXT8_	-37
#define _GETCURRENTFONT_	-36
#define _SETCURRENTFONTCCB_	-35
#define _FILLRECT_	-34
#define _DRAWTO_	-33
#define _DRAWCHAR_	-32

#define _MOVETO_	-30
#define _SETCLIPHEIGHT_	-29
#define _SETCLIPWIDTH_	-28
#define _REMOVESCREENGROUP_	-27
#define _ADDSCREENGROUP_	-26
#define _SETBGPEN_	-25
#define _SETFGPEN_	-24
#define _DELETESCREENGROUP_	-23
#define _SETSCREENCOLORS_	-22
#define _RESETSCREENCOLORS_	-21
#define _SETSCREENCOLOR_	-20
#define _DISABLEHAVG_	-19
#define _ENABLEHAVG_	-18
#define _DISABLEVAVG_	-17
#define _ENABLEVAVG_	-16
#define _SETCLIPORIGIN_	-15
#define _RESETREADADDRESS_	-14
#define _SETREADADDRESS_	-13

#define _CREATESCREENGROUP_	-12
#define _RESETFONT_	-11
/* #define _CLOSEFONT_	-10 */
#define _DRAWTEXT16_	-9
/* #define _OPENFILEFONT_	-8 */
/* #define _OPENRAMFONT_	-7 */
/* #define _SETFILEFONTCACHESIZE_	-6 */
#define _WRITEPIXEL_	-5
#define _GETPIXELADDRESS_	-4
#define _READVDLCOLOR_	-3
#define _READPIXEL_	-2
#define _MAPSPRITE_	-1


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef _GRAPHICS_INTERNAL
#define __swi(x)
#endif


int32 AddScreenGroup( Item screenGroup, TagArg *targs );
/* int32 CloseFont( void ); */
#define CreateBitmap(x) CreateItem(MKNODEID(NODE_GRAPHICS,TYPE_BITMAP),x)
Item CreateScreenGroup( Item *screenItemArray, TagArg *targs );
Err DeleteScreenGroup (Item screenGroupItem);
#define DeleteVDL(x) DeleteItem(x)
Err DisableHAVG( Item screenItem );
Err DisableVAVG( Item screenItem );
Err DisplayScreen( Item screenItem0, Item screenItem1 );
Err DrawCels( Item bitmapItem, CCB *ccb );
Err DrawChar( GrafCon *gcon, Item bitmapItem, uint32 character );
Err DrawScreenCels( Item screenItem, CCB *ccb );
Err DrawText16( GrafCon *gcon, Item bitmapItem, uint16 *text );
Err DrawText8( GrafCon *gcon, Item bitmapItem, uint8 *text );
Err DrawTo( Item bitmapItem, GrafCon *grafcon, Coord x, Coord y );
Err EnableHAVG( Item screenItem );
Err EnableVAVG( Item screenItem );
Err FillRect( Item bitmapItem, GrafCon *gc, Rect *r );
Font *GetCurrentFont( void );
void *GetPixelAddress( Item screenItem, Coord x, Coord y );
void MapCel( CCB *ccb, Point *quad );
void MoveTo( GrafCon *gc, Coord x, Coord y );
/* int32 OpenFileFont( char *filename );  */
Err OpenGraphicsFolio( void );
/* int32 OpenRAMFont( Font *font ); */
RGB888 ReadCLUTColor( uint32 index );
Color ReadPixel( Item bitmapItem, GrafCon *gc, Coord x, Coord y );
Err RemoveScreenGroup( Item screenGroup );
Err ResetCurrentFont( void );
Err ResetReadAddress( Item bitmapItem );
Err ResetScreenColors( Item screenItem );
void  SetBGPen( GrafCon *gc, Color c );
Err SetCEControl( Item bitmapItem, int32 controlWord, int32 controlMask );
Err SetCEWatchDog( Item bitmapItem, int32 db_ctr );
Err SetClipHeight( Item bitmapItem, int32 clipHeight );
Err SetClipOrigin( Item bitmapItem, int32 x, int32 y );
Err SetClipWidth( Item bitmapItem, int32 clipWidth );
Err SetCurrentFontCCB( CCB *ccb );
void SetFGPen( GrafCon *gc, Color c );
/* int32 SetFileFontCacheSize( int32 size ); */
Err SetReadAddress( Item bitmapItem, ubyte *buffer, int32 width );
Err SetScreenColor( Item screenItem, uint32 colorEntry );
Err SetScreenColors( Item screenItem, uint32 *entries, int32 count );
Err SetVDL( Item screenItem, Item vdlItem );
Item SubmitVDL( VDLEntry *VDLDataPtr, int32 length, int32 type );
Err WritePixel( Item bitmapItem, GrafCon *gc, Coord x, Coord y );

Err ModifyVDL (Item vdlItem, TagArg* vdlTags);

DisplayInfo* GetFirstDisplayInfo (void);

Item GetVRAMIOReq (void);
Err SetVRAMPages (Item ioreq, void *dest, int32 val, int32 numpages, int32 mask);
Err CopyVRAMPages (Item ioreq, void *dest, void *src, uint32 numpages, uint32 mask);
Err CloneVRAMPages (Item ioreq, void *dest, void *src, uint32 numpages, uint32 mask);
Err SetVRAMPagesDefer (Item ioreq, void *dest, int32 val, int32 numpages, int32 mask);
Err CopyVRAMPagesDefer (Item ioreq, void *dest, void *src, uint32 numpages, uint32 mask);
Err CloneVRAMPagesDefer (Item ioreq, void *dest, void *src, uint32 numpages, uint32 mask);

Item GetVBLIOReq (void);
Err WaitVBL (Item ioreq, uint32 numfields);
Err WaitVBLDefer (Item ioreq, uint32 numfields);


/*
 * fast mapcel routines by Greg Omi
 */


/*
*	Name:
*		FastMapCelInit
*	Purpose:
*		If width and height are powers of 2 then
*			ccb_Width = log2(ccb_Width)
*			ccb_Height = log2(ccb_Height)
*		else
*			ccb_Width = -(0x10000/ccb_Width)
*			ccb_Height = 0x10000/ccb_Height
*	Entry:
*		Cel index
*/
void FastMapCelInit (CCB *ccb);

/*
*	Name:
*		FastMapCel
*	Purpose:
*		Sets up delta fields to creat cel mapped to
*		four points in quad argument.
*	Entry:
*		CCB Pointer
*		Quad pointer
*	Exit:
*		None
*	Max time:
*		Approx. 103 + 16(ENTER) + 14(EXIT) = 133 cycles or 10.64Usec
*/
void FastMapCel (CCB *ccb, Point *quad);

/*
*	Name:
*		FastMapCelf16
*	Purpose:
*		Sets up delta fields to creat cel mapped to
*		four frac16 points in quad argument.
*	Entry:
*		CCB Pointer
*		Quad pointer
*	Exit:
*		None
*	Max time:
*		Approx. 111 + 16(ENTER) + 14(EXIT) = 141 cycles or 11.28Usec
*/
void FastMapCelf16 (CCB *ccb, Point *quad);







#ifdef _GRAPHICS_INTERNAL
#undef __swi
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif  /* of #define __GRAPHICS_H */
