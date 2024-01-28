
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: access.c,v 1.8 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/access
|||	access - Demonstrates how to use Access for simple user-interface
|||	    displays.
|||
|||	  Synopsis
|||
|||	    access
|||
|||	  Description
|||
|||	    Demonstrates how to use Access to display simple user-interface
prompts.
|||	    It creates a screen, and brings up Access displays in sequence.
|||
|||	  Associated Files
|||
|||	    access.c
|||
|||	  Location
|||
|||	    examples/NVRAM/access
|||
**/

#include "access.h"
#include "graphics.h"
#include "mem.h"
#include "msgport.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

/****************************************************************************/

#define ERROR(x, y)                                                           \
  {                                                                           \
    printf (x);                                                               \
    PrintfSysErr (y);                                                         \
    result = y;                                                               \
  }
#define NUM_SCREENS 1

/****************************************************************************/

static Item accessPortItem;
static Item accessItem;
static Item replyPortItem;
static Item msgItem;
static Message *msg;
static Item screenGroupItem;
static Item screenItems[NUM_SCREENS];

/****************************************************************************/

static void
ClearScreenPages (Item *screenItems)
{
  uint32 i;
  Screen *screen;
  Bitmap *bitmap;
  uint32 numBytes;
  Item sport;
  uint32 vramPageSize;

  /* If this call fails, SetVRAMPages() will also fail, and nothing bad
   * will happen. So we don't bother to check the error return.
   */
  sport = CreateVRAMIOReq ();

  for (i = 0; i < NUM_SCREENS; i++)
    {
      screen = (Screen *)LookupItem (screenItems[i]);
      bitmap = screen->scr_TempBitmap;
      numBytes = bitmap->bm_Width * bitmap->bm_Height * 2;

      vramPageSize = GetPageSize (MEMTYPE_VRAM);

      SetVRAMPages (sport, bitmap->bm_Buffer, 0,
                    (numBytes + vramPageSize - 1) / vramPageSize, ~0);
    }

  DeleteVRAMIOReq (sport);
}

/****************************************************************************/

static void
TransferScreenPages (Item *screenItems, bool toAccess)
{
  uint32 i;
  Screen *screen;
  Bitmap *bitmap;
  uint32 numBytes;

  for (i = 0; i < NUM_SCREENS; i++)
    {
      screen = (Screen *)LookupItem (screenItems[i]);
      bitmap = screen->scr_TempBitmap;
      numBytes = bitmap->bm_Width * bitmap->bm_Height * 2;

      if (toAccess)
        ControlMem ((void *)bitmap->bm_Buffer, numBytes, MEMC_OKWRITE,
                    accessItem);
      else
        ControlMem ((void *)bitmap->bm_Buffer, numBytes, MEMC_NOWRITE,
                    accessItem);
    }
}

/****************************************************************************/

static void
SendAccessMsg (uint32 callNumber, char *buffer, uint32 tags, ...)
{
  Err err;

  err = SendMsg (accessPortItem, msgItem, &tags, 0);

  if (err >= 0)
    {
      WaitPort (replyPortItem, 0);
      printf ("Call #%ld returned %ld, buffer = '%s'\n", callNumber,
              msg->msg_Result, buffer);
    }
  else
    {
      printf ("SendAccessMsg() #%ld failed with %ld\n", callNumber, err);
    }

  ClearScreenPages (screenItems);
}

/****************************************************************************/

