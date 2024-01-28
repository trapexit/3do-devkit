/****************************************************************
 *                                                              *
 * Include file for 3D lib                                      *
 *                                                              *
 * By:  Dan Duncalf	and Mike Smithwick                          *
 *                                                              *
 * Copyright (c) 1993,The 3DO Company                           *
 * All rights reserved                                          *
 * This document is proprietary and confidential                *
 *                                                              *
 ****************************************************************/

#pragma force_top_level
#pragma include_only_once

#ifndef __3lib_h
#define __3lib_h

#include "Init3DO.h"
#include "Parse3DO.h"
#include "Portfolio.h"

#include "mem.h" /* needed for the malloc() prototype */
#include "sound3d.h"
#include "stdio.h"
#include "userdefs3D.h"

#define COPYRIGHT_NOTICE                                                      \
  char *copyright[] = "Copyright 1993 The 3DO Company\nAll Rights Reserved"

#define YES 1
#define NO 0

/*** error codes ***/

/*** MAX3DCAMERAS and MAX3DSOUNDS are here and should be handled with care,
 *** since 3dlib might end up as system code one of these days, in which case
 *** the internal array sizes will be frozen and could conflict with
 *** application sizes. MAXSIMPLESOUNDS must be limited to 4 for the time
 *** being, due to mixer limitations. ***/

#define MAX3DCAMERAS 8
#define MAXSIMPLESOUNDS 4
#define MAX3DSOUNDS 4
#define MAXLIGHTS 4
#define MAXNUMGEODEFS 8

/*** Debug stuff, so isolate which routine the program crashes at. Make sure
 *that
 *** Debug_flag is declared. Make sure to declare Debug_flag in your own
 *code.*/

#define DBUG_FLAG

#ifdef DBUG_FLAG
#define ENTER(x)                                                              \
  if (Debug_flag)                                                             \
  printf ("entering %s\n", x)
#define EXIT(x)                                                               \
  if (Debug_flag)                                                             \
  printf ("exiting %s\n", x)

extern int32 Debug_flag;

#else
#define ENTER(X)
#define EXIT(x)
#endif

/*** The error-checking may be turned off for a minor speed enhancement. ***/

#define ERROR_CHECK 1

#define MIXER "mixer8x2.dsp"
#define SAMPLER "sampler.dsp"

/*** rotate methods ***/

#define ROTATE_XYZ 0
#define ROTATE_XZY 1
#define ROTATE_YXZ 2
#define ROTATE_YZX 3
#define ROTATE_ZXY 4
#define ROTATE_ZYX 5
#define NO_ROTATE 6         /* Don't rotate, used in ResolveObjectMatrix() */
#define VECTOR_ROTATE 7     /* calcs matrix based on object normals */
#define FLYTHROUGH_ROTATE 8 /* creates an incremental matrix used for */
                            /* for flying simulations. */

/*** used when accessing xyz points from a vector
 *** array (ie, wo_WorldPosition) ***/

#define X_COORD 0
#define Y_COORD 1
#define Z_COORD 2

/*** used when accessing xyz angles from a vector
 *** array (ie, wo_WorldOrientation) ***/

#define X_ANGLE 0
#define Y_ANGLE 1
#define Z_ANGLE 2

#define PITCH X_ANGLE
#define YAW Y_ANGLE
#define ROLL Z_ANGLE

/*** mesh related stuff ***/

#define MESH2 1
#define MESH4 2
#define MESH8 3
#define MESH16 4

#define MESH_GEOMETRY 1
#define NON_MESH_GEOMETRY 2

/*** face rotation : CCW is the mathematically "correct" way of doing things,
 *but
 *** due to the HW the images end up backwards. This is what was done for
 *release 1.0.
 *** For 1.1, the default is the old way, but the user can opt to go with CW
 *faces
 *** if it would not be too costly for them to convert all of their files. ***/

#define CW_FACES 1
#define CCW_FACES 2

/*** sortmethods as used in SortCameraList()/process3d.c ***/

#define DONT_SORT 0x00000010
#define BUBBLE_SORT 0x00000020
#define SUMMATION_SORT 0x00000040

/*** flags for the Destroy functions ***/

#define FREE_ENTIRE_OBJECT                                                    \
  0x00000001 /* frees all including the root pointer */
#define FREE_ENTIRE_WORLD FREE_ENTIRE_OBJECT

/*** flags for CloneObject() ***/

#define CLONE_GEOMETRY 0x00000001
#define SHARE_GEOMETRY 0x00000000

/*** some standard CEL flag settings (ppmpc values) ***/

#define SOLID_CEL 0x1f001f00
#define TRANSLUCENT_CEL 0x1f811f81
#define BLAZEMONGER_CEL 0xffffffff

/*** ModelDefinitionFlags ***/

#define MESH_MODEL 0x00000001
#define IMESH_MODEL 0x00000002

/*** GeoDefinitionFlags ***/

#define GD_MESH_MASK 0x00000007 /* masks only the mesh level counts */

/***************************************************************/
/*** object flags go here (used by the wo_ObjectFlags field) ***/
/***************************************************************/

