/*	****************** EZQTask.h **********************

                7/6/94		WSD		New File

                */

#define MaxFrameCount 5

#include "playEZQStream.h"

typedef struct
{
  char *pixMap;
  int32 valid;
} Frame;

typedef struct EZQ_Parms
{
  Item task_Item;
  int32 result;
  int32 running;
  int32 frameCount;
  int32 frameIndex;
  Frame frames[MaxFrameCount];
  char *streamFileName;
  PlayerFuntionContext *pfc;
} EZQ_Parms;

extern EZQ_Parms EZQ_Task_Parms;
extern void startEZQTask (char *streamFileName, PlayerFuntionContext *pfc,
                          int32 frameCount, char *bufferA, char *bufferB,
                          char *bufferC);

extern void runEZQTask (void);
extern void pauseEZQTask (void);
extern void stepEZQTask (void);
extern void stopEZQTask (void);