static void
CallThemAll (void)
{
  char buf[256];

  buf[0] = 0;

  SendAccessMsg (1, buf, ACCTAG_REQUEST, ACCREQ_ONEBUTTON, ACCTAG_SCREEN,
                 screenItems[0], ACCTAG_TITLE, "Hi ho", ACCTAG_TEXT,
                 "Isn't this a great example?", ACCTAG_BUTTON_ONE, "I agree",
                 ACCTAG_FG_PEN, 0x7000, TAG_END);

  SendAccessMsg (2, buf, ACCTAG_REQUEST, ACCREQ_TWOBUTTON, ACCTAG_SCREEN,
                 screenItems[0], ACCTAG_TITLE, "Space Zappers", ACCTAG_TEXT,
                 "Wanna play more Space Zappers?", ACCTAG_BUTTON_ONE, "Yep",
                 ACCTAG_BUTTON_TWO, "Nope", ACCTAG_FG_PEN, 0x0700, TAG_END);

  ControlMem (buf, sizeof (buf), MEMC_OKWRITE, accessItem);

  strcpy (buf, "Gee");
  SendAccessMsg (3, buf, ACCTAG_REQUEST, ACCREQ_LOAD, ACCTAG_SCREEN,
                 screenItems[0], ACCTAG_TITLE, "Space Zappers",
                 ACCTAG_STRINGBUF, buf, ACCTAG_STRINGBUF_SIZE, sizeof (buf),
                 TAG_END);

  strcpy (buf, "Wow");
  SendAccessMsg (4, buf, ACCTAG_REQUEST, ACCREQ_SAVE, ACCTAG_SCREEN,
                 screenItems[0], ACCTAG_TITLE, "Space Zappers",
                 ACCTAG_STRINGBUF, buf, ACCTAG_STRINGBUF_SIZE, sizeof (buf),
                 TAG_END);

  strcpy (buf, "Zawee");
  SendAccessMsg (5, buf, ACCTAG_REQUEST, ACCREQ_DELETE, ACCTAG_SCREEN,
                 screenItems[0], ACCTAG_TITLE, "Space Zappers",
                 ACCTAG_STRINGBUF, buf, ACCTAG_STRINGBUF_SIZE, sizeof (buf),
                 TAG_END);

  ControlMem (buf, sizeof (buf), MEMC_NOWRITE, accessItem);
}

/****************************************************************************/

int
main (uint32 argc, char *argv[])
{
  MsgPort *accessPort;
  Err result;
  Err err;

  err = OpenGraphicsFolio ();
  if (err >= 0)
    {
      screenGroupItem = CreateScreenGroupVA (
          screenItems, CSG_TAG_SCREENCOUNT, (void *)NUM_SCREENS, CSG_TAG_DONE);
      if (screenGroupItem >= 0)
        {
          ClearScreenPages (screenItems);
          AddScreenGroup (screenGroupItem, NULL);

          err = DisplayScreen (screenItems[0], 0);
          if (err >= 0)
            {
              accessPortItem = FindMsgPort ("access");
              if (accessPortItem >= 0)
                {
                  accessPort = (MsgPort *)LookupItem (accessPortItem);
                  if (accessPort)
                    {
                      accessItem = accessPort->mp.n_Owner;

                      replyPortItem = CreateMsgPort ("accessexample", 0, 0);
                      if (replyPortItem >= 0)
                        {
                          msgItem = CreateMsg ("accessmsg", 0, replyPortItem);
                          if (msgItem >= 0)
                            {
                              msg = (Message *)LookupItem (msgItem);
                              if (msg)
                                {
                                  TransferScreenPages (screenItems, TRUE);

                                  CallThemAll ();
                                  result = 0;

                                  TransferScreenPages (screenItems, FALSE);
                                }
                              else
                                {
                                  ERROR ("Could not lookup message\n", 0);
                                }
                              DeleteMsg (msgItem);
                            }
                          else
                            {
                              ERROR ("Could not create message\n", msgItem);
                            }
                          DeleteMsgPort (replyPortItem);
                        }
                      else
                        {
                          ERROR ("Could not create reply message port\n",
                                 replyPortItem);
                        }
                    }
                  else
                    {
                      ERROR ("Could not lookup the Access message port\n", -1);
                    }
                }
              else
                {
                  ERROR ("Could not find the Access message port\n",
                         accessPortItem);
                }
              RemoveScreenGroup (screenGroupItem);
            }
          else
            {
              ERROR ("Could not display screen group\n", err);
            }
          DeleteScreenGroup (screenGroupItem);
        }
      else
        {
          ERROR ("Couldn't create screen group\n", screenGroupItem);
        }
      CloseGraphicsFolio ();
    }
  else
    {
      ERROR ("Could not open the graphics folio\n", err);
    }

  return ((int)result);
}
