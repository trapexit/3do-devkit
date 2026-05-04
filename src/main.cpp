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
#include "filefunctions.h"
#include "item.h"
#include "task.h"

typedef struct Example
{
  const char *name;
  const char *path;
} Example;

static
int
run_example(const Example &example)
{
  Item task;

  task = LoadProgram((char*)example.path);
  if(task < 0)
    abort_err(task);

  while(LookupItem(task) != NULL)
    WaitSignal(SIGF_DEADTASK);

  return 0;
}

static const Example examples[] =
  {
    { "Cel rotation",      "$boot/cel_rotation" },
    { "3D 3DO logo",       "$boot/3d_3do_logo" },
    { "Rotating cube",     "$boot/rotating_cube" },
    { "Bounce",            "$boot/bounce" },
    { "Orbit",             "$boot/orbit" },
    { "Color Echo",        "$boot/colorecho" },
    { "24-bit Slide Show", "$boot/slide_show_24bit" },
    { "AA Player",         "$boot/aaplayer" },
    { "LR Extract",        "$boot/lrex" }
  };

static const int example_count = sizeof(examples) / sizeof(examples[0]);
static const int visible_example_count = 10;
static const int scroll_marker_x = 48;
static const int scroll_down_marker_y = 224;
static const char *scroll_down_marker = "v";

static
void
draw_menu(BasicDisplay &display,
          int           selected,
          int           top)
{
  int i;
  int count;

  display.clear();
  display.draw_text8(48, 36, "Select an example");
  display.draw_text8(48, 48, "Up/Down moves, A or Start runs");
  display.draw_text8(48, 60, "X to return to this menu");

  count = example_count - top;
  if(count > visible_example_count)
    count = visible_example_count;

  for(i = 0; i < count; i++)
    {
      const int index = top + i;
      const int y = 84 + (i * 14);

      display.draw_text8(48, y, (index == selected) ? "*" : " ");
      display.draw_text8(64, y, examples[index].name);
    }

  if((top + count) < example_count)
    display.draw_text8(scroll_marker_x, scroll_down_marker_y,
                       scroll_down_marker);

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
  int top;

  err = InitControlPad(1);
  if(err < 0)
    abort_err(err);

  display.clear();
  display.swap();
  top = selected - visible_example_count + 1;
  if(top < 0)
    top = 0;

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

      if(selected < top)
        top = selected;
      else if(selected >= (top + visible_example_count))
        top = selected - visible_example_count + 1;

      draw_menu(display, selected, top);

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
      run_example(examples[selected]);
    }

  return 0;
}
