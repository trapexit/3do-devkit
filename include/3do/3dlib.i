;--------------------------------------------------
;
; 3Dlib.i
;
;--------------------------------------------------

; MAX3DCAMERAS and MAX3DSOUNDS are here and should be handled with care, since 
; 3dlib might end up as system code one of these days, in which case the 
; internal array sizes will be frozen and could conflict with application sizes.
;
; If you do play with these, and your apps break in the future, "we told 
; you so!"

Max3DCameras	EQU	8
Max3DSounds     EQU	4
MaxSimpleSounds EQU	4
MaxLights	EQU     4

 BEGINSTRUCT	WorldPort
	LONG wp_CurrentPointHeap
	PTR  wp_PointHeapPtr	;
	LONG wp_CameraMask
	LONG wp_MaxObjects	;The maximum number of 3D objects.	
	LONG wp_PointHeapSize	;
	LONG wp_SortMethod	;The sorting strategy.
	LONG wp_TotalObjectCount
	LONG wp_HighestObjectIndex;
	LONG wp_TotalSortedObjects

;^^^^^^^^^^ DO NOT INSERT ANY NEW FIELDS ABOVE THIS LINE! 
;^^^^^^^^^^ DUE TO ASSEMBLY ADDRESSING RESTRICTIONS!!! APPEND
;^^^^^^^^^^ ANY NEW UTILITY FIELDS TO THE END OF THE STRUCT */

	LONG wp_Eye             ;for stereo ops
	LONG wp_EyeZDist        ;Dist of eye from screen
	LONG wp_EyeSep          ;Dist between eyes
	LONG wp_SortProc        ;Custom sorting callback
	LONG wp_CollisionProc   ;Call this for a collision
	LONG wp_TotalSimpleSounds

 ARRAY	LONG,wp_Reserved,10

	LONG wp_Temp1
	LONG wp_Temp2
	LONG wp_Temp3
	LONG wp_Temp4
  ARRAY	LONG,wp_UserObjects,Max3DObjects
  ARRAY	LONG,wp_SortedObjects,(Max3DObjects*(Max3DCameras+1))
  ARRAY	LONG,wp_ObjectCount,8
  ARRAY	LONG,wp_SoundHandles,Max3DSounds
  ARRAY	LONG,wp_CameraHandles,Max3DCameras
  ARRAY LONG,wp_LightPosition,(MaxLights*3)
  ARRAY LONG,wp_SimpleSoundHandles,(MaxSimpleSounds)
	
 ENDSTRUCT

 BEGINSTRUCT	WorldObject
  ARRAY	LONG,wo_WorldPosition,3		;x,y,z position in world co-ordinates.	
  ARRAY	LONG,wo_WorldOrientation,3	;X,Y,Z rotation
  ARRAY	LONG,wo_ObjectNormal,3		;The point an object "looks at"
  ARRAY	LONG,wo_ObjectNormalUp,3	;The up point for the above
  ARRAY	LONG,wo_ObjectAttach,3		;The attachment vector	
	LONG wo_WireLength		;The length of the attachment vector.
	LONG wo_AttachedObject		;pointer to WorldObject attached to.
	LONG wo_SphericalRadius      	;Geometric extent
  ARRAY	LONG,wo_ObjectMatrix,9		;object relative to the world
  ARRAY	LONG,wo_ResultMatrix,9		;View matrix * WorldMatrix
  ARRAY	LONG,wo_LocalPosition,3		;(ResultMatrix * WorldPosition)
	LONG wo_UpdateFlags		;(Internal) use to driver
	LONG wo_MatrixMethod		;Orientation method, and rotation order.
	LONG wo_ObjectDefinition	;Pointer to 3DObject
	LONG wo_TransformedData		;(Internal) points after transform
	LONG wo_SourceData              ;original (Untransformed) data
	LONG wo_PointCount              ;Number of points to operated on 

