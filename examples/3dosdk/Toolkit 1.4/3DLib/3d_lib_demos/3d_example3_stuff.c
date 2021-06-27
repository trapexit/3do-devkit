/**************************************************************************
 * 3d_example3_stuff : contains init and driver routines                  *
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
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************/
#include "graphics.h"
#include "3dlib.h"
#include "3d_examples.h"
#include "shapes.h"

struct WorldObject World_pyramid1;
struct WorldObject World_cube1;
struct WorldObject World_cube2;
struct WorldObject World_pyramid2;
struct WorldObject World_pyramid3;
struct WorldObject World_pyramid4;
struct WorldObject *Pyramid2;
struct WorldObject *Pyramid3;
struct WorldObject *Pyramid4;        

struct WorldObject World_camera1;
struct WorldPort   World1;

int32 World_cube2_id;

Obj3D Camera1_id;                    /* handle to my returned camera */

extern Item  BitmapItems[];
extern int32 Delta_camera_angle;
extern int32 Delta_x, Delta_y;
extern int32 Rotate_stuff;

/*** temporary storage area to hold the transformed points. Each point needs
 *** 12 bytes. ***/

char World1_pointheap[HEAP_SIZE];

struct Render3DPort Rport;

extern CCB *Ccbs[];                             /* ccb */

/**************************************************************************
 * init_worlds : called by render_whiz to test the 3d-lib               *
 **************************************************************************
 * NOTE : all of the objects are described in shapes.h                    *
 **************************************************************************/
void
init_worlds()
{
  
   static TagArg cubetags[]={F3D_TAG_MODEL_FLAGS,(void *)(CULL_BACKFACE|PSEUDO_PERSPECTIVE),
                         F3D_TAG_DONE,0};

   static TagArg pyramidtags[]={F3D_TAG_MODEL_FLAGS,(void *)(CULL_BACKFACE|PSEUDO_PERSPECTIVE),
                         F3D_TAG_DONE,0};

   static TagArg worldtags[]={
						 F3D_TAG_FACE_ORIENTATION,(void *)CW_FACES,
                         F3D_TAG_DONE,0};

  /*** Init a new Render port ***/

   InitRender3DPort(&Rport, BitmapItems[0]);

  /*********************
   *** create world1 ***
   *********************/
   
   InitNewWorld(&World1,World1_pointheap,HEAP_SIZE,worldtags);

  /*** create the central cube ***/

   InitModel(&World_cube1,&Cube,cubetags);
   AddModel(&World1,&World_cube1);
   
   SetModelCCB(&World_cube1,Ccbs[0],SHARE_CCB);   /* set all faces to the same image */
   SetFaceCCB(&World_cube1,0,1,Ccbs[1],SHARE_CCB);  /* set face 1 to another image */

  /*** create the far cube ***/

   InitModel(&World_cube2,&Cube,cubetags);
   AddModel(&World1,&World_cube2);
   
   SetModelCCB(&World_cube2,Ccbs[0],SHARE_CCB);   /* set all faces to the same image */
   SetFaceCCB(&World_cube2,0,1,Ccbs[1],SHARE_CCB);  /* set face 1 to another image */
   SetObjectLocation(&World_cube2,0,0,Convert32_F16(7));

  /*** create the orbiting pyramid ***/

   InitModel(&World_pyramid1,&Pyramid,pyramidtags);
   SetModelCCB(&World_pyramid1,Ccbs[0],SHARE_CCB);
   AddModel(&World1,&World_pyramid1);

  /*** create the 3 other pyramids ***/

   Pyramid2=CloneModel(&World_pyramid1,CLONE_GEOMETRY);
   AddModel(&World1,Pyramid2);
   SetObjectLocation(Pyramid2,0,0,Convert32_F16(3));

   Pyramid3=CloneModel(&World_pyramid1,CLONE_GEOMETRY);
   AddModel(&World1,Pyramid3);
   SetObjectLocation(Pyramid3,0,0,Convert32_F16(5));

   Pyramid4=CloneModel(&World_pyramid1,CLONE_GEOMETRY);
   AddModel(&World1,Pyramid4);
   SetObjectLocation(Pyramid4,0,Convert32_F16(3),Convert32_F16(9));

   InitCamera(&World_camera1,NULL);
   Camera1_id=AddCamera(&World1,&World_camera1);
   SetObjectLocation(&World_camera1,Convert32_F16(8),Convert32_F16(8),-Convert32_F16(2));
}

