
/******************************************************************************
 **
 **  Copyright(C) 1995, an unpublished work by The 3DO Company. All rights
 *reserved.
 **  This material contains confidential information that is the property of The
 *3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: lrex.c,v 1.16 1995/01/16 19:48:35 vertex Exp $
 **
 ******************************************************************************/

/**
   |||	AUTODOC PUBLIC examples/lrex
   |||	lrex - Demonstrates subcel extraction from an image.
   |||
   |||	  Synopsis
   |||
   |||	    lrex \<imagefile>
   |||
   |||	  Description
   |||
   |||	    Demonstrates how to extract a cel from a frame buffer or bitmap
   image,
   |||	    that can then be redrawn into the frame buffer. Can be used for
   things
   |||	    like breaking up a static or dynamic image(like the video in
   Mystery
   |||	    Matinee in Twisted) and reorganizing it.
   |||
   |||	    Subcel extraction from a frame buffer(bitmap) is slightly
   different from
   |||	    extraction from another cel. This program handles extraction from
   bitmaps
   |||	    only.
   |||
   |||	    The extraction can easily be done with the appropriate call to the
   Lib3DO
   |||	    functions(CreateLRFormCel), but this example shows how to do it
   long
   |||	    hand.
   |||
   |||	    When extracting from bitmaps, pay attention to the order in which
   the
   |||	    pixels are represented.
   |||
   |||	    The program is based on animsample and aaplayer. The user can
   manipulate
   |||	    the extracted cel.
   |||
   |||	  Caveats
   |||
   |||	    In performing the cel manipulation, the source for the cel may end
   up
   |||	    pointing to garbage. This is especially likely if the program is
   |||	    distorting the cel near the edge of the extracting bitmap.
   |||
   |||	  Arguments
   |||
   |||	    imagefile                    Name of 3DO image file.
   |||
   |||	  Associated Files
   |||
   |||	    lrex.c, lrexerror.h, lrex.h
   |||
   |||	  Location
   |||
   |||	    examples/Graphics/lrex
   |||
**/

#include "lrex.h"

void
Usage(void)
{
  printf("C                     toggles between moving the cel, and warping "
        "the corners\n");
  printf("d-pad                 moves cel around or warps the corners\n");
  printf("B + D-pad             scales the cel\n");
  printf("Left or Right Shift   cycles through current corner\n");
  printf("A + up or down        toggles pmode\n");
  printf("Stop                  exits\n");

  return;
}

/*********************
 **	Initialize - initialization routine
 **
 **	  Synopsis
 **
 **	    static int32 Initialize( ScreenContext *sc, int32 nScreens )
 **
 **	  Description
 **
 **	    This function opens the graphics and math folios, initializes for 1
 **	    controlpad, and creates an error text and a couple of of IO req
 *items
 **
 **	  Arguments
 **
 **	    sc	                 pointer to a screen context structure
 **
 **	    nScreens             the number of screens to create
 **
 **
 **	  Return Value
 **
 **	    The function returns 0 if successful or an error code(a negative
 *value) *	    if an error occurs.
 **
 **
 **	  Implementation
 **
 **	    Called prior to main loop
 **
 **	  Caveats
 **
 **	   none
 **
 **	  Associated Files
 **
 **	    lrex.h lrexerror.h controlpad.h controlpad.c
 **
 **	  See Also
 **
 **
 *********************/
static int32
Initialize(ScreenContext *sc, int32 nScreens)
{

  int32 status = 0;

  /*
    create an error text item to return opera style errors
  */
  status = CreateItem(MKNODEID(KERNELNODE, ERRORTEXTNODE), LREXErrorTags);
  if(status < 0)
    goto CLEANUP;

  gLREXErrorItem = status;

  /*
    open the graphics folio, fill in the screen context structure
    for nScreens screens
  */
  if(!CreateBasicDisplay(sc, DI_TYPE_NTSC,(int)nScreens))
    {
      status = LREXGRAPHICS_ERR;
      goto CLEANUP;
    }

  sc->sc_curScreen = 0;

  /*
    get an io req item for SPORT transfers, and for using the VBL timer
  */
  status = CreateVRAMIOReq();
  if(status < 0)
    goto CLEANUP;

  gVramIOReq = status;

  status = CreateVBLIOReq();
  if(status < 0)
    goto CLEANUP;

  gVblIOReq = status;

  /*
    init structures and resources for querying the controlpad
  */
  status = InitControlPad(1);
  if(status < 0)
    goto CLEANUP;

 CLEANUP:
  return status;
}

