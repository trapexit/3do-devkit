/*
  ISC License

  Copyright (c) 2026, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "controlpad.h"
#include "debug.h"
#include "displayutils.h"
#include "event.h"
#include "graphics.h"
#include "operror.h"
#include "stdio.h"
#include "types.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TEXT_HEIGHT 8
#define GLYPH_WIDTH 8
#define HELLO_TEXT "Hello, World!"
#define HELLO_TEXT_LEN (sizeof(HELLO_TEXT) - 1)
#define EXIT_TEXT "Press X to return"
#define EXIT_TEXT_LEN (sizeof(EXIT_TEXT) - 1)

static
void
ClearScreen(ScreenContext *sc_)
{
  Rect rect;
  GrafCon gc;

  rect.rect_XLeft = 0;
  rect.rect_YTop = 0;
  rect.rect_XRight = SCREEN_WIDTH;
  rect.rect_YBottom = SCREEN_HEIGHT;

  SetFGPen(&gc, MakeRGB15(0,0,0));

  FillRect(sc_->sc_BitmapItems[sc_->sc_curScreen],&gc,&rect);
}

static
void
DrawText(ScreenContext *sc_,
         const i32      x_,
         const i32      y_,
         const char    *text_,
         Color          color_)
{
  GrafCon gc;

  SetFGPen(&gc,color_);
  MoveTo(&gc,x_,y_);
  DrawText8(&gc,sc_->sc_BitmapItems[sc_->sc_curScreen],(const uint8*)text_);
}

static
void
WaitForExit(void)
{
  Err err;
  u32 buttons;

  do
    {
      buttons = 0;
      err = DoControlPad(1, &buttons, 0);
      if(err < 0)
        {
          printf("DoControlPad failed: ");
          PrintfSysErr(err);
          return;
        }
    }
  while((buttons & ControlX) == 0);
}

int
main(int    argc_,
     char **argv_)
{
  ScreenContext sc;
  Err err;
  int32 helloX;
  int32 helloY;
  int32 exitX;

  (void)argc_;
  (void)argv_;

  kprintf(HELLO_TEXT "\n");

  err = InitControlPad(1);
  if(err < 0)
    {
      printf("InitControlPad failed: ");
      PrintfSysErr(err);
      return (int)err;
    }

  err = CreateBasicDisplay(&sc, DI_TYPE_DEFAULT, 2);
  if(err < 0)
    {
      printf("CreateBasicDisplay failed: ");
      PrintfSysErr(err);
      KillControlPad();
      return (int)err;
    }

  sc.sc_curScreen = 0;
  helloX = ((SCREEN_WIDTH - (HELLO_TEXT_LEN * GLYPH_WIDTH)) / 2);
  helloY = ((SCREEN_HEIGHT - TEXT_HEIGHT) / 2);
  exitX  = ((SCREEN_WIDTH - (EXIT_TEXT_LEN * GLYPH_WIDTH)) / 2);

  ClearScreen(&sc);
  DrawText(&sc, helloX, helloY, HELLO_TEXT, MakeRGB15(31, 31, 31));
  DrawText(&sc, exitX, helloY + 20, EXIT_TEXT, MakeRGB15(18, 18, 18));
  DisplayScreen(sc.sc_Screens[sc.sc_curScreen], 0);

  WaitForExit();

  DeleteBasicDisplay(&sc);
  KillControlPad();
  return 0;
}
