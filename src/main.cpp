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

#include "abort.h"
#include "display.hpp"

#include "controlpad.h"
#include "debug.h"
#include "event.h"

extern     int main_cel_rotation();
extern "C" int main_3d_3do_logo();
extern "C" int main_rotating_cube();

typedef int (*ExampleMain)(void);

typedef struct Example
{
  const char  *name;
  ExampleMain  entry;
} Example;

static
int
run_cel_rotation(void)
{
  return main_cel_rotation();
}

static
int
run_3d_3do_logo(void)
{
  return main_3d_3do_logo();
}

static
int
run_rotating_cube(void)
{
  return main_rotating_cube();
}

static const Example examples[] =
  {
    { "Cel rotation",      run_cel_rotation },
    { "3D 3DO logo",      run_3d_3do_logo },
    { "Rotating cube",    run_rotating_cube }
  };

static const int example_count = sizeof(examples) / sizeof(examples[0]);

static
void
draw_menu(BasicDisplay &display,
          int           selected)
{
  int i;

  display.clear();
  display.draw_text8(48, 36, "Select an example");
  display.draw_text8(48, 52, "Up/Down moves, A or Start runs");

  for(i = 0; i < example_count; i++)
    {
      const int y = 84 + (i * 14);

      display.draw_text8(48, y, (i == selected) ? "*" : " ");
      display.draw_text8(64, y, examples[i].name);
    }

  display.display_and_swap();
}

static
void
wait_control_pad_release(BasicDisplay &display)
{
  Err err;
  uint32 buttons;

  do
    {
      buttons = 0;
      err = DoControlPad(1, &buttons, ControlUp | ControlDown | ControlA | ControlStart);
      if(err < 0)
        abort_err(err);

      display.waitvbl();
    }
  while(buttons != 0);
}

static
int
select_example(int selected)
{
  BasicDisplay display;
  Err err;

  err = InitControlPad(1);
  if(err < 0)
    abort_err(err);

  display.clear();
  display.swap();

  while(true)
    {
      uint32 buttons;

      buttons = 0;
      err = DoControlPad(1, &buttons, 0);
      if(err < 0)
        abort_err(err);

      if(buttons & ControlUp)
        selected = (selected + example_count - 1) % example_count;
      else if(buttons & ControlDown)
        selected = (selected + 1) % example_count;

      draw_menu(display, selected);

      if(buttons & (ControlA | ControlStart))
        break;

      display.waitvbl();
    }

  wait_control_pad_release(display);
  KillControlPad();

  return selected;
}

int
main(int   argc_,
     char *argv_)
{
  (void)argc_;
  (void)argv_;

  int selected = 0;

  while(true)
    {
      selected = select_example(selected);
      kprintf("launching example: %s\n", examples[selected].name);
      examples[selected].entry();
    }

  return 0;
}
