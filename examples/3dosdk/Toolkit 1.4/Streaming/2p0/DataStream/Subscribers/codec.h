
#ifndef __codec_h
#define __codec_h

/*
	codec.h
	Codec Includes
	Peter Barrett
	Copyright (C) 1992, SuperMac Technology.
*/

typedef char* codec;
typedef char* codecHandler;

codecHandler CreateCodecHandler(void);
void	DisposeCodecHandler(codecHandler ch);
		
codec	CreateCodec(codecHandler ch, long codecType);
void	DisposeCodec(codec c);
int		PreDecompress(codec c, void *compressedFrame);
int		Decompress(codec c, void *compressedFrame, char *baseAddr, long rowBytes);

#endif
