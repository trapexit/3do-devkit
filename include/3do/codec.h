#pragma include_only_once

#include "extern_c.h"

typedef char* codec;
typedef char* codecHandler;

EXTERN_C_BEGIN

codecHandler CreateCodecHandler(void);
void         DisposeCodecHandler(codecHandler ch);

codec CreateCodec(codecHandler ch, long codecType);
void  DisposeCodec(codec c);
int   PreDecompress(codec c, void *compressedFrame);
int   Decompress(codec c, void *compressedFrame, char *baseAddr, long rowBytes);

EXTERN_C_END
