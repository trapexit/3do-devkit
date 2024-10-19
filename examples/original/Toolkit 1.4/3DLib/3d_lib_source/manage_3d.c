/**************************************************************************
 * manage_3d : contains the 3dlib object management and initialization    *
 *     routines.                                                          *
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
 * Copyright (c) 1993, The 3DO Company.					  *
 * All rights reserved						          *
 * This document is proprietary and confidential			  *
 *								          *
 **************************************************************************
 *                                                                        *
 * Revision history :                                                     *
 *                                                                        *
 * 1.1.2 : fixed the memory leaks in DestroyModel and CloneModel          * *
 *                                                                        *
 * 1.1.3 : fixed a bug in CloneModel that caused a crash with a new Sherry*
 *                                                                        *
 **************************************************************************/

#include "3dlib.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"
#include "event.h"

#define VERSION "1.2.0"

/*** this is initializes at the first call of InitSimpleSound() ***/

Item MixerIns = 0;

/*** Facemap is used by the mesh support ***/

int32 Facemap[4][4]
    = { { 0, 4, 8, 7 }, { 4, 1, 5, 8 }, { 8, 5, 2, 6 }, { 7, 8, 6, 3 } };

/*** globals needed by SwapScreens()/misc_3d.c ***/

int32 Eye;
int32 Render_screen, Left_screen, Right_screen;
int32 View_mode = MONO_VIEW;

/*** face rotation : CCW is the mathematically "correct" way of doing things,
 *but
 *** due to the HW the images end up backwards. This is what was done for
 *release 1.0.
 *** For 1.1, the default is the old way, but the user can opt to go with CW
 *faces
 *** if it would not be too costly for them to convert all of their files. ***/

int32 Face_rotation_flag;

/*** checked in certain areas ***/

int32 Lib_initialized = 0;

/**************************************************************************
 * AddModel  : adds an object to the master list. This is simply a        *
 *     front-end to Add3DObject(), but since sounds, cameras and lights   *
 *     are all objects as well, the name Add3DObject didn't make sense.   *
 **************************************************************************/
Obj3D
AddModel (WorldPort *world, WorldObject *object)
{
  Obj3D object_id;

  object_id = Add3DObject (world, object);

  return (object_id);
}

/**************************************************************************
 * Add3DObject  : adds an object to the master list                       *
 **************************************************************************/
Obj3D
Add3DObject (WorldPort *world, WorldObject *object)
{
  uint32 j;
  int32 flags;
  uint32 k;

  ENTER ("Add3DObject");

#if ERROR_CHECK

  k = world->wp_MaxObjects;

  /*** look for a free slot in the userobjects list ***/

  for (j = 0; (!(world->wp_UserObjects[j] == 0)) & (j < k); j++)
    ;

  if (j == k)
    return -1;

#else

  for (j = 0; (!(world->wp_UserObjects[j] == 0)); j++)
    ;

#endif

  world->wp_UserObjects[j] = object;
  world->wp_TotalObjectCount++;
  world->wp_HighestObjectIndex++;

  /*** if this is a camera do not call ResortLinks since that is only for
   *** models and it would accidentally place the camera inside the model
   *** process list. ***/

  flags = object->wo_ObjectFlags;

  if ((flags & CAMERATYPE) || (flags & SOUNDTYPE) || (flags & SIMPLESOUNDTYPE))
    {
      EXIT ("Add3DObject");

      return (j);
    }
  else
    {
      world->wp_TotalSortedObjects++;
      ResortLinks (world); /* adds models to the model only list */
    }

  EXIT ("Add3DObject");

  return (j);
}

/**************************************************************************
 * Add3DSound : add a 3D sound object to the 3d object lists              *
 **************************************************************************/
Obj3D
Add3DSound (WorldPort *world, WorldObject *object, int32 flags)
{
  Obj3D handle;
  uint32 j;

  ENTER ("Add3DSound");

#if ERROR_CHECK
  for (j = 0; (!(world->wp_SoundHandles[j] < 0)) && (j < MAX3DSOUNDS); j++)
    ;

  if (j == MAX3DSOUNDS)
    return -1;

#else
  for (j = 0; !(world->wp_SoundHandles[j] < 0); j++)
    ;
#endif
  //  object->wo_ObjectFlags |= SOUNDTYPE;

  handle = Add3DObject (world, object);

  if (handle >= 0)
    world->wp_SoundHandles[j] = handle;

  object->wo_SoundDef->sd_Flags |= SOUND_ON;

  /*
     object->wo_MatrixMethod=volume;
  */
  EXIT ("Add3DSound");

  return j;
}

/**************************************************************************
 * AddCamera : adds a camera object to the 3d object lists                *
 **************************************************************************/
Obj3D
AddCamera (WorldPort *world, WorldObject *object)
{
  Obj3D ReturnHandle;
  int32 j;
  int32 rvalue;

  /*** check to see if there is any free slots in the CameraHandles array.
   *** If full, bail out, if not, add the camera to the UserObjects list
   *** and stuff its index into the Camera array. ***/

#if ERROR_CHECK

  j = 0;

  for (j = 0; j < MAX3DCAMERAS; j++)
    {
      if (world->wp_CameraHandles[j] == -1)
        break;
    }

  if (j == MAX3DCAMERAS)
    return -1;
#else

  for (j = 0; j < MAX3DCAMERAS; j++)
    {
      if (world->wp_CameraHandles[j] == -1)
        break;
    }

#endif

  //  object->wo_ObjectFlags|=CAMERATYPE;

  ReturnHandle = Add3DObject (world, object);

  if (ReturnHandle >= 0)
    {
      /*** Update camera handles if UserHandle available ***/

      world->wp_CameraHandles[j] = ReturnHandle;
      rvalue = j;
    }
  else
    rvalue = -1;

  return rvalue;
}

/**************************************************************************
 * AddObject2List : adds an object to the visible object list, which      *
 *    is the first column of pointers in the sortedObjects list.          *
 **************************************************************************
 * WARNING : Recursive routine!                                           *
 **************************************************************************/
Err
AddObject2List (WorldPort *world, int32 object_id)
{
  int32 ni;

  WorldObject *object;

  object = (WorldObject *)world->wp_UserObjects[object_id];

  /*** extra safety check, since cameras cannot go into
   *** the SortedObject list ***/

  if (object->wo_ObjectFlags & CAMERATYPE)
    return (-1);
  if (object->wo_ObjectFlags & SOUNDTYPE)
    return (-1);
  if (object->wo_ObjectFlags & SIMPLESOUNDTYPE)
    return (-1);

  /*** If it is alreadly in the list don't add it again ***/

  if (object->wo_UpdateFlags & 0x80000000)
    return (-1);

  object->wo_UpdateFlags |= 0x80000000;

  /*** If attached to another object, add it first ***/
  /*
     if (object->wo_AttachedObject>=0)
          AddObject2List(world,object->wo_AttachedObject);
  */

  /*** Now actually update object count, and add it to list ***/

  ni = world->wp_Temp4++;

  world->wp_UserObjects[world->wp_MaxObjects + ni]
      = (void *)world->wp_UserObjects[object_id];

  return (0);
}

/**************************************************************************
 * AddSimpleSound : add a simple sound object to the 3d object lists.     *
 *     This returns the index in the wo_SimpleSoundHandle[].              *
 **************************************************************************
 * NOTE : This is a very high level routine in that will also load in     *
 *     the desired sound, the sampler code, attach the two and connect it *
 *     to the mixer. It would seem at first that a lot of this stuff      *
 *     should go in the Init routine, but I needed the knowledge of the   *
 *     list's index value (the handle) which is only derived here.        *
 **************************************************************************/
Obj3D
AddSimpleSound (WorldPort *world, WorldObject *object, int32 flags)
{
  Obj3D handle;
  int32 j;
  Item sampler_item;
  char knobname[10];
  int32 rvalue;

  ENTER ("AddSimpleSound");

#if ERROR_CHECK
  for (j = 0;
       (!(world->wp_SimpleSoundHandles[j] < 0)) && (j < MAXSIMPLESOUNDS); j++)
    ;

  if (j == MAXSIMPLESOUNDS)
    return -1;

#else
  for (j = 0; !(world->wp_SimpleSoundHandles[j] < 0); j++)
    ;
#endif
  //  object->wo_ObjectFlags |= SIMPLESOUNDTYPE;

  handle = Add3DObject (world, object);

  if (handle >= 0)
    world->wp_SimpleSoundHandles[j] = handle;

  sampler_item = object->wo_SoundDef->sd_Sampler;

  /*** build the name of the mixer's input channels, since this is for
   *** a stereo sound, we snare input channels in pairs. The first sound
   *** allocated uses "input0" and "input1", etc. ***/

  sprintf (knobname, "Input%d", (j * 2));
  rvalue = ConnectInstruments (sampler_item, "Output", MixerIns, knobname);
  if (rvalue < 0)
    printf ("left connection was bad! rvalue=%ld\n", knobname);

  sprintf (knobname, "Input%d", (j * 2 + 1));
  rvalue = ConnectInstruments (sampler_item, "Output", MixerIns, knobname);
  if (rvalue < 0)
    printf ("right connection was bad! rvalue=%ld\n", knobname);

  sprintf (knobname, "LeftGain%ld", (j * 2));
  object->wo_SoundDef->sd_LeftGain = GrabKnob (MixerIns, knobname);

  sprintf (knobname, "RightGain%ld", (j * 2 + 1));
  object->wo_SoundDef->sd_RightGain = GrabKnob (MixerIns, knobname);

  if (object->wo_SoundDef->sd_RightGain < 0)
    printf ("can't grab right knob!\n");

  /*
     freqknob=GrabKnob((Item)Sampler_item,"Frequency");
     TweakKnob(freqknob,0x00000400);
  */

  world->wp_TotalSimpleSounds++;

  //  object->wo_SoundDef->sd_Sampler=sampler_item;

  if (flags & SOUND_ON)
    {
      StartInstrument (object->wo_SoundDef->sd_Sampler, NULL);
      object->wo_SoundDef->sd_Flags |= SOUND_ON;
    }

  EXIT ("AddSimpleSound");

  return j;
}

/***********************************************************************
 * AverageSpecialVertex : simple mesh utility function.                *
 ***********************************************************************/
void
AverageSpecialVertex (SpecialVertex *v1, SpecialVertex *v2, SpecialVertex *v3,
                      int32 vnum)
{
  AverageVertex (&v1->vertex, &v2->vertex, &v3->vertex);
  v3->id1 = v1->id1;
  v3->id2 = v2->id1;
  // printf("vnum=%ld, v3->id1=%ld, v3->id2=%ld\n\n",vnum,v3->id1,v3->id2);
}

/***********************************************************************
 * AverageVertex : takes two vertices and calculates the average,      *
 *     This could be used to find the middle of a line in 3space       *
 ***********************************************************************
 * NOTE : This is a utility function for the mesh support              *
 ***********************************************************************/
void
AverageVertex (point3f16 *v1, point3f16 *v2, point3f16 *v3)
{
  ENTER ("AverageVertex");

  v3->x = ((v1->x + v2->x) >> 1);
  v3->y = ((v1->y + v2->y) >> 1);
  v3->z = ((v1->z + v2->z) >> 1);

  // printf("v1 : x=%ld, y=%ld, z=%ld\n",v1->x,v1->y,v1->z);
  // printf("v2 : x=%ld, y=%ld, z=%ld\n",v2->x,v2->y,v2->z);
  // printf("v3 : x=%ld, y=%ld, z=%ld\n",v3->x,v3->y,v3->z);
  EXIT ("AverageVertex");
}

/**************************************************************************
 * ClearUpdateFlags : clears the update flag in the WorldObject structure *
 **************************************************************************/
void
ClearUpdateFlags (WorldPort *world, uint32 mask)
{
  int i;
  WorldObject *object;

  ENTER ("ClearUpdateFlags");

  for (i = MAX3DOBJECTS; i >= 0; --i)
    {
      object = (WorldObject *)world->wp_UserObjects[i];

      if (object)
        object->wo_UpdateFlags &= mask;
    }

  EXIT ("ClearUpdateFlags");
}

/**************************************************************************
 * CloneModel : Clones a model, allocating new memory and duplicating     *
 *     all of the data.                                                   *
 **************************************************************************/
