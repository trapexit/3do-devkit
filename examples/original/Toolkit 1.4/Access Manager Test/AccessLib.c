/*
        File:		AccessLib.c

        Contains:	Wrapper routines to use access manager routines.

        Written by:	Jay London

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                                 5/09/94	crm		include
   Form3DO.h to declare RGB555 type fixed illegal casts from enum to void* in
   tag-arg assigments <1>	 8/12/93	JML		First Writing.

        To Do:
*/

#include "AccessLib.h"
#include "Form3DO.h"
#include "access.h"
#include "mem.h"
#include "stdio.h"
#include "string.h"
#include "types.h"
#include "varargs.h"

static Item accessItem = 0, msgItem = 0, portItem = 0, shellItem = 0;
static MsgPort *accessport = NULL;
static Msg *msg;
static TagArg targs[100];
static int32 screenSize;
static Screen *screen;
static RGB555 accessForeColor = (RGB555)COLOR_DEFAULT,
              accessBackColor = (RGB555)COLOR_DEFAULT,
              accessHiliteColor = (RGB555)COLOR_DEFAULT,
              accessShadowColor = (RGB555)COLOR_DEFAULT;

TagArg PortTags[2] = { TAG_ITEM_NAME, 0, TAG_END, 0 };

TagArg msgtags[2] = { CREATEMSG_TAG_REPLYPORT, 0, TAG_END, 0 };

/******************************************************
 *	SetAccessColors
 *		Sets the fore, back, hilite, and shadow color
 *		of access dialogs
 *		COLOR_SAME for any value indicates keep color as is
 *		COLOR_DEFAULT resets a color to its default color
 ******************************************************/
int32
SetAccessColors (RGB555 foreColor, RGB555 backColor, RGB555 hiliteColor,
                 RGB555 shadowColor)
{
  if (foreColor != (RGB555)COLOR_SAME)
    accessForeColor = foreColor;
  if (backColor != (RGB555)COLOR_SAME)
    accessBackColor = backColor;
  if (hiliteColor != (RGB555)COLOR_SAME)
    accessHiliteColor = hiliteColor;
  if (shadowColor != (RGB555)COLOR_SAME)
    accessShadowColor = shadowColor;
  return 0;
}

/******************************************************
 *	GetShellItem
 *		Looks up the screen item, sets up
 *			screenSize and screen static globals
 *		Finds the access port and task
 *		Finds the shell item
 *		Allows access to our screen memory
 ******************************************************/
Item
GetShellItem (int32 screenItem)
{
  int32 res;

  screen = (Screen *)LookupItem (screenItem);
  if (screen == NULL)
    {
      printf ("Error: LookupItem of screen failed.\n");
      return ACCESS_LIB_BAD_SCREENITEM;
    }

  if (shellItem <= 0)
    { // if shellItem is positive, then we already got it

      // locate access port and access task
      if ((accessItem
           = FindNamedItem (MKNODEID (KERNELNODE, MSGPORTNODE), "access"))
          < 0)
        {
          printf ("Error: can't find access port!\n");
          return accessItem;
        }

      // Get the item number of the shell itself
      accessport = (MsgPort *)LookupItem (accessItem);
      if (accessport == NULL)
        {
          printf ("Error: LookupItem of accessport failed.\n");
          return ACCESS_LIB_BAD_ACCESSPORT;
        }
      shellItem = accessport->mp.n_Owner;
      if (shellItem < 0)
        {
          printf ("Error: ShellItem is invalid!\n");
          return shellItem;
        }
    }

  screenSize = screen->scr_TempBitmap->bm_Width
               * screen->scr_TempBitmap->bm_Height * 2;
  res = ControlMem ((void *)screen->scr_TempBitmap->bm_Buffer, screenSize,
                    MEMC_OKWRITE, shellItem);
  if (res < 0)
    {
      printf (
          "Error: Allowing shell task access to our screen memory failed.\n");
      return res;
    }
  return shellItem;
}

/******************************************************
 *	AllocateMsgAndReply
 *		Allocates the reply port and msg item for
 *			sending and receiving messages to/from
 *			the shell task
 ******************************************************/
