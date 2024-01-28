#include "Init3DO.h"
#include "Portfolio.h"
#include "filefunctions.h"

#if DEBUG
#define DIAGNOSTIC(s) kprintf ("Error: %s\n", s)
#define DIAGSTRING(s, t) kprintf ("Fyi: %s %s\n", s, t)
#else
#define DIAGNOSTIC(s)
#define DIAGSTRING(s, t)
#endif

bool
OpenGraphics (ScreenContext *sc, int nScreens)
{
  long width, height;
  Screen *screen;
  Int32 i;
  Item item;

  TagArg taScreenTags[]
      = { CSG_TAG_SPORTBITS, (void *)0,    CSG_TAG_SCREENCOUNT,
          (void *)2,         CSG_TAG_DONE, (void *)0 };
  sc->sc_nScreens = nScreens;
  taScreenTags[1].ta_Arg = (void *)(sc->sc_nScreens);
  sc->sc_nScreens = nScreens;
  if (OpenGraphicsFolio ())
    {
      DIAGNOSTIC ("Cannot open graphics folio");
      return FALSE;
    }

  taScreenTags[0].ta_Arg = (void *)GETBANKBITS (GrafBase->gf_ZeroPage);

  item = CreateScreenGroup (&(sc->sc_Screens[0]), taScreenTags);
  if (item < 0)
    {
      DIAGNOSTIC ("Cannot create screen group");
      return FALSE;
    }
  AddScreenGroup (item, NULL);

  for (i = 0; i < sc->sc_nScreens; i++)
    {
      screen = (Screen *)LookupItem (sc->sc_Screens[i]);

      if (screen == NULL)
        {
          DIAGNOSTIC ("Cannot locate screens");
          return FALSE;
        }
      sc->sc_BitmapItems[i] = screen->scr_TempBitmap->bm.n_Item;
      sc->sc_Bitmaps[i] = screen->scr_TempBitmap;
      EnableHAVG (sc->sc_Screens[i]);
      EnableVAVG (sc->sc_Screens[i]);
    }

  width = sc->sc_Bitmaps[0]->bm_Width;
  height = sc->sc_Bitmaps[0]->bm_Height;

  sc->sc_nFrameBufferPages
      = (width * 2 * height + GrafBase->gf_VRAMPageSize - 1)
        / GrafBase->gf_VRAMPageSize;

  sc->sc_nFrameByteCount
      = sc->sc_nFrameBufferPages * GrafBase->gf_VRAMPageSize;
  sc->sc_curScreen = 0;

  return TRUE;
}
