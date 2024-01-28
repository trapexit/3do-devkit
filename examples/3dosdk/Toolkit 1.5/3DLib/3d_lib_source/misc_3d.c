/**************************************************************************
 * misc_3d.c : contains some 3d lib misc. routines.                       *
 **************************************************************************
 *                                                                        *
 * Written and Directed by Mike Smithwick and Dan Duncalf                 *
 *                                                                        *
 * Produced by Dan Duncalf                                                *
 *                                                                        * *
 **************************************************************************
 *                                                                        *
 * Written : April 1993                                                   *
 *                                                                        *
 **************************************************************************
 *                                                                        *
 * Copyright (c) 1993, New Technologies Group, Inc. * All rights reserved
 ** This document is proprietary and confidential *
 *								                                          *
 **************************************************************************/

#include "3dlib.h"
#include "Form3DO.h"
#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"
#include "Utils3DO.h"

extern int32 Debug_flag;
extern int32 Face_rotation_flag;

void printfF16 (char *name, frac16 value);

/*** this following array creates 16 shades of intensity for a cel face for the
 *** lighting stuff, and works as follows :
 *** 	PPMPC_1S_PDC	:	primary source is coming from the decoder (as
 opposed to the frame buffer)
 ***	PPMPC_MS_CCB	:	uses the primary scalar value from this word,
 as defined by the PPMPC_MF_* values following.
 ***	PPMPC_MF_*		:	sets PMV to all possible values,
 ramping up from 1 to 8. Combining this with the PDV (primary divide value) of
 8 our intensity levels go from 1/8 to 8/8 for the first 8 entries. Setting the
 final output secondary divide by two, cuts this in half so the intensity goes
 from 1/16 to 1/2. The second 8 values use PPMPC_2D_PDC, which adds the pixel
 value to itself, effectively doubling it.
 ***    PPMPC_SF_8      :   primary divide value, intensity multiplier is
 created by MF/SF.
 ***    PPMPC_2S_*     	:	2S_0, sez that the secondary source is 0, or
 turned off. If 2S_PDC, it adds the pixel to itself.
 ***	PPMPC_2D_2		:	secondary divide value, divides the
 results by 2.
 ***/

uint32 PPMPs[] = { PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_1 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_2 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_3 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_4 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8
                       | PPMPC_2S_0 | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_1 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_2 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_3 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_4 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_5 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_6 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_7 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2,
                   PPMPC_1S_PDC | PPMPC_MS_CCB | PPMPC_MF_8 | PPMPC_SF_8
                       | PPMPC_2S_PDC | PPMPC_2D_2 };

/**************************************************************************
 * BuildLookAtMatrix : a matrix to support the LookAtMatrix() given       *
 *     a camera, target and up vector.                                    *
 **************************************************************************
 * NOTE : This is taken from Graphics Gems I. It is described on page     *
 *     516, and code is supplied on page 778.                             *
 **************************************************************************/
void
BuildLookAtMatrix (mat33f16 mat, vec3f16 target, vec3f16 camera, vec3f16 up)
{
  vec3f16 look, above, cross;
  frac16 dot;

  ENTER ("BuildLookAtMatrix");

  look[X_COORD] = camera[X_COORD] - target[X_COORD];
  look[Y_COORD] = camera[Y_COORD] - target[Y_COORD];
  look[Z_COORD] = camera[Z_COORD] - target[Z_COORD];

  NormalizeVector (look);

  above[X_COORD] = up[X_COORD] - target[X_COORD];
  above[Y_COORD] = up[Y_COORD] - target[Y_COORD];
  above[Z_COORD] = up[Z_COORD] - target[Z_COORD];

  NormalizeVector (above);

  dot = Dot3_F16 (above, look);

  above[X_COORD] -= (MulSF16 (dot, look[X_COORD]));
  above[Y_COORD] -= (MulSF16 (dot, look[Y_COORD]));
  above[Z_COORD] -= (MulSF16 (dot, look[Z_COORD]));

  NormalizeVector (above);

  Cross3_F16 (cross, above, look);

  mat[0][0] = cross[X_COORD];
  mat[1][0] = cross[Y_COORD];
  mat[2][0] = cross[Z_COORD];

  mat[0][1] = above[X_COORD];
  mat[1][1] = above[Y_COORD];
  mat[2][1] = above[Z_COORD];

  mat[0][2] = look[X_COORD];
  mat[1][2] = look[Y_COORD];
  mat[2][2] = look[Z_COORD];

  EXIT ("BuildLookAtMatrix");
}

/**************************************************************************
 * CenterModelByRadius : adjusts an object's points to center them        *
 *     based on the maximum radius.                                       *
 **************************************************************************/
void
CenterModelByRadius (ModelDefinition *item)
{
  GeoDefinition *gd;
  register int32 x, y, z;
  int32 minx, miny, minz, maxx, maxy, maxz;
  int32 j, k;
  int32 *points;

  ENTER ("CenterModelByRadius");

  minx = miny = minz = maxx = maxy = maxz = 0;

  for (j = 0; j < item->md_NumberOfScales; j++)
    {
      gd = (GeoDefinition *)item->md_GeoDefs[j];
      points = (int32 *)gd->gd_PointList;

      for (k = 0; k < gd->gd_PointCount; k++)
        {
          x = *(points + k * 3);
          y = *(points + k * 3 + 1);
          z = *(points + k * 3 + 2);

          if (minx > x)
            minx = x;

          if (miny > y)
            miny = y;

          if (minz > z)
            minz = z;

          if (maxx < x)
            maxx = x;

          if (maxy < y)
            maxy = y;

          if (maxz < z)
            maxz = z;
        }

      x = (maxx - minx) >> 1;
      y = (maxy - miny) >> 1;
      z = (maxz - minz) >> 1;

      OffsetModel (x, y, z, gd);
    }

  EXIT ("CenterModelByRadius");
}

/**************************************************************************
 * CenterModelByWeight : adjusts an object's points to center them        *
 *     based on the average value of all of the vertex distances.         *
 **************************************************************************/
void
CenterModelByWeight (ModelDefinition *item)
{
  GeoDefinition *gd;
  int32 j, k, x, y, z;
  int32 *points;

  ENTER ("CenterModelByWeight");

  x = y = z = 0;

  for (j = 0; j < item->md_NumberOfScales; j++)
    {
      gd = (GeoDefinition *)item->md_GeoDefs[j];
      points = (int32 *)gd->gd_PointList;

      for (k = 0; k < gd->gd_PointCount; k++)
        {
          x = x + *(points + k * 3);
          y = y + *(points + k * 3 + 1);
          z = z + *(points + k * 3 + 2);
        }

      x = x / (gd->gd_PointCount);
      y = y / (gd->gd_PointCount);
      z = z / (gd->gd_PointCount);
      OffsetModel (x, y, z, gd);
    }

  EXIT ("CenterModelByWeight");
}

/**************************************************************************
 * ChangeLens : simple front end to changing the lens on a camera         *
 **************************************************************************/
void
ChangeLens (WorldPort *world, Obj3D camera_id, int32 fov)
{
  WorldObject *cam;

  cam = GetCameraAddress (world, camera_id);
  cam->wo_CameraLens = fov;
}

WorldObject *CloseObj = NULL; // used by ClosestObject() below

/**************************************************************************
 * closest_proc : callback to ClosestObject, simply figgers out if this   *
 *     object is closest.                                                 *
 **************************************************************************/