int32
AllocateMsgAndReply (void)
{

  PortTags[0].ta_Arg = (void *)"atestreply";
  portItem = CreateItem (MKNODEID (KERNELNODE, MSGPORTNODE), PortTags);
  if (portItem < 0)
    {
      printf ("Error: Creating ReplyPort.\n");
      return portItem;
    }
  msgtags[0].ta_Arg = (void *)portItem;
  msgItem = CreateItem (MKNODEID (KERNELNODE, MESSAGENODE), msgtags);
  if (msgItem < 0)
    {
      printf ("Error: Creating Message.\n");
      return msgItem;
    }
  msg = (Msg *)LookupItem (msgItem);
  if (msg == NULL)
    {
      printf ("Error: LookupItem of message failed.\n");
      return ACCESS_LIB_BAD_MSGITEM;
    }
  return 0;
}

/******************************************************
 *	AccessInit
 *		Looks up the screen item, sets up
 *			screenSize and screen static globals
 *		Finds the access port and task
 *		Finds the shell item
 *		Allows access to our screen memory
 *		Allocates the reply port and msg item for
 *			sending and receiving messages to/from
 *			the shell task
 ******************************************************/
int32
AccessInit (Item screenItem)
{
  int32 res;
  res = GetShellItem (screenItem);
  if (res < 0)
    return res;
  res = AllocateMsgAndReply ();
  return res;
}

/******************************************************
 *	DeallocateMsgAndReply
 *		Deallocates Reply and Message Items
 ******************************************************/
void
DeallocateMsgAndReply (void)
{
  DeleteItem (msgItem);
  DeleteItem (portItem);
}

/******************************************************
 *	AccessCleanup
 *		Removes write access to our screen memory
 *		Deallocates Reply and Message Items
 ******************************************************/
void
AccessCleanup (void)
{
  int32 res;

  res = ControlMem ((void *)screen->scr_TempBitmap->bm_Buffer, screenSize,
                    MEMC_NOWRITE, shellItem);
  if (res < 0)
    {
      printf (
          "Error: Removing shell task access to our screen memory failed.\n");
    }
  DeallocateMsgAndReply ();
}

/******************************************************
 *	SendMsgToAccess
 *		Sends the msg to the Access Mgr and wait for reply
 ******************************************************/
int32
SendMsgToAccess (void)
{
  SendMsg (accessItem, msgItem, (void *)targs, sizeof (targs));
  if (msgItem != WaitPort (portItem, 0))
    {
      printf ("Error in WaitPort\n");
      return ACCESS_LIB_BAD_MSGITEM;
    }
  msg = (Msg *)LookupItem (msgItem);
  if (msg == NULL)
    {
      printf ("Error: LookupItem of message failed.\n");
      return ACCESS_LIB_BAD_MSGITEM;
    }
  return (msg->msg_Result);
}

/******************************************************
 *	SetColorTags
 *		Sets up the tags for the fore, back, shadow,
 *		and hilite colors for the access manager dialogs.
 ******************************************************/
void
SetColorTags (int32 *tagnum)
{
  int32 tempTagNum = *tagnum;

  if (accessForeColor != (RGB555)COLOR_DEFAULT)
    {
      targs[tempTagNum].ta_Tag = ACCTAG_FG_PEN;
      targs[tempTagNum++].ta_Arg = (void *)accessForeColor;
    }
  if (accessBackColor != (RGB555)COLOR_DEFAULT)
    {
      targs[tempTagNum].ta_Tag = ACCTAG_BG_PEN;
      targs[tempTagNum++].ta_Arg = (void *)accessBackColor;
    }
  if (accessHiliteColor != (RGB555)COLOR_DEFAULT)
    {
      targs[tempTagNum].ta_Tag = ACCTAG_HILITE_PEN;
      targs[tempTagNum++].ta_Arg = (void *)accessHiliteColor;
    }
  if (accessShadowColor != (RGB555)COLOR_DEFAULT)
    {
      targs[tempTagNum].ta_Tag = ACCTAG_SHADOW_PEN;
      targs[tempTagNum++].ta_Arg = (void *)accessShadowColor;
    }
  *tagnum = tempTagNum;
}

/******************************************************
 *	GetLoadFileName
 *		Allows the user to select a file name from which
 *			to load the game.  This only gets the filename,
 *			it does not load the game.
 ******************************************************/
