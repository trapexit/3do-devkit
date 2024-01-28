
/**************************************************************************
 * 3d_example2_stuff : contains the init and driver code                  *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : April 1993                                                   *
 *                                                                        *
 **************************************************************************
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 **************************************************************************/
#include "3d_examples.h"
#include "3dlib.h"
#include "graphics.h"
#include "shapes.h"

struct WorldObject World_pyramid1;
struct WorldObject World_cube1;
struct WorldObject World_cube2;
struct WorldObject World_pyramid2;
struct WorldObject World_pyramid3;
struct WorldObject World_pyramid4;

struct WorldObject World_camera1;
struct WorldObject World_camera2;
struct WorldObject World_camera3;
struct WorldPort World1;
struct WorldPort World2;

int32 World_cube2_id;

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Camera2_id; /* handle to my returned camera */
Obj3D Camera3_id; /* handle to my returned camera */

extern Item BitmapItems[];
extern int32 View_X, View_Y, Camera_angle;

/*** temporary storage area to hold the transformed points. Each point needs
 *** 12 bytes. ***/

char World1_pointheap[HEAP_SIZE];
char World2_pointheap[HEAP_SIZE];

struct Render3DPort Rport;

void my_rotate_object (void);

extern CCB *Ccbs[]; /* ccb */

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-lib               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds ()
{
  static TagArg cubetags[]
      = { F3D_TAG_MODEL_FLAGS, (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
          F3D_TAG_DONE, 0 };

  static TagArg pyramidtags[]
      = { F3D_TAG_MODEL_FLAGS, (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
          F3D_TAG_DONE, 0 };

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
  AddModel (&World1, &World_cube1);

  SetModelCCB (&World_cube1, Ccbs[0],
               SHARE_CCB); /* set all faces to the same image */
  SetFaceCCB (&World_cube1, 0, 1, Ccbs[1],
              SHARE_CCB); /* set face 1 to another image */

  /*** create the pyramid/cube object which wheels around the
   *** central cube. ***/

  InitModel (&World_pyramid1, &Pyramid_cube, pyramidtags);
  AddModel (&World1, &World_pyramid1);
  SetModelCCB (&World_pyramid1, Ccbs[0], SHARE_CCB);

  SetObjectLocation (&World_pyramid1, 0, 0, Convert32_F16 (5));

  /*** create the distant pyramid ***/

  InitModel (&World_pyramid2, &Pyramid, pyramidtags);
  AddModel (&World1, &World_pyramid2);

  SetModelCCB (&World_pyramid2, Ccbs[0], SHARE_CCB);
  SetObjectLocation (&World_pyramid2, 0, 0, Convert32_F16 (5));

  InitCamera (&World_camera1, NULL);
  Camera1_id = AddCamera (&World1, &World_camera1);
  SetObjectLocation (&World_camera1, 0, 0, -Convert32_F16 (5));

  InitCamera (&World_camera2, NULL);
  Camera2_id = AddCamera (&World1, &World_camera2);
  SetObjectLocation (&World_camera2, -Convert32_F16 (10), 0,
                     Convert32_F16 (8));

  /*********************
   *** create world2 ***
   *********************/

  InitNewWorld (&World2, World2_pointheap, 1000, worldtags);

  InitModel (&World_pyramid3, &Pyramid, pyramidtags);
  SetModelCCB (&World_pyramid3, Ccbs[0], SHARE_CCB);
  AddModel (&World2, &World_pyramid3);

  InitCamera (&World_camera3, NULL);
  Camera3_id = AddCamera (&World2, &World_camera3);

  if (Camera3_id < 0)
    getout (0);

  World_camera3.wo_WorldPosition[Z_COORD] = -Convert32_F16 (6);
  SetObjectLocation (&World_camera3, 0, 0, -Convert32_F16 (6));
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

  if (world_id == 0)
    wport = &World1;
  else
    wport = &World2;

  Rport.rp_BitmapItem = bitmapitem; /* flips the buffers */

  move_things ();

  Snapshot (wport, camera_id);

  RenderWorld (&Rport, wport, camera_id);
}

/**************************************************************************
 * move_things :                                                          *
 **************************************************************************/
void
move_things (void)
{
  static int32 angle1 = 140, angle2 = 0, angle3 = 0, angle4 = 0;
  frac16 tempf16;

  /*** generate some different angles ***/

  angle2 += (1 << 16);
  angle2 %= (256 << 16);

  angle3 += (2 << 16);
  angle3 %= (256 << 16);

  angle4 += 48000; /* ~ 1.5 units */
  angle4 %= (256 << 16);

  /*** update the position of the pyramid, make it move around the cube ***/

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

  RotateModel (&World_pyramid1, angle2, angle2, angle3);

  /*** the stationary pyramid rotates on it's Y axis in a CCW direction ***/

  RotateModel (&World_pyramid2, angle2, angle2, angle3);

  /*** the pyramid in World 2 tumbles ***/

  RotateModel (&World_pyramid3, angle2, angle2, angle3);

  SetObjectLocation (&World_camera1, View_X, View_Y,
                     World_camera1.wo_WorldPosition[Z_COORD]);

  World_camera1.wo_WorldOrientation[Y_ANGLE] = Camera_angle;

  /*** the camera's matrix is built up via MakeCRotationMatrix() as called
   *** by ResolveObjectMatrix ***/

  World_camera2.wo_WorldOrientation[Y_ANGLE] = (82 << 16);
}
