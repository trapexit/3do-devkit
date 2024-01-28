#define SH	65536
#define SM	32768
#define CH	10000

#include "3dlib.h"

long StarPoints[]={
	0,SH,0,
	SM,SM,0,
	SH,0,0,
	SM,-SM,0,
	0,-SH,0,
	-SM,-SM,0,
	-SH,0,0,
	-SM,SM,0,
	0,0,-1000,
	0,0,1000  };
	

long StarFacits[]={
	0,1,9,9,
	1,2,9,9,
	2,3,9,9,
	3,4,9,9,
	4,5,9,9,
	5,6,9,9,
	6,7,9,9,
	7,1,9,9,	
	0,1,8,8,
	1,2,8,8,
	2,3,8,8,
	3,4,8,8,
	4,5,8,8,
	5,6,8,8,
	6,7,8,8,
	7,1,8,8
};

long CubePoints[]={
	 32768,32768,-32768,
	 32768,-32768,-32768,
	-32768,-32768,-32768,
	-32768,32768,-32768,
	 32768,32768,32768,
	 32768,-32768,32768,
	-32768,-32768,32768,
	-32768,32768,32768
};

long CubeFacits[]=
{ 
	0,1,2,3,
  	0,4,5,1,
 	4,7,6,5,
 	2,6,7,3,
	0,3,7,4,
	1,5,6,2
};

GeoDefinition cube_geodef={
	(1<<16),
	(400<<16),
	8,
	6,
	&CubePoints,
	&CubeFacits,
	0,
        0,
        0,
        0,
	0,
        0,
        0
};

ModelDefinition Cube ={
	1,
        &cube_geodef,
        0,0,0,0,0,0,0,
	0
};

/*** this uses the cube data to generate a single
 *** square face. ***/

long SquareFacits[]=
{
	0,1,2,3,
 	0,4,5,1,
 	4,7,6,5,
 	2,6,7,3,
	0,3,7,4,
	1,5,6,2
};

GeoDefinition square_geodef={
	(2<<16),
	(400<<16),
	8,
	6,
	&CubePoints,
	&SquareFacits,
	0,
        0,
        0,
        0,
	0,
        0,
        0
};

ModelDefinition Square ={
	1,
        &square_geodef,
        0,0,0,0,0,0,0
};

GeoDefinition star_geo ={
	0,
	(400<<16),
	10,
	16,
	&StarPoints,
	&StarFacits,
	0,
        0,
        0,
	0,
	0,
        0,
        0
};

ModelDefinition Star ={
	1,
        &star_geo,
        0,0,0,0,0,0,0,0
};

long PyramidPoints[]={
              0, 98304,     0,              
	 -32768,-65536,-32768,
	  32768,-65536,-32768,
	  32768,-65536, 32768,
	 -32768,-65536, 32768
};

long PyramidFacits[]=
{
	0,2,1,0,
	0,3,2,0,
	0,4,3,0,
	0,1,4,0,
	1,2,3,4
};

GeoDefinition pyramid_geodef=
{
        (1<<16),
	(400<<16),
	5,
	5,
	&PyramidPoints,
	&PyramidFacits,
	0,
        0,
        0,
        0,
        0,
        0,
        0
};

ModelDefinition Pyramid ={
	1,
        &pyramid_geodef,
        0,0,0,0,0,0,0,0
};

/*** this will show a cube close up, which will then switch into
 *** a pyramid farther away. ***/

GeoDefinition   pyramid_cube_geodef =
{
        (0<<16),             /* geodef 0 */                  
	(5<<16),
	8,
	6,
	&CubePoints,
	&CubeFacits,
	0,
        0,
        0,
        0,
        0,
        0,
        0
};

GeoDefinition   pyramid2_geodef =
{
        (5<<16),             /* geodef 1 */
	(400<<16),
	5,
	5,
	&PyramidPoints,
	&PyramidFacits,
	0,
        0,
        0,
        0,
        0,
        0,
        0
};

ModelDefinition Pyramid_cube =
{
	2,
        &pyramid_cube_geodef,
        &pyramid2_geodef,
        0,0,0,0,0,0,0
};

