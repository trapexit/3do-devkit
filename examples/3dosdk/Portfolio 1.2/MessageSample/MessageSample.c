/*
**
**		MessageSample.c
**			A sample that demonstrates the Kernel's messaging
*capabilities
**
**			GREEN Hardware, 4B2 release
**
**		7/22/93		NMD		Started
**		8/1/93		NMD		Added snazzy graphics
**		8/11/93		NMD		Added snazzy sound support
**
**		What's the point:	Demonstrating messaging between
*processes, dynamic registration of processes,
**							using sound effects,
*etc...
**
**		How you use it:		Run MessageSample.  As soon as it
*starts up and announces its rediness, run *
*the program MessageClient a few times.  You can have up to 3 MessageClients
*running
**							at the same time.  Press the
*'A' Button on the control pad to "Play"
**
*/

#include "Audio.h"
#include "Init3DO.h"
#include "Kernel.h"
#include "Mem.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "String.h"
#include "Task.h"
#include "Utils3DO.h"
#include "event.h"
#include "filefunctions.h"
#include "filestream.h"
#include "filestreamfunctions.h"

#include "MarcoMessages.h"

#ifdef DEBUG
#define CHECKRESULTSTATUS(res, s)                                             \
  {                                                                           \
    printf ("%s - ", s);                                                      \
    PrintfSysErr (res);                                                       \
    printf ("\n");                                                            \
  }

#define FAILNILSTATUS(s)                                                      \
  {                                                                           \
    printf ("%s - returned NULL\n", s);                                       \
  }
#endif

#ifndef DEBUG
#define CHECKRESULTSTATUS(res, s)
#define FAILNILSTATUS(s)
#endif

#define CHECKRESULT(res, s)                                                   \
  if (res < 0)                                                                \
    {                                                                         \
      CHECKRESULTSTATUS (res, s);                                             \
      goto FAILED;                                                            \
    }

#define FAILNIL(ptr, s)                                                       \
  if (ptr == 0)                                                               \
    {                                                                         \
      FAILNILSTATUS (s);                                                      \
      goto FAILED;                                                            \
    }

typedef struct PlayerNode
{
  Node theNode;
  Item msgPortItem;
  Item theMsgItem;
} PlayerNode, *PlayerNodePtr;

typedef struct
{
  Item sample;
  Item instrument;
  Item attachment;
  Item cue;
  int32 cueSignal;
} AudioSampleStructure, *AudioSampleStructurePtr;

#define CURRENT_TASK_PRIORITY (KernelBase->kb_CurrentTask->t.n_Priority)
#define THREAD_PARENT                                                         \
  ((Item)KernelBase->kb_CurrentTask->t_ThreadTask->t.n_Item)
#define TheOtherScreen(n) (n ? 0 : 1)

#define XMAX 320
#define YMAX 240
#define XMIN 0
#define YMIN 0

#define kNumJoyPads 1

#define MAX_CLIENTS 3

#define FILE_MARCOSAMPLE "marcoPolo/MarcoSound.AIF"
#define FILE_POLOSAMPLE "marcoPolo/PoloSoundx.AIF"

ScreenContext gTheScreen;
bool gAlive = true;

int32 gPlayersRegistered;

int32 gMarcoSignalFlag;

CCB *gMarcoCel;
CCB *gPoloCel;

AudioSampleStructurePtr gMarcoSample;
Item gMarcoCue;

AudioSampleStructurePtr gPoloSamples[MAX_CLIENTS];
Item gMixer;

int32 Init3DOBox (void);
void DoUserAction (uint32 inputFlags, List *playerList, int32 *repliesNeeded);
bool ProcessIncommingMessages (Item thePort, List *playerList,
                               int32 *repliesNeeded);
int32 BroadcastMessageToList (List *playerList, int32 message, int32 arg);
bool InitGameAudio (void);
AudioSampleStructurePtr SetUpAudioItem (char *sampleFileName,
                                        char *instrumentName);
void DeleteAudioItem (AudioSampleStructurePtr theSample);
Item SetupMixer (void);
int32 PlugAudioSampleIntoMixer (Item theMixer, char *where,
                                AudioSampleStructurePtr theSample);
