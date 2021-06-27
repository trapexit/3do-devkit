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

* MakeBanner v1.0b by Charles Doty
* 3doiso v0.1 by nikk
* 3DOEncrypt v0.6a by Charles Doty
* modbin: [trapexit's recreation](https://github.com/trapexit/modbin) of the original SDK's tool by the same name
* 3dt: [trapexit's 3DO Disc Tool](https://github.com/trapexit/3dt)


### Examples

* 3DO Portfolio 2.5 Examples
* CEL rotation and zoom demo


### Misc

* 3DO "takeme" CDROM base files from Portfolio 2.5 w/ a swapped out boot_code from Game Guru


## Usage

### Linux

Make sure you have WINE installed. Some tools are currently only available on Windows.

#### makefile

1. Get dev kit: `git clone https://github.com/trapexit/3do-devkit`
1. Enter the directory: `cd 3do-devkit`
1. Source the environment: `source activate-env`
1. Run make: `make`
1. Generates file `helloworld.iso`


### Windows

We need project files for common Windows IDEs.

#### WSL2

Same as Linux


## Notes

* ARM C++ 1.11 is a pre-standard compiler. From section 3.10 in the ref guide:
  * Exceptions are NOT supported
  * Namespaces are NOT supported
  * RTTI is only partially suported
  * C++ style casting is only partially supported
  * While technically mostly supported templates can be buggy and complex
    usage may crash the compiler


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

* 3do-devkit: https://github.com/trapexit/3do-devkit
* 3DO Development Repo: https://3dodev.com
* The 3DO Community Discord: https://discord.com/invite/kvM9cQG
* OpenLara: https://github.com/XProger/OpenLara
