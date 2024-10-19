
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: lightgun.c,v 1.4 1995/01/04 20:49:12 mattm Exp $
**
**  Drop in routines to provide support for both the LightGun and JoyPad.
**
******************************************************************************/

#include "stdio.h"
#include "string.h"

#include "event.h"
#include "init3do.h"
#include "item.h"
#include "msgport.h"
#include "semaphore.h"

#include "lightgun.h"

/* Macro to simplify error checking. */
#define CHECKRESULT(string, val)                                              \
  if (val < 0)                                                                \
    {                                                                         \
      printf ("%s", string);                                                  \
      PrintfSysErr (val);                                                     \
    }

static uint32 blueColorEntry[32];
static uint32 standardColorEntry[32];
static uint32 timeOffset = NTSC_DEFAULT_TIMEOFFSET;

/*
 * DESCRIPTION OF LIGHTGUN HARDWARE
 * --------------------------------
 * The lightgun is not capable of returning an (X, Y) position value when
 * reporting events.  It does report a counter value that returns the time it
 * took between the beginning of a field (vertical blank) and when the
 * scan-beam passes into the field of view of the light sensor in the lightgun.
 * The X and Y value can be calculated from the timer value returned from the
 * lightgun based on how long it takes to draw from the start of one scan line
 * to the start of the scan line immediately below it (YSCANTIME) and how long
 * it takes to draw from the first pixel to the last pixel on the same scan
 * line (XSCANTIME).  A third required value, is a time offset value that takes
 * into consideration the time between the beginning of a field and when
 * scan-beam begings drawing to the first pixel on the first scan line
 * (TIMEOFFSET).
 *
 * ALTERNATIVE DISPLAYS (PAL, PC's, and LCD)
 * -----------------------------------------
 * The algorithm below should work for PAL with the only exception that the
 * YSCANTIME, XSCANTIME, and WIDTH need to be adjusted appropriately along with
 * the formulas adjusted to account for the different screen resolution.
 *
 * In theory, the lightgun should also work with PC's.  However, this is a very
 * difficult problem considering that display performance will vary from
 * machine to machine and that the size of the window displaying 3DO output can
 * vary (under Windows).  Personally ,I'm stumped since in order to get precise
 * enough numbers, you need to know the exact pixel the gun is pointed at and
 * the lightgun sensor is not capable of reliably reading a single pixel.  If
 * anyone thinks of a way to solve the PC problem, please let me know!
 *
 * The lightgun will not work with an LCD display since the hardware is based
 * around a raster display.
 *
 * EFFECTS OF SPLITTER
 * -------------------
 * The addition of a splitter will alter the timing of exactly when the
 * lightgun timer is reset at the beginning of a field.  Fortunately, the
 * change will be constant and can be fixed by altering the TIMEOFFSET.  The
 * calibration algorithm provided below will handle this.
 *
 * NOTE ON CALIBRATION
 * -------------------
 * The method used for calibrating the lightgun is based on the assumption that
 * the YSCANTIME and XSCANTIME are constant from display and scan line to scan
 * line. This is not really true.  However, the error involved with these
 * values is considerably less then the error caused by the inaccuracy of the
 * light sensor in the lightgun.  Because of this, the following calibration
 * routines are sufficient.
 *
 * NOTE ON ACCURACY
 * ----------------
 * The lightgun is inacurate by nature.  In order to help solve that, the
 * routine below averages 2 valid sensor readings into 1 value.  However, you
 * can still expect the values to be +/- 3 pixels (possibly even more) from the
 * true pixel pointed at.
 */

/*
 * handleEvent is set up to return just 1 event for each trigger press.  For a
 * valid lighgun result, the x and y value is a composite of two "hits" in
 * order to increase accuracy.  The value returned in the lged_Counter field is
 * the returned counter for the second sample and does not reflect the X,Y
 * value returned.  The reason why the lged_Counter is returned is in case you
 * need that type of information for some other purpose.
 */
