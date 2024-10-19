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

#include "svc_mem.h"

#include "abort.h"

#include "stdio.h"
#include "operamath.h"
#include "operror.h"

#define ROW_HEIGHT 8

#define MULUF16_OFFSET -1
#define CROSS3_F16_OFFSET (-8 + -15)

ufrac16
my_muluf16(ufrac16 m1_,
           ufrac16 m2_)
{
  (void)m1_;
  (void)m2_;

  return 0x0BADF00D;
}


void
my_cross3_f16(void *a_,
              void *b_,
              void *c_)
{
  (void)b_;
  (void)c_;

  *(int*)a_ = 0xDEADBEEF;
}

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
  int x;
  int y;
  i32 v;
  u32 *dst;
  Err err;
  Item svc_mem_dev;
  Item folio_item;
  Folio *folio_ptr;
  BasicDisplay display;

  display.clear();
  display.display();

  x = 16;
  y = 16;

  OpenMathFolio();

  err = svc_mem_init();
  print(display,x,y+=ROW_HEIGHT,"svc_mem_init = %p\n",err);
  if(err < 0)
    abort_err(err);

  svc_mem_dev = svc_mem_open_device();
  print(display,x,y+=ROW_HEIGHT,"svc_mem_dev = %x\n",svc_mem_dev);
  if(svc_mem_dev < 0)
    abort_err(svc_mem_dev);

  folio_item = FindFolio("Operamath");
  print(display,x,y+=ROW_HEIGHT,"operamath item = %p\n",folio_item);
  folio_ptr = (Folio*)LookupItem(folio_item);
  print(display,x,y+=ROW_HEIGHT,"operamath ptr = %p\n",folio_ptr);

  print(display,x,y+=ROW_HEIGHT,"folio->f_MaxUserFunctions = %d\n",
        folio_ptr->f_MaxUserFunctions);
  print(display,x,y+=ROW_HEIGHT,"folio->f_MaxSwiFunctions = %d\n",
        folio_ptr->f_MaxSwiFunctions);

  err = svc_mem_r_u32(svc_mem_dev,
                      (u32*)folio_ptr,
                      CROSS3_F16_OFFSET,
                      (u32*)&dst,
                      1);
  if(err < 0)
    abort_err(err);
  print(display,x,y+=ROW_HEIGHT,"Cross3_F16 = %p\n",dst);

  err = svc_mem_r_u32(svc_mem_dev,
                      (u32*)folio_ptr,
                      MULUF16_OFFSET,
                      (u32*)&dst,
                      1);
  if(err < 0)
    abort_err(err);
  print(display,x,y+=ROW_HEIGHT,"MulUF16 = %p\n",dst);


  // Overwrite function pointers in Operamath folio
  err = svc_mem_w1_u32(svc_mem_dev,
                       (u32)my_cross3_f16,
                       (u32*)folio_ptr,
                       CROSS3_F16_OFFSET);
  if(err < 0)
    abort_err(err);
  print(display,x,y+=ROW_HEIGHT,"Overwrote Cross3_F16 SWI = %p\n",my_cross3_f16);

  err = svc_mem_w1_u32(svc_mem_dev,
                       (u32)my_muluf16,
                       (u32*)folio_ptr,
                       MULUF16_OFFSET);
  if(err < 0)
    abort_err(err);
  print(display,x,y+=ROW_HEIGHT,"Overwrote MulUF16 = %p\n",my_muluf16);

  v = MulUF16(0,0);
  if(v != 0x0BADF00D)
    abort("Failed to overwrite MulUF16\n");
  print(display,x,y+=ROW_HEIGHT,"MulUF16() = %p\n",v);

  Cross3_F16(&v,&v,&v);
  if(v != 0xDEADBEEF)
    abort("Failed to overwite Cross3_F16\n");
  print(display,x,y+=ROW_HEIGHT,"Cross3_F16() = %p\n",v);

  print(display,x,y+=ROW_HEIGHT,"Everything succeeded\n");
  
  while(1);

  return 0;
}