int32
closest_proc (WorldPort *world, WorldObject *object)
{
  frac16 dist;
  static frac16 old_dist;

  if (object->wo_ObjectFlags & POLYGONTYPE
      || object->wo_ObjectFlags & BITMAPTYPE)
    {
      dist = VectorLength (object->wo_LocalPosition);

      /*** if CloseObj==NULL, this is the first time through so init
       *** some stuff. ***/

      // printf("object : %lx\n",object);

      if (!CloseObj)
        {
          CloseObj = object;
          old_dist = dist;
        }
      else if (dist < old_dist)
        {
          CloseObj = object;
          old_dist = dist;
        }
    }

  return (0);
}

/**************************************************************************
 * ClosestModel : returns a handle to the closest model                   *
 **************************************************************************/
WorldObject *
ClosestModel (WorldPort *world, Obj3D camera_id)
{
  ENTER ("ClosestModel");

  CloseObj = NULL;

  ScanObjects (world, camera_id, closest_proc);

  EXIT ("ClosestModel");

  return (CloseObj);
}

/**************************************************************************
 * DeRefCameraHandle : returns the real interally used camera handle      *
 *     when given the handle returnd by AddCamera().                      *
 **************************************************************************/
Obj3D
DeRefCameraHandle (WorldPort *world, Obj3D obj)
{
#if ERROR_CHECK
  if ((obj >= 8) || (world->wp_CameraHandles[obj] < 0))
    return -1;
#endif
  return (world->wp_CameraHandles[obj]);
}

/**************************************************************************
 * DoCollisionHandling : checks all possible collision combinations of    *
 *     only objects with they're collision flag set                       *
 **************************************************************************/
void
DoCollisionHandling (WorldPort *world, Obj3D camera_id)
{
  WorldObject *obj1, *obj2, *cam;
  int32 i, j, camera_count, index;

  ENTER ("DoCollisionHandling");

  cam = GetCameraAddress (world, camera_id);

  index = (world->wp_MaxObjects) * (camera_id + 2);

  camera_count = world->wp_ObjectCount[camera_id];

  for (i = -1; i < camera_count; i++)
    {
      /*** obj1 is set up for collision detection, scan the rest of the list,
       *above obj1
       *** for any other collision objects, rinse and repeat. ***/

      /*** we need to see if the camera intersects with any of the objects as
       *well,
       *** so make it the first object through  the loop ***/

      if (i == -1)
        obj1 = cam;
      else
        obj1 = (WorldObject *)world->wp_UserObjects[index + i];

      // printf("aaaa, obj1=%lx, i=%ld\n",obj1,i);
      if (obj1->wo_ObjectFlags & COLLISION_DETECT)
        {

          for (j = i + 1; j < camera_count; j++)
            {
              obj2 = (WorldObject *)world->wp_UserObjects[index + j];

              // printf("doing collision detection!  obj1=%lx, obj2=%lx, i=%ld,
              // j=%ld, index=%ld\n",obj1,obj2,i,j,index);

              if (obj2->wo_ObjectFlags & COLLISION_DETECT)
                {
                  if (DoObjectsIntersect (obj1, obj2)
                      && world->wp_CollisionProc)
                    (*world->wp_CollisionProc) (world, obj1, obj2);
                }
            }
        }
    }

  EXIT ("DoCollisionHandling");
}

#define ABS(a) ((a > 0) ? a : -a)

/**************************************************************************
 * DoObjectsIntersect : returns TRUE if the objects intersect based on    *
 *     the largest of the two sphereical diameters                        *
 **************************************************************************/
int32
DoObjectsIntersect (WorldObject *obj1, WorldObject *obj2)
{
  frac16 radius;
  frac16 del;
  frac16 dist;

  ENTER ("DoObjectsIntersect");

  radius = obj1->wo_SphericalRadius + obj2->wo_SphericalRadius;

  /*** perform a high-level test first to see if the obj1 bounding
   *** box intercepts object 2 at all, then if that is true, use
   *** the more time-intensive distance calculations to see if
   *** the bounding sphere intersects the other object's sphere, since the
   *** could only be touching corners of the box, just out of range of
   *** the sphere. ***/

  del = (obj1->wo_WorldPosition[X_COORD] - obj2->wo_WorldPosition[X_COORD]);
  del = ((del > 0) ? del : -del);

  // printfF16("radius=",radius);
  // printf(" obj1=%lx, obj2=%lx\n",obj1,obj2);

  if (del < radius)
    {
      del = (obj1->wo_WorldPosition[Y_COORD]
             - obj2->wo_WorldPosition[Y_COORD]);
      del = ((del > 0) ? del : -del);

      if (del < radius)
        {
          del = (obj1->wo_WorldPosition[Z_COORD]
                 - obj2->wo_WorldPosition[Z_COORD]);
          del = ((del > 0) ? del : -del);

          if (del < radius)
            {
              dist = Distance3D (obj1->wo_WorldPosition,
                                 obj2->wo_WorldPosition);

              if (dist < radius)
                return (TRUE);
            }
        }
    }

  EXIT ("DoObjectsIntersect");

  return (FALSE);
}

/**************************************************************************
 * DeRefSoundHandle : returns the real interally used sound handle        *
 *     when given the handle returnd by AddSound().                       *
 **************************************************************************/
Obj3D
DeRefSoundHandle (WorldPort *world, Obj3D obj)
{
#if ERROR_CHECK
  if ((obj >= 8) || (world->wp_SoundHandles[obj] < 0))
    return -1;
#endif
  return (world->wp_SoundHandles[obj]);
}

/**************************************************************************
 * GetFaceCCB : returns the ccb of the supplied face                      *
 **************************************************************************/
CCB *
GetFaceCCB (WorldObject *object, int32 scale, int32 face_id, int32 flags)
{
  struct GeoDefinition *geodef;
  struct ModelDefinition *mdef;
  ModelSurface *surfacetype;
  CCB *ccb;

  ENTER ("GetFaceCCB");

  if (flags & USE_MESH_FACE)
    mdef = (ModelDefinition *)object->wo_MeshDefinition;
  else
    mdef = (ModelDefinition *)object->wo_ModelDefinition;

  if (!mdef)
    return (NULL);

  geodef = mdef->md_GeoDefs[scale];

  if (face_id >= geodef->gd_SurfaceCount)
    return (NULL);
  if (scale >= mdef->md_NumberOfScales)
    return (NULL);
  if (!geodef->gd_SurfaceType)
    return (NULL);

  surfacetype = (ModelSurface *)(geodef->gd_SurfaceType);

  surfacetype += face_id;
  ccb = (CCB *)surfacetype->ms_Control;

  EXIT ("GetFaceCCB");

  return (ccb);
}

/**************************************************************************
 * GetModelChunk : recovers the next chunk from the data stream.          *
 **************************************************************************/
char *
GetModelChunk (Stream *stream, uint32 *chunk_ID)
{
  static char *buf = NULL;
  static int32 bufsize = 0;
  static LIST_CHUNK chunk;
  int32 cc;

  /*** free the memory from the last usage ***/

  cc = ReadDiskStream (stream, (char *)&chunk, 12L);

  if (cc < 0)
    {
      printf ("unable to read stream!\n");
      return (NULL);
    }

  // printf("trying to allocate chunk, %ld bytes\n",chunk.header.chunkSize);

  if ((buf = (char *)ALLOCMEM (chunk.header.chunkSize, MEMTYPE_DRAM)) == NULL)
    return (NULL);

  chunk.Data = buf;

  cc = ReadDiskStream (stream, chunk.Data, chunk.header.chunkSize - 12);

  if (cc < 0)
    {
      //      printf("ERROR : unable to read stream!\n");
      return (NULL);
    }

  if (cc == 0)
    {
      //       printf("end of file\n");
      return (NULL);
    }

  bufsize = chunk.header.chunkSize;

  memcpy ((char *)chunk_ID, (char *)&chunk.header.chunkID[0], 4L);

  //   printf("chunk ID=%4c, %4lx\n",*chunk_ID,*chunk_ID);

  return ((char *)&chunk);
}

