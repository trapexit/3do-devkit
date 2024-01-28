/*
        File:		CApplication.h

        Contains:	Simple application class (header)

        Written by:	Paul A. Bauersfeld
                                Jon A. Weiner

        Copyright:	© 1994 by The 3DO Company. All rights reserved.
                                This material constitutes confidential and
   proprietary information of the 3DO Company and shall not be used by any
   Person or for any purpose except as expressly authorized in writing by the
   3DO Company.

        Change History (most recent first):

                <2>		1/19/94		cde
   Converted to use event broker. <1>		10/28/93	pab
   New today.

        To Do:
*/

#ifndef _CAPPLICATION_H_
#define _CAPPLICATION_H_

#define kControlAll                                                           \
  (ControlStart | ControlX | ControlUp | ControlDown | ControlLeft            \
   | ControlRight | ControlA | ControlB | ControlC)

class CApplication
{
public:
  CApplication (void);
  ~CApplication (void);

  virtual void Run (void);
  virtual void Stop (void);

protected:
  virtual uint32 DoControlPad (uint32 continuousMask);

  virtual void OnStart (void){};
  virtual void OnX (void){};

  virtual void OnUp (void){};
  virtual void OnDown (void){};
  virtual void OnLeft (void){};
  virtual void OnRight (void){};

  virtual void OnA (void){};
  virtual void OnB (void){};
  virtual void OnC (void){};

private:
  long fContinue;
};

#endif
