/**************************************************************************
 * render : contains the 3dlib rendering routines.                        *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick and Dan Duncalf                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        * *
 **************************************************************************
 *                                                                        *
 * Written : March 1993                                                   *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, New Technologies Group, Inc.	           	  *
 * All rights reserved						          *
 * This document is proprietary and confidential			  *
 *								          *
 **************************************************************************/
#include "Portfolio.h"
#include "Utils3DO.h"

#include "3dlib.h"

#define LINKCHANGE
extern int32 Debug_flag;
extern printfF16 (char *name, frac16 value);

CCB *HeadCCB = NULL;
CCB *LastCCB = NULL;

/**************************************************************************
 * CalcCameraField : calculates the field-width value, wo_CameraField,    *
 *     needed for use with the CAMERA_PERSPECTIVE option.                 *
 **************************************************************************
 * NOTE : This is called by RenderObjects() if the camera-lens field is   *
 *     set, which implies that at least one of the objects will use this  *
 *     value.                                                             *
 **************************************************************************/
void
CalcCameraField (Render3DPort *rport, WorldObject *cam)
{

  frac16 half_fov, cos_half_fov, sin_half_fov, tan_half_fov;
  frac16 field_width;
  frac16 tempf16;

  cam->wo_DistToOrigin = VectorLength (cam->wo_WorldPosition);

  /*** solve for the field-scaling factor. This causes the image to shrink the
   *further
   *** we get from it AND takes into account the "magnification" the camera is
   *using. ***/

  half_fov = Convert32_F16 ((cam->wo_CameraLens >> 1));

  cos_half_fov = CosF16 (half_fov);
  sin_half_fov = SinF16 (half_fov);

  tan_half_fov = DivSF16 (sin_half_fov, cos_half_fov);

  field_width = MulSF16 (tan_half_fov, cam->wo_DistToOrigin);

  /*** get half the display width ***/

  tempf16 = Convert32_F16 (rport->rp_Width);
  tempf16 = (tempf16 >> 1); /* div-by-2 */

  cam->wo_CameraField = DivSF16 (
      tempf16, field_width); /* pixels-per-unit at the origins distance */

  /*
  {
  printf("cam=%lx\n",cam);
  printfF16("field_width=",field_width);
  printfF16("dist_to_origin=",cam->wo_DistToOrigin);
  printfF16("half_fov=",half_fov);
  printfF16("Camera x=",cam->wo_WorldPosition[X_COORD]);
  printfF16("Camera y=",cam->wo_WorldPosition[Y_COORD]);
  printfF16("Camera z=",cam->wo_WorldPosition[Z_COORD]);
  }
  */
}

/**************************************************************************
 * CameraMapping : Maps a face to screen coords using the camera          *
 *     perspective model.                                                 *
 **************************************************************************/
int32
CameraMapping (Render3DPort *rport, WorldObject *object, WorldObject *cam,
               int32 *vertex_array, int32 *result, int32 pointcount)
{
  vec3f16 tempvec;
  frac16 dist_to_vertex;
  frac16 persp;
  frac16 tempf16;
  int32 temp32;
  int32 i;

  ENTER ("CameraMapping");

  /*** this is a mildly bogus error trap. If the user forgot to let the
   *cameralens, and trys
   *** to use CameraMapping, nutt'n will show. So, trap this condx here and
   *give a default
   *** CameraLens value. I can't do that at normal initialization since the
   *camera object at
   *** that time has no knowledge of the existance of any cameramapped objects.
   ****/

  if (cam->wo_CameraLens == 0)
    {
      cam->wo_CameraLens = 32;
      CalcCameraField (rport, cam);
    }

  for (i = 0; i < pointcount; i++)
    {
      tempvec[0] = *vertex_array;
      tempvec[1] = *(vertex_array + 1);
      tempvec[2] = *(vertex_array + 2);

      dist_to_vertex = VectorLength (tempvec);

      persp = DivSF16 (dist_to_vertex, cam->wo_DistToOrigin);

      /***
       * sp[i].pt_X=field_scale*(*(vertex_array+X_COORD)/persp)+rport->rp_Center_x;
       * ***/

      tempf16 = DivSF16 (*(vertex_array + X_COORD), persp);
      tempf16 = MulSF16 (tempf16, cam->wo_CameraField);

      temp32 = ConvertF16_32 (tempf16);

      *(result++) = temp32 + rport->rp_Center_x;

      /***
       * sp[i].pt_Y=screen_height-(field_scale*(*(vertex_array+Y_COORD))/persp+rport->rp_Center_y);
       * ***/

      tempf16 = DivSF16 (*(vertex_array + Y_COORD), persp);
      tempf16 = MulSF16 (tempf16, cam->wo_CameraField);

      temp32 = ConvertF16_32 (tempf16);

      *(result++) = rport->rp_Height - (temp32 + rport->rp_Center_y);
      vertex_array += 3;
    }

  EXIT ("CameraMapping");

  return (1);
}

