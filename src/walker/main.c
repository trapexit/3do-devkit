
/******************************************************************************
**
**  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: walker.c,v 1.14 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/walker
|||	walker - Recursively displays the contents of a directory, and all
nested
|||	    directories.
|||
|||	  Synopsis
|||
|||	    walker \<dir1> [dir2] [dir3]...
|||
|||	  Description
|||
|||	    This program demonstrates how to scan a directory, and recursively
scan
|||	    any directories it contains.
|||
|||	  Arguments
|||
|||	    dir1                         Name of the directory to list. You can
|||	                                 specify an arbitrary number of
directory
|||	                                 names, and they will all get listed.
If no
|||	                                 name is specified, then the current
|||	                                 directory in which the ls program is
located
|||	                                 will get displayed.
|||
|||	  Associated Files
|||
|||	    walker.c
|||
|||	  Location
|||
|||	    $c/Walker
|||
|||	    examples/FileSystem
|||
**/

#include "directory.h"
#include "directoryfunctions.h"
#include "displayutils.h"
#include "event.h"
#include "filefunctions.h"
#include "filesystem.h"
#include "graphics.h"
#include "io.h"
#include "operror.h"
#include "stdarg.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

/*****************************************************************************/

#define DISPLAY_TYPE DI_TYPE_NTSC
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define TEXT_X 8
#define TEXT_Y 8
#define TEXT_LINE_HEIGHT 11
#define TEXT_COLUMNS 39
#define TEXT_ROWS 20

typedef struct Terminal
{
  ScreenContext screen;
  char          lines[TEXT_ROWS][TEXT_COLUMNS + 1];
  int32         count;
} Terminal;

static Terminal gTerminal;

static
void
ClearScreen(ScreenContext *sc)
{
  GrafCon gc;
  Rect    r;

  r.rect_XLeft = 0;
  r.rect_YTop = 0;
  r.rect_XRight = SCREEN_WIDTH;
  r.rect_YBottom = SCREEN_HEIGHT;
  SetFGPen(&gc, 0);
  FillRect(sc->sc_BitmapItems[sc->sc_curScreen], &gc, &r);
}

static
void
DrawLine(ScreenContext *sc,
         int32          row,
         const char    *text,
         Color          color)
{
  GrafCon gc;

  SetFGPen(&gc, color);
  MoveTo(&gc, TEXT_X, TEXT_Y + row * TEXT_LINE_HEIGHT);
  DrawText8(&gc, sc->sc_BitmapItems[sc->sc_curScreen], (ubyte *)text);
}

static
void
TerminalDraw(Terminal *term)
{
  int32 i;

  ClearScreen(&term->screen);
  for(i = 0; i < term->count; i++)
    DrawLine(&term->screen, i, term->lines[i], MakeRGB15(24, 24, 24));

  DisplayScreen(term->screen.sc_Screens[term->screen.sc_curScreen], 0);
  term->screen.sc_curScreen = 1 - term->screen.sc_curScreen;
}

static
void
TerminalAddLine(Terminal  *term,
                const char *text)
{
  int32 i;

  if(term->count >= TEXT_ROWS)
    {
      for(i = 1; i < TEXT_ROWS; i++)
        strcpy(term->lines[i - 1], term->lines[i]);
      term->count = TEXT_ROWS - 1;
    }

  strncpy(term->lines[term->count], text, TEXT_COLUMNS);
  term->lines[term->count][TEXT_COLUMNS] = 0;
  term->count++;
  TerminalDraw(term);
  printf("%s\n", text);
}

static
void
TerminalPrintf(Terminal  *term,
               const char *format,
               ...)
{
  va_list args;
  char    line[128];

  va_start(args, format);
  vsprintf(line, format, args);
  va_end(args);
  TerminalAddLine(term, line);
}

static
void
WaitForExit(void)
{
  ControlPadEventData cped;

  do
    {
      GetControlPad(1, TRUE, &cped);
    }
  while(!(cped.cped_ButtonBits & ControlX));
}

