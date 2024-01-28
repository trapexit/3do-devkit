#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

/******************************/
/* Utility routine prototypes */
/******************************/
Boolean StartUp (void);
void EraseScreen (ScreenContext *sc, int32 screenNum);

/*===========================================================================================
 ============================================================================================
                                                                        Utility
 Routines
 ============================================================================================
 ===========================================================================================*/

extern ScreenContext gScreenContext;
Item gVBLIOReq;

/*********************************************************************************************
 * Routine to perform any one-time initializations
 *********************************************************************************************/
Boolean
StartUp (void)
{
  gScreenContext.sc_nScreens = 2;

  gVBLIOReq = GetVBLIOReq ();

  if (!OpenGraphics (&gScreenContext, 2))
    return false;

  if (!OpenMacLink ())
    ;

  if (!OpenSPORT ())
    return false;

  if (!OpenAudio ())
    return false;

  if ((OpenMathFolio () != 0))
    return false;

  return true;
}

/*********************************************************************************************
 * Routine to erase the current screen.
 *********************************************************************************************/
void
EraseScreen (ScreenContext *sc, int32 screenNum)
{
  Item VRAMIOReq;

  VRAMIOReq = GetVRAMIOReq ();
  SetVRAMPages (VRAMIOReq, sc->sc_Bitmaps[screenNum]->bm_Buffer, 0,
                sc->sc_nFrameBufferPages, -1);
  DeleteItem (VRAMIOReq);
}