/**************************************************************************
 * GetModelRadius :                                                       *
 **************************************************************************/
frac16
GetModelRadius (WorldObject *object)
{
  ModelDefinition *md;
  GeoDefinition *gd;
  int32 *points;
  int32 k;
  frac16 x, y, z;
  frac16 minx, miny, minz;
  frac16 maxx, maxy, maxz;
  int64 dx, dy, dz;

  ENTER ("GetModelRadius");

  minx = miny = minz = (32000 < 16);
  maxx = maxy = maxz = (-32000 < 16);

  if (1)
    {
      k = object->wo_ScaleUsed;
      md = (ModelDefinition *)object->wo_ModelDefinition;

      if (k < 0)
        return (0);

      if (k > md->md_NumberOfScales)
        k = 0;

      gd = (GeoDefinition *)md->md_GeoDefs[k];
      points = (int32 *)gd->gd_PointList;
      /*
      printf("pointcount=%ld, %lx, k=%ld\n",gd->gd_PointCount,gd,k);
      printf("object=%lx, md=%lx, pointlist=%lx\n",object,md,points);
      */

      for (k = 0; k < gd->gd_PointCount; k++)
        {
          x = *(points + k * 3);
          y = *(points + k * 3 + 1);
          z = *(points + k * 3 + 2);

          if (minx > x)
            minx = x;

          if (miny > y)
            miny = y;

          if (minz > z)
            minz = z;

          if (maxx < x)
            maxx = x;

          if (maxy < y)
            maxy = y;

          if (maxz < z)
            maxz = z;
        }

      x = (maxx - minx);
      y = (maxy - miny);
      z = (maxz - minz);

      MulS32_64 (&dx, x, x);
      MulS32_64 (&dy, y, y);
      Add64 (&dz, &dx, &dy);
      MulS32_64 (&dy, z, z);
      Add64 (&dx, &dz, &dy);

      x = Sqrt64_32 (&dx);
      x = DivSF16 (x, (2 << 16));
    }

  EXIT ("GetModelRadius");

  return (x);
}

/**************************************************************************
 * GetObjectLocation : dumb interface to set an object's location,        *
 *     handy in the case we change the structure.                         *
 **************************************************************************/
void
GetObjectLocation (WorldObject *object, frac16 *x, frac16 *y, frac16 *z)
{
  ENTER ("GetObjectLocation");

  *x = object->wo_WorldPosition[X_ANGLE];
  *y = object->wo_WorldPosition[Y_ANGLE];
  *z = object->wo_WorldPosition[Z_ANGLE];

  EXIT ("GetObjectLocation");
}

/**************************************************************************
 * GetObjectOrientation : dumb interface to set an object's orientation,  *
 *     handy in the case we change the structure.                         *
 **************************************************************************/
void
GetObjectOrientation (WorldObject *object, frac16 *x, frac16 *y, frac16 *z)
{
  ENTER ("SetObjectOrientation");

  *x = object->wo_WorldOrientation[X_ANGLE];
  *y = object->wo_WorldOrientation[Y_ANGLE];
  *z = object->wo_WorldOrientation[Z_ANGLE];

  EXIT ("GetObjectOrientation");
}

/**************************************************************************
 * GetObjectSpeed : This can be used for either models or cameras, which  *
 *      is why I use "object" in the name.                                *
 **************************************************************************/
int32
GetObjectSpeed (WorldPort *world, Obj3D obj, vec3f16 direction)
{
  int32 speed;
  WorldObject *object;

  /*
     if (obj > world->wp_MaxObjects) return -1;
  */

  object = (WorldObject *)world->wp_UserObjects[obj];

  direction[X_ANGLE] = object->wo_ObjectDirection[X_ANGLE];
  direction[Y_ANGLE] = object->wo_ObjectDirection[Y_ANGLE];
  direction[Z_ANGLE] = object->wo_ObjectDirection[Z_ANGLE];

  speed = object->wo_ObjectVelocity;

  return speed;
}

/**************************************************************************
 * IsFrontFace : returns a yes if this is aimed towards the viewer        *
 *     using the standard normal vector procedure.                        *
 **************************************************************************/
int32
IsFrontFace (Point sp[4])
{
  int32 a1, b1, a2, b2, z;

  a1 = sp[1].pt_X - sp[0].pt_X;
  b1 = sp[1].pt_Y - sp[0].pt_Y;

  a2 = sp[2].pt_X - sp[0].pt_X;
  b2 = sp[2].pt_Y - sp[0].pt_Y;

  z = a2 * b1 - a1 * b2;

  if (Face_rotation_flag == CCW_FACES)
    {
      if (z < 0)
        return (0);
      else
        return (1);
    }
  else
    {
      if (z < 0)
        return (1);
      else
        return (0);
    }
}

/**************************************************************************
 * LightFace : Sets the illumination of a given face from a value of      *
 *     0 to 15                                                            *
 **************************************************************************/
int32
LightFace (WorldObject *object, int32 scale, int32 face_id, int32 brightness,
           int32 flags)
{
  struct GeoDefinition *geodef;
  struct ModelDefinition *mdef;
  ModelSurface *surfacetype;
  CCB *ccb;

  ENTER ("LightFace");

  if (flags & USE_MESH_FACE)
    mdef = (ModelDefinition *)object->wo_MeshDefinition;
  else
    mdef = (ModelDefinition *)object->wo_ModelDefinition;

  if (!mdef)
    return (-1);

  geodef = mdef->md_GeoDefs[scale];

  if (face_id >= geodef->gd_SurfaceCount)
    return (-1);
  if (scale >= mdef->md_NumberOfScales)
    return (-1);
  if (!geodef->gd_SurfaceType)
    return (-1);

  surfacetype = (ModelSurface *)(geodef->gd_SurfaceType);

  surfacetype += face_id;
  ccb = (CCB *)surfacetype->ms_Control;
  ccb->ccb_PIXC = PPMPs[brightness];

  EXIT ("LightFace");

  return (0);
}

/**************************************************************************
 * LightModel : basic illumination function. Illuminates an object from   *
 *     the location "lightsource".                                        *
 **************************************************************************
 * NOTE : Currenly this supports only a single, white lightsource and     *
 *     assumes flat surface quality (ie, non-metallic). So the brightest  *
 *     a face can become is equivalent to it's original colormap.         *
 *                                                                        *
 * WARNING : Inorder for this to work right, each face MUST have it's own *
 *     personal CCB. When calling SetObjectCCB() use the CLONE_CCB_CONTROL*
 *     flag.                                                              *
 **************************************************************************/
