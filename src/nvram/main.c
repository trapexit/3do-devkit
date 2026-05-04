
/******************************************************************************
**
**  Copyright(C) 1995, an unpublished work by The 3DO Company. All rights
*reserved.
**  This material contains confidential information that is the property of The
*3DO Company.
**  Any unauthorized duplication, disclosure or use is prohibited.
**  $Id: nvram.c,v 1.3 1995/01/16 19:48:35 vertex Exp $
**
******************************************************************************/

/**
|||	AUTODOC PUBLIC examples/nvram
|||	nvram - Demonstrate how to save files to nvram and other writable
devices.
|||
|||	  Synopsis
|||
|||	    nvram
|||
|||	  Description
|||
|||	    Demonstrates how to save data to a file in NVRAM or to any other
writable
|||	    device.
|||
|||	    The SaveDataFile() function takes a name, a data pointer, and a
size
|||	    indicator, and creates a file of that name and size, containing the
|||	    supplied data.
|||
|||	    Writing data to a file requires a bit of finesse. Data can only be
written
|||	    in blocks, not bytes. If the data being written is not a multiple
of the
|||	    target device's blocksize, the last chunk of data must be copied to
a
|||	    buffer which is the size of a block, and the block can then be
written.
|||
|||	  Associated Files
|||
|||	    nvram.c
|||
|||	  Location
|||
|||	    examples/Nvram
|||
**/

#include "directory.h"
#include "directoryfunctions.h"
#include "displayutils.h"
#include "event.h"
#include "filefunctions.h"
#include "filesystem.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "mem.h"
#include "operror.h"
#include "stdio.h"
#include "string.h"
#include "types.h"

/*****************************************************************************/

Err
SaveDataFile(const char *name,
             void       *data,
             uint32      dataSize)
{
  Item       fileItem;
  Item       ioReqItem;
  IOInfo     ioInfo;
  void      *temp;
  Err        result;
  uint32     numBlocks;
  uint32     blockSize;
  uint32     roundedSize;
  FileStatus status;

  // get rid of the file if it was already there
  DeleteFile((char *)name);

  result = CreateFile((char *)name);
  if(result < 0)
    return result;

  fileItem = OpenDiskFile((char *)name);
  if(fileItem < 0)
    {
      DeleteFile((char *)name);
      return fileItem;
    }

  ioReqItem = CreateIOReq(NULL, 0, fileItem, 0);
  if(ioReqItem < 0)
    {
      CloseDiskFile(fileItem);
      DeleteFile((char *)name);
      return ioReqItem;
    }

  // get the block size of the file
  memset(&ioInfo, 0, sizeof(IOInfo));
  ioInfo.ioi_Command = CMD_STATUS;
  ioInfo.ioi_Recv.iob_Buffer = &status;
  ioInfo.ioi_Recv.iob_Len = sizeof(FileStatus);
  result = DoIO(ioReqItem, &ioInfo);
  if(result < 0)
    {
      DeleteIOReq(ioReqItem);
      CloseDiskFile(fileItem);
      DeleteFile((char *)name);
      return result;
    }

  blockSize = status.fs.ds_DeviceBlockSize;
  numBlocks = (dataSize + blockSize - 1) / blockSize;

  // allocate the blocks we need for this file
  ioInfo.ioi_Command = FILECMD_ALLOCBLOCKS;
  ioInfo.ioi_Recv.iob_Buffer = NULL;
  ioInfo.ioi_Recv.iob_Len = 0;
  ioInfo.ioi_Offset = numBlocks;
  result = DoIO(ioReqItem, &ioInfo);
  if(result < 0)
    {
      DeleteIOReq(ioReqItem);
      CloseDiskFile(fileItem);
      DeleteFile((char *)name);
      return result;
    }

  // tell the system how many bytes for this file
  memset(&ioInfo, 0, sizeof(IOInfo));
  ioInfo.ioi_Command = FILECMD_SETEOF;
  ioInfo.ioi_Offset = dataSize;
  result = DoIO(ioReqItem, &ioInfo);
  if(result < 0)
    {
      DeleteIOReq(ioReqItem);
      CloseDiskFile(fileItem);
      DeleteFile((char *)name);
      return result;
    }

  roundedSize = 0;
  if(dataSize >= blockSize)
    {
      // If we have more than one block's worth of data, write as much of it as
      // possible.
      roundedSize = (dataSize / blockSize) * blockSize;
      ioInfo.ioi_Command = CMD_WRITE;
      ioInfo.ioi_Send.iob_Buffer = (void *)data;
      ioInfo.ioi_Send.iob_Len = roundedSize;
      ioInfo.ioi_Offset = 0;
      result = DoIO(ioReqItem, &ioInfo);
      if(result < 0)
        {
          DeleteIOReq(ioReqItem);
          CloseDiskFile(fileItem);
          DeleteFile((char *)name);
          return result;
        }

      data = (void *)((uint32)data + roundedSize);
      dataSize -= roundedSize;
    }

  if(dataSize)
    {
      // If the amount of data left isn't as large as a whole block, we must
      // allocate a memory buffer of the size of the block, copy the rest of the
      // data into it, and write the buffer to disk.
      temp = AllocMem(blockSize, MEMTYPE_DMA | MEMTYPE_FILL);
      if(!temp)
        {
          DeleteIOReq(ioReqItem);
          CloseDiskFile(fileItem);
          DeleteFile((char *)name);
          return NOMEM;
        }

      memcpy(temp, data, dataSize);
      ioInfo.ioi_Command = CMD_WRITE;
      ioInfo.ioi_Send.iob_Buffer = temp;
      ioInfo.ioi_Send.iob_Len = blockSize;
      ioInfo.ioi_Offset = roundedSize;
      result = DoIO(ioReqItem, &ioInfo);
      FreeMem(temp, blockSize);
      if(result < 0)
        {
          DeleteIOReq(ioReqItem);
          CloseDiskFile(fileItem);
          DeleteFile((char *)name);
          return result;
        }
    }

  DeleteIOReq(ioReqItem);
  CloseDiskFile(fileItem);
  return result;
}

