
#define SH	65536
#define SM	32768
#define CH	10000

#include "3dlib.h"

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
	0,3,2,1,
	0,1,5,4,
	4,5,6,7,
	2,3,7,6,
	0,4,7,3,
	1,2,6,5
};

GeoDefinition cube_geodef=
{
	0,
	(500<<16),
	8,
	6,
	&CubePoints,
	&CubeFacits,
	0,
        0,
        0,
        0
};

ModelDefinition Cube ={
	1,
        &cube_geodef,
        0,0,0,0,0,0,0
};





