/**************************************************************************
 * process3d : contains the 3dlib sorting routines.                       *
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

#include "3dlib.h"
#include "Operamath.h"

#define ABS(x) (((x) > 0) ? (x) : -(x))

extern int32 Debug_flag;

/*** remove before flight ***/

void dump_object_list (WorldPort *world, Obj3D camera);
void getlocalpositionsC (WorldPort *world, Obj3D camera_id, WorldObject *cam);

/**************************************************************************
 * BubbleSortFaces : sorts the order of faces using the bubble-sort       *
 *      approach.                                                         *
 **************************************************************************/
int32
BubbleSortFaces (WorldPort *world, Obj3D camera_id)
{
  int32 flag;
  int32 count;
  WorldObject *obj;
  GeoDefinition *geodef;
  ModelDefinition *model;
  int32 face1, face2;
  int32 index, bottom_index, top_index;
  int32 object_index;
  int32 *facelist;
  int32 i;
  point3f16 *center1, *center2;
  point3f16 *centers; /* pointer to the centers list */

  ENTER ("BubbleSortFaces");

  flag = 0;

  object_index = (world->wp_MaxObjects) * (camera_id + 2);

  for (i = world->wp_ObjectCount[camera_id] - 1; i >= 0; i--)
    {
      // printf("bbbb, i=%ld\n",i);
      obj = (WorldObject *)world->wp_UserObjects[object_index + i];

      if (!(obj->wo_ObjectFlags & FACE_SORTING))
        continue;
      if (obj->wo_ScaleUsed < 0)
        continue;

      model = (ModelDefinition *)obj->wo_ObjectDefinition;

      geodef = model->md_GeoDefs[obj->wo_ScaleUsed];

      facelist = (int32 *)geodef->gd_SortedList;
      centers = (point3f16 *)geodef->gd_FaceCentersXformed;

      bottom_index = 0;
      top_index = geodef->gd_SurfaceCount - 1;
      count = geodef->gd_SurfaceCount;

      /*** now sort dem suckahs! ***/

      flag = 0;

      /*** sort from the top down first, followed by the bottom up ***/

      /*
      printf("aaaa\n");
      printf("object_index=%ld, count=%ld,
      flag=%ld\n",object_index,count,flag); printf("top_index=%ld,
      bottom_index=%ld\n",top_index,bottom_index); printf("geo=%lx,
      model=%lx,obj=%lx, facelist=%lx\n",geodef,model,obj,facelist);
      printf("geodef->gd_SurfaceCount=%ld\n",geodef->gd_SurfaceCount);
      printf("centers=%lx\n",centers);

      for(j=0;j<geodef->gd_SurfaceCount;j++)
      {
      face1= *(facelist+j);
      printf("%ld : face %ld\n",j,face1);
      }
      */
      while (count)
        {
          for (index = (top_index - 1); index >= bottom_index; index--)
            {
              face1 = *(facelist + index);
              face2 = *(facelist + index + 1);

              center1 = centers + face1;
              center2 = centers + face2;

              if (center1->z < center2->z)
                {
                  *(facelist + index) = face2;
                  *(facelist + index + 1) = face1;

                  flag++;
                }
            }

          bottom_index++;
          count--;

          if ((count == 0) || (flag == 0))
            break; /* Its already sorted. */
          flag = 0;

          for (index = bottom_index; index <= (top_index - 1); index++)
            {
              face1 = *(facelist + index);
              face2 = *(facelist + index + 1);

              center1 = centers + face1;
              center2 = centers + face2;

              if (center1->z < center2->z)
                {
                  *(facelist + index) = face2;
                  *(facelist + index + 1) = face1;

                  flag++;
                }
            }

          top_index--;
          count--;

          if (flag == 0)
            break; /* its already sorted */
        }
    }

  EXIT ("BubbleSortFaces");

  return flag;
}

/**************************************************************************
 * BubbleSortObjects : sorts the order of objects using the bubble-sort   *
 *      approach.                                                         *
 **************************************************************************/
