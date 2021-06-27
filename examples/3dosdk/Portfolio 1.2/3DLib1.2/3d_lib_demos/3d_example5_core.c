/**************************************************************************
 * 3d_example5 : This example demonstrates a couple of the new features   *
 *      in 3dlib, 1.1. Two cubes are displayed. The closest is a mesh-    *
 *      object which will switch over to the mesh at a distance of 2.     *
 *      each face will become 16 sub-cels. The other cube demonstrates    *
 *      scrolling cels. Two faces are animated, scrolling the astronaut's *
 *      image back and fourth and zooming in and out.                     * 
 **************************************************************************
 *                                                                        *
 * Created by Mike Smithwick, October 1993.                               *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, The 3DO Company.                                   *
 * All rights reserved                                                    *
 * This document is proprietary and confidential                          *
 *                                                                        *
 **************************************************************************/

#include "types.h"
#include "debug.h"
#include "nodes.h"
#include "kernelnodes.h"
#include "list.h"
#include "folio.h"
#include "task.h"
#include "kernel.h"
#include "mem.h"
#include "operamath.h"
#include "semaphore.h"
#include "io.h"
#include "strings.h"
#include "stdlib.h"
#include "event.h"
#include "audio.h"
#include "stdio.h"

#include "graphics.h"

#include "3dlib.h"
#include "Init3DO.h"
#include "Form3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"
#include "3d_examples.h"

/*** globals ***/

long Debug_flag=NO;
long Rotate_object=YES;
long Rotate_stuff=YES;
long  View_X=0, View_Y=0, View_Z=(0x00000001<<14), Velocity=0;
long  View_roll=0, View_pitch=0, View_yaw=0;
frac16 Object_yaw=0, Object_pitch=0;
long Delta_x=0, Delta_y=0;
long  Delta_pitch, Delta_yaw, Delta_roll;
int32 Camera_id=0, World_id=0;


Item  BitmapItems[2];
Bitmap *Bitmaps[2];
ScreenContext Screencontext;
extern WorldObject World_camera1;

extern WorldPort World1;
extern WorldObject *Objects[];

char Celfile1[30]="3d_data/sw.cel";
char Celfile2[30]="3d_data/dp128.uu16.cel";

int32 ScreenPageCount;

/*** this stuff is used by loadcel() ***/

CCB *Ccbs[5];                             /* ccb */

/***********************************************************************
 * close_everything :                                                  *
 ***********************************************************************/
void
close_everything( void )
{
   ENTER("close_everything");

   KillEventUtility();

   EXIT("close_everything");
}

/***********************************************************************
 * get_joystick_state : returns the current state of the joystick bits.*
 ***********************************************************************/
long
get_joystick_state( void )
{
   ControlPadEventData cpaddata;

   ENTER("get_joystick_state");

   cpaddata.cped_ButtonBits=0;

   GetControlPad(1,0,&cpaddata);

   EXIT("get_joystick_state");

   return ( cpaddata.cped_ButtonBits );
}

/***********************************************************************
 * getout : exits the program                                          *
 ***********************************************************************/
void
getout(rvalue)

long rvalue;
{
   ENTER("getout");

   FadeToBlack( &Screencontext,FADE_FRAMECOUNT );
   close_everything();
   printf("exiting : %ld\n",rvalue);

   exit((int)rvalue);
}

/**************************************************************************
 * init_cels : initializes any cels needed                                *
 **************************************************************************/
bool
init_cels(void)
{
   bool rvalue=TRUE;
   char filename[30];

   ENTER("init_cels");

   sprintf(filename,"%s",Celfile1);

   Ccbs[0]=LoadCel(filename,MEMTYPE_CEL);
   
   if(Ccbs[0])
   {
      Ccbs[0]->ccb_Flags|=CCB_BGND;      
      Ccbs[0]->ccb_PIXC=SOLID_CEL;
      Ccbs[0]->ccb_PIXC=SOLID_CEL;
   }
   else
   {
      printf("unable to load %s\n",filename);
      getout(GENERIC_ERROR);
   }

   sprintf(filename,"%s",Celfile2);

   Ccbs[1]=LoadCel(filename,MEMTYPE_CEL);
   
   if(Ccbs[1])
   {
      Ccbs[1]->ccb_Flags|=CCB_BGND;      
      Ccbs[1]->ccb_PIXC=SOLID_CEL;
      Ccbs[1]->ccb_PIXC=SOLID_CEL;
   }
   else
   {
      printf("unable to load %s\n",filename);
      getout(GENERIC_ERROR);
   }

   EXIT("init_cels");

   return(rvalue);
}

/**************************************************************************
 * init_graphics : uses the Lib3DO stuff                              *
 **************************************************************************/
bool
init_graphics(void)
{
   int i;
   long width,height;

   ENTER("init_graphics");

   if(!OpenGraphics(&Screencontext,2))return(FALSE);

   for(i=0;i<2;i++)
   {

	  BitmapItems[i] = Screencontext.sc_BitmapItems[i];
      Bitmaps[i] = Screencontext.sc_Bitmaps[i];
   }

   width = Bitmaps[0]->bm_Width;
   height = Bitmaps[0]->bm_Height;

   ScreenPageCount = (width*2*height+ GrafBase->gf_VRAMPageSize-1) / GrafBase->gf_VRAMPageSize;

   EXIT("init_graphics");

   return(TRUE);
}

/**************************************************************************
 * init_stuff : initializes everything                                    *
 **************************************************************************/
