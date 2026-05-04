/*
  ISC License

  Copyright (c) 2022, Antonio SJ Musumeci <trapexit@spawn.link>

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

#include "abort.h"

#include "debug.h"
#include "operror.h"
#include "stdlib.h"
#include "types.h"

void
__ABORT__(const char *module_,
          const int   line_,
          const char *str_)
{
  kprintf("abort @ %s:%d - %s\n",module_,line_,str_);
  while(1);
}

void
__ABORT_ERR__(const char *module_,
              const int   line_,
              const Err   err_)
{
  char errstr[256];

  GetSysErr(errstr,sizeof(errstr),err_);

  __ABORT__(module_,line_,errstr);
}
