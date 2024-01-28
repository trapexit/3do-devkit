/**************************************************************************
 * 3d_example5_stuff : contains init and driver routines                  *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written by Mike Smithwick, October 1993.                               *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************/

#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "graphics.h"

#include "3d_examples.h"
#include "3dlib.h"
#include "shapes.h"

#define MAX_SW_OBJECTS 25
#define SIZE_ANGLE_ARRAY 7

//#define DO_CUBE_FIELD

#ifdef DO_CUBE_FIELD
#define MAX_DISPLAYABLE_OBJECTS 25

#define MAX_DISPLAYABLE_OBJECTS                                               \
  10 /* any more than 10 will cause audiofolio to crash !?!? */
#define USE_SOUND
#endif

struct WorldObject Light_cube;
struct WorldObject Anim_cube, Mesh_cube, *Cloned_cube;
CCB *Face0ccb, *Face2ccb, *Screencel;

struct WorldObject World_camera1;
struct WorldPort World1;

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Cube_id;

extern Item BitmapItems[];
extern long View_X, View_Y, View_Z, Velocity;
extern long View_roll, View_pitch, View_yaw;
extern long Delta_pitch, Delta_yaw, Delta_roll;
extern long Rotate_stuff;
extern ScreenContext Screencontext;
extern frac16 Object_yaw, Object_pitch;

/*** temporary storage area to hold the transformed points. Each point needs
 *** 12 bytes. ***/

char World1_pointheap[HEAP_SIZE];

/*** the Surfaces data is needed to hold the flag and ccb data when assigned
 *** to an object. ***/

struct ModelSurface Cube_surfaces[8];
struct ModelSurface Pyramid_surfaces[5];

struct Render3DPort Rport;

/*** this stuff is used by loadcel() ***/

extern CCB *Ccbs[]; /* ccb */

void ReportMemoryUsage (void);

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-folio               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds ()
{
  CCB *clipped_ccb;
  int32 i;

  static TagArg tagslites[]
      = { F3D_TAG_MODEL_LIGHT_AMBIENT,
          (void *)3,
          F3D_TAG_MODEL_LIGHTS,
          (void *)LIGHT_0,
          F3D_TAG_MODEL_NORMALS,
          (void *)TRUE,
          F3D_TAG_MODEL_FLAGS,
          (void *)(CULL_BACKFACE | CAMERA_PERSPECTIVE | FACE_CULLING3),
          F3D_TAG_MODEL_RADIUS,
          (void *)(0),
          F3D_TAG_DONE,
          0 };

  static TagArg cameratags[] = { F3D_TAG_CAMERA_MATRIX_METHOD,
                                 (void *)FLYTHROUGH_ROTATE,
                                 F3D_TAG_CAMERA_CAMERALENS,
                                 (void *)24,
                                 F3D_TAG_CAMERA_FIELDSCALE,
                                 (void *)350,
                                 F3D_TAG_DONE,
                                 0 };

  static TagArg worldtags[]
      = { F3D_TAG_FACE_ORIENTATION, (void *)CW_FACES, F3D_TAG_DONE, 0 };

  /*** Init a new Render port ***/

  InitRender3DPort (&Rport, BitmapItems[0]);

  /*********************
   *** create world1 ***
   *********************/

  InitNewWorld (&World1, World1_pointheap, HEAP_SIZE, worldtags);

  /*** create the cube with the animated faces ***/

  InitModel (&Anim_cube, &Square, tagslites);
  AddModel (&World1, &Anim_cube);
  SetModelCCB (&Anim_cube, Ccbs[1], CLONE_CCB_CONTROL);
  SetObjectLocation (
      &Anim_cube, -1 << 16, 0,
      5 << 16); // place it behind and to the right of other cube

  clipped_ccb = ClipCel (Ccbs[0], 0, 0, 64, 64); // get a clipped cel

  Face0ccb = GetFaceCCB (&Anim_cube, 0, 0,
                         NO_FLAGS); // get the ccbs for faces 0 and 2,
  Face2ccb
      = GetFaceCCB (&Anim_cube, 0, 2, NO_FLAGS); // used for later animation

  /*** create the mesh cube ***/

  InitModel (&Mesh_cube, &Cube, tagslites);

  SetModelCCB (&Mesh_cube, Ccbs[0], CLONE_CCB_CONTROL);
  SetObjectLocation (&Mesh_cube, 0, 0,
                     0); // place it behind and to the right of other cube
  CreateIntMeshDefinition (&Mesh_cube, 0, 0, (2 << 16), 4, 4, NULL);

  AddModel (&World1, &Mesh_cube);

  /*** create the camera ***/

  InitCamera (&World_camera1, cameratags);
  Camera1_id = AddCamera (&World1, &World_camera1);

  SetObjectLocation (&World_camera1, 0, 0, -8 << 16);
  World_camera1.wo_SphericalRadius = Convert32_F16 (3);
}

/**************************************************************************
 * move_things :                                                          *
 **************************************************************************/
void
move_things ()
{
  static frac16 angle1, angle2 = 0, angle3 = 0;

  angle1 += (3 << 16);

  World1.wp_LightPosition[0][X_COORD] = -5 << 16;
  World1.wp_LightPosition[0][Y_COORD] = 0;
  World1.wp_LightPosition[0][Z_COORD] = -15 << 16;

  angle2 += (1 << 15);
  ;
  angle3 += (1 << 15);
  ;
  RotateModel (&Anim_cube, angle3, angle2, 0);

  World_camera1.wo_ObjectVelocity = -Velocity;

  /*  use this stuff with matrix method of FLYTHROUGH_ROTATE
   */
  World_camera1.wo_WorldOrientation[PITCH] = Delta_pitch;
  World_camera1.wo_WorldOrientation[YAW] = Delta_yaw;
  World_camera1.wo_WorldOrientation[ROLL] = Delta_roll;
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

  /*** use this stuff to animate the face ***/

  static int32 x = 64, y = 0, w = 64;
  static delta_w = -1;

  ENTER ("render_world");

  wport = &World1;

  Rport.rp_BitmapItem = bitmapitem; /* flips the buffers */

  /*** builds up the sorted list ***/

  if (Rotate_stuff)
    {
      move_things ();
    }

  x++;
  y++;

  w += delta_w; /* delta_w causes the animated image to stretch */

  if (w > 64)
    delta_w = -1;
  else if (w < 24)
    delta_w += 1;

  if (x >= 64)
    {
      x = 0;
      y = 0;
    }

  ScrollCel (Face0ccb, Ccbs[1], x, y, w, w); /* scroll faces 0 and 2 */
  ScrollCel (Face2ccb, Ccbs[1], x, y, w, w);

  /*** the matrix method is NO_ROTATE so we can build our own matrix. Create
   *the
   *** new worldmatrix based on the delta angles instead of the actual
   *** total angles. The direction vector is solved in RotateObject. ***/

  Snapshot (wport, camera_id);

  World_camera1.wo_WorldPosition[X_COORD]
      -= World_camera1.wo_ObjectDirection[X_COORD];
  World_camera1.wo_WorldPosition[Y_COORD]
      -= World_camera1.wo_ObjectDirection[Y_COORD];
  World_camera1.wo_WorldPosition[Z_COORD]
      -= World_camera1.wo_ObjectDirection[Z_COORD];

  RenderWorld (&Rport, wport, camera_id);

  EXIT ("render_world");
}