void
LightModel (WorldPort *world, WorldObject *object, int32 camera_id,
            int32 flags)
{
  int32 *surface_list;
  point3f16 *vertex_list;
  int32 model_scale;
  ModelDefinition *model_def;
  int32 i, j;
  point3f16 *vertex0, *vertex1, *vertex2;
  frac16 a1, b1, a2, b2, z;
  vec3f16 vect1, normal_addr;
  vec3f16 *tempvect;
  vec3f16 *normal;
  frac16 length_vect1, length_normal;
  frac16 cos_angle;
  GeoDefinition *geodef;
  ModelSurface *surfacetype;
  int32 number_of_surfaces;
  int32 point_id;
  int32 point_counter;
  frac16 dot;
  CCB *ccb;
  frac16 vect1_times_normal;
  WorldObject *cam;
  vec3f16 light_location;
  int32 light_id;
  int32 minglow; /* dimmest value for non-illuminated faces */
  IntMeshDefs *imdef;
  int32 faces_per_normal = 1;
  int32 mesh_model = FALSE;
  /*
     int32 face_id1=-1, face_id2=-1;
  */

  ENTER ("LightModel");

  vertex0 = NULL;
  normal = (vec3f16 *)&normal_addr[0];

  /*** only the first lightsource is used currently ***/

  if (!(object->wo_LightPrefs & LIGHT_0))
    return;

  light_id = 0;
  minglow = (object->wo_LightPrefs & 0x0000000f);

  cam = GetCameraAddress (world, camera_id);

  model_scale = object->wo_ScaleUsed;

  if (model_scale < 0)
    return;

  model_def = (struct ModelDefinition *)object->wo_ObjectDefinition;
  geodef = model_def->md_GeoDefs[model_scale];

  surface_list = (int32 *)geodef->gd_SurfaceList;
  surfacetype = (ModelSurface *)geodef->gd_SurfaceType;

  vertex_list = (struct point3f16 *)object->wo_TransformedData;

  number_of_surfaces = geodef->gd_SurfaceCount;

  MulVec3Mat33_F16 (light_location, world->wp_LightPosition[light_id],
                    cam->wo_ObjectMatrix);

  light_location[X_COORD] -= cam->wo_LocalPosition[X_COORD];
  light_location[Y_COORD] -= cam->wo_LocalPosition[Y_COORD];
  light_location[Z_COORD] -= cam->wo_LocalPosition[Z_COORD];

  faces_per_normal = 1; // for non-meshed objects

  /*** find which of the original surfaces cooresponds to the current
   *** mesh surface, and pick out the normal for that set. That is, for
   *** a single subdivision, MESH2, the first 4 faces use the first normal,
   *** the next four use the second normal, and so on. ***/

  if (model_def->md_Flags & MESH_MODEL)
    {
      mesh_model = TRUE;

      /*** get the number of subdivided surfaces created from the parent face
       * ***/

      faces_per_normal = (2 << (geodef->gd_Flags & GD_MESH_MASK) - 1);
      faces_per_normal
          *= faces_per_normal; /* square it for the total num surfaces */
    }
  else if (model_def->md_Flags & IMESH_MODEL)
    {
      mesh_model = TRUE;

      imdef = (IntMeshDefs *)object->wo_SourceData;
      faces_per_normal = imdef->im_WidthDiv * imdef->im_HeightDiv;
    }

  /*** now calculate the lighting for each face ***/

  for (i = 0; i < number_of_surfaces; i++)
    {
      /*** calculate the face normal if needed ***/

      // printf("object=%lx, geodef=%lx,
      // facenormals=%lx\n",object,geodef,geodef->gd_FaceNormals);

      if (geodef->gd_FaceNormals)
        {

          tempvect = (vec3f16 *)geodef->gd_FaceNormals + i / faces_per_normal;

          MulVec3Mat33_F16 (*normal, *tempvect, object->wo_ResultMatrix);

          /*** need to get the vertex0 for the dot product below ***/

          point_counter = i * 4;
          point_id = surface_list[point_counter];
          vertex0 = (struct point3f16 *)(vertex_list + point_id);

          /*** get two other points so we can check for backfaces, so we
           *** don't have to perform the normal calcs for them. ***/

          vertex1 = (struct point3f16 *)(vertex_list + point_id + 1);
          vertex2 = (struct point3f16 *)(vertex_list + point_id + 2);

          a1 = vertex1->x - vertex0->x;
          b1 = vertex1->y - vertex0->y;

          a2 = vertex2->x - vertex0->x;
          b2 = vertex2->y - vertex0->y;

          z = MulSF16 (a1, b2) - MulSF16 (a2, b1);
        }
      /*
      if((i==face_id1) || (i==face_id2))
      {
      printfF16("normal x :",(*normal)[X_COORD]);
      printfF16("normal y :",(*normal)[Y_COORD]);
      printfF16("normal z :",(*normal)[Z_COORD]);

      printfF16("vertex0->x :",vertex0->x);
      printfF16("vertex0->y :",vertex0->y);
      printfF16("vertex0->z :",vertex0->z);

      printfF16("vertex1->x :",vertex1->x);
      printfF16("vertex1->y :",vertex1->y);
      printfF16("vertex1->z :",vertex1->z);

      printfF16("vertex2->x :",vertex2->x);
      printfF16("vertex2->y :",vertex2->y);
      printfF16("vertex2->z :",vertex2->z);

      printfF16("vect1 x :",vect1[X_COORD]);
      printfF16("vect1 y :",vect1[Y_COORD]);
      printfF16("vect1 z :",vect1[Z_COORD]);

      printfF16("vect2 x :",vect2[X_COORD]);
      printfF16("vect2 y :",vect2[Y_COORD]);
      printfF16("vect2 z :",vect2[Z_COORD]);

      printf("\n");
      }
      */
      /*** find the angle between the normal and the lightsource ***/

      vect1[X_COORD] = (vertex0->x - light_location[X_COORD]);
      vect1[Y_COORD] = (vertex0->y - light_location[Y_COORD]);
      vect1[Z_COORD] = (vertex0->z - light_location[Z_COORD]);

      length_vect1 = VectorLength (vect1);
      length_normal = VectorLength (*normal);

      vect1_times_normal = MulSF16 (length_vect1, length_normal);

      NormalizeVector (vect1);
      NormalizeVector (*normal);

      dot = Dot3_F16 (vect1, *normal);

      cos_angle = dot;

      /*** modify the ccb of each face ***/

      ccb = (CCB *)surfacetype->ms_Control;

      if (object->wo_LightProc)
        {
          (*object->wo_LightProc) (world, object, light_location, ccb,
                                   cos_angle, normal);
        }
      else
        {
          cos_angle = cos_angle >> 12; /* scales it down to between 0 and 16 */

          if (Face_rotation_flag == CW_FACES)
            cos_angle *= -1;

          /*
          printf("face : %ld, cos_angle=%ld,
          minglow=%ld\n",i,cos_angle,minglow);
          */
          if (cos_angle > 15)
            cos_angle = 15;

          /*** here we set the light PIXC value all at once for all of the mesh
           *faces to
           *** scads of time. ***/

          if (mesh_model)
            {
              for (j = 0; j < faces_per_normal; j++)
                {
                  ccb = (CCB *)surfacetype->ms_Control;

                  if (cos_angle >= minglow)
                    ccb->ccb_PIXC
                        = (PPMPs[cos_angle] | (PPMPs[cos_angle] << 16));
                  else
                    ccb->ccb_PIXC
                        = (PPMPs[minglow]
                           | (PPMPs[minglow]
                              << 16)); /* darkens the face automatically */

                  surfacetype++;
                  i++; /* egads! fiddling with the index counter! */
                }

              surfacetype--; /* counteracts the surfacetype++ below */
            }
          else
            {
              if (cos_angle >= minglow)
                ccb->ccb_PIXC = (PPMPs[cos_angle] | (PPMPs[cos_angle] << 16));
              else
                ccb->ccb_PIXC
                    = (PPMPs[minglow]
                       | (PPMPs[minglow]
                          << 16)); /* darkens the face automatically */
            }
        }

      surfacetype++;
    }

  EXIT ("LightModel");
}

/**************************************************************************
 * LoadModel :                                                            *
 **************************************************************************/
