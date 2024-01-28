
/**************************************************************************
 * 3d_example1_stuff : the core code for the 3d demonstration             *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : March 1993                                                   *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************/
#include "3d_examples.h"
#include "3dlib.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "filestream.h"
#include "filestreamfunctions.h"
#include "graphics.h"
#include "shapes.h"

struct WorldObject World_pyramid1;
struct WorldObject World_cube1;

struct WorldObject World_camera1;
struct WorldPort World1;

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Cube_id;

extern Item BitmapItems[];
extern int32 View_X, View_Y, Camera_angle;
extern int32 Rotate_stuff;

/*** temporary storage area to hold the transformed points. Each point needs
 *** 12 bytes. ***/

char World1_pointheap[HEAP_SIZE];

struct Render3DPort Rport;

extern CCB *Ccbs[]; /* ccb */

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-lib               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds ()
{
  static TagArg cubetags[] = { F3D_TAG_MODEL_FLAGS,
                               (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
                               F3D_TAG_MODEL_MATRIX_METHOD,
                               (void *)ROTATE_YXZ,
                               F3D_TAG_DONE,
                               0 };

  static TagArg pyramidtags[]
      = { F3D_TAG_MODEL_FLAGS, (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
          F3D_TAG_DONE, 0 };

  static TagArg cameratags[]
      = { F3D_TAG_CAMERA_RADIUS, (void *)(1 << 3), F3D_TAG_DONE, 0 };

  static TagArg worldtags[]
      = { F3D_TAG_FACE_ORIENTATION, (void *)CW_FACES, F3D_TAG_DONE, 0 };

  /*** Init a new Render port ***/

  InitRender3DPort (&Rport, BitmapItems[0]);

  /*********************
   *** create world1 ***
   *********************/

  InitNewWorld (&World1, World1_pointheap, HEAP_SIZE, worldtags);

  /*** create the central cube ***/

  InitModel (&World_cube1, &Cube, cubetags);
  Cube_id = AddModel (&World1, &World_cube1);

  SetModelCCB (&World_cube1, Ccbs[0],
               SHARE_CCB); /* set all faces to the same image */

  /*** create the orbiting pyramid ***/

  InitModel (&World_pyramid1, &Pyramid, pyramidtags);
  AddModel (&World1, &World_pyramid1);
  SetModelCCB (&World_pyramid1, Ccbs[0], SHARE_CCB);
  SetObjectLocation (&World_pyramid1, 0, 0, Convert32_F16 (3));

  InitCamera (&World_camera1, cameratags);
  Camera1_id = AddCamera (&World1, &World_camera1);

  SetObjectLocation (&World_camera1, 0, 0, -Convert32_F16 (4));
}

/**************************************************************************
 * render_world : performs the rotations on the desired world and renders *
 *     it.                                                                *
 **************************************************************************/
void render_world (bitmapitem, cel_id, camera_id, world_id)

    int32 bitmapitem;
int32 cel_id;
int32 camera_id;
int32 world_id;
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
  static int32 angle1 = 140, angle2 = 0, angle3 = 0, angle4 = 0;
  frac16 tempf16;

  /*** this rotates the camera for now so I can find the object, rotates around
   *** the Y axis so it pans 20 degrees at a time when I press the B button. I
   *** stop it by pressing it again. ***/

  /*** the units are the operamath angles, where 1 unit=1/256th of a circle,
   *** or about 1.4 degrees ***/

  angle2 += (1 << 16);
  angle2 %= (256 << 16);

  angle3 += (2 << 16);
  angle3 %= (256 << 16);

  angle4 += 48000; /* ~ 1.5 units */
  angle4 %= (256 << 16);

  /*** update the position of the pyramid, make it move around the cube at
   *** a radius of 3 ***/

  angle1 += (2 << 16);

  tempf16 = SinF16 (angle1);
  World_pyramid1.wo_WorldPosition[X_COORD] = MulSF16 (tempf16, (3 << 16));

  tempf16 = CosF16 (angle1);
  World_pyramid1.wo_WorldPosition[Z_COORD] = MulSF16 (tempf16, (3 << 16));

  /*** you could also set the wo_Orientation fields as needed and call
   *** ResolveObjectMatrix() or MakeCRotationMatrix() instead ***/

  /*** the cube tumbles around ***/

  RotateModel (&World_cube1, angle2, angle2, angle3);

  /*** the moving object rotates CW on it's Y axis ***/

  RotateModel (&World_cube1, 0, angle2, 0);
  SetObjectLocation (&World_camera1, View_X, View_Y,
                     World_camera1.wo_WorldPosition[Z_COORD]);

  World_camera1.wo_WorldOrientation[Y_ANGLE] = Camera_angle;
}