int32 PlayAudioSample (AudioSampleStructurePtr theSample);
void ClearBackground (void);

int
main (int argc, char **argv)
{
  int32 err;

  ControlPadEventData *controlData;

  int32 marcoSignalFlag;
  Item marcoPort;

  List playerList;
  int32 playerReplies;

  gPlayersRegistered = playerReplies = 0;

  InitList (&playerList, "Player List");

  controlData = (ControlPadEventData *)AllocMem (sizeof (ControlPadEventData),
                                                 MEMTYPE_DMA);
  FAILNIL (controlData, "AllocMem for controlData");

  err = Init3DOBox ();
  CHECKRESULT (err, "Init3DOBox");

  InitGameAudio ();

  //	Establish a Message port for this task
  //
  marcoSignalFlag = AllocSignal (0);
  CHECKRESULT (marcoSignalFlag, "(main)AllocSignal");

  marcoPort = CreateMsgPort ("MARCO", 0, marcoSignalFlag);
  CHECKRESULT (marcoPort, "(main) CreateMsgPort");
  printf ("***Marco Server Ready (%x)\n\n", marcoPort);

  //	While we're supposed to be doing our thing
  //
  while (gAlive)
    {

      // Wait for any non-system signal
      //
      WaitSignal (0x7fffff00);

      err = GetControlPad (1, false, controlData);
      CHECKRESULT (err, "GetControlPad");

      DoUserAction (controlData->cped_ButtonBits, &playerList, &playerReplies);

      ProcessIncommingMessages (marcoPort, &playerList, &playerReplies);
    }

  FadeToBlack (&gTheScreen, 1 * 60);

  printf ("Shell Exit\n\n");

  KillEventUtility ();
  return TRUE;

FAILED:
  KillEventUtility ();

  return FALSE;
}

int32
Init3DOBox (void)
{
  int32 err;
  Item copyReq;

  //	Set up the graphics context
  //		In this application, we're not really going to be using the screen
  //a whole lot, but 		we'll pretend we're going to be doublebuffering.
  //
  gTheScreen.sc_nScreens = 2;
  gTheScreen.sc_curScreen = 0;

  err = OpenGraphics (&gTheScreen, (int)gTheScreen.sc_nScreens);
  CHECKRESULT (err, "OpenGraphics");

  err = OpenSPORT ();
  CHECKRESULT (err, "OpenSPORT");

  // Load a copy of the background image, and clone it to an offscreen buffer
  //
  LoadImage ("MarcoPolo/sky.img", gTheScreen.sc_Bitmaps[0]->bm_Buffer, 0,
             &gTheScreen);
  copyReq = GetVRAMIOReq ();
  CopyVRAMPages (
      copyReq,
      gTheScreen.sc_Bitmaps[TheOtherScreen (gTheScreen.sc_curScreen)]
          ->bm_Buffer,
      gTheScreen.sc_Bitmaps[gTheScreen.sc_curScreen]->bm_Buffer,
      gTheScreen.sc_nFrameBufferPages, ~0);
  DeleteItem (copyReq);

  // Load the cels we'll be displaying
  //
  gMarcoCel = LoadCel ("MarcoPolo/MarcoBalloon.CEL", MEMTYPE_CEL);
  FAILNIL (gMarcoCel, "LoadCel on MarcoCel");

  gMarcoCel->ccb_NextPtr = NULL;
  gMarcoCel->ccb_Flags |= CCB_LAST;
  gMarcoCel->ccb_XPos = 100 << 16;
  gMarcoCel->ccb_YPos = 70 << 16;

  gPoloCel = LoadCel ("MarcoPolo/PoloBalloon.CEL", MEMTYPE_CEL);
  FAILNIL (gMarcoCel, "LoadCel on MarcoCel");

  gPoloCel->ccb_NextPtr = NULL;
  gPoloCel->ccb_Flags |= CCB_LAST;
  gPoloCel->ccb_XPos = 50 << 16;
  gPoloCel->ccb_YPos = 50 << 16;

  DisplayScreen (gTheScreen.sc_Screens[gTheScreen.sc_curScreen], 0);

  // Set up the Event Broker
  //		We're going to be looking at the control pad quite a bit, so we'll
  //want to use the 		event broker to avoid that evil of evils: Busy waiting.
  //

  err = InitEventUtility (kNumJoyPads, 0, 0);
  CHECKRESULT (err, "InitEventUtility");

  err = OpenAudioFolio ();
  CHECKRESULT (err, "OpenAudioFolio");

  // ShutDown ();

  return 0;

FAILED:
  // ShutDown ();

  return -1;
}

