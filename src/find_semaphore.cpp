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

#include "display.hpp"

#include "abort.h"

#include "operamath.h"
#include "semaphore.h"
#include "stdio.h"

#define ROW_HEIGHT 8

static
void
print(BasicDisplay &display_,
      const int     x_,
      const int     y_,
      const char   *fmt_,
      ...)
{
  va_list args;

  va_start(args,fmt_);
  vprintf(fmt_,args);
  display_.draw_vprintf(x_,y_,fmt_,args);
  va_end(args);
}


int
main_find_semaphore()
{
  int x;
  int y;
  int32 err;
  Item sema_item;
  Semaphore *sema_ptr;
  BasicDisplay display;

  display.clear();
  display.display();

  x = 16;
  y = 16;

  err = OpenMathFolio();
  if(err < 0)
    abort("OpenMathFolio() failed to open");

  sema_item = FindSemaphore("MathSemaphore");
  if(sema_item < 0)
    abort_err(sema_item);
  print(display,x,y+=ROW_HEIGHT,"MathSemaphore Item = %p\n",sema_item);

  sema_ptr = (Semaphore*)LookupItem(sema_item);
  print(display,x,y+=ROW_HEIGHT,"MathSemaphore Ptr = %p\n",sema_ptr);
  print(display,x,y+=ROW_HEIGHT,"MathSemaphore->s.n_Item = %p\n",sema_ptr->s.n_Item);

  while(1);

  return 0;
}