/*****************************************************************************/

#define DISPLAY_TYPE DI_TYPE_NTSC
#define FILE_DATA_SIZE 512
#define MAX_FILES 64
#define VISIBLE_FILES 12
#define VISIBLE_DUMP_LINES 13
#define DUMP_BYTES_PER_LINE 8

typedef struct NVRAMFile
{
  char   name[FILESYSTEM_MAX_NAME_LEN];
  char   path[FILESYSTEM_MAX_NAME_LEN + 8];
  uint32 byteCount;
} NVRAMFile;

typedef struct AppState
{
  ScreenContext screen;
  NVRAMFile     files[MAX_FILES];
  int32         fileCount;
  int32         selected;
  int32         top;
  int32         dumpTop;
  uint8         fileData[FILE_DATA_SIZE];
  uint32        fileDataSize;
  char          status[96];
} AppState;

static AppState appState;

static
void
WaitForRelease(void);

static
void
ClearScreen(ScreenContext *sc)
{
  GrafCon gc;
  Rect r;

  r.rect_XLeft = 0;
  r.rect_YTop = 0;
  r.rect_XRight = 320;
  r.rect_YBottom = 240;
  SetFGPen(&gc, 0);
  FillRect(sc->sc_BitmapItems[sc->sc_curScreen], &gc, &r);
}

static
void
DrawText(ScreenContext *sc,
         int32          x,
         int32          y,
         const char    *text,
         Color          color)
{
  GrafCon gc;

  SetFGPen(&gc, color);
  MoveTo(&gc, x, y);
  DrawText8(&gc, sc->sc_BitmapItems[sc->sc_curScreen], (ubyte *)text);
}

static
void
Present(ScreenContext *sc)
{
  DisplayScreen(sc->sc_Screens[sc->sc_curScreen], 0);
  sc->sc_curScreen = 1 - sc->sc_curScreen;
}