void
init_stuff(void)
{
   ENTER("init_stuff");

   init_system();

   init_graphics();

   init_cels();

   Init3DLib(MONO_VIEW,0L);

   init_worlds();

   EXIT("init_stuff");
}

/**************************************************************************
 * init_system : initializes main system level stuff, libraries, etc.     *
 **************************************************************************/
void
init_system(void)
{
   ENTER("init_system");

   if(OpenMathFolio()<0)
      printf("\n\nOperamath returned an error during FindMathFolio!!!\n\n");

   if(OpenGraphicsFolio()<0)
   {
      printf("unable to open Audiofolio!\n");
      getout(GENERIC_ERROR);
   }

   if(OpenAudioFolio()<0)
   {
      printf("unable to open Audiofolio!\n");
      getout(GENERIC_ERROR);
   }    

   if (!OpenMacLink() )
   {
      printf("mac link failed!\n");
      getout(GENERIC_ERROR);
   }

   if (!OpenSPORT())
   {
      printf("open sport failed!\n");
      getout(GENERIC_ERROR);
   }

   if(InitEventUtility(1,0,LC_Observer)<0)
   {
      printf("unable to open the event broker\n");
   }

   EXIT("init_system");
}

/***********************************************************************
 * main_loop :                                                         *
 ***********************************************************************/
long
main_loop(void)
{
   long rvalue=0;
   long screen_select=0;        /* determines which screen to write to */
   long counter=0;
   Item ioreq, vblio;

   ENTER("main_loop");

   EnableVAVG(Screencontext.sc_Screens[0]);
   EnableHAVG(Screencontext.sc_Screens[0]);
   EnableVAVG(Screencontext.sc_Screens[1]);
   EnableHAVG(Screencontext.sc_Screens[1]);

   ioreq=GetVRAMIOReq();
   vblio=GetVBLIOReq();

   for(;;)
   {
      /*** if the user has pressed the START button, exit ***/

       if(process_joystick()==START_BUTTON)goto DONE;

       WaitVBL(vblio,0);
       SetVRAMPages(ioreq,Bitmaps[screen_select]->bm_Buffer,MakeRGB15Pair(3,0,8), ScreenPageCount, -1 );

       render_world(BitmapItems[ screen_select ],0,Camera_id,World_id);

       rvalue = DisplayScreen( Screencontext.sc_Screens[screen_select], 0 );

       if(rvalue<0)
       {
          printf("DisplayScreen failed!\n");
          goto DONE;
       }

      /*** alternates buffers ***/

       if(screen_select==0)screen_select=1;
       else                screen_select=0;

       counter++;
   }

   EXIT("main_loop");

DONE:
   return(rvalue);
}

/**************************************************************************
 * process_joystick :                                                     *
 **************************************************************************/
long
process_joystick(void)
{
   long rvalue=0;
   long joybits;
   static long delta_yaw=0, delta_pitch=0, delta_roll=0;
   long delta_velocity=0;
   long delta_angle=(1<<11);
   long max_delta= 8<<16;
   long min_delta=-8<<16;
 
   ENTER("process_joystick");

   joybits=get_joystick_state();

   if (joybits&ControlStart)
   {
      rvalue=START_BUTTON;
      goto DONE;
   }

   rvalue=0;

  /*** toggle between worlds ***/

   if(joybits&ControlA)
   {
      Velocity=0;
	  Object_yaw=0;
      Object_pitch=0;

      delta_pitch=0;
      delta_yaw=0;
      delta_roll=0;
      delta_velocity=0;

      Velocity=0;

      while(get_joystick_state()&ControlA); /* hang until ButtonA is released */

      NormalizeMatrix(World_camera1.wo_ObjectMatrix);
   }

   if(joybits&ControlB)
   {
      delta_velocity=(-1<<11);
   }

   if(joybits&ControlC)
      delta_velocity=(1<<10);

  /*** start motion on the first click, stop on the second ***/

   if(joybits&ControlDown)
   {
      delta_pitch-=(delta_angle);
   }

   if(joybits&ControlUp)
   {
      delta_pitch+=(delta_angle); 
   }

   if(joybits&ControlLeft)
   {
      delta_yaw-=delta_angle;
   }

   if(joybits&ControlRight)
   {
      delta_yaw+=delta_angle;
   }

   if(!joybits)
   {

   }

  /*** set and clip the velocity ***/

   Velocity=delta_velocity;

   if(Velocity>(1<<17))Velocity=(1<<17);
   else if(Velocity<(-1<<17))Velocity=-1<<17;

  /*** set and clip the angles and deltas ***/

   if(delta_pitch>max_delta)delta_pitch=max_delta;
   else if(delta_pitch<min_delta)delta_pitch=min_delta;

   if(delta_roll>max_delta)delta_roll=max_delta;
   else if(delta_roll<min_delta)delta_roll=min_delta;


   View_pitch+=delta_pitch;
   View_yaw+=delta_yaw;
   View_roll+=delta_roll;

   Delta_pitch=delta_pitch;
   Delta_yaw  =delta_yaw;
   Delta_roll =delta_roll;

   EXIT("process_joystick");

DONE:
   return(rvalue);
}

/**************************************************************************
 * main :                                                                 *
 **************************************************************************/
int
main(argc, argv)

long argc;
char *argv[];
{
   long rvalue=0;

   init_stuff();

   main_loop();

   getout(rvalue);
}

