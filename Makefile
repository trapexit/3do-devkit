DEBUG = 1

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
# -za0      : LDR is not restricted to accessing word-aligned addresses.
# -zps0     :
# -zpv1     :
# -zi4      : The compiler selects a value for the maximum number of instructions
#             allowed to generate an integer literal inline before using LDR rx,=value
# -fa       : Checks for certain types of data flow anomalies.
# -fh       : Checks "all external objects are declared before use",
#             "all file-scoped static objects are used",
#             "all predeclarations of static functions are used between
#              their declaration and their definition".
# -fv       : Reports on all unused declarations, including those from standard headers.
# -fx       : Enables all warnings that are suppressed by default.
# -fpu none : No FPU. Use software floating point library.
# -arch 3   : Compile using ARM3 architecture.
# -apcs     : nofp - Do not use a frame-pointer register. This is the default.
#             softfp - Use software floating-point library functions for floating-point code.
#             noswstackcheck - No software stack-checking PCS variant. This is the default.
OPT      = -O2
CFLAGS   = $(OPT) -bigend -za1 -zps0 -zi4 -fa -fh -fx -fpu none -arch 3 -apcs '/softfp/nofp/swstackcheck'
CXXFLAGS = $(CFLAGS)
ASFLAGS  = -BI -i ./include/3do
INCPATH  = -I ./include/3do -I ./include/ttl
LIBPATH  = ./lib
LDFLAGS  = -match 0x1 -nodebug -noscanlib -verbose -remove -aif -reloc -dupok -ro-base 0 -sym $(EXENAME).sym
STARTUP  = $(LIBPATH)/3do/cstartup.o

LIBS =					\
	$(LIBPATH)/3do/clib.lib		\
	$(LIBPATH)/community/cpplib.lib	\
	$(LIBPATH)/3do/graphics.lib     \
	$(LIBPATH)/3do/lib3do.lib       \
	$(LIBPATH)/3do/filesystem.lib   \
	$(LIBPATH)/3do/operamath.lib

SRC_S   = $(wildcard src/*.s)
SRC_C   = $(wildcard src/*.c)
SRC_CXX = $(wildcard src/*.cpp)

OBJ += $(SRC_S:src/%.s=build/%.s.o)
OBJ += $(SRC_C:src/%.c=build/%.c.o)
OBJ += $(SRC_CXX:src/%.cpp=build/%.cpp.o)

all: launchme modbin iso

launchme: builddir $(OBJ)
	$(LD) -o $(EXENAME) $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJ)

modbin:
	$(MODBIN) --stack=$(STACKSIZE) $(EXENAME) $(EXENAME)

banner:
	$(MAKEBANNER) $(BANNER) $(FILESYSTEM)/BannerScreen

iso:
	$(3DOISO) -in $(FILESYSTEM) -out $(ISONAME)
	$(3DOENCRYPT) genromtags $(ISONAME)

builddir:
	mkdir -p build/

build/%.s.o: src/%.s
	$(AS) $(INCPATH) $(ASFLAGS) $< -o $@

build/%.c.o: src/%.c
	$(CC) $(INCPATH) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp
	$(CXX) $(INCPATH) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) -vf $(OBJ) $(EXENAME) $(EXENAME).sym $(ISONAME)

.PHONY: clean modbin banner iso
