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

#include "example_folio.h"

#include "abort.h"

#include "stdio.h"
#include "operror.h"

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
main(void)
{
  Err err;
  i32 i;
  int x;
  int y;
  BasicDisplay display;

  x = 16;
  y = 16;

  display.clear();
  display.display();

  err = OpenExampleFolio();
  print(display,x,y+=ROW_HEIGHT,"OpenExampleFolio() = %x\n",err);
  if(err < 0)
    {
      char errstr[256];

      GetSysErr(errstr,sizeof(errstr),err);
      print(display,x,y+=ROW_HEIGHT,"%s\n",errstr);
      abort_err(err);
    }

  i = AddI32(5,7);
  print(display,x,y+=ROW_HEIGHT,"AddI32(5,7) = %d\n",i);

  i = SWIAddI32(5,8);
  print(display,x,y+=ROW_HEIGHT,"SWIAddI32(5,8) = %d\n",i);

  i = SWIMADAMRev();
  print(display,x,y+=ROW_HEIGHT,"SWIMADAMRev() = %p\n",i);

  i = SWICLIORev();
  print(display,x,y+=ROW_HEIGHT,"SWICLIORev() = %p\n",i);

  err = CloseExampleFolio();
  print(display,x,y+=ROW_HEIGHT,"CloseExampleFolio() = %x\n",err);
  if(err < 0)
    {
      char errstr[256];

      GetSysErr(errstr,sizeof(errstr),err);
      print(display,x,y+=ROW_HEIGHT,"%s\n",errstr);
      abort_err(err);
    }

  while(1);

  return 0;
}
