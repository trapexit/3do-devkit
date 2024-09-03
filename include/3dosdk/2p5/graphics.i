	IF :LNOT::DEF:	|__GRAPHICS_H|
	GBLL	|__GRAPHICS_H|

;*****************************************************************************
;*
;*  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
;*  This material contains confidential information that is the property of The 3DO Company.
;*  Any unauthorized duplication, disclosure or use is prohibited.
;*  $Id: graphics.i,v 1.10 1994/09/10 02:04:22 vertex Exp $
;*
;*  Graphics folio interface definitions
;*
;*****************************************************************************

	INCLUDE	nodes.i
	INCLUDE	folio.i
	INCLUDE	item.i
	INCLUDE	list.i
	INCLUDE	hardware.i


;  ===  =========  ======================================================
;  ===             ======================================================
;  ===  Constants  ======================================================
;  ===             ======================================================
;  ===  =========  ======================================================

GRAPHICSFOLIO	EQU 2
GRAfSWI		EQU (GRAPHICSFOLIO:SHL:16)

;  These represent the value one (1) in various number formats.
;  For example, ONE_12_20 is the value of 1 in fixed decimal format
;  of 12 bits integer, 20 bits fraction
;
ONE_12_20  EQU (1:SHL:20)
ONE_16_16  EQU (1:SHL:16)


;  === Some typical PPMP modes ===
PPMP_MODE_NORMAL   EQU &01F40
PPMP_MODE_AVERAGE  EQU &01F81


;  These are the types of VDL's that can exist in the system
VDLTYPE_FULL     EQU 1
VDLTYPE_COLOR    EQU 2
VDLTYPE_ADDRESS  EQU 3
VDLTYPE_SIMPLE   EQU 4
VDLTYPE_DYNAMIC  EQU 5


;  These are the type arguments for the tag args that can be used to create
;  a screen group.
;
CSG_TAG_DONE                 EQU 0
CSG_TAG_DISPLAYHEIGHT        EQU 1
CSG_TAG_SCREENCOUNT          EQU 2
CSG_TAG_SCREENHEIGHT         EQU 3
CSG_TAG_BITMAPCOUNT          EQU 4
CSG_TAG_BITMAPWIDTH_ARRAY    EQU 5
CSG_TAG_BITMAPHEIGHT_ARRAY   EQU 6
CSG_TAG_BITMAPBUF_ARRAY      EQU 7
CSG_TAG_VDLTYPE              EQU 8
CSG_TAG_VDLPTR_ARRAY         EQU 9
CSG_TAG_VDLLENGTH_ARRAY     EQU 10  ; JCR
CSG_TAG_SPORTBITS           EQU 11


;  NOTE: THESE OFFSETS MUST CORROSPOND TO SPORTCmdTable IN sportdev.c
SPORTCMD_CLONE  EQU 4
SPORTCMD_COPY   EQU 5
FLASHWRITE_CMD  EQU 6  ; JCR


;  === Node and Item type numbers for graphics folio ==
NODE_GRAPHICS	EQU 2

;  These are the graphics folio's item types
TYPE_SCREENGROUP	EQU 1
TYPE_SCREEN	EQU 2
TYPE_BITMAP	EQU 3
TYPE_VDL	EQU 4

;  JCR. The default CE watch dog time out vertical blank counter
WATCHDOG_DEFAULT	EQU 1000000





;  ===  ===============  ================================================
;  ===                   ================================================
;  ===  Data Structures  ================================================
;  ===                   ================================================
;  ===  ===============  ================================================

	TYPEDEF	INT32,VDLEntry

	TYPEDEF	LONG,Color
	TYPEDEF	LONG,Coord
	TYPEDEF	LONG,RGB888
	TYPEDEF	UBYTE,CharMap


;  temporary definition of cel data structure
	TYPEDEF	ULONG,CelData