#define CULL_BACKFACE 0x00000001
#define WIREFRAME 0x00000002
#define CAMERA_PERSPECTIVE 0x00000004
#define PSEUDO_PERSPECTIVE 0x00000008

/*** FACE_CULLING1 is best for the PSEUDO_PERSPECTIVE, and is the most
 *** general case. FACE_CULLING2 will still let part of a face show,
 *** but will not work well with the PSEUDO_CAMERA perspective unless you
 *** can garauntee that no part of the face will cross into -z space,
 *** otherwise weirdness will result. FC3 culls faces if the face
 *** centerpoint is not in theview-cone, slighly faster than the other
 *** methods. ***/

/*** FACE_CULLING1 : cull a face if at least one vertex not visible
 *** FACE_CULLING2 : cull a face if all vertices are not visible
 *** FACE_CULLING3 : cull a face if the centerpoint is not visible */

#define FACE_CULLING1 0x00000010
#define FACE_CULLING2 0x00000020
#define FACE_CULLING3 0x00000040
#define FACE_CULLING (FACE_CULLING1 | FACE_CULLING2 | FACE_CULLING3)

#define MESH_FLAGS CAMERA_PERSPECTIVE | FACE_CULLING3

#define MESH_OBJECT 0x00000080

#define FACE_OVERLAP 0x00000100 /* Uses QuadInView() */

/*** if 1, collisions are detected with all other objects with the same
 *** flag set, and wo_CollisionProc is called. ***/

#define COLLISION_DETECT 0x00000200

/*** selecting FACE_SORTING will cause Init3DModel() to generate
 *** a face-center list (if not already created) and then
 *** sort faces besides objects. ***/

#define FACE_SORTING 0x00000400
#define FAST_OBJECT 0x00000800

/*** yup, these are in objectflags as well ***/

#define POLYGONTYPE 0x00010000
#define BITMAPTYPE 0x00020000
#define CAMERATYPE 0x00040000
#define SOUNDTYPE 0x00080000
#define SIMPLESOUNDTYPE 0x00100000

/*** flags for LightObject ***/

#define CONSTANT_ILLUMINATION 0x00000001 /* distance not factored in */
#define VARIABLE_ILLUMINATION 0x00000002 /* not used yet */

#define LIGHT_0 0x00000010
#define LIGHT_1 0x00000020
#define LIGHT_2 0x00000040
#define LIGHT_3 0x00000080
#define NO_LIGHTS 0x00000000

/*** Flags for SetObjectCCB ***/

#define CLONE_CCB_CONTROL 0x00000002
#define SHARE_CCB 0x00000008
#define COPY_CCB_CONTROL 0x00000001
#define USE_MESH_FACE 0x00000004

/*** tag lists ***/

#define F3D_TAG_DONE 0

#define F3D_TAG_OBJECT_FLAGS 1
#define F3D_TAG_CAMERA_FIELDSCALE 2
#define F3D_TAG_MODEL_LIGHT_AMBIENT 3
#define F3D_TAG_MODEL_LIGHTS 4
#define F3D_TAG_MODEL_NORMALS 5
#define F3D_TAG_CAMERA_MATRIX_METHOD 6
#define F3D_TAG_MODEL_MATRIX_METHOD F3D_TAG_CAMERA_MATRIX_METHOD
#define F3D_TAG_CAMERA_CAMERALENS 7
#define F3D_TAG_MODEL_MAXZSCALE 8
#define F3D_TAG_OBJECT_RADIUS 10
#define F3D_TAG_SOUND_MIN_DIST 11
#define F3D_TAG_SOUND_MAX_DIST 12
#define F3D_TAG_SOUND_MIN_VOLUME 13

#define F3D_TAG_MODEL_FLAGS F3D_TAG_OBJECT_FLAGS
#define F3D_TAG_SOUND_FLAGS F3D_TAG_OBJECT_FLAGS
#define F3D_TAG_CAMERA_FLAGS F3D_TAG_OBJECT_FLAGS

#define F3D_TAG_CAMERA_RADIUS F3D_TAG_OBJECT_RADIUS
#define F3D_TAG_MODEL_RADIUS F3D_TAG_OBJECT_RADIUS

#define F3D_TAG_VISIBILITY 14 /* camera visibility */

#define F3D_TAG_EYE_ZDIST 1
#define F3D_TAG_EYE_SEP 2

#define F3D_TAG_FACE_ORIENTATION 15
#define F3D_TAG_MODEL_PARENT 16

/*** camera visibility flags ***/

#define CAM1 0x00000001
#define CAM2 0x00000002
#define CAM3 0x00000004
#define CAM4 0x00000008
#define CAM5 0x00000010
#define CAM6 0x00000020
#define CAM7 0x00000040
#define CAM8 0x00000080
#define ALL_CAMERAS 0x000000ff
#define NO_CAMERAS 0
#define ALL_DISTANCES 0x0000ff00

/*** proc flags, used by SetCallBacks() ***/