WorldObject *
CloneModel (WorldObject *object, int32 flags)
{
  WorldObject *newobject;
  ModelDefinition *newmodel, *oldmodel, *mesh_mdef;
  GeoDefinition *newgd, *oldgd;
  int32 len;
  int32 i, j;
  ModelSurface *newsurface, *oldsurface;
  IntMeshDefs *imdef;

  ENTER ("CloneModel");

  newobject = (WorldObject *)malloc (sizeof (WorldObject));

  if (!newobject)
    return (NULL);

  memcpy ((char *)newobject, (char *)object, sizeof (WorldObject));

  if (flags & CLONE_GEOMETRY)
    {
      newmodel = (ModelDefinition *)malloc (sizeof (ModelDefinition));
      oldmodel = (ModelDefinition *)object->wo_ModelDefinition;

      if (!newmodel)
        return (NULL);

      newobject->wo_ObjectDefinition = (int32 *)newmodel;
      newobject->wo_ModelDefinition = (int32 *)newmodel;
      newobject->wo_SourceData = NULL;

      newmodel->md_NumberOfScales = oldmodel->md_NumberOfScales;
      newmodel->md_Flags = oldmodel->md_Flags;

      for (i = 0; i < newmodel->md_NumberOfScales; i++)
        {
          // printf("sizeof GeoDef=%ld\n",sizeof(GeoDefinition));

          newmodel->md_GeoDefs[i]
              = (GeoDefinition *)malloc (sizeof (GeoDefinition));

          newgd = (GeoDefinition *)newmodel->md_GeoDefs[i];
          oldgd = (GeoDefinition *)oldmodel->md_GeoDefs[i];

          newgd->gd_RangeMin = oldgd->gd_RangeMin;
          newgd->gd_RangeMax = oldgd->gd_RangeMax;
          newgd->gd_PointCount = oldgd->gd_PointCount;
          newgd->gd_SurfaceCount = oldgd->gd_SurfaceCount;
          newgd->gd_FaceCenters = NULL;
          newgd->gd_FaceCentersXformed = NULL;
          newgd->gd_SortedList = NULL;

          len = sizeof (point3f16) * newgd->gd_PointCount;

          newgd->gd_PointList = (void *)malloc (len);
          if (!newgd->gd_PointList)
            return (NULL);
          memcpy ((char *)newgd->gd_PointList, (char *)oldgd->gd_PointList,
                  len);

          len = 16 * newgd->gd_SurfaceCount;
          newgd->gd_SurfaceList = (void *)malloc (len);
          if (!newgd->gd_SurfaceList)
            return (NULL);
          memcpy ((char *)newgd->gd_SurfaceList, (char *)oldgd->gd_SurfaceList,
                  len);

          len = sizeof (ModelSurface) * newgd->gd_SurfaceCount;
          newgd->gd_SurfaceType = (void *)malloc (len);
          if (!newgd->gd_SurfaceType)
            return (NULL);
          memcpy ((char *)newgd->gd_SurfaceType, (char *)oldgd->gd_SurfaceType,
                  len);

          newsurface = (ModelSurface *)newgd->gd_SurfaceType;
          oldsurface = (ModelSurface *)oldgd->gd_SurfaceType;

          for (j = 0; j < newgd->gd_SurfaceCount; j++)
            {
              newsurface->ms_Control
                  = (int32)MakeNewDupCCB ((CCB *)oldsurface->ms_Control);
              newsurface++;
              oldsurface++;
            }

          if (oldgd->gd_FaceNormals)
            {
              len = newgd->gd_SurfaceCount * sizeof (vec3f16);
              //             len=sizeof(ModelSurface)*newgd->gd_SurfaceCount*sizeof(vec3f16);
              newgd->gd_FaceNormals = (void *)malloc (len);
              if (!newgd->gd_FaceNormals)
                return (NULL);
              memcpy ((char *)newgd->gd_FaceNormals,
                      (char *)oldgd->gd_FaceNormals, len);
            }
          else
            newgd->gd_FaceNormals = NULL;

          if (oldgd->gd_FaceCenters)
            {
              CreateFaceCenters (newobject);
            }

          /*
          newgd->gd_PointList=oldgd->gd_PointList;
          newgd->gd_SurfaceCount=oldgd->gd_SurfaceCount;
          newgd->gd_SurfaceType=oldgd->gd_SurfaceType;
          newgd->gd_SortedList=oldgd->gd_SortedList;
          */
        }
    }
  else
    newobject->wo_ModelDefinition = object->wo_ModelDefinition;
  /*
  surfacetype=newgd->gd_SurfaceType;

  for(i=0;i<newgd->gd_SurfaceCount;i++)
  {
     printf("object : %lx, CCB=%lx\n",newobject,surfacetype->ms_Control);
     surfacetype++;
  }
  */

  if (object->wo_ObjectFlags & MESH_OBJECT)
    {
      imdef = (IntMeshDefs *)object->wo_SourceData;
      oldgd = (GeoDefinition *)imdef->im_ParentGdef;
      mesh_mdef = (ModelDefinition *)object->wo_MeshDefinition;

      CreateIntMeshDefinition (newobject, imdef->im_Scale,
                               mesh_mdef->md_GeoDefs[0]->gd_RangeMin,
                               mesh_mdef->md_GeoDefs[0]->gd_RangeMax,
                               imdef->im_WidthDiv, imdef->im_HeightDiv, NULL);
    }

  EXIT ("CloneModel");

  return (newobject);
}

/***********************************************************************
 * CopySpecialVertex : simple utility function for mesh support.       *
 ***********************************************************************/
void
CopySpecialVertex (SpecialVertex *dest, SpecialVertex *src, int32 vnum)
{
  memcpy (dest, src, sizeof (SpecialVertex));

  // printf("vertex %ld: id1=%ld, x=%ld, y=%ld,
  // z=%ld\n\n",vnum,dest->id1,dest->vertex.x,dest->vertex.y,dest->vertex.z);
}

/**************************************************************************
 * CreateFaceCenters : Creates a list of face centers and sticks it       *
 *     into the supplied object.                                          *
 **************************************************************************/
void
CreateFaceCenters (WorldObject *object)
{
  struct GeoDefinition *geodef;
  ModelDefinition *model;
  int32 i;

  ENTER ("CreateFaceCenters");

  model = (ModelDefinition *)object->wo_ModelDefinition;

  // printf("object=%lx, model=%lx,
  // numscales=%lx\n",object,model,model->md_NumberOfScales);

  for (i = 0; i < model->md_NumberOfScales; i++)
    {
      geodef = model->md_GeoDefs[i];

      CreateFaceCentersForGdef (object, geodef);
    }

  EXIT ("CreateFaceCenters");
}

/**************************************************************************
 * CreateFaceCentersForGdef : Creates a list of face centers for the      *
 *     supplied GeoDef.                                                   *
 **************************************************************************/
void
CreateFaceCentersForGdef (WorldObject *object, GeoDefinition *geodef)
{
  int32 j, k;
  int32 point_id;
  int32 point_counter;
  point3f16 *vertex, *center;
  int32 len;
  point3f16 *vertex_list;
  int32 *surface_list;
  int32 *sorted_list;

  ENTER ("CreateFaceCentersForGdef");

  len = sizeof (vec3f16) * geodef->gd_SurfaceCount;
  geodef->gd_FaceCenters = (void *)malloc (len);
  geodef->gd_FaceCentersXformed = (void *)malloc (len);
  geodef->gd_SortedList = (void *)malloc (4 * geodef->gd_SurfaceCount);
  sorted_list = (int32 *)geodef->gd_SortedList;

  vertex_list = (struct point3f16 *)geodef->gd_PointList;
  surface_list = (int32 *)geodef->gd_SurfaceList;
  center = (point3f16 *)geodef->gd_FaceCenters;

  // printf("Geodef=%lx, Sorted_list=%lx,
  // count=%ld\n",geodef,geodef->gd_SortedList,geodef->gd_SurfaceCount);

  for (j = 0; j < geodef->gd_SurfaceCount; j++)
    {
      point_counter = (j << 2); /* mult by 4 */

      center->x = 0;
      center->y = 0;
      center->z = 0;

      /*** merely init this list with a linearly ascending array of values ***/

      sorted_list[j] = j;

      /*** find the center-of-gravity by taking the average of all 4 points
       * ***/

      for (k = 0; k < 4; k++)
        {
          point_id = surface_list[point_counter + k];
          vertex = (struct point3f16 *)(vertex_list + point_id);

          center->x += vertex->x;
          center->y += vertex->y;
          center->z += vertex->z;
        }

      center->x >>= 2; /* div by 4 */
      center->y >>= 2;
      center->z >>= 2;

      /*
      printf("face %ls\n",j);
      printfF16("center->x=",center->x);
      printfF16("center->y=",center->y);
      printfF16("center->z=",center->z);
      */
      center++;
    }

  EXIT ("CreateFaceCentersForGdef");
}

/***********************************************************************
 * CreateMeshDefinition : creates the ModelDefinition for a meshed     *
 *     definition of the supplied object and hooks it into the data.   *
 ***********************************************************************/
int32
CreateMeshDefinition (WorldObject *object, int32 obj_id, int32 mindist,
                      int32 maxdist, int32 div_factor, TagArg *taglist)
{
  ModelDefinition *mesh_mdef, *model_mdef;
  GeoDefinition *model_geodef, *mesh_geodef, *old_mesh_geodef;
  int32 len;
  int32 i;

  ENTER ("CreateMeshDefinition");

  model_mdef = (ModelDefinition *)object->wo_ModelDefinition;
  model_geodef = model_mdef->md_GeoDefs[0];

  len = sizeof (ModelDefinition);
  object->wo_MeshDefinition = (int32 *)malloc (len);

  mesh_mdef = (ModelDefinition *)object->wo_MeshDefinition;
  mesh_mdef->md_NumberOfScales = 1;
  mesh_geodef = model_mdef->md_GeoDefs[0];

  for (i = 0; i < div_factor; i++)
    {
      old_mesh_geodef = mesh_geodef;

      /*** i+1 cooresponds to the MESHx #define codes ***/

      mesh_geodef = CreateMeshGeodef (object, old_mesh_geodef, i + 1);

      if (!mesh_geodef)
        {
          free (object->wo_MeshDefinition);
          object->wo_MeshDefinition = NULL;
          return (-1); /* oops! probably a packed CEL in the object */
        }

      if (i >= 1)
        FreeGeodef (old_mesh_geodef);
    }

  mesh_mdef->md_GeoDefs[obj_id] = mesh_geodef;

  mesh_mdef->md_Flags |= MESH_MODEL;

  mesh_mdef->md_GeoDefs[0]->gd_RangeMin = mindist;
  mesh_mdef->md_GeoDefs[0]->gd_RangeMax = maxdist;
  object->wo_ObjectFlags |= MESH_OBJECT;

  EXIT ("CreateMeshDefinition");

  return (0);
}

/***********************************************************************
 * CreateIntMeshDefinition : creates the ModelDefinition for an imesh  *
 *     definition of the supplied object and hooks it into the data.   *
 ***********************************************************************/
int32
CreateIntMeshDefinition (WorldObject *object, int32 scale_id, int32 mindist,
                         int32 maxdist, int32 width, int32 height,
                         TagArg *taglist)
{
  ModelDefinition *mesh_mdef, *model_mdef;
  GeoDefinition *model_geodef, *mesh_geodef;
  int32 len;
  IntMeshDefs *imdef;

  ENTER ("CreateIntMeshDefinition");

  model_mdef = (ModelDefinition *)object->wo_ModelDefinition;

  len = sizeof (ModelDefinition);
  object->wo_MeshDefinition = (int32 *)malloc (len);
  model_geodef = model_mdef->md_GeoDefs[scale_id];

  object->wo_SourceData = (int32 *)malloc (sizeof (IntMeshDefs));

  imdef = (IntMeshDefs *)object->wo_SourceData;
  imdef->im_ParentGdef = model_geodef;

  imdef->im_WidthDiv = width;
  imdef->im_HeightDiv = height;
  imdef->im_Scale = scale_id;

  /*** not used right now, but here for expansion purposes later on ***/

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  mesh_mdef = (ModelDefinition *)object->wo_MeshDefinition;
  mesh_mdef->md_NumberOfScales = 1;
  //   mesh_mdef->md_GeoDefs[0]=model_geodef;

  mesh_geodef = CreateIntMeshGeodef (object, model_geodef);

  if (!mesh_geodef)
    {
      free (object->wo_SourceData);
      object->wo_SourceData = NULL;

      free (object->wo_MeshDefinition);
      object->wo_MeshDefinition = NULL;
      return (-1); /* oops! probably a packed CEL in the object */
    }

  mesh_mdef->md_GeoDefs[0] = mesh_geodef;

  mesh_mdef->md_Flags |= IMESH_MODEL;

  mesh_mdef->md_GeoDefs[0]->gd_RangeMin = mindist;
  mesh_mdef->md_GeoDefs[0]->gd_RangeMax = maxdist;

  object->wo_ObjectFlags |= MESH_OBJECT;

  EXIT ("CreateIntMeshDefinition");

  return (0);
}

