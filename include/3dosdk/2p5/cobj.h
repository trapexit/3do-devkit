#ifndef __COBJ_H
#define __COBJ_H

#pragma force_top_level
#pragma include_only_once


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


#ifdef NOT_ARM
void bcopy ( char *s, char *d, long n )
{
	long i;
	for (i=0; i<n; i++) *d++ = *s++;
}
#include <stdio.h>
typedef long int32;
#else

#endif

#define VALID_OBJECT_KEY  (0xABCD4321)
#define COBJ_ERR_NO_MEM (-1)
#define COBJ_ERR_NO_METHOD (-2)
#define COBJ_ERR_DATA_SIZE (-3)
#define COBJ_ERR_NULL_OBJECT (-4)

typedef struct COBClass
{
	struct  COBClass *Super;		/* Superclass */
	int32    DataSize;				/* Size of an object of this class */
	int32    (*Init)();
	int32    (*Term)();
	int32    (*Print)();
	int32    (*SetInfo)();
	int32    (*GetInfo)();
	int32    (*Alloc)();
	int32    (*Free)();
	int32    (*Add)();
	int32    (*Clear)();
	int32    (*GetNthFrom)();
	int32    (*RemoveNthFrom)();
	int32    (*Start)();
	int32    (*Stop)();
	int32    (*Bump)();
	int32    (*Rewind)();
	int32    (*Pause)();
	int32    (*Unpause)();
	int32    (*Abort)();
	int32    (*Finish)();
	int32    (*Done)();
} COBClass;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

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

#define COBObjectIV \
	Node      COBNode; \
	COBClass  *Class;  \
	uint32    cob_ValidationKey

typedef struct COBObject
{
	COBObjectIV;
} COBObject;

int32 DefineClass( COBClass *Class, COBClass *SuperClass, int32 DataSize);
COBObject *CreateObject( COBClass *Class);
int32 DestroyObject( COBObject *Object );
int32 ValidateObject( COBObject *cob );

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __COBJ_H */
