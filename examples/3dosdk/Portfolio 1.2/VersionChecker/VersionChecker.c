/*
        File:		VersionChecker.c

        Contains:	Version Checker.
                                Finds a task and displays its version

        Written by:	Charlie Eckhaus (tip o' the hat to Mike Ramirez)

        Copyright:	© 1993 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                 <1>	 7/8/93		CDE		Derived from
   TaskDemon.c

        To Do:
*/

/***** Version string *****/
#define kVersionStr "V1.0"

/***** Default log filename: *****/
#define kDefaultLogFile "VersionChecker.out"

/***** Joystick control *****/
#define JOYALL                                                                \
  (JOYSTART | JOYUP | JOYDOWN | JOYLEFT | JOYRIGHT | JOYFIREA | JOYFIREB      \
   | JOYFIREC)

/***** Screen display control *****/
#define kLineHeight 9
#define kDisplayTopOffset 40
#define kMinDisplayLine 1
#define kMaxDisplayLine 10
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

/***** Include files *****/
#include "baseUtils.h"

/***********************************************************************************\
*
*	Globals
*
\***********************************************************************************/

/* log file */
char gLogFile[64];
Boolean gUseLogFile;
FILE *gTheFile;

/* Parameter-checking */
Boolean gInvalidArg;

/* Other test options */
int32 gDisplayLine = kMinDisplayLine;
GrafCon gGrafCon;
ScreenContext gSC;

/***** Utilities *****/

/* Straight from Kernighan & Ritchie (typed by Mike); Added explicit cast of j
 * in for loop */
void
reverse (char s[])
{
  int c, i, j;

  for (i = 0, j = (int)(strlen (s) - 1); i < j; i++, j--)
    {
      c = s[i];
      s[i] = s[j];
      s[j] = c;
    }
}

void
itoa (int n, char s[])
{
  int i, sign;

  if ((sign = n) < 0)
    n = -n;
  i = 0;
  do
    {
      s[i++] = n % 10 + '0';
    }
  while ((n /= 10) > 0);
  if (sign < 0)
    s[i++] = '-';
  s[i] = '\0';
  reverse (s);
}

/**** (End Utilities) ****/

void
ShowHelp ()
/* Show Help for this tool */
/* HINTS:

        ¥ Incorporate your tool name, your name, additional options, all of the
   category's call names
*/
{
  kprintf (
      "\n#################################################################\n");
  kprintf ("Version Checker %s HelpÉ\n", kVersionStr);
  kprintf ("Written by Charlie Eckhaus for The 3DO Company\n");
  kprintf (
      "#################################################################\n");
  kprintf ("\n");

  kprintf ("Check versions of specific 3DO_OS items\n");
  kprintf ("\n");

  kprintf ("-out << no parameter>>\tDo not use a log file.\n");
  kprintf ("     <<filename>>     \tUse specified log file.\n");
  kprintf ("\n");

  kprintf (
      "#################################################################\n");
}

void
ShowOptions ()
/* Show the option settings for this session */
{
  kprintf (
      "\n#################################################################\n");
  kprintf ("Version Checker %s - options\n", kVersionStr);
  kprintf ("Written by Charlie Eckhaus for The 3DO Company\n");
  kprintf (
      "#################################################################\n");

  if (gUseLogFile)
    kprintf ("-out: Use log file: %s.\n", gLogFile);
  else
    kprintf ("Do not use a log file.\n");

  kprintf (
      "#################################################################\n");
}

