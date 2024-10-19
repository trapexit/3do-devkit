/*
        File:		AccessLib.h

        Contains:	Wrapper routines to use access manager routines.

        Written by:	Jay London

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 8/12/93	JML		First Writing.

        To Do:
*/

#define BUF_SIZE                                                              \
  256 // Used in GetLoadFileName and GetSaveFileName - user must provide a 256
      // character block

// dialog styles
#define OK_DIALOG 0
#define OK_CANCEL_DIALOG 1
#define ONE_BUTTON_DIALOG 2
#define TWO_BUTTON_DIALOG 3

// color defines
#define COLOR_SAME -1
#define COLOR_DEFAULT -2

// access library return codes
#define ACCESS_LIB_OUT_OF_MEM -5001
#define ACCESS_LIB_BAD_FILESIZE -5002
#define ACCESS_LIB_BAD_MSGITEM -5003
#define ACCESS_LIB_BAD_ACCESSPORT -5004
#define ACCESS_LIB_BAD_SCREENITEM -5005

int32 Answer (int32 dialogType, Item screenItem, char *title, char *text, ...);
int32 StdLoadFile (char *title, char **buf, Item screenItem);
int32 StdSaveFile (char *title, char *buf, int32 bufSize, Item screenItem,
                   char *defaultFileName);
int32 StdDeleteFile (char *title, Item screenItem);
int32 SetAccessColors (RGB555 foreColor, RGB555 backColor, RGB555 hiliteColor,
                       RGB555 shadowColor);