/******************************************************************************
 * CreateIntMeshGeodef : creates a geodef for an interpolated mesh            *
 ******************************************************************************/
GeoDefinition *
CreateIntMeshGeodef (WorldObject *object, GeoDefinition *src_geodef)
{
  int32 corner;
  int32 *surface_list;
  int32 *conn;
  int32 len;
  int32 i, j, k;
  GeoDefinition *mesh_geodef;
  point3f16 *vertex_list;
  ModelSurface *surfacetype, *modelsurface;
  int32 x, y, w, h;
  CCB *ccb;
  IntMeshDefs *imesh_def;
  int32 seg_width, seg_height;
  int32 number_of_corners;
  int32 points_per_src_face; // number of new points in a the mesh for a source
                             // face

  vertex_list = (struct point3f16 *)src_geodef->gd_PointList;
  surface_list = (int32 *)src_geodef->gd_SurfaceList;
  modelsurface = (ModelSurface *)src_geodef->gd_SurfaceType;
  imesh_def = (IntMeshDefs *)object->wo_SourceData;

  seg_width = imesh_def->im_WidthDiv;
  seg_height = imesh_def->im_HeightDiv;

  /*** create the packed vertex list ***/

  mesh_geodef = (GeoDefinition *)malloc (sizeof (GeoDefinition));
  memset ((char *)mesh_geodef, (int)0x00, (size_t)sizeof (GeoDefinition));

  /*** the point count doesn't take into account possible duplicate vertices,
   *since this
   *** is done on-the-fly, checking for dupes would take too long. ***/

  points_per_src_face = (seg_width + 1) * (seg_height + 1);

  mesh_geodef->gd_SurfaceCount
      = seg_width * seg_height * src_geodef->gd_SurfaceCount;
  mesh_geodef->gd_PointCount
      = points_per_src_face * src_geodef->gd_SurfaceCount;

  mesh_geodef->gd_PointList = NULL;

  /*** create the connectivity array. ***/

  number_of_corners = mesh_geodef->gd_SurfaceCount * 4;
  len = number_of_corners * 4; // in bytes
  mesh_geodef->gd_SurfaceList = (void *)malloc (len);

  conn = (int32 *)mesh_geodef->gd_SurfaceList;

  /*** we are dealing with an array of size w*h. Let's start with a face having
   *a "corner" at
   *** the origin. We then circle around the face. The next point over is the
   *corner position+1,
   ***  after that we drop down, which means adding the "w" value to cycle over
   *to the next row,
   *** along with 1 since the number of vertices along an axis is 1+ the number
   *of segments (due
   *** to the 1 extra vertex to end things). Another 1 is added (hence the +2)
   *since we're
   *** 1 column over from the starting point. We now move "left" to the last
   *point, so it is
   *** the last corner -1. ***/

  for (i = 0; i < src_geodef->gd_SurfaceCount; i++)
    {
      /*** this line skips over to each new face, jumping over the ones just
       * completed. ***/

      corner = i * points_per_src_face;

      for (j = 0; j < seg_height; j++)
        {
          for (k = 0; k < seg_width; k++)
            {
              *conn = corner;
              conn++;

              *conn = corner + 1;
              conn++;

              *conn = corner + seg_width + 2;
              conn++;

              *conn = corner + seg_width + 1;
              conn++;

              corner++;
            }
          corner++; // skip and extra count to reset ourselves to the next row
        }
    }

  /*** dump the connectivity array for testing ***/

  conn = (int32 *)mesh_geodef->gd_SurfaceList;
  for (i = 0; i < number_of_corners; i++)
    {
      //   printf("%ld\n",*conn);

      conn++;
    }
  len = sizeof (ModelSurface) * mesh_geodef->gd_SurfaceCount;
  mesh_geodef->gd_SurfaceType = (void *)malloc (len);

  surfacetype = (ModelSurface *)mesh_geodef->gd_SurfaceType;

  for (i = 0; i < src_geodef->gd_SurfaceCount; i++)
    {
      x = 0;
      y = 0;

      ccb = (CCB *)modelsurface->ms_Control;

      if (ccb->ccb_Flags & CCB_PACKED)
        {
          free (mesh_geodef->gd_SurfaceType);
          free (mesh_geodef->gd_SurfaceList);
          free (mesh_geodef->gd_PointList);
          free (mesh_geodef);
          return (NULL);
        }

      w = ccb->ccb_Width / seg_width;
      h = ccb->ccb_Height / seg_height;

      for (j = 0; j < seg_height; y += h, j++)
        {
          for (k = 0; k < seg_width; x += w, k++)
            {
              surfacetype->ms_Control = (int32)ClipCel (
                  (CCB *)modelsurface->ms_Control, x, y, w, h);
              // printf("cel : x=%ld, y=%ld, w=%ld, h=%ld\n",x,y,w,h);

              surfacetype++;
            }

          x = 0;
        }

      modelsurface++;
    }

  /*** create a mesh normal list. In this case I only create a single normal
   *for each mesh array, since
   *** all sub-faces are facing the same way, with the same normal. When
   *lighting is calculated it is based
   *** only on that single normal, saving alot of time. ***/

  len = src_geodef->gd_SurfaceCount * sizeof (struct point3f16);

  if (src_geodef->gd_FaceNormals)
    {
      mesh_geodef->gd_FaceNormals = malloc (len);
      memcpy ((char *)mesh_geodef->gd_FaceNormals,
              (char *)src_geodef->gd_FaceNormals, len);
    }

  /*** notice that only the xformed face-center memory is allocated since they
   *are calculated
   *** on the fly already transformed. ***/

  if (src_geodef->gd_FaceCenters)
    {
      len = sizeof (vec3f16) * mesh_geodef->gd_SurfaceCount;
      mesh_geodef->gd_FaceCentersXformed = (void *)malloc (len);
    }

  return (mesh_geodef);

  EXIT ("CreateIntMeshGeodef");
}

/******************************************************************************
 * CreateMeshGeodef : creates a geodef for a mesh                             *
 ******************************************************************************/