void
DoUserAction (uint32 inputFlags, List *playerList, int32 *repliesNeeded)
{
  int32 err;

  switch (inputFlags)
    {
    case ControlStart:

      //	The user pressed start: exit and tell all our clients to go
      //away
      //
      BroadcastMessageToList (playerList, kMarco_GameOver, 0);
      gAlive = false;
      break;

    case ControlA:

      //	The user pressed 'A': do an Audio-Visual Marco and tell the
      //clients we 	shouted.
      //
      if (*repliesNeeded == 0)
        {
          DrawScreenCels (gTheScreen.sc_Screens[gTheScreen.sc_curScreen],
                          gMarcoCel);
          DisplayScreen (gTheScreen.sc_Screens[gTheScreen.sc_curScreen], 0);

          err = PlayAudioSample (gMarcoSample);
          CHECKRESULT (err, "StartInstrument");

          WaitSignal (gMarcoSample->cueSignal);

          ClearBackground ();

          BroadcastMessageToList (playerList, kMarco_ShoutMarco, 0);

          *repliesNeeded = gPlayersRegistered;
        }
      break;
    }
FAILED:
  return;
}

bool
ProcessIncommingMessages (Item thePort, List *playerList, int32 *repliesNeeded)
{
  PlayerNodePtr thisNode;
  int32 err;

  Item playerMsgItem;
  Message *playerMsg;

  uint32 reply;

  TagArg targs[3];

  playerMsgItem = GetMsg (thePort);
  CHECKRESULT (playerMsgItem, "GetMsg");
  if (playerMsgItem != 0)
    {
      //	FAILNIL (playerMsgItem, "GetMsg");

      playerMsg = (Message *)LookupItem (playerMsgItem);
      FAILNIL (playerMsg, "LookupItem");

      // First, Make sure that we got a Short Message (the only kind
      //	we know how to deal with)
      //

      if (playerMsg->msg.n_Flags & MESSAGE_SMALL)
        {

          //	Set up the default reply
          //
          reply = kMarco_OK;

          switch ((int32)playerMsg->msg_DataPtr)
            {
            case kMarco_RegisterPlayer:

              //	REGISTER A NEW PLAYER
              //		Allocate space for a new player in our list,
              //add 		its message port Item number, and create a message 		item to
              //use for communication.
              //

              if (gPlayersRegistered < MAX_CLIENTS)
                {

                  ++gPlayersRegistered;

                  thisNode = (PlayerNodePtr)AllocMem (sizeof (PlayerNode),
                                                      MEMTYPE_DMA);
                  FAILNIL (thisNode, "player AllocMem");

                  thisNode->msgPortItem = playerMsg->msg_ReplyPort;
                  printf ("\n\n***Server: message port %x registered\n",
                          playerMsg->msg_ReplyPort);

                  //	We want to have a message item associated with each
                  //	registered node to avoid conflicts
                  //
                  targs[0].ta_Tag = CREATEMSG_TAG_REPLYPORT;
                  targs[0].ta_Arg = (void *)thePort;
                  targs[1].ta_Tag = CREATEMSG_TAG_MSG_IS_SMALL;
                  targs[1].ta_Arg = (void *)8;
                  targs[2].ta_Tag = TAG_END;

                  thisNode->theMsgItem
                      = CreateItem (MKNODEID (KERNELNODE, MESSAGENODE), targs);
                  CHECKRESULT (thisNode->theMsgItem, "CreateItem");

                  AddHead (playerList, (Node *)thisNode);
                }
              else
                {
                  reply = kMarco_YouCantPlay;
                }
              break;

            case kMarco_ShoutPolo:

              // Deal with a Polo shouted back at us in an Audio-Visual manner
              //
              --(*repliesNeeded);

              gPoloCel->ccb_XPos = (ReadHardwareRandomNumber () & 150) << 16;
              gPoloCel->ccb_YPos = (ReadHardwareRandomNumber () & 100) << 16;

              DrawScreenCels (gTheScreen.sc_Screens[gTheScreen.sc_curScreen],
                              gPoloCel);
              DisplayScreen (gTheScreen.sc_Screens[gTheScreen.sc_curScreen],
                             0);

              err = PlayAudioSample (gPoloSamples[*repliesNeeded]);
              CHECKRESULT (err, "PlayAudioSample");

              if (*repliesNeeded == 0)
                {
                  WaitSignal (gPoloSamples[*repliesNeeded]->cueSignal);
                  ClearBackground ();
                }

              break;

            case kMarco_OK:
              break;

            default:
              reply = kMarco_SayWhat;
            }

          //	Acknowledge the reciept of a new message, but don't reply to a
          //	reply
          //
          if (playerMsg->msg_Result != kMarco_Recieved)
            {
              err = ReplySmallMsg (playerMsgItem, kMarco_Recieved, reply, 0);
              CHECKRESULT (err, "replySmallMsg");
            }
        }

      return 1;
    }
FAILED:
  return 0;
}