/**************************************************************************
 * DoRotateMath : selects which version of an object is visible, and      *
 *     performs the local rotation on it. If the object is out of range   *
 *     and hence not processed, a -1 is returned, otherwise the version   *
 *     index is returned.                                                 *
 *                                                                        *
 *     Note that the object's centerpoint, the wo_LocalPosition has       *
 *     already been calculated and is used to determine the final         *
 *     position of all of the vertices for a given object. LocalPosition  *
 *     calculated based on the camera's wo_ObjectMatrix in                *
 *     GetLocalPositions() (called from BuildCameraList()/process_3d.c    *
 **************************************************************************
 * NOTE : This will put the rotated data into the CurrentPointHeap, and   *
 *     adjusts the objects own TransformedData pointer to look right      *
 *     at that data.                                                      *
 **************************************************************************/
Err
DoRotateMath (WorldPort *world, WorldObject *obj)
{
  ModelDefinition *model;
  GeoDefinition *geo;
  char *heap_ptr;
  int32 rvalue;
  point3f16 *vertex;
  int32 diff;
  frac16 z;
  frac16 stereo_offset;
  frac16 tempf16;
  IntMeshDefs *imdefs;

  ENTER ("DoRotateMath");

  rvalue = -1;

  model = (ModelDefinition *)obj->wo_ObjectDefinition;

  /*** If this is an IMESH model, calculate the parent (source) data and stuff
   *it into the
   *** the heap. After this routine, we then pick it up off of the heap and
   *generate the interpolated
   *** mesh points. ***/

  if (model->md_Flags
      & IMESH_MODEL) // hey! We got ourselves an interpolated mesh!
    {
      imdefs = (IntMeshDefs *)obj->wo_SourceData;
      geo = imdefs->im_ParentGdef;
    }
  else
    {
      geo = model->md_GeoDefs[obj->wo_ScaleUsed];
    }
  /*
  if(Debug_flag)
  {
  printf("object=%lx, model=%lx, geo=%lx,
  scaleused=%ld\n",obj,model,geo,obj->wo_ScaleUsed); printf("geo0=%lx,
  geo1=%lx\n",&model->md_GeoDefs[0],&model->md_GeoDefs[1]); printmatrix("matrix
  :",obj->wo_ResultMatrix);

  }
  */
  if (obj->wo_ScaleUsed >= 0)
    {
      obj->wo_TransformedData = (int32 *)world->wp_CurrentPointHeap;

      /*** transform the object and stuff its data into the heap ***/

      /*
      if(Debug_flag)
      {
      int32 j;

      vertex=(point3f16 *)geo->gd_PointList;
      for(j=0;j<geo->gd_PointCount;j++)
      {

      printf("source point %ld\n",j);
      printfF16("x=",vertex->x);
      printfF16("y=",vertex->y);
      printfF16("z=",vertex->z);
      vertex++;
      }

      printf("c-heap=%lx, t_data=%lx\n",
         world->wp_CurrentPointHeap,obj->wo_TransformedData);

      printf("REsultMatrix=%lx, pointcount=%ld, pointlist=%lx\n",
         &obj->wo_ResultMatrix,geo->gd_PointCount,geo->gd_PointList);
      }
      */

      MulManyVec3Mat33_F16 ((vec3f16 *)obj->wo_TransformedData,
                            (vec3f16 *)geo->gd_PointList, obj->wo_ResultMatrix,
                            geo->gd_PointCount);

      vertex = (point3f16 *)obj->wo_TransformedData;

      // printf("point count=%ld\n",geo->gd_PointCount);

      /*** if wp_Eye is the RIGHT_EYE, or if it is <0 which is mono-mode, do
       *the
       *** normal translation ***/

      if (world->wp_Eye == LEFT_EYE)
        {
          z = obj->wo_LocalPosition[Z_COORD];

          tempf16 = DivSF16 (z, z + world->wp_EyeZDist);
          stereo_offset = MulSF16 (tempf16, world->wp_EyeSep);

          obj->wo_LocalPosition[X_COORD] += stereo_offset;
          VectorTranslate ((point3f16 *)vertex,
                           (vec3f16 *)obj->wo_LocalPosition,
                           geo->gd_PointCount);
          obj->wo_LocalPosition[X_COORD] -= stereo_offset;
        }
      else
        VectorTranslate ((point3f16 *)vertex, (vec3f16 *)obj->wo_LocalPosition,
                         geo->gd_PointCount);

      heap_ptr = (char *)world->wp_CurrentPointHeap + geo->gd_PointCount * 12;

      /*** if we overflow the Heap, render this object invisible ***/

      diff = ((int32)heap_ptr - (int32)world->wp_PointHeapPtr);

      if (diff > world->wp_PointHeapSize)
        {
          printf ("object overflow! Can't rotate object %lx\n", obj);
          rvalue = -1;
        }
      else
        {
          world->wp_CurrentPointHeap = heap_ptr;
          rvalue = 0;
        }
    }
  else
    {
      rvalue = -1;
    }

  EXIT ("DoRotateMath");

  return (rvalue);
}

/**************************************************************************
 * DrawModel : draws a given object                                       *
 **************************************************************************
 * NOTE : The world pointer supplied is not used in this current version, *
 *     but is supplied "just in case" we might need it later on.          *
 **************************************************************************/
