#pragma once

#include "types_ints.h"

/*
  Count the number of bits set in a word.
  See portfolio_os/src/libs/c/CountBits.c
*/
int CountBits(u32 word);

/*
  Find least significant bit position in 32bit word.
  See portfolio_os/src/libs/c/FindLSB.s
*/
int FindLSB(u32 word);

/*
  Find most significant bit position in 32bit word.
  See portfolio_os/src/libs/c/FindMSB.s
*/
int FindMSB(u32 word);
