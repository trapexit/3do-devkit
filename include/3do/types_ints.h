#pragma include_only_once

/* 8bit */
typedef signed char    i8;
typedef unsigned char  u8;
typedef i8             int8;
typedef i8             sint8;
typedef i8             schar;
typedef i8             sbyte;
typedef u8             uint8;
typedef u8             uchar;
typedef u8             ubyte;
typedef volatile i8    vi8;
typedef volatile u8    vu8;
typedef volatile int8  vint8;
typedef volatile sint8 vsint8;
typedef volatile uint8 vuint8;

/* 16bit */
/* generally avoid 16bit ints */
typedef signed short    i16;
typedef unsigned short  u16;
typedef i16             int16;
typedef i16             sint16;
typedef u16             uint16;
typedef u16             unichar;
typedef volatile i16    vi16;
typedef volatile u16    vu16;
typedef volatile int16  vint16;
typedef volatile sint16 vsint16;
typedef volatile uint16 vuint16;

/* 32bit */
typedef signed long     i32;
typedef unsigned long   u32;
typedef i32             int32;
typedef i32             sint32;
typedef u32             uint32;
typedef u32             ulong;
typedef volatile i32    vi32;
typedef volatile u32    vu32;
typedef volatile int32  vint32;
typedef volatile sint32 vsint32;
typedef volatile uint32 vuint32;
typedef volatile ulong  vulong;