int32
DrawModel (Render3DPort *rport, WorldPort *world, WorldObject *model,
           WorldObject *cam)
{
  int32 flags; /* wo_ObjectFlags */
  int32 number_of_surfaces;
  int32 cx, cy;
  struct ModelDefinition *model_def;
  int32 i, j;
  int32 face_id;
  point3f16 *vertex_list;
  Point sp[4];
  int32 *surface_list;
  int32 *sorted_list;
  int32 point_id;
  GeoDefinition *geo;
  ModelSurface *surfacetype_list, *surfacetype;
  int32 model_scale = 0;
  struct GrafCon gcon;
  int32 facevisible;
  int32 faceclip = 0;
  int32 minus_z_vertex; /* TRUE if any vertex is -z, CULLING3 flag */
  point3f16 *vertex_array[4];
  int32 face_offset;
  int32 *spptr; /* pointer to screen points. */
  int32 error;
  // CCB *ccb;

  ENTER ("DrawModel");

  // printf("DrawModel!\n");

  facevisible = minus_z_vertex
      = FALSE; /* TRUE if any vertex is -z, CULLING3 flag */

  flags = model->wo_ObjectFlags;

  cx = rport->rp_Center_x;
  cy = rport->rp_Center_y;

  model_scale = model->wo_ScaleUsed;

  if (model_scale < 0)
    return (-1);

  model_def = (struct ModelDefinition *)model->wo_ObjectDefinition;

  geo = model_def->md_GeoDefs[model_scale];

  surface_list = (int32 *)geo->gd_SurfaceList;
  sorted_list = (int32 *)geo->gd_SortedList;

#ifdef ERROR_CHECK
  if (!surface_list)
    return (-1);
#endif

  vertex_list = (struct point3f16 *)model->wo_TransformedData;

  if ((char *)world->wp_CurrentPointHeap + (geo->gd_PointCount * 6)
          - (char *)world->wp_PointHeapPtr
      > world->wp_PointHeapSize)
    printf ("#Error DrawModel:  Not enough PointHeap space to draw model!!!!");

  spptr = (int32 *)world->wp_CurrentPointHeap;
  (*model->wo_PerspProc) (rport, model, cam, (int32 *)vertex_list,
                          (int32 *)spptr, geo->gd_PointCount);

  /*** dump the point list ***/

  /*
  {
  point3f16 *vertex;

  vertex=vertex_list;
  for(i=0;i<10;i++)
  {
     printf("vertex=%lx, x=%lx, y=%lx,
  z=%lx\n",vertex,vertex->x,vertex->y,vertex->z); vertex++;
  }
  }

  */
  surfacetype_list = (ModelSurface *)geo->gd_SurfaceType;
  surfacetype = surfacetype_list;

#ifdef ERROR_CHECK
  if (!surfacetype)
    return (-1);
#endif

  number_of_surfaces = geo->gd_SurfaceCount;

  /*** a little precalculation here to save a trivial amount of time ***/

  faceclip = (flags & FACE_CULLING);

  for (i = 0; i < number_of_surfaces; i++)
    {
      if (flags & FACE_SORTING)
        face_id = sorted_list[i];
      else
        face_id = i;

      /*
      if(face_id==0)
         printf("geodef=%lx, face_id=%ld,
      sorted_list=%lx\n",geo,face_id,sorted_list);
      */
      facevisible = FALSE;
      minus_z_vertex = FALSE;

      face_offset = face_id << 2;

      for (j = 0; j < 4; j++)
        {
          /*** collect together all 4 quad points ***/

          point_id = surface_list[face_offset + j];
          vertex_array[j] = (struct point3f16 *)(vertex_list + point_id);
          sp[j].pt_X = *(spptr + (point_id << 1));
          sp[j].pt_Y = *(spptr + (point_id << 1) + 1);

          /*
          if(flags&FACE_CULLING2)
          {
          printf("vertex %ld, point id=%ld\n",j,point_id);
          printfF16("x=",vertex_array[j]->x);
          printfF16("y=",vertex_array[j]->y);
          printfF16("z=",vertex_array[j]->z);
          }
          */

          if (flags & FACE_OVERLAP)
            if (vertex_array[j]->z < 0)
              minus_z_vertex = TRUE;
        }

      if (model->wo_FaceCullingProc)
        {
          facevisible = (*model->wo_FaceCullingProc) (world, model, geo,
                                                      face_id, vertex_array);
        }
      else
        facevisible = TRUE; /* always TRUE when there is no face-Culling */

      /*
      if(face_id==0)
      printf("face %ld, visibility=%ld\n",i,facevisible);
      */
      if (!model->wo_RenderTimeProc)
        {
          /*** check to see if QuadInView() only if ALL of a faces
           *** vertices were +z, meaning the face is right in front
           *** of us, the most likely culling condition in which
           *** the cone culling check will render a FALSE reading. ***/

          if ((flags & FACE_OVERLAP) && (!minus_z_vertex))
            {
              if (QuadInView (rport, sp))
                facevisible = TRUE;
            }

          if (facevisible)
            {
              int32 forewards;

              if (flags & CULL_BACKFACE)
                {
                  forewards = IsFrontFace (sp);

                  /*
                   if(face_id==0)
                  printf("culling backface, forewards=%ld\n",forewards);
                  */
                }
              else
                forewards = 1;

              if (forewards)
                {
                  surfacetype = surfacetype_list + face_id;
                  /*
                  ccb=(CCB *)surfacetype->ms_Control;
                  printf("surfacetype=%lx, Pixc=%lx, object=%lx,
                  ccb=%lx\n",surfacetype,ccb->ccb_PIXC,model,ccb);
                  */

                  MapCelQuick ((CCB *)surfacetype->ms_Control, sp);
                  // printf("drawing cels\n");

                  if (flags & FAST_OBJECT)
                    {
                      if (LastCCB == NULL)
                        HeadCCB = (CCB *)surfacetype->ms_Control;
                      else
                        LinkCel (LastCCB, (CCB *)surfacetype->ms_Control);

                      LastCCB = (CCB *)surfacetype->ms_Control;
                    }
                  else
                    {
/*
if(face_id==0)
{
printf("rendering face %ld\n",i);
printf("1 x=%ld, y=%ld\n",sp[0].pt_X,sp[0].pt_Y);
printf("2 x=%ld, y=%ld\n",sp[1].pt_X,sp[1].pt_Y);
printf("3 x=%ld, y=%ld\n",sp[2].pt_X,sp[2].pt_Y);
printf("4 x=%ld, y=%ld\n",sp[3].pt_X,sp[3].pt_Y);
}
*/
#ifdef LINKCHANGE
                      if (LastCCB != NULL) /*  FLUSH LIST  */
                        {
                          LastCCB->ccb_Flags |= 0x40000000;
                          DrawCels (rport->rp_BitmapItem, HeadCCB);
                          LastCCB = NULL;
                        }
#endif
                      error = DrawCels (rport->rp_BitmapItem,
                                        (CCB *)surfacetype->ms_Control);

                      if (error < 0)
                        printf ("DrawCels error, %ld\n", error);
                    }
                }
            }
        }
      else /* else if(model->wo_RenderTimeProc) */
        {
#ifdef LINKCHANGE
          if (LastCCB != NULL) /* Flush linked list to call user routine */
            {
              LastCCB->ccb_Flags |= 0x40000000;
              DrawCels (rport->rp_BitmapItem, HeadCCB);
              LastCCB = NULL;
            }

#endif
          (*model->wo_RenderTimeProc) (world, model, face_id, sp);
        }

      /*** This stuff will outline the object so you can
       *** see each face more clearly. ***/

      if (flags & WIREFRAME && facevisible)
        {
          MoveTo (&gcon, (Coord)sp[0].pt_X, (Coord)sp[0].pt_Y);

          for (j = 1; j <= 3; j++)
            {
              DrawTo (rport->rp_BitmapItem, &gcon, (Coord)sp[j].pt_X,
                      (Coord)sp[j].pt_Y);
            }

          DrawTo (rport->rp_BitmapItem, &gcon, (Coord)sp[0].pt_X,
                  (Coord)sp[0].pt_Y);
        }

      /*
          surfacetype=surfacetype_list+face_id;
      printf("surfacetype=%lx, surfacetype_list=%lx,
      face_id=%ld\n",surfacetype,surfacetype_list,face_id);
      */
    } /*  end for(i=0;i<number_of_surfaces;i++) */

  EXIT ("DrawModel");

  return (0);
}

