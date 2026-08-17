#pragma once
/****************************************************************************
 **
 **  Copyright (C) 1995, an unpublished work by The 3DO Company. All rights reserved.
 **  This material contains confidential information that is the property of The 3DO Company.
 **  Any unauthorized duplication, disclosure or use is prohibited.
 **  $Id: cobj.h,v 1.12 1994/09/10 00:17:48 peabody Exp $
 **
 **  CObject support
 **
 **  By: Phil Burk
 **
 ****************************************************************************/


#include "extern_c.h"
#include "types.h"
#include "nodes.h"


#ifdef NOT_ARM
void bcopy ( char *s, char *d, long n )
{
  long i;
  for (i=0; i<n; i++) *d++ = *s++;
}
#include "stdio.h"

#else

#endif

#define VALID_OBJECT_KEY     (0xABCD4321)
#define COBJ_ERR_NO_MEM      (-1)
#define COBJ_ERR_NO_METHOD   (-2)
#define COBJ_ERR_DATA_SIZE   (-3)
#define COBJ_ERR_NULL_OBJECT (-4)

typedef struct COBClass
{
  struct  COBClass *Super;	/* Superclass */
  s32             DataSize;	/* Size of an object of this class */
  s32    (*Init)();
  s32    (*Term)();
  s32    (*Print)();
  s32    (*SetInfo)();
  s32    (*GetInfo)();
  s32    (*Alloc)();
  s32    (*Free)();
  s32    (*Add)();
  s32    (*Clear)();
  s32    (*GetNthFrom)();
  s32    (*RemoveNthFrom)();
  s32    (*Start)();
  s32    (*Stop)();
  s32    (*Bump)();
  s32    (*Rewind)();
  s32    (*Pause)();
  s32    (*Unpause)();
  s32    (*Abort)();
  s32    (*Finish)();
  s32    (*Done)();
} COBClass;

EXTERN_C_BEGIN

#define PrintObject(obj) obj->Class->Print(obj)
#define SetObjectInfo(obj,tags) obj->Class->SetInfo(obj,tags)
#define GetObjectInfo(obj,tags) obj->Class->GetInfo(obj,tags)
#define StartObject(obj,time,nrep,par) obj->Class->Start(obj,time,nrep,par)
#define StopObject(obj,time) obj->Class->Stop(obj,time)
#define AbortObject(obj,time) obj->Class->Abort(obj,time)
#define AllocObject(obj,n) obj->Class->Alloc(obj,n)
#define FreeObject(obj) obj->Class->Free(obj)
#define GetNthFromObject(obj,n,ptr) obj->Class->GetNthFrom(obj,n,ptr)
#define RemoveNthFromObject(obj,n) obj->Class->RemoveNthFrom(obj,n)

#define COBObjectIV                             \
  Node      COBNode;                            \
  COBClass *Class;                              \
  u32    cob_ValidationKey

typedef struct COBObject
{
  COBObjectIV;
} COBObject;

s32 DefineClass( COBClass *Class, COBClass *SuperClass, s32 DataSize);
COBObject *CreateObject( COBClass *Class);
s32 DestroyObject( COBObject *Object );
s32 ValidateObject( COBObject *cob );

EXTERN_C_END