static uchar
handleEvent (ScreenContext *screen, EventBrokerHeader *hdr, LG_EventData *lged,
             ControlPadEventData *cped)
{
  EventFrame *frame;
  LightGunEventData *tmpLged = NULL;
  static LG_EventData sample[2];
  uint32 cnt = 0, counter;
  static uint32 sampleCount, trackCount;
  static uchar screenFlashed = FALSE, gunRead = FALSE, initialize = FALSE;
  uchar padReturn = FALSE, gunReturn = FALSE;
  uint8 color;

  if (initialize == FALSE)
    {
      /*
       * Setup the Color Indexes for use with "flashing" the screen blue.  If
       * your program uses it's own custom clut, this will not work because it
       * assumes that the standard clut is being used.  As an alternative, you
       * may wish to use a white cel to "flash" the screen.
       *
       * In order for the "flash" to succeed, the task calling LG_WaitEvent()
       * must be the owner of the Screen Items.  I can think of two ways of
       * doing this.  The first is to have the task or thread that owns the
       * screen items call LG_WaitEvent().  The other is to give ownership up
       * to the task or thread calling LG_WaitEvent right up until the original
       * screen item owner plans on drawing, at which point the event handling
       * task must give ownership up.  This can be done either through
       * signalling or semaphores (the best way to do this depends on how
       * relative priorities are set).
       */
      for (cnt = 0; cnt < 32; cnt++)
        {
          color = (uint8)((cnt << 3)
                          | (cnt >> 2)); /* Compute standard color ramp */
          blueColorEntry[cnt] = MakeCLUTColorEntry (cnt, color, color, 255);
          standardColorEntry[cnt]
              = MakeCLUTColorEntry (cnt, color, color, color);
        }

      initialize = TRUE;
    }

  switch (hdr->ebh_Flavor)
    {
    case EB_EventRecord:
      frame = (EventFrame *)(hdr + 1);
      while (TRUE)
        {
          switch (frame->ef_EventNumber)
            {
            case EVENTNUM_ControlButtonPressed:
            case EVENTNUM_ControlButtonReleased:
            case EVENTNUM_ControlButtonUpdate:
              /*
               * Only save events on the first ControlPad.  If you want to get
               * info on others, you will need to return the eventdata along
               * with which pad it was on.
               */
              if (frame->ef_GenericPosition == 1)
                {
                  cped->cped_ButtonBits = frame->ef_EventData[0];
                  padReturn = LG_CONTROLPAD_EVENT;
                }
              else
                cped->cped_ButtonBits = 0;

              break;
            case EVENTNUM_LightGunUpdate:
              break;
            case EVENTNUM_LightGunButtonPressed:
              tmpLged = (LightGunEventData *)frame->ef_EventData;

              /*
               * If the Trigger was pressed, ramp the screen to blue and
               * initialize the variables used to track the gun location.
               */
              if (tmpLged->lged_ButtonBits & LightGunTrigger)
                {
                  /*
                   * Screen ramping is done by changing the clut.  In the
                   * default case, (0,0,0) will be "transparant" and not be
                   * flashed.
                   */
                  for (cnt = 0; cnt < screen->sc_nScreens; cnt++)
                    SetScreenColors (screen->sc_Screens[cnt], blueColorEntry,
                                     32);

                  sampleCount = trackCount = 0;
                  screenFlashed = TRUE;
                  gunRead = FALSE;

                  memset ((void *)sample, 0, 2 * sizeof (LightGunEventData));
                }
              else
                {
                  /* Calibration Button Was Pressed ... must return this */
                  memcpy ((void *)&lged->eventData, (void *)tmpLged,
                          sizeof (LightGunEventData));
                  gunReturn = LG_LIGHTGUN_EVENT;
                }

              break;
            case EVENTNUM_LightGunButtonReleased:
              /*
               * If the Trigger was released, ramp the screen back
               * down to default colors if it hasn't already.
               */
              if (screenFlashed
                  && (tmpLged->lged_ButtonBits & LightGunTrigger))
                {
                  for (cnt = 0; cnt < screen->sc_nScreens; cnt++)
                    SetScreenColors (screen->sc_Screens[cnt],
                                     standardColorEntry, 32);

                  screenFlashed = FALSE;
                }

              break;
            case EVENTNUM_LightGunFireTracking:
              /* Ignore any tracking events after a result has been returned */
              if (gunRead)
                break;

              tmpLged = (LightGunEventData *)frame->ef_EventData;

              if (tmpLged->lged_LinePulseCount > 0)
                {
                  memcpy ((void *)&sample[sampleCount], (void *)tmpLged,
                          sizeof (LightGunEventData));

                  counter = (uint32)(tmpLged->lged_Counter + timeOffset);

                  sample[trackCount & 1].x
                      = (((10 * counter) % NTSC_DEFAULT_YSCANTIME)
                         * NTSC_WIDTH)
                        / (NTSC_DEFAULT_XSCANTIME * 10);
                  sample[trackCount & 1].y
                      = (10 * counter) / NTSC_DEFAULT_YSCANTIME;

                  sampleCount++;

                  if (sampleCount > 1)
                    {
                      lged->x = (sample[0].x + sample[1].x) >> 1;
                      lged->y = (sample[0].y + sample[1].y) >> 1;

                      memcpy ((void *)&lged->eventData, tmpLged,
                              sizeof (LightGunEventData));

                      gunReturn = gunRead = LG_LIGHTGUN_EVENT;
                    }
                }

              trackCount++;

              if (trackCount > 5)
                {
                  memset ((void *)&lged->eventData, 0,
                          sizeof (LightGunEventData));

                  lged->x = lged->y = 0;

                  gunReturn = gunRead = LG_LIGHTGUN_EVENT;
                }

              /*
               * Ramp down screen if lightgun is not pointed at screen and
               * return an "empty" event.
               */
              if (gunReturn)
                {
                  for (cnt = 0; cnt < screen->sc_nScreens; cnt++)
                    SetScreenColors (screen->sc_Screens[cnt],
                                     standardColorEntry, 32);
                  screenFlashed = FALSE;
                }

              break;
            case EVENTNUM_EventQueueOverflow:
              printf ("Overflow\n");
              break;
            default:
              printf ("Unknown Event: %d\n", frame->ef_EventNumber);
              break;
            }

          frame = (EventFrame *)(frame->ef_ByteCount + (char *)frame);

          /* Check to see if end of record has been reached */
          if (frame->ef_ByteCount == 0)
            break;
        }
      break;
    default:
      printf ("Unknown Flavor\n");
      break;
    }

  return (gunReturn | padReturn);
}