ModelDefinition *
LoadModel (char *file)
{
  int32 buffSize;
  int32 tempSize;
  uint32 chunk_ID;
  ModelDefinition *model;
  GeoDefinition *geo;
  LIST_CHUNK *model_chunk;
  int32 counter = 0;
  int32 i;
  int32 *surface;
  QUAD_DATA *quad_data;
  int32 len;
  GENERAL_INFO *gen_info;
  char *textname;
  int32 bad_data;
  int32 surfcount;
  /*
     int32 result;
     point3f16 *point;
     int32 *surface_list;
     point3f16 *vertex_list;
     int32 point_id;
     int32 j;
  */
  Stream *stream;

  ENTER ("LoadModel");

  quad_data = NULL;

  if ((buffSize = GetFileSize (file)) <= 0)
    return NULL;

  if ((model
       = (ModelDefinition *)ALLOCMEM (sizeof (ModelDefinition), MEMTYPE_DRAM))
      == NULL)
    return NULL;

  stream = OpenDiskStream (file, 1000L);

  if (!stream)
    {
      //      printf("unable to open stream!\n");
      return NULL;
    }

  /*	Init the vars for our chunk hunting while loop
   */

  tempSize = buffSize;

  model->md_NumberOfScales = 1;
  model->md_Flags = 0;

  geo = model->md_GeoDefs[0]
      = (GeoDefinition *)malloc (sizeof (GeoDefinition));

  geo->gd_RangeMin = 0;
  geo->gd_RangeMax = (410 << 16);
  geo->gd_SurfaceType = NULL;
  geo->gd_SortedList = NULL;
  geo->gd_FaceNormals = NULL;
  geo->gd_FaceCenters = NULL;
  geo->gd_FaceCentersXformed = NULL;

  /*
 Iterate through each chunk and set the CCB.
  */

  while ((model_chunk = (LIST_CHUNK *)GetModelChunk (stream, &chunk_ID))
         != NULL)
    {
      switch (chunk_ID)
        {
        case CHUNK_3DVL:
          geo->gd_PointCount = model_chunk->nElement;

          len = geo->gd_PointCount * 12;

          // printf("allocating chunk, %ld bytes\n",len);

          if ((geo->gd_PointList = (char *)ALLOCMEM (len, MEMTYPE_DRAM))
              == NULL)
            return NULL;

          memcpy ((char *)geo->gd_PointList, (char *)model_chunk->Data, len);
          break;

        case CHUNK_3DQL:
          geo->gd_SurfaceCount = model_chunk->nElement;
          quad_data = (QUAD_DATA *)model_chunk->Data;
          break;

        case CHUNK_3DTL:
          break;

        case CHUNK_3DNL:
          break;

        case CHUNK_3DGI:
          gen_info = (GENERAL_INFO *)model_chunk->Data;
          break;

        case CHUNK_MATR:
          textname = (char *)model_chunk->Data;
          //            printf("texture=%s\n",textname);
          break;

        default:
          /*
                                  printf("Chunk : type=%.4s/%lx, address=%lx
             was parsed but not used.\n", &chunk_ID,chunk_ID,model_chunk);
                      printf("tempSize=%ld\n",tempSize);
                      printf("texture=%s\n",textname);
          */
          break;
        }

      /*** checks for some bad files generated by the 3DO-Construction set ***/

      if (counter > 100)
        {
          //    printf("oops! file run on!\n");
          break;
        }

      counter++;
    }

  /*
  point=(point3f16 *)geo->gd_PointList;


  for(i=0;i<geo->gd_PointCount;i++)
  {
  printf("i=%ld, x=%lx, y=%lx, z=%lx\n",i,point->x,point->y,point->z);
  printfF16("x=",point->x);
  printfF16("y=",point->y);
  printfF16("z=",point->z);

     point++;
  }

  getout(0);
  */

  geo->gd_SurfaceList
      = (void *)ALLOCMEM (geo->gd_SurfaceCount * 16, MEMTYPE_DRAM);
  surface = (int32 *)geo->gd_SurfaceList;

  //   printf("surface count=%ld\n",geo->gd_SurfaceCount);

  surfcount = geo->gd_SurfaceCount;

  for (i = 0; i < surfcount; i++)
    {
      bad_data = 0;

      if (quad_data->ul > 1000000)
        bad_data = 1;
      if (quad_data->ll > 1000000)
        bad_data = 2;
      if (quad_data->lr > 1000000)
        bad_data = 3;
      if (quad_data->ur > 1000000)
        bad_data = 4;

      if (!bad_data)
        {
          *surface = quad_data->ul;
          surface++;
          *surface = quad_data->ll;
          surface++;
          *surface = quad_data->lr;
          surface++;
          *surface = quad_data->ur;
          surface++;
        }
      else
        {
          geo->gd_SurfaceCount--;
          /*
                   printf("corrupted data?? skipping face %ld\n",i);
                   printf("ul=%ld, ur=%ld, lr=%ld,
             ll=%ld\n",quad_data->ul,quad_data->ur,quad_data->lr,quad_data->ll);
          */
        }

      quad_data++;
    }

#ifdef MIKE
  printf ("surfacecount=%ld, i=%ld\n", geo->gd_SurfaceCount, i);

  surface_list = (int32 *)geo->gd_SurfaceList;
  vertex_list = (struct point3f16 *)geo->gd_PointList;

  for (i = 0; i < geo->gd_SurfaceCount; i++)
    {
      printf ("face %ld\n", i);
      for (j = 0; j < 4; j++)
        {
          /*** collect together all 4 quad points ***/

          point_id = surface_list[(i << 2) + j];
          printf ("point_id=%ld\n", point_id);
          point = (struct point3f16 *)(vertex_list + point_id);
          printf ("j=%ld, x=%lx, y=%lx, z=%lx\n", j, point->x, point->y,
                  point->z);
        }
    }
#endif

  CloseDiskStream (stream);

  EXIT ("LoadModel");

  return (model);
}

/**************************************************************************
 *  LookAtMatrix  :  Creates a matrix with just two points, a             *
 *  camera point, and a target point (the point that the camera is        *
 *  looking at.  Along with a roll 256.0 degrees in a unit circle where   *
 *  0 is normal.                                                          *
 **************************************************************************/
Err
LookAtMatrix (mat33f16 matrix, vec3f16 camera, vec3f16 target, frac16 roll)
{
  vec3f16 up;
  int32 error;

  ENTER ("LookAtMatrix");

  error = 0;

  up[X_COORD] = camera[X_COORD];
  up[Y_COORD] = camera[Y_COORD] + 0x10000;
  up[Z_COORD] = camera[Z_COORD];

  /*** This deals with the situation when the camera is looking at an object
   *** directly "Above" it and Roll is undefined. ***/

  if ((target[X_COORD] == up[X_COORD]) && (target[Z_COORD] == up[Z_COORD]))
    {
      up[Z_COORD] += 0x10000;
      up[Y_COORD] = camera[Y_COORD];
      error = -1;
    }

  BuildLookAtMatrix (matrix, camera, target, up);
  RotateOnZ (roll, matrix);

  EXIT ("LookAtMatrix");

  return error;
}

/**************************************************************************
 * NormalizeMatrix:  Makes sure that your matrix is still Square          *
 **************************************************************************/
int32
NormalizeMatrix (mat33f16 mat)
{
  vec3f16 target, up;
  vec3f16 null = { 0, 0, 0 };
  int i;
  int32 Err = 0;

#ifdef ERROR_CHECK
  frac16 ztest1 = 0, ztest2 = 0, ztest3 = 0;
#endif

  for (i = 0; i < 3; i++)
    {
      target[i] = -mat[i][2];
      up[i] = mat[i][1];
#ifdef ERROR_CHECK
      ztest1 |= up[i];
      ztest2 |= target[i];
      ztest3 |= (target[i] - up[i]);
#endif
    }

#ifdef ERROR_CHECK
  if ((ztest1 == 0) || (ztest2 == 0) || (ztest3 == 0))
    {
      if (ztest1 == 0)
        target[Z_COORD] = 0x10000;
      if (ztest2 == 0)
        up[Y_COORD] = 0x10000;
      if (ztest3 == 0)
        up[Y_COORD] += 0x1000;
      Err = -1;
    }
#endif

  BuildLookAtMatrix (mat, target, null, up);
  return Err;
}