static
void
DrawHeader(AppState *app, const char *title)
{
  DrawText(&app->screen, 16, 18, title, MakeRGB15(31, 31, 31));
  if(app->status[0])
    DrawText(&app->screen, 16, 210, app->status, MakeRGB15(31, 24, 0));
}

static
void
FillRandomData(uint8  *data,
               uint32  size)
{
  uint32 i;
  uint32 r = 0;

  for(i = 0; i < size; i++)
    {
      if((i & 3) == 0)
        r = ReadHardwareRandomNumber();
      data[i] = (uint8)(r >> ((i & 3) * 8));
    }
}

static
Err
SaveRandomFile(AppState *app)
{
  uint8 data[FILE_DATA_SIZE];
  char name[32];
  Err err;

  sprintf(name, "/NVRAM/r%08lx", ReadHardwareRandomNumber());
  FillRandomData(data, sizeof(data));

  err = SaveDataFile(name, data, sizeof(data));
  if(err < 0)
    {
      sprintf(app->status, "Save failed: 0x%lx", err);
      PrintfSysErr(err);
    }
  else
    {
      sprintf(app->status, "Saved %s", name);
    }
  return err;
}

static
Err
RefreshFileList(AppState *app)
{
  Directory      *dir;
  DirectoryEntry de;
  int32          result;
  int32          oldSelected;

  oldSelected = app->selected;
  app->fileCount = 0;

  dir = OpenDirectoryPath("/NVRAM");
  if(!dir)
    {
      strcpy(app->status, "Could not open /NVRAM");
      return -1;
    }

  while((result = ReadDirectory(dir, &de)) >= 0)
    {
      if(app->fileCount < MAX_FILES)
        {
          strncpy(app->files[app->fileCount].name, de.de_FileName,
                  FILESYSTEM_MAX_NAME_LEN - 1);
          app->files[app->fileCount].name[FILESYSTEM_MAX_NAME_LEN - 1] = 0;
          sprintf(app->files[app->fileCount].path, "/NVRAM/%s",
                  app->files[app->fileCount].name);
          app->files[app->fileCount].byteCount = de.de_ByteCount;
          app->fileCount++;
        }
    }

  CloseDirectory(dir);
  if(oldSelected >= app->fileCount)
    oldSelected = app->fileCount - 1;
  if(oldSelected < 0)
    oldSelected = 0;
  app->selected = oldSelected;
  if(app->top > app->selected)
    app->top = app->selected;
  if(app->selected >= app->top + VISIBLE_FILES)
    app->top = app->selected - VISIBLE_FILES + 1;
  if(app->top < 0)
    app->top = 0;

  if(app->fileCount == 0)
    strcpy(app->status, "No files in /NVRAM");
  else
    sprintf(app->status, "%ld files in /NVRAM", app->fileCount);

  return 0;
}

static
Err
LoadDataFile(const char *name,
             uint8      *data,
             uint32      dataCapacity,
             uint32     *dataSize)
{
  Item       fileItem;
  Item       ioReqItem;
  IOInfo     ioInfo;
  FileStatus status;
  uint32     readSize;
  Err        result;

  *dataSize = 0;
  fileItem = OpenDiskFile((char *)name);
  if(fileItem < 0)
    return fileItem;

  ioReqItem = CreateIOReq(NULL, 0, fileItem, 0);
  if(ioReqItem < 0)
    {
      CloseDiskFile(fileItem);
      return ioReqItem;
    }

  memset(&ioInfo, 0, sizeof(IOInfo));
  ioInfo.ioi_Command = CMD_STATUS;
  ioInfo.ioi_Recv.iob_Buffer = &status;
  ioInfo.ioi_Recv.iob_Len = sizeof(FileStatus);
  result = DoIO(ioReqItem, &ioInfo);
  if(result >= 0)
    {
      readSize = status.fs_ByteCount;
      if(readSize > dataCapacity)
        readSize = dataCapacity;

      memset(&ioInfo, 0, sizeof(IOInfo));
      ioInfo.ioi_Command = CMD_READ;
      ioInfo.ioi_Recv.iob_Buffer = data;
      ioInfo.ioi_Recv.iob_Len = readSize;
      ioInfo.ioi_Offset = 0;
      result = DoIO(ioReqItem, &ioInfo);
      if(result >= 0)
        *dataSize = readSize;
    }

  DeleteIOReq(ioReqItem);
  CloseDiskFile(fileItem);
  return result;
}