/**************************************************************************
 * LookAt : Similar to SnapShot except aims the camera based the          *
 *     target/camera/roll model. Used instead of SnapShot.                *
 **************************************************************************/
void
LookAt (WorldPort *world, Obj3D camera_id, vec3f16 target, frac16 roll)
{
  WorldObject *cam;

  ENTER ("LookAt");

  world->wp_CurrentPointHeap = world->wp_PointHeapPtr;
  cam = GetCameraAddress (world, camera_id);

  PreProcessCallbacks (world, camera_id);

  LookAtMatrix (cam->wo_ObjectMatrix, cam->wo_WorldPosition, target, roll);
  BuildCameraList (world, camera_id);

  RotateObjects (world, camera_id);       /* World transform everything */
  DoCollisionHandling (world, camera_id); /* handles collision detection */

  SortCameraList (world, camera_id);

  EXIT ("LookAt");
}

/**************************************************************************
 * PseudoMapping : Maps a face to screen coords using the pseudo          *
 *     perspective model.                                                 *
 **************************************************************************/
int32
PseudoMapping (Render3DPort *rport, WorldObject *object, WorldObject *cam,
               int32 *vertex_array, int32 *result, int32 pointcount)
{
  int32 i;
  frac16 fieldscale;
  static mat33f16 mat;
  static int32 firsttime = TRUE;
  mmv3m33d mm3;
  point3f16 *vect;
  int32 x32, y32;
  frac16 xf16, yf16;

  ENTER ("PseudoMapping");

  fieldscale = (cam->wo_FieldScale << 16);

  if (firsttime)
    {
      /*** create a general-matrix ***/
      BuildXYZ (0, 0, 0, mat);
      firsttime = FALSE;
    }

  mm3.src = (vec3f16 *)vertex_array;
  mm3.dest = (vec3f16 *)malloc (sizeof (vec3f16) * pointcount);
  mm3.mat = &mat;
  mm3.n = fieldscale;
  mm3.count = pointcount;

  MulManyVec3Mat33DivZ_F16 (&mm3);

  vect = (point3f16 *)mm3.dest;

  for (i = 0; i < pointcount; i++)
    {
      xf16 = vect->x;
      yf16 = vect->y;

      x32 = ConvertF16_32 (xf16);
      y32 = ConvertF16_32 (yf16);

      *(result++) = x32 + rport->rp_Center_x;
      *(result++) = rport->rp_Height - (y32 + rport->rp_Center_y);

      vect++;
    }

  free (mm3.dest);

  EXIT ("PseudoMapping");

  return (1);
}