/**********************************************************************
 * NAME
 *	LG_ConnectEventBroker
 *
 * PURPOSE
 *	Set up a connection to the event broker and express an interest in
 *	all lightgun events.  Code for connecting to the ControlPad is also
 *	included.
 *
 * PARAMETERS
 *	msgPortPtr
 *	msgPortItem
 *	msgItemPtr
 *	deviceFlags
 *
 * RETURN VALUE
 *	A result greater then or equal to 0 is a success.  A negative
 *	number indicates an error.
 *
 * NOTES
 *
 **********************************************************************/

int32
LG_ConnectEventBroker (MsgPort **msgPortPtr, Item *msgPortItemPtr,
                       Item *msgItemPtr, uint32 deviceFlags)
{
  Item brokerPortItem, msgPortItem, msgItem;
  MsgPort *msgPort;
  int32 sent;
  ConfigurationRequest config;

  brokerPortItem
      = FindNamedItem (MKNODEID (KERNELNODE, MSGPORTNODE), EventPortName);
  CHECKRESULT ("LG_ConnectEventBroker (FindNamedItem): ", brokerPortItem);
  if (brokerPortItem < 0)
    return (LG_ERROR);

  msgPortItem = CreateMsgPort (EVENT_MSGPORTITEM_NAME, 0, 0);
  CHECKRESULT ("LG_ConnectEventBroker (CreateMsgPort): ", msgPortItem);
  if (msgPortItem < 0)
    return (LG_ERROR);

  msgPort = (MsgPort *)LookupItem (msgPortItem);
  if (msgPort == NULL)
    {
      printf ("msgPortItem does not exist.\n");
      return (LG_ERROR);
    }

  msgItem = CreateMsg (NULL, 0, msgPortItem);
  CHECKRESULT ("LG_ConnectEventBroker (CreateMsg): ", msgItem);
  if (msgItem < 0)
    return (LG_ERROR);

  config.cr_Header.ebh_Flavor = EB_Configure;
  config.cr_Category = LC_Observer;
  config.cr_QueueMax = 10;

  memset (&config.cr_TriggerMask, 0x00, sizeof (config.cr_TriggerMask));
  memset (&config.cr_CaptureMask, 0x00, sizeof (config.cr_CaptureMask));

  config.cr_TriggerMask[0]
      = EVENTBIT0_ControlButtonPressed | EVENTBIT0_ControlButtonReleased
        | EVENTBIT0_ControlButtonUpdate | EVENTBIT0_LightGunButtonPressed
        | EVENTBIT0_LightGunButtonReleased | EVENTBIT0_LightGunUpdate
        | EVENTBIT0_LightGunFireTracking;
  config.cr_TriggerMask[2] = EVENTBIT2_ControlPortChange;

  sent = SendMsg (brokerPortItem, msgItem, &config, sizeof (config));
  CHECKRESULT ("LG_ConnectEventBroker (SendMsg): ", sent);
  if (sent < 0)
    return (LG_ERROR);

  *msgPortPtr = msgPort;
  *msgPortItemPtr = msgPortItem;
  *msgItemPtr = msgItem;

  return (LG_SUCCESS);
}