static
char
HexDigit(uint32 v)
{
  v &= 15;
  return (char)(v < 10 ? ('0' + v) : ('A' + v - 10));
}

static
void
HexByte(char  *out,
        uint8  v)
{
  out[0] = HexDigit(v >> 4);
  out[1] = HexDigit(v);
}

static
void
MakeHexLine(char   *out,
            uint32  offset,
            uint8  *data,
            uint32  size)
{
  uint32 i;
  char  *p;

  sprintf(out, "%04lx:", offset);
  p = out + strlen(out);
  for(i = 0; i < DUMP_BYTES_PER_LINE; i++)
    {
      *p++ = ' ';
      if((offset + i) < size)
        {
          HexByte(p, data[offset + i]);
          p += 2;
        }
      else
        {
          *p++ = ' ';
          *p++ = ' ';
        }
    }
  *p = 0;
}

static
void
DrawFileListHelp(AppState *app)
{
  DrawText(&app->screen, 16, 196, "A view   B delete   C random   X quit",
           MakeRGB15(18, 18, 18));
}

static
void
DrawFileList(AppState *app)
{
  int32 i;
  int32 index;
  char line[96];

  ClearScreen(&app->screen);
  DrawHeader(app, "NVRAM Files");
  DrawFileListHelp(app);
  for(i = 0; i < VISIBLE_FILES; i++)
    {
      index = app->top + i;
      if(index >= app->fileCount)
        break;

      sprintf(line, "%c %-22s %5lu", (index == app->selected) ? '*' : ' ',
              app->files[index].name, app->files[index].byteCount);
      DrawText(&app->screen, 16, 35 + i * 13, line,
               (index == app->selected) ? MakeRGB15(31, 31, 0)
                                        : MakeRGB15(24, 24, 24));
    }
  Present(&app->screen);
}

static
int32
ConfirmDelete(AppState *app)
{
  ControlPadEventData cped;
  char line[96];

  while(1)
    {
      ClearScreen(&app->screen);
      DrawHeader(app, "Delete File");
      DrawText(&app->screen, 32, 82, "Delete this file?",
               MakeRGB15(31, 31, 31));
      sprintf(line, "%s", app->files[app->selected].name);
      DrawText(&app->screen, 32, 104, line, MakeRGB15(31, 24, 0));
      DrawText(&app->screen, 32, 146, "A delete   B cancel",
               MakeRGB15(24, 24, 24));
      Present(&app->screen);

      GetControlPad(1, TRUE, &cped);
      if(cped.cped_ButtonBits & ControlA)
        return 1;
      if(cped.cped_ButtonBits & (ControlB | ControlX))
        return 0;
      WaitForRelease();
    }
}

static
void
DeleteSelectedFile(AppState *app)
{
  Err  err;
  char deletedName[FILESYSTEM_MAX_NAME_LEN];

  if(app->fileCount <= 0)
    return;

  if(!ConfirmDelete(app))
    {
      strcpy(app->status, "Delete cancelled");
      return;
    }

  strcpy(deletedName, app->files[app->selected].name);
  err = DeleteFile(app->files[app->selected].path);
  if(err < 0)
    {
      sprintf(app->status, "Delete failed: 0x%lx", err);
      PrintfSysErr(err);
    }
  else
    {
      RefreshFileList(app);
      sprintf(app->status, "Deleted %s", deletedName);
    }
}