int32
GetLoadFileName (char *title, char *fName, Item screenItem)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  res = ControlMem ((void *)fName, 256, MEMC_OKWRITE, shellItem);
  if (res < 0)
    {
      printf ("ControlMem on fileName string failed.\n");
      return res;
    }

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_LOAD);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF;
  targs[tagnum++].ta_Arg = (void *)fName;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF_SIZE;
  targs[tagnum++].ta_Arg = (void *)BUF_SIZE;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  AccessCleanup ();
  return res;
}

/******************************************************
 *	GetDeleteFileName
 *		Allows the user to select a file name which will
 *		be deleted.
 ******************************************************/
int32
GetDeleteFileName (char *title, char *fName, Item screenItem)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  res = ControlMem ((void *)fName, 256, MEMC_OKWRITE, shellItem);
  if (res < 0)
    {
      printf ("ControlMem on fileName string failed.\n");
      return res;
    }

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_DELETE);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF;
  targs[tagnum++].ta_Arg = (void *)fName;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF_SIZE;
  targs[tagnum++].ta_Arg = (void *)BUF_SIZE;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  AccessCleanup ();
  return res;
}

/******************************************************
 *	StdReadFile
 *		Read a file from NVRAM or a memory card into buf
 ******************************************************/
int32
StdReadFile (char *fName, char **buf)
{
  int32 err, fileSize;
  Item fd, req;
  IOReq *reqp;
  IOInfo params;
  DeviceStatus ds;

  fd = OpenDiskFile (fName);
  if (fd < 0)
    {
      PrintfSysErr (fd);
      return fd;
    }

  /* get blockSize */
  req = CreateIOReq (NULL, 0, fd, 0);
  reqp = (IOReq *)LookupItem (req);
  memset (&params, 0, sizeof (IOInfo));
  memset (&ds, 0, sizeof (DeviceStatus));
  params.ioi_Command = CMD_STATUS;
  params.ioi_Recv.iob_Buffer = &ds;
  params.ioi_Recv.iob_Len = sizeof (DeviceStatus);
  err = DoIO (req, &params);
  if (err < 0 || reqp->io_Error)
    {
      //		CloseItem(fd);
      CloseDiskFile (fd);
      return err;
    }

  fileSize = ds.ds_DeviceBlockCount * ds.ds_DeviceBlockSize;
  if (fileSize <= 0) // file is empty or  bad
    return ACCESS_LIB_BAD_FILESIZE;
  if (*buf == NULL) // allocate the buffer
    *buf = (char *)ALLOCMEM (fileSize, MEMTYPE_ANY);
  if (*buf == NULL) // couldn't allocate buffer
    return ACCESS_LIB_OUT_OF_MEM;

  /* try and read our buffer in */
  memset (&params, 0, sizeof (IOInfo));
  params.ioi_Command = CMD_READ;
  params.ioi_Recv.iob_Len = fileSize;
  params.ioi_Recv.iob_Buffer = *buf;
  err = DoIO (req, &params);
  if (err < 0 || reqp->io_Error)
    {
      //		CloseItem(fd);
      CloseDiskFile (fd);
      return err;
    }
  CloseDiskFile (fd);
  //	CloseItem(fd);

  return 0;
}

/******************************************************
 *	EasyDeleteFile
 *		Deletes a file from NVRAM or a memory card
 ******************************************************/
int32
EasyDeleteFile (char *fName)
{
  int32 err;

  err = DeleteFile (fName);
  if (err < 0)
    {
      PrintfSysErr (err);
      return err;
    }

  return 0;
}

/******************************************************
 *	StdDeleteFile
 *		Allows the user to select a file name of a file
 *			to be deleted
 ******************************************************/
int32
StdDeleteFile (char *title, Item screenItem)
{
  int32 result;
  char fName[256];

  result = GetDeleteFileName (title, fName, screenItem);
  if ((result < 0) || (fName[0] == 0))
    return result;
  return (EasyDeleteFile (fName));
}

/******************************************************
 *	StdLoadFile
 *		Allows the user to select a file name from which
 *			to load the game and then reads the file into
 *			the buffer, buf, passed in.  If *buf is NULL,
 *			LoadFile will allocate the buffer.
 ******************************************************/
int32
StdLoadFile (char *title, char **buf, Item screenItem)
{
  int32 result;
  char fName[256];

  result = GetLoadFileName (title, fName, screenItem);
  if ((result < 0) || (fName[0] == 0))
    return result;
  return (StdReadFile (fName, buf));
}