int32
BubbleSortObjects (WorldPort *world, Obj3D camera)
{
  int32 count;
  int32 flag;
  WorldObject *obj1, *obj2;
  int32 index, bottom_index, top_index;
  int32 total_objects;

  ENTER ("BubbleSortObjects");

  total_objects = world->wp_ObjectCount[camera];
  count = total_objects - 1;

  bottom_index = (world->wp_MaxObjects) * (camera + 2) + (total_objects - 1);
  top_index = bottom_index - total_objects;
  flag = 0;

  while (count)
    {
      flag = 0;
      for (index = (bottom_index - 1); index >= top_index; index--)
        {
          obj1 = (WorldObject *)world->wp_UserObjects[index];
          obj2 = (WorldObject *)world->wp_UserObjects[index + 1];

          if (obj1->wo_LocalPosition[Z_COORD]
              > obj2->wo_LocalPosition[Z_COORD])
            {
              world->wp_UserObjects[index] = obj2;
              world->wp_UserObjects[index + 1] = obj1;

              flag++;
            }
        }
      top_index++;
      count--;

      if ((count == 0) || (flag == 0))
        break; /* Its already sorted. */
      flag = 0;

      for (index = top_index; index <= (bottom_index - 1); index++)
        {
          obj1 = (WorldObject *)world->wp_UserObjects[index];
          obj2 = (WorldObject *)world->wp_UserObjects[index + 1];

          if (obj1->wo_LocalPosition[Z_COORD]
              > obj2->wo_LocalPosition[Z_COORD])
            {
              world->wp_UserObjects[index] = obj2;
              world->wp_UserObjects[index + 1] = obj1;

              flag++;
            }
        }
      bottom_index--;
      count--;
      if (flag == 0)
        break; /* its already sorted */
    }

  EXIT ("BubbleSortObjects");

  return flag;
}

/**************************************************************************
 * BuildCameraList : get the cameras world matrix, and create a list in   *
 *     SortedObjects[Camera]                                              *
 **************************************************************************
 * NOTE : SortedObjects is a computed address.                            *
 **************************************************************************/
void
BuildCameraList (WorldPort *world, Obj3D camera_id)
{
  int32 i;
  frac16 x, y, z;
  WorldObject *o, *cam;
  int32 cam_vis_flag, dist_vis_flag;
  int32 index;
  int32 object_visible;

  ENTER ("BuildCameraList");

  object_visible = FALSE;

  /*** Get camera mask to see if objects are seen by this camera ***/

  cam_vis_flag = (0x00000001 << ((int32)camera_id));
  dist_vis_flag = cam_vis_flag << DIST_CULL_OFFSET;

  cam = GetCameraAddress (world, camera_id);

  world->wp_ObjectCount[camera_id]
      = 0; /* Set this cameras object count to 0 */

  /*** GetLocalPositions() actually performs the rotations on the center point,
   *** the WorldPosition, of each visible object and copies that over to the
   *wo_LocalPosition
   *** field. If an object is NOT visible to the camera, it's distance is not
   *calculated.
   *** If it is visible, then we check to see if it is culled-by-distance and
   *if so,
   *** it is excluded from the list. ***/

  GetLocalPositions (world, camera_id, cam);

  for (i = (world->wp_TotalSortedObjects - 1); i >= 0; --i)
    {

      o = (WorldObject *)world->wp_UserObjects[world->wp_MaxObjects + i];

      // printf("o=%lx, i=%ld, index=%ld\n",o,i,(world->wp_MaxObjects+i));

      object_visible = (o->wo_CameraVisibility & cam_vis_flag);

      if (!object_visible)
        continue;

      if (o->wo_CameraVisibility & dist_vis_flag)
        {
          object_visible = FALSE;

          if (o->wo_ObjectCullingProc)
            {
              object_visible = (*o->wo_ObjectCullingProc) (world, o);
            }
          else
            {
              x = o->wo_LocalPosition[X_COORD];
              y = o->wo_LocalPosition[Y_COORD];
              z = o->wo_LocalPosition[Z_COORD];

              if (x < 0)
                x = -x;
              if (y < 0)
                y = -y;

              /*** see if an object is visible in the view cone. Then check for
               *the
               *** face culling which supercedes view-cone visibility. If
               *FACE_CULLING
               *** is set, then automatically stuff the object into the camera
               *list,
               *** then handle the culling in DrawModel()/render_3d.c ***/

              if (o->wo_ObjectFlags & FACE_CULLING)
                object_visible = TRUE;
              else
                {
                  if ((z > o->wo_SphericalRadius) && /* Check View CONE */
                      ((x - o->wo_SphericalRadius) < z)
                      && ((y - o->wo_SphericalRadius) < z))
                    object_visible = TRUE;
                }
            }
          /*	This routine is not in the same place before Dan twiddled with
           * it!!!  */
          /*
          printf("bb : object-visible=%ld, object=%lx\n",object_visible,o);
          printfF16("x=",x);
          printfF16("y=",y);
          printfF16("z=",z);
          printfF16("radius=",o->wo_SphericalRadius);
          */
          if (object_visible)
            {
              index = (world->wp_MaxObjects) * (camera_id + 2)
                      + world->wp_ObjectCount[camera_id];

              world->wp_UserObjects[index] = o;
              world->wp_ObjectCount[camera_id]++;
            }
          else
            {

              /* put any debugging code here if you want to find out why some
               * objects are being culled out. ***/
            }
        }
    }

  EXIT ("BuildCameraList");
}

