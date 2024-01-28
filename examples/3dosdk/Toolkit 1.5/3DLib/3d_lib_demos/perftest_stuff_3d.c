/**************************************************************************
 * perftest_stuff :                                                       *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        * *
 **************************************************************************
 *                                                                        *
 * Written : May 1993                                                     *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************/

#include "3dlib.h"
#include "graphics.h"
#include "perftest_3d.h"
#include "perftest_shapes_3d.h"

#define OFF FALSE
#define ON TRUE

#define MAX_SW_OBJECTS 25
#define SIZE_ANGLE_ARRAY 7

#define DO_CUBE_FIELD

#define MAX_DISPLAYABLE_OBJECTS 25

#define WHITE_CUBE 0

WorldObject *Objects[MAX_SW_OBJECTS];

struct WorldObject Root_cube;
struct WorldObject *Image_root_cube = &Root_cube;

struct WorldObject Root_pyramid;

struct WorldObject World_camera1;
struct WorldPort World1;

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Cube_id;

extern Item BitmapItems[];
extern int32 View_X, View_Y, View_Z, Velocity;
extern int32 View_roll, View_pitch, View_yaw;
extern int32 Delta_pitch, Delta_yaw, Delta_roll;
extern int32 Rotate_stuff;

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

struct Cube_props
{
  int32 iroll; /* indices into the delta-angle array */
  int32 ipitch;
  int32 iyaw;
  frac16 roll; /* total angles */
  frac16 pitch;
  frac16 yaw;
  frac16 x, y, z;
  int32 version; /* which image is being used?? */
};

int32 Rotation_state = OFF;
int32 Lite_state = OFF;
int32 Transparency_state = OFF;
int32 Image_state = OFF;
int32 Small_image_state = OFF;
int32 State_changed = TRUE;
int32 State_cycle = 0;
CCB *Color_ccb[8];

struct Cube_props Cube_init[MAX_SW_OBJECTS];

/**************************************************************************
 * initcubeprops : initializes the position and rotations of the cubes    *
 **************************************************************************/