/**************************************************************************
 * OffsetModel : adds point offsets to an object's point description      *
 **************************************************************************/
void
OffsetModel (frac16 x, frac16 y, frac16 z, GeoDefinition *gd)
{
  int32 *points;
  int32 k;

  ENTER ("OffsetModel");

  points = (int32 *)gd->gd_PointList;

  for (k = 0; k < gd->gd_PointCount; k++)
    {
      *(points + k * 12) -= x;
      *(points + k * 12 + 4) -= y;
      *(points + k * 12 + 8) -= z;
    }

  EXIT ("OffsetModel");
}

/**************************************************************************
 * PreProcessCallbacks : calls all of the callbacks from                  *
 *     wo_PreProcessObject field.                                         *
 **************************************************************************/
void
PreProcessCallbacks (WorldPort *world, Obj3D camera_id)
{
  WorldObject *obj;
  int32 i;
  int32 (*fnptr) ();

  ENTER ("PreProcessCallbacks");

  /*** render from farthest to nearest ***/

  for (i = world->wp_ObjectCount[camera_id] - 1; i >= 0; i--)
    {
      obj = (WorldObject *)world
                ->wp_UserObjects[(world->wp_MaxObjects) * (camera_id + 2) + i];

      fnptr = obj->wo_PreProcessObject;

      if (fnptr)
        (*fnptr) (world, obj);
    }

  EXIT ("PreProcessCallbacks");
}

/**************************************************************************
 * QuadInView : returns a YES is a quad is in view of the screen          *
 **************************************************************************
 * NOTE : This is used in the case when all 4 vertices of a quad are      *
 * off the edge of the screen, but the face is still visible, as in the   *
 * case when the eyepoint is really, really close and only the center     *
 * of the face is in the view.                                            *
 **************************************************************************/
bool
QuadInView (Render3DPort *rport, Point quad[4])
{
  int32 i;
  int32 maxx, minx, maxy, miny;
  Point sq[4];
  int32 x, y;

  ENTER ("QuadInView");

  maxx = -1000;
  minx = 1000;
  maxy = -1000;
  miny = 1000;

  /*** get the extents of the face, and to a point-in-rect test. ***/

  for (i = 0; i < 4; i++)
    {
      x = quad[i].pt_X;
      y = quad[i].pt_Y;

      if (x > maxx)
        maxx = x;
      if (x < minx)
        minx = x;

      if (y > maxy)
        maxy = y;
      if (y < miny)
        miny = y;
    }

  SetQuad (sq, 0, 0, rport->rp_Width, rport->rp_Height);

  for (i = 0; i < 4; i++)
    {
      x = sq[i].pt_X;
      y = sq[i].pt_Y;

      if ((x >= minx) && (x <= maxx))
        {
          if ((y >= miny) && (y <= maxy))
            {
              return (TRUE);
            }
        }
    }

  EXIT ("QuadInView");

  return (FALSE);
}

/**************************************************************************
 * ResortLinks : readjusts the visible object list when an object is      *
 *     added or deleted.                                                  *
 **************************************************************************/
void
ResortLinks (WorldPort *world)
{
  int32 index;
  int32 total_objects;
  WorldObject *object;

  ENTER ("ResortLinks");

  if (world->wp_TotalObjectCount == 0)
    return; /* no objects to sort */

  ClearUpdateFlags (world, 0x7fffffff);

  /*** new total of objects in list, temp4 is set in AddObject2List(), and
   *** represents the total models in the model list. Yup, this is really
   *** sloppy, but there must be a reason why Dan did it this way. ***/

  total_objects = world->wp_Temp4 + 1;
  world->wp_Temp4 = 0; /* Clear the storing index */

  index = 0;

  while (index < world->wp_HighestObjectIndex)
    {
      object = (WorldObject *)world->wp_UserObjects[index];

      if (!((object->wo_ObjectFlags) & (CAMERATYPE | SOUNDTYPE)))
        {
          AddObject2List (world, index);
        }

      index++;
    }

  EXIT ("ResortLinks");
}

/**************************************************************************
 * RotateModel : Front-end to MakeRotationMatrix()                        *
 **************************************************************************/
void
RotateModel (WorldObject *object, frac16 x, frac16 y, frac16 z)
{
  ENTER ("RotateModel");

  MakeRotationMatrix (x, y, z, object->wo_ObjectMatrix,
                      (object->wo_MatrixMethod & 0x000f));

  EXIT ("RotateModel");
}

/**************************************************************************
 * SetCallBacks : front-end to setting the callbacks, needed in the case  *
 *      the structure changes.                                            *
 **************************************************************************/
void
SetCallBacks (WorldPort *wport, WorldObject *object, int32 selection,
              int32 (*fn) ())
{
  ENTER ("SetCallBacks");

  switch (selection)
    {
    case PREPROCESSPROC:
      object->wo_PreProcessObject = fn;
      break;

    case RENDERTIMEPROC:
      object->wo_RenderTimeProc = fn;
      break;

    case LIGHTPROC:
      object->wo_LightProc = fn;
      break;

    case MAPPINGPROC:
      object->wo_MappingProc = fn;
      break;

    case OBJECTCULLINGPROC:
      object->wo_ObjectCullingProc = fn;
      break;

    case FACECULLINGPROC:
      object->wo_FaceCullingProc = fn;
      break;

    case SIMPLESOUNDPROC:
      object->wo_SimpleSoundProc = fn;
      break;

    case SCALEPROC:
      object->wo_ScaleProc = fn;
      break;

    case PERSPPROC:
      object->wo_PerspProc = fn;
      break;

    case SORTPROC:
      wport->wp_SortProc = fn;
      break;

    case COLLISIONPROC:
      wport->wp_CollisionProc = fn;
      break;
    }

  EXIT ("SetCallBacks");
}

/**************************************************************************
 * SetFaceCCB : inits CCB to a single face                                *
 **************************************************************************
 * NOTE : Scale is the detail level of the object, 0 will probably be     *
 *     the most detailed object. Face is the face number. This requires   *
 *     close knowledge of the data format which will probably be more     *
 *     difficult to aquire when the EA tool comes out. But in that case   *
 *     the tool will probably set the individual CCBs internally.         *
 **************************************************************************/
Err
SetFaceCCB (WorldObject *object, int32 scale, int32 face_id, CCB *ccb,
            int32 flags)
{
  struct GeoDefinition *geodef;
  struct ModelDefinition *mdef;
  ModelSurface *surfacetype;
  int32 len;

  ENTER ("SetFaceCCB");

  if (flags & USE_MESH_FACE)
    mdef = (ModelDefinition *)object->wo_MeshDefinition;
  else
    mdef = (ModelDefinition *)object->wo_ModelDefinition;

  if (!mdef)
    return (-1);

  geodef = mdef->md_GeoDefs[scale];

  if (face_id >= geodef->gd_SurfaceCount)
    return (-1);
  if (scale >= mdef->md_NumberOfScales)
    return (-1);
  if (!geodef->gd_SurfaceType)
    return (-1);

  surfacetype = (ModelSurface *)(geodef->gd_SurfaceType);

  surfacetype += face_id;
  surfacetype->ms_Flags = 0;

  len = sizeof (CCB);

  surfacetype->ms_Control = (int32)ccb;

  if (flags & SHARE_CCB)
    surfacetype->ms_Control = (int32)ccb;
  else if (flags & CLONE_CCB_CONTROL)
    surfacetype->ms_Control = (int32)MakeNewDupCCB (ccb);
  else if (flags & COPY_CCB_CONTROL)
    memcpy ((char *)surfacetype->ms_Control, (char *)ccb, len);

  EXIT ("SetFaceCCB");

  return (0);
}

