/**************************************************************************
 * perftest.h : header file used by the perftest example                  *
 **************************************************************************/

#define NO_ERROR 0L
#define GENERIC_ERROR 1L
#define SOLID_CEL 0x1f001f00
#define TRANSLUCENT_CEL 0x1f811f81

#define YES 1
#define NO 0

#define START_BUTTON 1

#define LO_RES_IMAGE 1 /* use lo-res face image */
#define HI_RES_IMAGE 0 /* use hi-res face image */

#define HEAP_SIZE 30000 /* should handle 100 cubes, plus */

void init_system (void);
void init_worlds (void);
int32 process_joystick (void);
void render_world (Item bitmapitem, long cel_id, long camera_id,
                   long world_id);
void getout (int32 rvalue);
