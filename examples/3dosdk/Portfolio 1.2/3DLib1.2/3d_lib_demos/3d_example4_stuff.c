/**************************************************************************
 * 3d_example4_stuff : contains init and driver routines                  *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick                                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : May 1993                                                     *
 *                                                                        *
 **************************************************************************/
#include "3d_examples.h"
#include "3dlib.h"
#include "graphics.h"
#include "shapes.h"

struct WorldObject *World_pyramid1;
struct WorldObject *World_cube1;
struct WorldObject *World_cube2;
struct WorldObject Root_cube;
struct WorldObject Root_pyramid;
struct WorldObject Soundaddr;
struct WorldObject *Rumblesound = 0;

struct WorldObject World_camera1;
struct WorldPort World1;

Sound3D *Rumble;
char Audiofile1[50] = "Rumble.aiff";

Obj3D Camera1_id; /* handle to my returned camera */
Obj3D Cube_id;

extern Item BitmapItems[];
extern int32 View_X, View_Y, Camera_angle;
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

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-lib                 *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds ()
{
  CCB *colorccb;
  static TagArg cubetags[] = { F3D_TAG_MODEL_LIGHT_AMBIENT,
                               (void *)3,
                               F3D_TAG_MODEL_LIGHTS,
                               (void *)LIGHT_0,
                               F3D_TAG_MODEL_FLAGS,
                               (void *)(CULL_BACKFACE | PSEUDO_PERSPECTIVE),
                               F3D_TAG_MODEL_NORMALS,
                               (void *)TRUE,
                               F3D_TAG_DONE,
                               0 };

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

  InitModel (&Root_cube, &Cube, cubetags);
  SetModelCCB (&Root_cube, Ccbs[0], CLONE_CCB_CONTROL);

  InitModel (&Root_pyramid, &Pyramid, pyramidtags);
  SetModelCCB (&Root_pyramid, Ccbs[0], CLONE_CCB_CONTROL);

  /*** create the central cube ***/

  /*** give the cube the surface array to store the ccbs and flags ***/

  World_cube1 = CloneModel (&Root_cube, 0);
  Cube_id = AddModel (&World1, World_cube1);

  SetModelCCB (World_cube1, Ccbs[0],
               CLONE_CCB_CONTROL); /* set all faces to the same image */

  colorccb = CreateColorCCB (31, 0, 0);
  SetFaceCCB (World_cube1, 0, 1, colorccb, SHARE_CCB);
  colorccb = CreateColorCCB (0, 0, 31);
  SetFaceCCB (World_cube1, 0, 3, colorccb, SHARE_CCB);

  World_cube2 = CloneModel (&Root_cube, 0);
  AddModel (&World1, World_cube2);
  SetObjectLocation (World_cube2, 0, 0, Convert32_F16 (4));

  SetModelCCB (World_cube2, Ccbs[0],
               CLONE_CCB_CONTROL); /* set all faces to the same image */

  /*** create the orbiting pyramid. The pyramid is the light-source, and is
   *colored
   *** white. In order to distinguish the faces from one another, they are
   *varied
   *** slightly in shading and colors. ***/

  World_pyramid1 = CloneModel (&Root_pyramid, 0);
  AddModel (&World1, World_pyramid1);
  colorccb = CreateColorCCB (31, 31, 31);
  SetModelCCB (World_pyramid1, colorccb, CLONE_CCB_CONTROL);
  colorccb = CreateColorCCB (31, 31, 27);
  SetFaceCCB (World_pyramid1, 0, 1, colorccb, SHARE_CCB);
  SetFaceCCB (World_pyramid1, 0, 3, colorccb, SHARE_CCB);
  colorccb = CreateColorCCB (10, 10, 0);
  SetFaceCCB (World_pyramid1, 0, 4, colorccb, SHARE_CCB);

  InitCamera (&World_camera1, NULL);
  Camera1_id = AddCamera (&World1, &World_camera1);
  SetObjectLocation (&World_camera1, 0, 0, -Convert32_F16 (2));

  /*** load an init the sound object ***/

  Rumblesound = &Soundaddr;

  if (Init3DSound (Rumblesound, Audiofile1, NULL) >= 0)
    {
      Add3DSound (&World1, Rumblesound, 0);
      SetObjectLocation (Rumblesound, -Convert32_F16 (3), 0, 0);

      Toggle3DSound (Rumblesound, SOUND_ON);
    }
  else
    {
      printf ("Init3Dsound failed!\n");
      getout (GENERIC_ERROR);
    }
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

  angle1 += (0x20000);

  tempf16 = SinF16 (angle1);
  World_pyramid1->wo_WorldPosition[X_COORD] = MulSF16 (tempf16, (3 << 16));

  tempf16 = CosF16 (angle1);
  World_pyramid1->wo_WorldPosition[Z_COORD] = MulSF16 (tempf16, (3 << 16));

  /*** position the light on the pyramid ***/

  World1.wp_LightPosition[0][X_COORD]
      = World_pyramid1->wo_WorldPosition[X_COORD];
  World1.wp_LightPosition[0][Y_COORD] = Convert32_F16 (0);
  World1.wp_LightPosition[0][Z_COORD]
      = World_pyramid1->wo_WorldPosition[Z_COORD];

  /*** position the sound on the pyramid ***/

  SetObjectLocation (Rumblesound, World_pyramid1->wo_WorldPosition[X_COORD], 0,
                     World_pyramid1->wo_WorldPosition[Z_COORD]);

  /*** the cube tumbles around ***/

  RotateModel (World_cube1, angle2, angle1, -angle3);
  RotateModel (World_cube2, 0, angle2, 0);

  /*** the moving object rotates CW on it's Y axis ***/

  RotateModel (World_pyramid1, 0, angle3, 0);
  SetObjectLocation (&World_camera1, View_X, View_Y,
                     World_camera1.wo_WorldPosition[Z_COORD]);

  World_camera1.wo_WorldOrientation[Y_ANGLE] = Camera_angle;
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

  LookAt (wport, camera_id, World_cube1->wo_WorldPosition, 0);

  RenderWorld (&Rport, wport, camera_id);
  /*
  RenderSound(&World1,Rumblesound,camera_id);
  */
}
