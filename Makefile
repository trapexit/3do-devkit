NAME       = helloworld
ISONAME    = iso/$(NAME).iso
FILESYSTEM = takeme
LAUNCHME   = $(FILESYSTEM)/LaunchMe
STACKSIZE  = 8192
BANNER	   = banner.png

ifeq ($(OS),Windows_NT)
  ifeq ($(origin MSYSTEM),undefined)
    IS_POSIX_SHELL := 0
  else
    IS_POSIX_SHELL := 1
  endif
else
  IS_POSIX_SHELL := 1
endif

ifeq ($(origin TDO_DEVKIT_PATH),undefined)
  $(warning WARNING: run "source activate-env" to have access to tooling)
  $(shell sleep 3)
  ifeq ($(wildcard .devkit-path),)
    TDO_DEVKIT_PATH := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
  else
    TDO_DEVKIT_PATH := $(shell cat .devkit-path)/
  endif
  ifeq ($(IS_POSIX_SHELL),1)
	ifeq ($(OS),Windows_NT)
		PATH := $(TDO_DEVKIT_PATH)bin/compiler/win:$(TDO_DEVKIT_PATH)bin/tools/win:$(TDO_DEVKIT_PATH)bin/buildtools/win:$(PATH)
	else
		PATH := $(TDO_DEVKIT_PATH)bin/compiler/linux:$(TDO_DEVKIT_PATH)bin/tools/linux:$(TDO_DEVKIT_PATH)bin/buildtools/linux:$(PATH)
	endif
  else
    PATH := $(TDO_DEVKIT_PATH)bin\compiler\win;$(TDO_DEVKIT_PATH)bin\tools\win;$(TDO_DEVKIT_PATH)bin\buildtools\win;$(PATH)
  endif
endif

$(info PATH=$(PATH))

.DEFAULT_GOAL := all

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

# ===== Flat source files in src/ become the LaunchMe boot binary =====
LAUNCHME_SRCS_C   = $(wildcard src/*.c)
LAUNCHME_SRCS_CXX = $(wildcard src/*.cpp)
LAUNCHME_SRCS_S   = $(wildcard src/*.s)
LAUNCHME_OBJS     = $(LAUNCHME_SRCS_S:src/%.s=build/%.s.o) $(LAUNCHME_SRCS_C:src/%.c=build/%.c.o) $(LAUNCHME_SRCS_CXX:src/%.cpp=build/%.cpp.o)

# ===== Subdirectory apps (src/<name>/main.*) =====
APP_DIRS = $(sort $(dir $(wildcard src/*/main.c src/*/main.cpp src/*/main.s)))
APPS     = $(notdir $(patsubst %/,%,$(APP_DIRS)))

PROGRAMS       = $(APPS)
PROGRAM_PATHS  = $(PROGRAMS:%=$(FILESYSTEM)/%)
APP_BUILD_DIRS = $(APPS:%=build/%)

OBJS = $(LAUNCHME_OBJS)

define APP_TEMPLATE
$(1)_SRCS_S   := $$(wildcard src/$(1)/*.s)
$(1)_SRCS_C   := $$(wildcard src/$(1)/*.c)
$(1)_SRCS_CXX := $$(wildcard src/$(1)/*.cpp)
$(1)_OBJS     := $$($(1)_SRCS_S:src/%.s=build/%.s.o) $$($(1)_SRCS_C:src/%.c=build/%.c.o) $$($(1)_SRCS_CXX:src/%.cpp=build/%.cpp.o)
OBJS          += $$($(1)_OBJS)

$(FILESYSTEM)/$(1): builddir $$($(1)_OBJS)
	armlink -o $$@ $$(LDFLAGS) $$(STARTUP) $$(LIBS) $$($(1)_OBJS)
endef

$(foreach app,$(APPS),$(eval $(call APP_TEMPLATE,$(app))))

OBJS := $(sort $(OBJS))

ifneq ($(strip $(LAUNCHME_OBJS)),)
$(LAUNCHME): builddir $(LAUNCHME_OBJS)
	armlink -o $@ $(LDFLAGS) $(STARTUP) $(LIBS) $(LAUNCHME_OBJS)
endif

DEPS = $(OBJS:.o=.d)

all: launchme programs modbin iso encrypt-iso

build/.touched:
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p build
	touch $@
else
	if not exist "build" mkdir "build"
	type nul > $@
endif

$(APP_BUILD_DIRS): build/.touched
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p "$@"
else
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
endif

builddir: build/.touched $(APP_BUILD_DIRS)

objs: builddir $(OBJS)

ifneq ($(strip $(LAUNCHME_OBJS)),)
launchme: $(LAUNCHME)
else
launchme:
endif

programs: $(PROGRAM_PATHS)

copy-data:
ifeq ($(IS_POSIX_SHELL),1)
	for d in $(wildcard src/*/takeme); do cp -r "$$d"/* "$(FILESYSTEM)"/; done
else
	for %%d in ($(subst /,\,$(wildcard src/*/takeme))) do xcopy /E /Y "%%d\*" "$(subst /,\,$(FILESYSTEM))"
endif

modbin: modbin-launchme $(PROGRAMS:%=modbin-%)

ifneq ($(strip $(LAUNCHME_OBJS)),)
modbin-launchme: $(LAUNCHME)
	modbin --name="$(NAME)" --time --stack=$(STACKSIZE) "$(LAUNCHME)" "$(LAUNCHME)"
else
modbin-launchme:
endif

modbin-%: $(FILESYSTEM)/%
	modbin --name="$*" --time --stack=$(or $($*_STACKSIZE),$(STACKSIZE)) "$<" "$<"

banner:
	3it to-banner -o "$(FILESYSTEM)/BannerScreen" "$(BANNER)"

iso/.touched:
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p iso
	touch $@
else
	if not exist "iso" mkdir "iso"
	type nul > $@
endif

isodir: iso/.touched

iso: isodir copy-data
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
ifeq ($(IS_POSIX_SHELL),1)
	rm -rvf "build" "iso" "$(LAUNCHME)" $(PROGRAM_PATHS:%="%")
else
	if exist "build" rmdir /S /Q "build"
	if exist "iso" rmdir /S /Q "iso"
	if exist $(subst /,\,$(LAUNCHME)) del $(subst /,\,$(LAUNCHME))
	for %%f in ($(subst /,\,$(PROGRAM_PATHS))) do if exist "%%f" del "%%f"
endif

distclean: clean
	git clean -xfd

run:
ifeq ($(OS),Windows_NT)
	run-iso.bat "$(ISONAME)"
else
	run-iso "$(ISONAME)"
endif

.PHONY: builddir isodir clean distclean launchme programs copy-data modbin modbin-launchme banner iso encrypt-iso run

-include $(DEPS)