/*********************
 **	SetLRForm - sets ccb fields for an LR form cel
 **
 **	  Synopsis
 **
 **	    void SetLRForm( ScreenContext *sc, CCB *lrccb )
 **
 **	  Description
 **
 **	    This function sets the fields for an LR form cel used in sub cel
 *extraction *	    from bitmaps or frame buffers
 **
 **	  Arguments
 **
 **	    sc	                 pointer to a screen context structure
 **
 **	    lrccb          		 pointer to the ccb whose fields will
 *be set
 **
 **
 **	  Return Value
 **
 **	    The function is void
 **
 **
 **	  Implementation
 **
 **	    Called after allocating memory for the ccb
 **
 **	  Caveats
 **
 **	   none
 **
 **	  Associated Files
 **
 **	    lrex.h lrexerror.h controlpad.h controlpad.c
 **
 **	  See Also
 **
 **
 *********************/
void
SetLRForm(ScreenContext *sc, CCB *lrccb)
{

  lrccb->ccb_Flags
    = CCB_LDSIZE   /* Load hdx-y and vdx-y from CCB */
    | CCB_LDPRS  /* Load ddx-y from CCB */
    | CCB_LDPPMP /* Load the PIXC word from CCB */
    | CCB_YOXY   /* Load X,YPos from the ccb */
    | CCB_CCBPRE /* Load the preamble words from CCB,
                    as opposed to them being in the source
                    data */
    | CCB_ACW    /* Pixels facing forward will be seen */
    | CCB_ACCW   /* Pixels facing backward will be seen */
    | CCB_SPABS  /* SourcePtr is absolute ptr, not a relative one */
    | CCB_NPABS  /* NextPtr is an absolute ptr, not a relative one */
    | CCB_PPABS  /* PLUTPtr is absolute ptr, not a relative one */
    | CCB_BGND   /* black will be drawn black */
    | CCB_ACE    /* Enable both cel engines */
    | PMODE_ONE  /* use pmode zero to render the cel */
    | CCB_LAST;  /* Set the last flag, remember to set NextPtr = NULL */

  /*
    set the source of this cel to point into the 3DO image
  */

  lrccb->ccb_NextPtr = 0;
  lrccb->ccb_PLUTPtr = 0;

  lrccb->ccb_XPos = 0;
  lrccb->ccb_YPos = 0;
  lrccb->ccb_HDX = 1 << 20;
  lrccb->ccb_HDY = 0;
  lrccb->ccb_VDX = 0;
  lrccb->ccb_VDY = 1 << 16;
  lrccb->ccb_HDDX = 0;
  lrccb->ccb_HDDY = 0;
  lrccb->ccb_PIXC
    = 0x1F001F81; /* 50% translucent in pmode-0, fully opaque in pmode 1 */

  /*
    set the height and width for the cel... this is just information, the
    real information in located in the preamble words of the cel.

    Important note, is that due to the layout of the bitmaps, the height
    specified(implying how many rows the cel takes) is half of what you would
    expect.  So, for example, we want to extract a 64x64 pixel cel.  That
    means we want 64 rows out of what we see on the screen... which is
    actually 32 "rows" in the memory mapping of the image.
  */
  lrccb->ccb_Width = 64;
  lrccb->ccb_Height = 32;

  /*
    now, to make this a valid cel, we have to set the preamble words,
    which is where all the important information about rendering the cel is
    located.

    PRE0 contains some basic information about the cel, such as how many
    bits per pixel it is, and how many rows of source data there are...
    remember this is half of what you think it would be for an LR-form cel.

    The important bits of PRE0 are 15-6, or VCNT, which is the number of
    vertical rows minus 1 of data, bit 4 for whether the cel is coded or
    uncoded, and bits 2-0 for how many bits per pixel the cel is.

    The important bits of PRE1 are 25-16 for the word offset for an 8 or
    16 bpp cel(WOFFSET(10))--the number of cel data words minus two from the
    beginning of one row to the beginning of the next	row of cel data, bit 11
    for specifing whether this cel's is LR-form or linear form, and bits 10-0
    which specify how many pixels minus one in each row of the cel's data
   (HPCNT)

    hardware.h contains a number of useful #define's for shifts and stuff
  */

  lrccb->ccb_PRE0 =((lrccb->ccb_Height - PRE0_VCNT_PREFETCH)
                     << PRE0_VCNT_SHIFT) /* #rows - 1 */
    | PRE0_LINEAR        /* uncoded cel */
    | PRE0_BPP_16;       /* 16 bpp */

  lrccb->ccb_PRE1 =((sc->sc_Bitmaps[0]->bm_Width - PRE1_WOFFSET_PREFETCH)
                     << PRE1_WOFFSET10_SHIFT) /* width of bitmap - 2 */
    | PRE1_LRFORM             /* LR-form data */
    |(lrccb->ccb_Width - PRE1_TLHPCNT_PREFETCH)
    << PRE1_TLHPCNT_SHIFT; /* #pixels-1 per row */

  return;
}

