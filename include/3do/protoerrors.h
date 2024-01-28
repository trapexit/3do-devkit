#pragma include_only_once

#include "types.h"

enum ProtoErrorCode
  {
   ProtoErrStreamVersionInvalid, /* Subscriber not compatible this version */
   ProtoErrChanOutOfRange	 /* channel number not valid in context */
  };