/**************************************************************************
 * SetFacePIXC : Sets the PIXC value of a single face, similar            *
 *     to SetModelPIXC().                                                 *
 **************************************************************************/
void
SetFacePIXC (WorldObject *object, int32 scale, int32 face_id, int32 pflags,
             int32 flags)
{
  CCB *ccb;

  ENTER ("SetFacePIXC");

  ccb = GetFaceCCB (object, scale, face_id, flags);
  ccb->ccb_PIXC = pflags | (pflags << 16);

  EXIT ("SetFacePIXC");
}

/**************************************************************************
 * SetLightPosition : No brainer light positioning routine.               *
 **************************************************************************/
void
SetLightPosition (WorldPort *world, int32 lightid, frac16 x, frac16 y,
                  frac16 z)
{
  ENTER ("SetLightPosition");

  lightid = 0; // enforces only 1 light-source for now

  world->wp_LightPosition[lightid][X_COORD] = x;
  world->wp_LightPosition[lightid][Y_COORD] = y;
  world->wp_LightPosition[lightid][Z_COORD] = z;

  //   SetVec3f16(world->wp_LightPosition[lightid],x,y,z);

  EXIT ("SetLightPosition");
}

/**************************************************************************
 * SetModelCCB : inits CCB globally to an object                          *
 **************************************************************************/
void
SetModelCCB (WorldObject *model, CCB *ccb, int32 flags)
{
  struct GeoDefinition *geodef;
  ModelDefinition *mdef, *meshdef;
  ModelSurface *surfacetype;
  int32 i, j;
  int32 x, y;
  int32 len;
  CCB *oldccb;
  int32 seg_width, seg_height; // mesh cel stuff
  int32 h, w;
  IntMeshDefs *imesh_def;

  ENTER ("SetModelCCB");

  mdef = (ModelDefinition *)model->wo_ModelDefinition;

  len = sizeof (CCB);

  for (i = 0; i < mdef->md_NumberOfScales; i++)
    {
      geodef = mdef->md_GeoDefs[i];

      if (!geodef->gd_SurfaceType)
        {
          len = sizeof (ModelSurface) * geodef->gd_SurfaceCount;
          geodef->gd_SurfaceType = (void *)malloc (len);
        }

      surfacetype = (ModelSurface *)geodef->gd_SurfaceType;

      for (j = 0; j < geodef->gd_SurfaceCount; j++)
        {

          // printf("object=%lx,surfacetype=%lx,
          // j=%ld,ccb=%lx\n",object,surfacetype,j,ccb);

          surfacetype->ms_Flags = 0;

          if (flags & SHARE_CCB)
            surfacetype->ms_Control = (int32)ccb;
          else if (flags & CLONE_CCB_CONTROL)
            {
              surfacetype->ms_Control = (int32)MakeNewDupCCB (ccb);
              // printf("ms_Control=%lx\n",surfacetype->ms_Control);
            }
          else if (flags & COPY_CCB_CONTROL)
            {
              memcpy ((char *)surfacetype->ms_Control, (char *)ccb, len);
            }

          surfacetype++;
        }
    }

  /*** if this is a meshed object, take the supplied ccb and split it across
   *the meshed
   *** faces. ***/

  if (model->wo_ObjectFlags & MESH_OBJECT)
    {
      imesh_def = (IntMeshDefs *)model->wo_SourceData;

      meshdef = (ModelDefinition *)model->wo_MeshDefinition;
      surfacetype = (ModelSurface *)meshdef->md_GeoDefs[0]->gd_SurfaceType;

      seg_width = imesh_def->im_WidthDiv;
      seg_height = imesh_def->im_HeightDiv;

      x = 0;
      y = 0;

      w = ccb->ccb_Width / seg_width;
      h = ccb->ccb_Height / seg_height;

      for (i = 0; i < seg_height; y += h, i++)
        {
          for (j = 0; j < seg_width; x += w, j++)
            {
              oldccb = (CCB *)surfacetype->ms_Control;

              /*** free just the CCB, since the source data is shared by others
               * ***/

              FreeMem (oldccb, sizeof (CCB));

              surfacetype->ms_Control
                  = (int32)ClipCel ((CCB *)ccb, x, y, w, h);

              surfacetype++;
            }

          x = 0;
        }
    }

  EXIT ("SetModelCCB");
}

/**************************************************************************
 * SetModelPIXC : sets the PIXC (PPMPC) values of an entire model.        *
 **************************************************************************/
Err
SetModelPIXC (WorldObject *model, int32 pflags)
{
  struct GeoDefinition *geodef;
  ModelDefinition *mdef, *meshdef;
  ModelSurface *surfacetype;
  int32 i, j;
  CCB *ccb;
  IntMeshDefs *imesh_def;

  ENTER ("SetModelPIXC");

  /*** CHECK : Should the pflags arg be a taglist instead so I can handle some
   *CCB
   *** flags along with the PIXC?? ***/

  mdef = (ModelDefinition *)model->wo_ModelDefinition;

  for (i = 0; i < mdef->md_NumberOfScales; i++)
    {
      geodef = mdef->md_GeoDefs[i];

      if (!geodef->gd_SurfaceType)
        {
          return (-1);
        }

      surfacetype = (ModelSurface *)geodef->gd_SurfaceType;

      for (j = 0; j < geodef->gd_SurfaceCount; j++)
        {
          surfacetype->ms_Flags = 0;

          ccb = (CCB *)surfacetype->ms_Control;
          ccb->ccb_PIXC = pflags | (pflags << 16);

          surfacetype++;
        }
    }

  if (model->wo_ObjectFlags & MESH_OBJECT)
    {
      imesh_def = (IntMeshDefs *)model->wo_SourceData;
      meshdef = (ModelDefinition *)model->wo_MeshDefinition;
      surfacetype = (ModelSurface *)meshdef->md_GeoDefs[0]->gd_SurfaceType;

      for (i = 0; i < imesh_def->im_HeightDiv; i++)
        {
          for (j = 0; j < imesh_def->im_WidthDiv; j++)
            {
              ccb = (CCB *)surfacetype->ms_Control;
              ccb->ccb_PIXC = pflags | (pflags << 16);
              surfacetype++;
            }
        }
    }

  EXIT ("SetModelPIXC");

  return (0);
}

/**************************************************************************
 * SetObjectLocation : dumb interface to set an object's worldlocation    *
 **************************************************************************/
void
SetObjectLocation (WorldObject *object, frac16 x, frac16 y, frac16 z)
{
  ENTER ("SetObjectLocation");
  /*
     object->wo_WorldPosition[X_COORD]=x;
     object->wo_WorldPosition[Y_COORD]=y;
     object->wo_WorldPosition[Z_COORD]=z;
  */

  SetVec3f16 (object->wo_WorldPosition, x, y, z);

  EXIT ("SetObjectLocation");
}

/**************************************************************************
 * SetObjectOrientation : dumb interface to set an object's orientation,  *
 *     handy in the case we change the structure.                         *
 **************************************************************************/
void
SetObjectOrientation (WorldObject *object, frac16 x, frac16 y, frac16 z)
{
  ENTER ("SetObjectOrientation");
  /*
     object->wo_WorldOrientation[X_ANGLE]=x;
     object->wo_WorldOrientation[Y_ANGLE]=y;
     object->wo_WorldOrientation[Z_ANGLE]=z;
  */

  SetVec3f16 (object->wo_WorldOrientation, x, y, z);

  EXIT ("SetObjectOrientation");
}