;  Here's the new font data structures
	BEGINSTRUCT	 FontEntry
		STRUCT	Node,ft
		INT32	ft_CharValue
		INT32	ft_Width
		PTR	ft_Image
		INT32	ft_ImageByteCount

		PTR	ft_LesserBranch
		PTR	ft_GreaterBranch
	ENDSTRUCT


	BEGINSTRUCT	 ScreenGroup
		STRUCT	ItemNode,sg

		;  display location, 0 == top of screen
		LONG	sg_Y

		;  total height of each screen
		LONG	sg_ScreenHeight

		;  display height of each screen (can be less than the screen's
		;  actual height)
		;
		LONG	sg_DisplayHeight

		; /* list of tasks that have shared access to this ScreenGroup */
		STRUCT	List,sg_SharedList

		;  Flag verifying that user has called AddScreenGroup()
		;  Just a temp solution for now (4-21-93)
		INT32	sg_Add_SG_Called
	ENDSTRUCT


	BEGINSTRUCT	 Bitmap
		STRUCT	ItemNode,bm

		PTR	bm_Buffer

		INT32	bm_Width
		INT32	bm_Height
		INT32	bm_VerticalOffset
		INT32	bm_Flags

		INT32	bm_ClipWidth
		INT32	bm_ClipHeight
		INT32	bm_ClipX
		INT32	bm_ClipY
		INT32	bm_WatchDogCtr   ; JCR
		INT32	bm_SysMalloc   ; If set, CreateScreenGroup MALLOCED for bm. JCR

		;/* List of tasks that have share access to this Bitmap */
		STRUCT	List,bm_SharedList

		INT32	bm_CEControl
		INT32	bm_REGCTL0
		INT32	bm_REGCTL1
		INT32	bm_REGCTL2
		INT32	bm_REGCTL3
	ENDSTRUCT
;  VDLVDL
	BEGINSTRUCT	VDL
		STRUCT	ItemNode,vdl	; link VDL's in screen lists
		PTR	vdl_ScreenPtr
		PTR	vdl_DataPtr	; addr of concatenation of VDLEntries
		INT32	vdl_Type
		INT32	vdl_DataSize	; length of concat

		INT32	vdl_Flags	; flags field
		INT32	vdl_DisplayType	; type of display that this VDL was created for
		INT32	vdl_Height	; number of lines this VDL encompasses
		INT32	vdl_Offset	; number of blank lines to pad preceding this VDL
	ENDSTRUCT
;  JCR
	BEGINSTRUCT	Screen
		STRUCT	ItemNode,scr

		PTR	scr_ScreenGroupPtr

		PTR	scr_VDLPtr
		STRUCT	ITEM,scr_VDLItem  ; Item # for above VDL
		INT32	scr_VDLType

		INT32	scr_BitmapCount
		STRUCT	List,scr_BitmapList

		STRUCT	List,scr_SharedList
		PTR	scr_TempBitmap
	ENDSTRUCT

;  ??? The BitmapInfo and ScreenInfo stuff is under construction.
;  ??? I'm thinking about it ... I'm workin' on it, I'm workin' on it!
;
	BEGINSTRUCT	 BitmapInfo
		STRUCT	ITEM,bi_Item
		PTR	bi_Bitmap
		PTR	bi_Buffer
	ENDSTRUCT
