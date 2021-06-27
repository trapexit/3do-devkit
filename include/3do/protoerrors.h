#ifndef __PROTOERRORS_H__
#define __PROTOERRORS_H__

#include "types.h"

enum ProtoErrorCode
  {
   ProtoErrStreamVersionInvalid, /* Subscriber not compatible this version */
   ProtoErrChanOutOfRange	/* channel number not valid in context */
  };

#endif 	/* __PROTOERRORS_H__ */