/**************************************************************************
 * CullByDistance : globally culls objects by their distance  from the    *
 *    supplied camera. This can be used if you really know your world.    *
 *    So if you know that you are really close to a big object which      *
 *    will obscure a bunch of other objects behind this, turn 'em off     *
 *    using this routine. Also handy for maze situations. You are in      *
 *    a hallway of known dimensions, you can call all of the other rooms  *
 *    and hallways that are further than the furthest visible wall.       *
 *                                                                        *
 *    This uses the Z distance only, for speed. So the culling will fail  *
 *    for objects with tiny Z values, but then the cone-culling can kick  *
 *    in and take care of that.                                           *
 **************************************************************************/
void
CullByDistance (WorldPort *world, Obj3D camera_id, frac16 z)
{
  int32 camera_flag; // camera's visibility mask
  int32 i;
  int32 index;
  WorldObject *cam;
  WorldObject *obj;

  ENTER ("CullByDistance");

  cam = GetCameraAddress (world, camera_id);
  index = world->wp_MaxObjects;
  camera_flag = 1 << (int32)(camera_id + DIST_CULL_OFFSET);

  for (i = world->wp_TotalSortedObjects; i >= 0; i--)
    {
      obj = (WorldObject *)world->wp_UserObjects[index + i];

      if (!obj)
        continue; // traps the first time this is called, before the
                  // object list is built.

      if (obj->wo_LocalPosition[Z_COORD] > z)
        obj->wo_CameraVisibility &= (~camera_flag);
      else
        obj->wo_CameraVisibility |= camera_flag;

      // printf("i=%ld, obj=%lx, pos=%s,
      // cam_vis=%lx\n",i,obj,Frac16str(obj->wo_LocalPosition[Z_COORD]),obj->wo_CameraVisibility);
    }

  EXIT ("CullByDistance");
}

/**************************************************************************
 * FaceCulling1Proc : performs FACE_CULLING1. Returns a TRUE if all       *
 *    vertices are visible in the view cone. This is the "best" most      *
 *    predictable form of culling, except it will make faces vanish       *
 *    sooner than you might want them to be. This is good for small faces *
 *    and a narrow field-of-view.                                         *
 **************************************************************************
 * NOTE : the viewcone is currently assumed to be 90 degrees, 45 degrees  *
 *    on each side.                                                       *
 **************************************************************************/
int32
FaceCulling1Proc (WorldPort *world, WorldObject *object, GeoDefinition *geo,
                  int32 face_id, point3f16 *vertices[4])
{
  int32 i;
  frac16 x, y, z;

  ENTER ("FaceCulling1Proc");

  // printf("faceculling 1 proc\n");
  for (i = 0; i < 4; i++)
    {
      x = vertices[i]->x;
      y = vertices[i]->y;
      z = vertices[i]->z;

      if (x < 0)
        x = -x;
      if (y < 0)
        x = -y;

      /*** CHECK : Does this compile to faster code, then doing "if(A||B||C). .
       * ." ??? ***/

      if (z < 0)
        return (FALSE);
      if (x > z)
        return (FALSE);
      if (y > z)
        return (FALSE);
    }

  EXIT ("FaceCulling1Proc");

  return (TRUE);
}

/**************************************************************************
 * FaceCulling2Proc : performs FACE_CULLING2. Returns a TRUE if only one  *
 *    vertex is visible, a FALSE if all vertices are out of the viewing   *
 *    cone. This can only be used with CAMERA_PERSPECTIVE.                *
 **************************************************************************
 * NOTE : the viewcone is currently assumed to be 90 degrees, 45 degrees  *
 *    on each side.                                                       *
 **************************************************************************/
int32
FaceCulling2Proc (WorldPort *world, WorldObject *object, GeoDefinition *geo,
                  int32 face_id, point3f16 *vertices[4])
{
  int32 i;
  frac16 x, y, z;

  ENTER ("FaceCulling2Proc");

  // printf("faceculling 2 proc\n");
  for (i = 0; i < 4; i++)
    {
      x = vertices[i]->x;
      y = vertices[i]->y;
      z = vertices[i]->z;

      if (x < 0)
        x = -x;
      if (y < 0)
        x = -y;

      /*** CHECK : Does this compile to faster code, then doing "if(A&&B&&C). .
       * ." ??? ***/

      if (z > 0)
        if (x < z)
          if (y < z)
            return (TRUE);
    }

  EXIT ("FaceCulling2Proc");

  return (FALSE);
}

/**************************************************************************
 * FaceCulling3Proc : performs FACE_CULLING3. Returns a TRUE if the       *
 *    face's center point is in the viewing cone.                         *
 **************************************************************************/
