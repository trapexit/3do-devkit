/*
** Copyright (C) 1992, 3DO Company.
** All Rights Reserved
** Confidential and Proprietary
**
**	Module: us.h
**	Application: uso
**	Date: 4/93
**	Author: ccw
**
**
**
**
*/


#include "l3Error.h"

#ifdef DEBUG
#define STRICT
#define ODS(s)	kprintf("%s\n", s)
extern void __swi(0x101) Debugger ( void );
#define BREAKPOINT Debugger();

#else
#define ODS(s)

#endif

#define CURRENT_TASK_PRIORITY (KernelBase->kb_CurrentTask->t.n_Priority)

/*
// uso data types
*/

typedef int BOOL;

typedef struct usoGlobals {
  ScreenContext uso_sc;	// a screen context for all tasks
  Item uso_iTask;			// the uso task
  uint8 uso_nPriority;	// the priority of the USO task
  Item uso_iMp;			// the uso parent message port

} USOGLOBALS, *PUSOGLOBALS;

// screen application goodies
typedef struct saTask {
  char *pTaskName;		// the executable name
  char *pMsgPort;			// the ASCII rep of the message port
  Item iTask;				// the task Item
  Item iMsgPort;			// the message port Item
} SA_TASK, *PSA_TASK;

#define TASKNAME_USO "uso"
extern PUSOGLOBALS pUso;

// uso signals
#define USIG_MESSAGE	0x1000		// the uso standard message port signal
#define USIG_PING		0x10000	  	// childs acknowledgement of completion

// lib3DO stuff
extern BOOL l3Init(void);
extern void l3Shutdown(void);
extern BOOL l3InitGraphics(void);

// extern void *l3AllocMem(int32 size, uint32 type);
extern BOOL l3ReAlloc(void **pMem, int32 nSize, int nOldSize);

extern BOOL initScreenApp(PUSOGLOBALS *pg, int nMsgSig, char *pMsgPort);