#define PREPROCESSPROC 1
#define RENDERTIMEPROC 2
#define LIGHTPROC 3
#define MAPPINGPROC 4
#define OBJECTCULLINGPROC 5
#define FACECULLINGPROC 6
#define SIMPLESOUNDPROC 7
#define SCALEPROC 8
#define PERSPPROC 9
#define SORTPROC 10
#define COLLISIONPROC 11

/*** misc flags ***/

#define ITCHY_FLAG 0x00000001
#define SCRATCHY_FLAG 0x00000002

#define NO_FLAGS 0x00000000

/*** sound flags ***/

#define SOUND_ON 0x00000001
#define SOUND_OFF 0x00000000

/*** stereo stuff ***/

#define RIGHT_EYE 0L
#define LEFT_EYE 1L

#define STEREO_VIEW 0L
#define MONO_VIEW 1L

/*** This is the bit offset in the CameraVisibility flag. We normally use only
 *the first
 *** 8 bits to see if an object is visible to a certain camera. If an object is
 *** culled by distance it adds a second set of flags to check which overide
 *the
 *** CameraVisibilty stuff. Internal, when an object is invisible by distance,
 *it's distance
 *** still is checked. ***/

#define DIST_CULL_OFFSET                                                      \
  8L // bit offset into the CameraVisibilty word for distance culling bits

/*** ALL TYPEDEFS GO HERE ***/

typedef int32 vector[3];
typedef int32 Index2[2];
typedef int32 Obj3D; /*  handle to an object */

/*** STRUCTURE DEFINITIONS ***/

/*** A point in 3 space ***/

typedef struct point3f16
{
  frac16 x, y, z;
} point3f16;

typedef struct SoundDefinition
{
  frac16 sd_MinSoundDistance; /* distance sound will is the loudest */
  frac16 sd_MaxSoundDistance; /* distance sound sound is the softest */
  frac16 sd_MinSoundVolume;   /* lowest sound vol, max vol is DSP dependent */

  frac16 sd_SoundScale; /* maps 3D geometry units into 3D sound units */

  int32 sd_Flags;

  Item sd_LeftGain; /* used by simplesound stuff */
  Item sd_RightGain;
  Item sd_Sampler; /* sampler handler, used to get Frequency knob */

  PolarPosition4D sd_StartSound;
  PolarPosition4D sd_EndSound;
} SoundDefinition;