static
void
DrawDump(AppState *app)
{
  int32  i;
  uint32 offset;
  char   line[64];

  ClearScreen(&app->screen);
  DrawHeader(app, app->files[app->selected].name);
  DrawText(&app->screen, 16, 224, "Up/Down scroll   X back",
           MakeRGB15(18, 18, 18));
  for(i = 0; i < VISIBLE_DUMP_LINES; i++)
    {
      offset = (app->dumpTop + i) * DUMP_BYTES_PER_LINE;
      if(offset >= app->fileDataSize)
        break;
      MakeHexLine(line, offset, app->fileData, app->fileDataSize);
      DrawText(&app->screen, 16, 42 + i * 13, line, MakeRGB15(24, 24, 24));
    }
  Present(&app->screen);
}

static
void
ViewSelectedFile(AppState *app)
{
  ControlPadEventData cped;
  uint32              joy;
  Err                 err;

  if(app->fileCount <= 0)
    return;

  err = LoadDataFile(app->files[app->selected].path, app->fileData,
                     sizeof(app->fileData), &app->fileDataSize);
  if(err < 0)
    {
      sprintf(app->status, "Load failed: 0x%lx", err);
      return;
    }

  sprintf(app->status, "Loaded %lu bytes", app->fileDataSize);
  app->dumpTop = 0;
  while(1)
    {
      uint32 maxTop;

      DrawDump(app);
      GetControlPad(1, TRUE, &cped);
      joy = cped.cped_ButtonBits;
      maxTop = (app->fileDataSize + DUMP_BYTES_PER_LINE - 1)
               / DUMP_BYTES_PER_LINE;
      if(maxTop > VISIBLE_DUMP_LINES)
        maxTop -= VISIBLE_DUMP_LINES;
      else
        maxTop = 0;

      if(joy & ControlX)
        break;
      if((joy & ControlUp) && app->dumpTop > 0)
        app->dumpTop--;
      if((joy & ControlDown) && ((uint32)app->dumpTop < maxTop))
        app->dumpTop++;
      WaitForRelease();
    }
}

static
void
WaitForRelease(void)
{
  ControlPadEventData cped;

  do
    {
      GetControlPad(1, FALSE, &cped);
    }
  while(cped.cped_ButtonBits);
}

static
void
FileListLoop(AppState *app)
{
  ControlPadEventData cped;
  uint32              joy;

  if(RefreshFileList(app) < 0)
    return;

  while(1)
    {
      DrawFileList(app);
      GetControlPad(1, TRUE, &cped);
      joy = cped.cped_ButtonBits;

      if(joy & ControlX)
        return;
      if((joy & ControlB) && app->fileCount > 0)
        DeleteSelectedFile(app);
      if(joy & ControlC)
        {
          char savedStatus[96];

          SaveRandomFile(app);
          strcpy(savedStatus, app->status);
          RefreshFileList(app);
          strcpy(app->status, savedStatus);
        }
      if((joy & ControlUp) && app->selected > 0)
        {
          app->selected--;
          if(app->selected < app->top)
            app->top = app->selected;
        }
      if((joy & ControlDown) && app->selected < (app->fileCount - 1))
        {
          app->selected++;
          if(app->selected >= (app->top + VISIBLE_FILES))
            app->top = app->selected - VISIBLE_FILES + 1;
        }
      if((joy & ControlA) && app->fileCount > 0)
        ViewSelectedFile(app);
      WaitForRelease();
    }
}

int
main(int32 argc, char **argv)
{
  AppState *app;
  Err       result;

  (void)argc;
  (void)argv;

  app = &appState;
  memset(app, 0, sizeof(*app));

  result = InitEventUtility(1, 0, LC_ISFOCUSED);
  if(result < 0)
    {
      printf("InitEventUtility failed: 0x%lx\n", result);
      return (int)result;
    }

  result = CreateBasicDisplay(&app->screen, DISPLAY_TYPE, 2);
  if(result < 0)
    {
      printf("CreateBasicDisplay failed: 0x%lx\n", result);
      KillEventUtility();
      return (int)result;
    }

  app->screen.sc_curScreen = 0;
  strcpy(app->status, "Ready");

  FileListLoop(app);

  DeleteBasicDisplay(&app->screen);
  KillEventUtility();
  return 0;
}