bool
GetParams (int theArgCount, char **theArgList)
/* Get the tool options from the command line arguments */
{

  int paramIndx;
  int32 paramLen;
  char tempStr[64];
  Boolean done = false;

  paramIndx = 0;

  /* log file */
  strcpy (gLogFile, kDefaultLogFile);
  gUseLogFile = FALSE;

  /* parameter-checking */
  gInvalidArg = FALSE;

  /* other parameters */

  while (!done)
    {
      paramLen = strlen (
          theArgList[paramIndx]); // get the length of the current param

      if (strncasecmp (theArgList[paramIndx], "?") == 0)
        {
          ShowHelp ();
          return kShowHelp;
        }
      else if (strncasecmp (theArgList[paramIndx], "-out") == 0)
        {
          /* Need to use a log file:
                   If the user specifies a name, use it.
                   Otherwise, the default will be used. */

          if (GetNextParam (theArgCount, theArgList, paramIndx, tempStr))
            {
              strcpy (gLogFile, tempStr);
              paramIndx++;
            }

          gUseLogFile = OpenMacLink ();
          if (!gUseLogFile)
            kprintf ("Can't open the log file");
        }

      if (gInvalidArg)
        {
          ShowHelp ();
          return kAbortTest;
        }

      paramIndx++;
      if (paramIndx > theArgCount)
        done = true;

    } //	end while

  return kRunTest;
} // end GetParams

void
WaitForJoyPad (void)
{
  long lControlvals;

  kprintf ("Waiting for JoyPad...\n");
  while (TRUE)
    {
      lControlvals = ReadControlPad (JOYALL);
      if (lControlvals & JOYSTART)
        {
          SleepAudioTicks (120);
          break;
        }
    }
}

void
ClearScreen ()
{
  Rect aRect;

  aRect.rect_XLeft = 0;
  aRect.rect_XRight = SCREEN_WIDTH - 1;
  aRect.rect_YTop = 0;
  aRect.rect_YBottom = SCREEN_HEIGHT;
  SetFGPen (&gGrafCon, MakeRGB15 (4, 0, 6));
  FillRect (gSC.sc_BitmapItems[0], &gGrafCon, &aRect);
}

void
InspectNode (char *aName, NodeP aNode)
{
#ifdef FOR_MAC
  pipeData ("%s:\n", aName);
  pipeData ("{\n");
  pipeData ("n_Next: $%x\n", aNode->n_Next);
  pipeData ("n_Prev: $%x\n", aNode->n_Prev);
  pipeData ("n_SubsysType: %i\n", aNode->n_SubsysType);
  pipeData ("n_Type: %i\n", aNode->n_Type);
  pipeData ("n_Priority: %i\n", aNode->n_Priority);
  pipeData ("n_Flags: %i\n", aNode->n_Flags);
  pipeData ("n_Size: %i\n", aNode->n_Size);
  pipeData ("n_Name: %s\n", aNode->n_Name);
#endif
}

void
InspectItemNode (char *aName, ItemNodeP aItemNode)
{
  char aString[64];
  char a2ndString[64];

#ifdef FOR_MAC
  InspectNode (aName, (NodeP)aItemNode);
  pipeData ("ItemNode:\n");
  pipeData ("n_Version: %i\n", aItemNode->n_Version);
  pipeData ("n_Revision: %i\n", aItemNode->n_Revision);
  pipeData ("n_FolioFlags: $%x\n", aItemNode->n_FolioFlags);
  pipeData ("n_Item: $%x\n", aItemNode->n_Item);
  pipeData ("n_Owner: $%x\n", aItemNode->n_Owner);
#endif

  sprintf (aString, "%s: V%i.%i", aName, aItemNode->n_Version,
           aItemNode->n_Revision);

  /* Dave said not to use sprintf, so all this does what the above line did */
  /*	strcpy(aString, aName);
          strcat(aString, ": V");
          itoa((int) (aItemNode->n_Version), a2ndString);
          strcat(aString, a2ndString);
          strcat(aString, ".");
          itoa((int) (aItemNode->n_Revision), a2ndString);
          strcat(aString, a2ndString);
  */
  if (gDisplayLine > kMaxDisplayLine)
    {
      WaitForJoyPad ();
      ClearScreen ();
      gDisplayLine = kMinDisplayLine;
    }
#ifdef FOR_MAC
  pipeData (aString);
  pipeData ("\n\n");
#endif

  gGrafCon.gc_PenX = ((SCREEN_WIDTH / 2) - strlen (aString)) / 2;
  gGrafCon.gc_PenY = (Coord)(kDisplayTopOffset + gDisplayLine * kLineHeight);
  DrawText8 (&gGrafCon, gSC.sc_BitmapItems[0], aString);
  DisplayScreen (gSC.sc_Screens[0], 0);

  gDisplayLine++;
}

