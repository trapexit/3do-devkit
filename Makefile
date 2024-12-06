NAME       = helloworld
ISONAME    = iso/$(NAME).iso
FILESYSTEM = takeme
LAUNCHME   = $(FILESYSTEM)/LaunchMe
STACKSIZE  = 4096
BANNER	   = banner.png

## Flag definitions ##
# -bigend   : Compiles code for an ARM operating with big-endian memory. The most
#             significant byte has lowest address.
# -za1      : LDR may only access word-aligned addresses.
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
ifeq ($(DEBUG),1)
OPT      = -O0
DEFFLAGS = -DDEBUG=1
else
OPT      = -O2
DEFFLAGS = -DNDEBUG=1
endif

INCPATH  = ${TDO_DEVKIT_PATH}/include
INCFLAGS = -I$(INCPATH)/3do -I$(INCPATH)/community -I$(INCPATH)/ttl
CFLAGS   = $(OPT) -bigend -za1 -zi4 -fa -fh -fx -fpu none -arch 3 -apcs "3/32/fp/swst/wide/softfp"
CXXFLAGS = $(CFLAGS)
ASFLAGS  = -bigend -fpu none -arch 3 -apcs "3/32/fp/swst"
LIBPATH  = ${TDO_DEVKIT_PATH}/lib
LDFLAGS  = -match 0x1 -nodebug -noscanlib -nozeropad -verbose -remove -aif -reloc -dupok -ro-base 0
STARTUP  = $(LIBPATH)/3do/cstartup.o

LIBS =						\
	$(LIBPATH)/3do/3dlib.lib		\
	$(LIBPATH)/3do/audio.lib		\
	$(LIBPATH)/3do/clib.lib			\
	$(LIBPATH)/3do/codec.lib		\
	$(LIBPATH)/3do/compression.lib		\
	$(LIBPATH)/3do/cpluslib.lib		\
	$(LIBPATH)/3do/DataAcq.lib		\
	$(LIBPATH)/3do/DataAcqShuttle.lib	\
	$(LIBPATH)/3do/DS.lib			\
	$(LIBPATH)/3do/DSShuttle.lib		\
	$(LIBPATH)/3do/exampleslib.lib		\
	$(LIBPATH)/3do/filesystem.lib		\
	$(LIBPATH)/3do/graphics.lib		\
	$(LIBPATH)/3do/input.lib		\
	$(LIBPATH)/3do/international.lib	\
	$(LIBPATH)/3do/intmath.lib		\
	$(LIBPATH)/3do/lib3do.lib		\
	$(LIBPATH)/3do/music.lib		\
	$(LIBPATH)/3do/mvelib.lib		\
	$(LIBPATH)/3do/operamath.lib		\
	$(LIBPATH)/3do/pgl.lib			\
	$(LIBPATH)/3do/string.lib		\
	$(LIBPATH)/3do/Subscriber.lib		\
	$(LIBPATH)/3do/swi.lib			\
	$(LIBPATH)/community/cpplib.lib		\
	$(LIBPATH)/community/example_folio.lib	\
	$(LIBPATH)/community/svc_funcs.lib      \
	$(LIBPATH)/community/svc_mem.lib        \
#	$(LIBPATH)/3do/burger.lib		\
#	$(LIBPATH)/3do/jstring.lib		\
#	$(LIBPATH)/3do/memdebug.lib		\
#	$(LIBPATH)/3do/obsoletelib3do.lib	\


SRCS_S   = $(wildcard src/*.s)
SRCS_C   = $(wildcard src/*.c)
SRCS_CXX = $(wildcard src/*.cpp)

OBJS += $(SRCS_S:src/%.s=build/%.s.o)
OBJS += $(SRCS_C:src/%.c=build/%.c.o)
OBJS += $(SRCS_CXX:src/%.cpp=build/%.cpp.o)

DEPS = $(OBJS:.o=.d)

all: launchme modbin iso encrypt-iso

build/.touched:
ifeq ($(OS),Windows_NT)
	if not exist "build" mkdir "build"
	type nul > $@
else
	mkdir -p "build"
	touch $@
endif

builddir: build/.touched

objs: builddir $(OBJS)

$(LAUNCHME): objs
	armlink -o $@ $(LDFLAGS) $(STARTUP) $(LIBS) $(OBJS)

launchme: $(LAUNCHME)

modbin:
	modbin --name="$(NAME)" --time --stack=$(STACKSIZE) "$(LAUNCHME)" "$(LAUNCHME)"

banner:
	3it to-banner -o "$(FILESYSTEM)/BannerScreen" "$(BANNER)"

iso/.touched:
ifeq ($(OS),Windows_NT)
	if not exist "iso" mkdir "iso"
	type nul > $@
else
	mkdir -p "iso"
	touch $@
endif

isodir: iso/.touched

iso: isodir
	3doiso -in "$(FILESYSTEM)" -out "$(ISONAME)"

encrypt-iso: $(ISONAME)
	3DOEncrypt genromtags "$(ISONAME)"

build/%.s.o: src/%.s
	armasm $(INCFLAGS) $(DEFFLAGS) $(ASFLAGS) $< -o $@

build/%.c.o: src/%.c
	armcc $(INCFLAGS) $(DEFFLAGS) $(CFLAGS) -M $< -o $@ > ${@:.o=.d}
	armcc $(INCFLAGS) $(DEFFLAGS) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp
	armcpp $(INCFLAGS) $(DEFFLAGS) $(CXXFLAGS) -M $< -o $@ > ${@:.o=.d}
	armcpp $(INCFLAGS) $(DEFFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
ifeq ($(OS),Windows_NT)
	if exist "build" rmdir /S /Q "build"
	if exist "iso" rmdir /S /Q "iso"
	if exist $(subst /,\,$(LAUNCHME)) del $(subst /,\,$(LAUNCHME))
else
	rm -rvf "build" "iso" $(LAUNCHME)
endif

run:
	run-iso "$(ISONAME)"

.PHONY: builddir isodir clean modbin banner iso encrypt-iso run

-include $(DEPS)