/**************************************************************************
 * RenderSound : "Renders" a sound, either 3D or simple                   *
 **************************************************************************/
int32
RenderSound (WorldPort *world, WorldObject *soundobject, Obj3D camera_id)
{
  Sound3D *sound3d;
  SoundDefinition *sounddef;
  vec3f16 vector;
  frac16 radius;
  frac16 sounddelta;
  frac16 tempf16, vol_decrease, volume;
  frac16 fudgefactor, volumefraction;
  int32 leftvolume, rightvolume;
  frac16 angle, deltaangle;
  int32 maxvolume;
  int32 temp32;

  ENTER ("RenderSound");

  vol_decrease = 0;
  leftvolume = rightvolume = 0;
  deltaangle = 0;

  sounddef = soundobject->wo_SoundDef;

  if (!(sounddef->sd_Flags & SOUND_ON))
    return (0);

  if (soundobject->wo_SimpleSoundProc)
    {
      (*soundobject->wo_SimpleSoundProc) (world, soundobject);
      return (0);
    }

  /*** convert the point to sphereical coords (right now we use
   *** polar coordinates since 3dsound stuff is only actually, 2d ***/

  vector[X_COORD] = soundobject->wo_LocalPosition[X_COORD];
  vector[Y_COORD] = soundobject->wo_LocalPosition[Y_COORD];
  vector[Z_COORD] = soundobject->wo_LocalPosition[Z_COORD];

  /*
  printf("Soundobject, x=%lx, z=%lx\n",soundobject->wo_LocalPosition[X_COORD],
                  soundobject->wo_LocalPosition[Z_COORD]);
  */

  radius = VectorLength (vector);

  if (vector[X_COORD] == 0)
    vector[X_COORD] = 0x0001;

  if (soundobject->wo_ObjectFlags & SIMPLESOUNDTYPE
      && (radius < sounddef->sd_MaxSoundDistance))
    {
      /*** decrease the volume based on distance into the "decrease" range.
       *** 0x3fff0000 is the frac16 version of the max volume value, 0x3fff,
       *** used by Audiofolio. ***/

      sounddelta
          = (sounddef->sd_MaxSoundDistance - sounddef->sd_MinSoundDistance);

      if (sounddelta >= 0)
        {
          tempf16 = radius - sounddef->sd_MinSoundDistance;
          vol_decrease = DivSF16 (tempf16, sounddelta);
        }

      /*** the total volume of all inputs must not exceed 0x7fff ***/

      maxvolume = 0x00007fff / world->wp_TotalSimpleSounds;

      temp32 = (maxvolume << 16);

      volume = temp32 - MulSF16 (temp32, vol_decrease);

      /*** create pseudo-3d effect now to separate left-right ear intensities.
       *** As the soundsource favors one ear over another, the volume will go
       *up
       *** or down, the loudest for being either 90 or 270 degrees when it is
       *directly
       *** in line with either the left of right ear. I take the normal volume
       *and
       *** add a small fudgefactor of +- 12% (the volume/8) or so, (add to the
       *** closest ear, subtract from the farthest ear) ***/

      angle = Atan2F16 (vector[Z_COORD], vector[X_COORD]);

      if (angle >= 0
          && angle <= QUARTERCIRCLE) /* on the rightfront of the head */
        deltaangle = angle;
      else if (angle > QUARTERCIRCLE
               && angle <= HALFCIRCLE) /* on the rightback of the head */
        deltaangle = (HALFCIRCLE - angle);
      else if (angle > HALFCIRCLE
               && angle <= (192 << 16)) /* on the leftback of the head */
        deltaangle = (angle - HALFCIRCLE);
      else if (angle > (192 << 16)
               && angle <= FULLCIRCLE) /* on the leftfront of the head */
        deltaangle = (FULLCIRCLE - angle);

      fudgefactor = DivSF16 (deltaangle, QUARTERCIRCLE);
      volumefraction = MulSF16 ((volume >> 1), fudgefactor);

      if (angle >= 0
          && angle <= HALFCIRCLE) /* on the rightfront of the head */
        {
          rightvolume = (int32)(((volume >> 1) + (volumefraction >> 1)) >> 15);
          leftvolume = (int32)(((volume >> 1) - (volumefraction >> 1)) >> 15);

          /*
          printf("aaa angle=%ld, leftvol=%lx,
          rightvol=%lx\n",angle>>16,leftvolume,rightvolume);
          printfF16("volfraction=",volumefraction);
          printfF16("volume=",volume);
          printfF16("volume decrease=",vol_decrease);
          */
        }
      else if (angle > HALFCIRCLE
               && angle <= FULLCIRCLE) /* on the rightfront of the head */
        {
          rightvolume = (int32)(((volume >> 1) - (volumefraction >> 1)) >> 15);
          leftvolume = (int32)(((volume >> 1) + (volumefraction >> 1)) >> 15);
          /*
          printf("bbb angle=%ld, leftvol=%lx,
          rightvol=%lx\n",angle>>16,leftvolume,rightvolume);
          printfF16("volfraction=",volumefraction);
          printfF16("volume=",volume);
          */
        }

      if (rightvolume > maxvolume)
        rightvolume = maxvolume;
      if (leftvolume > maxvolume)
        leftvolume = maxvolume;

      // printf("bbb angle=%ld, leftvol=%lx,
      // rightvol=%lx\n",angle>>16,leftvolume,rightvolume);

      TweakKnob (soundobject->wo_SoundDef->sd_LeftGain, leftvolume);
      TweakKnob (soundobject->wo_SoundDef->sd_RightGain, rightvolume);
    }
  else if (soundobject->wo_ObjectFlags & SOUNDTYPE)
    {
      sound3d = (Sound3D *)soundobject->wo_SourceData;

      sounddef->sd_EndSound.pp4d_Theta
          = Atan2F16 (vector[Z_COORD], vector[X_COORD]);
      sounddef->sd_EndSound.pp4d_Phi = 0;
      sounddef->sd_EndSound.pp4d_Radius
          = (MulSF16 (radius, soundobject->wo_SoundDef->sd_SoundScale) >> 16);
      sounddef->sd_EndSound.pp4d_Time = Get3DSoundTime ();

      Move3DSound (sound3d, &sounddef->sd_StartSound, &sounddef->sd_EndSound);

      sounddef->sd_StartSound = sounddef->sd_EndSound;
    }

  EXIT ("RenderSound");

  return (0);
}

