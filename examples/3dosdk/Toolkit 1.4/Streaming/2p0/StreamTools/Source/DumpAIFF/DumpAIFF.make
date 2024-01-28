#   File:       DumpAIFF.make
#   Target:     DumpAIFF
#   Sources:    DumpAIFF.c
#
#	Copyright © 1993-4 The 3DO Company
#
# 	All rights reserved. This material constitutes confidential and proprietary 
#	information of the 3DO Company and shall not be used by any Person or for any 
#	purpose except as expressly authorized in writing by the 3DO Company.
#
#
#	History:
#	05/27/94	DLD		Fixed some dependency problems
#						for the ToolKit directory structure.


OBJECTS = DumpAIFF.c.o
#DEBUG_FLAGS = -sym full
DEBUG_FLAGS = 


DumpAIFF ÄÄ DumpAIFF.make DumpAIFF.r {OBJECTS}
	Rez  -o DumpAIFF DumpAIFF.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{DEBUG_FLAGS} ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o DumpAIFF
DumpAIFF.c.o Ä DumpAIFF.make DumpAIFF.c
	 C {DEBUG_FLAGS} -r  DumpAIFF.c
