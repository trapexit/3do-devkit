/* Video.c -- reading and writing pict files.
 */

#include <Memory.h>
#include <QDOffscreen.h>
#include <Quickdraw.h>
#include <StandardFile.h>
#include <Types.h>
#include <errors.h>
/* #include <Sysequ.h>  */
#include <Math.h>
#include <Sane.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#include "EZQ_Defs.h"
#include "myGworld.h"

static CGrafPtr prevPort;
static GDHandle prevDev;

#define ElectricGreen_red 28575
#define ElectricGreen_green 65535
#define ElectricGreen_blue 6681

/*
 * We create an offscreen device 32-bits deep and use its pixmap in a grafport
 * In a window on the display.
 * I don't know any other way to guarantee that I've got a pixmap deep enough.
 */

int
Min (int a, int b)
{
  if (a < b)
    return a;
  return b;
}

int
Max (int a, int b)
{
  if (a > b)
    return a;
  return b;
}

short directAccess;
Rect directClipRect;
Ptr directPixmapAddr;
unsigned short directRowBytes;

extern Boolean useAlphaChannel;

setGWorldAccess (GDHandle theGWorldDevice, GWorldData *GWDPtr)
{
  PixMapHandle hPixMap;
  hPixMap = (*theGWorldDevice)->gdPMap;
  directAccess = 0;
  if (((*theGWorldDevice)->gdType != directType)
      || ((*hPixMap)->pixelSize != 32))
    {
      SysBeep (3);
      SysBeep (3);
      SysBeep (3);
    }
  else
    {
      directPixmapAddr = GetPixBaseAddr (hPixMap);
      if (directPixmapAddr == 0)
        {
          SysBeep (3);
          SysBeep (3);
          SysBeep (3);
          SysBeep (3);
        }
      else
        {
          directRowBytes = (*hPixMap)->rowBytes & 0x3FFF;
          directClipRect = (*hPixMap)->bounds;
          OffsetRect (&directClipRect, -directClipRect.left,
                      -directClipRect.top);
          directClipRect.right -= 1;
          directClipRect.bottom -= 1;

          directAccess = -1;
        }
    }
  if (GWDPtr)
    if (directAccess)
      {
        GWDPtr->initialized = -1;
        GWDPtr->clipRect = directClipRect;
        GWDPtr->rowBytes = directRowBytes;
        GWDPtr->pixmapAddr = directPixmapAddr;
      }
    else
      GWDPtr->initialized = 0;
}

#define UseSetPixel

#ifdef UseSetPixel
extern void mySetPixel (Rect *r, short x, short y, RGBColor *pixel);
#define setQPixel(r, x, y, p)                                                 \
  {                                                                           \
    if (directAccess)                                                         \
      mySetPixel (r, x, y, p);                                                \
    else                                                                      \
      xSetQPixel (r, x, y, p);                                                \
  }
#else
#define setQPixel xSetQPixel
#endif

#define RGBtoColor32(R, G, B)                                                 \
  ((long)(((R & 0xFF00L) << 8) | (G & 0xFF00L) | (long)(B >> 8)))
static void
xSetQPixel (Rect *r, short x, short y, RGBColor *pixel)
{
  register Rect *clipRect;
  register long *pixelAddr;
  if (directAccess)
    {
      x += r->left;
      y += r->top;

      if ((x >= (clipRect = &directClipRect)->right)
          || (y >= clipRect->bottom))
        return;
      if ((x <= clipRect->left) || (y <= clipRect->top))
        return;

      pixelAddr = (long *)&directPixmapAddr[(long)y * directRowBytes + x * 4];
      *pixelAddr = RGBtoColor32 (pixel->red, pixel->green, pixel->blue);
    }
  else
    SetCPixel (r->left + x, r->top + y, pixel);
}

set4Pixels (Rect *r, short x, short y, RGBColor *pixel)
{
  setQPixel (r, x, y, pixel);         /* 0 */
  setQPixel (r, x + 1, y, pixel);     /* 1 */
  setQPixel (r, x + 1, y + 1, pixel); /* 2 */
  setQPixel (r, x, y + 1, pixel);     /* 3 */
}

static Boolean
sameRect (Rect r1, Rect r2)
{
  if ((r1.left == r2.left) && (r1.right == r2.right) && (r1.top == r2.top)
      && (r1.bottom == r2.bottom))
    return true;
  return false;
}

