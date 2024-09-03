This is a 3DO Opera (M1) development environment. It is built from a number of
components both new and old. The hope is that by making setup more turnkey and
bringing documentation and examples into one location it will help facilitate
more homebrew on the 3DO.

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
* Roguewave STL which originally came with ARM C++ (probably not worth
  using)


### Documentation

It is *strongly* suggested that new developers start with the original
3DO SDK documentation. While the layout is imperfect the documentation
is reasonably thorough. The 3DO uses a high level operating system
that abstracts the hardware and provides many features such as
semaphores, threading, message passing, signals, etc. An initial focus
on the [OS
APIs](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/pgsfldr/00pgs)
and how
[graphics](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/ggsfldr/00ggs)
work on the console is suggested.

* [3DO SDK's "Developer's Documentation Set"](https://3dodev.com/documentation/development_documents)
  * Also found in the repo in the original form: [docs/3dosdk](docs/3dosdk)
* [Dev Notes and Gotchas](https://3dodev.com/documentation/software/opera/portfolio_os/notes_and_gotchas)
* [ARM SDT and ARM C++ docs](docs/compilers)

More can be found at https://3dodev.com


### Tooling

* [3it](https://github.com/trapexit/3it/releases): trapexit's 3DO
  Image Tool
* [3dt](https://github.com/trapexit/3dt/releases): trapexit's 3DO Disc
  Tool
* [3ct](https://github.com/trapexit/3ct/releases): trapexit's 3DO
  Compression Tool
* [modbin](https://github.com/trapexit/modbin/releases): trapexit's
  recreation of the original SDK's tool by the same name
* 3doiso v0.1 by nikk
* 3DOEncrypt v0.6a by Charles Doty
* MakeBanner v1.0b by Charles Doty
* BMPTo3DOCel v0.6a by Charles Doty
* BMPTo3DOImage v1.0b by Charles Doty
* BMPTo3DOAnim v0.6a by Charles Doty
* [GNU make for Windows](https://gnuwin32.sourceforge.net/packages/make.htm)

trapexit's tooling may not always be fully up to date in this repo so
double check by visiting the links provided above.


### Examples

* [3DO Portfolio 2.5 Examples](examples/3dosdk/Portfolio%202.5): These
  examples do not yet have makefiles to build them using the DevKit
  but porting them is trivial.
* [CEL rotation and zoom demo](src/cel_rotation.cpp)
* [Example Folio](src/example_folio.cpp)
* [Reading the ROM using svc_mem](src/read_rom.cpp)
* [Finding Items at runtime](src/find_semaphore.cpp)
* [Overwriting existing Folio functions](src/overwrite_folio_func.cpp)


### Misc

* 3DO "takeme" CDROM base files from Portfolio 2.5 w/ a swapped out
  boot_code from Game Guru.


## Install

There really isn't a need to "install". You can simply download the
repo and start using it in place. See [#Usage](#Usage) below. However,
you can set things up in a manner similar to a global install.


### Windows

* [Download](https://github.com/trapexit/3do-devkit/archive/refs/heads/master.zip)
the dev kit. 
* Uncompress and move the folder into its final location.
* Run `buildtools\setup-env.bat`
* This will setup the appropriate enviroment variables at a global
level. This removes the need to use `activate-env.bat` or
`activate-env.ps1` each terminal session.


### Linux

* Download: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
* Add `source /path/to/3do-devkit/activate-env` to your shell config file.


## Usage

This setup is intended to be as simple as possible to get going. To
that end it is primarily designed to be used in-place. There is no
install required. Just download, activate environment, and build. 

### General

* Modify `Makefile` to change the project `NAME`. Not much else should
  need to be modified in the `Makefile` for basic usage.
* The makefile handles assembly source files (`*.s`), C files (`*.c`),
  and C++ files (`*.cpp`) in the root of `src/`.
* Add and/or remove files from `src/` as needed for your project.
* Run `make` to build object files, link executable, build ISO, and
  sign ISO for retail use.
  
See below for OS specific workflows.


### Windows

#### General

* [Download](https://github.com/trapexit/3do-devkit/archive/refs/heads/master.zip)
* Uncomppress and move the folder into its final location
* From a terminal (cmd.exe or PowerShell):
  * Enter the directory: `cd 3do-devkit`
  * Source the environment: `activate-env` or `.\activate-env`
  * Run make: `make`
    * Generates `iso\helloworld.iso`
  * Run in RetroArch Opera emulator (if installed): `make run`
* From Explorer:
  * Enter the directory
  * Run `make.bat` to build the project
  * Run `make-run.bat` to run the created iso via Opera if RetroArch
    is installed


#### WSL2

Same as Linux


### Linux

Make sure you have WINE installed. Some tools are currently only
available on Windows.


#### makefile

* Download: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
* Enter the directory: `cd 3do-devkit`
* Source the environment: `source activate-env`
* Run make: `make`
  * Generates `iso/helloworld.iso`
  * Run in RetroArch Opera emulator (if installed): `make run`


## Bootstrapping a New Project

If you have "installed" the dev kit as described above you can create
code and asset repositories in different directories which will use
the binaries found in the "install" location.

To simplify the creation of these there are the `bootstrap-project`
scripts. One for Windows and one Linux.


### Windows

* Run `buildtools\setup-env.bat` (if not done already) to setup paths globally
* Make a directory for your project
* Copy `buildtools\bootstrap-project.bat` to that directory
* Run `bootstrap-project.bat` from within that directory

This will copy all relevant files into the path and can use it the
same as described above. Feel free to remove `bootstrap-project.bat`
afterwards.


### Linux

* Source `activate-env` as described above
* Make a directory for your project
* Copy `buildtools/bootstrap-project` to that directory 
* Run `bootstrap-project` from within that directory
* You can also simply run `buildtools/bootstrap-project PATH`

This will copy all relevant files into the path and can use it the
same as described above. Feel free to remove `bootstrap-project`
afterwards.


## RetroArch Opera Setup

`make run` will launch the Opera core of RetroArch if installed and
run the built ISO. RetroArch can be found at https://retroarch.com but
you will need ROMs for Opera to work correctly.

The script `buildtools/download-retroarch-opera-roms` will attempt to
download required ROMs to the "system" directory of RetroArch if
already installed. On Windows or Linux simply launch from a file
explorer or a terminal.

NOTE: The scripts look in known directories to copy the ROMs to but it
is possible you have a different setup. You can just go to the
[site](https://3dodev.com) and download the ROMs manually and place
them into the RetroArch `system` folder.


## DevKit Layout

* bin/: All core binaries such as compilers, linkers, and media
  conversion tools.
* buildtools/: Misc tools to setup the build environment.
* src/: Directory storing all source.
* takeme/: Directory storing all CDROM artifacts. Target for final
  "Launchme" executable. The name "takeme" originates from the
  original 3DO SDK.
* art/: 3DO artwork. Currently only original 3DO SDK art.
* docs/: Misc documentation from the original SDK and compiler suites.
* include/: All include files from original SDK and community projects.
* lib/: All libraries from original SDK and community projects.
* examples/: Primarily examples from all available original 3DO SDK
  releases.
* build/: Automatically created directory during build to store object
  files.
* iso/: Automatically created directory during build to store ISO file.


## Media Conversion

### images

`3it` is a comprehensive tool supporting conversion to and from CELs,
IMAGs, Banners, etc. Older tools are included for completeness but
should not be needed.

Read more about CEL formats at:
* https://3dodev.com/documentation/file_formats/media/image/cel
* https://3dodev.com/documentation/file_formats/media/container/3do


### audio

[ffmpeg](https://ffmpeg.org) can be used to convert files to a couple
3DO compatible formats but not all. ffmpeg does not currently have a
SDX2 encoder (only decoder) leaving only the original MacOS software
capable of encoding the format.

SDX2 compresses the audio to 8bits per sample and according to the
original author, Phil Burk, sounds noticeably better than using raw
8bit samples.


#### uncompressed AIFF signed 16bit bigendian

```
ffmpeg -i input.file -ar 22050 -c:a pcm_s16be output.aiff"
```


#### uncompressed AIFF signed 8bit

```
ffmpeg -i input.file -ar 22050 -c:a pcm_s8 output.aiff"
```


#### uncompressed raw signed 16bit bigendian

Raw files can be useful if you want to create multiple samples at
runtime from the same file.

```
ffmpeg -i input.file -ar 22050 -f s16be -acodec pcm_s16be output.raw"
```


#### uncompressed raw signed 8bit

```
ffmpeg -i input.file -ar 22050 -f s8 -acodec pcm_s8 output.raw"
```


#### compressed 4bit ADPCM IMA WS

Make sure you have a more recent version of ffmpeg to use `adpcm_ima_ws`

```
ffmpeg -i input.file -c:a adpcm_ima_ws output.aifc
```


### video

A Cinepak library was included in the original SDK. Unfortunately,
`ffmpeg` does not support the 3DO Stream container format nor does the
Cinepak encoder generate frames which align properly for the 3DO.

Till a new decoder is written or `ffmpeg` modified to provide proper
alignment you will need to use original Classic MacOS software.

* https://3dodev.com/tutorials/trapexit/creating_3do_compatible_fmv
* http://3dodev.com/software/sdks#prebuilt_qemu_macos_9_vm


## Notes

### sysload

Uncomment the line in `takeme/AppStartup` regarding `sysload` to add a
CPU and DSP resource overlay to your app.


### Development Notes and Gotchas

https://3dodev.com/documentation/software/opera/portfolio_os/notes_and_gotchas


### ARM C++ 1.11 compiler

* ARM C++ 1.11 is a pre-standard compiler. From section 3.10 in the ref guide:
  * Exceptions are NOT supported
  * Namespaces are NOT supported
  * RTTI is only partially supported
  * C++ style casting is only partially supported
  * While technically mostly supported templates can be buggy and complex
    usage may crash the compiler


### Memory Management

ARM C++ 1.11 treats POD (plain old data) and objects differently so POD must be
expressly handled/ignored when worrying about object destruction. There are no
default destructors for objects either meaning it is not possible to simulate a
'placement delete' to force destruction. obj->~OBJ() will not work unless
expressly defined. As a result it is difficult to write generic template
based data types. To work around this the STL like library provided removed
memory freeing from 'delete' to force its use as a placement delete. Meaning
that when wanting to free data from 'new' one should use `memory_delete(ptr)`.
That said best to use or create higher level objects to manage such things.
Simple versions of shared_ptr and unique_ptr are provided.

There might be a better solution to this problem but after many attempts,
including attempting to use the strategy by RogueWave which didn't seem
to work, this was settled on till something better could be be done.


## TODO

* Project files for popular IDEs.
* Continue to enhance the C++ standard library replacement.
* C++ based 3DO specific libraries.
* Create a new iso building tool.
* Create a new iso encrypt tool.
* More examples.
* Better compiler support. Possibly use CLANG or GCC to generate assembly and
  translate it to work with Norcroft armasm.
* More languages? If you have a ARMv3 compatible compiler and AIF linker
  (even if just different versions of Norcroft compilers) please reach out.
* A version of the Opera emulator tailored for development work?


## Thanks

* @ArmSoftwareDev on Twitter and Arm Support: for providing me
with copies of ARM SDT 2.51 and ARM C++ 1.11. After reaching out to former
Norcroft employees, software archivists, and even Bjarne Stroustrup without
success finding a copy of ARM C++ from the 3DO era I reached out to ARM
directly and they were able to find some copies and offered them to me for
this project.
* everyone at The 3DO Community Discord
* XProger (author of OpenLara) who was the first project to use this dev kit


## Links

* 3DO Development Repo: https://3dodev.com
* 3do-devkit: https://github.com/trapexit/3do-devkit
* Portfolio OS: https://github.com/trapexit/portfolio_os
* The 3DO Community Discord: https://discord.com/invite/kvM9cQG
* OpenLara: https://github.com/XProger/OpenLara


## Donations / Sponsorship

If you find 3do-devkit useful please consider supporting its ongoing
development.

https://github.com/trapexit/support
