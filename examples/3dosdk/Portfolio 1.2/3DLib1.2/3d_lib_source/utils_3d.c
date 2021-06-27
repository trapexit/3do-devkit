/**************************************************************************
 * utils_3d.c : contains some 3d lib utility routines. Similar to         *
 *		misc_3d.c except with more general purpose routines               *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick and Dan Duncalf                 *
 *                                                                        *                                                                        *
 **************************************************************************
 *                                                                        *
 * Written : April 1993                                                   *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, New Technologies Group, Inc.	                	  *
 * All rights reserved						                              *
 * This document is proprietary and confidential			              *
 *								                                          *
 **************************************************************************/

#include "Portfolio.h"
#include "3dlib.h"
#include "Init3DO.h"
#include "Form3DO.h"
#include "Parse3DO.h"
#include "Utils3DO.h"

extern int32 Debug_flag;
extern int32 Face_rotation_flag;

void printmatrix(char *text,mat33f16 Matrix)
{
   printf("%s:\n | 0x%lx  ,  0x%lx  , 0x%lx |\n",
      text,Matrix[0][0],Matrix[0][1],Matrix[0][2]);
   printf(" | 0x%lx  ,  0x%lx  , 0x%lx |\n",
      Matrix[1][0],Matrix[1][1],Matrix[1][2]);
   printf(" | 0x%lx  ,  0x%lx  , 0x%lx |\n\n",
      Matrix[2][0],Matrix[2][1],Matrix[2][2]);
}
/**************************************************************************
 * CreateColorCCB : makes a generic CCB set to the desired rgb colors.    *
 *     This is used to create a blank colored face without having to      *
 *     use an art program to create a single-pixel cel.                   *
 **************************************************************************/
CCB *CreateColorCCB(int32 r,int32 g,int32 b)
{
   static CCB *blankccb;
   Rectf16 one_pixel_rect;
   uint32 color;

   ENTER("CreateColorFace");

   SetRectf16(&one_pixel_rect,0,0,2,2);
   blankccb=MakeNewCel(&one_pixel_rect);

  /*** create two encoded pixels in the single long word so it won't
   *** matter whether which endian-ness (is that a word?) this is compiled at. ***/

   color=((r<<10)|(g<<5)|(b));

  /*** MakeNewCel() above creates a 4x4 cel, all I am using is a 2x2 portion
   *** in the upper-left corner. So I only need to stuff data into elements
   *** 0 and 2, where 1 and 3 are for portions I am not using. ***/

   (*blankccb->ccb_SourcePtr)[0]=(color<<16)|color;
   (*blankccb->ccb_SourcePtr)[2]=(color<<16)|color;

   EXIT("CreateColorFace");

   return(blankccb);
}

/***********************************************************************
 * ClipCel : creates a sub-ccb from an exisiting one, the datapointer *
 *     points into the supplied ccb-data and effectivley extracts a    *
 *     rect. The data is not duplicted, merely flags and values are    *
 *     are set to define the sub-rect. See PRG section on "Extracting  *
 *     Cel Source Data. . ." (p. 221ff in the 1.0 docs)                *
 ***********************************************************************/