;  The ScreenInfo structure contains critical information about a
;  screen and all its associated data structures.
;
;  The ScreenInfo ends with an instance of the BitmapInfo structure.
;  In actuality, there can be any number of BitmapInfo structures at the
;  end of the ScreenInfo structure.  In the simple case, which almost
;  everyone will use, a screen will be comprised of a single bitmap.
;  To simplify references to the ScreenInfo fields, the ScreenInfo
;  structure is defined as having a single instance of a BitmapInfo
;  structure.  Furthermore, to simplify allocation of and referencing to
;  ScreenInfo structures with more than a single bitmap, the ScreenInfo2
;  and ScreenInfo3 structures are defined to describe screens that have
;  two and three bitmaps.  These are defined for your convenience.
;
;  The InitScreenInfo() call presumes that your ScreenInfo argument
;  points to a ScreenInfo structure with the correct number of BitmapInfo
;  fields at the end of it.
;
;  Hmm:
;    ScreenInfo ScreenInfos[2];
;    ScreenInfo *ScreenInfoPtrs[2] = {&ScreenInfos[0], &ScreenInfos[1]};
;    CreateScreenGroup( ScreenInfoPtrs, TagArgs );
;    DrawCels( ScreenInfos[ScreenSelect].si_BitmapInfo.bi_Item, &Cel );
;    DisplayScreen( ScreenInfos[ScreenSelect].si_Item, 0 );
;    ScreenSelect = 1 - ScreenSelect;
;
	BEGINSTRUCT	 ScreenInfo
		STRUCT	ITEM,si_Item
		PTR	si_Screen
		STRUCT	BitmapInfo,si_BitmapInfo
	ENDSTRUCT
	BEGINSTRUCT	 ScreenInfo2
		STRUCT	ITEM,si2_Item
		PTR	si2_Screen
		ARRAY	BitmapInfo,si2_BitmapInfo,2
	ENDSTRUCT
	BEGINSTRUCT	 ScreenInfo3
		STRUCT	ITEM,si3_Item
		PTR	si3_Screen
		ARRAY	BitmapInfo,si3_BitmapInfo,3
	ENDSTRUCT

	BEGINSTRUCT	 Point
		STRUCT	Coord,pt_X
		STRUCT	Coord,pt_Y
	ENDSTRUCT

	BEGINSTRUCT	 Rect
		STRUCT	Coord,rect_XLeft
		STRUCT	Coord,rect_YTop
		STRUCT	Coord,rect_XRight
		STRUCT	Coord,rect_YBottom
	ENDSTRUCT

;  Graphics Context structure
	BEGINSTRUCT	 GrafCon
		STRUCT	Node,gc
		STRUCT	Color,gc_FGPen
		STRUCT	Color,gc_BGPen
		STRUCT	Coord,gc_PenX
		STRUCT	Coord,gc_PenY
		ULONG	gc_Flags
	ENDSTRUCT

;  temporary definition of cel control block
	BEGINSTRUCT	 CCB
		ULONG	ccb_Flags

		PTR	ccb_NextPtr
		PTR	ccb_SourcePtr
		PTR	ccb_PLUTPtr

		STRUCT	Coord,ccb_XPos
		STRUCT	Coord,ccb_YPos
		LONG	ccb_HDX
		LONG	ccb_HDY
		LONG	ccb_VDX
		LONG	ccb_VDY
		LONG	ccb_HDDX
		LONG	ccb_HDDY
		ULONG	ccb_PIXC
		ULONG	ccb_PRE0
		ULONG	ccb_PRE1

		;  These are special fields, tacked on to support some of the
		;  rendering functions.
		;
		LONG	ccb_Width
		LONG	ccb_Height
	ENDSTRUCT

;  These are temporary definitions of the data structures the text
;  rendering routines will require.  All of this is probably going to
;  change dramatically when the real stuff comes online
;

;  The FontChar structure defines the image for a single character
;  The text value of the character is defined with an int32 to allow
;  either 8-bit or 16-bit text character definitions.
;
	BEGINSTRUCT	 FontChar
		UINT32	fc_CharValue
		UINT8	fc_Width
		PTR	fc_Image
	ENDSTRUCT
;  The Font definition provides a font to be used with the text rendering
;  routines.  It defines a mapping from text characters to their images
;  by pointing to an array of FontChar definitions.  It also allows
;  the programmer to control the appearance of the rendered text imagery
;  by providing for a CCB to be used when printing the characters,
;  allowing the programmer to control both the CCB's Flags field and the
;  PPMP value.
;
;  The PPMP value will come from the GrafCon supplied to the DrawChar()
;  call, as soon as I define a PPMP field in the GrafCon.
;
	BEGINSTRUCT	 Font
		UINT8	font_Height
		UINT8	font_Flags
		PTR	font_CCB

		; font_FontEntries field is significant only with RAM-resident fonts
		PTR	font_FontEntries
	ENDSTRUCT

