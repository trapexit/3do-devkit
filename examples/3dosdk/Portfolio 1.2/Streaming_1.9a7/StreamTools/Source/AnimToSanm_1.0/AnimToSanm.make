#   File:       AnimToSanm.make
#   Target:     AnimToSanm
#   Sources:    GetOpt.c AnimToSanm.c
#   Created:    Tuesday, July 20, 1993 4:39:46 PM


OBJECTS = GetOpt.c.o AnimToSanm.c.o



AnimToSanm ÄÄ AnimToSanm.make AnimToSanm.r {OBJECTS}
	Rez  -o AnimToSanm AnimToSanm.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		-o AnimToSanm
GetOpt.c.o Ä AnimToSanm.make GetOpt.c
	 C -r  GetOpt.c
AnimToSanm.c.o Ä AnimToSanm.make AnimToSanm.c
	 C -r  AnimToSanm.c
