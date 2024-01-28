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
* trapexit's basic replacement C++ standard library
* Roguewave STL which originally came with ARM C++


### Documentation

* 3DO SDK's "Developer's Documentation Set"
* ARM SDT and ARM C++ docs


### Tooling

* 3it: [trapexit's 3DO Image Tool](https://github.com/trapexit/3it/releases)
* 3dt: [trapexit's 3DO Disc Tool](https://github.com/trapexit/3dt/releases)
* 3ct: [trapexit's 3DO Compression Tool](https://github.com/trapexit/3ct/releases)
* modbin: [trapexit's recreation](https://github.com/trapexit/modbin/releases)
  of the original SDK's tool by the same name
* 3doiso v0.1 by nikk
* 3DOEncrypt v0.6a by Charles Doty
* MakeBanner v1.0b by Charles Doty
* BMPTo3DOCel v0.6a by Charles Doty
* BMPTo3DOImage v1.0b by Charles Doty
* BMPTo3DOAnim v0.6a by Charles Doty

trapexit's tooling may not always be fully up to date in this repo so
double check by visiting the links provided above.


### Examples

* [3DO Portfolio 2.5 Examples](examples/3dosdk/Portfolio%202.5)
* [CEL rotation and zoom demo](src/cel_rotation.cpp)


### Misc

* 3DO "takeme" CDROM base files from Portfolio 2.5 w/ a swapped out
  boot_code from Game Guru



## Usage

### Linux

Make sure you have WINE installed. Some tools are currently only
available on Windows.


#### makefile

1. Get dev kit: `git clone --depth=1 https://github.com/trapexit/3do-devkit`
  * or download: `https://github.com/trapexit/3do-devkit/zipball/master`
  * `wget -O 3do-devkit.zip https://github.com/trapexit/3do-devkit/zipball/master`
1. Enter the directory: `cd 3do-devkit`
1. Source the environment: `source activate-env`
1. Run make: `make`
1. Generates file `helloworld.iso`


### Windows

We need project files for common Windows IDEs.


#### WSL2

Same as Linux


## Media Conversion

The most comprehensive tooling is only available from the original SDK. It can
be used via QEMU base emulation. There are some older tools to convert BMP
to 'image', 'cel', or banners. New tools are in the works.

* http://3dodev.com/software/sdks#prebuilt_qemu_macos_9_vm
* BMPTo3DOCel
* BMPTo3DOImage
* BMPTo3DOAnim
* MakeBanner


## Notes

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
* Create a new banner conversion tool.
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
* 3do-cpplib: https://github.com/trapexit/3do-cpplib
* Portfolio OS: https://github.com/trapexit/portfolio_os
* The 3DO Community Discord: https://discord.com/invite/kvM9cQG
* OpenLara: https://github.com/XProger/OpenLara


## Donations / Sponsorship

If you find 3do-devkit useful please consider supporting its ongoing
development.

https://github.com/trapexit/support