/**************************************************************************
 * RenderWorld : renders everything                                       *
 **************************************************************************/
void
RenderWorld (Render3DPort *rport, WorldPort *world, Obj3D camera_id)
{
  WorldObject *obj, *cam, *soundobject;
  int32 i;
  int32 sound_id;
  int32 index;

  ENTER ("RenderWorld");

  cam = GetCameraAddress (world, camera_id);

  if (cam->wo_CameraLens)
    {
      CalcCameraField (rport, cam);
    }

  /*** render from farthest to nearest ***/

  index = (world->wp_MaxObjects) * (camera_id + 2);

  for (i = world->wp_ObjectCount[camera_id] - 1; i >= 0; i--)
    {
      LastCCB = NULL;

      obj = (WorldObject *)world->wp_UserObjects[index + i];

      if (obj->wo_ScaleUsed >= 0)
        {
          if (obj->wo_LightPrefs & 0x000000f0)
            {
              LightModel (world, obj, camera_id, CONSTANT_ILLUMINATION);
              // printf("lighting object %lx, i=%ld\n",obj,i);
            }

          DrawModel (rport, world, obj, cam);

          /*** notice the check for the validity of LastCCB, this is needed in
           *case the user switches in
           *** their own renderproc, which would supercede this block of code.
           ****/

          if (obj->wo_ObjectFlags & FAST_OBJECT && LastCCB)
            {
              LastCCB->ccb_Flags |= 0x40000000;
              DrawCels (rport->rp_BitmapItem, HeadCCB);
              LastCCB = NULL;
            }
        }
    }

  for (i = 0; i < MAX3DSOUNDS; i++)
    {
      sound_id = world->wp_SoundHandles[i];

      if (sound_id >= 0)
        {
          soundobject = (WorldObject *)world->wp_UserObjects[sound_id];
          RenderSound (world, soundobject, camera_id);
        }
    }

  for (i = 0; i < MAXSIMPLESOUNDS; i++)
    {
      sound_id = world->wp_SimpleSoundHandles[i];

      if (sound_id >= 0)
        {
          soundobject = (WorldObject *)world->wp_UserObjects[sound_id];
          RenderSound (world, soundobject, camera_id);
        }
    }

  EXIT ("RenderWorld");
}

/**************************************************************************
 * RotateObjects : rotates everything                                     *
 **************************************************************************/