static
int32
ExitPressed(void)
{
  ControlPadEventData cped;

  GetControlPad(1, FALSE, &cped);
  return (cped.cped_ButtonBits & ControlX) != 0;
}

/*****************************************************************************/

static int32
TraverseDirectory(Item      dirItem,
                  Terminal *term,
                  int32     depth)
{
  Directory *dir;
  DirectoryEntry de;
  Item subDirItem;
  char indent[32];
  char name[96];
  int32 i;

  for(i = 0; i < depth * 2 && i < (int32)(sizeof(indent) - 1); i++)
    indent[i] = ' ';
  indent[i] = 0;

  dir = OpenDirectoryItem(dirItem);
  if(dir)
    {
      while(ReadDirectory(dir, &de) >= 0)
        {
          if(ExitPressed())
            {
              CloseDirectory(dir);
              return 1;
            }

          strcpy(name, de.de_FileName);

          if(de.de_Flags & FILE_IS_DIRECTORY)
            {
              strcat(name, "/");
              TerminalPrintf(term, "%s%-24s %5ld", indent, name,
                             de.de_ByteCount);

              subDirItem = OpenDiskFileInDir(dirItem, de.de_FileName);
              if(subDirItem >= 0)
                {
                  if(TraverseDirectory(subDirItem, term, depth + 1))
                    {
                      CloseDiskFile(subDirItem);
                      CloseDirectory(dir);
                      return 1;
                    }
                  CloseDiskFile(subDirItem);
                }
              else
                {
                  TerminalPrintf(term, "%s  open failed: 0x%lx", indent,
                                 subDirItem);
                }
            }
          else
            {
              TerminalPrintf(term, "%s%-24s %5ld", indent, name,
                             de.de_ByteCount);
            }
        }
      CloseDirectory(dir);
    }
  else
    {
      TerminalAddLine(term, "OpenDirectoryItem() failed");
    }

  return 0;
}

/*****************************************************************************/

static Err
Walk(const char *path,
     Terminal   *term)
{
  Item startItem;
  int32 interrupted;

  startItem = OpenDiskFile((char *)path);
  if(startItem >= 0)
    {
      TerminalPrintf(term, "%s:", path);
      interrupted = TraverseDirectory(startItem, term, 1);
      CloseDiskFile(startItem);
      if(interrupted)
        return 1;
      return 0;
    }
  else
    {
      TerminalPrintf(term, "OpenDiskFile(\"%s\") failed", path);
      PrintfSysErr(startItem);
      return startItem;
    }
}

/*****************************************************************************/

int
main(int32 argc, char **argv)
{
  Terminal *term;
  Err       result;
  int32     interrupted;
  int       i;

  term = &gTerminal;
  memset(term, 0, sizeof(*term));

  result = InitEventUtility(1, 0, LC_ISFOCUSED);
  if(result < 0)
    {
      printf("InitEventUtility failed: 0x%lx\n", result);
      return (int)result;
    }

  result = CreateBasicDisplay(&term->screen, DISPLAY_TYPE, 2);
  if(result < 0)
    {
      printf("CreateBasicDisplay failed: 0x%lx\n", result);
      KillEventUtility();
      return (int)result;
    }
  term->screen.sc_curScreen = 0;
  interrupted = 0;

  if(argc <= 1)
    {
      /* if no directory name was given, scan the boot filesystem */
      interrupted = (Walk("$boot", term) == 1);
    }
  else
    {
      for(i = 1; i < argc && !interrupted; i++)
        interrupted = (Walk(argv[i], term) == 1);
    }

  if(!interrupted)
    {
      TerminalAddLine(term, "");
      TerminalAddLine(term, "Press X to exit");
      WaitForExit();
    }

  DeleteBasicDisplay(&term->screen);
  KillEventUtility();
  return 0;
}
