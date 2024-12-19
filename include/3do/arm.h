#pragma once

#include "types_ints.h"

/* Get PSR register contents & 0x1F */
u32 getPSR(void);
/* ((getPSR() & 0x3) == 0) */
int isUser(void);