GeoDefinition *
CreateMeshGeodef (WorldObject *object, GeoDefinition *src_geodef, int32 div)
{
  int32 *surface_list;
  int32 *conn;
  int32 off;
  int32 len;
  int32 num_unique_vertices;
  int32 num_subdivided_faces;
  int32 sub_vertices_per_face;
  int32 occurance;
  int32 numvertices;
  int32 i, j, k;
  int32 point_counter, point_id;
  GeoDefinition *mesh_geodef;
  SpecialVertex vertex[4], *svtx1, *svtx2, *vtx1, *vtx2, *destvtx;
  point3f16 *vertex_list, *point;
  ModelSurface *surfacetype, *modelsurface;
  int32 vnum;
  int32 x, y, w, h;
  CCB *ccb;

  vertex_list = (struct point3f16 *)src_geodef->gd_PointList;
  surface_list = (int32 *)src_geodef->gd_SurfaceList;
  modelsurface = (ModelSurface *)src_geodef->gd_SurfaceType;

  /*** allocate a temporary array to hold the calculated vertices. The size if
   *** the max possible (for the first subdivision), 9 vertices/face.
   *** Find each set of vertices for each face, calculate the new set and
   *perserve
   *** the point_id numbers of the vertices used to generate each new one.
   *Afterwards
   *** the entire set is analyzed and duplicate vertices are tossed out. The
   *data
   *** is then packed into the expected format. (Yes, this does incur some
   *overhead since
   *** it ends up calculating alot of vertices 2 or 3 times, but the averaging
   *process
   *** is only integer math, so is very fast.) ***/

  len = 9 * sizeof (SpecialVertex) * src_geodef->gd_SurfaceCount;
  svtx1 = (SpecialVertex *)malloc (len);
  svtx2 = (SpecialVertex *)malloc (len);

  memset (svtx1, 0, len);
  memset (svtx2, 0, len);

  off = 0;
  vnum = 0;

  for (j = 0; j < src_geodef->gd_SurfaceCount; j++)
    {
      point_counter = (j << 2); /* mult by 4 */

      for (k = 0; k < 4; k++)
        {
          point_id = surface_list[point_counter + k];
          memcpy (&vertex[k].vertex,
                  (struct point3f16 *)(vertex_list + point_id),
                  sizeof (point3f16));
          // printf("point_counter=%ld,
          // j=%ld,k=%ld,point_id=%ld\n",point_counter,j,k,point_id);
          vertex[k].id1 = point_id;
          vertex[k].id2 = -1; /* unused in this case */
          vertex[k].dupe = 0;
        }

      /*** subdivide the quad described by vertex[] into 4 squares, or 9
       *verticies
       *** total.***/
      // printf("subdividing face %ld\n",j);

      CopySpecialVertex ((SpecialVertex *)(svtx1 + 0 + off), &vertex[0],
                         vnum++);
      CopySpecialVertex ((SpecialVertex *)(svtx1 + 1 + off), &vertex[1],
                         vnum++);
      CopySpecialVertex ((SpecialVertex *)(svtx1 + 2 + off), &vertex[2],
                         vnum++);
      CopySpecialVertex ((SpecialVertex *)(svtx1 + 3 + off), &vertex[3],
                         vnum++);

      AverageSpecialVertex (&vertex[0], &vertex[1],
                            (SpecialVertex *)(svtx1 + 4 + off), vnum++);
      AverageSpecialVertex (&vertex[1], &vertex[2],
                            (SpecialVertex *)(svtx1 + 5 + off), vnum++);
      AverageSpecialVertex (&vertex[2], &vertex[3],
                            (SpecialVertex *)(svtx1 + 6 + off), vnum++);
      AverageSpecialVertex (&vertex[3], &vertex[0],
                            (SpecialVertex *)(svtx1 + 7 + off), vnum++);
      AverageSpecialVertex (&vertex[0], &vertex[2],
                            (SpecialVertex *)(svtx1 + 8 + off),
                            vnum++); // center point

      off += 9; // jumps to the next face storage block, 9 vertices long
    }

  /*** create a new array minus duplicate vertices. The original is still kept
   *since that
   *** has the connectivity information embedded in its structure. Take a
   *vertex from
   *** the source array, and compare it with vertices later in the array. If it
   *** is a dupe, flag it, if not (the first occurance) copy it over to the
   *destination
   *** array, and in its id3 field, place the index of the first vertex. So for
   *instance,
   *** if vertices 8 and 10 are identical to vertex 2, then 8 and 10 will have
   *the value of "2"
   *** (-1 of course) stuffed into id3. Later on I use that mapping value when
   *building the
   *** connectivity array. ***/

  vtx1 = svtx1;
  vtx2 = svtx1;
  destvtx = svtx2; // the current destination vertex
  occurance = 0;

  numvertices = 9 * src_geodef->gd_SurfaceCount;
  num_unique_vertices = 0;

  for (j = 0; j < numvertices; j++)
    {
      if (vtx1->dupe)
        {
          vtx1++;
          vtx2 = vtx1;
          occurance = 0;
          continue; // if a dupe, go to the next vertex
        }

      for (k = j; k < numvertices; k++)
        {

          //  printf("j=%ld, k=%ld, vtx1=%lx, vtx2=%lx\n",j,k,vtx1,vtx2);
          //  printf("vertex %ld,v1=%ld/%ld, v2=
          //  %ld/%ld\n",k,vtx1->id1,vtx1->id2,vtx2->id1,vtx2->id2);

          /*** We can't be sure what order the source points are, so check
           * multiple ways. ***/

          if (((vtx1->id1 == vtx2->id1) && (vtx1->id2 == vtx2->id2))
              || ((vtx1->id2 == vtx2->id1) && (vtx1->id1 == vtx2->id2)))
            {
              // printf("j=%ld, k=%ld, vtx1=%lx, vtx2=%lx\n",j,k,vtx1,vtx2);
              // printf("vertex %ld,v1=%ld/%ld, v2=
              // %ld/%ld\n",k,vtx1->id1,vtx1->id2,vtx2->id1,vtx2->id2);
              occurance++;

              if (occurance == 1) // first copy of this vertex
                {
                  memcpy (destvtx, vtx1, sizeof (SpecialVertex));
                  destvtx++;
                  num_unique_vertices++;
                  // printf("vertex %ld is original\n",k);
                }
              else
                {
                  // printf("vertex %ld is a dupe\n",k);
                  vtx2->dupe = 1; // flag this as a dupe
                }

              /*** num_unique_vertices-1 serves as the index into the filtered
               *array, the array
               *** with no dupes. So if vertex 8 is a dupe of vertex 3 which is
               *the second element
               *** of the filtered array, id3 will equal 2-1, or 1. ***/

              vtx2->id3 = num_unique_vertices
                          - 1; // map the dupe vtx to it's unique copy
                               // printf("vtx2=%lx, id3=%ld\n",vtx2,vtx2->id3);
            }

          vtx2++;
          // printf("\n\n");
        }

      // if(j>2)getout(0);

      vtx1++;
      vtx2 = vtx1;
      occurance = 0;
    }

  // printf("num unique_vertices=%ld\n",num_unique_vertices);
  /*** create the packed vertex list ***/

  mesh_geodef = (GeoDefinition *)malloc (sizeof (GeoDefinition));
  memset ((char *)mesh_geodef, (int)0x00, (size_t)sizeof (GeoDefinition));

  mesh_geodef->gd_PointCount = num_unique_vertices;
  mesh_geodef->gd_SurfaceCount = 4 * src_geodef->gd_SurfaceCount;

  len = sizeof (point3f16) * num_unique_vertices;
  mesh_geodef->gd_PointList = malloc (len);

  point = (point3f16 *)mesh_geodef->gd_PointList;
  vtx2 = svtx2;

  for (i = 0; i < num_unique_vertices; i++)
    {
      memcpy (point, vtx2, sizeof (point3f16));
      point++;
      vtx2++;
    }

  /*** create the connectivity array. Each block of 9 vertices in the svtx1
   *array
   *** represents the various faces. The first block is face 0, the next, face
   *1,
   *** and so on. Facemap[][] tells what vertices each subdivided face is made
   *of, so
   *** we index off of the beginning of each block. ***/

  len = sizeof (int32) * 4
        * mesh_geodef
              ->gd_SurfaceCount; // numfaces*verticess_per_face*bytes_per_vertex
  mesh_geodef->gd_SurfaceList = (void *)malloc (len);
  conn = (int32 *)mesh_geodef->gd_SurfaceList;
  vtx1 = svtx1;
  num_subdivided_faces = 4;
  sub_vertices_per_face
      = 9; // 9 new vertices when a face is divided into quarters.

  // printf("len=%ld, conn=%lx\n",len,conn);

  for (i = 0; i < src_geodef->gd_SurfaceCount; i++)
    {
      for (j = 0; j < num_subdivided_faces;
           j++) // collect data for each new face
        {
          // printf("\nface %ld, subface %ld\n",i,j);
          for (k = 0; k < 4; k++) // vertex loop
            {
              vtx2 = (vtx1 + Facemap[j][k]);
              *conn = vtx2->id3;
              // printf("vertex id=%ld, conn=%lx, vtx1=%lx, i=%ld, j=%ld,
              // k=%ld\n",*conn,conn,vtx1,i,j,k); printf("x=%ld, y=%ld,
              // z=%ld\n",vtx2->vertex.x,vtx2->vertex.y,vtx2->vertex.z);
              conn++;
            }
        }
      // printf("vtx1=%lx\n",vtx1);

      vtx1 += sub_vertices_per_face; // go to the next collection of vertices
    }

  len = sizeof (ModelSurface) * mesh_geodef->gd_SurfaceCount;
  mesh_geodef->gd_SurfaceType = (void *)malloc (len);

  surfacetype = (ModelSurface *)mesh_geodef->gd_SurfaceType;

  for (i = 0; i < src_geodef->gd_SurfaceCount; i++)
    {
      x = 0;
      y = 0;

      for (j = 0; j < num_subdivided_faces;
           j++) // collect data for each new face
        {
          surfacetype->ms_Flags = modelsurface->ms_Flags;

          //         surfacetype->ms_Control=modelsurface->ms_Control;

          ccb = (CCB *)modelsurface->ms_Control;

          if (ccb->ccb_Flags & CCB_PACKED)
            {
              free (mesh_geodef->gd_SurfaceType);
              free (mesh_geodef->gd_SurfaceList);
              free (mesh_geodef->gd_PointList);
              free (mesh_geodef);
              return (NULL);
            }

          w = (ccb->ccb_Width >> 1);
          h = (ccb->ccb_Height >> 1);

          if (j == 1)
            {
              x = w;
              y = 0;
            }
          else if (j == 2)
            {
              x = w;
              y = h;
            }
          else if (j == 3)
            {
              x = 0;
              y = h;
            }

          surfacetype->ms_Control
              = (int32)ClipCel ((CCB *)modelsurface->ms_Control, x, y, w, h);

          // printf("surfacetype=%lx,
          // modelsurface=%lx,control=%lx\n",surfacetype,modelsurface,surfacetype->ms_Control);

          surfacetype++;
        }

      modelsurface++;
    }

  /*
     mesh_geodef->gd_FaceNormals=NULL;
     mesh_geodef->gd_FaceCenters=NULL;
     mesh_geodef->gd_FaceCentersXformed=NULL;
  */

  /*** create a mesh normal list. In this case I only create a single normal
   *for each mesh array, since
   *** all sub-faces are facing the same way, with the same normal. When
   *lighting is calculated it is based
   *** only on that single normal, saving alot of time. ***/

  len = src_geodef->gd_SurfaceCount * sizeof (struct point3f16);

  if (src_geodef->gd_FaceNormals)
    {
      mesh_geodef->gd_FaceNormals = malloc (len);
      memcpy ((char *)mesh_geodef->gd_FaceNormals,
              (char *)src_geodef->gd_FaceNormals, len);
    }

  mesh_geodef->gd_Flags = (src_geodef->gd_Flags & (~GD_MESH_MASK));
  mesh_geodef->gd_Flags |= (div & GD_MESH_MASK);

  if (src_geodef->gd_FaceCenters)
    CreateFaceCentersForGdef (object, mesh_geodef);

  free (svtx1);
  free (svtx2);

  return (mesh_geodef);

  EXIT ("CreateMeshGeodef");
}

/**************************************************************************
 * CreateNormalList : Creates a list of normals, one normal for each      *
 *     face on the supplied object. Used of a person wants lighting.      *
 **************************************************************************/
void
CreateNormalList (WorldObject *object)
{
  struct GeoDefinition *geodef;
  ModelDefinition *model;
  int32 i;

  ENTER ("CreateNormalList");

  model = (ModelDefinition *)object->wo_ModelDefinition;

  // printf("object=%lx, model=%lx,
  // numscales=%lx\n",object,model,model->md_NumberOfScales);

  for (i = 0; i < model->md_NumberOfScales; i++)
    {
      geodef = model->md_GeoDefs[i];

      CreateNormalListForGdef (object, geodef);
    }

  EXIT ("CreateNormalList");
}

/**************************************************************************
 * CreateNormalListForGdef : creates a normal list for a single gdef      *
 **************************************************************************/
void
CreateNormalListForGdef (WorldObject *object, GeoDefinition *geodef)
{
  int32 j;
  int32 point_id;
  int32 point_counter;
  point3f16 *vertex0, *vertex1, *vertex2;
  vec3f16 vect1, vect2, normal_addr;
  vec3f16 *normal = (vec3f16 *)&normal_addr[0];
  int32 len;
  point3f16 *vertex_list;
  frac16 localx, localy, localz;
  int32 *surface_list;

  ENTER ("CreateNormalListForGdef");

  len = sizeof (vec3f16) * geodef->gd_SurfaceCount;
  geodef->gd_FaceNormals = (void *)malloc (len);
  normal = (vec3f16 *)geodef->gd_FaceNormals;
  vertex_list = (struct point3f16 *)geodef->gd_PointList;
  surface_list = (int32 *)geodef->gd_SurfaceList;

  // printf("normals for object %lx, gd=%lx,
  // normallist=%lx\n",object,geodef,geodef->gd_FaceNormals);

  localx = object->wo_LocalPosition[X_COORD];
  localy = object->wo_LocalPosition[Y_COORD];
  localz = object->wo_LocalPosition[Z_COORD];

  for (j = 0; j < geodef->gd_SurfaceCount; j++)
    {
      normal = (vec3f16 *)geodef->gd_FaceNormals;
      normal += j;

      point_counter = j * 4;

      point_id = surface_list[point_counter];

      // printf("vertex_list=%lx, point_id=%ld\n");

      vertex0 = (struct point3f16 *)(vertex_list + point_id);
      vertex0->x -= localx;
      vertex0->y -= localy;
      vertex0->z -= localz;

      point_id = surface_list[point_counter + 1];
      vertex1 = (struct point3f16 *)(vertex_list + point_id);
      vertex1->x -= localx;
      vertex1->y -= localy;
      vertex1->z -= localz;

      point_id = surface_list[point_counter + 2];
      vertex2 = (struct point3f16 *)(vertex_list + point_id);
      vertex2->x -= localx;
      vertex2->y -= localy;
      vertex2->z -= localz;

      vect1[X_COORD] = vertex0->x - vertex1->x;
      vect1[Y_COORD] = vertex0->y - vertex1->y;
      vect1[Z_COORD] = vertex0->z - vertex1->z;

      vect2[X_COORD] = vertex0->x - vertex2->x;
      vect2[Y_COORD] = vertex0->y - vertex2->y;
      vect2[Z_COORD] = vertex0->z - vertex2->z;

      NormalizeVector (vect1);
      NormalizeVector (vect2);

      Cross3_F16 (*normal, vect1, vect2);
    }

  EXIT ("CreateNormalListForGdef");
}

/**************************************************************************
 * DestroyGeodef : destroys a 3D geodef, used internally only by          *
 *     DestroyModel().                                                    *
 **************************************************************************/
void
DestroyGeodef (GeoDefinition *geodef, int32 flags)
{
  int32 j;
  ModelSurface *surface;

  ENTER ("DestroyGeodef");

  if (geodef)
    {
      if (geodef->gd_PointList)
        free (geodef->gd_PointList);
      if (geodef->gd_SurfaceList)
        free (geodef->gd_SurfaceList);
      if (geodef->gd_SortedList)
        free (geodef->gd_SortedList);
      if (geodef->gd_FaceNormals)
        free (geodef->gd_FaceNormals);
      if (geodef->gd_FaceCenters)
        free (geodef->gd_FaceCenters);
      if (geodef->gd_FaceCentersXformed)
        free (geodef->gd_FaceCentersXformed);

      if (geodef->gd_SurfaceType)
        {
          surface = (ModelSurface *)geodef->gd_SurfaceType;

          for (j = 0; j < geodef->gd_SurfaceCount; j++)
            {
              FreeMem ((void *)surface->ms_Control, sizeof (CCB));
              surface++;
            }

          free (geodef->gd_SurfaceType);
        }

      free (geodef);
    }

  EXIT ("DestroyGeodf");
}

/**************************************************************************
 * DestroyModel : destroys a 3D model, handy for destroying cloned objects*
 **************************************************************************/
