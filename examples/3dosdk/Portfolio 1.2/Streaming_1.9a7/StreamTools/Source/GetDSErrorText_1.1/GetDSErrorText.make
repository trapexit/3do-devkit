#   File:       GetDSErrorText.make
#   Target:     GetDSErrorText
#   Sources:    GetDSErrorText.c DataStreamDebug.c
#   Created:    Monday, November 15, 1993 6:54:31 PM


OBJECTS = GetDSErrorText.c.o

GetDSErrorText ÄÄ GetDSErrorText.make GetDSErrorText.r {OBJECTS}
	Rez  -o GetDSErrorText GetDSErrorText.r -a -ov
	Link -d -c 'MPS ' -t MPST ¶
		{OBJECTS} ¶
		"{CLibraries}"StdClib.o ¶
		"{Libraries}"Stubs.o ¶
		"{Libraries}"Runtime.o ¶
		"{Libraries}"Interface.o ¶
		-o GetDSErrorText
GetDSErrorText.c.o Ä GetDSErrorText.make GetDSErrorText.c
	 C -r  GetDSErrorText.c