Boolean
prepGWorld (WindowPtr window, Rect *rect, GWorldData *GWD)
{
  SetPort (window);
  GetGWorld (&prevPort, &prevDev);

  SetGWorld (prevPort, prevDev);

  LocalToGlobal ((Point *)&rect->top);
  LocalToGlobal ((Point *)&rect->bottom);

  setGWorldAccess (prevDev, GWD);
  if (!GWD->initialized)
    {
      fprintf (stdout, "\nCould not initialize Graf World Device");
      fflush (stdout);
      return (false);
    }
  return (true);
}

void
showImage (int width, int height, int imageDepth, long *pixmap,
           WindowPtr myCWindow)
{
  // char windowName[256];
  // Rect r;
  GrafPtr oldPort;
  DisplayInfo monitor;
  short bytesPerPixel;
  short x, y;
  short r, g, b;
  char *pixelPtr;
  RGBColor temp;
  // RGBTriple *imagePixel;

  GetPort (&oldPort);

  if (myCWindow == nil)
    {
      fprintf (stderr, "\nCould not create monitor window");
      fflush (stderr);
      return;
    }

  SetPort (myCWindow);

  monitor.window = myCWindow;
  monitor.r = myCWindow->portRect;
  monitor.r.left++;
  monitor.r.top++;
  monitor.GWD.initialized = 0;

  /* Calculate bytes per pixel in original image */

  switch (imageDepth)
    {
    case 16:
      bytesPerPixel = 2;
      break;
    case 24:
      bytesPerPixel = 3;
      break;
    case 32:
      bytesPerPixel = 4;
      break;
    case 40:
      bytesPerPixel = 1;
      break;
    default:
      fprintf (stderr, "\t*** Image depth = %d not supported ***", imageDepth);
      return;
    }

  if (monitor.window != nil)
    prepGWorld (monitor.window, &monitor.r, &monitor.GWD);
  if (!monitor.GWD.initialized)
    {
      fprintf (stderr, "\nCould not initialize monitor");
      fflush (stderr);
    }
  else
    {
      for (y = 0; y < height; y++)
        {
          initGWorldRow (&monitor.r, 0, y, &monitor.GWD);
          for (x = 0, pixelPtr = &((char *)pixmap)[y * width * bytesPerPixel];
               x < width; x++, pixelPtr += bytesPerPixel)
            {
              switch (imageDepth)
                {
                case 16:
                  r = (((*((unsigned short *)pixelPtr)) >> 10) & 0x1F) << 3;
                  g = (((*((unsigned short *)pixelPtr)) >> 5) & 0x1F) << 3;
                  b = (((*((unsigned short *)pixelPtr)) >> 0) & 0x1F) << 3;
                  break;
                case 24:
                  r = ColorValue (pixelPtr[0]);
                  g = ColorValue (pixelPtr[1]);
                  b = ColorValue (pixelPtr[2]);
                  break;
                case 32:
                  if (useAlphaChannel)
                    {
                      r = Max (0, Min (255, ((char)pixelPtr[0]) / 2 + 127));
                      g = r;
                      b = r;
                      /*
                      if (pixelPtr[0] == 0)
                              { // alpha channel = 0, mask off image to 0
                              r = ElectricGreen_red;
                              g = ElectricGreen_green;
                              b = ElectricGreen_blue;
                              }
                      else
                              {
                              r = Max(1, ColorValue(pixelPtr[1]));
                              g = Max(1, ColorValue(pixelPtr[2]));
                              b = Max(1, ColorValue(pixelPtr[3]));
                              }
                      */
                    }
                  else
                    {
                      r = ColorValue (pixelPtr[1]);
                      g = ColorValue (pixelPtr[2]);
                      b = ColorValue (pixelPtr[3]);
                    }
                  break;
                case 40:
                  r = ColorValue (pixelPtr[0]);
                  g = ColorValue (pixelPtr[0]);
                  b = ColorValue (pixelPtr[0]);
                  break;
                }
              temp.red = r << 8;
              temp.green = g << 8;
              temp.blue = b << 8;
              storePixelandIncrement (&monitor.GWD, &temp);
            }
        }
    }

  /*
  if (myCWindow != nil)
          {
          SetPort(myCWindow);
          EraseRect(&myCWindow->portRect);
          }
  */
  SetPort (oldPort);
}