int32
FaceCulling3Proc (WorldPort *world, WorldObject *object, GeoDefinition *geo,
                  int32 face_id, point3f16 *vertices[4])
{
  point3f16 *center;

  ENTER ("FaceCulling3Proc");

  center = (point3f16 *)geo->gd_FaceCentersXformed;

  center += face_id;

  /*** CHECK : Does this compile to faster code, then doing "if(A||B||C). . ."
   * ??? ***/
  /*
  printfF16("center->x=",center->x);
  printfF16("center->y=",center->y);
  printfF16("center->z=",center->z);
  */
  if (center->z < 0)
    return (FALSE);
  if (center->x > center->z)
    return (FALSE);
  if (center->y > center->z)
    return (FALSE);

  EXIT ("FaceCulling3Proc");

  return (TRUE);
}
/**************************************************************************
 * ResolveObjectMatrix : creates the matrix for an object based upon      *
 * its MatrixMethod.  It also alters UpdateFlags based upon the           *
 * CameraMask so that it doesn't have to create a matrix twice.           *
 * The object in this case will most likely be a camera object            *
 **************************************************************************/
void
ResolveObjectMatrix (WorldPort *world, WorldObject *object)
{
  int32 method;
  mat33f16 incmatrix;

  ENTER ("ResolveObjectMatrix");

  method = object->wo_MatrixMethod & 0x000f;

  /*** Already got a matrix?? ***/

  if (object->wo_UpdateFlags && world->wp_CameraMask)
    return;

  object->wo_UpdateFlags |= world->wp_CameraMask; /* Mark matrix as correct */

  if (method == NO_ROTATE)
    {
      return;
    }

  if (method < 5)
    {
      MakeCRotationMatrix (object->wo_WorldOrientation,
                           object->wo_ObjectMatrix, method);
      return;
    }
  else if (method == VECTOR_ROTATE)
    {
      BuildLookAtMatrix (object->wo_ObjectMatrix, object->wo_WorldPosition,
                         object->wo_ObjectNormal, object->wo_ObjectNormalUp);
    }
  else if (method == FLYTHROUGH_ROTATE)
    {
      BuildYXZ (object->wo_WorldOrientation[PITCH],
                object->wo_WorldOrientation[YAW],
                object->wo_WorldOrientation[ROLL], incmatrix);

      MulMat33Mat33_F16 (object->wo_ObjectMatrix, object->wo_ObjectMatrix,
                         incmatrix);
    }

  EXIT ("ResolveObjectMatrix");
}

/**************************************************************************
 * SortCameraList : sorts the order of objects based on the selected      *
 *      sort method.                                                      *
 **************************************************************************/
void
SortCameraList (WorldPort *world, Obj3D camera_id)
{
  WorldObject *cam;
  int32 method, c;

  ENTER ("SortCameraList");

  if (world->wp_SortProc)
    {
      (*world->wp_SortProc) (world, camera_id);
      return;
    }

  cam = GetCameraAddress (world, camera_id);

  c = world->wp_ObjectCount[camera_id];
  method = (int32)(cam->wo_MatrixMethod)
           & 0x00f0; /* select only the method bits */

  if (method == DONT_SORT)
    return;

  if (method == BUBBLE_SORT)
    {
      if (c >= 2)
        BubbleSortObjects (world, camera_id);
      BubbleSortFaces (world, camera_id);
    }
  else if (method == SUMMATION_SORT)
    {
      if (c >= 2)
        SummationSort (world, camera_id, c);
    }

  EXIT ("SortCameraList");
}

/**************************************************************************
 * SummationSort : sorts the order of objects using the summation-sort    *
 *      approach.                                                         *
 **************************************************************************/
void
SummationSort (WorldPort *world, Obj3D camera, int32 count)
{
  int32 iterations, nc;
  WorldObject *obj1, *obj2;
  int32 index;

  ENTER ("SummationSort");

  iterations = (Sqrt32 (count * 8 + 1) - 1) / 2;
  if (((1 + iterations) * iterations) / 2 < count)
    iterations++;

  index = world->wp_MaxObjects * (camera + 2);

  if (iterations < 4)
    {
      BubbleSortObjects (world, camera);
      printf ("BubbleSort %lx, %lx\n", iterations, count);
      return;
    }

  // printf("Summation Sort Iterations: 0x%lx,   Count
  // %lx\n",iterations,count);

  while (iterations--)
    {
      nc = count - iterations;
      while (nc--)
        {
          obj1 = (WorldObject *)world->wp_UserObjects[index + nc];
          obj2 = (WorldObject *)world->wp_UserObjects[index + iterations + nc];

          if (obj1->wo_LocalPosition[Z_COORD]
              > obj2->wo_LocalPosition[Z_COORD])
            {
              world->wp_UserObjects[index + nc] = obj2;
              world->wp_UserObjects[index + iterations + nc] = obj1;
            }
        }
    }

  EXIT ("SummationSort");
}
