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

#pragma once

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

Err svc_mem_init(void);
Err svc_mem_destroy(void);

Item svc_mem_open_device(void);
Err  svc_mem_close_device(Item device);

Item svc_mem_create_ioreq(Item device);

Err svc_mem_r_u8(Item device, u8 *src, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32(Item device, u32 *src, i32 offset, u32 *dst, i32 len);

Err svc_mem_r_u8_unit(Item device, u8 unit, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_unit(Item device, u8 unit, i32 offset, u32 *dst, i32 len);

Err svc_mem_r_u8_dram(Item device, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_dram(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u8_vram(Item device, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_vram(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u8_rom1(Item device, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_rom1(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u8_rom2(Item device, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_rom2(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u8_nvram(Item device, i32 offset, u8 *dst, i32 len);
Err svc_mem_r_u32_madam(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u32_clio(Item device, i32 offset, u32 *dst, i32 len);
Err svc_mem_r_u32_sport(Item device, i32 offset, u32 *dst, i32 len);

Err svc_mem_w_u8(Item device, u8 *src, i32 len, u8 *dst, i32 offset);
Err svc_mem_w_u32(Item device, u32 *src, i32 len, u32 *dst, i32 offset);

Err svc_mem_w1_u32(Item device, u32 src, u32 *dst, i32 offset);

Err svc_mem_w_u8_unit(Item device, u8 *src, i32 len, u8 unit, i32 offset);
Err svc_mem_w_u32_unit(Item device, u32 *src, i32 len, u8 unit, i32 offset);

Err svc_mem_w_u8_dram(Item device, u8 *src, i32 len, i32 offset);
Err svc_mem_w_u32_dram(Item device, u32 *src, i32 len, i32 offset);
Err svc_mem_w_u8_vram(Item device, u8 *src, i32 len, i32 offset);
Err svc_mem_w_u32_vram(Item device, u32 *src, i32 len, i32 offset);
Err svc_mem_w_u8_nvram(Item device, u8 *src, i32 len, i32 offset);
Err svc_mem_w_u32_madam(Item device, u32 *src, i32 len, i32 offset);
Err svc_mem_w_u32_clio(Item device, u32 *src, i32 len, i32 offset);
Err svc_mem_w_u32_sport(Item device, u32 *src, i32 len, i32 offset);

#ifdef __cplusplus
}
#endif