void
HandleItem (uint8 ftype, uint8 ntype, char *aName)
{
  Item aItem;
  ItemNodeP aItemNodePtr;

  /* Find the item */
  aItem = FindNamedItem (MKNODEID (ftype, ntype), aName);
  aItemNodePtr = (ItemNodeP)LookupItem (aItem);

  if (aItemNodePtr == NULL)
    kprintf ("Item %s not found!\n", aName);
  else
    {
      InspectItemNode (aName, aItemNodePtr);
#ifdef FOR_MAC
      pipeData ("}\n");
#endif
    }
}

void
RunVersionChecker ()
{
  ClearScreen ();

  HandleItem (KERNELNODE, FOLIONODE, "kernel");
  HandleItem (KERNELNODE, FOLIONODE, "Graphics");
  HandleItem (KERNELNODE, FOLIONODE, "File");
  HandleItem (KERNELNODE, FOLIONODE, "audio");
  HandleItem (KERNELNODE, FOLIONODE, "Operamath");
  HandleItem (KERNELNODE, ERRORTEXTNODE, "Kernel Extended Error Table");
  HandleItem (KERNELNODE, ERRORTEXTNODE, "Graphics Folio Error Table");
  HandleItem (KERNELNODE, ERRORTEXTNODE, "Audio Folio Error Table");
  HandleItem (KERNELNODE, TASKNODE, "Operator");
  HandleItem (KERNELNODE, TASKNODE, "shell");
  HandleItem (KERNELNODE, TASKNODE, "eventBroker");
  HandleItem (KERNELNODE, DRIVERNODE, "File");
  HandleItem (KERNELNODE, DRIVERNODE, "CD-ROM");
  HandleItem (KERNELNODE, DRIVERNODE, "SPORT");

  WaitForJoyPad ();
}

/***********************************************************************************\
*
*	Just Do It!
*
\***********************************************************************************/

Boolean
InitForDrawText ()
{
  if (!OpenGraphics (&gSC, 1))
    {
      kprintf ("Can't open graphics!");
      return FALSE;
    }

#ifdef USE_4B1
  if (ResetCurrentFont () < 0)
    {
      kprintf ("Can't reset font!");
      return FALSE;
    }
#else
  ResetCurrentFont ();
#endif
  return TRUE;
}

Boolean
InitProgram ()
{
  int32 theError;

  if (OpenAudioFolio ())
    {
      kprintf ("Can't open the audio folio!");
      return FALSE;
    }

  if (OpenGraphicsFolio () < 0)
    {
      kprintf ("Can't open the graphics folio!");
      return FALSE;
    }

  if (!InitForDrawText ())
    {
      kprintf ("Can't initialize for drawing text!");
      return FALSE;
    }

  theError = SleepSeconds (1);
  if (theError)
    {
      PrintfSysErr (theError);
      return FALSE;
    }

  return TRUE;
}

int
main (int argc, char **argv)
{
  if (InitProgram ())
    {
      if (GetParams (argc, argv) == kRunTest)
        {
          ShowOptions ();

          if (gUseLogFile)
            {
              gTheFile = fopen (gLogFile, "w");
#ifdef FOR_MAC
              pipeData ("## Begin Version Checker\n");
              pipeData ("##\n");
#endif
            }

          RunVersionChecker ();

          if (gUseLogFile)
            {
#ifdef FOR_MAC
              pipeData ("##\n");
              pipeData ("## End Version Checker\n");
              fclose (gTheFile);
#endif
            }
        }

      kprintf ("##############################################################"
               "###\n");
      kprintf ("Version Checker - EndÉ\n");
      kprintf ("##############################################################"
               "###\n");
    }
}
