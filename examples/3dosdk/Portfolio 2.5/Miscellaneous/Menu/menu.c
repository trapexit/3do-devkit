
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
**  This material contains confidential information that is the property of The 3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: menu.c,v 1.10 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/menu
|||	menu - Displays a list of programs and lets the user launch one.
|||
|||	  Synopsis
|||
|||	    menu [program list]
|||
|||	  Description
|||
|||	    This program displays a list of programs on the screen, lets the user pick
|||	    one, and runs it. After the selected program completes, the list of
|||	    programs is displayed again and the user can make another selection.
|||
|||	    When the list is on screen, the user can press the up and down arrows on
|||	    the control pad to change the selected program, press A to select a
|||	    program to run, and presses Stop to exit.
|||
|||	    During startup, this program reads a file which defines the list of
|||	    programs to display on screen. Every line in this file defines a program,
|||	    using the following format:
|||
|||	    ["] label ["] cmd-line label the string that represents this program in
|||	    the screen display. Surround this string with quotes (") if it contains
|||	    any spaces. cmd-line used as a command-line. Whenever the user selects the
|||	    entry, this command-line is executed as is.
|||
|||	    Within the program list, empty lines are ignored. A semicolon indicates
|||	    the start of a comment. Any text following a semicolon is ignored until
|||	    the end of the line. Linefeed or a carriage return indicate the end of a
|||	    line.
|||
|||	  Arguments
|||
|||	    program list                 Name of a file containing a list of programs
|||	                                 to run. If this argument is not supplied,
|||	                                 the default name of menu.script is assumed.
|||
|||	  Associated Files
|||
|||	    menu.c, gfxutils.h, gfxutils.c, programlist.h, programlist.c
|||
|||	  Location
|||
|||	    examples/Miscellaneous/menu
|||
**/

#include "types.h"
#include "mem.h"
#include "stdio.h"
#include "graphics.h"
#include "event.h"
#include "task.h"
#include "string.h"
#include "ctype.h"
#include "gfxutils.h"
#include "programlist.h"


/*****************************************************************************/


/* color values for different rendering component */
#define BACKGROUND_COLOR      MakeRGB15(0,0,7)
#define PANEL_COLOR           MakeRGB15(0,0,16)
#define TEXT_COLOR            MakeRGB15(20,24,24)
#define HIGHLIGHT_PANEL_COLOR MakeRGB15(31,31,5)
#define HIGHLIGHT_TEXT_COLOR  MakeRGB15(5,5,5)
#define SHINE_COLOR           MakeRGB15(22,22,22)
#define SHADOW_COLOR          MakeRGB15(1,1,1)

/* some spacing values */
#define BORDER_WIDTH          7
#define BORDER_HEIGHT         7
#define BANNER_SPACING        8

/* text to display above the list of programs */
#define BANNER_1 "Select a program. Press A to"
#define BANNER_2 "run it, press Stop to quit."


/*****************************************************************************/


/* Render everything needed by this program. The program list contains
 * the list of programs we must present to the user. The bitmap is where
 * the menu should be displayed. oldSelection indicates the previously
 * selected item. This is negative if we're rendering the menu for the
 * first time and there was no previous selection. Finally, the newSelection
 * argument indicates which element is currently selected.
 */
static uint32 RenderMenu(ProgramList *programList, Bitmap *bitmap,
                         int32 oldSelection, int32 newSelection)
{
uint32       newWidth, newHeight;
uint32       totalWidth, totalHeight;
uint32       numVisible;
uint32       pad;
int32        left, top;
uint32       i;
uint32       labelHeight;
uint32       innerLeft;
uint32       innerWidth;
ProgramNode *node;

    /* Calculate some sizes */
    totalWidth  = 0;
    totalHeight = BORDER_HEIGHT * 2 + TextHeight(BANNER_1) + TextHeight(BANNER_2) + BANNER_SPACING;
    numVisible  = 0;
    SCANLIST(&programList->pl_Programs,node,ProgramNode)
    {
        newWidth  = TextWidth(node->pn_Label);
        newHeight = TextHeight(node->pn_Label);

        if (newHeight + totalHeight > bitmap->bm_Height)
            break;

        numVisible++;
        totalHeight += newHeight;

        if (newWidth > totalWidth)
            totalWidth = newWidth;
    }

    totalWidth += BORDER_WIDTH * 2;

    pad = 0;
    if (totalHeight + numVisible <= bitmap->bm_Height)
    {
        if (totalHeight + (numVisible * 2) <= bitmap->bm_Height)
        {
            pad = 2;
            totalHeight += (numVisible - 1) * 2;
        }
        else
        {
            pad = 1;
            totalHeight += numVisible - 1;
        }
    }

    left = (bitmap->bm_Width - totalWidth) / 2;
    top  = (bitmap->bm_Height - totalHeight) / 2;

    if (oldSelection < 0)   /* tells us to draw everything */
    {
        /* draws the banner above the box */
        SetFgColor(TEXT_COLOR);
        DrawText(BANNER_1,(bitmap->bm_Width - TextWidth(BANNER_1)) / 2,top);
        top += TextHeight(BANNER_1);
        DrawText(BANNER_2,(bitmap->bm_Width - TextWidth(BANNER_2)) / 2,top);
        top += TextHeight(BANNER_2) + BANNER_SPACING;

        totalHeight -= TextHeight(BANNER_1) + TextHeight(BANNER_2) + BANNER_SPACING;

        /* draws the box background */
        SetFgColor(PANEL_COLOR);
        DrawBox(left,top,left + totalWidth - 1, top + totalHeight - 1);

        /* draws the box's shiny border */
        SetFgColor(SHINE_COLOR);
        DrawLine(left, top, left + totalWidth - 1, top);
        DrawLine(left, top + 1, left + totalWidth - 2, top + 1);
        DrawLine(left, top, left, top + totalHeight - 1);
        DrawLine(left + 1, top, left + 1, top + totalHeight - 2);

        /* draws the box's shadowy border */
        SetFgColor(SHADOW_COLOR);
        DrawLine(left + 2, top + totalHeight - 2, left + totalWidth - 1, top + totalHeight - 2);
        DrawLine(left + 1, top + totalHeight - 1, left + totalWidth - 1, top + totalHeight - 1);
        DrawLine(left + totalWidth - 2, top + 2, left + totalWidth - 2, top + totalHeight - 1);
        DrawLine(left + totalWidth - 1, top + 1, left + totalWidth - 1, top + totalHeight - 1);
    }
    else
    {
        top += TextHeight(BANNER_1) + TextHeight(BANNER_2) + BANNER_SPACING;
    }

    innerLeft   = left + 2;
    innerWidth  = totalWidth - 4;
    left       += BORDER_WIDTH;
    top        += BORDER_HEIGHT;

    i = 0;
    SCANLIST(&programList->pl_Programs,node,ProgramNode)
    {
        if (i == numVisible)
            break;

        labelHeight = TextHeight(node->pn_Label) + pad;

        if ((oldSelection < 0)
         || (i == oldSelection)
         || (i == newSelection))
        {
            if (i == newSelection)
            {
                SetFgColor(HIGHLIGHT_PANEL_COLOR);
                DrawBox(innerLeft, top, innerLeft + innerWidth - 1, top + labelHeight - 1);
                SetFgColor(HIGHLIGHT_TEXT_COLOR);
                SetBgColor(HIGHLIGHT_PANEL_COLOR);
            }
            else
            {
                if (i == oldSelection)
                {
                    SetFgColor(PANEL_COLOR);
                    DrawBox(innerLeft, top, innerLeft + innerWidth - 1, top + labelHeight - 1);
                }
                SetFgColor(TEXT_COLOR);
                SetBgColor(PANEL_COLOR);
            }

            DrawText(node->pn_Label,left,top + pad / 2);
        }

        top += labelHeight;
        i++;
    }

    return numVisible;
}


/*****************************************************************************/


/* Handle the control pad, and make needed calls to redraw the display
 * based on user actions.
 */
static int32 EventLoop(ProgramList *programList, Bitmap *bitmap,
		       int32 selection)
{
Err                 err;
ControlPadEventData cped;
uint32              numSelections;

    numSelections = RenderMenu(programList,bitmap,-1,selection);
    while (TRUE)
    {
        err = GetControlPad(1, 1, &cped);
        if (err < 0)
        {
            selection = -1;
            break;
        }
        else if (cped.cped_ButtonBits & ControlX)
        {
            selection = -1;
            break;
        }
        else if (cped.cped_ButtonBits & ControlA)
        {
            break;
        }
        else if (cped.cped_ButtonBits & ControlUp)
        {
            if (selection > 0)
            {
                selection--;
                RenderMenu(programList,bitmap,selection + 1,selection);
            }
        }
        else if (cped.cped_ButtonBits & ControlDown)
        {
            if (selection + 1 < numSelections)
            {
                selection++;
                RenderMenu(programList,bitmap,selection - 1,selection);
            }
        }
    }

    return selection;
}


/*****************************************************************************/


/* This sits in a loop and brings up a display, waits for user selection,
 * takes down the display, and runs a program. If the user hits Stop, this
 * loop exits, and the program quits
 */
static void SetupLoop(ProgramList *programList)
{
Item         screenGroupItem;
Item         screenItem;
Item         bitmapItem;
Screen      *screen;
Bitmap      *bitmap;
int32        selection;
int32        lastSelection;
Item         vramIOReq;

    lastSelection = 0;
    while (TRUE)
    {
        selection = -1;

        vramIOReq = CreateVRAMIOReq();
        if (vramIOReq >= 0)
        {
            screenGroupItem = CreateScreenGroupVA(&screenItem,
                                                  CSG_TAG_SCREENCOUNT, 1,
                                                  CSG_TAG_DISPLAYTYPE, DI_TYPE_DEFAULT,
                                                  CSG_TAG_DONE);
            if (screenGroupItem >= 0)
            {
                EnableHAVG(screenItem);
                EnableVAVG(screenItem);
                AddScreenGroup(screenGroupItem, NULL);

                screen     = (Screen *)LookupItem(screenItem);
                bitmap     = screen->scr_TempBitmap;
                bitmapItem = bitmap->bm.n_Item;

                SetGfxUtilsItems(bitmapItem,vramIOReq);
                SetBitmap(BACKGROUND_COLOR);

                if (DisplayScreen(screenItem, 0) >= 0)
                {
                    selection = EventLoop(programList,bitmap,lastSelection);
                    lastSelection = selection;
                    RemoveScreenGroup(screenGroupItem);
                }
                DeleteScreenGroup(screenGroupItem);
            }
            DeleteVRAMIOReq(vramIOReq);
        }

        if (selection < 0)
            break;

        /* run the selection */
        RunProgram(programList,selection);
    }
}


/*****************************************************************************/


int main(int32 argc, char **argv)
{
char        *scriptName;
Err          err;
ProgramList  programList;

    if (argc > 2)
    {
        printf("Usage: menu [script file]\n");
        return 0;
    }

    scriptName = "menu.script";
    if (argc == 2)
        scriptName = argv[1];

    err = LoadProgramList(scriptName,&programList);
    if (err >= 0)
    {
        err = OpenGraphicsFolio();
        if (err >= 0)
        {
            err = InitEventUtility(1,0,1);
            if (err >= 0)
            {
                SetupLoop(&programList);
                KillEventUtility();
            }
            CloseGraphicsFolio();
        }
        UnloadProgramList(&programList);
    }

    return 0;
}