/**************************************************************************
 * SetObjectSpeed :                                                       *
 **************************************************************************/
Err
SetObjectSpeed (WorldObject *object, vec3f16 direction, frac16 speed)
{
  ENTER ("SetObjectSpeed");
  /*
     object->wo_ObjectDirection[X_ANGLE]=direction[X_ANGLE];
     object->wo_ObjectDirection[Y_ANGLE]=direction[Y_ANGLE];
     object->wo_ObjectDirection[Z_ANGLE]=direction[Z_ANGLE];
  */
  SetVec3f16 (object->wo_ObjectDirection, direction[X_ANGLE],
              direction[Y_ANGLE], direction[Z_ANGLE]);
  object->wo_ObjectVelocity = speed;

  NormalizeVector (object->wo_ObjectDirection);

  EXIT ("SetObjectSpeed");

  return 0;
}

extern int32 Eye;
extern int32 Render_screen, Left_screen, Right_screen;
extern int32 View_mode;

/**************************************************************************
 * SwapScreens : swaps and displays the desired screens. This is a no-    *
 *     brainer for double-buffered systems, but is really useful for      *
 *     systems using the stereo glasses as it must cycle through 3 buffers*
 **************************************************************************
 * NOTE : The value returned is the next screen to be used for rendering. *
 *     a -1 is returned if DisplayScreen() fails.                         *
 **************************************************************************/
int32
SwapScreens (ScreenContext sc)
{
  int32 rvalue;

  ENTER ("SwapScreens");

  if (View_mode == STEREO_VIEW)
    {
      if (Eye == LEFT_EYE)
        Left_screen = Render_screen;
      else
        Right_screen = Render_screen;

      rvalue = DisplayScreen (sc.sc_Screens[Left_screen],
                              sc.sc_Screens[Right_screen]);

      if (rvalue < 0)
        return (rvalue);

      /*** this trick gives me the index of the unused buffer ***/

      Render_screen = 3 - Left_screen - Right_screen;

      if (Eye == LEFT_EYE)
        Eye = RIGHT_EYE;
      else
        Eye = LEFT_EYE;
    }
  else
    {
      rvalue = DisplayScreen (sc.sc_Screens[Render_screen], 0);

      Render_screen = (Render_screen == 0) ? 1 : 0;

      if (rvalue < 0)
        return (rvalue);
    }

  EXIT ("SwapScreens");

  return (Render_screen);
}

/**************************************************************************
 * StretchModel :  Resizes a model                                        *
 **************************************************************************
 * NOTE : this WILL modify the original data. X_size... represents the    *
 *   scaling factor.                                                      *
 **************************************************************************/
void
StretchModel (WorldObject *object, frac16 x_size, frac16 y_size, frac16 z_size)
{
  GeoDefinition *geodef;
  ModelDefinition *model;
  point3f16 *vertex, *vertex_list;
  int32 i, j;

  ENTER ("StretchModel");

  model = (ModelDefinition *)object->wo_ModelDefinition;

  for (i = 0; i < model->md_NumberOfScales; i++)
    {
      geodef = (GeoDefinition *)model->md_GeoDefs[i];

      vertex_list = (struct point3f16 *)geodef->gd_PointList;

      for (j = 0; j < geodef->gd_PointCount; j++)
        {
          vertex = (struct point3f16 *)(vertex_list + j);

          vertex->x = MulSF16 (x_size, vertex->x);
          vertex->y = MulSF16 (y_size, vertex->y);
          vertex->z = MulSF16 (z_size, vertex->z);
        }
    }

  EXIT ("StretchModel");
}

/**************************************************************************
 * Toggle3DSound : toggles a simplesound on and off. Not a terribly       *
 *    complex routine, but handy.                                         *
 **************************************************************************/
void
Toggle3DSound (WorldObject *object, int32 flags)
{
  ENTER ("Toggle3DSound");

  if (flags & SOUND_ON)
    {
      object->wo_SoundDef->sd_Flags |= SOUND_ON;
      Start3DSound (((Sound3D *)object->wo_SourceData), NULL, NULL);
    }
  else
    {
      object->wo_SoundDef->sd_Flags &= (~SOUND_ON);
      Stop3DSound (((Sound3D *)object->wo_SourceData), NULL);
    }

  EXIT ("Toggle3DSound");
}

/**************************************************************************
 * ToggleSimpleSound : toggles a simplesound on and off. Not a terribly   *
 *    complex routine, but handy.                                         *
 **************************************************************************/
void
ToggleSimpleSound (WorldObject *object, int32 flags)
{
  ENTER ("ToggleSimpleSound");

  if (flags & SOUND_ON)
    {
      object->wo_SoundDef->sd_Flags |= SOUND_ON;
      StartInstrument (object->wo_SoundDef->sd_Sampler, NULL);
    }
  else
    {
      object->wo_SoundDef->sd_Flags &= (~SOUND_ON);
      StopInstrument (object->wo_SoundDef->sd_Sampler, NULL);
    }

  EXIT ("ToggleSimpleSound");
}

/**************************************************************************
 * WorldTick : updates the positions of all objects based on the supplied *
 *    time delta (timeval) and the objects velocity.                      *
 **************************************************************************/
void
WorldTick (WorldPort *world, uint32 timeval)
{
  uint32 i;
  frac16 dist, tempf16;
  WorldObject *object;

  ENTER ("WorldTick");

  for (i = 0; i < world->wp_HighestObjectIndex; i++)
    {
      object = (WorldObject *)world->wp_UserObjects[i];

      tempf16 = Convert32_F16 (timeval);

      dist = MulSF16 (object->wo_ObjectVelocity, tempf16);

      if (dist && object) /* if the object is moving ... */
        {
          object->wo_WorldPosition[X_COORD]
              += MulSF16 (object->wo_ObjectDirection[X_ANGLE], dist);
          object->wo_WorldPosition[Y_COORD]
              += MulSF16 (object->wo_ObjectDirection[Y_ANGLE], dist);
          object->wo_WorldPosition[Z_COORD]
              += MulSF16 (object->wo_ObjectDirection[Z_ANGLE], dist);
        }
    }

  EXIT ("WorldTick");
}

/**************************************************************************
 * WorldToScreen : returns the screen coordinates of the supplied 3d      *
 *     vertex.                                                            *
 **************************************************************************/
bool
WorldToScreen (Render3DPort *rport, vec3f16 vtx, WorldObject *cam,
               int32 mapping_form, int32 *vx, int32 *vy)
{
  int32 array[2];
  vec3f16 x_vtx;

  ENTER ("WorldToScreen");

  vtx[X_COORD] -= cam->wo_WorldPosition[X_COORD];
  vtx[Y_COORD] -= cam->wo_WorldPosition[Y_COORD];
  vtx[Z_COORD] -= cam->wo_WorldPosition[Z_COORD];

  MulVec3Mat33_F16 (x_vtx, vtx, cam->wo_ObjectMatrix);

  if (mapping_form == PSEUDO_PERSPECTIVE)
    {
      PseudoMapping (rport, NULL, cam, (int32 *)x_vtx, array, 1L);
    }
  else if (mapping_form == CAMERA_PERSPECTIVE)
    {
      CameraMapping (rport, NULL, cam, (int32 *)x_vtx, array, 1L);
    }

  *vx = array[0];
  *vy = array[1];

  if ((*vx >= 0 && *vx < rport->rp_Width)
      && (*vy > 0 && *vy < rport->rp_Height))
    return (TRUE);
  else
    return (FALSE);

  EXIT ("WorldToScreen");
}