typedef struct WorldObject
{
  vec3f16 wo_WorldPosition;    /* X,Y,Z world co-ordinates */
  vec3f16 wo_WorldOrientation; /* X,Y,Z rotation NOT A VECTOR */
  vec3f16 wo_ObjectNormal;     /* Where the object is looking at */
  vec3f16 wo_ObjectNormalUp;   /* The normal "UP" point */
  vec3f16 wo_ObjectAttach;     /* Attaches two objects together */
  frac16 wo_WireLength;        /* The length of the ObjectAttach vector */
  struct WorldObject *wo_ParentObject; /* for hier. objects. NULL if this is
                                          not hierarchacal */
  frac16 wo_SphericalRadius;           /* the bounding circle */
  mat33f16 wo_ObjectMatrix;            /* The object's local matrix */
  mat33f16 wo_ResultMatrix; /* cameras ObjectMatrix  * ObjectMatrix */
  vec3f16 wo_LocalPosition; /* Position relative to camera */
  int32 wo_UpdateFlags;     /* Internal use to driver */

  /*** in MatrixMethod, the lowest byte is the rotation order, the second byte
   *** specifies the sorting method. ***/

  int32 wo_MatrixMethod;
  int32 *wo_ObjectDefinition; /* points to ModelDefinition */
  int32 *wo_TransformedData;  /* Ptr to transformed vertex data */
  int32 *wo_SourceData;       /* use for handle to a sound */
  int32 wo_PointCount;        /* Number of points that to multiply */

  /* ^^^^^^^^^^ DO NOT INSERT ANY NEW FIELDS ABOVE THIS LINE!
     ^^^^^^^^^^ DUE TO ASSEMBLY ADDRESSING RESTRICTIONS!!! APPEND
     ^^^^^^^^^^ ANY NEW FIELDS TO THE END OF THE STRUCT */

  int32 (*wo_PreProcessObject) ();  /* Code to execute during MATH time */
  int32 (*wo_RenderTimeProc) ();    /* Code you want to execute before the */
                                    /* CCB's are cued up. */
  int32 (*wo_LightProc) ();         /* handles lighting considerations */
  int32 (*wo_MappingProc) ();       /* handles screen mapping */
  int32 (*wo_ObjectCullingProc) (); /* handles any custom object culling */
  int32 (*wo_FaceCullingProc) ();   /* handles any custom object culling */
  int32 (*wo_SimpleSoundProc) ();   /* handles simple-sound processing */
  int32 (*wo_ScaleProc) ();         /* called when scales are changed */
  int32 (*wo_PerspProc) ();         /* Perspective proc */
  int32 wo_UserDefinedData;

  /*** wo_ModelDefinition points to the main model, and the MeshDef
   *** points to a meshed-version (based on the highest resolution
   *** version of the actual model). The user specifies if a model
   *** is to be meshed, the numbers of meshed versions and the
   *** distances that each mesh is visible. The object is always rendered
   *** based on the wo_ObjectDefinition above which may point to either
   *** one of the two fields below. Under most cases the pointer supplied
   *** by wo_ModelDefinition will be used, but if a mesh is called for
   *** then the system will load that pointer into the ObjectDef field. ***/

  int32 *wo_ModelDefinition;
  int32 *wo_MeshDefinition;

  /*** wo_LightPrefs : first 4 bits contains a minimum lighting value which
   *** specifies the minimum level of lighting for a face. This will let you
   *** show a face even when the light source is behind it, giving the
   *** impression of backlight at very low cost. 0 sez, turn the face black,
   *** 15 effectively turns off any dimming effect. The next 4 bits activate
   *** which lights to use, ie, if an object is to be illuminated by all 4
   *** lights turn on all bits, if just the first light, use the first bit
   *** (number 4). Currently only a single light source is available. ***/

  int32 wo_LightPrefs;
  frac16 wo_ObjectVelocity;

  /*** Camera MASK. Bit 0=camera 0, etc. for the first 8 bits. The next 8 bits
   *are used
   *** for distance visibility flags. If not set for particular camera, the
   *object is too
   *** far away to be seen based on a distance value passed to
   *CullByDistance(). I couldn't
   *** just use the camera flags since other routines use them and would be
   *interfered with
   *** with if I tinkered with them. Plus the system doesn't perform distance
   *calcs for
   *** and invisible object, so I needed to bypass that with a second set of
   *flags. ***/

  int32 wo_CameraVisibility;
  vec3f16 wo_ObjectDirection; /* The objects direction */

  /*** ZScale is a value calculated inside DrawModel for the Pseudoperspective
   *** calculations. If you get very close to a large object zscale goes
   *** way	up and the perspective goesto hell. This is easily	seen if
   *the
   *** image starts to	twist or only half of a face is rendered.
   *** MaxPPerspective sets an upper limit on the scalingand is one of those
   *** seat-'o-the-pants values. That is, set it until it looks good.
   *** Internally the vertex-x and vertex-y values are multiplied by zscale
   *** to arrive at the proper	screen coordinates,	so if zscale is	5 and
   *** vertex-x is 3, the screen-x	will be	15 (not	counting translational
   *** offsets). To use properly set this value so the final screen width and
   *** height of an object stay rational, that is, a rendered face should not
   *** be too much larger than the screen.
   ***/

  frac16 wo_MaxZScale;

  /*** Tells what scale of	object was used	for	last transform.	It is
   *-1 if	the
   *** object is not visible. ***/

  int32 wo_ScaleUsed;

  SoundDefinition *wo_SoundDef;

  /***	the	following fields for for internal use only and should be of
   *** little or no use to	the	programmer.	***/

  int32 wo_HeirLevel;     /* 0 if root, 1 for first child, etc. */
  int32 wo_CameraLens;    /* field-of-view */
  frac16 wo_CameraField;  /* like	FieldScale,but for CAMERA_PERSPECTIVE
                           */
  frac16 wo_DistToOrigin; /* Camera's distance to origin */
  frac16 wo_FieldScale;   /* the smaller,	the wider angle the	"lens" */
  int32 wo_ObjectFlags;   /* misc	stuff attribute	flags */
} WorldObject;

/*** 3D	lib uses its own definition which spells out the contents of the
 *** private area. */

/*** wp_CameraHandles and wp_SoundHandles are merely lists of the object
 *** indices in	the UserHandles list, as UserHandles contain ALL objects,
 *** geometric,	sound and cameras mixed	together. Note that the first element
 *** in	the SortedHandles list is reserved a list of just the visual
 *** objects for sorting (the same as UserObjects, but excludes	the
 *** camera and sound objects. ***/