/*********************
 **	main - main routine
 **
 **	  Synopsis
 **
 **	    int main( int32 argc, char *argv[] )
 **
 **	  Description
 **
 **	    This function initializes the system, loads in a 3DO background and
 **	    creates an LR form cel, drops into a main loop where it queries the
 *control *	    pad and reacts to it, double buffers, and renders all the
 *artwork
 **
 **	  Arguments
 **
 **	    argc	         number of command line arguments
 **
 **	    argv		 the command line arguments
 **
 **
 **	  Return Value
 **
 **	    The function exits with  0.
 **
 **
 **	  Implementation
 **
 **
 **	  Caveats
 **
 **	   none
 **
 **	  Associated Files
 **
 **	    lrex.h lrexerror.h controlpad.h controlpad.c
 **
 **	  See Also
 **
 **
 *********************/
int
main(int32 argc, char *argv[])
{
  ScreenContext sc;
  ubyte *background = NULL;

  uint32 button;

  CCB *lrccb = NULL;

  int32 status = 0, offset;
  char *imagefile;

  printf("Usage:: %s <imagefile>\n", argv[0]);  
  if(argc != 2)
    {
      imagefile = "seafloor.imag";
    }
  else
    {
      imagefile = argv[1];
    }

  printf("%s %s\n", argv[0], VERSION);
  Usage();

  if((status = Initialize(&sc, 2)) < 0)
    {
      PrintfSysErr(status);
      exit(0);
    }

  /*
    Load in the background 3DO image.  Let LoadImage() allocate a buffer
    for us(second parameter)... and we don't care about the VDL with
    the image( third parameter)
  */
  background =(ubyte *)LoadImage(imagefile, NULL, NULL, &sc);
  if(background == NULL)
    {
      status = LREXLOADIMAGE_ERR;
      goto CLEANUP;
    }

  /*
    Allocate some space for a ccb.  We will use this later.  The reason
    why we want to dynamically allocate the ccb* rather than declare
    a ccb and set a pointer to it is future compatibility.  Allocating
    ensures where we get the memory from.  In this case, its DMA accessible
   (very important for the cel engine).  Not doing the allocation means
    that we can't control were the memory comes from... and in the future,
    that might be bad(ie. the code would break)

    Also, set all the relevant stuff for this cel to be rendered by the cel
    engine as an LR-form cel.

    What is LR-form, you ask?  Well, that is the name given to the arrangement
    of bytes in a 3DO image and for the frame buffers(ie, for a bitmap). What
    this means is that whereas, what you see on the screen is an arrangement
    of pixels like so:

    1	2	3	4	5
    6	7	8	9	10				(assume a 5 pixel wide
    screen)....

    The pixels are arranged in memory(in the array that really is the bitmap)
    like so:

    1	6	2	7	3	8	4	9	5	10

    So, its that adjacent vertical pixels that are adjacent in memory.

    This makes things a little weird... especially in the realm of pointer
    arithmetic and extracting a cel from this format.... you need to tell the
    cel engine that the source data for this cel is arranged differently from
    what it would normally expect.

    Something else to keep in mind... images are pointer to ubyte's... that is
    8 bits... though each pixel is 16 bits....

    This weirdness also limits what you can extract.  You can only really
    extract from even lines of the bitmap, because the LRFORM bit only affects
    how the cel engine displays the data in its source ptr, not the way that
    it grabs the data.  So for instance, if I were to try to grab the odd line
    from the above data, I would get source for the cel that looks like:

    6  2  7  3  8... and so on.  Not exactly what you want, is it?
  */
  lrccb =(CCB *)AllocMem(sizeof(CCB), MEMTYPE_CEL | MEMTYPE_FILL | 0x00);
  if(lrccb == NULL)
    {
      status = LREXALLOC_ERR;
      goto CLEANUP;
    }

  SetLRForm(&sc, lrccb);

  /*
    Set some global variables that are going to be used to specify
    where to draw the cel.
  */
  gP[0].pt_X = gP[3].pt_X = lrccb->ccb_XPos >> 16;
  gP[1].pt_X = gP[2].pt_X = gP[0].pt_X + lrccb->ccb_Width;
  gP[0].pt_Y = gP[1].pt_Y = lrccb->ccb_YPos >> 16;
  gP[2].pt_Y = gP[3].pt_Y = gP[0].pt_Y + lrccb->ccb_Height;

  gMoveIncr = MOVE_DELTA << 16;

  /*
    Drop into the main loop.  We stay here until the user quits
    by hitting stop on the control pad.

    In this loop, we react to the control pad, double buffer by toggling
    what the current screen is, and draw the background and the lr form cel.
  */
  while(true)
    {

      /*
        Query the control pad
      */
      status = DoControlPad(1, &button, CONTROL_ALL);
      if(status < 0)
        break;

      /*
        react to any button presses
      */
      if(!HandleButton(button, lrccb))
        break;

      /*
        Here is where it gets confusing... finding out the address
        of the pixel in the frame buffer or bitmap that we want the cel to
        start at.

        Basically,

        offset =(y/2) * 1280 + 4x +(y & 1) << 1

        Reasons:

        x identifies the pixel.  Given the LR form... in any row, a pixel at
        position x is really at a position twice that(remember, the second
        row's pixels are inbetween).

        Then, pixel 2x is at byte 4x(2x * 2) because there are 2 bytes per
        pixel.

        y identifies which pixel.  For any odd y, the y is really located in
        the first row. So y/2, tells us the row.  Then, multiply that by 2 *
        bitmap width gives us where in the bitmap the pixel really is.

        Multiply that by two to get the byte offset, again because each pixel
        is 2 bytes.

        Yikes!

        The last term is to adjust the offset for odd vs even lines.  For odd
        lines, you have to move over to the next pixel(multiply by 2 for 2
        bytes per pixel).  If you are extracting only from the even lines,
        then that term drops out.  We are only extracting from even lines, so
        this term is not included below.

        NOTE::

        There is also, technically, a bug here in the program.  If you take
        the cel, flip it vertically, then you are bound to get bogus data
        written into the cel as the cel engine tries to read stuff from memory
        addresses well beyond the defined bitmap.  Moral of this story... try
        to extract stuff from valid addresses.  Normally, you don't see
        this... the cel engine draws bogus data, but clips itself before it
        really draws.  With strange distortions, the cel engine goes ahead and
        draws whatever it finds in the memory specified.

      */
      offset
        =(gP[0].pt_Y / 2) * sc.sc_Bitmaps[0]->bm_Width * 4 + gP[0].pt_X * 4;

      lrccb->ccb_SourcePtr =(CelData *)(background + offset);

      /*
        Toggle the current screen counter.

        Copy the background pic to the current screen.  We could have
        used DrawImage... but it just calls this function... so we
        deal with some extra overhead.

        BTW... this does an implicit WaitVBL before it returns... so
        we don't need to do one ourselves in worrying about synching
        with the electron beam.

        One the other hand, if we used SetVRAMPages(), then we might have
        to.

        Also, we might have chosen to use the Defer family of these functions
        if we had some extra processing to do while we were waiting
        for the vbl.  We don't, so we don't use that function

        We draw the cel with DrawCels().  If we had multiple cels to draw,
        I would have linked them all together through the NextPtr and
        drawn them all with a single call.

        This is substantially faster than calling DrawCels() on each
        cel.

        Also, I don't use DrawScreenCels()... that was designed for
        multiple bitmaps, and we don't have that... so, again, why
        incur some extra overhead?
      */
      sc.sc_curScreen ^= 1;

      CopyVRAMPages(gVramIOReq, sc.sc_Bitmaps[sc.sc_curScreen]->bm_Buffer,
                     background, sc.sc_nFrameBufferPages, ~0);

      DrawCels(sc.sc_BitmapItems[sc.sc_curScreen], lrccb);

      /*
        This queues up the screen we just drew to be displayed at
        the next vertical blank.  The function returns immediately, however
      */
      DisplayScreen(sc.sc_Screens[sc.sc_curScreen], 0);
    }

 CLEANUP:
  if(status < 0)
    PrintfSysErr(status);

  FadeToBlack(&sc, 20);

  FreeMem(lrccb, sizeof(CCB));
  if(background)
    UnloadImage(background);

  DeleteVBLIOReq(gVblIOReq);
  DeleteVRAMIOReq(gVramIOReq);
  DeleteItem(gLREXErrorItem);

  KillControlPad();

  DeleteBasicDisplay(&sc);

  printf("Exiting\n");

  exit(0);
}