int32
BroadcastMessageToList (List *playerList, int32 message, int32 arg)
{
  int32 err;
  PlayerNodePtr thisNode;

  err = 0;

  if (!IsEmptyList (playerList))
    {

      thisNode = (PlayerNodePtr)FirstNode (playerList);

      for (thisNode = (PlayerNodePtr)FirstNode (playerList);
           IsNode (playerList, (Node *)thisNode);
           thisNode = (PlayerNodePtr)NextNode ((Node *)thisNode))
        {

          err = SendSmallMsg (thisNode->msgPortItem, thisNode->theMsgItem,
                              message, arg);
          CHECKRESULT (err, "SendSmallMsg in BroadCastMessage");
        }
    }

FAILED:

  return err;
}

bool
InitGameAudio (void)
{
  int32 i;
  int32 err;
  char inputx[7] = "Inputx";
  char poloSoundx[] = FILE_POLOSAMPLE;

  gMixer = SetupMixer ();
  CHECKRESULT (gMixer, "InitGameAudio: SetUpMixer");

  gMarcoSample = SetUpAudioItem (FILE_MARCOSAMPLE, "halfmono8.dsp");
  FAILNIL (gMarcoSample, "InitGameAudio: SetupAudioItem for Marco");

  err = PlugAudioSampleIntoMixer (gMixer, "Input0", gMarcoSample);
  CHECKRESULT (err, "InitGameAudio: PlugAudioSampleIntoMixer for Marco");

  for (i = 0; i < MAX_CLIENTS; i++)
    {

      // Load the individual Polo Sounds and connect them to the appropriate
      // mixer ins
      //

      poloSoundx[19] = '0';
      poloSoundx[19] += (char)i;
      gPoloSamples[i] = SetUpAudioItem (poloSoundx, "halfmono8.dsp");
      FAILNIL (gPoloSamples[i], "InitGameAudio: SetupAudioItem for a Polo");

      inputx[5] = '1';
      inputx[5] += (char)i;
      err = PlugAudioSampleIntoMixer (gMixer, inputx, gPoloSamples[i]);
      CHECKRESULT (err, "InitGameAudio: PlugAudioSampleIntoMixer for polo");
    }

  return 1;

FAILED:

  return 0;
}