CCB *
ClipCel(CCB *ccb,int32 x,int32 y,int32 w,int32 h)
{
   CCB *subccb;
   Rectf16 rect;
   int32 woffset;
   int32 lrform;
   int32 skipx;
   int32 bpp;

   ENTER("ClipCel");

   skipx=0;

  /*** canna do packed CCBs, Kiptain sir! ***/

   if(ccb->ccb_Flags&CCB_PACKED)return(NULL);

  /*** get width of source data, note that I don't bother with shifting it, merely get the
   *** proper bits that I can feed into the equation down below. ***/

   bpp=(ccb->ccb_PRE0 & PRE0_BPP_MASK);

   if((bpp==PRE0_BPP_8) || (bpp==PRE0_BPP_16))
      woffset= ((ccb->ccb_PRE1&PRE1_WOFFSET10_MASK)>>PRE1_WOFFSET10_SHIFT)+2;
   else
      woffset= ((ccb->ccb_PRE1&PRE1_WOFFSET8_MASK)>>PRE1_WOFFSET8_SHIFT)+2;

   lrform=(ccb->ccb_PRE1&PRE1_LRFORM);

   SetRectf16(&rect,0,0,2,2);

//   subccb=MakeNewCel(&rect);

//   subccb=malloc(sizeof(CCB));
   subccb=MakeNewDupCCB(ccb);

   subccb->ccb_Flags=ccb->ccb_Flags;

   subccb->ccb_NextPtr=0;
//   subccb->ccb_PIXC=0x1f001f00;
   subccb->ccb_PIXC=ccb->ccb_PIXC;

   subccb->ccb_Width=w;
   subccb->ccb_Height=h;

   subccb->ccb_PRE0=ccb->ccb_PRE0;
   subccb->ccb_PRE1=ccb->ccb_PRE1;

   subccb->ccb_PLUTPtr=ccb->ccb_PLUTPtr;

  /*** slip in the proper VCNT value, by masking out the source ccb vcnt and ORing in 
   *** the new one. ***/


   if(lrform)
   {
      subccb->ccb_PRE0=   ((subccb->ccb_PRE0 & (~PRE0_VCNT_MASK))) | 
	     			   (((h>>1)-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT)|PRE0_BGND; 
   }
   else
   {
      subccb->ccb_PRE0=((subccb->ccb_PRE0 & (~PRE0_VCNT_MASK))) | 
	     			   ((h-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT) | PRE0_BGND;
   }

   subccb->ccb_PRE1=   ((subccb->ccb_PRE1 & (~PRE1_TLHPCNT_MASK))) | 
                        ((w-PRE1_TLHPCNT_PREFETCH)<<PRE1_TLHPCNT_SHIFT);


   if(lrform)
   {
     /*** this decodes the address for an LRFORM pixel. Since each x value is in effect
      *** 1 word, there is no down conversion needed for that. The y value is divided
      *** in half since each line of data is actually two lines. This gives us the
      *** address of the word we are in, then set SKIPX if this is an odd line, to skip
      *** the first pixel in the word and go directly to the next one. ***/

      subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x] + (woffset*(y/2)));

      if(y%2)subccb->ccb_PRE0=((subccb->ccb_PRE0 & (~PRE0_SKIPX_MASK)))|(1<<PRE0_SKIPX_SHIFT);
   }
   else
   {
      switch(bpp)
      {
         case PRE0_BPP_16:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/2] + woffset*y);
            skipx=x%2;
            break;

         case PRE0_BPP_8:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/4] + woffset*y);
            skipx=x%4;
            break;

         case PRE0_BPP_6:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/5] + woffset*y);
            skipx=x%5;
            break;

         case PRE0_BPP_4:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/8] + woffset*y);
            skipx=x%8;
            break;

         case PRE0_BPP_2:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/16] + woffset*y);
            skipx=x%16;
            break;

         case PRE0_BPP_1:
            subccb->ccb_SourcePtr= (CelData *)(&(*ccb->ccb_SourcePtr)[x/32] + woffset*y);
            skipx=x%32;
            break;
      }

      subccb->ccb_PRE0=((subccb->ccb_PRE0 & (~PRE0_SKIPX_MASK)))|(skipx<<PRE0_SKIPX_SHIFT);
   }

   EXIT("ClipCel");

   return(subccb);
}

/**************************************************************************
 * Distance3D : returns the distance between two points, in frac16 form   *
 **************************************************************************/
frac16
Distance3D(point1,point2)

vec3f16 point1;
vec3f16 point2;
{
   frac16 fx,fy,fz;
   ufrac16 dist;
   frac32 tempx,tempy,tempz;
   frac32 temp_sum;

   fx= (frac16)(point1[X_COORD]-point2[X_COORD]);
   fy= (frac16)(point1[Y_COORD]-point2[Y_COORD]);
   fz= (frac16)(point1[Z_COORD]-point2[Z_COORD]);
   if (fx<0) fx=0-fx;
   if (fy<0) fy=0-fy;
   if (fz<0) fz=0-fz;
      
   if ( (fx&0xffff0000)|(fy&0xffff0000)|(fz&0xffff0000) )	/* Check for overflow condition */
   {
   	  MulSF16_F32(&tempx,fx,fx);
      MulSF16_F32(&tempy,fy,fy);
      MulSF16_F32(&tempz,fz,fz);

      AddF32(&temp_sum,&tempx,&tempy);         /* add x-sqr and y-sqr */
      AddF32(&temp_sum,&temp_sum,&tempz);      /* add that to z-sqr */

      dist=SqrtF32_F16(&temp_sum);
    }

	else
	{
	  dist=Sqrt32(fx*fx+fy*fy+fz*fz);
	}
	  
   return((frac16)dist);
}

/**************************************************************************
 * Frac16str : converts frac16 values to human readible strings           *
 **************************************************************************/
