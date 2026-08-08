NAME       = helloworld
ISO_DIR    = iso
ISONAME    = $(ISO_DIR)/$(NAME).iso
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
# nofp      : Part of the -apcs string. In the default fp variant, register r11 is
#             reserved as the frame pointer: every function with a non-trivial frame
#             saves it in the prologue, points it at the current stack pointer, and
#             addresses its locals and parameters through it, which also lets a
#             debugger walk the frame chain to produce backtraces.
#             With nofp, r11 is freed for general use as an ordinary callee-saved
#             variable register (v6). The prologue/epilogue no longer save and
#             restore the frame pointer, locals are addressed relative to sp instead
#             of r11, and the compiler gains a full extra register to relieve
#             register pressure. Net effect: smaller, faster code.
#             It is ABI compatible: arguments and return values are passed identically
#             in both variants, and r11 is callee-saved either way, so objects
#             compiled with and without a frame pointer link and interoperate freely.
#             The only thing lost is debugger stack backtraces, which is irrelevant
#             here because the original remote debugger is not used.
# -zpno_check_stack : -zp passes options to the compiler's pragma system, so this is
#             equivalent to #pragma no_check_stack. By default the compiler emits a
#             stack-limit check in every function prologue that allocates stack space:
#             the new sp is compared against the stack limit maintained by the runtime
#             and a call to __rt_stkovf_split or __rt_stkovf_split_small is made when
#             the frame could overflow, ultimately invoking __rt_stkovf_handler to
#             abort cleanly.
#             no_check_stack removes that compare-plus-call from every prologue,
#             saving cycles (a call costs a pipeline refill on the ARM60). The
#             tradeoff: stack exhaustion is no longer detected. The stack silently
#             grows into adjacent memory and corrupts it (or raises a data abort on
#             an unmapped page) instead of aborting with a clear error. Acceptable
#             because 3DO task stacks and frame usage are bounded and known.
ifeq ($(DEBUG),1)
OPT      = -O0
DEFFLAGS = -DDEBUG=1
else
OPT      = -O2 -zpno_check_stack
DEFFLAGS = -DNDEBUG=1
endif

INCPATH  = ${TDO_DEVKIT_PATH}/include
INCFLAGS = -I$(INCPATH)/3do -I$(INCPATH)/community -I$(INCPATH)/ttl
CFLAGS   = $(OPT) -bigend -za1 -zi4 -fa -fh -fx -fpu none -arch 3 -apcs "3/32/nofp/swst/wide/softfp"
CXXFLAGS = $(CFLAGS)
ASFLAGS  = -bigend -fpu none -arch 3 -apcs "3/32/nofp/swst"
ARMLIB   = armlib
ARMLIBFLAGS = -c -o
LIBPATH  = ${TDO_DEVKIT_PATH}/lib
LDFLAGS  = -match 0x1 -nodebug -noscanlib -nozeropad -verbose -remove -aif -reloc -dupok -ro-base 0
STARTUP  = $(LIBPATH)/3do/cstartup.o

LIBS =						\
	$(LIBPATH)/3do/3dlib.lib		\
	$(LIBPATH)/3do/audio.lib		\
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
	$(LIBPATH)/community/libc.lib       	\
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