;^^^^^^^^^^ DO NOT INSERT ANY NEW FIELDS ABOVE THIS LINE! 
;^^^^^^^^^^ DUE TO ASSEMBLY ADDRESSING RESTRICTIONS!!! APPEND
;^^^^^^^^^^ ANY NEW FIELDS TO THE END OF THE STRUCT 

	LONG wo_PreProcessObject	;Call back code prior to obj rotate.
	LONG wo_RenderTimeProc		;Call back code before rendering.
	LONG wo_LightProc               ;Lighting callback
	LONG wo_MappingProc 		;Custom screen mapping
	LONG wo_ObjectCullingProc       ;Custom object culling
	LONG wo_FaceCullingProc     	;Custom face culling
	LONG wo_SimpleSoundProc         ;Callback for SimpleSound support
	LONG wo_ScaleProc               ;Called when the scale changes
        LONG wo_wo_PerspProc            ;Called to handle perspective

	LONG wo_UserDefinedData		;Pointer to any user defined data.

	LONG wo_ModelDefinition         ;(see note in 3dlib.h)
	LONG wo_MeshDefinition          ;   "          "

	LONG wo_LightPrefs;  		;Lighting prefs (see note in 3dlib.h)    
	LONG wo_ObjectVelocity		;Speed of the object.
	LONG wo_CameraVisibility	;What cameras the object is visible to.
  ARRAY	LONG,wo_ObjectDirection,3	;Vector where object is going towards.

	LONG wo_MaxZScale               ;Perspective cap, see 3dlib.h notes
	LONG wo_ScaleUsed		;(Internal) Tells last object scale.
        LONG wo_SoundDef                ;pointer to a sound definition

	LONG wo_HeirLevel		;Heirarchacal level
	LONG wo_CameraLens              ;Camera's field of view
	LONG wo_CameraField            	;Like FieldScale for CameraPerspective
	LONG wo_DistToOrigin            ;Used in camera-persp calcs
	LONG wo_FieldScale             	;needed for screen mapping
	LONG wo_ObjectFlags            	;Global flags for object
 ENDSTRUCT
		

 BEGINSTRUCT	ModelDefinition
	LONG op_NumberOfScales          ;Number of detail levels
  ARRAY	LONG,op_GeoDefs,8	
	LONG op_Flags 
 ENDSTRUCT

 BEGINSTRUCT	GeoDef
	LONG gd_RangeMin		;Minimum distance this scale used.
	LONG gd_RangeMax		;Maximum distance this scale used.
	LONG gd_PointCount		;Number of 3d points.
	LONG gd_SurfaceCount		;Number of 3D rectangular surfaces.
	LONG gd_PointList		;The pointer to the 3D points
	LONG gd_SurfaceList		;The pointer to the connectivity data.
	LONG gd_SurfaceType		;Two words, 1st is flag,
						;   2nd is CCB or PPMP,color
	LONG gd_SortedList		;A pointer to 8 presorted surface lists.
	LONG gd_FaceNormals		;List of face normals
	LONG gd_FaceCenters		;Array of face centerpoints
	LONG gd_FaceCentersXformed	;Transformed face centers 
	LONG gd_Flags 			;Misc flags
	LONG gd_Res			;Reserve space
 ENDSTRUCT
;Obj3D Add3dObject(long *world,long *object);
;Err Delete3dObject(long *world,Obj3D Obj);

	IMPORT	AddModel
	IMPORT	RemoveModel

;Obj3D AddCamera(long *world,long *object);
;Err DeleteCamera(long *world,Obj3D Obj);

	IMPORT	AddCamera
	IMPORT	RemoveCamera

;Obj3D AddSound(long *world,long *object,uint32 volume);
;Err DeleteSound(long *world,long *object);

	IMPORT	Add3DSound
	IMPORT	Remove3DSound

;Err SetObjectSpeed(long *world,Obj3D Obj,vector3 Direction,long speed);
;long GetObjectSpeed(long *world,Obj3D Obj,vector3 Direction);

	IMPORT	SetObjectSpeed
	IMPORT	GetObjectSpeed

;Err SetObjectMatrix(long *world,Obj3D Obj,mat3 matrix)
	
;	IMPORT	SetObjectMatrixx

;Err DrawWorld(long *world,Obj3D camera);
;void WorldTick(long *world,uint32 timeval);

;	IMPORT	DrawWorld
	IMPORT	WorldTick


	END