char *
Frac16str(frac16 value,char *buffer,int32 len)
{
   int32 left,right,tempvalue;

   ENTER("Frac16str");

   left=value/65536;

  /*** subtracting left<<16 gives the "fractional" value ***/

   tempvalue=(value-(left<<16));
   right=(tempvalue*10000)/65536;

  /*** this checks for the case when the number is <1 and negative. If
   *** it were printed normally it would be something like : "0.-5"
   *** which doesn't make a whole lotta sense. So flip the "right" value
   *** to positive, and check to see if the left value is 0 or not. If it
   *** is 0 there is nothing to carry the negative sign, so we have to 
   *** supply that directly. ***/

   if((right<0) && (left==0))
   {
      right*=-1;
	  sprintf(buffer,"-0.%04ld",right);
   }
   else
   {
      if(right<0)right*=-1;

      sprintf(buffer,"%ld.%04ld",left,right);
   }
   EXIT("Frac16str");

   return(buffer);
}

/**************************************************************************
 * NormalizeVector : returns a normalized vector                          *
 **************************************************************************/
void
NormalizeVector(vec3f16 vector)
{
   frac16 length;

   ENTER("NormalizeVector");

   length=VectorLength(vector);

   if(length!=0)
   {
      vector[X_COORD]=DivSF16(vector[X_COORD],length);
      vector[Y_COORD]=DivSF16(vector[Y_COORD],length);
      vector[Z_COORD]=DivSF16(vector[Z_COORD],length);
   }

   EXIT("NormalizeVector");
}


/**************************************************************************
 * printfF16 : prints out an F16 number                                   *
 **************************************************************************
 * Note : this is a small utility routine used for debugging that you're  *
 * not supposed to know about.                                            *
 **************************************************************************/
void
printfF16(name,value)

char *name;
frac16 value;
{
   int32 left_value;
   int32 lvalue;
   int32 right_value;

   lvalue=value;

   left_value=(lvalue>>16);
   right_value=(lvalue&0x0000ffff);

   printf("%s %ld.%lx\n",name,left_value,right_value); 
}

/**************************************************************************
 * Rotate2DQuad : 2d rotation                                             *
 **************************************************************************/
void
RotateQuad2D(Point srcquad[4],Point dstquad[4], int32 cx, int32 cy, frac16 angle)
{
   Point tmpquad[4];
   frac16 sinangle, cosangle;
   int32 i;
   frac16 ptx,pty,ptx_prime,pty_prime;

   memcpy(tmpquad,srcquad,sizeof(Point)*4);

   sinangle=SinF16(angle);
   cosangle=CosF16(angle);

   for(i=0;i<4;i++)
   {
      ptx=(tmpquad[i].pt_X<<16);
      pty=(tmpquad[i].pt_Y<<16);

      ptx_prime= (MulSF16(ptx,cosangle)-MulSF16(pty,sinangle));
      pty_prime= (MulSF16(ptx,sinangle)+MulSF16(pty,cosangle));

      dstquad[i].pt_X= (ptx_prime>>16)+cx;
      dstquad[i].pt_Y= (pty_prime>>16)+cy;
   }
}

/**************************************************************************
 * ScreenToCel : returns a literal CCB, ready to accept screen data.      *
 **************************************************************************/ 