# ===== Subdirectory libraries (src/<name>/* without main.*) =====
SRC_DIRS = $(sort $(dir $(wildcard src/*/*.s src/*/*.c src/*/*.cpp)))
LIB_DIRS = $(filter-out $(APP_DIRS),$(SRC_DIRS))
LIBRARIES = $(notdir $(patsubst %/,%,$(LIB_DIRS)))

LIBRARY_PATHS = $(foreach library,$(LIBRARIES),build/$(library)/$(library).lib)
LIBRARY_BUILD_DIRS = $(LIBRARIES:%=build/%)
INSTALL_LIB_DIR = $(LIBPATH)/community

SUBDIR_BUILD_DIRS = $(sort $(APP_BUILD_DIRS) $(LIBRARY_BUILD_DIRS))
LINK_LIBRARY_PATHS = $(filter $(LIBRARY_PATHS),$(LIBS))

OBJS = $(LAUNCHME_OBJS)

rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))
DATA_DIRS = $(patsubst %/,%,$(wildcard src/*/takeme/))
DATA_INPUTS = $(foreach dir,$(DATA_DIRS),$(call rwildcard,$(dir)/,*))
FILESYSTEM_INPUTS = $(call rwildcard,$(FILESYSTEM)/,*)
COPY_DATA_STAMP = build/.copy-data.stamp

define APP_TEMPLATE
$(1)_SRCS_S   := $$(wildcard src/$(1)/*.s)
$(1)_SRCS_C   := $$(wildcard src/$(1)/*.c)
$(1)_SRCS_CXX := $$(wildcard src/$(1)/*.cpp)
$(1)_OBJS     := $$($(1)_SRCS_S:src/%.s=build/%.s.o) $$($(1)_SRCS_C:src/%.c=build/%.c.o) $$($(1)_SRCS_CXX:src/%.cpp=build/%.cpp.o)
OBJS          += $$($(1)_OBJS)

$(FILESYSTEM)/$(1): $$(LINK_LIBRARY_PATHS) $$($(1)_OBJS) | build $$(SUBDIR_BUILD_DIRS)
	armlink -o $$@ $$(LDFLAGS) $$(STARTUP) $$(LIBS) $$($(1)_OBJS)
	modbin --name="$(1)" --time --stack=$$(or $$($(1)_STACKSIZE),$$(STACKSIZE)) "$$@" "$$@"
endef

$(foreach app,$(APPS),$(eval $(call APP_TEMPLATE,$(app))))

define LIBRARY_TEMPLATE
$(1)_SRCS_S   := $$(wildcard src/$(1)/*.s)
$(1)_SRCS_C   := $$(wildcard src/$(1)/*.c)
$(1)_SRCS_CXX := $$(wildcard src/$(1)/*.cpp)
$(1)_OBJS     := $$($(1)_SRCS_S:src/%.s=build/%.s.o) $$($(1)_SRCS_C:src/%.c=build/%.c.o) $$($(1)_SRCS_CXX:src/%.cpp=build/%.cpp.o)
OBJS          += $$($(1)_OBJS)

build/$(1)/$(1).lib: $$($(1)_OBJS) | build $$(SUBDIR_BUILD_DIRS)
	$$(ARMLIB) $$(ARMLIBFLAGS) $$@ $$($(1)_OBJS)
endef

$(foreach library,$(LIBRARIES),$(eval $(call LIBRARY_TEMPLATE,$(library))))

OBJS := $(sort $(OBJS))

ifneq ($(strip $(LAUNCHME_OBJS)),)
LAUNCHME_TARGETS = $(LAUNCHME)

$(LAUNCHME): $(LINK_LIBRARY_PATHS) $(LAUNCHME_OBJS) | build $(SUBDIR_BUILD_DIRS)
	armlink -o $@ $(LDFLAGS) $(STARTUP) $(LIBS) $(LAUNCHME_OBJS)
	modbin --name="$(NAME)" --time --stack=$(STACKSIZE) "$@" "$@"
else
LAUNCHME_TARGETS =
endif

DEPS = $(OBJS:.o=.d)

ifneq ($(strip $(wildcard $(FILESYSTEM)/.)),)
IMAGE_TARGETS = launchme programs $(ISONAME)
else
IMAGE_TARGETS =
endif

all: libraries $(IMAGE_TARGETS)

build:
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p build
else
	if not exist "build" mkdir "build"
endif

$(SUBDIR_BUILD_DIRS): | build
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p "$@"
else
	if not exist "$(subst /,\,$@)" mkdir "$(subst /,\,$@)"
endif

builddir: build $(SUBDIR_BUILD_DIRS)

objs: $(OBJS)

ifneq ($(strip $(LAUNCHME_OBJS)),)
launchme: $(LAUNCHME)
else
launchme:
endif

programs: $(PROGRAM_PATHS)

libraries: $(LIBRARY_PATHS)

install: install-libs

install-libs: libraries
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p "$(INSTALL_LIB_DIR)"
	cp $(LIBRARY_PATHS) "$(INSTALL_LIB_DIR)/"
else
	if not exist "$(subst /,\,$(INSTALL_LIB_DIR))" mkdir "$(subst /,\,$(INSTALL_LIB_DIR))"
	for %%f in ($(subst /,\,$(LIBRARY_PATHS))) do copy /Y "%%f" "$(subst /,\,$(INSTALL_LIB_DIR))\"
endif

$(COPY_DATA_STAMP): $(DATA_INPUTS) | build
ifeq ($(IS_POSIX_SHELL),1)
	for d in $(DATA_DIRS); do cp -r "$$d"/* "$(FILESYSTEM)"/; done
	touch $@
else
	for %%d in ($(subst /,\,$(DATA_DIRS))) do xcopy /E /Y "%%d\*" "$(subst /,\,$(FILESYSTEM))"
	type nul > "$(subst /,\,$@)"
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

isodir:
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p "$(ISO_DIR)"
else
	if not exist "$(subst /,\,$(ISO_DIR))" mkdir "$(subst /,\,$(ISO_DIR))"
endif

iso: $(ISONAME)

$(ISONAME): $(LAUNCHME_TARGETS) $(PROGRAM_PATHS) $(COPY_DATA_STAMP) $(FILESYSTEM_INPUTS)
ifeq ($(IS_POSIX_SHELL),1)
	mkdir -p "$(ISO_DIR)"
else
	if not exist "$(subst /,\,$(ISO_DIR))" mkdir "$(subst /,\,$(ISO_DIR))"
endif
	3dt pack "$(FILESYSTEM)" -o "$(ISONAME)"

build/%.s.o: src/%.s | build $(SUBDIR_BUILD_DIRS)
	armasm $(INCFLAGS) $(ASFLAGS) $< -o $@

build/%.c.o: src/%.c | build $(SUBDIR_BUILD_DIRS)
	armcc $(INCFLAGS) $(DEFFLAGS) $(CFLAGS) -M $< -o $@ > ${@:.o=.d}
	armcc $(INCFLAGS) $(DEFFLAGS) $(CFLAGS) -c $< -o $@

build/%.cpp.o: src/%.cpp | build $(SUBDIR_BUILD_DIRS)
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

.PHONY: builddir isodir clean distclean launchme programs libraries install install-libs modbin modbin-launchme banner iso run

-include $(DEPS)