typedef struct WorldPort
{
  void *wp_CurrentPointHeap; /* Should be *PointHeapPtr + ??? */
  void *wp_PointHeapPtr;     /* 3D-Transform	rendering area HEAP	*/
  int32 wp_CameraMask;       /* The mask of the current camera */
  int32 wp_MaxObjects;       /* This	MUST be	Max3DObjects */
  int32 wp_PointHeapSize;    /* This	is the size of the PointHeap in bytes
                              */
  int32 wp_SortMethod;       /* The sorting strategy. */
  int32 wp_TotalObjectCount; /* Total objects in database */
  int32 wp_HighestObjectIndex;
  int32 wp_TotalSortedObjects; /* Total	of objects in sorted list */

  /* ^^^^^^^^^^ DO	NOT	INSERT ANY NEW FIELDS ABOVE	THIS LINE!
     ^^^^^^^^^^ DUE TO	ASSEMBLY ADDRESSING	RESTRICTIONS!!!	APPEND
     ^^^^^^^^^^ ANY NEW UTILITY FIELDS TO THE END OF THE STRUCT */

  int32 wp_Eye;                 /* either LEFT or RIGHT eye, right=default*/
  int32 wp_EyeZDist;            /* distance of eyes from video screen */
  int32 wp_EyeSep;              /* seperation of eyes in inches */
  int32 (*wp_SortProc) ();      /* custom sorting hook */
  int32 (*wp_CollisionProc) (); /* called when a collision occurs */
  int32 wp_TotalSimpleSounds;   /* needed to set max-input volume */

  int32 wp_Reserved[10];

  int32 wp_Temp1;
  int32 wp_Temp2;
  int32 wp_Temp3;
  int32 wp_Temp4;

  void *wp_UserObjects[MAX3DOBJECTS];

  /*** This is the sorted lists of objects. wp_SortedObjects has all models,
   *based on their
   *** cameravisibility. The first column, has ALL models, the rest support the
   *visible
   *** lists for each camera, hence "MAX3DCAMERAS+1". If you want to look at
   *all models,
   *** then use somethine like ...wp_SortedObjects[i++][0], where i goes from
   *** 0 to total number of loaded objects. */

  void *wp_SortedObjects[MAX3DOBJECTS][MAX3DCAMERAS + 1];
  int32 wp_ObjectCount[8];              /* Count of objects for each camera */
  int32 wp_SoundHandles[MAX3DSOUNDS];   /* The list of sound objects */
  int32 wp_CameraHandles[MAX3DCAMERAS]; /* The list of cameras */

  vec3f16 wp_LightPosition[MAXLIGHTS]; /* light-source positions */
  int32 wp_SimpleSoundHandles[MAXSIMPLESOUNDS];
} WorldPort;

typedef struct Render3DPort
{
  Item rp_BitmapItem; /* bitmap to draw on */
  int32 rp_Width;
  int32 rp_Height;
  int32 rp_Center_x;
  int32 rp_Center_y;
} Render3DPort;

/*** GeoDefinition describes the actual geometry of the object. (Well, you
 *** got me, pd_ScaleThreshold is technically NOT a shape descriptor, but what
 *** are you going to do about it, huh?). Notice in PolyDefinition that we
 *** supply room for 5 sets of geometry to describe this item. These are
 *** used to create various levels of detail. If you have a complex object
 *** 3 parsecs away, you don't need to show all of the detail, instead you
 *** can choose to show a simpler version. pd_ScaleThreshold is the distance
 *** at which this particular object becomes visible. If you have only 1
 *object,
 *** this value is set to 0. If you have multiple versions, the most simple
 *** one goes first, the most complex, last. If a particular item's Z
 *coordinate
 *** (the distance) is between its pd_ScaleThreshold and that for the next one
 *** further out, it will show. gd_FaceCenters is used to store a central
 *** reference point for FACE_CULLING3 and dynamic face sorting. It is
 *** allocated and calculated ONLY when it is needed by the desired options.
 ****/

/*** The transformed face-center data may be moved into the main heap
 *** later on. ***/

typedef struct GeoDefinition
{
  int32 gd_RangeMin;
  int32 gd_RangeMax;
  int32 gd_PointCount;
  int32 gd_SurfaceCount;
  void *gd_PointList;          /* vertices */
  void *gd_SurfaceList;        /* connectivity array */
  void *gd_SurfaceType;        /* Two words, 1st is flag, 2nd is CCB or PIXC */
  void *gd_SortedList;         /* pointer to a table of eight sort lists */
  void *gd_FaceNormals;        /* array of face normals (vec3f16 format) */
  void *gd_FaceCenters;        /* array of face center points */
  void *gd_FaceCentersXformed; /* transformed face-center data */
  int32 gd_Flags;
  void *res;
} GeoDefinition;

/*** this is inserted into the object structure courtesy of the
 *** wo_ObjectDefinition field. ***/

typedef struct ModelDefinition
{
  int32 md_NumberOfScales; /* number of detail levels (GeoDefs) */
  struct GeoDefinition *md_GeoDefs[MAXNUMGEODEFS];
  int32 md_Flags;
} ModelDefinition;

/*** defines the gd_SurfaceType above. Use one of these for each face. ***/

typedef struct ModelSurface
{
  int32 ms_Flags;
  int32 ms_Control; /* CCB, PPMP, color, etc. */
} ModelSurface;

/*** special temporary vertex struct used when subdividing all of the faces of
 *an
 *** object up. Id1 and id2 are used to indicate the vertex number from the
 *original
 *** array, that make up the new vertices. If a new vertex is a direct copy of
 *an
 *** old one, id1 will have that id, and id2 will have a -1. I a new vertex is
 *** the average of 2, then the two values will have the id numbers used to
 *make
 *** the new one. Id3 is used later when creating a new vertex list minus the
 *** duplicate ones, since the first array made no effort to filter out the
 *dupes.
 *** Id3 contains the id of the unique vertex in the reduced array. Say, vertex
 *2, 4 and
 *** 12 in the original array are all the same. So only vertex 2 is copied over
 *as
 *** vertex 2 in the reduced version. Id3 for all 3 original vertices will
 *contain "2". ***/

typedef struct SpecialVertex
{
  point3f16 vertex;
  int32 id1;
  int32 id2;
  int32 id3;
  int32 dupe;
} SpecialVertex;