CCB *
ScreenToCel(Item screen_item)
{
   int32 row_word_width=320;
   Rectf16 rect;
   CCB *newccb;
   Screen *screen;
   Bitmap *bitmap=NULL;
   int32 w,h;

   ENTER("ScreenToCel");

   screen=(Screen *)LookupItem(screen_item);

   if(!screen)return(NULL);

   bitmap=screen->scr_TempBitmap;

   w=bitmap->bm_Width;
   h=bitmap->bm_Height;

   rect.rectf16_XLeft=0;
   rect.rectf16_YTop=0;
   rect.rectf16_XRight=(w<<16);
   rect.rectf16_YBottom=(h<<16);

   newccb=MakeNewCel(&rect);

   if(!newccb)
   {
      printf("unable to create new ccb!\n");
      return(NULL);
   }

   newccb->ccb_Flags= CCB_SPABS  | CCB_NPABS  |
						  CCB_PPABS  | CCB_LDSIZE | CCB_LDPPMP | CCB_LDPRS |
                          CCB_YOXY   | CCB_CCBPRE |
                          CCB_ACW | CCB_ACCW   ;

   newccb->ccb_NextPtr=0;
   newccb->ccb_PIXC=0x1f001f00;

   newccb->ccb_PRE0=PRE0_LITERAL | PRE0_BPP_16 | PRE0_LINEAR | 
                        (((h>>1)-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT);

   newccb->ccb_PRE1=PRE1_TLLSB_PDC0 | PRE1_LRFORM |
                        ((w-PRE1_TLHPCNT_PREFETCH)<<PRE1_TLHPCNT_SHIFT) |
                        (((row_word_width-PRE1_WOFFSET_PREFETCH)<<PRE1_WOFFSET10_SHIFT));

   newccb->ccb_SourcePtr=(CelData *)((int32)bitmap->bm_Buffer);

   EXIT("ScreenToCel");

   return(newccb);
}

/***********************************************************************
 * ScrollCel : Scroll a cel's image around in a subwindow. This is     *
 *     based on ClipCel(), but doesn't allocate any new memory, merely *
 *     resets the data pointers. This can be used for both scrolling   *
 *     the image around or zooming in or out.                          *
 ***********************************************************************/
void
ScrollCel(CCB *ccb,CCB *srcccb,int32 x,int32 y,int32 w,int32 h)
{
   int32 woffset;
   int32 lrform;
   int32 skipx;
   int32 bpp;

   ENTER("ScrollCel");

   skipx=0;

  /*** canna do packed CCBs, Kiptain sir! ***/

   if(srcccb->ccb_Flags&CCB_PACKED)return;

  /*** get width of source data, note that I don't bother with shifting it, merely get the
   *** proper bits that I can feed into the equation down below. ***/

   bpp=(srcccb->ccb_PRE0 & PRE0_BPP_MASK);

   if((bpp==PRE0_BPP_8) || (bpp==PRE0_BPP_16))
      woffset= ((srcccb->ccb_PRE1&PRE1_WOFFSET10_MASK)>>PRE1_WOFFSET10_SHIFT)+2;
   else
      woffset= ((srcccb->ccb_PRE1&PRE1_WOFFSET8_MASK)>>PRE1_WOFFSET8_SHIFT)+2;

   lrform=(srcccb->ccb_PRE1&PRE1_LRFORM);

   ccb->ccb_Width=w;
   ccb->ccb_Height=h;

  /*** slip in the proper VCNT value, by masking out the source ccb vcnt and ORing in 
   *** the new one. ***/


   if(lrform)
   {
      ccb->ccb_PRE0=((srcccb->ccb_PRE0 & (~PRE0_VCNT_MASK))) | 
	     			   (((h>>1)-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT)|PRE0_BGND; 
   }
   else
   {
      ccb->ccb_PRE0=((srcccb->ccb_PRE0 & (~PRE0_VCNT_MASK))) | 
	     			   ((h-PRE0_VCNT_PREFETCH)<<PRE0_VCNT_SHIFT) | PRE0_BGND;
   }
 
   if(lrform)
   {
     /*** LRFORM data must always start on a word boundry and an even line. So round odd 
      *** coordinate  values down by 1. ***/

      if(x%2)x-=1;     
      if(y%2)y-=1;

     /*** this decodes the address for an LRFORM pixel. Since each x value is in effect
      *** 1 word, there is no down conversion needed for that. The y value is divided
      *** in half since each line of data is actually two lines. This gives us the
      *** address of the word we are in. SKIPX is not used for LRFORM data. ***/


      ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x] + (woffset*(y/2)));
   }
   else
   {
      switch(bpp)
      {
         case PRE0_BPP_16:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x/2] + woffset*y);
            skipx=x%2;
            break;

         case PRE0_BPP_8:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x/4] + woffset*y);
            skipx=x%4;
            break;

         case PRE0_BPP_6:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[(x/16)*3] + woffset*y);
            skipx=x%16;
            break;

         case PRE0_BPP_4:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x/8] + woffset*y);
            skipx=x%8;
            break;

         case PRE0_BPP_2:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x/16] + woffset*y);
            skipx=x%16;
            break;

         case PRE0_BPP_1:
            ccb->ccb_SourcePtr= (CelData *)(&(*srcccb->ccb_SourcePtr)[x/32] + woffset*y);
            skipx=x%16;
            break;
      }                          // end switch

      ccb->ccb_PRE0=((ccb->ccb_PRE0 & (~PRE0_SKIPX_MASK)))|(skipx<<PRE0_SKIPX_SHIFT);

     /*** the skipx value MUST be added to the width, to maintain proper width. Otherwise
      *** the image is truncated by that amount. ***/

      w+=skipx;

      ccb->ccb_PRE1=((srcccb->ccb_PRE1 & (~PRE1_TLHPCNT_MASK))) | 
                        ((w-PRE1_TLHPCNT_PREFETCH)<<PRE1_TLHPCNT_SHIFT);

   }                             // end else if(...)

   EXIT("ScrollCel");
}

/**************************************************************************
 * SetVec3f16 : no-brainer vector setting routine.                        *
 **************************************************************************/
void
SetVec3f16(vec3f16 vector,frac16 x, frac16 y, frac16 z)
{
   ENTER("SetVec3f16");

   vector[X_COORD]=x;
   vector[Y_COORD]=y;
   vector[Z_COORD]=z;

   EXIT("SetVec3f16");
}
