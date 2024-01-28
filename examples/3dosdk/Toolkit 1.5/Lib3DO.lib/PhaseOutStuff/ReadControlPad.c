/*****************************************************************************
 *	File:			ReadControlPad.c
 *
 *	Contains:		Bad Old Routine to read the control pad.
 *
 *	Copyright:		(c) 1993-1994 The 3DO Company.  All Rights
 *Reserved.
 *
 *	History:
 *	07/12/94  Ian 	General library cleanup.
 *
 *	Implementation notes:
 *
 *	This is not thread-safe, and is generally obsolete.  The routines in
 *the JoyJoy example are much more robust.
 ****************************************************************************/
#include "Utils3DO.h"
#include "event.h"

static Boolean EventUtilityInited;

static int32
mapPadDataToOldJoyStick (ControlPadEventData padData)
{
  int32 rtnval = 0;

  if (padData.cped_ButtonBits & ControlDown)
    rtnval |= JOYDOWN;
  if (padData.cped_ButtonBits & ControlUp)
    rtnval |= JOYUP;
  if (padData.cped_ButtonBits & ControlRight)
    rtnval |= JOYRIGHT;
  if (padData.cped_ButtonBits & ControlLeft)
    rtnval |= JOYLEFT;
  if (padData.cped_ButtonBits & ControlA)
    rtnval |= JOYFIREA;
  if (padData.cped_ButtonBits & ControlB)
    rtnval |= JOYFIREB;
  if (padData.cped_ButtonBits & ControlC)
    rtnval |= JOYFIREC;
  if (padData.cped_ButtonBits & ControlStart)
    rtnval |= JOYSTART;
  return (rtnval);
}

int32
ReadControlPad (int32 lControlMask)
{
  static int32 oldjoy;
  int32 newjoy, returnjoy;
  Err err;
  ControlPadEventData padData;

  if (EventUtilityInited)
    ;
  else
    {
      newjoy = 0;
      oldjoy = 0;
      if ((err = InitEventUtility (1, 0, 1)) < 0)
        {
          PrintfSysErr (err);
          goto Done;
        }
      else
        {
          //			kprintf( "ReadControlPad Inited\n" );
          EventUtilityInited = true;
        }
    }

  if ((err = GetControlPad (1, 0, &padData)) < 0)
    {
      PrintfSysErr (err);
      newjoy = oldjoy;
    }
  else if (err == 1)
    {
      newjoy = mapPadDataToOldJoyStick (padData);
      //		 kprintf( "padData=%8.8X newjoy=%8.8X oldjoy %8.8X\n",
      //padData.cped_ButtonBits, newjoy, oldjoy );
    }
  else
    {
      //		 	kprintf( "oldjoy & lControlMask=%8.8X\n",
      //oldjoy & lControlMask );
      return (oldjoy & lControlMask);
    }

Done:
  returnjoy = newjoy ^ oldjoy;

  returnjoy &= newjoy;
  returnjoy |= (newjoy & lControlMask);

  oldjoy = newjoy;
  return (returnjoy);
}