/*** Extra data for interpolated mesh objects. im_ParentGdef is a pointer to
 *the parent
 *** data which DoRotateMath needs to perform the rotations and calculations.
 *The corners
 *** of each face are then used to interpolate the mesh. The real geodef conn
 *array and
 *** surfacetype stuff is then mapped onto those points. ***/

typedef struct IntMeshDefs
{
  int32 im_WidthDiv;            // number of segments wide
  int32 im_HeightDiv;           // number of segments high
  GeoDefinition *im_ParentGdef; // pointer to parent geodef
  void *im_ParentData;          // pointer to parent's transformed data used to
                       // intepolate the mesh data.
  int32 im_Scale; // which of the parent's scales was used for this mesh
                  //(needed for cloning meshed objects)
} IntMeshDefs;

/*** struct used for 3dlib status ***/

typedef struct Lib3DState
{
  char l3ds_Version[10];
  int32 l3ds_TotalObjectCount;
  int32 l3ds_ModelCount;
  int32 l3ds_CameraCount;
  int32 l3ds_SoundCount;
  int32 l3ds_SimpleSoundCount;
  int32 l3ds_MeshCount;
  int32 l3ds_Size;
  int32 l3ds_HeapReq;
  int32 l3ds_MemReq;
  int32 l3ds_VertexCount;
  int32 l3ds_FaceCount;
  int32 l3ds_MaxPoints;
  int32 l3ds_HeapSize;
} Lib3DState;

/*** stuff for the IFF model files ***/

typedef struct CHUNK_HDR
{
  char chunkID[4];
  int32 chunkSize;
} CHUNK_HDR;

typedef struct LIST_CHUNK
{
  struct CHUNK_HDR header;
  int32 nElement;
  char *Data;
} LIST_CHUNK;

typedef struct QUAD_DATA
{
  int32 ul, ur, lr, ll;
  int32 texture_num;
} QUAD_DATA;

/*** this first field to this struct is a combined version number and flags,
 *** each of which are 2 bytes. Since the ARM pukes on 16 bit ints, I must
 *** combine both together. ***/

typedef struct GENERAL_INFO
{
  int32 VersionNumberFlags;
  int32 OrderingList[8];
  frac16 scaling[3];
  frac16 rotation[3];
  frac16 translation[3];
  frac16 radius;
} GENERAL_INFO;

#define CHUNK_3DVL CHAR4LITERAL ('3', 'D', 'V', 'L')
#define CHUNK_3DQL CHAR4LITERAL ('3', 'D', 'Q', 'L')
#define CHUNK_3DTL CHAR4LITERAL ('3', 'D', 'T', 'L')
#define CHUNK_3DNL CHAR4LITERAL ('3', 'D', 'N', 'L')
#define CHUNK_3DGI CHAR4LITERAL ('3', 'D', 'G', 'I')
#define CHUNK_MATR CHAR4LITERAL ('M', 'A', 'T', 'R')

/*** PROTOTYPES ***/

/*** the following are documented calls ***/

Obj3D Add3DSound (WorldPort *world, WorldObject *object, int32 flags);
Obj3D AddCamera (WorldPort *world, WorldObject *object);
Obj3D AddModel (WorldPort *world, WorldObject *object);
Obj3D AddSimpleSound (WorldPort *world, WorldObject *object, int32 flags);
void BuildCameraList (WorldPort *world, Obj3D Camera);
void BuildLookAtMatrix (mat33f16 mat, vec3f16 target, vec3f16 camera,
                        vec3f16 up);