void
DestroyModel (WorldObject *object, int32 flags)
{
  GeoDefinition *geodef;
  ModelDefinition *mesh_mdef, *model;
  int32 i;

  ENTER ("DestroyModel");

  model = (ModelDefinition *)object->wo_ModelDefinition;

  // printf("object=%lx, model=%lx,
  // numscales=%lx\n",object,model,model->md_NumberOfScales);

  if (object->wo_ObjectFlags & MESH_OBJECT)
    {
      if (object->wo_SourceData)
        free (object->wo_SourceData);
      if (object->wo_MeshDefinition)
        {
          mesh_mdef = (ModelDefinition *)object->wo_MeshDefinition;

          DestroyGeodef (mesh_mdef->md_GeoDefs[0], NO_FLAGS);
          free (mesh_mdef);
        }
    }

  for (i = 0; i < model->md_NumberOfScales; i++)
    {
      geodef = model->md_GeoDefs[i];

      if (geodef)
        {
          DestroyGeodef (geodef, 0);
        }
    }

  if (model)
    free (model);
  if (object)
    free (object);

  EXIT ("DestroyModel");
}

/**************************************************************************
 * DestroySound : destroys both a 3D and Simple sound object.             *
 **************************************************************************/
void
DestroySound (WorldObject *object, int32 flags)
{
  ENTER ("DestroySound");

  if (object->wo_SoundDef)
    {
      if (object->wo_SoundDef->sd_Sampler)
        {
          UnloadSample (object->wo_SoundDef->sd_Sampler);
        }
      free (object->wo_SoundDef);
    }

  if (flags & FREE_ENTIRE_OBJECT) /* for non-static allocations */
    free (object);

  EXIT ("DestroySound");
}

/**************************************************************************
 * DestroyWorld : destroys an entire world, Mwa-hah-hah-hah-hah-hahhhhh!  *
 **************************************************************************
 * NOTE : This is a very dangerous routine, all objects must be allocated *
 *     objects.                                                           *
 **************************************************************************/
void
DestroyWorld (WorldPort *world, int32 flags)
{
  int i;
  WorldObject *object;

  ENTER ("DestroyWorld");

  for (i = 0; i < MAX3DOBJECTS; i++)
    {
      if (world->wp_UserObjects[i])
        {
          object = (WorldObject *)world->wp_UserObjects[i];

          if (object)
            {
              if (object->wo_ObjectFlags & POLYGONTYPE
                  || object->wo_ObjectFlags & BITMAPTYPE)
                {
                  DestroyModel (object, FREE_ENTIRE_OBJECT);
                }
              else if (object->wo_ObjectFlags & SOUNDTYPE)
                {
                  DestroySound (object, FREE_ENTIRE_OBJECT);
                }
              else
                {
                  free (object);
                }
            }
        }
    }

  if (FREE_ENTIRE_WORLD & flags)
    free (world);

  EXIT ("DestroyWorld");
}

/***********************************************************************
 * FreeGeodef : frees up a geodef, used by CreateMeshGeodef            *
 ***********************************************************************/
void
FreeGeodef (GeoDefinition *geodef)
{
  if (geodef->gd_PointList)
    free (geodef->gd_PointList);
  if (geodef->gd_SurfaceList)
    free (geodef->gd_SurfaceList);
  if (geodef->gd_SurfaceType)
    free (geodef->gd_SurfaceType);
  if (geodef->gd_SortedList)
    free (geodef->gd_SortedList);
  if (geodef->gd_FaceNormals)
    free (geodef->gd_FaceNormals);
  if (geodef->gd_FaceCenters)
    free (geodef->gd_FaceCenters);
  if (geodef->gd_FaceCentersXformed)
    free (geodef->gd_FaceCentersXformed);

  if (geodef)
    free (geodef);
}

/******************************************************************************
 * GenerateImeshData : takes an IMesh object after the rotations, collects    *
 *     the vertices and generates the interpolated data, stuffing it back     *
 *     onto the heap.                                                         *
 ******************************************************************************
 * NOTE : once the deltas are calculated the main core of this merely adds    *
 *     the deltas to each previous point. The loop that handles that has      *
 *     7 adds at 2 cycles ea. and say, about 20 memory operations of 3 cycles *
 *     each for a total of 74 cycles. Compare this to a hardware 3x3 matrix   *
 *     multiply required for each vertex, which is about 625 cycles. So       *
 *     theoretically speaking, this method is about 8 times faster.           *
 ******************************************************************************/
void
GenerateImeshData (WorldPort *wport, WorldObject *object)
{
  ModelDefinition *mdef;
  GeoDefinition *gdef;  // the imesh object
  GeoDefinition *pgdef; // the parent
  int32 *surface_list;
  int32 point_id;
  int32 i, j, k;
  int32 face_offset;
  point3f16 *vertex_array[4];
  frac16 hdx, hdy, hdz, vdx, vdy, vdz, hddx, hddy, hddz;
  frac16 x, y, z;
  IntMeshDefs *imdef;
  int32 seg_width, seg_height;
  frac16 *imesh_data;
  struct point3f16 *vertex_list, *center, *vertex, starting_point;
  char *heap_ptr;
  int32 *conn;

  ENTER ("GenerateImeshData");

  mdef = (ModelDefinition *)object->wo_ObjectDefinition;
  gdef = mdef->md_GeoDefs[0];
  imdef = (IntMeshDefs *)object->wo_SourceData;
  pgdef = imdef->im_ParentGdef;
  surface_list = (int32 *)pgdef->gd_SurfaceList;

  seg_width = imdef->im_WidthDiv;
  seg_height = imdef->im_HeightDiv;

  vertex_list = (struct point3f16 *)object->wo_TransformedData;

  imesh_data = (frac16 *)wport->wp_CurrentPointHeap;

  /*** this is virtually identical to the cel engine except for
   *** 3 dimensions vs. 2 ***/

  for (i = 0; i < pgdef->gd_SurfaceCount; i++)
    {
      // printf("FACE %ld\n",i);

      face_offset = i << 2; // number of points into the conn array

      /*** collect together all 4 quad points ***/

      for (j = 0; j < 4; j++)
        {
          point_id = surface_list[face_offset + j];
          vertex_array[j] = (struct point3f16 *)(vertex_list + point_id);
        }

      /*** find the deltas for the horizontal lines ***/

      hdx = (vertex_array[1]->x - vertex_array[0]->x);
      hdx = DivSF16 (hdx, (seg_width << 16));

      hdy = (vertex_array[1]->y - vertex_array[0]->y);
      hdy = DivSF16 (hdy, (seg_width << 16));

      hdz = (vertex_array[1]->z - vertex_array[0]->z);
      hdz = DivSF16 (hdz, (seg_width << 16));

      /*** now find the delta for the "vertical", or left border of
       *** the face. ***/

      vdx = (vertex_array[3]->x - vertex_array[0]->x);
      vdx = DivSF16 (vdx, (seg_height << 16));

      vdy = (vertex_array[3]->y - vertex_array[0]->y);
      vdy = DivSF16 (vdy, (seg_height << 16));

      vdz = (vertex_array[3]->z - vertex_array[0]->z);
      vdz = DivSF16 (vdz, (seg_height << 16));

      /*** find the delta-delta values for the horizontal deltas. These
       *** are the values hdx,hdy and hdz change for each scan line. They are
       *** calculated based on a comparison of the first scanline and last
       *scanline
       *** values. Then take the average delta per line using the height value.
       ****/

      hddx = vertex_array[2]->x - vertex_array[3]->x;
      hddx = DivSF16 (hddx, (seg_width << 16)) - hdx;
      hddx = DivSF16 (hddx, (seg_height << 16));

      hddy = vertex_array[2]->y - vertex_array[3]->y;
      hddy = DivSF16 (hddy, (seg_width << 16)) - hdy;
      hddy = DivSF16 (hddy, (seg_height << 16));

      hddz = vertex_array[2]->z - vertex_array[3]->z;
      hddz = DivSF16 (hddz, (seg_width << 16)) - hdz;
      hddz = DivSF16 (hddz, (seg_height << 16));

      /*** the starting point, consider it the "upper-left" corner of the
       *** face. ***/

      // printf(" vdx=%s,  vdy=%s,
      // vdz=%s\n",Frac16str(vdx,buff1,20),Frac16str(vdy,buff2,20),Frac16str(vdz,buff3,20));
      // printf("hddx=%s, hddy=%s,
      // hddz=%s\n",Frac16str(hddx,buff1,20),Frac16str(hddy,buff2,20),Frac16str(hddz,buff3,20));

      memcpy (&starting_point, vertex_array[0], (size_t)sizeof (point3f16));

      for (j = 0; j <= seg_height; j++)
        {
          x = starting_point.x;
          y = starting_point.y;
          z = starting_point.z;

          for (k = 0; k <= seg_width; k++)
            {
              *imesh_data = x;
              imesh_data++;

              *imesh_data = y;
              imesh_data++;

              *imesh_data = z;
              imesh_data++;

              // printf("(%ld,%ld) x=%s, y=%s,
              // z=%s\n",j,k,Frac16str(x,buff1,20),Frac16str(y,buff2,20),Frac16str(z,buff3,20));

              x += hdx;
              y += hdy;
              z += hdz;
            }

          /*** reset to the start of the next data line ***/

          starting_point.x += vdx;
          starting_point.y += vdy;
          starting_point.z += vdz;

          // printf(" hdx=%s,  hdy=%s,
          // hdz=%s\n",Frac16str(hdx,buff1,20),Frac16str(hdy,buff2,20),Frac16str(hdz,buff3,20));

          hdx += hddx;
          hdy += hddy;
          hdz += hddz;
        }
    }

  object->wo_TransformedData = (int32 *)wport->wp_CurrentPointHeap;

  /*** need to do this since I cannot directly add values to a void * ***/

  heap_ptr = (char *)wport->wp_CurrentPointHeap;
  heap_ptr += (sizeof (point3f16) * gdef->gd_PointCount);
  wport->wp_CurrentPointHeap = (void *)heap_ptr;

  /*** generate the face centers here also, now since we have the vertex data.
   * ***/

  conn = (int32 *)gdef->gd_SurfaceList;
  vertex_list = (struct point3f16 *)object->wo_TransformedData;
  surface_list = (int32 *)gdef->gd_SurfaceList;

  /*** we only need the transformed face-center array, since there are no
   * "fixed" center-points ***/

  center = (struct point3f16 *)gdef->gd_FaceCentersXformed;

  for (i = 0; i < gdef->gd_SurfaceCount; i++)
    {
      center->x = 0;
      center->y = 0;
      center->z = 0;

      for (j = 0; j < 4; j++)
        {
          /*** collect together all 4 quad points ***/

          point_id = *conn;
          vertex = (struct point3f16 *)(vertex_list + point_id);

          center->x += vertex->x;
          center->y += vertex->y;
          center->z += vertex->z;

          conn++;
        }

      center->x >>= 2; /* div by 4 */
      center->y >>= 2;
      center->z >>= 2;

      center++;
    }

  EXIT ("GenerateImeshData");
}

/**************************************************************************
 * GetCameraAddress : returns the address of a camera from the supplied   *
 *     handle.                                                            *
 **************************************************************************/
WorldObject *
GetCameraAddress (WorldPort *world, Obj3D camera_id)
{
  int32 camera_index;
  WorldObject *cam, *rvalue;

  if (camera_id < MAX3DCAMERAS)
    {
      camera_index = world->wp_CameraHandles[camera_id];
      cam = (WorldObject *)world->wp_UserObjects[camera_index];

      rvalue = cam;
    }
  else
    rvalue = NULL;

  return (rvalue);
}

/**************************************************************************
 * Get3DLibState : collects together data about the current state of the  *
 *     3D-lib system on a World by World basis.                           *
 **************************************************************************/
