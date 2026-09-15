This is a 3DO Opera development environment for Linux and Window. It
is built from a number of components both new and old. The hope is
that by making setup more turnkey and bringing documentation and
examples into one location it will help facilitate more homebrew on
the 3DO.


## Components

### Compilers

* Norcroft ARM C v4.91 (ARM Ltd SDT2.51) [Build number 130] for Linux
* Norcroft ARM C v4.91 (ARM Ltd SDT2.51) [Build number 128] for Windows
* Norcroft ARM C++ v0.61/v4.91 (ARM Ltd C++1.11) [Build number 130] for Linux
* Norcroft ARM C++ v0.61/v4.91 (ARM Ltd C++1.11) [Build number 128] for Windows


### Libraries

* Original 3DO libraries from Portfolio 2.5 SDK
* [cpplib](https://github.com/trapexit/3do-cpplib): basic replacement
  C++ standard library
* [svc_funcs](https://github.com/trapexit/3do-svc-funcs): provides
  access to kernel functions while in supervisor mode
* [svc_mem](https://github.com/trapexit/3do-svc-mem-device): example
  device driver and its library
* [example_folio](https://github.com/trapexit/3do-example-folio):
  example Folio (shared library)
* ttl: trapexit's template library (simplistic replacement to STL)


### Documentation

It is *strongly* suggested that new developers start with the original
3DO SDK documentation. While the layout is imperfect the documentation
is reasonably thorough. The 3DO uses a high level operating system
that abstracts the hardware and provides many features such as
semaphores, threading, message passing, signals, etc. An initial focus
on the [OS
APIs](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/pgsfldr/00pgs)
and how
[graphics](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/programming_3do_graphics)
work on the console is suggested.

* [3DO SDK's "Developer's Documentation Set"](https://3dodev.com/documentation/development_documents)
  * Also found in the repo in the original form: [docs/3dosdk](docs/3dosdk)
* [ARM60 datasheet](docs/cpu)
* [ARM SDT and ARM C++ docs](docs/compilers)
* [Development notes](docs/devnotes/README.md): Collection of notes
  collected from the homebrew community.

More can be found at https://3dodev.com


### Tooling

* [3it](https://github.com/trapexit/3it/releases): trapexit's 3DO
  Image Tool
* [3at](https://github.com/trapexit/3at/releases): trapexit's 3DO
  Audio Tool
* [3dt](https://github.com/trapexit/3dt/releases): trapexit's 3DO Disc
  Tool
* [3ct](https://github.com/trapexit/3ct/releases): trapexit's 3DO
  Compression Tool
* [modbin](https://github.com/trapexit/modbin/releases): trapexit's
  recreation of the original SDK's tool by the same name
* [GNU make for Windows](https://gnuwin32.sourceforge.net/packages/make.htm)

trapexit's tooling may not always be fully up to date in this repo so
double check by visiting the links provided above.


### Examples

* [README](examples/README) for more information.
* [Community Examples](examples/community): A collection of examples
  from the 3DO homebrew community.
* [Reworked Examples](examples/reworked): A collection of examples reworked from
  the original 3DO SDKs to work out of the box with this 3DO DevKit.
* [Original Examples](examples/original): These are all the examples
  from the original 3DO Portfolio and Toolkit releases in their
  original form (outside text encoding conversion.) They are not
  buildable without some effort.
* [src/main.cpp](src/main.cpp): The built-in LaunchMe menu for
  selecting and launching the bundled example apps in `src/`.

The main `src/` tree includes a runnable example disc menu plus
standalone apps under `src/<app>/main.*`. Each app is linked as its
own executable and launched from the menu with `LoadProgram()`.

| App | Source | Description |
|-----|--------|-------------|
| 3VX Player | [src/3vxplayer](src/3vxplayer) | Directory-driven video menu with NTSC 15/20/30-fps comparisons and end-of-clip profiling. |
| Hello, World! | [src/helloworld](src/helloworld) | Minimal C example that logs and draws centered text. |
| Cel rotation | [src/cel_rotation](src/cel_rotation) | C++ cel rotation and zoom demo. |
| 3D 3DO logo | [src/3d_3do_logo](src/3d_3do_logo) | 3D logo rendering demo. |
| Rotating cube | [src/rotating_cube](src/rotating_cube) | Simple rotating 3D cube demo. |
| Bounce | [src/bounce](src/bounce) | 3DO logo components bouncing in a room with animation and sound. |
| Orbit | [src/orbit](src/orbit) | Animated 3DO logo elements orbiting over a background with sound effects. |
| Color Echo | [src/colorecho](src/colorecho) | Recursive video feedback and color pattern demo. |
| 24-bit Slide Show | [src/slide_show_24bit](src/slide_show_24bit) | Slideshow comparing 3DO 24-bit image and VDL modes. |
| AA Player | [src/aaplayer](src/aaplayer) | Anti-aliased animation player with frame and cel controls. |
| LR Extract | [src/lrex](src/lrex) | Sub-cel extraction and frame buffer redraw example. |
| NVRAM | [src/nvram](src/nvram) | NVRAM file create, read, write, and delete example. |
| Messages | [src/msgpassing](src/msgpassing) | Kernel message passing example. |
| File Walker | [src/walker](src/walker) | File system traversal example. |
| FILE API | [src/file_api](src/file_api) | Writes and reads a small NVRAM file through `fopen`, `fputs`, `fwrite`, `fflush`, `fread`, and `fclose`. |
| Font Library | [src/fontlibexample](src/fontlibexample) | Font loading and text rendering example. |
| Drum Box | [src/drumbox](src/drumbox) | Interactive audio drum sequencer example. |


### Misc

* 3DO "takeme" CDROM base files from Portfolio 2.5 w/ a swapped out
  boot_code from Game Guru.


## Download and Install

There really isn't a need to "install". You can download the repo and
start using it in place. See [#Usage](#Usage) below. However, you can
set things up in a manner similar to a global install.


### Windows

Note: If using WSL or MSYS you can follow the Linux instructions.

* [Download](https://github.com/trapexit/3do-devkit/archive/refs/heads/master.zip)
the dev kit.
* Uncompress and move the folder into its final location.
* Run `bin\buildtools\win\setup-3do-devkit-env.bat`
* This will setup the appropriate environment variables at a global
level. This removes the need to use `activate-env.bat` or
`activate-env.ps1` each terminal session.

Note: the devkit includes `make.bat` and `make-run.bat` which need no
activation and can be run from Explorer.


### Linux

* Download: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
* Add `source /path/to/3do-devkit/activate-env` to your shell config file.


## Usage

This setup is intended to be as simple as possible to start using. To
that end it is primarily designed to be used in-place. There is no
install required. Just download, activate environment, and build.

### General

* Modify `Makefile` to change the project `NAME`. Not much else should
  need to be modified in the `Makefile` for basic usage.
* See [Makefile and src/ Layout](#makefile-and-src-layout) for how
  source files, apps, libraries, and app data are discovered.
* The included [src/main.cpp](src/main.cpp) program is a LaunchMe menu
  that starts the bundled app executables with `LoadProgram()`.
* Run `make` to build object files, link executable, build ISO, and
  sign ISO for retail system usage.

See below for OS specific workflows.


### Makefile and src/ Layout

The root `Makefile` is designed around a small set of conventions in
the `src/` tree. Most projects should only need to change the variables
at the top of the file, especially `NAME`, `ISONAME`, `FILESYSTEM`, and
`STACKSIZE`.

Source files in the root of `src/` are treated as the boot program. The
Makefile compiles root-level `src/*.s`, `src/*.c`, and `src/*.cpp`
files into `build/*.o`, then links them into `takeme/LaunchMe`. In this
repo that program is the LaunchMe menu in [src/main.cpp](src/main.cpp).

Each `src/<name>/` directory that contains `main.c`, `main.cpp`, or
`main.s` is treated as a standalone app. All assembly, C, and C++
sources in that directory are compiled into `build/<name>/`, then
linked into an executable named `takeme/<name>`. The directory name is
the app name, so adding `src/myapp/main.c` creates the `takeme/myapp`
build target. If the app should appear in the bundled menu, also add it
to [src/main.cpp](src/main.cpp).

Each `src/<name>/` directory that contains source files but no
`main.*` file is treated as a static library directory. Its sources are
compiled into `build/<name>/` and archived with `armlib` as
`build/<name>/<name>.lib`. These libraries are build artifacts; add a
library to `LIBS` if an app or the boot program should link against it.

Optional app data belongs under `src/<name>/takeme/`. The `copy-data`
target copies the contents of each of those directories into `takeme/`
before the ISO is packed, preserving the same disc filesystem layout
the app will see at runtime.

The default `all` target always runs `libraries`. When the configured
`FILESYSTEM` directory exists, it also runs `launchme`, `programs`,
`modbin`, and `iso`. In order, that archives static libraries, builds
the boot program, links each standalone app, applies `modbin` metadata
to executables, and runs `3dt pack --sign` to create the signed ISO.


### Windows

#### General

* [Download](https://github.com/trapexit/3do-devkit/archive/refs/heads/master.zip)
* Uncompress and move the folder into its final location
* From a terminal (cmd.exe or PowerShell):
  * Enter the directory: `cd 3do-devkit`
  * Source the environment: `activate-env` (CMD) or `.\activate-env` (PowerShell)
  * Run make: `make`
    * Generates `iso\helloworld.iso`
  * Run in RetroArch Opera emulator (if installed): `make run`
* From Explorer:
  * Enter the directory
  * Run `make.bat` to build the project
  * Run `make-run.bat` to run the created iso via Opera if RetroArch
    is installed
  * Run `make-clean.bat` to remove build files


#### WSL

Same as Linux


#### MSYS

Same as Linux (when in the MSYS bash shell).


### Linux

The ARM compile suite binaries are x86 32bit. Be sure to install 32bit
support for your distro. On Debian / Ubuntu based distros that would
be:

```console
sudo dpkg --add-architecture i386
sudo apt-get update
sudo apt-get install libc6:i386
```

Also ensure you have WINE installed. Some tools are currently only
available on Windows.


#### makefile

* Download: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
* Enter the directory: `cd 3do-devkit`
* Source the environment: `source activate-env`
* Run make: `make`
  * Generates `iso/helloworld.iso`
  * Run in RetroArch Opera emulator (if installed): `make run`
* Run `make clean` to clean up build files


## Bootstrapping a New Project

If you have "installed" the dev kit as described above or activated
the environment you can easily copy code, assets, and build tools into
their own directories which will use the binaries found in the
"install" location.

To simplify the creation of these there is the `bootstrap-3do-project`
script.


### Windows

* Run `setup-3do-devkit-env.bat` (if not done already) to setup paths
  globally or `activate-env`
* Make a directory for your project
* Change the directory to your new project path
* Run `bootstrap-3do-project.bat`
* Or copy `bootstrap-3do-project.bat` to that directory and run it
  (from shell or Explorer)

This will copy all relevant files into the path and can be used the
same as described above. Feel free to remove
`bootstrap-3do-project.bat` afterwards.


### Linux

* Source `activate-env` as described above
* Make a directory for your project
* Run `bootstrap-3do-project` from within that directory
* You can also run `bootstrap-3do-project PATH`


## RetroArch Opera Setup

`make run` will launch the Opera core of RetroArch if installed and
run the built ISO. RetroArch can be found at https://retroarch.com but
you will need ROMs for Opera to work correctly.

The script `download-retroarch-opera-roms` will attempt to download
required ROMs to the "system" directory of RetroArch if already
installed. On Windows or Linux launch from a file explorer or a
terminal.

NOTE: The scripts look in known directories to copy the ROMs to but it
is possible you have a different setup. You can just go to the
[site](https://3dodev.com/software/roms) and download the ROMs
manually and place them into the RetroArch `system` folder.

* https://3dodev.com/_media/roms/panafz1.bin
* https://3dodev.com/_media/roms/panafz1j-kanji.bin


## DevKit Layout

* [bin/](bin/): All core binaries such as compilers, linkers, media
  conversion tools, and misc tooling.
* [src/](src/): Directory storing application source. Root-level
  sources build into LaunchMe, while `src/<name>/main.*` directories
  build into standalone executable programs for the disc.
* takeme/: Directory storing all CDROM artifacts. Target for final
  "Launchme" executable, standalone app executables, and copied app
  data. The name "takeme" originates from the original 3DO SDK.
* [art/](art/): 3DO artwork. Currently only original 3DO SDK art.
* [docs/](docs/): Documentation from the original SDK, compiler
  suites, related hardware, and development notes.
* [include/](include/): All include files from original SDK and
  community projects.
   * 3do: The main headers for Portfolio, Lib3DO, and other original
     SDK libraries. Cleaned and restructured for 3DO-DevKit.
   * 3dosdk: The original headers from different SDKS. Unused. For
     reference.
   * community: Headers for any included modern community libraries.
   * ttl: "trapexit's template library". A simple work in progress
     replacement for C++ STL.
* lib/: All libraries from original SDK and community projects.
* [examples/](examples/): Examples from all available original 3DO SDK
  releases as well as new and reworked examples. Some of them also
  exist in [src/](src/).
* build/: Automatically created directory during build to store object
  files and generated static libraries.
* iso/: Automatically created directory during build to store ISO file.


## Media Conversion

### images

[3it](https://github.com/trapexit/3it) is a comprehensive tool
supporting conversion to and from CELs, IMAGs, Banners, etc. Older
tools are included for completeness but should not be needed.

Read more about CEL formats at:
* https://3dodev.com/documentation/file_formats/media/image/cel
* https://3dodev.com/documentation/file_formats/media/container/3do


### audio

The 3DO can [handle multiple sample rates, sample sizes, channels, and
codecs.](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/mgsfldr/mpgfldr/03mpg004)

SDX2 compresses the audio to 8bits per sample and according to the
original author, Phil Burk, sounds noticeably better than using raw
8bit samples. Intel/DVI ADP4 compresses down to 4 bits per sample but
only allows for mono audio and is notably worse sounding compared to
SDX2. SDX2 will typically be best for music and ADP4 for sound effects.

[ffmpeg](https://ffmpeg.org) can be used to convert files to a couple
raw audio formats which 3DO can use however ffmpeg does not currently
have a SDX2 encoder (only decoder) or a Intel/DVI ADP4 encoder or
decoder (though adpcm_ima_ws is similar.)

[trapexit's 3at](https://github.com/trapexit/3at) can be used to
convert to and from SDX2 and Intel/DVI ADP4.


### video

A Cinepak library was included in the original SDK. Unfortunately,
`ffmpeg` does not support the 3DO Stream container format nor does the
Cinepak encoder generate frames which align properly for the 3DO.

For a modern custom-decoder path, `src/3vxplayer/` contains a PortfolioOS
3VX player adapted from the sibling `3vt/player` sources. It streams video
from CD into a persistent LR-form framebuffer, uses ARM60 block painters
and triple-buffered CEL presentation, and plays SDX2 stereo through the
Portfolio sound spooler. It does not use the legacy Cinepak subscriber.

Build its bootable disc from the devkit root:

```sh
source activate-env
make 3vxplayer-iso
```

The result is `iso/3vxplayer.iso`. The normal `make` also includes one
**3VX Player** entry in the top-level menu (`iso/helloworld.iso`).
The executable `takeme/3vxplayer` scans `$boot/3vxplayer_data` for `.3vx`
files, sorts their names, and displays a scrolling file-selection menu.
Place additional videos in `src/3vxplayer/takeme/3vxplayer_data/` and rebuild;
both disc targets include the directory contents without changing source.

Included files are `hackers-15.3vx` (15000/1001 fps), `hackers-20.3vx`
(20000/1001 fps), and `video.3vx` (the original 30000/1001-fps trailer).
All retain 320x240 video and 22050 Hz stereo audio at normal speed.
Up/Down selects a file; A or Start plays it. During playback Start pauses
or resumes and X returns to the file menu. At EOF the profiling summary
remains visible: A replays with fresh counters, X returns to file selection.
The player and its screens remain alive between videos; no reboot or
relaunch is needed to compare them.

The release-optimized summary reports decode/draw mean and maximum times,
decode p99 in 5 ms buckets, over-budget decode calls, drops/skips, recovery
entries and late submissions. It now separates drops immediately after
decoding (D) from drops after drawing, at presentation (P), including counts
within one NTSC field of the first rejected audio-sample boundary.
`Draws / rows` reports actual CEL draw calls and total pixel rows copied,
including images later discarded. Frames whose target screen is already
current need no draw. Draw timing includes staging bookkeeping for those frames.
`Late 0/1/2` and `Late 3+` bucket submissions by fields beyond their phase
deadline, using the fresh timer sample taken at the presentation decision.
`Gaps` buckets intervals between successive submissions in fields (ideal:
four at 15 fps, three at 20 fps, two at 30 fps). `max` is the longest gap.
These measure submission timing, NOT actual VDL latch or scanout cadence.
Intentional pauses start a new gap/streak segment and are excluded from gap
statistics. `Phase start/reset` counts phase acquisition and discard resets;
pause invalidation is not a discard reset.

To keep the screen readable, the print channel carries the complete gap and
lateness histograms (bin 8 means 8+ fields), drop excess/streak details,
hidden-audio drops, stage-wait maximum (including pauses/stalls), field-clock
age, eight slow decode indices and the decode histogram. Timing uses a
dedicated GetUSecTime IOReq; clock
overhead is displayed, not subtracted. Photograph the summary on an NTSC
console for hardware comparisons. Opera timings are not measurements of
physical CEL or CD performance.

Phase acquisition is relative to the prepared frame's media time: a frame
already trailing audio does not receive another fresh two-field delay.
The lateness/drop thresholds and decoder remain unchanged. In a controlled
Opera test adding an approximately 11.35 ms draw delay, this correction
reduced 30-fps trailer drops from 1888 to zero. Adding 25% decode delay as
well reduced drops from 4095 to 212. These artificial delays isolate the
scheduling mechanism; they do not simulate all hardware contention or prove
console performance. Evidence is in `build/3vx-scheduling/`.

A subsequent bounded-acceptance experiment retained the presentation
deadline while admitting later decoded images. With the same synthetic
11.35 ms draw time and 25% decode delay, the current gate dropped 203 frames
with a maximum 15-field submission gap. Extensions of approximately 5 ms,
16.6 ms, and one video frame instead dropped 253/407/873 frames, with maximum
gaps of 28/52/164 fields. All were rejected: extra staging work delayed
subsequent frames and increased post-draw losses. Only the cadence
diagnostics were retained; acceptance thresholds remain unchanged.
Full receipts and variant source are in `build/3vx-acceptance/`.

The decoder now batches full/range codebook loads: aligned V4 updates copy
their already-native words in one operation, and V1 expansion hoists shape
and alignment decisions out of the entry loop. Unaligned byte-load fallbacks
remain. A stats-free ARMv3 VEC interpreter keeps command/row state across
runs and paints inline, eliminating per-run C/assembly call overhead.
There is one decoder API: `vx_dec_frame(dec, payload, bytes)`. It always
uses the optimized validated-input path, with no checked variant, mode flag
or per-command statistics. The ARM VEC loop omits input-length/run-bound
checks and requires nonempty complete grids, sufficient indices and
row-contained runs. Malformed VEC input may overwrite memory or hang.
Chunk/codebook framing, prediction state and runtime I/O/ownership checks
remain. Compile-time layout checks bind assembly offsets to VxDec.

Actual big-endian ARM execution was compared against the previous compiled
decoder for every frame of all three movies, including framebuffer and
codebook state. An additional 16000 VEC cases exercised valid, truncated,
cross-row and unaligned streams, and 6000 codebook cases checked malformed
sizes/ranges and alignments. Callee-saved registers and stack preservation
were checked. Host fixtures, the original 300-frame pixel manifest and all
five encoder byte anchors passed. The full 30-fps Opera decode mean/max
changed from 12268/32768 us to 7507/22016 us; codebook batching alone measured
11610/30272 us. These are emulator timings, not promised console speedups.
Scheduling, media, and quality settings are unchanged. Evidence is under
`build/3vx-decoder-opt/`.

The subsequent trusted-asset pass removed redundant VEC checks, kept the V4
table base in a register, replaced repeat-count stack traffic with a register,
and tested V4 literal-loop unrolling. Mean 30-fps Opera decode time changed
from about 7506 us to 7152 us (trusted/register changes), 7105 us (two-block
unroll) and 7080 us (retained four-block unroll). These are measured changes,
not a universal performance optimum or physical-console prediction.
Validate new files offline with `tools/verify_3vx_stream.py` in 3vt before
adding them to the player's data directory. Validation is a separate tool,
not a runtime decoder mode. The former checked entry, statistics structure,
C-to-ARM run dispatch and superseded painter assembly have been removed.
The host build implements the same unchecked VEC contract portably for
pixel verification; the 3DO build uses only the optimized ARM interpreter.
Historical trusted/checked comparison evidence is under `build/3vx-trusted/`;
single-API cutover evidence is under `build/3vx-single/`.

The normal player now renders only the conservative dirty vertical band of
each destination screen. The decoder records the first/end block rows that
contain coded (non-skip) commands while executing the existing VEC stream.
The player unions those bounds separately for all three screen buffers,
including decoded-but-dropped frames, and clears a buffer's bounds only
after staging into it. Keyframes and playback restarts invalidate every
screen fully. This preserves the existing version-1 file format and all
bundled videos; no private metadata or separate experimental player is needed.
The measured dispatch improvements are also enabled: common opcode classes
are tested before V4-repeat, and literal-loop exits branch directly to row
advance. The single decoder and scheduler thresholds are unchanged.

Harness verification matched all 5191 decoded framebuffers and codebooks,
33 normal movie captures and 626 matched post-stall captures against the
full-frame renderer exactly. Randomized decode/stage/presentation drops
also preserved destination-screen pixels. All three full videos presented
every frame, with working replay and file-menu return. The 30-fps movie
copied 728224 pixel rows rather than 1245840 (41.5% fewer). Runtime row
tracking adds about 0.1 ms per frame in Opera versus the previous decoder;
the hardware Draw-time saving is the performance gate. Evidence for the
normal-player integration is in `build/3vx-main-integration/`.

The next cost pass keeps the coded-row flag in the unused high bit of the
remaining-row register, derives row width from the resident stride, unrolls
V4 repeat stores, and uses duplicated values in distinct registers for V1
burst stores (never duplicate registers in an STM list). Sparse codebook
loads hoist type/alignment decisions and inline entry conversion; aligned
V1 batch expansion is unrolled. A dedicated band CCB removes full-CCB
reinitialization after every draw while preserving the timing boundaries.
No format, media, quality, scheduling or recovery-threshold changes result.

Matched Opera measurements: 30-fps trailer decode mean/max 7172/21600 us
before, 6796/18592 us after; repeated dense-keyframe workload 16904/17728 us
before, 12713/13584 us after. Full ARM framebuffer/codebook/dirty-bound/ABI
comparisons passed on all three movies plus 12000 valid geometry/run cases.
All-rate playback, 26 matched rendering captures, replay/file-menu return,
pause and 500 ms recovery passed. A native pre-expanded V1 wire format was
rejected: it added 621048 bytes for only 21 us average decode saving. Packed
chunk-header reads gave no established useful gain and were not retained.
These timing figures are emulator measurements; they do not prove an
absolute minimum or guarantee arbitrary-content hardware playback. Current
hardware baseline is 8945/32240 us decode and 6942/13712 us draw, with all
5191 frames presented and no reported visual artifacts. The new stack's
hardware gain remains unmeasured. Evidence: `build/3vx-mincost/receipt.json`.

A subsequent full review of the player and decoder retained four changes.
V4 literal commands, 37.6 percent of all commands in the sample stream,
now reach their handler in two class tests instead of three via a range
test; each coded row loads the dirty-bounds base from a previously dead
stack slot instead of recomputing it; and `finish_read` re-reads the
request's `io_Error` after a non-negative wait, matching Portfolio's own
`LoadFile`, so a device read error wedges instead of advancing the window
over bytes the drive never delivered.

The dispatch and stack-slot changes alone traded 31 us of mean decode time
for 16 us on the worst frame; a fourth change removed that tail cost.
`dirty_first` is now written only on the first coded row, latched by bit 30
of the row-count register, rather than re-derived with a load, compare and
conditional store on every coded row; rows are visited in increasing order,
so the first coded row is already the minimum. Interleaved A/B/A/B against a
pristine worktree at the previous commit, with zero microseconds of
within-arm scatter across repeated runs of either build, puts Opera 30-fps
decode at 6796/18608 us before and 6764/18608 us after: the mean improves
and the worst frame is back to the baseline maximum. The read-error check is
not a performance change.

The decode maximum is a keyframe cost and is not reachable from the decoder.
All 218 keyframes code every one of the 4800 blocks and carry about 500
codebook entries, while the worst delta frame codes 3072. A cycle sketch
puts V4 literals at 78 percent of keyframe paint cost, but every keyframe
must write 4800 times 32 equals 153600 framebuffer bytes whatever the block
mode, so the tail is dominated by mandatory memory traffic rather than
dispatch or addressing overhead. Reordering the V4 literal index loads ahead
of the codebook loads measured exactly neutral on both mean and maximum:
this part has no cache and does not reorder loads, so only the number of
accesses matters. Lowering the maximum requires changing what a keyframe has
to write, which is an encoder or format decision.

The rendering side is likewise closed at this geometry. `CCB_BGND` already
selects the opaque path by disabling the transparency test, `PMODE_ONE`
selects a plain source copy, and because the cel engine takes `ccb_HDX >> 4`
the configured `1 << 20` is exactly unity scale, so the presentation cel
already qualifies for the unscaled line-copy path. Hardware cost is 47.1 us
per row full-frame and 49.3 us per row banded, so the draw is linear in rows
copied with negligible per-call overhead, and dirty-band rendering already
captured the available saving.

The same review rejected five candidates on measurement rather than
judgement: replacing the aligned V4 codebook `memcpy` with wide-register
copies (6847 us, slower than the SDK routine); consolidating the per-field
audio-clock reads (no gain, and the call also drives spooler completion
processing); expanding the resident V1 codebook to eight words per entry in
paint order, which removes every duplication `mov` from both V1 painters
but measured 6788/18992 us against 6765/18624 because the extra 4 KiB of
resident table costs more in codebook-load traffic than the saved
instructions recover; drawing exact per-row spans instead of one band
(724180 versus 728224 rows, a 0.56 percent difference); and wider V4 index
loads, which need a format change because only 25.2 percent of index groups
are word-aligned in real streams.

Known remaining gaps, none implemented: a single transient CD read error
still ends the movie, where a bounded re-issue of the same request would
recover; `vx_audio_fill` still runs while paused and inflates its
`no_free` counter; the first read is a full 128 KiB, which lengthens
boot-to-first-frame; and with a zero-height block grid the host decoder
no-ops while the ARM interpreter loops, a divergence outside the
documented input contract and unreachable from the encoder. Evidence and
the full census are in `build/3vx-review2/receipt.json`.

The dedicated ISO stages under `build/3vxplayer-disc/` and does not replace
the shared `takeme/LaunchMe`. An optional sample generator creates FFmpeg's
300-frame test pattern with 440/660 Hz stereo tones. The following commands
**replace `video.3vx` with that test pattern**, leaving other videos intact:

```sh
make 3vxplayer-sample
make 3vxplayer-iso
```

`python3 src/3vxplayer/generate_sample.py --encoder /path/to/3vt` supports
another encoder location. FFmpeg generates BGR24/PCM AVI in
`build/3vxplayer-media/sample.avi`; 3vt encodes quality 100 with the quality
preset, three-frame codebook planning, and a 45-frame keyframe interval.
The encoded asset is retained in `src/3vxplayer/takeme/3vxplayer_data/`.

This sample is 1,939,164 bytes, averages 6,463.6 media bytes/frame, and
passes the 3vt validator and full 300-frame host decode. Its measured
0.25/0.5/1/2-second burst sizes are 63,976/110,496/207,420/400,248 bytes,
all below the validator's 2x-CD budgets. Opera verification covered three
complete loops, changing video frames, zero reported decode errors,
440/660 Hz audio, pause/resume, and restart. Playback follows the actual
audio DMA byte cursor, not a nominal 240 Hz timer or counted VBL waits.
The exact 147147-samples/200-frames ratio eliminates cumulative timing
roundoff. Pause and starvation freeze media time. More than three frames
of video lag triggers discard-through-keyframe recovery, with at most two
decodes and 32 queue scans per service call. The last good screen is held
until a current independent keyframe is available; audio is never skipped
to make video catch up. A drained audio pipeline behind a full video queue
also releases video slots rather than deadlocking. Late decodes need not
be presented; stream cadence is not a guarantee of 300 screen presents.

The supported header limit is 450000 frames (4 h 10 min 15 s), subject to
the signed-32-bit BlockFile size limit. DMA FIFO read-ahead and display
refresh introduce bounded timing uncertainty. Physical-console testing
has not been performed; arbitrary IO failures or insufficient decode
throughput cannot guarantee uninterrupted video. Both this example and
the sibling 3vt player contain the same timing/recovery implementation.
Generation validates frame/audio capacities, decode cost and all four CD
burst windows before publishing the asset. Source-matched testing of this
sample selected quality/q100 (41.5634 dB PSNR) over quality/q90 (41.5167)
and exhaustive/q100 (41.5633). The differences are small; this is the best
measured candidate for this sample, not a universal optimum for movies.

For the original SDK's legacy workflow:

* https://3dodev.com/tutorials/trapexit/creating_3do_compatible_fmv
* https://3dodev.com/software/sdks#prebuilt_qemu_macos_9_vm


## TODO

* Project files for popular IDEs.
* Rework libc. Add missing functions.
* Rebuild original privileged libraries and enhance them where possible.
* Continue to enhance the C++ standard library replacement.
* C++ based 3DO specific libraries.
* More examples.
* Better compiler support. Possibly use CLANG or GCC to generate assembly and
  translate it to work with Norcroft armasm.
* More languages? If you have a ARMv3 compatible compiler and AIF linker
  (even if just different versions of Norcroft compilers) please reach out.
* A version of the Opera emulator tailored for development work?


## FAQ

### Why not use a more modern compiler suite?

The 3DO is an early ARMv3 based system which used ARM's Norcroft
compiler, relocatable AIF executable format, AOF object format,
etc. Support for these formats were abandoned by ARM around the time
the console was retired and the limited open source compiler suite
support for these formats were removed long long ago. Support for
ARMv3 era CPUs have also been removed.

trapexit has spoken with John Fitch of Codemist and ARM regarding
licensing or open sourcing their ARM compiler suite as well as
individuals who maintain the compiler suite for RISCOS. Unfortunately,
none were able to help. The latter having narrowly scoped licensing
with no ability to sublicense.

Adding ARMv3 support to a modern compiler might not be so hard given
ARMv4 is still supported by but adding AIF and AOF to modern linkers
would likely be a decent amount of work and maintenance. There are
other ideas ranging from an ELF to relocatable AIF converter to
transpiling ARMv4 assembly to `armasm` compatible assembly.


## Thanks

* @ArmSoftwareDev on Twitter and Arm Support: for providing me with
copies of ARM SDT 2.51 and ARM C++ 1.11. After reaching out to former
Norcroft employees, software archivists, and even Bjarne Stroustrup
without success finding a copy of ARM C++ from the 3DO era I reached
out to ARM directly and they were able to find some copies and offered
them to me for this project.
* Everyone at The 3DO Community Discord.
* XProger: author of 3DO OpenLara and Wipeout prototype ports.
* Shaun 3DOHD: author of REAL 3DO Tetris and MK2 3DO Edition.
* zyzix: author of Biofury, JinglesDefense, REAL Tempest, and the Matinicus engine.


## Links

* 3DO Development Repo: https://3dodev.com
* 3do-devkit: https://github.com/trapexit/3do-devkit
* Portfolio OS: https://github.com/trapexit/portfolio_os
* The 3DO Community Discord: https://discord.com/invite/kvM9cQG


## Donations / Sponsorship

If you find 3do-devkit and the surrounding tooling useful please
consider supporting its ongoing development.

https://github.com/trapexit/support