void BuildXYZ (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void BuildXZY (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void BuildYXZ (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void BuildYZX (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void BuildZXY (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void BuildZYX (frac16 xr, frac16 yr, frac16 zr, mat33f16 resultmatrix);
void CallBacks (WorldPort *world, Obj3D Camera);
void ChangeLens (WorldPort *world, Obj3D camera_id, int32 fov);
WorldObject *CloneModel (WorldObject *object, int32 flags);
CCB *CreateColorCCB (int32 r, int32 g, int32 b);
void DestroyModel (WorldObject *object, int32 flags);
void DestroySound (WorldObject *object, int32 flags);
void DestroyWorld (WorldPort *world, int32 flags);
frac16 Distance3D (vec3f16 point1, vec3f16 point2);
void DoCollisionHandling (WorldPort *world, Obj3D camera_id);
int32 DoObjectsIntersect (WorldObject *obj1, WorldObject *obj2);
int32 DoRotateMath (WorldPort *wport, WorldObject *obj);
int32 DrawModel (Render3DPort *rport, WorldPort *world, WorldObject *model,
                 WorldObject *camera);
WorldObject *GetCameraAddress (WorldPort *world, Obj3D camera_id);
CCB *GetFaceCCB (WorldObject *object, int32 scale, int32 face_id, int32 flags);
frac16 GetModelRadius (WorldObject *object);
void GetObjectLocation (WorldObject *object, frac16 *x, frac16 *y, frac16 *z);
void GetObjectOrientation (WorldObject *object, frac16 *x, frac16 *y,
                           frac16 *z);
Err InitModel (WorldObject *initobj, ModelDefinition *mdef, TagArg *taglist);
int32 Init3DLib (int32 view_mode, int32 flags);
Err Init3DSound (WorldObject *initobj, char *soundname, TagArg *taglist);
Err InitCamera (WorldObject *initobj, TagArg *taglist);
Err InitGlasses (Item eb_port, Item reply_port);
Err InitNewWorld (WorldPort *world, char *point, int32 pointsize,
                  TagArg *taglist);
void InitRender3DPort (Render3DPort *rport, Item bitmapitem);
Err InitSimpleSound (WorldObject *initobj, char *soundname, TagArg *taglist);
int32 LightFace (WorldObject *object, int32 scale, int32 face_id,
                 int32 brightness, int32 flags);
void LightModel (WorldPort *world, WorldObject *object, int32 camera_id,
                 int32 flags);
int32 LookAtMatrix (mat33f16 matrix, vec3f16 camera, vec3f16 target,
                    frac16 roll);
void LookAt (WorldPort *world, Obj3D camera, vec3f16 target, frac16 roll);
ModelDefinition *LoadModel (char *file);
void MakeCRotationMatrix (vec3f16 orientation, mat33f16 matrix, int32 method);
void MapCelQuick (CCB *ccb, Point *quad);
void ModifyObject (WorldObject *object, TagArg *taglist);
int32 NormalizeMatrix (mat33f16 matrix);
void NormalizeVector (vec3f16 vect);
void OffsetModel (frac16 x, frac16 y, frac16 z, GeoDefinition *gd);
bool QuadInView (Render3DPort *rport, Point quad[4]);
Err Remove3DSound (WorldPort *world, Obj3D Obj);
Err RemoveCamera (WorldPort *world, Obj3D Obj);
Err RemoveModel (WorldPort *world, Obj3D Obj);
Err RemoveSimpleSound (WorldPort *world, Obj3D Obj);
int32 RenderSound (WorldPort *world, WorldObject *object, Obj3D camera_id);
void RenderWorld (Render3DPort *rport, WorldPort *world, Obj3D Camera);
void ResolveObjectMatrix (WorldPort *world, WorldObject *item);
void RotateModel (WorldObject *object, frac16 x, frac16 y, frac16 z);
void SetCallBacks (WorldPort *wport, WorldObject *object, int32 selection,
                   int32 (*fn) ());
Err SetFaceCCB (WorldObject *object, int32 scale, int32 face_id, CCB *ccb,
                int32 flags);
void SetFacePIXC (WorldObject *object, int32 scale, int32 face_id, int32 pixc,
                  int32 flags);
void SetModelCCB (WorldObject *object, CCB *ccb, int32 flags);
Err SetModelPIXC (WorldObject *object, int32 pixc);
void SetObjectLocation (WorldObject *object, frac16 x, frac16 y, frac16 z);
void SetObjectOrientation (WorldObject *object, frac16 x, frac16 y, frac16 z);
Err SetObjectSpeed (WorldObject *object, vec3f16 Direction, frac16 speed);
void ScanFaces (WorldPort *world, WorldObject *model, int32 (*fn) ());
void ScanObjects (WorldPort *world, Obj3D camera_id, int32 (*fn) ());
void StretchModel (WorldObject *object, frac16 x_size, frac16 y_size,
                   frac16 z_size);
void Snapshot (WorldPort *world, Obj3D camera);
void SortCameraList (WorldPort *world, Obj3D Camera);
int32 SwapScreens (ScreenContext sc);
void ToggleSimpleSound (WorldObject *object, int32 flags);
frac16 VectorDot (vec3f16 Vect1, vec3f16 Vect2);
frac16 VectorLength (vec3f16 x);
void WorldTick (WorldPort *world, uint32 timeval);

/*** The following are undocumented calls. This routines were designed for
 *** internal use only and are deemed too specific to be of practical use
 *** to the application developer. If curiosity gets the best of you feel
 *** free to snoop around in the source code and perhaps you will find
 *** some of these to be of use. ***/

Obj3D Add3DObject (WorldPort *world, WorldObject *object);
Err AddObject2List (WorldPort *world, int32 object_handle);
int32 BubbleSortObjects (WorldPort *world, Obj3D Camera);
void BuildRotateX (frac16 Angle, mat33f16 *resultmatrix);
void BuildRotateY (frac16 Angle, mat33f16 *resultmatrix);
void BuildRotateZ (frac16 Angle, mat33f16 *resultmatrix);
void CalcCameraField (Render3DPort *rport, WorldObject *cam);
int32 CameraMapping (Render3DPort *rport, WorldObject *object,
                     WorldObject *cam, int32 *vertex_array, int32 *result,
                     int32 pointcount);
void CenterModelByRadius (ModelDefinition *item);
void CenterModelByWeight (ModelDefinition *item);
void CreateFaceCenters (WorldObject *object);
void CreateNormalList (WorldObject *object);
void ClearUpdateFlags (WorldPort *world, uint32 mask);
void CopyMatrix (mat33f16 matrix, mat33f16 matrix);
Obj3D DeRefCameraHandle (WorldPort *world, Obj3D Obj);
Obj3D DeRefSoundHandle (WorldPort *world, Obj3D Obj);
int32 FaceCulling1Proc (WorldPort *world, WorldObject *object,
                        GeoDefinition *geo, int32 face_id,
                        point3f16 *vertices[4]);
int32 FaceCulling2Proc (WorldPort *world, WorldObject *object,
                        GeoDefinition *geo, int32 face_id,
                        point3f16 *vertices[4]);
int32 FaceCulling3Proc (WorldPort *world, WorldObject *object,
                        GeoDefinition *geo, int32 face_id,
                        point3f16 *vertices[4]);
void GetLocalPositions (WorldPort *world, Obj3D camera_id, WorldObject *cam);
int32 GetObjectSpeed (WorldPort *world, Obj3D Obj, vec3f16 Direction);
void GlueMulMat33Mat33_F16 (mat33f16 resultmat, mat33f16 obj_world_matrix,
                            mat33f16 cam_world_matrix);
Err Init3DObject (WorldObject *initobj, ModelDefinition *mdef, TagArg *taglist,
                  int32 type);
int32 IsFrontFace (Point sp[4]);
void MakeRotationMatrix (frac16 xrot, frac16 yrot, frac16 zrot,
                         mat33f16 matrix, int32 method);
void PreProcessCallbacks (WorldPort *world, Obj3D camera_id);
int32 PseudoMapping (Render3DPort *rport, WorldObject *object,
                     WorldObject *cam, int32 *vertex_array, int32 *result,
                     int32 pointcount);
void RotateOnX (frac16 Angle, mat33f16 matrix);
void RotateOnY (frac16 Angle, mat33f16 matrix);
void RotateOnZ (frac16 Angle, mat33f16 matrix);
void ResetHighestIndex (WorldPort *world, Obj3D obj);
void ResortLinks (WorldPort *world);
void RotateObjects (WorldPort *world, Obj3D Camera);
int32 SetModelScale (WorldPort *world, WorldObject *obj, int32 object_type);
void SummationSort (WorldPort *world, Obj3D Camera, int32 count);
void VectorTranslate (point3f16 *vertex1, vec3f16 *vertex2, int32 pointcount);

/*** misc. prototypes ***/

// void *malloc(int32 size);
// int32 printf(const char *fmt, ...);

/*** Remember : Don't put your pet gerbil in a microwave to dry it off. ***/

/*** new routines for 3dlib 1.1, starting Sept. 1993 ***/

int32 CreateMeshDefinition (WorldObject *object, int32 obj_id, int32 mindist,
                            int32 maxdist, int32 div, TagArg *tags);
CCB *ClipCel (CCB *ccb, int32 x, int32 y, int32 w, int32 h);
void Toggle3DSound (WorldObject *object, int32 flags);
void CreateNormalListForGdef (WorldObject *object, GeoDefinition *geodef);
void CreateFaceCentersForGdef (WorldObject *object, GeoDefinition *geodef);
GeoDefinition *CreateMeshGeodef (WorldObject *object, GeoDefinition *src_geo,
                                 int32 div);
void FreeGeodef (GeoDefinition *geodef);
void AverageVertex (point3f16 *v1, point3f16 *v2, point3f16 *v3);
void AverageSpecialVertex (SpecialVertex *v1, SpecialVertex *v2,
                           SpecialVertex *v3, int32 vnum);
void ScrollCel (CCB *ccb, CCB *srcccb, int32 x, int32 y, int32 w, int32 h);
CCB *ScreenToCel (Item screenitem);
void RotateQuad2D (Point srcquad[4], Point dstquad[4], int32 cx, int32 cy,
                   frac16 angle);
bool WorldToScreen (Render3DPort *rport, vec3f16 vtx, WorldObject *cam,
                    int32 mapping_form, int32 *vx, int32 *vy);

WorldObject *ClosestModel (WorldPort *world, Obj3D camera_id);
void Get3DLibState (WorldPort *wport, Lib3DState *state);

/*** new routines for 3dlib 1.2, starting November 12, 1993 ***/

void SetLightPosition (WorldPort *world, int32 lightid, frac16 x, frac16 y,
                       frac16 z);
void SetVec3f16 (vec3f16 vector, frac16 x, frac16 y, frac16 z);
char *Frac16str (frac16 value, char *buffer, int32 len);
void CullByDistance (WorldPort *world, Obj3D camera_id, frac16 distance);
int32 CreateIntMeshDefinition (WorldObject *object, int32 obj_id,
                               int32 mindist, int32 maxdist, int32 width,
                               int32 height, TagArg *taglist);
GeoDefinition *CreateIntMeshGeodef (WorldObject *object,
                                    GeoDefinition *src_geodef);
void DestroyGeodef (GeoDefinition *geodef, int32 flags);
void GenerateImeshData (WorldPort *wport, WorldObject *object);

#endif /* _3dlib_H */