/*********************
 **	HandleButton - handles events off of the control pad
 **
 **	  Synopsis
 **
 **	    static bool HandleButton( uint32 button, CCB *ccb )
 **
 **	  Description
 **
 **	    This function takes an event to from the control pad and reacts to
 *it. *	    according to the following logic.
 **
 **	    ControlX					quits the program
 **	    ControlB + DPAD				scales the cel
 **	    ControlA + up/down			toggles pmode
 **	    ControlC					toggles between moving
 *the cel and warping the corners *	    Left/RightShift
 *cycles through which corner to warp
 **	    DPAD						moves the cel or
 *warps a corner, depending on mode
 **
 **	  Arguments
 **
 **	    button	         which buttons are down in this event
 **
 **	    ccb			 pointer to the cel whose positioning we want
 *to modify
 **
 **
 **	  Return Value
 **
 **	    The function returns nothing
 **
 **
 **	  Implementation
 **
 **
 **	  Caveats
 **
 **	   Whoa!  Tons of stuff.  If you were doing this yourself, how would
 *you do it?
 **
 **	   The idea is to try to minimize the calls to MapCel().  Why would
 *you want to *	   minimize those calls?  Because MapCel() is slow.  There are
 *8 fields in the CCB *	   which determine the region in which the cel engines
 *render a cel.  MapCel() sets *	   all 8 of those fields.  Six of those,
 *require divides, which are slower than *	   molasses on a cold winters
 *day.  Most position changes for a cel don't *	   depend on all 8 of those
 *fields... some can stay the same... so you end up *	   wasting a lot of
 *time doing unnecessary math.  This is most generally the *	   case when
 *you are doing simple moving and scaling of the cel(ie. adding no *
 *perspective).  Most of the time, you can pretty much figure out what the
 **	   ccb scaling factors are and do the math yourself.  The formulas are
 *(given *	   that the order of points is clockwise, starting at the upper
 *left):
 **
 **	   x = x0
 **	   y = y0
 **	   hdx =(x1-x0)/w
 **	   hdy =(y1-y0)/w
 **
 **	   vdx =(x3-x0)/h
 **	   vdy =(y3-y0)/h
 **
 **	   hddx = [(x2-x3) -(x1-x0)]/wh =(hdx1-hdx0)/h
 **	   hddy = [(y2-y3) -(y1-y0)]/wh =(hdy1-hdy0)/w
 **
 **	   where w and h are the pixel dimensions of the cel.
 **
 **	   Keep in mind what base the ccb fields are.  XPos, YPos, VDX, and VDY
 *are 16.16 *	   HDX, HDY, HDDX and HDDY are 12.20.
 **
 **	   Secondly, there is MapP2Cel(), which works on cels whose dimensions
 *are powers of
 **	   two.  There is also FastMapCel() and InitFastMapCel() which are
 *faster *	   than MapCel(), but provide less precision.
 **
 **	  Associated Files
 **
 **	    lrex.h lrexerror.h controlpad.h controlpad.c
 **
 **	  See Also
 **
 **	    MapCel(), MapP2Cel(), FastMapCel(), InitFastMapCel()
 *********************/