/******************************************************
 *	GetSaveFileName
 *		Allows the user to enter a file name in which
 *			to save the game.  This only gets the filename,
 *			it does not save the game.
 ******************************************************/
int32
GetSaveFileName (char *title, char *fName, Item screenItem)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  res = ControlMem ((void *)fName, 256, MEMC_OKWRITE, shellItem);
  if (res < 0)
    {
      printf ("ControlMem on fileName string failed.\n");
      return res;
    }
  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_SAVE);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF;
  targs[tagnum++].ta_Arg = (void *)fName;
  targs[tagnum].ta_Tag = ACCTAG_STRINGBUF_SIZE;
  targs[tagnum++].ta_Arg = (void *)BUF_SIZE;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  AccessCleanup ();
  return res;
}

/******************************************************
 *	StdWriteFile
 *		Writes out  a file to NVRAM or a memory card
 ******************************************************/
int32
StdWriteFile (char *fName, char *buf, int32 bufSize)
{
  int32 err;
  Item fd, req;
  IOReq *reqp;
  IOInfo params;
  DeviceStatus ds;

  fd = OpenDiskFile (fName);
  if (fd < 0)
    {
      fd = CreateFile (fName);
      if (fd < 0)
        {
          PrintfSysErr (fd);
          return fd;
        }
      fd = OpenDiskFile (fName);
      if (fd < 0)
        {
          PrintfSysErr (fd);
          return fd;
        }
    }

  /* get blockSize, not garranteed to have figured this out before!!! */
  req = CreateIOReq (NULL, 0, fd, 0);
  reqp = (IOReq *)LookupItem (req);
  memset (&params, 0, sizeof (IOInfo));
  memset (&ds, 0, sizeof (DeviceStatus));
  params.ioi_Command = CMD_STATUS;
  params.ioi_Recv.iob_Buffer = &ds;
  params.ioi_Recv.iob_Len = sizeof (DeviceStatus);
  err = DoIO (req, &params);
  if (err < 0 || reqp->io_Error)
    {
      /* CloseItem(fd);		 XXXX maybe this should be DeleteItem() */
      CloseDiskFile (fd);
      return 0;
    }

  /* make sure file is right size */
  if (ds.ds_DeviceBlockSize * ds.ds_DeviceBlockCount < bufSize)
    {
      int32 currentSize;

      currentSize = ds.ds_DeviceBlockSize * ds.ds_DeviceBlockCount;
      memset (&params, 0, sizeof (IOInfo));
      params.ioi_Command = FILECMD_ALLOCBLOCKS;
      params.ioi_Offset = (bufSize - currentSize + ds.ds_DeviceBlockSize - 1)
                          / ds.ds_DeviceBlockSize;
      err = DoIO (req, &params);
      if (err < 0 || reqp->io_Error)
        {
          //	CloseItem(fd);
          CloseDiskFile (fd);
          return 0;
        }
    }

  /* try and write our buffer out */
  memset (&params, 0, sizeof (IOInfo));
  params.ioi_Command = CMD_WRITE;
  params.ioi_Send.iob_Buffer = buf;
  params.ioi_Send.iob_Len = bufSize;
  err = DoIO (req, &params);
  if (err < 0 || reqp->io_Error)
    {
      // CloseItem(fd);		/* XXXX maybe this should be DeleteItem() */
      CloseDiskFile (fd);
      return err;
    }
  // CloseItem(fd);		/* XXXX maybe this should be DeleteItem() */
  CloseDiskFile (fd);
  return 0;
}

/******************************************************
 *	StdSaveFile
 *		Allows the user to enter a file name in which
 *			to save the game and then writes the buffer, buf,
 *			of length buffSize to the file.
 ******************************************************/
int32
StdSaveFile (char *title, char *buf, int32 bufSize, Item screenItem,
             char *fileName)
{
  char fName[256];
  int32 result;

  if (fileName == NULL)
    fName[0] = 0;
  else
    strcpy (fName, fileName);
  result = GetSaveFileName (title, fName, screenItem);
  if ((result < 0) || (result == 1))
    return result;
  if (fileName != NULL)
    strcpy (fileName, fName); // copy back so user can see what user typed
  return (StdWriteFile (fName, buf, bufSize));
}

/******************************************************
 *	OKDialog
 *		Puts up a dialog using the access mgr
 ******************************************************/
