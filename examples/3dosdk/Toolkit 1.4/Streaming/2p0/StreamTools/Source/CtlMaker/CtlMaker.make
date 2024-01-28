#   File:       CtlMaker.make
#   Target:     CtlMaker
#   Sources:    CtlMaker.c
#   Created:    Saturday, July 10, 1993 10:56:34 PM
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


OBJECTS = CtlMaker.c.o
#DEBUGOPTIONS	= -sym full 
DEBUGOPTIONS = 


CtlMaker ÄÄ CtlMaker.make CtlMaker.r {OBJECTS}
	Rez  -o CtlMaker CtlMaker.r -a -ov
	Link {DEBUGOPTIONS} -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		#"{CLibraries}"CSANELib.o ¶
		#"{CLibraries}"Math.o ¶
		#"{CLibraries}"Complex.o ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		#"{Libraries}"ToolLibs.o ¶
		-o CtlMaker
CtlMaker.c.o Ä CtlMaker.make CtlMaker.c
	 C {DEBUGOPTIONS} -r  CtlMaker.c
