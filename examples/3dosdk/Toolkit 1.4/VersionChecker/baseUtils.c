
/*
        File:		baseUtils.c

        Contains:	Base utilities for folio call tests

        Written by:	Robert Chinn, Charlie Eckhaus

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <2>	 6/29/93	CDE		General fixup per
   Robert (new comments, deletion of unused code) Added gInvalidArg, command
   line argument validation
                 <1>	 6/22/93	CDE		Prepared for public
   consumption

        To Do:
*/

#include "baseUtils.h"

extern Boolean gUseLogFile;
extern FILE *gTheFile;
extern Boolean gInvalidArg;

long
power (long x, long y)
/* Return x to the yth power */
{
  long i, p;

  p = 1;
  for (i = 1; i <= y; ++i)
    p = p * x;

  return p;
}

void
printd (int32 n, FILE *theFile)
/* Used by pipedata to output decimal values to the log file */
{
  if (n / 10)
    printd (n / 10, theFile);
  putc ((char)n % 10 + '0', theFile);
}

void
printHex (int32 n, FILE *theFile)
/* Used by pipedata to output hex values to the log file */
{
  long indx;
  char *hexChar[] = { "0", "1", "2", "3", "4", "5", "6", "7",
                      "8", "9", "a", "b", "c", "d", "e", "f" };

  for (indx = 7; indx >= 0; indx--)
    {
      fputs (hexChar[((n) >> indx * 4) & 0xF], theFile);
    }
}

void
pipeData (char *formatStr, ...)
/* Used like printf to output to the log file if it's being used,
        or to standard output otherwise */
{
  va_list ap;
  char *p, *sval, schar;
  int32 ival;

  va_start (ap, formatStr);
  for (p = formatStr; *p; p++)
    {
      if (*p == 0x0A) // look explicitly for a newline hex 10 or '\n'
        {
          if (!gUseLogFile)
            kprintf ("\n");
          else
            putc (0x0D, gTheFile);

          continue;
        }
      if (*p != '%')
        {
          if (!gUseLogFile)
            kprintf ("%c", *p);
          else
            putc (*p, gTheFile);

          continue;
        }
      switch (*++p)
        {
        case 'd':
        case 'i':
        case 'u':
          ival = va_arg (ap, int32);
          if (!gUseLogFile)
            kprintf ("%i", ival);
          else
            printd (ival, gTheFile);
          break;

        case 'x':
          ival = va_arg (ap, int32);
          if (!gUseLogFile)
            kprintf ("%08lx", ival);
          else
            printHex (ival, gTheFile);
          break;

        case 's':
          sval = va_arg (ap, char *);
          if (!gUseLogFile)
            kprintf ("%s", sval);
          else
            fputs (sval, gTheFile);
          break;

        case 'c':
          schar = va_arg (ap, char);
          if (!gUseLogFile)
            kprintf ("%c", schar);
          else
            putc (schar, gTheFile);
          break;

        default:
          if (!gUseLogFile)
            kprintf ("%c", *p);
          else
            putc (*p, gTheFile);
          break;
        }
    }
  va_end (ap);
}

Item
GetMyItem (void)
/* Get the item corresponding to the current task */
{
  return KernelBase->kb_CurrentTask->t.n_Item;
}

Boolean
GetNextParam (int theArgCount, char **theArgList, int paramIndx,
              char *paramStr)
/* Get the next command line argument */
{
  char tempStr[64];

  gInvalidArg = false;
  if (paramIndx < (theArgCount - 1))
    {
      strcpy (tempStr, theArgList[paramIndx + 1]);
      if (tempStr[0] != '-')
        {
          strcpy (paramStr, tempStr);
          return true;
        }
    }

  return false;
}

Boolean
GetFlag (int theArgCount, char **theArgList, int *paramIndx, char *paramName,
         bool *theFlag)
/* Set a binary-valued tool option using the current command line arguments */
{
  char tempStr[64];

  if (strncasecmp (theArgList[*paramIndx], paramName) == 0)
    {
      if (GetNextParam (theArgCount, theArgList, *paramIndx, tempStr))
        {
          if (strncasecmp (tempStr, kOn) == 0)
            {
              *theFlag = true;
              (*paramIndx)++;
            }
          else if (strncasecmp (tempStr, kOff) == 0)
            {
              *theFlag = false;
              (*paramIndx)++;
            }
          else if (strlen (tempStr) != 0)
            gInvalidArg = true;
        }
      return true;
    }
  else
    return false;
}

int32
SleepSeconds (int32 Secs)
/* Sleep the specified number of seconds */
/* NOTE: You must open the audio folio to use this! */
{
  frac16 Rate;
  Rate = GetAudioRate () >> 16;
  return SleepAudioTicks (Secs * Rate);
}