AudioSampleStructurePtr
SetUpAudioItem (char *sampleFileName, char *instrumentName)
{
  int32 err;

  AudioSampleStructurePtr newSample;

  newSample = (AudioSampleStructurePtr)AllocMem (sizeof (AudioSampleStructure),
                                                 MEMTYPE_DMA);
  FAILNIL (newSample, "SetUpAudioItem: AllocMem");

  // Load the specified sample-player instrument
  //
  newSample->instrument = LoadInstrument (instrumentName, 0, 100);
  CHECKRESULT (newSample->instrument, "SetUpAudioItem: LoadInstrument");

  // Load the specified sample
  //
  newSample->sample = LoadSample (sampleFileName);
  CHECKRESULT (newSample->sample, "SetUpAudioItem: LoadSample");

  // Create a cue item to monitor the sample
  //
  newSample->cue = CreateItem (MKNODEID (AUDIONODE, AUDIO_CUE_NODE), NULL);
  CHECKRESULT (gMarcoCue, "SetUpAudioItem: CreateItem Cue");
  newSample->cueSignal = GetCueSignal (newSample->cue);

  // Link the sample to an instrument
  //
  newSample->attachment
      = AttachSample (newSample->instrument, newSample->sample, 0);
  CHECKRESULT (newSample->attachment, "SetUpAudioItem: AttachSample");

  // Activate a signal to monitor the attachment
  //
  err = MonitorAttachment (newSample->attachment, newSample->cue, CUE_AT_END);
  CHECKRESULT (err, "SetUpAudioItem: MonitorAttachment");
  return newSample;

FAILED:

  return 0;
}

void
DeleteAudioItem (AudioSampleStructurePtr theSample)
{
  DeleteItem (theSample->sample);
  DeleteItem (theSample->instrument);
  DeleteItem (theSample->cue);
  DeleteItem (theSample->attachment);

  FreeSignal (theSample->cueSignal);

  FreeMem (theSample, sizeof (AudioSampleStructure));
}

Item
SetupMixer (void)
{
  int32 err;
  Item mixer;
  Item knob;

  // Load a 4x2 mixer and set the output of each channel to be equal
  //  (the brute force method, of course)
  err = mixer = LoadInstrument ("mixer4x2.dsp", 0, 100);
  CHECKRESULT (mixer, "LoadInstrument: mixer");
  StartInstrument (mixer, NULL);

  err = knob = GrabKnob (mixer, "LeftGain0");
  CHECKRESULT (knob, "GrabKnob L0");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob L0");

  err = knob = GrabKnob (mixer, "RightGain0");
  CHECKRESULT (knob, "GrabKnob R0");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob R0");

  err = knob = GrabKnob (mixer, "LeftGain1");
  CHECKRESULT (knob, "GrabKnob L1");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob L1");

  err = knob = GrabKnob (mixer, "RightGain1");
  CHECKRESULT (knob, "GrabKnob R1");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob R1");

  err = knob = GrabKnob (mixer, "LeftGain2");
  CHECKRESULT (knob, "GrabKnob L2");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob L2");

  err = knob = GrabKnob (mixer, "RightGain2");
  CHECKRESULT (knob, "GrabKnob R2");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob R2");

  err = knob = GrabKnob (mixer, "LeftGain3");
  CHECKRESULT (knob, "GrabKnob L3");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob L3");

  err = knob = GrabKnob (mixer, "RightGain3");
  CHECKRESULT (knob, "GrabKnob R3");
  err = TweakKnob (knob, 0x1fff);
  CHECKRESULT (err, "TweakKnob R3");

  return mixer;
FAILED:
  return err;
}

int32
PlugAudioSampleIntoMixer (Item theMixer, char *where,
                          AudioSampleStructurePtr theSample)
{
  int32 err;

  err = ConnectInstruments (theSample->instrument, "Output", theMixer, where);

  return err;
}

int32
PlayAudioSample (AudioSampleStructurePtr theSample)
{
  int32 err;
  TagArg Tags[2];

  Tags[0].ta_Tag = AF_TAG_AMPLITUDE;
  Tags[0].ta_Arg = (int32 *)0x7FFF;
  Tags[1].ta_Tag = TAG_END;

  err = StartInstrument (theSample->instrument, &Tags[0]);

  return err;
}

void
ClearBackground (void)
{
  Item copyReq;

  copyReq = GetVRAMIOReq ();
  CopyVRAMPages (
      copyReq, gTheScreen.sc_Bitmaps[gTheScreen.sc_curScreen]->bm_Buffer,
      gTheScreen.sc_Bitmaps[TheOtherScreen (gTheScreen.sc_curScreen)]
          ->bm_Buffer,
      gTheScreen.sc_nFrameBufferPages, ~0);
  DeleteItem (copyReq);
}