;  === font_Flags definitions ===
FONT_ASCII            EQU &01  ; This is an ASCII font
FONT_ASCII_UPPERCASE  EQU &02  ; Lowercase will be translated to upper
FONT_FILEBASED        EQU &04  ; Font is file-based (not RAM-resident
FONT_VERTICAL         EQU &08  ; Font rendered vertically


	BEGINSTRUCT	 GrafFolio
		STRUCT	Folio,gf

		ULONG	gf_Flags

		ULONG	gf_VBLNumber

		PTR	gf_ZeroPage
		PTR	gf_VIRSPage

		ULONG	gf_VRAMPageSize
		INT32	gf_DefaultDisplayWidth
		INT32	gf_DefaultDisplayHeight

		PTR	gf_TimeoutTimer

		INT32	gf_Reserved5
		INT32	gf_Reserved6
		INT32	gf_Reserved7

		PTR	gf_VDLForcedFirst
		PTR	gf_VDLPreDisplay
		PTR	gf_VDLPostDisplay
		PTR	gf_VDLBlank
		PTR	gf_CurrentVDLEven
		PTR	gf_CurrentVDLOdd
		PTR	gf_VDLDisplayLink

		INT32	gf_Reserved1
		INT32	gf_Reserved3

		PTR	gf_CelSemaphore	 ; who has the Cel Engine?

		INT32	gf_VBLTime
		INT32	gf_VBLFreq

		INT32	gf_Reserved2

		PTR	gf_CurrentFontStream
		INT32	gf_FileFontCacheSize
		INT32	gf_FileFontCacheAlloc
		PTR	gf_FileFontCache
		PTR	gf_FontEntryHead
		PTR	gf_FontEntryButt
		STRUCT	List,gf_FontLRUList
		INT32	gf_FileFontFlags
		INT32	gf_FontBaseChar
		INT32	gf_FontMaxChar
		PTR	gf_CurrentFont
		INT32	gf_CharArrayOffset
		INT32	gf_fileFontCacheUsed


	ENDSTRUCT
;  === gf_Flags bits ===
;  none defined just now



;  ===  ===============================  ================================
;  ===                                   ================================
;  ===  Externs and Function Prototypes  ================================
;  ===                                   ================================
;  ===  ===============================  ================================

;/* routine numbers for user mode folio calls */

_SETCEWATCHDOG_	EQU	-45
_DRAWSCREENCELS_	EQU	-44
_DRAWCELS_	EQU	-43
_SUBMITVDL_	EQU	-42
_SETVDL_	EQU	-41
_DISPLAYSCREEN_	EQU	-40
;
_SETCECONTROL_	EQU	-38
_DRAWTEXT8_	EQU	-37
_GETCURRENTFONT_	EQU	-36
_SETCURRENTFONTCCB_	EQU	-35
_FILLRECT_	EQU	-34
_DRAWTO_	EQU	-33
_DRAWCHAR_	EQU	-32
;
_MOVETO_	EQU	-30
_SETCLIPHEIGHT_	EQU	-29
_SETCLIPWIDTH_	EQU	-28
_REMOVESCREENGROUP_	EQU	-27
_ADDSCREENGROUP_	EQU	-26
_SETBGPEN_	EQU	-25
_SETFGPEN_	EQU	-24
_NULLROUTINE_	EQU	-23
_SETSCREENCOLORS_	EQU	-22
_RESETSCREENCOLORS_	EQU	-21
_SETSCREENCOLOR_	EQU	-20
_DISABLEHAVG_	EQU	-19
_ENABLEHAVG_	EQU	-18
_DISABLEVAVG_	EQU	-17
_ENABLEVAVG_	EQU	-16
_SETCLIPORIGIN_	EQU	-15
_RESETREADADDRESS_	EQU	-14
_SETREADADDRESS_	EQU	-13

_CREATESCREENGROUP_	EQU	-12
_RESETFONT_	EQU	-11
;_CLOSEFONT_	EQU	-10
_DRAWTEXT16_	EQU	-9
;_OPENFILEFONT_	EQU	-8
;_OPENRAMFONT_	EQU	-7
;_SETFILEFONTCACHESIZE_	EQU	-6
_WRITEPIXEL_	EQU	-5
_GETPIXELADDRESS_	EQU	-4
_READVDLCOLOR_	EQU	-3
_READPIXEL_	EQU	-2
_MAPSPRITE_	EQU	-1


	ENDIF	   ; of IF :LNOT::DEF: |__GRAPHICS_H|

	END