void
Get3DLibState (WorldPort *world, Lib3DState *state)
{
  int32 i, j;
  int32 flags;
  ModelDefinition *mdef;
  GeoDefinition *gdef;
  int32 maxpoints = 0;       // point count for largest object
  int32 normalcount = 0;     // used for calculating memory needed by normals
  int32 facecentercount = 0; //     "             "  face centers
  int32 gdefcount = 0;
  WorldObject *object;

  ENTER (Get3DLibState);

  memset (state, 0x00, sizeof (Lib3DState));

  sprintf (state->l3ds_Version, "%10s", VERSION);

  state->l3ds_TotalObjectCount = world->wp_TotalObjectCount;
  state->l3ds_HeapSize = world->wp_PointHeapSize;

  /*
  typedef struct Lib3DState
  {
                // point count of largest object

     if((flags&CAMERATYPE)||(flags&SOUNDTYPE)||(flags&SIMPLESOUNDTYPE))
     {
  */

  for (i = 0; i < state->l3ds_TotalObjectCount; i++)
    {
      object = (WorldObject *)world->wp_UserObjects[i];

      if (object)
        {
          flags = object->wo_ObjectFlags;

          if (flags & CAMERATYPE)
            state->l3ds_CameraCount++;
          else if (flags & POLYGONTYPE)
            state->l3ds_ModelCount++;
          else if (flags & BITMAPTYPE)
            state->l3ds_ModelCount++;
          else if (flags & SOUNDTYPE)
            state->l3ds_SoundCount++;
          else if (flags & SIMPLESOUNDTYPE)
            state->l3ds_SimpleSoundCount++;

          if (flags & BITMAPTYPE)
            {

              mdef = (ModelDefinition *)object->wo_ModelDefinition;

              for (j = 0; j < mdef->md_NumberOfScales; j++)
                {
                  gdef = mdef->md_GeoDefs[j];

                  state->l3ds_FaceCount += gdef->gd_SurfaceCount;
                  state->l3ds_VertexCount += gdef->gd_PointCount;

                  if (gdef->gd_PointCount > maxpoints)
                    maxpoints = gdef->gd_PointCount;

                  if (gdef->gd_FaceNormals)
                    normalcount += gdef->gd_SurfaceCount;
                  if (gdef->gd_FaceCenters)
                    facecentercount += 2 * gdef->gd_SurfaceCount;

                  gdefcount++;
                }

              mdef = (ModelDefinition *)object->wo_MeshDefinition;

              if (mdef)
                {
                  state->l3ds_MeshCount++;

                  for (j = 0; j < mdef->md_NumberOfScales; j++)
                    {
                      gdef = mdef->md_GeoDefs[j];

                      state->l3ds_FaceCount += gdef->gd_SurfaceCount;
                      state->l3ds_VertexCount += gdef->gd_PointCount;

                      if (gdef->gd_PointCount > maxpoints)
                        maxpoints = gdef->gd_PointCount;
                      if (gdef->gd_FaceNormals)
                        normalcount += gdef->gd_SurfaceCount;
                      if (gdef->gd_FaceCenters)
                        facecentercount += 2 * gdef->gd_SurfaceCount;

                      gdefcount++;
                    }
                }
            }
        }
    }

  state->l3ds_HeapReq = state->l3ds_VertexCount * sizeof (point3f16)
                        + state->l3ds_MaxPoints * sizeof (point3f16);

  state->l3ds_MemReq
      = state->l3ds_HeapSize + sizeof (WorldPort)
        + state->l3ds_SimpleSoundCount * sizeof (SoundDefinition)
        + state->l3ds_VertexCount * sizeof (point3f16)
        + state->l3ds_TotalObjectCount * sizeof (WorldObject)
        + state->l3ds_ModelCount * sizeof (ModelDefinition)
        + gdefcount * sizeof (GeoDefinition) + normalcount * sizeof (vec3f16)
        + facecentercount * sizeof (vec3f16);

  state->l3ds_MaxPoints = maxpoints;

  EXIT (Get3DLibState);
}

/**************************************************************************
 * Init3DLib : initialize some internal variables used by SwapScreen,     *
 *     and returns the index for the first render_screen.                 *
 **************************************************************************/
int32
Init3DLib (int32 view_mode, int32 flags)
{
  ENTER ("Init3DLib");

  View_mode = view_mode;

  if (view_mode == STEREO_VIEW)
    {
      Eye = LEFT_EYE;
      Render_screen = 0;
      Left_screen = 1;
      Right_screen = 2;
    }
  else
    {
      Render_screen = 0;
    }

  Lib_initialized = TRUE;

  EXIT ("Init3DLib");

  return (Render_screen);
}

/**************************************************************************
 * Init3DObject : initializes an object structure to predictable known    *
 *     values.                                                            *
 **************************************************************************/
