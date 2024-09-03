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

* 3DO libraries from Portfolio 2.5 (including Lib3DO)
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

* [3DO SDK's "Developer's Documentation Set"](docs/3dosdk)
* [ARM SDT and ARM C++ docs](docs/compilers)

Also found at https://3dodev.com


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

* [3DO Portfolio 2.5 Examples](examples/3dosdk/Portfolio%202.5)
* [CEL rotation and zoom demo](src/cel_rotation.cpp)
* [Example Folio](src/example_folio.cpp)
* [Reading the ROM using svc_mem](src/read_rom.cpp)
* [Finding Items at runtime](src/find_semaphore.cpp)
* [Overwriting existing Folio functions](src/overwrite_folio_func.cpp)


### Misc

* 3DO "takeme" CDROM base files from Portfolio 2.5 w/ a swapped out
  boot_code from Game Guru


## Usage

This setup is intended to be as simple as possible to get going. To
that end it is primarily designed to be used in-place. There is no
install required. Just download, activate environemnt, and build. The
repo isn't so large that you couldn't have multiple instances for
multiple projects but if you need to "install" it then you can set the
environment variables as done in the activate scripts and used in the
Makefile.


### Linux

Make sure you have WINE installed. Some tools are currently only
available on Windows.


#### makefile

* Get dev kit: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
* You can also download a zip file from the GitHub page
* Enter the directory: `cd 3do-devkit`
* Source the environment: `source activate-env`
* Run make: `make`
  * Generates file `helloworld.iso`
* Run in Opera emulator (via RetroArch): `make run`


### Windows

Note: We need project files for common Windows IDEs. If you have an
example project file for your favorite IDE please submit a PR with
it.


#### General

* Get dev kit: [download](https://github.com/trapexit/3do-devkit/archive/refs/heads/master.zip)
* Unzip to wherever you like
* From a terminal (cmd.exe or PowerShell):
  * Enter the directory: `cd 3do-devkit`
  * Source the environment: `activate-env` or `.\activate-env`
  * Run make: `make`
    * Generates file `helloworld.iso`
  * Run in Opera emulator (via RetroArch): `make run`
* From Explorer:
  * Enter the directory
  * Run `make.bat` to build the project
  * Run `make-run.bat` to run the created iso via Opera if RetroArch
    is installed


#### WSL2

Same as Linux


### TDO_DEVKIT_PATH

This environment variable is set by `activate-env` on all platforms
and can be used to have a separate directory from the dev kit for your
`src`, `takeme`, and other files.


## Media Conversion

### images

`3it` is a pretty comprehensive tool supporting conversion to and from
CELs, IMAGs, Banners, etc. Older tools are included for completeness
but should not be needed.

### audio

`ffmpeg` can be used to convert files to `AIFF` files but to compress
them you will need to use the original software for now. See below.

SDX2 encoding is not currently supported by `ffmpeg` but work is being
done to build an encoder.


#### uncompressed AIFF signed 16bit bigendian

```
ffmpeg -i input.file -ar 22050 -c:a pcm_s16be output.aiff"
```


#### uncompressed AIFF signed 8bit

```
ffmpeg -i input.file -ar 22050 -c:a pcm_s8 output.aiff"
```


#### uncompressed raw signed 16bit bigendian

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

Same as audio. Cinepak creation by `ffmpeg` is not compatible with the
3DO provided decoder. https://3dodev.com has a tutorial.

http://3dodev.com/software/sdks#prebuilt_qemu_macos_9_vm


## Notes

### sysload

Uncomment the line in `takeme/AppStartup` regarding `sysload` to add a
CPU and DSP resource overlay to your app.


### ARM C++ 1.11 compiler

* ARM C++ 1.11 is a pre-standard compiler. From section 3.10 in the ref guide:
  * Exceptions are NOT supported
  * Namespaces are NOT supported
  * RTTI is only partially suported
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