int32
OKDialog (char *title, char *text, Item screenItem)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_OK);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_TEXT;
  targs[tagnum++].ta_Arg = (void *)text;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  AccessCleanup ();
  return res;
}

/******************************************************
 *	OKCancelDialog
 *		Puts up a dialog using the access mgr
 *		Returns true in OK if the OK button
 *			is selected, otherwise OK is false
 ******************************************************/
int32
OKCancelDialog (char *title, char *text, Item screenItem, Boolean *OK)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_OKCANCEL);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_TEXT;
  targs[tagnum++].ta_Arg = (void *)text;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  if (res == 0)
    *OK = true;
  else if (res == 1)
    {
      res = 0;
      *OK = false;
    }
  AccessCleanup ();
  return res;
}

/******************************************************
 *	DialogOneButton
 *		Puts up a dialog using the access mgr
 ******************************************************/
int32
DialogOneButton (char *title, char *btnText, char *text, Item screenItem)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_ONEBUTTON);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_TEXT;
  targs[tagnum++].ta_Arg = (void *)text;
  targs[tagnum].ta_Tag = ACCTAG_BUTTON_ONE;
  targs[tagnum++].ta_Arg = (void *)btnText;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  AccessCleanup ();
  return res;
}

/******************************************************
 *	DialogTwoButton
 *		Puts up a dialog using the access mgr
 *		Returns true in firstButton if the first button
 *			is selected, otherwise firstButton is false
 ******************************************************/
int32
DialogTwoButton (char *title, char *btn1Text, char *btn2Text, char *text,
                 Item screenItem, Boolean *firstButton)
{
  int32 res, tagnum = 0;

  res = AccessInit (screenItem);
  if (res < 0)
    return res;

  // set up tag args for the call to access
  targs[tagnum].ta_Tag = ACCTAG_REQUEST;
  targs[tagnum++].ta_Arg = (void *)((uint32)ACCREQ_TWOBUTTON);
  targs[tagnum].ta_Tag = ACCTAG_SCREEN;
  targs[tagnum++].ta_Arg = (void *)screenItem;
  targs[tagnum].ta_Tag = ACCTAG_TITLE;
  targs[tagnum++].ta_Arg = (void *)title;
  targs[tagnum].ta_Tag = ACCTAG_TEXT;
  targs[tagnum++].ta_Arg = (void *)text;
  targs[tagnum].ta_Tag = ACCTAG_BUTTON_ONE;
  targs[tagnum++].ta_Arg = (void *)btn1Text;
  targs[tagnum].ta_Tag = ACCTAG_BUTTON_TWO;
  targs[tagnum++].ta_Arg = (void *)btn2Text;
  SetColorTags (&tagnum);
  targs[tagnum].ta_Tag = TAG_END;

  res = SendMsgToAccess ();
  if (res == 0)
    *firstButton = true;
  else if (res == 1)
    {
      res = 0;
      *firstButton = false;
    }
  AccessCleanup ();
  return res;
}

/******************************************************
 *	Answer
 *		Common routine to bring up all the Access Mgr Dialogs
 ******************************************************/
int32
Answer (int32 dialogStyle, Item screenItem, char *title, char *text, ...)
{
  char *btn1Text;
  char *btn2Text;
  Boolean firstButton;
  va_list ap;
  int32 result = 0;

  va_start (ap, text);

  switch (dialogStyle)
    {
    case OK_DIALOG:
      result = OKDialog (title, text, screenItem);
      break;
    case OK_CANCEL_DIALOG:
      result = OKCancelDialog (title, text, screenItem, &firstButton);
      if (result != 0)
        return result;
      else if (firstButton)
        return 0;
      else
        return 1;
      break;
    case ONE_BUTTON_DIALOG:
      btn1Text = va_arg (ap, char *);
      result = DialogOneButton (title, btn1Text, text, screenItem);
      break;
    case TWO_BUTTON_DIALOG:
      btn1Text = va_arg (ap, char *);
      btn2Text = va_arg (ap, char *);
      result = DialogTwoButton (title, btn1Text, btn2Text, text, screenItem,
                                &firstButton);
      if (result != 0)
        return result;
      else if (firstButton)
        return 0;
      else
        return 1;
      break;
    }
  va_end (ap);
  return result;
}