Err
Init3DObject (WorldObject *initobj, ModelDefinition *mdef, TagArg *taglist,
              int32 objecttype)
{
  int32 i, j;
  int32 len;
  GeoDefinition *geodef;
  int32 normallist_created;
  static CCB *whiteccb = NULL;
  ModelSurface *surface;

  ENTER ("Init3DObject");

  normallist_created = 0;

  initobj->wo_WorldPosition[X_COORD] = 0;
  initobj->wo_WorldPosition[Y_COORD] = 0;
  initobj->wo_WorldPosition[Z_COORD] = 0;

  /*** make the object (if its a camera) look into the screen along the z axis
   * ***/

  initobj->wo_ObjectDirection[X_COORD] = 0;
  initobj->wo_ObjectDirection[Y_COORD] = 0;
  initobj->wo_ObjectDirection[Z_COORD] = 0;

  initobj->wo_ObjectNormal[X_COORD] = 0;
  initobj->wo_ObjectNormal[Y_COORD] = 0;
  initobj->wo_ObjectNormal[Z_COORD] = 1 << 16;

  /*** default to a bubblesort and xyz rotate method. ***/

  initobj->wo_MatrixMethod = ROTATE_YXZ | BUBBLE_SORT;
  initobj->wo_ParentObject = NULL;  /* Object attach not supported yet. */
  initobj->wo_PreProcessObject = 0; /* No preprocessing code */
  initobj->wo_RenderTimeProc = 0;   /* No code during CCB load */
  initobj->wo_SphericalRadius = 0;
  initobj->wo_ObjectVelocity = 0; /* Make the object at rest */
  initobj->wo_CameraVisibility
      = ALL_CAMERAS
        | ALL_DISTANCES; /* visible to all cameras at all distances */
  initobj->wo_MeshDefinition = 0;
  initobj->wo_FieldScale = 250;
  initobj->wo_LightPrefs = 0;
  initobj->wo_LightProc = NULL; /* No custom lighting code */
  initobj->wo_MappingProc = NULL;
  initobj->wo_ObjectCullingProc = NULL;
  initobj->wo_FaceCullingProc = NULL;
  initobj->wo_SimpleSoundProc = NULL;
  initobj->wo_ScaleProc = NULL;
  initobj->wo_ObjectFlags = PSEUDO_PERSPECTIVE | CULL_BACKFACE | objecttype;
  initobj->wo_CameraLens = 0;  /* if>0 system will solve for CameraField */
  initobj->wo_CameraField = 0; /* like FieldScale but for Camera-perspective */
  initobj->wo_MaxZScale = Convert32_F16 (32767);
  initobj->wo_UpdateFlags = 0;
  initobj->wo_ScaleUsed = 0;
  //   initobj->wo_ObjectFlags=objecttype;
  initobj->wo_PerspProc = PseudoMapping;
  initobj->wo_HeirLevel = 0;

  /*** if !mdef, no model is supplied so use the existing value ***/

  if (mdef)
    {
      initobj->wo_ObjectDefinition = (int32 *)mdef;
      initobj->wo_ModelDefinition = (int32 *)mdef;
      initobj->wo_SphericalRadius = GetModelRadius (initobj);
    }

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            case F3D_TAG_VISIBILITY:
              initobj->wo_CameraVisibility = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_OBJECT_RADIUS:
              initobj->wo_SphericalRadius = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_MODEL_MAXZSCALE:
              initobj->wo_MaxZScale = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_CAMERALENS:
              initobj->wo_CameraLens = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_MATRIX_METHOD:
              /*** erase only the bottom byte where the method is stored, to
               *preserve
               *** other data. ***/
              initobj->wo_MatrixMethod &= 0xfffffff0;
              initobj->wo_MatrixMethod |= (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_OBJECT_FLAGS:
              initobj->wo_ObjectFlags = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_FIELDSCALE:
              initobj->wo_FieldScale = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_MODEL_LIGHT_AMBIENT:
              initobj->wo_LightPrefs |= ((int32)taglist->ta_Arg & 0x0000000f);
              break;

            case F3D_TAG_MODEL_LIGHTS:
              initobj->wo_LightPrefs
                  ^= (((int32)taglist->ta_Arg) & 0x000000f0);
              break;

            case F3D_TAG_MODEL_NORMALS:
              /*** it's necessary to use a flag to specify if the normallist
               *was
               *** created instead of using the list pointer itself since it
               *** is uninitialized if this tag is unused and therefore it's
               *value
               *** is indeterminate. ***/

              normallist_created = TRUE;
              if (mdef)
                CreateNormalList (initobj);
              break;

            case F3D_TAG_MODEL_PARENT:
              initobj->wo_ParentObject = (WorldObject *)taglist->ta_Arg;
              initobj->wo_HeirLevel
                  = initobj->wo_ParentObject->wo_HeirLevel + 1;
              break;
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  if (initobj->wo_ObjectFlags & FACE_CULLING1)
    initobj->wo_FaceCullingProc = FaceCulling1Proc;
  else if (initobj->wo_ObjectFlags & FACE_CULLING2)
    initobj->wo_FaceCullingProc = FaceCulling2Proc;
  else if (initobj->wo_ObjectFlags & FACE_CULLING3)
    {
      initobj->wo_FaceCullingProc = FaceCulling3Proc;
    }

  if (initobj->wo_ObjectFlags & CAMERA_PERSPECTIVE)
    initobj->wo_PerspProc = CameraMapping;

  if (!whiteccb)
    whiteccb = CreateColorCCB (31L, 31L, 31L);

  /*
  if(Debug_flag)
  {
  printf("initobject whiteccb=%lx,
  whitedata=%lx\n",whiteccb,whiteccb->ccb_CelData);
  }
  */

  if (mdef)
    {
      if ((objecttype == BITMAPTYPE) || (objecttype == POLYGONTYPE))
        {
          initobj->wo_ObjectDefinition = (int32 *)mdef;
          initobj->wo_ModelDefinition = (int32 *)mdef;
          // SourceData is now used by mesh objects
          //         initobj->wo_SourceData=(int32
          //         *)mdef->md_GeoDefs[0]->gd_PointList;

          for (i = 0; i < mdef->md_NumberOfScales; i++)
            {
              geodef = mdef->md_GeoDefs[i];

              /*** allocated a surfacetype array ***/

              // printf("geodef=%lx, count=%lx,
              // surface_type=%lx\n",geodef,geodef->gd_SurfaceCount,geodef->gd_SurfaceType);
              if (!geodef->gd_SurfaceType)
                {
                  len = sizeof (ModelSurface) * geodef->gd_SurfaceCount;
                  geodef->gd_SurfaceType = (void *)malloc (len);

                  // printf("allocating surfacetype=%lx, len=%ld\n",
                  // geodef->gd_SurfaceType,len);
                }

              surface = (ModelSurface *)(geodef->gd_SurfaceType);

              /*** init all faces to white ***/

              for (j = 0; j < geodef->gd_SurfaceCount; j++)
                {
                  surface->ms_Control = (int32)whiteccb;
                  surface++;
                }

              geodef->gd_FaceCenters = NULL;
              geodef->gd_FaceCentersXformed = NULL;

              if (!normallist_created)
                geodef->gd_FaceNormals = NULL;
            }
        }
    }

  /*** allocated a face-center array if FC5 or FACE_SORTING is requested. ***/

  if ((initobj->wo_ObjectFlags & FACE_CULLING3)
      || (initobj->wo_ObjectFlags & FACE_SORTING))
    {
      CreateFaceCenters (initobj);
    }

  /*
  vertex=initobj->wo_SourceData;

  for(i=0;i<10;i++)
  {
  printf("vertex=%lx, x=%lx, y=%lx,
  z=%lx\n",vertex,vertex->x,vertex->y,vertex->z); vertex++;
  }
  */

  for (i = 0; i < 3; i++)
    {
      for (j = 0; j < 3; j++)
        {
          if (i == j)
            initobj->wo_ObjectMatrix[i][j] = (frac16)0x10000;
          else
            initobj->wo_ObjectMatrix[i][j] = (frac16)0;
        }
    }

  EXIT ("Init3DObject");

  return 0; /* No error */
}

/**************************************************************************
 * Init3DSound : initializes a sound object structure to predictable known*
 *     values.                                                            *
 **************************************************************************/
Err
Init3DSound (WorldObject *initobj, char *soundname, TagArg *taglist)
{
  SoundDefinition *sound;
  int32 rvalue;
  Item sample;
  Item attachment;
  char fullfilename[100];
  Sound3D *sound3d;
  Item leftknob, rightknob;

  rvalue = 0;
  sound3d = NULL;

  ENTER ("Init3DSound");

  /*** fire up the mixer if not already loaded ***/

  if (!MixerIns)
    {
      MixerIns = LoadInstrument (MIXER, 0, 0);

      if (!MixerIns)
        {
          printf ("unable to load %s\n", MIXER);
        }

      leftknob = GrabKnob (MixerIns, "LeftGain0");
      rightknob = GrabKnob (MixerIns, "RightGain0");

      TweakKnob (leftknob, 0x3fff);
      TweakKnob (rightknob, 0x0);

      leftknob = GrabKnob (MixerIns, "LeftGain1");
      rightknob = GrabKnob (MixerIns, "RightGain1");

      if (leftknob < 0)
        printf ("leftgain1 is bad\n");
      if (rightknob < 0)
        printf ("rightgain1 is bad\n");

      TweakKnob (leftknob, 0);
      TweakKnob (rightknob, 0x3fff);

      StartInstrument (MixerIns, NULL);
    }

  /*** load in and start the 3D sound ***/

  /*** LoadSample() crashes in spriteworld ***/

  if (fullfilename)
    {
      sprintf (fullfilename, "$audio/aiff/%s", soundname);
      sample = LoadSample (fullfilename);

      if (!sample)
        {
          return (-1);
        }

      sound3d = Open3DSound ();

      if (Load3DSound (sound3d) < 0)
        {
          return (-1);
        }

      rvalue = ConnectInstruments (sound3d->s3d_InsItem, "OutputLeft",
                                   MixerIns, "Input0");
      if (rvalue < 0)
        printf ("left connection was bad! rvalue=%ld\n", rvalue);

      rvalue = ConnectInstruments (sound3d->s3d_InsItem, "OutputRight",
                                   MixerIns, "Input1");
      if (rvalue < 0)
        printf ("right connection was bad! rvalue=%ld\n", rvalue);

      attachment = AttachSample (sound3d->s3d_InsItem, sample, "LeftInFIFO");
      attachment = AttachSample (sound3d->s3d_InsItem, sample, "RightInFIFO");

      if (rvalue < 0)
        {
          printf ("Unable to attach item!\n");
          return (-1);
        }
    }

  /*** init the sound object structure ***/

  Init3DObject (initobj, NULL, NULL, SOUNDTYPE);

  initobj->wo_SoundDef = (SoundDefinition *)malloc (sizeof (SoundDefinition));

  if (!initobj->wo_SoundDef)
    {
      rvalue = 0;
      goto DONE;
    }

  initobj->wo_SourceData = (int32 *)sound3d;

  sound = initobj->wo_SoundDef;

  sound->sd_MinSoundDistance = 0;
  sound->sd_MaxSoundDistance = Convert32_F16 (50);

  sound->sd_StartSound.pp4d_Radius = 0;
  sound->sd_StartSound.pp4d_Theta = 0;
  sound->sd_StartSound.pp4d_Phi = 0;
  sound->sd_StartSound.pp4d_Time = 0;

  sound->sd_EndSound.pp4d_Radius = 0;
  sound->sd_EndSound.pp4d_Theta = 0;
  sound->sd_EndSound.pp4d_Phi = 0;
  sound->sd_EndSound.pp4d_Time = 0;
  sound->sd_Flags = 0;

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            case F3D_TAG_SOUND_MIN_DIST:
              sound->sd_MinSoundDistance = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_SOUND_MAX_DIST:
              sound->sd_MaxSoundDistance = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_SOUND_MIN_VOLUME:
              sound->sd_MinSoundVolume = (frac16)taglist->ta_Arg;
              break;
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  /*** the SoundScale maps the user's 3D world distance to the 3d sound
   *distance units
   *** which are calculated based on the speed of sound. This goes into the
   *** pp4d_Radius value of Sound3D, where each unit equals about .28 inches.
   ****/

  /*** Check : The 6000 is a hardcoded value for now, which is the approximate
   *** distance at which the 3d sound amplitude drops to inaudible. ***/

  sound->sd_SoundScale = DivSF16 ((6000 << 16), sound->sd_MaxSoundDistance);

  EXIT ("Init3DSound");

DONE:
  return (rvalue);
}

/**************************************************************************
 * InitCamera : initializes an object structure to predictable known      *
 *     values. Simple front-end to Init3DObject, to correct some naming   *
 *     problems. This could assume further duties in future revs if       *
 *     a greater distiction is made between cameras and other objects.    *
 **************************************************************************/
Err
InitCamera (WorldObject *initobj, TagArg *taglist)
{
  Err error;

  error = Init3DObject (initobj, NULL, taglist, CAMERATYPE);

  return (error);
}

/**************************************************************************
 * InitGlasses : initialize the stereo glasses for the most common case   *
 *     of the left/right lenses alternating with the video-frame.         *
 **************************************************************************/
Err
InitGlasses (Item eb_port, Item reply_port)
{
  Item smsgitem, rmsgitem;
  ConfigurationRequest configmsg;
  Message *msgptr;
  EventBrokerHeader ebheader, *ebheader_ptr;
  PodDescriptionList *pdl;
  int32 i;
  Err error;
  static int32 glasspod_id = -1;
  PodData pod_command;
  static int32 internal_eb_port = FALSE;
  static int32 internal_reply_port = FALSE;

  ENTER ("InitGlasses");

  /*** initialize the ports : find the event-broker port, create my port for
   *the
   *** return acknowledge messages, and allocate a message structure for them.
   ****/

  if (!eb_port)
    {
      eb_port
          = FindNamedItem (MKNODEID (KERNELNODE, MSGPORTNODE), "eventbroker");
      if (eb_port < 0)
        return (-6);

      internal_eb_port = TRUE;
    }

  if (!reply_port)
    {
      reply_port = CreateMsgPort ("3dmsgport", 0, 0);
      if (reply_port < 0)
        return (-5);

      internal_reply_port = TRUE;
    }

  smsgitem = CreateSizedMsg ("msg", 0, reply_port, 200);

  /*** initialize the event-brokers hello! message ***/

  configmsg.cr_Header.ebh_Flavor = EB_Configure;
  configmsg.cr_Category = LC_Observer;

  for (i = 0; i < 8; i++)
    {
      configmsg.cr_TriggerMask[i] = 0;
      configmsg.cr_CaptureMask[i] = 0;
    }

  configmsg.cr_QueueMax = EVENT_QUEUE_DEFAULT;

  /*** send the configuration message and receive the reply ***/

  error = SendMsg (eb_port, smsgitem, (void *)&configmsg,
                   sizeof (struct ConfigurationRequest));
  if (error < 0)
    printf ("error with SendMsg!\n");

  rmsgitem = WaitPort (reply_port, 0);
  if (rmsgitem < 0)
    printf ("error with WaitPort!\n");

  msgptr = (Message *)LookupItem (rmsgitem);
  if (!msgptr)
    printf ("unable to LookupItem!\n");

  ebheader_ptr = (EventBrokerHeader *)msgptr->msg_DataPtr;

  if (((Err)msgptr->msg_Result) < 0)
    {
      return (-2);
    }

  /*** get the pod number of the glasses ***/

  ebheader.ebh_Flavor = EB_DescribePods;

  /*** leaving this SendMsg line here causes the printf bug above ***/

  error = SendMsg (eb_port, smsgitem, (void *)&ebheader,
                   sizeof (EventBrokerHeader));

  if (error < 0)
    printf ("error with SendMsg!\n");

  rmsgitem = WaitPort (reply_port, 0);
  if (rmsgitem < 0)
    return (-3);

  msgptr = (Message *)LookupItem (rmsgitem);
  if (!msgptr)
    return (-4);

  pdl = (PodDescriptionList *)msgptr->msg_DataPtr;

  pod_command.pd_Header.ebh_Flavor = EB_IssuePodCmd;

  pod_command.pd_WaitFlag = 0;
  pod_command.pd_DataByteCount = 4;
  pod_command.pd_Data[0] = GENERIC_GlassesCtlr;
  pod_command.pd_Data[1] = GENERIC_GLASSES_SetView;
  pod_command.pd_Data[2] = GLASSES_OnEvenField;
  pod_command.pd_Data[3] = GLASSES_OnOddField;

  /*** this looks for all glasses and activates them ***/

  for (i = 0; i < pdl->pdl_PodCount; i++)
    {
      if (pdl->pdl_Pod[i].pod_Flags & POD_IsGlassesCtlr)
        {
          glasspod_id = pdl->pdl_Pod[i].pod_Number;
          pod_command.pd_PodNumber = glasspod_id;

          error = SendMsg (eb_port, smsgitem, (void *)&pod_command,
                           sizeof (PodData));
          WaitPort (reply_port, 0);
        }
    }

  DeleteMsg (smsgitem);

  if (internal_eb_port)
    DeleteMsgPort (eb_port);
  if (internal_reply_port)
    DeleteMsgPort (reply_port);

  EXIT ("InitGlasses");

  return (0);
}

/**************************************************************************
 * InitModel : initializes an object structure to predictable known       *
 *     values. Simple front-end to Init3DObject, to correct some naming   *
 *     problems.                                                          *
 **************************************************************************/
Err
InitModel (WorldObject *initobj, ModelDefinition *mdef, TagArg *taglist)
{
  Err error;

  error = Init3DObject (initobj, mdef, taglist, BITMAPTYPE);

  return (error);
}

/**************************************************************************
 * InitNewWorld : inits a WorldPort structure                             *
 **************************************************************************
 * NOTE : additional important general notes                              *
 **************************************************************************/
Err
InitNewWorld (WorldPort *world, char *pointheap, int32 pointheapsize,
              TagArg *taglist)
{
  int32 i, j;

  ENTER ("InitNewWorld");

  if (!Lib_initialized)
    return (-1);

  world->wp_MaxObjects = MAX3DOBJECTS;
  world->wp_PointHeapPtr = pointheap;
  world->wp_PointHeapSize = pointheapsize;
  world->wp_TotalSortedObjects = 0;
  world->wp_TotalObjectCount = 0;
  world->wp_SortProc = NULL;
  world->wp_TotalSimpleSounds = 0;
  world->wp_SortProc = NULL;
  world->wp_CollisionProc = NULL;

  world->wp_EyeZDist = 8 << 16;
  world->wp_EyeSep = 8 << 12;

  if (View_mode == STEREO_VIEW)
    world->wp_Eye = RIGHT_EYE;
  else
    world->wp_Eye = -1; /* -1=MONO-ViewMode */

  Face_rotation_flag = CCW_FACES; // the old, wrong, way. maintained for
                                  // backward compatibility

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            case F3D_TAG_EYE_ZDIST:
              world->wp_EyeZDist = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_EYE_SEP:
              world->wp_EyeSep = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_FACE_ORIENTATION:
              Face_rotation_flag = (int32)taglist->ta_Arg;
              break;
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  /*** highest index used in the object list, needed to object
   *** searches from hunting through the entire list. ***/

  world->wp_HighestObjectIndex = 0;

  /*** Must be a better way to initialize memory, but for now,
   *** I will do it like this. */

  while ((--pointheapsize) >= 0)
    *pointheap++ = 0;

  i = (MAX3DCAMERAS + 2) * MAX3DOBJECTS;
  j = 0;

  /*** This inits both the UserObjects array AND the sorted objects
   *** array at the same time. ***/

  while ((--i) >= 0)
    world->wp_UserObjects[j++] = 0;

  for (i = 0; i < MAX3DCAMERAS; i++)
    world->wp_CameraHandles[i] = -1;
  for (i = 0; i < MAX3DSOUNDS; i++)
    world->wp_SoundHandles[i] = -1;
  for (i = 0; i < MAXSIMPLESOUNDS; i++)
    world->wp_SimpleSoundHandles[i] = -1;
  for (i = 0; i < MAXLIGHTS; i++)
    {
      world->wp_LightPosition[i][X_COORD] = 0;
      world->wp_LightPosition[i][Y_COORD] = 0;
      world->wp_LightPosition[i][Z_COORD] = 0;
    }

  EXIT ("InitNewWorld");

  return (0);
}

/**************************************************************************
 * InitRender3DPort : inits a Render3DPort structure                      *
 **************************************************************************/
void
InitRender3DPort (Render3DPort *rport, Item bitmapitem)
{
  Bitmap *bitmap;

  ENTER ("InitRender3DPort\n");

  bitmap = (Bitmap *)LookupItem (bitmapitem);

  rport->rp_BitmapItem = bitmapitem;

  rport->rp_Width = bitmap->bm_Width;
  rport->rp_Height = bitmap->bm_Height;

  rport->rp_Center_x = rport->rp_Width / 2;
  rport->rp_Center_y = rport->rp_Height / 2;

  EXIT ("InitRender3DPort\n");
}

/**************************************************************************
 * InitSimpleSound : initializes a sound object structure to predictable  *
 *     known values.                                                      *
 **************************************************************************/
Err
InitSimpleSound (WorldObject *initobj, char *soundname, TagArg *taglist)
{
  SoundDefinition *sound;
  frac16 tempf16;
  int32 rvalue;
  Item sample;

  rvalue = 0;

  ENTER ("InitSimpleSound");

  // printf("loading MIXER, %s, and SAMPLER %s\n",MIXER,SAMPLER);

  if (!MixerIns)
    {
      MixerIns = LoadInstrument (MIXER, 0, 0);

      if (!MixerIns)
        {
          printf ("unable to load %s\n", MIXER);
          rvalue = -1;
          goto DONE;
        }
      else
        StartInstrument (MixerIns, NULL);
    }

  Init3DObject (initobj, NULL, NULL, SIMPLESOUNDTYPE);

  initobj->wo_SoundDef = (SoundDefinition *)malloc (sizeof (SoundDefinition));

  if (!initobj->wo_SoundDef)
    {
      rvalue = 0;
      goto DONE;
    }

  sound = initobj->wo_SoundDef;

  sound->sd_MinSoundDistance = 0;
  sound->sd_MaxSoundDistance = Convert32_F16 (50);
  sound->sd_MinSoundVolume = 0x00007fff;
  sound->sd_Flags = 0;

  /*** load in the sample. If that works, load in the instrument. ***/

  sample = LoadSample (soundname);

  if (sample < 0)
    {
      printf ("unable to open sample, error=%ld\n", sample);
      return (-1);
    }

  if ((initobj->wo_SoundDef->sd_Sampler = LoadInstrument (SAMPLER, 0, 100))
      < 0)
    {
      printf ("unable to open the %ls\n", SAMPLER);
      return (-1);
    }

  AttachSample (initobj->wo_SoundDef->sd_Sampler, sample, NULL);

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            case F3D_TAG_SOUND_MIN_DIST:
              sound->sd_MinSoundDistance = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_SOUND_MAX_DIST:
              sound->sd_MaxSoundDistance = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_SOUND_MIN_VOLUME:
              sound->sd_MinSoundVolume = (int32)taglist->ta_Arg;
              break;
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  tempf16 = (sound->sd_MaxSoundDistance - sound->sd_MinSoundDistance);

  sound->sd_SoundScale = DivSF16 (0x7fff0000, tempf16);

  EXIT ("InitSimpleSound");

DONE:
  return (rvalue);
}

/**************************************************************************
 * ModifyObject:  Modifies an object on the fly using tag lists           *
 **************************************************************************/
void
ModifyObject (WorldObject *object, TagArg *taglist)
{
  ENTER ("ModifyObject");

  if (taglist)
    {
      do
        {
          switch (taglist->ta_Tag)
            {
            case F3D_TAG_VISIBILITY:
              object->wo_CameraVisibility = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_OBJECT_RADIUS:
              object->wo_SphericalRadius = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_MODEL_MAXZSCALE:
              object->wo_MaxZScale = (frac16)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_CAMERALENS:
              object->wo_CameraLens = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_MATRIX_METHOD:
              /*** erase only the bottom byte where the method is stored, to
               *preserve
               *** other data. ***/
              object->wo_MatrixMethod &= 0xfffffff0;
              object->wo_MatrixMethod |= (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_OBJECT_FLAGS:
              object->wo_ObjectFlags = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_CAMERA_FIELDSCALE:
              object->wo_FieldScale = (int32)taglist->ta_Arg;
              break;

            case F3D_TAG_MODEL_LIGHT_AMBIENT:
              object->wo_LightPrefs |= ((int32)taglist->ta_Arg & 0x0000000f);
              break;

            case F3D_TAG_MODEL_LIGHTS:
              /* clear out the previous lights in the second nybble */

              object->wo_LightPrefs &= 0xffffff0f;
              object->wo_LightPrefs |= ((int32)taglist->ta_Arg);
              break;

            case F3D_TAG_MODEL_NORMALS:
              /*** it's necessary to use a flag to specify if the normallist
               *was
               *** created instead of using the list pointer itself since it
               *** is uninitialized if this tag is unused and therefore it's
               *value
               *** is indeterminate. ***/

              CreateNormalList (object);
              break;
            }

          taglist++;
        }
      while (taglist->ta_Tag);
    }

  if (object->wo_ObjectFlags & FACE_CULLING1)
    object->wo_FaceCullingProc = FaceCulling1Proc;
  else if (object->wo_ObjectFlags & FACE_CULLING2)
    object->wo_FaceCullingProc = FaceCulling2Proc;
  else if (object->wo_ObjectFlags & FACE_CULLING3)
    object->wo_FaceCullingProc = FaceCulling3Proc;

  if (object->wo_ObjectFlags & CAMERA_PERSPECTIVE)
    object->wo_PerspProc = CameraMapping;

  EXIT ("ModifyObject");
}

/**************************************************************************
 * RemoveModel : removes an object from the lists                         *
 **************************************************************************/
Err
RemoveModel (WorldPort *world, Obj3D obj)
{
  int32 flags;

#if ERROR_CHECK
  uint32 k;
  WorldObject *object;

  k = world->wp_MaxObjects;

  if ((obj > k) || (world->wp_UserObjects[obj] == 0))
    return -1;
#endif

  object = (WorldObject *)world->wp_UserObjects[obj];

  world->wp_UserObjects[obj] = 0;
  world->wp_TotalObjectCount--;

  ResetHighestIndex (world, obj);

  flags = object->wo_ObjectFlags;

  if ((flags & CAMERATYPE) || (flags & SOUNDTYPE) || (flags & SIMPLESOUNDTYPE))
    {
      return (0);
    }
  else
    {
      world->wp_TotalSortedObjects--;
      ResortLinks (world);
    }

  return 0;
}

/**************************************************************************
 * RemoveCamera : deletes a camera from the 3d object lists               *
 **************************************************************************/
Err
RemoveCamera (WorldPort *world, Obj3D obj)
{
  Err error;
  uint32 j;

  j = world->wp_MaxObjects;

#if ERROR_CHECK
  if ((obj > MAX3DCAMERAS) || (world->wp_CameraHandles[obj] < 0)
      || (world->wp_CameraHandles[obj] > j))
    return -1;
#endif

  error = RemoveModel (world, world->wp_CameraHandles[obj]);
#if ERROR_CHECK
  if (!error)
    world->wp_CameraHandles[obj] = -1;
#else
  world->wp_CameraHandles[obj] = -1;
#endif

  return error;
}

/**************************************************************************
 * Remove3DSound : removes a 3D sound object from the object list         *
 **************************************************************************/
Err
Remove3DSound (WorldPort *world, Obj3D obj)
{
  Err error;
  uint32 j;

  j = world->wp_MaxObjects;

#if ERROR_CHECK
  if ((obj > MAX3DSOUNDS) || (world->wp_SoundHandles[obj] < 0)
      || (world->wp_SoundHandles[obj] > j))
    return -1;
#endif

  error = RemoveModel (world, world->wp_SoundHandles[obj]);

#if ERROR_CHECK
  if (!error)
    world->wp_SoundHandles[obj] = -1;
#else
  world->wp_SoundHandles[obj] = -1;
#endif

  return error;
}

/**************************************************************************
 * RemoveSimpleSound : removes a simple sound object from the object list *
 **************************************************************************/
Err
RemoveSimpleSound (WorldPort *world, Obj3D obj)
{
  Err error;
  uint32 j;

  j = world->wp_MaxObjects;

#if ERROR_CHECK
  if ((obj > MAX3DSOUNDS) || (world->wp_SimpleSoundHandles[obj] < 0)
      || (world->wp_SimpleSoundHandles[obj] > j))
    return -1;
#endif
  error = RemoveModel (world, world->wp_SimpleSoundHandles[obj]);

#if ERROR_CHECK
  if (!error)
    {
      world->wp_SimpleSoundHandles[obj] = -1;
      world->wp_TotalSimpleSounds--;
    }
#else
  world->wp_SimpleSoundHandles[obj] = -1;
  world->wp_TotalSimpleSounds--;
#endif

  return error;
}

/**************************************************************************
 * ResetHighestIndex : recalculate the HighestObjectIndex. If the top     *
 *     object is removed, it will go down, if not, it will stay the same. *
 *     The loop is needed to decrement the index. If some earlier objects *
 *     have been removed leaving gaps in the list, then the top one is    *
 *     gone exposing one of the gaps, I need to subtract one for the top  *
 *     object, AND skip over the gaps as well.                            *
 **************************************************************************/
void
ResetHighestIndex (WorldPort *world, Obj3D obj)
{
  int32 i;

  ENTER ("ResetHighestIndex");

  if (world->wp_HighestObjectIndex == obj)
    {
      for (i = world->wp_HighestObjectIndex; i >= 0; i++)
        if (world->wp_UserObjects[i])
          break; /* found the next highest object */

      world->wp_HighestObjectIndex = i;
    }

  EXIT ("ResetHighestIndex");
}

/**************************************************************************
 * ScanFaces : scan through an facelist of an object and calls the        *
 *     supplied callback for each found.                                  *
 **************************************************************************/
void
ScanFaces (WorldPort *world, WorldObject *model, int32 (*scanproc) ())
{
  int32 h, i, j, k;
  struct GeoDefinition *geodef;
  point3f16 *vertex[4], *vertex_list;
  int32 *surface_list;
  ModelDefinition *mdefs;
  int32 point_id, point_counter;
  int32 flags;

  ENTER ("ScanFaces");

  mdefs = (ModelDefinition *)model->wo_ModelDefinition;
  flags = model->wo_ObjectFlags;

  for (h = 0; h <= 1; h++)
    {
      if (h == 1)
        {
          if (model->wo_ObjectFlags |= MESH_OBJECT)
            mdefs = (ModelDefinition *)model->wo_MeshDefinition;
          else
            break; // not a mesh object
        }

      for (i = 0; i < mdefs->md_NumberOfScales; i++)
        {
          geodef = mdefs->md_GeoDefs[i];

          vertex_list = (struct point3f16 *)geodef->gd_PointList;
          surface_list = (int32 *)geodef->gd_SurfaceList;

          for (j = 0; j < geodef->gd_SurfaceCount; j++)
            {
              point_counter = (j << 2); /* mult by 4 */

              for (k = 0; k < 4; k++)
                {
                  point_id = surface_list[point_counter + k];
                  vertex[k] = (struct point3f16 *)(vertex_list + point_id);
                }

              (*scanproc) (world, model, geodef, i, vertex);
            }
        }
    }

  EXIT ("ScanFaces");
}

/**************************************************************************
 * ScanObjects : scan through an object list and call the supplied        *
 *     callback for each object found. If camera_id>=0 scan only the      *
 *     objects associated with that camera, otherwise scan the entire list*
 *     Note,that this works only for visible objects!!!                   *
 **************************************************************************/
void
ScanObjects (WorldPort *world, Obj3D camera_id, int32 (*scanproc) ())
{
  WorldObject *obj, *cam;
  int32 index;
  int32 i;

  ENTER ("ScanObjects");

  if (camera_id >= 0)
    {
      cam = GetCameraAddress (world, camera_id);
      index = (world->wp_MaxObjects) * (camera_id + 2);

      for (i = world->wp_ObjectCount[camera_id] - 1; i >= 0; i--)
        {
          obj = (WorldObject *)world->wp_UserObjects[index + i];

          (*scanproc) (world, obj);
        }
    }
  else
    {
      for (i = 0; i < world->wp_HighestObjectIndex; i++)
        {
          obj = (WorldObject *)world->wp_UserObjects[i];

          (*scanproc) (world, obj);
        }
    }

  EXIT ("ScanObjects");
}