void
RotateObjects (WorldPort *world, Obj3D camera_id)
{
  WorldObject *obj, *cam, *sound, *parent;
  int32 sound_id;
  int32 i, j;
  vec3f16 root_direction;
  mat33f16 transpose;
  ModelDefinition *model;
  GeoDefinition *geo;
  int32 num_objects = 0;
  int32 objects_processed = 0;
  int32 heirarchy_level = 0;

  // point3f16 *center;

  ENTER ("RotateObjects");

  cam = GetCameraAddress (world, camera_id);

  world->wp_CurrentPointHeap = world->wp_PointHeapPtr;

  /*** rotate models ***/

  if (world->wp_Eye >= 0)
    world->wp_Eye = (world->wp_Eye == RIGHT_EYE) ? LEFT_EYE : RIGHT_EYE;

  num_objects = world->wp_ObjectCount[camera_id] - 1;

  i = num_objects;
  // printf("aaaa, num_objects=%ld\n",num_objects);
  while (objects_processed <= num_objects)
    {

      // printf("objects processed=%ld, heir
      // level=%ld\n",objects_processed,heirarchy_level);

      for (i = num_objects; i >= 0; i--)
        {
          /*** Get the object pointer from the wp_SortedObjects array. We must
           *** index off of the UserObjects array, instead of going directly to
           *** SortedObjects, since the user can define the size of UserObjects
           *via
           *** UserDefs3D.h. So when this code becomes a system level routine,
           *it
           *** will lose the knowledge of how big UserObjects is. ***/

          obj = (WorldObject *)world
                    ->wp_UserObjects[(world->wp_MaxObjects) * (camera_id + 2)
                                     + i];

          if (obj->wo_HeirLevel == heirarchy_level)
            {
              // printf("processing object  %lx\n",obj);
              if (heirarchy_level == 0)
                {
                  GlueMulMat33Mat33_F16 (obj->wo_ResultMatrix,
                                         obj->wo_ObjectMatrix,
                                         cam->wo_ObjectMatrix);

                  objects_processed++;
                }
              else
                {
                  // printf("processing heir object  %lx\n",obj);

                  parent = obj->wo_ParentObject;

                  GlueMulMat33Mat33_F16 (obj->wo_ResultMatrix,
                                         obj->wo_ObjectMatrix,
                                         parent->wo_ResultMatrix);

                  MulVec3Mat33_F16 (obj->wo_LocalPosition,
                                    obj->wo_WorldPosition,
                                    obj->wo_ResultMatrix);

                  obj->wo_LocalPosition[X_COORD]
                      += (parent->wo_LocalPosition[X_COORD]);
                  obj->wo_LocalPosition[Y_COORD]
                      += (parent->wo_LocalPosition[Y_COORD]);
                  obj->wo_LocalPosition[Z_COORD]
                      += (parent->wo_LocalPosition[Z_COORD]);

                  objects_processed++;
                }

              if (SetModelScale (world, obj, NON_MESH_GEOMETRY) >= 0)
                {
                  DoRotateMath (world, obj);
                }
              else /* model isn't visible, so skip to the next i */
                {
                  continue;
                }

              model = (ModelDefinition *)obj->wo_ObjectDefinition;

              if (model->md_Flags & IMESH_MODEL)
                {
                  GenerateImeshData (world, obj);
                }

              geo = model->md_GeoDefs[obj->wo_ScaleUsed];

              if (geo->gd_FaceCenters) // transform any face-center data if it
                                       // is allocated
                {
                  MulManyVec3Mat33_F16 ((vec3f16 *)geo->gd_FaceCentersXformed,
                                        (vec3f16 *)geo->gd_FaceCenters,
                                        obj->wo_ResultMatrix,
                                        geo->gd_SurfaceCount);

                  VectorTranslate ((point3f16 *)geo->gd_FaceCentersXformed,
                                   (vec3f16 *)obj->wo_LocalPosition,
                                   geo->gd_SurfaceCount);
                } // end if(face_centers. . .
            }
        } // for(i=. . .

      heirarchy_level++;
    } // while(objects_processes. . .

  /*** rotate the camera's vector which is used for determining the trajectory
   *** during flythroughs ***/

  root_direction[X_COORD] = 0;
  root_direction[Y_COORD] = 0;
  root_direction[Z_COORD] = cam->wo_ObjectVelocity;

  /* this wants to crash the machine
    Transpose33_F16(transpose,cam->wo_ObjectMatrix);
  */

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        transpose[i][j] = cam->wo_ObjectMatrix[j][i];
    }

  MulVec3Mat33_F16 (cam->wo_ObjectDirection, root_direction, transpose);

  /*** rotate sound objects ***/

  for (i = 0; i < MAX3DSOUNDS; i++)
    {
      sound_id = world->wp_SoundHandles[i];

      if (sound_id >= 0)
        {
          sound = (WorldObject *)world->wp_UserObjects[sound_id];

          GlueMulMat33Mat33_F16 (sound->wo_ResultMatrix,
                                 sound->wo_ObjectMatrix, cam->wo_ObjectMatrix);

          MulVec3Mat33_F16 (sound->wo_LocalPosition, sound->wo_WorldPosition,
                            sound->wo_ResultMatrix);

          /*** one might expect that VectorTranslate() would work here, and one
           *would
           *** also be totally wrong, since VT() adds the vectors, and here we
           *** need to subtract them. ***/

          sound->wo_LocalPosition[X_COORD] -= cam->wo_WorldPosition[X_COORD];
          sound->wo_LocalPosition[Y_COORD] -= cam->wo_WorldPosition[Y_COORD];
          sound->wo_LocalPosition[Z_COORD] -= cam->wo_WorldPosition[Z_COORD];
          /*
          printf("Soundlocal Z=%lx, SoundWorld Z=%lx, camera
          Z=%lx\n",sound->wo_LocalPosition[Z_COORD],
          sound->wo_WorldPosition[Z_COORD],cam->wo_LocalPosition[Z_COORD]);
          */
        }
    }

  for (i = 0; i < MAXSIMPLESOUNDS; i++)
    {
      sound_id = world->wp_SimpleSoundHandles[i];

      if (sound_id >= 0)
        {
          sound = (WorldObject *)world->wp_UserObjects[sound_id];

          GlueMulMat33Mat33_F16 (sound->wo_ResultMatrix,
                                 sound->wo_ObjectMatrix, cam->wo_ObjectMatrix);

          MulVec3Mat33_F16 (sound->wo_LocalPosition, sound->wo_WorldPosition,
                            sound->wo_ResultMatrix);

          sound->wo_LocalPosition[X_COORD] -= cam->wo_WorldPosition[X_COORD];
          sound->wo_LocalPosition[Y_COORD] -= cam->wo_WorldPosition[Y_COORD];
          sound->wo_LocalPosition[Z_COORD] -= cam->wo_WorldPosition[Z_COORD];
        }
    }

  EXIT ("RotateObjects");
}

/**************************************************************************
 * SetModelScale : selects which scale of object is to be displayed       *
 *    and returns it, AND sticks it in the wo_ScaleUsed field.            *
 **************************************************************************
 * NOTE : the object_type is needed for recursive calls, to identify      *
 *    if this is being called for a mesh or non-mesh object. In the case  *
 *    where we are using a mesh object and come closer than it's min      *
 *    dist, this gets into an infinit recursive loop, and bad things      *
 *    happen.                                                             *
 **************************************************************************/
