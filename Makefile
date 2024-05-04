FILESYSTEM = takeme
EXENAME	   = $(FILESYSTEM)/LaunchMe
ISONAME    = helloworld.iso
STACKSIZE  = 4096
BANNER	   = banner.bmp

CC	   = armcc
CXX	   = armcpp
AS 	   = armasm
LD	   = armlink
RM	   = rm
MODBIN     = modbin
MAKEBANNER = MakeBanner
3DOISO     = 3doiso
3DOENCRYPT = 3DOEncrypt

## Flag definitions ##
# -bigend   : Compiles code for an ARM operating with big-endian memory. The most
#             significant byte has lowest address.
# -za1      : LDR may only access word-aligned addresses.
# -zps0     : Unknown
# -zpv1     : Unknown
# -zi4      : The compiler selects a value for the maximum number of instructions
#             allowed to generate an integer literal inline before using LDR rx,=value
# -fa       : Checks for certain types of data flow anomalies.
# -fh       : Checks "all external objects are declared before use",
#             "all file-scoped static objects are used",
#             "all predeclarations of static functions are used between
#              their declaration and their definition".
# -fx       : Enables all warnings that are suppressed by default.
# -fpu none : No FPU. Use software floating point library.
# -arch 3   : Compile using ARM3 architecture.
# -apcs     : See page 1-13 of ARM SDT Ref Guide
#             The default procedure call standard for the ARM compiler in SDT 2.11a was
#             -apcs 3/32/fp/swst/wide/softfp.
#             The default Procedure Call Standard (PCS) for the ARM compiler, and
#             the assembler in SDT 2.50 and C++ 1.10 is now:
#             -apcs 3/32/nofp/noswst/narrow/softfp
OPT      = -O2
CFLAGS   = $(OPT) -bigend -za1 -zps0 -zi4 -fa -fh -fx -fpu none -arch 3 -apcs '3/32/nofp/swst/wide/softfp'
CXXFLAGS = $(CFLAGS)
ASFLAGS  = -BI -i ./include/3do -bigend -fpu none -arch 3 -apcs '3/32/nofp/swst'
INCPATH  = ./include
INCFLAGS = -I$(INCPATH)/3do -I$(INCPATH)/community -I$(INCPATH)/ttl
LIBPATH  = ./lib
LDFLAGS  = -match 0x1 -nodebug -noscanlib -verbose -remove -aif -reloc -dupok -ro-base 0 -sym $(EXENAME).sym
STARTUP  = $(LIBPATH)/3do/cstartup.o

LIBS =						\
	$(LIBPATH)/3do/clib.lib			\
	$(LIBPATH)/3do/filesystem.lib		\
	$(LIBPATH)/3do/graphics.lib		\
	$(LIBPATH)/3do/lib3do.lib		\
	$(LIBPATH)/3do/operamath.lib		\
	$(LIBPATH)/community/cpplib.lib		\
	$(LIBPATH)/community/example_folio.lib	\
	$(LIBPATH)/community/svc_funcs.lib      \
	$(LIBPATH)/community/svc_mem.lib

SRC_S   = $(wildcard src/*.s)
SRC_C   = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)

OBJ += $(SRC_S:src/%.s=build/%.s.o)
OBJ += $(SRC_C:src/%.c=build/%.c.o)
OBJ += $(SRC_CXX:src/%.cpp=build/%.cpp.o)

all: launchme modbin iso

launchme: builddir $(OBJ)
	$(LD) -o $(EXENAME) $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)

modbin: launchme
	$(MODBIN) --stack=$(STACKSIZE) $(EXENAME) $(EXENAME)

banner:
	$(MAKEBANNER) $(BANNER) $(FILESYSTEM)/BannerScreen

iso: modbin
	$(3DOISO) -in $(FILESYSTEM) -out $(ISONAME)
	$(3DOENCRYPT) genromtags $(ISONAME)

builddir:
	mkdir -p build/

build/%.s.o: src/%.s
	$(AS) $(INCFLAGS) $(ASFLAGS) $< -o $@

build/%.c.o: src/%.c
	$(CC) $(INCFLAGS) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp
	$(CXX) $(INCFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -vf $(OBJ) $(EXENAME) $(EXENAME).sym $(ISONAME)

.PHONY: clean modbin banner iso
