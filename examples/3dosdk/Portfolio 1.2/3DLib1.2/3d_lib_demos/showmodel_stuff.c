
/**************************************************************************
 * 3d_example1_stuff : the second exaple for usage of the 3dfolio.        *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 * And introducing in its first ever starring role : 3D-Folio!!           *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : March 1993                                                   *
 *                                                                        *
 **************************************************************************/
#include "audio.h"
#include "debug.h"
#include "event.h"
#include "folio.h"
#include "io.h"
#include "kernel.h"
#include "kernelnodes.h"
#include "list.h"
#include "mem.h"
#include "nodes.h"
#include "operamath.h"
#include "semaphore.h"
#include "stdio.h"
#include "stdlib.h"
#include "strings.h"
#include "task.h"
#include "types.h"

#include "3d_examples.h"
#include "3dlib.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "graphics.h"
#include "shapes.h"

#define HEAP_SIZE2 30000

struct WorldObject World_pyramid1;
struct WorldObject World_object1;

struct WorldObject World_camera1;
struct WorldPort World1;

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Cube_id;

extern char Model_file[50];

extern Item BitmapItems[];
extern long View_X, View_Y, Camera_angle;
extern long View_Z;
extern int32 View_pitch, View_yaw;

extern long Rotate_stuff;

/*** temporary storage area to hold the transformed points. Each point needs
 *** 12 bytes. ***/

char World1_pointheap[HEAP_SIZE2];

struct Render3DPort Rport;

extern CCB *Ccbs[]; /* ccb */

extern int32 Cull_backface;
extern int32 Show_outlines;

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-folio               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds ()
{
  ModelDefinition *newmodel;
  CCB *flatccb;

  static TagArg cubetags[] = { F3D_TAG_MODEL_FLAGS,
                               (void *)(FACE_SORTING | PSEUDO_PERSPECTIVE),
                               F3D_TAG_MODEL_MATRIX_METHOD,
                               ROTATE_XYZ,
                               F3D_TAG_MODEL_NORMALS,
                               (void *)TRUE,
                               F3D_TAG_MODEL_LIGHT_AMBIENT,
                               (void *)0,
                               F3D_TAG_MODEL_LIGHTS,
                               (void *)LIGHT_0,
                               F3D_TAG_DONE,
                               0 };

  static TagArg cameratags[]
      = { F3D_TAG_CAMERA_RADIUS, (void *)(1 << 3), F3D_TAG_DONE, 0 };

  InitGlasses (0, 0);

  /*** Init a new Render port ***/

  InitRender3DPort (&Rport, BitmapItems[0]);

  /*********************
   *** create world1 ***
   *********************/

  InitNewWorld (&World1, World1_pointheap, HEAP_SIZE2, NULL);

  newmodel = LoadModel (Model_file);

  if (!newmodel)
    {
      printf ("unable to load model %s\n", Model_file);
      getout (0);
    }

  InitModel (&World_object1, newmodel, cubetags);
  Cube_id = AddModel (&World1, &World_object1);

  if (Cull_backface)
    World_object1.wo_ObjectFlags |= CULL_BACKFACE;
  if (Show_outlines)
    World_object1.wo_ObjectFlags |= WIREFRAME;

  flatccb = CreateColorCCB (31, 20, 0);
  SetModelCCB (&World_object1, flatccb,
               CLONE_CCB_CONTROL); /* set all faces to the same image */

  InitCamera (&World_camera1, cameratags);
  Camera1_id = AddCamera (&World1, &World_camera1);

  View_Z = MulSF16 ((-2 << 16), World_object1.wo_SphericalRadius);

  SetObjectLocation (&World_camera1, 0, 0, View_Z);
}

/**************************************************************************
 * render_world : performs the rotations on the desired world and renders *
 *     it.                                                                *
 **************************************************************************/
void render_world (bitmapitem, cel_id, camera_id, world_id)

    long bitmapitem;
long cel_id;
long camera_id;
long world_id;
{
  WorldPort *wport;

  wport = &World1;

  Rport.rp_BitmapItem = bitmapitem; /* flips the buffers */

  /*** builds up the sorted list ***/

  if (Rotate_stuff)
    {
      move_things ();
    }

  Snapshot (wport, camera_id); /* perform the sorting, clipping, etc. */

  RenderWorld (&Rport, wport, camera_id);
}

/**************************************************************************
 * move_things :                                                          *
 **************************************************************************/
void
move_things ()
{
  /*** put the light twice the object's radius towards the right ***/

  World1.wp_LightPosition[0][X_COORD]
      = MulSF16 ((2 << 16), World_object1.wo_SphericalRadius);
  World1.wp_LightPosition[0][Y_COORD]
      = MulSF16 ((2 << 16), World_object1.wo_SphericalRadius);
  World1.wp_LightPosition[0][Z_COORD] = 0;

  /*** you could also set the wo_Orientation fields as needed and call
   *** ResolveObjectMatrix() or MakeCRotationMatrix() instead ***/

  /*** the cube tumbles around ***/

  RotateModel (&World_object1, View_pitch, View_yaw, 0);

  /*** the moving object rotates CW on it's Y axis ***/

  SetObjectLocation (&World_camera1, View_X, View_Y, View_Z);

  World_camera1.wo_WorldOrientation[Y_ANGLE] = Camera_angle;
}