/**********************************************************************
 * NAME
 *	LG_WaitEvent
 *
 * PURPOSE
 *	Reads events from the Event Broker and returns the appropriate
 *	information concerning the event received.  This handles both
 *	LightGun and ControlPad events.  If no events are currently queued
 *	up, or not enough events have been received to return a valid light
 *  gun hit, the program will return immediately.  Sections of the code
 *  can be uncommented out below in order to make the routine wait for
 *  events.  This is usefull for when events are handled in a seperate
 *  task or thread.
 *
 * PARAMETERS
 *	screen
 *	msgPortPtr
 *	msgPortItem
 *	msgItem
 *	lged
 *	cped
 *
 * RETURN VALUE
 *	A negative number indicates an error. A value of 0 indicates that
 *	no event registered.  A positive value indicates that an event was
 *	received.  Be sure to check the return value for an event or you may
 *	be using bad data from the event data structures passed in.
 *
 * NOTES
 *
 **********************************************************************/

int32
LG_WaitEvent (ScreenContext *screen, MsgPort *msgPort, Item msgPortItem,
              Item msgItem, LG_EventData *lged, ControlPadEventData *cped)
{
  Item eventItem;
  Message *event;
  EventBrokerHeader *msgHeader;
  uchar result = 0;

  while ((eventItem = GetMsg (msgPortItem)) != 0)
    {
      CHECKRESULT ("LG_WaitEvent (GetMsg): ", eventItem);
      if (eventItem < 0)
        return (LG_ERROR);

      event = (Message *)LookupItem (eventItem);

      msgHeader = (EventBrokerHeader *)event->msg_DataPtr;

      if (eventItem == msgItem)
        {
          CHECKRESULT ("LG_WaitEvent (event->msg_Result): ",
                       (Err)event->msg_Result);
          if ((int32)event->msg_Result < 0)
            return (LG_ERROR);
        }
      else
        {
          result |= handleEvent (screen, msgHeader, lged, cped);
          ReplyMsg (eventItem, 0, NULL, 0);
        }
    }

  return (result);
}

/**********************************************************************
 * NAME
 *	LG_CalibrateGun
 *
 * PURPOSE
 *	Calibrates the lightgun on a single point.  This function will be
 *  going away for a better one eventually.
 *
 * PARAMETERS
 *	screen
 *	msgPortPtr
 *	msgPortItem
 *	msgItem
 *	x
 *	y
 *
 * RETURN VALUE
 *	A zero indicates that the calibration was not done.  A positive
 *	number indicates success.
 *
 * NOTES
 *
 **********************************************************************/
int32
LG_CalibrateGun (ScreenContext *screen, MsgPort *msgPort, Item msgPortItem,
                 Item msgItem, int32 x, int32 y)
{
  int32 sig = 0;
  LG_EventData lged;
  ControlPadEventData cped;

  while (sig == 0)
    sig = LG_WaitEvent (screen, msgPort, msgPortItem, msgItem, &lged, &cped);

  /* Check to make sure a valid X,Y lightgun position was returned from
   * LG_WAITEVENT */
  if (!(sig & LG_LIGHTGUN_EVENT) || (lged.eventData.lged_LinePulseCount == 0))
    return (0);

  /*
   * Since the lightgun returns a composite X,Y value from 2 samples, the
   * theoretical lged_Counter needs to be computed in order to set the new
   * timeoffset.
   */
  timeOffset = ((NTSC_DEFAULT_YSCANTIME * y / 10)
                + (NTSC_DEFAULT_XSCANTIME * x / NTSC_WIDTH))
               - ((lged.x * NTSC_DEFAULT_XSCANTIME / NTSC_WIDTH)
                  + (lged.y * NTSC_DEFAULT_YSCANTIME / 10) - timeOffset);

  return (1);
}
