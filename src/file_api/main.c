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
#include "hardware.h"
#include "operror.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TEXT_HEIGHT 8
#define GLYPH_WIDTH 8
#define ROW_HEIGHT 12
#define FILE_PATH "/NVRAM/file_api.txt"
#define RANDOM_TEXT_MAX 16

static const char randomChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

static
void
GenerateRandomString(char *buffer_,
                     int32 bufferSize_)
{
  uint32 randomValue;
  int32 length;
  int32 i;
  int32 maxLength;

  maxLength = bufferSize_ - 1;
  if(maxLength > RANDOM_TEXT_MAX)
    maxLength = RANDOM_TEXT_MAX;

  randomValue = ReadHardwareRandomNumber();
  length = (int32)(randomValue % maxLength) + 1;

  for(i = 0; i < length; i++)
    {
      randomValue = ReadHardwareRandomNumber();
      buffer_[i] = randomChars[randomValue % (sizeof(randomChars) - 1)];
    }

  buffer_[length] = 0;
}

static
int32
CheckReadback(const char *buffer_,
              int32       bytesRead_,
              const char *expected_)
{
  int32 expectedSize;

  expectedSize = (int32)strlen(expected_);

  if(bytesRead_ != expectedSize)
    return -1;

  if(strcmp(buffer_, expected_) != 0)
    return -1;

  return 0;
}

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

  SetFGPen(&gc, MakeRGB15(0, 0, 0));
  FillRect(sc_->sc_BitmapItems[sc_->sc_curScreen], &gc, &rect);
}

static
void
DrawText(ScreenContext *sc_,
         int32          x_,
         int32          y_,
         const char    *text_,
         Color          color_)
{
  GrafCon gc;

  SetFGPen(&gc, color_);
  MoveTo(&gc, x_, y_);
  DrawText8(&gc, sc_->sc_BitmapItems[sc_->sc_curScreen], (uint8 *)text_);
}

static
void
WaitForExit(void)
{
  Err err;
  uint32 buttons;

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

static
int32
WriteExampleFile(const char *path_,
                 const char *text_)
{
  FILE *file;
  int32 wrote;
  int32 textSize;

  file = fopen(path_, "w");
  if(file == NULL)
    return -1;

  textSize = (int32)strlen(text_);
  wrote = fwrite(text_, 1, textSize, file);
  if(wrote != textSize)
    {
      fclose(file);
      return -1;
    }

  if(fflush(file) < 0)
    {
      fclose(file);
      return -1;
    }

  if(fclose(file) < 0)
    return -1;

  return 0;
}

static
int32
ReadExampleFile(const char *path_,
                char       *buffer_,
                int32       bufferSize_)
{
  FILE *file;
  int32 got;

  file = fopen(path_, "r");
  if(file == NULL)
    return -1;

  got = fread(buffer_, 1, bufferSize_ - 1, file);
  if(got < 0)
    got = 0;
  buffer_[got] = 0;

  if(fclose(file) < 0)
    return -1;

  return got;
}

int
main(int    argc_,
     char **argv_)
{
  ScreenContext sc;
  Err err;
  char buffer[96];
  char expected[RANDOM_TEXT_MAX + 1];
  char status[96];
  int32 bytesRead;
  int32 titleX;
  int32 exitX;
  const char title[] = "FILE API";
  const char exitText[] = "Press X to return";

  (void)argc_;
  (void)argv_;

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
  ClearScreen(&sc);

  titleX = (SCREEN_WIDTH - ((sizeof(title) - 1) * GLYPH_WIDTH)) / 2;
  exitX = (SCREEN_WIDTH - ((sizeof(exitText) - 1) * GLYPH_WIDTH)) / 2;

  DrawText(&sc, titleX, 36, title, MakeRGB15(31, 31, 31));

  remove(FILE_PATH);

  GenerateRandomString(expected, sizeof(expected));

  err = WriteExampleFile(FILE_PATH, expected);
  if(err < 0)
    {
      DrawText(&sc, 32, 64 + ROW_HEIGHT, "write failed", MakeRGB15(31, 8, 8));
      printf("failed to write %s\n", FILE_PATH);
    }
  else
    {
      DrawText(&sc, 32, 64 + ROW_HEIGHT, "WRITE: wrote file",
               MakeRGB15(18, 31, 18));
      sprintf(status, "WRITE data: %s", expected);
      DrawText(&sc, 32, 64 + (ROW_HEIGHT * 2), status,
               MakeRGB15(31, 31, 31));
      printf("WRITE: wrote %s\n", FILE_PATH);
      printf("WRITE data: %s\n", expected);

      bytesRead = ReadExampleFile(FILE_PATH, buffer, sizeof(buffer));
      if(bytesRead < 0)
        {
          DrawText(&sc, 32, 64 + (ROW_HEIGHT * 3), "READ: failed",
                   MakeRGB15(31, 8, 8));
          printf("failed to read %s\n", FILE_PATH);
        }
      else
        {
          sprintf(status, "READ: read %ld bytes", bytesRead);
          DrawText(&sc, 32, 64 + (ROW_HEIGHT * 3), status,
                   MakeRGB15(18, 31, 18));
          sprintf(status, "READ data: %s", buffer);
          DrawText(&sc, 32, 64 + (ROW_HEIGHT * 4), status,
                   MakeRGB15(31, 31, 31));
          printf("READ: read %ld bytes from %s\n", bytesRead, FILE_PATH);
          printf("READ data: %s\n", buffer);

          if(CheckReadback(buffer, bytesRead, expected) < 0)
            {
              DrawText(&sc, 32, 64 + (ROW_HEIGHT * 5), "FAIL: data mismatch",
                       MakeRGB15(31, 8, 8));
              printf("%s: readback mismatch\nexpected: %s\nactual: %s\n",
                     FILE_PATH, expected, buffer);
            }
          else
            {
              DrawText(&sc, 32, 64 + (ROW_HEIGHT * 5), "PASS: data matched",
                       MakeRGB15(18, 31, 18));
              printf("PASS: data matched\n");
            }
        }
    }

  remove(FILE_PATH);

  DrawText(&sc, exitX, SCREEN_HEIGHT - 32, exitText, MakeRGB15(18, 18, 18));
  DisplayScreen(sc.sc_Screens[sc.sc_curScreen], 0);

  WaitForExit();

  DeleteBasicDisplay(&sc);
  KillControlPad();
  return 0;
}