/**************************************************************************
 * move_things : animates updates the positions and attitudes of the      *
 *     things in the world.                                               *
 **************************************************************************/
void
move_things(void)
{
   static int32 angle1=140,angle2=0,angle3=0, angle4=0;
   frac16 tempf16;
   static frac16 p3_delta_y=(1<<14);      /* delta y for pyramid 3's vertical position */
   static frac16 p3_max_y=(3<<16);        /* highest point for p3 */
   frac16 p3y;                            /* current Y position of pyramid 3 */

   angle2+=(1<<16);
   angle2%=(256<<16);

   angle3+=(2<<16);
   angle3%=(256<<16);

   angle4+=48000;              /* ~ 1.5 units */
   angle4%=(256<<16);

  /*** update the position of the pyramid, make it move around the cube ***/

   angle1+=(2<<16);

   tempf16=SinF16(angle1);    
   World_pyramid1.wo_WorldPosition[X_COORD]=MulSF16(tempf16,(3<<16));
 
   tempf16=CosF16(angle1);
   World_pyramid1.wo_WorldPosition[Z_COORD]=MulSF16(tempf16,(3<<16));

  /*** you could also set the wo_Orientation fields as needed and call
   *** ResolveObjectMatrix() or MakeCRotationMatrix() instead ***/

  /*** the cube tumbles around ***/

   RotateModel(&World_cube2,angle3,0,-angle2);
   RotateModel(&World_cube1,angle2,angle2,angle3);

  /*** the moving object rotates CW on it's Y axis ***/

   RotateModel(&World_pyramid1,0,-angle3,0);

  /*** the stationary pyramid rotates on it's Y axis in a CCW direction ***/

   RotateModel(Pyramid2,0,-angle2,0);

  /*** pyramid 3 and 4 move up and down like pistons, and rotate ***/

   RotateModel(Pyramid3,0,-angle2,0);

   p3y=Pyramid3->wo_WorldPosition[Y_COORD];

   if(p3_delta_y>0)                        /* we're going up */
   {
      if(p3y>p3_max_y)p3_delta_y= -p3_delta_y;    /* start going down */
   }
   else
   {
      if(p3y<0)p3_delta_y= -p3_delta_y;    /* start going up */
   }

   Pyramid3->wo_WorldPosition[Y_COORD]+=p3_delta_y;
   Pyramid4->wo_WorldPosition[Y_COORD]-=p3_delta_y;
   
  /*** move the camera ***/

   World_camera1.wo_WorldPosition[X_COORD]+=Delta_x;
   World_camera1.wo_WorldPosition[Y_COORD]+=Delta_y;
}

/**************************************************************************
 * render_world : performs the rotations on the desired world and renders *
 *     it.                                                                *
 **************************************************************************/
void
render_world(bitmapitem,cel_id,camera_id,world_id)

int32 bitmapitem;
int32 cel_id;
int32 camera_id;
int32 world_id;
{
   WorldPort *wport;
   static frac16 roll=0;

   wport=&World1;

   Rport.rp_BitmapItem=bitmapitem;              /* flips the buffers */

  /*** builds up the sorted list ***/

   if(Rotate_stuff)move_things();

   roll+=Delta_camera_angle;

   LookAt(wport,camera_id,World_cube1.wo_WorldPosition,roll);

   RenderWorld(&Rport,wport,camera_id);
}
