#include "display.hpp"

#include "crc32b.h"
#include "abort.h"

#include "celutils.h"

#include "mem.h"
#include "stdio.h"
#include "svc_mem.h"
#include "time.h"

#define ROM_SIZE   (1024 * 1024)
#define NVRAM_SIZE (32 * 1024)
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
main_read_rom()
{
  int x;
  int y;
  Err err;
  i32 offset;
  u8 *data;
  crc32b_t crc;
  Item svc_mem_dev;
  BasicDisplay display;

  display.clear();
  display.display();

  x = 16;
  y = 16;
  offset = 0;

  svc_mem_init();
  svc_mem_dev = svc_mem_open_device();
  if(svc_mem_dev < 0)
    {
      char errstr[256];
      GetSysErr(errstr,sizeof(errstr),svc_mem_dev);
      print(display,x,y+=ROW_HEIGHT,
            "%.04us: error opening svc_mem device\n",
            SampleSystemTime());
      print(display,x,y+=ROW_HEIGHT,
            "%.04us: %s\n",
            SampleSystemTime(),
            errstr);
      abort_err(svc_mem_dev);
    }

  {
    print(display,x,y+=ROW_HEIGHT,"%.04us: Allocating 1MB RAM\n",SampleSystemTime());
    data = (uint8*)AllocMem(ROM_SIZE,MEMTYPE_ANY);
    if(data == NULL)
      {
        const char errstr[] = "Failed to allocate memory for ROM\n";
        display.draw_text8(x,y+=ROW_HEIGHT,errstr);
        abort(errstr);
      }
  }

  y += ROW_HEIGHT;

  {
    print(display,x,y+=ROW_HEIGHT,"%.04us: Reading ROM1\n",SampleSystemTime());
    err = svc_mem_r_u32_rom1(svc_mem_dev,offset,(u32*)data,ROM_SIZE/sizeof(u32));
    if(err)
      {
        print(display,x,y+=ROW_HEIGHT,
              "%.04us: error reading ROM - \n",SampleSystemTime());
        printf("svc_mem_dev = %p\n",svc_mem_dev);
        abort_err(err);
      }
    print(display,x,y+=ROW_HEIGHT,"%.04us: finished reading ROM1\n",SampleSystemTime());

    print(display,x,y+=ROW_HEIGHT,"%.04us: Calculating ROM1 CRC32B\n",SampleSystemTime());
    crc = crc32b(data,ROM_SIZE);
    print(display,x,y+=ROW_HEIGHT,"%.04us: ROM1 CRC32B = %.08x\n",SampleSystemTime(),crc);
  }

  y += ROW_HEIGHT;

  {
    print(display,x,y+=ROW_HEIGHT,"%.04us: Reading ROM2\n",SampleSystemTime());
    err = svc_mem_r_u32_rom2(svc_mem_dev,offset,(u32*)data,ROM_SIZE/sizeof(u32));
    if(err)
      abort_err(err);
    print(display,x,y+=ROW_HEIGHT,"%.04us: finished reading ROM2\n",SampleSystemTime());

    print(display,x,y+=ROW_HEIGHT,"%.04us: Calculating ROM2 CRC32B\n",SampleSystemTime());
    crc = crc32b(data,ROM_SIZE);
    print(display,x,y+=ROW_HEIGHT,"%.04us: ROM2 CRC32B = %.08x\n",SampleSystemTime(),crc);
  }

  y += ROW_HEIGHT;

  {
    print(display,x,y+=ROW_HEIGHT,"%.04us: Reading NVRAM\n",SampleSystemTime());

    err = svc_mem_r_u8_nvram(svc_mem_dev,offset,data,NVRAM_SIZE);
    if(err)
      abort_err(err);
    print(display,x,y+=ROW_HEIGHT,"%.04us: finished reading NVRAM\n",SampleSystemTime());

    print(display,x,y+=ROW_HEIGHT,"%.04us: Calculating NVRAM CRC32B\n",SampleSystemTime());
    crc = crc32b(data,NVRAM_SIZE);
    print(display,x,y+=ROW_HEIGHT,"%.04us: NVRAM CRC32B = %.08x\n",SampleSystemTime(),crc);
  }

  svc_mem_close_device(svc_mem_dev);
  svc_mem_destroy();

  while(true);

  return 0;
}
