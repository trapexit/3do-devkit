#include "display.hpp"

#include "celutils.h"
#include "debug.h"
#include "graphics.h"
#include "hardware.h"
#include "io.h"
#include "item.h"
#include "mem.h"
#include "operamath.h"
#include "stdio.h"
#include "string.h"

static
void
ZoomRotateCel(CCB    *ccb_,
              frac16  x_,
              frac16  y_,
              frac16  zoom_,
              frac16  angle_)
{
  int hdx;
  int hdy;
  int vdx;
  int vdy;

  hdx = MulSF16(CosF16(angle_),zoom_);
  hdy = MulSF16(SinF16(angle_),zoom_);
  vdx = -hdy;
  vdy =  hdx;

  ccb_->ccb_HDX = hdx << 4;
  ccb_->ccb_HDY = hdy << 4;
  ccb_->ccb_VDX = vdx;
  ccb_->ccb_VDY = vdy;

  hdx = MulSF16(hdx,Convert32_F16(ccb_->ccb_Width) >> 1);
  hdy = MulSF16(hdy,Convert32_F16(ccb_->ccb_Width) >> 1);
  vdx = MulSF16(vdx,Convert32_F16(ccb_->ccb_Height) >> 1);
  vdy = MulSF16(vdy,Convert32_F16(ccb_->ccb_Height) >> 1);

  ccb_->ccb_XPos = x_ - hdx - vdx;
  ccb_->ccb_YPos = y_ - hdy - vdy;
}

static
bool
clamp(const frac16  min_,
      const frac16  max_,
      frac16       &val_)
{
  if(val_ > max_)
    {
      val_ = max_;
      return true;
    }

  if(val_ < min_)
    {
      val_ = min_;
      return true;
    }

  return false;
}

int
main_cel_rotation()
{
  CCB *logo;
  BasicDisplay display;

  OpenMathFolio();

  logo = LoadCel("art/3do_logo_unpacked.cel",MEMTYPE_CEL);

  display.clear();
  display.swap();

  frac16       x        = display.sc->sc_BitmapWidth >> 1;
  frac16       y        = display.sc->sc_BitmapHeight >> 1;
  frac16       angle    = 0;
  frac16       zoom     = 0;
  frac16       dzoom    = DivSF16(Convert32_F16(1),Convert32_F16(200));
  const frac16 max_zoom = Convert32_F16(3);
  const frac16 min_zoom = (Convert32_F16(1) >> 6);

  while(true)
    {
      ZoomRotateCel(logo,x,y,zoom,angle);
      angle += Convert32_F16(1);
      zoom  += dzoom;
      if(clamp(min_zoom,max_zoom,zoom))
        {
          dzoom = -dzoom;
          x = Convert32_F16(ReadHardwareRandomNumber() % display.sc->sc_BitmapWidth);
          y = Convert32_F16(ReadHardwareRandomNumber() % display.sc->sc_BitmapHeight);
          kprintf("x,y: %d, %d\n",
                  ConvertF16_32(x),
                  ConvertF16_32(y));
        }

      display.clear();
      display.draw_cels(logo);
      display.draw_printf(16,16,"x: %d",ConvertF16_32(x));
      display.draw_printf(16,24,"y: %d",ConvertF16_32(y));
      display.display_and_swap();

      display.waitvbl();
    }

  UnloadCel(logo);

  return 0;
}