int32
SetModelScale (WorldPort *world, WorldObject *object, int32 object_type)
{
  int32 z;
  ModelDefinition *model;
  GeoDefinition *geo;
  int32 i;
  int32 number_of_versions;
  static int32 counter = 0;

  ENTER ("SetModelScale");

  geo = NULL;

  z = object->wo_LocalPosition[Z_COORD];
  if (z < 0)
    z = -z;

  model = (ModelDefinition *)object->wo_ObjectDefinition;

  number_of_versions = model->md_NumberOfScales;

  /*** decided which version of the object is visible based on the
   *** distance, and detail level specs. First see if it is still within
   *** the boundries of the last scale used. If not, find another suitable
   *** scale. This permits the overlapping boundry conditions which
   *** may be used to prevent flashing between one scale and another when
   *** the object is right on a boundry. ***/

  if (object->wo_ScaleUsed != -1)
    geo = model->md_GeoDefs[object->wo_ScaleUsed];

  /*** if TRUE, we've gone into a new scale region, so find out which one
   *** it is. ***/
  /*
  if(object->wo_UserDefinedData==7)
  {
  printf("geo=%lx, Number of versions=%ld\n",geo,number_of_versions);
  printf("object->wo_ScaleUsed=%ld\n",object->wo_ScaleUsed);
  printfF16("z=%",z);
  printf("RangeMin=%ld,
  RangeMax=%ld\n",geo->gd_RangeMin>>16,geo->gd_RangeMax>>16);
  }
  */

  /*** this first segment checks for the case when the last scale was
   *** in view, the second one has to do with the special case when the
   *** object is out of range and comes back into range. ***/

  if (object->wo_ScaleUsed >= 0)
    {
      if ((z < geo->gd_RangeMin) || (z > geo->gd_RangeMax))
        {
          for (i = 0; i < number_of_versions; i++)
            {
              geo = model->md_GeoDefs[i];

              if ((z >= geo->gd_RangeMin) && (z <= geo->gd_RangeMax))
                {

                  /*** if the scale is changed ***/

                  if ((object->wo_ScaleUsed != i) && (object->wo_ScaleProc))
                    (*object->wo_ScaleProc) (world, model, i);

                  object->wo_ScaleUsed = i;
                  break;
                }

              if (i == (number_of_versions - 1))
                {
                  if ((object->wo_ScaleUsed >= 0) && (object->wo_ScaleProc))
                    (*object->wo_ScaleProc) (world, model, -1);

                  object->wo_ScaleUsed = -1; /* object out of range */
                  break;
                }
            }
        }
    }
  else /* see if we're coming back into view */
    {
      for (i = 0; i < number_of_versions; i++)
        {
          geo = model->md_GeoDefs[i];

          if ((z >= geo->gd_RangeMin) && (z <= geo->gd_RangeMax))
            {

              /*** if the scale is changed ***/

              if (object->wo_ScaleProc)
                (*object->wo_ScaleProc) (world, model, i);

              object->wo_ScaleUsed = i;
              break;
            }
        }
    }

  counter++;
  // printf("aaaa, %ld\n",counter);
  /*** if the object is out of range, see if it is closer then allowed, and if
   *so,
   *** check to see if this is a mesh object. If that is the case we'll use the
   *mesh
   *** version instead. ***/

  if (object->wo_ObjectFlags & MESH_OBJECT)
    {
      if ((object->wo_ScaleUsed < 0) && (z <= geo->gd_RangeMin))
        {
          // printf("cccc, %ld\n",counter);
          object->wo_ObjectDefinition = object->wo_MeshDefinition;

          /*** only make this recursive call when transitioning from non-mesh
           *** geomtry to mesh geometry. ***/

          if (object_type == NON_MESH_GEOMETRY)
            SetModelScale (world, object, MESH_GEOMETRY);
        }
      else if ((object->wo_ScaleUsed < 0) && (z > geo->gd_RangeMax))
        {
          object->wo_ObjectDefinition = object->wo_ModelDefinition;
          SetModelScale (world, object, NON_MESH_GEOMETRY);
        }
    }

  /*
  if(object->wo_UserDefinedData==7)
  printf("scaleused=%ld\n",object->wo_ScaleUsed);
  */

  EXIT ("SetModelScale");

  return (object->wo_ScaleUsed);
}

/**************************************************************************
 * Snapshot : clip all objects to the ViewCone and Z-Sort them.           *
 *     It also calls PreProcessObject if it has been defined.             *
 **************************************************************************/
void
Snapshot (WorldPort *world, Obj3D camera_id)
{
  WorldObject *cam;

  ENTER ("Snapshot");

  world->wp_CurrentPointHeap = world->wp_PointHeapPtr;

  cam = GetCameraAddress (world, camera_id);

  PreProcessCallbacks (world, camera_id);

  ResolveObjectMatrix (world, cam);

  BuildCameraList (world, camera_id);

  RotateObjects (world, camera_id);       /* World transform everything */
  DoCollisionHandling (world, camera_id); /* handles collision detection */

  SortCameraList (world, camera_id);

  EXIT ("Snapshot");
}