void
initcubeprops ()
{
  int32 i;
  static int32 roll_index = 0, pitch_index = 0, yaw_index = 0;

  for (i = 0; i < MAX_SW_OBJECTS; i++)
    {
      roll_index %= SIZE_ANGLE_ARRAY;
      pitch_index %= SIZE_ANGLE_ARRAY;
      yaw_index %= SIZE_ANGLE_ARRAY;

      Cube_init[i].iroll = roll_index;
      Cube_init[i].ipitch = pitch_index;
      Cube_init[i].iyaw = yaw_index;

      Cube_init[i].roll = 0;
      Cube_init[i].pitch = 0;
      Cube_init[i].yaw = 0;

      roll_index += 1; /* odd numbers ensure getting all values */
      pitch_index += 3;
      yaw_index += 5;

      Cube_init[i].version = HI_RES_IMAGE;
    }
}

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-folio               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds (void)
{
  int32 i, j;
  int32 object_counter = 0;
  Err error;

  static TagArg tagsnolites[]
      = { F3D_TAG_MODEL_FLAGS, (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
          F3D_TAG_DONE, 0 };

  static TagArg tagslites[]
      = { F3D_TAG_MODEL_LIGHT_AMBIENT,
          (void *)3,
          F3D_TAG_MODEL_LIGHTS,
          (void *)NO_LIGHTS,
          F3D_TAG_MODEL_NORMALS,
          (void *)TRUE,
          F3D_TAG_MODEL_FLAGS,
          (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE | FAST_OBJECT),
          F3D_TAG_MODEL_RADIUS,
          (void *)(1 << 16),
          F3D_TAG_DONE,
          0 };

  static TagArg cameratags[] = { F3D_TAG_CAMERA_MATRIX_METHOD,
                                 (void *)FLYTHROUGH_ROTATE,
                                 F3D_TAG_CAMERA_CAMERALENS,
                                 (void *)32,
                                 F3D_TAG_CAMERA_FIELDSCALE,
                                 (void *)350,
                                 F3D_TAG_DONE,
                                 0 };

  static TagArg worldtags[]
      = { F3D_TAG_EYE_SEP, (void *)(6 << 12), F3D_TAG_DONE, 0 };

  error = InitGlasses (0, 0);

  if (error < 0)
    getout (0);

  /*** Init a new Render port ***/

  InitRender3DPort (&Rport, BitmapItems[0]);

  /*********************
   *** create world1 ***
   *********************/

  InitNewWorld (&World1, World1_pointheap, HEAP_SIZE, worldtags);
  initcubeprops ();

  InitModel (Image_root_cube, &Cube, tagslites);

  SetModelCCB (Image_root_cube, CreateColorCCB (31, 0, 0), CLONE_CCB_CONTROL);

  /*** create a 10x10 grid of objects ***/

  for (i = 0; i < MAX_SW_OBJECTS; i++)
    Objects[i] = NULL;

  for (i = 0; i < 5; i++)
    {
      for (j = 0; j < 5; j++)
        {
          if (object_counter >= MAX_DISPLAYABLE_OBJECTS)
            break;

          Objects[object_counter]
              = CloneModel (Image_root_cube, CLONE_GEOMETRY);

          if (!Objects[object_counter])
            {
              printf ("unable to create object %ld\n", object_counter);
              getout (GENERIC_ERROR);
            }

          AddModel (&World1, Objects[object_counter]);

          SetObjectLocation (Objects[object_counter], Convert32_F16 (i * 3), 0,
                             Convert32_F16 (j * 3));
          object_counter++;
        }
    }

  Color_ccb[0] = CreateColorCCB (31, 31, 31);

  Color_ccb[1] = CreateColorCCB (31, 0, 0);
  Color_ccb[2] = CreateColorCCB (0, 31, 0);
  Color_ccb[3] = CreateColorCCB (0, 0, 31);
  Color_ccb[4] = CreateColorCCB (15, 0, 31);
  Color_ccb[5] = CreateColorCCB (16, 16, 0);
  Color_ccb[6] = CreateColorCCB (0, 31, 31);

  /*** create the light-source ***/

  InitModel (Objects[WHITE_CUBE], NULL, tagsnolites);
  SetModelCCB (Objects[WHITE_CUBE], Color_ccb[0], COPY_CCB_CONTROL);

  InitCamera (&World_camera1, cameratags);
  Camera1_id = AddCamera (&World1, &World_camera1);

  SetObjectLocation (&World_camera1, Convert32_F16 (7), Convert32_F16 (2),
                     Convert32_F16 (-4));
}

/**************************************************************************
 * move_things :                                                          *
 **************************************************************************/
void
move_things ()
{
  static frac16 angles[SIZE_ANGLE_ARRAY]
      = { 1 << 16, 2 << 16, 3 << 16, 1 << 15, -2 << 16, -3 << 16, 48000 };

  static TagArg tagslites[]
      = { F3D_TAG_MODEL_LIGHTS, (void *)LIGHT_0, F3D_TAG_DONE, 0 };

  static TagArg tagsnolites[]
      = { F3D_TAG_MODEL_LIGHTS, (void *)NO_LIGHTS, F3D_TAG_DONE, 0 };

  frac16 tempf16;
  int32 iyaw, iroll, ipitch;
  int32 i;
  static frac16 angle1;

  switch (State_cycle)
    {
    case 0:
      Rotation_state = OFF;
      Lite_state = OFF;
      Transparency_state = OFF;
      Image_state = OFF;
      Small_image_state = OFF;
      break;

    case 1:
      Rotation_state = ON;
      Lite_state = OFF;
      Transparency_state = OFF;
      Image_state = OFF;
      Small_image_state = OFF;
      break;

    case 2:
      Rotation_state = ON;
      Lite_state = ON;
      Transparency_state = OFF;
      Image_state = OFF;
      Small_image_state = OFF;
      break;

    case 3:
      Rotation_state = ON;
      Lite_state = OFF;
      Transparency_state = OFF;
      Image_state = ON;
      Small_image_state = OFF;
      break;

    case 4:
      Rotation_state = ON;
      Lite_state = ON;
      Transparency_state = OFF;
      Image_state = ON;
      break;

    case 5:
      Rotation_state = ON;
      Lite_state = ON;
      Transparency_state = OFF;
      Small_image_state = ON;
      Image_state = OFF;
      break;

    case 6:
      Rotation_state = ON;
      Lite_state = OFF;
      Transparency_state = ON;
      Small_image_state = OFF;
      Image_state = ON;
      break;
    }

  if (State_changed)
    {
      for (i = 0; i < MAX_DISPLAYABLE_OBJECTS; i++)
        {
          if (Image_state)
            SetModelCCB (Objects[i], Ccbs[0], COPY_CCB_CONTROL);
          else if (Small_image_state)
            SetModelCCB (Objects[i], Ccbs[1], COPY_CCB_CONTROL);
          else
            SetModelCCB (Objects[i], Color_ccb[i % 7], COPY_CCB_CONTROL);

          if (Lite_state)
            ModifyObject (Objects[i], tagslites);
          else
            ModifyObject (Objects[i], tagsnolites);

          if (Transparency_state)
            SetModelPIXC (Objects[i], TRANSLUCENT_CEL);
          else
            SetModelPIXC (Objects[i], SOLID_CEL);
        }

      printf ("State_cycle=%ld\n", State_cycle);

      if (Small_image_state)
        printf ("using small images\n");
      else if (Image_state)
        printf ("using large images\n");
      else
        printf ("using mono-chrome faces\n");

      printf ("lights : %ld\n", Lite_state);
      printf ("Translucency : %ld\n", Transparency_state);
      printf ("Rotation     : %ld\n\n", Rotation_state);

      State_changed = NO;
    }

  for (i = 0; i < MAX_DISPLAYABLE_OBJECTS; i++)
    {
      iroll = Cube_init[i].iroll;
      ipitch = Cube_init[i].ipitch;
      iyaw = Cube_init[i].iyaw;

      Cube_init[i].roll += angles[iroll];
      Cube_init[i].pitch += angles[ipitch];
      Cube_init[i].yaw += angles[iyaw];

      Cube_init[i].roll %= FULLCIRCLE;
      Cube_init[i].pitch %= FULLCIRCLE;
      Cube_init[i].yaw %= FULLCIRCLE;

      if (Rotation_state)
        {
          RotateModel (Objects[i], Cube_init[i].pitch, Cube_init[i].yaw,
                       Cube_init[i].roll);
        }
    }

  angle1 += (3 << 16);

  tempf16 = SinF16 (angle1);
  Objects[WHITE_CUBE]->wo_WorldPosition[X_COORD]
      = MulSF16 (tempf16, (20 << 16));

  tempf16 = CosF16 (angle1);
  Objects[WHITE_CUBE]->wo_WorldPosition[Z_COORD]
      = MulSF16 (tempf16, (20 << 16));

  /*** position the light on the pyramid ***/

  World1.wp_LightPosition[0][X_COORD]
      = Objects[WHITE_CUBE]->wo_WorldPosition[X_COORD];
  World1.wp_LightPosition[0][Y_COORD] = Convert32_F16 (0);
  World1.wp_LightPosition[0][Z_COORD]
      = Objects[WHITE_CUBE]->wo_WorldPosition[Z_COORD];

  /*** dunno why I have to negate the velocity ***/

  World_camera1.wo_ObjectVelocity = -Velocity;

  World_camera1.wo_WorldOrientation[PITCH] = Delta_pitch;
  World_camera1.wo_WorldOrientation[YAW] = Delta_yaw;
  World_camera1.wo_WorldOrientation[ROLL] = Delta_roll;
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
  static int32 counter = 0;

  ENTER ("render_world");

  wport = &World1;

  Rport.rp_BitmapItem = bitmapitem; /* flips the buffers */

  /*** builds up the sorted list ***/

  if (Rotate_stuff)
    {
      move_things ();
    }

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
