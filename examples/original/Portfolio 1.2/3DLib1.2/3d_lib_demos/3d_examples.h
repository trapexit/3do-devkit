/**************************************************************************
 * 3d_examples.h : header file used by the 3d-lib examples              *
 **************************************************************************/

#ifndef _RW_DEFS_H
#define _RW_DEFS_H

#define NO_ERROR 0L
#define GENERIC_ERROR 1L
#define FADE_FRAMECOUNT 48

#define HEAP_SIZE 12000

#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

#define START_BUTTON 1

/*** macros ***/

#define MAKETAG(a, b, c, d)                                                   \
  (((int32)(a) << 24) | ((int32)(b) << 16) | ((int32)(c) << 8) | (int32)(d))

void close_everything (void);
void copy_back_pic (Bitmap *bitmap);
void fade_to_black (int32 frames);
void getout (long rvalue);
bool init_backpic (char *filename);
bool init_cels (void);
bool init_graphics (void);
void init_stuff (void);
void init_system (void);
long main_loop (void);
void move_things (void);
bool open_mac_link (void);
bool open_SPORT (void);
void process_args (long argc, char *argv[]);
void loadbackground (char *filename);
int32 openmacdevice (void);
long get_joystick_state (void);
long process_joystick (void);
void render_object (long screen, long cel_id, long camera_id);
void init_worlds (void);
void render_world (long screen_id, long cel_id, long camera_id, long world_id);

#endif