static bool
HandleButton(uint32 button, CCB *ccb)
{

  if(button == ControlX)
    return false;

  if(button & ControlB)
    {

      if(button & ControlUp)
        {
          gP[2].pt_Y -= MOVE_DELTA;
          gP[3].pt_Y -= MOVE_DELTA;

          ccb->ccb_VDY =((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;
        }

      if(button & ControlDown)
        {
          gP[2].pt_Y += MOVE_DELTA;
          gP[3].pt_Y += MOVE_DELTA;

          ccb->ccb_VDY =((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;
        }

      if(button & ControlRight)
        {
          gP[1].pt_X += MOVE_DELTA;
          gP[2].pt_X += MOVE_DELTA;

          ccb->ccb_HDX =((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
        }

      if(button & ControlLeft)
        {
          gP[1].pt_X -= MOVE_DELTA;
          gP[2].pt_X -= MOVE_DELTA;

          ccb->ccb_HDX =((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
        }
    }

  else if(button == ControlLeftShift)
    {
      if(--gCorner < 0)
        gCorner = 3;

      printf("Corner %ld\n", gCorner);
    }

  else if(button == ControlRightShift)
    {
      if(++gCorner > 3)
        gCorner = 0;

      printf("Corner %ld\n", gCorner);
    }

  else if(button == ControlC)
    {
      if(gMoveCorners)
        {
          gMoveCorners = false;
          printf("Moving cel mode\n");
        }
      else
        {
          gMoveCorners = true;
          printf("Warp corners mode\n");
        }
    }

  else if(button & ControlA)
    {

      if(button & ControlUp)
        {

          /*
            This will tweak the pmode setting in the flags fields.

            CCB_POVER_MASK is a bit mask with 1's in all the three relevant
            bits of the flags, 0's elsewhere.  You can use this &'d with a
            PMODE_ONE or PMODE_ZERO or PMODE_PDC, to find out what is the
            current pmode for the cel.  &=~'d it with the flags, will clear
            the current pmode and allow you to set whichever one you want.

            PMODE_ONE	tells the cel engine to render the cel with the upper
            16 bits of the PIXC PMODE_ZERO	tells the cel engine to render
            the cel with the upper 16 bits of the PIXC PMODE_PDC	tells
            the cel engine to render the cel with whatever pmode comes out of
            the pixel decoder(ie. per pixel pmode)
          */

          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_ONE;
        }

      if(button & ControlDown)
        {
          ccb->ccb_Flags &= ~CCB_POVER_MASK;
          ccb->ccb_Flags |= PMODE_ZERO;
        }
    }

  else
    {
      if(gMoveCorners)
        {

          if(button & ControlUp)
            gP[gCorner].pt_Y -= MOVE_DELTA;

          if(button & ControlDown)
            gP[gCorner].pt_Y += MOVE_DELTA;

          if(button & ControlRight)
            gP[gCorner].pt_X += MOVE_DELTA;

          if(button & ControlLeft)
            gP[gCorner].pt_X -= MOVE_DELTA;

          if(button)
            {
              switch(gCorner)
                {
                case 0:

                  /*
                    Every field in the ccb is changing, so I might as well use
                    MapCel() In all the other cases, I only need to
                    recalculate those fields which have changed due to the
                    changed coordinates.  So I spare myself most of the
                    divides that would take place, and speed up my program
                    just a bit.
                  */
                  MapCel(ccb, gP);
                  break;
                case 1:
                  ccb->ccb_HDX
                    =((gP[1].pt_X - gP[0].pt_X) << 20) / ccb->ccb_Width;
                  ccb->ccb_HDY
                    =((gP[1].pt_Y - gP[0].pt_Y) << 20) / ccb->ccb_Width;

                  ccb->ccb_HDDX =(((gP[2].pt_X - gP[3].pt_X)
                                    -(gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY =(((gP[2].pt_Y - gP[3].pt_Y)
                                    -(gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);
                  break;
                case 2:
                  ccb->ccb_HDDX =(((gP[2].pt_X - gP[3].pt_X)
                                    -(gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY =(((gP[2].pt_Y - gP[3].pt_Y)
                                    -(gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);
                  break;
                case 3:
                  ccb->ccb_VDX
                    =((gP[3].pt_X - gP[0].pt_X) << 16) / ccb->ccb_Height;
                  ccb->ccb_VDY
                    =((gP[3].pt_Y - gP[0].pt_Y) << 16) / ccb->ccb_Height;

                  ccb->ccb_HDDX =(((gP[2].pt_X - gP[3].pt_X)
                                    -(gP[1].pt_X - gP[0].pt_X))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);

                  ccb->ccb_HDDY =(((gP[2].pt_Y - gP[3].pt_Y)
                                    -(gP[1].pt_Y - gP[0].pt_Y))
                                   << 20)
                    /(ccb->ccb_Height * ccb->ccb_Width);
                  break;
                default:
                  break;
                }
            }
        }

      else
        {

          if(button & ControlUp)
            {
              ccb->ccb_YPos -= gMoveIncr;
              gP[0].pt_Y -= MOVE_DELTA;
              gP[1].pt_Y -= MOVE_DELTA;
              gP[2].pt_Y -= MOVE_DELTA;
              gP[3].pt_Y -= MOVE_DELTA;
            }

          if(button & ControlDown)
            {
              ccb->ccb_YPos += gMoveIncr;
              gP[0].pt_Y += MOVE_DELTA;
              gP[1].pt_Y += MOVE_DELTA;
              gP[2].pt_Y += MOVE_DELTA;
              gP[3].pt_Y += MOVE_DELTA;
            }

          if(button & ControlRight)
            {
              ccb->ccb_XPos += gMoveIncr;
              gP[0].pt_X += MOVE_DELTA;
              gP[1].pt_X += MOVE_DELTA;
              gP[2].pt_X += MOVE_DELTA;
              gP[3].pt_X += MOVE_DELTA;
            }

          if(button & ControlLeft)
            {
              ccb->ccb_XPos -= gMoveIncr;
              gP[0].pt_X -= MOVE_DELTA;
              gP[1].pt_X -= MOVE_DELTA;
              gP[2].pt_X -= MOVE_DELTA;
              gP[3].pt_X -= MOVE_DELTA;
            }
        }
    }

  return true;
}
