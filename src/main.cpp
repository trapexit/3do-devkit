/*
  ISC License

  Copyright (c) 2024, Antonio SJ Musumeci <trapexit@spawn.link>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
  As a collection of examples grow it's be nice to create a menu to
  allow a user to select which example to execute. Until then please
  just move the function to the top which you wish to use.
*/

#include "stdio.h"

static struct { int a; int b; } foo;
extern int bar = 0xdeadbeef;

int
main(int   argc_,
     char *argv_)
{
  foo.a = 0xa5a5a5a5;
  foo.b = 0x5a5a5a5a;
  
  printf("&foo = %p (%x,%x); &bar = %p (%x);\n",
         &foo,
         foo.a,
         foo.b,
         &bar,
         bar);

  (&foo)[1].a = 0x8badfood;
  printf("&foo = %p (%x,%x); &bar = %p (%x);\n",
         &foo,
         foo.a,
         foo.b,
         &bar,
         bar);
  
  while(true);
  return 0;
}
