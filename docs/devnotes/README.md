# 3DO Development Notes

## Hardware

### CPU

* CPU within the 3DO is an
  [ARM60](../cpu/arm60_datasheet_-_zarlink_semiconductor.pdf) of the
  ARMv3 architecture.
* 12.5Mhz, ~10MIPS, big endian, 16 32bit registers, 32bit data bus,
32bit address bus
* No integer division instructions, uses C library functions
`__rt_sdiv` and `__rt_udiv` for signed and unsigned division
respectively. Can take 20-140 cycles. See
[armdbccg](https://github.com/trapexit/armdbccg) for optimized
code to divide by known values.
* No floating point hardware.
* No 16bit integer instructions. Reads/writes to memory will be more
  expensive.
* Registers are 32bit so best to avoid using `char` and `short` for
local variables. Otherwise will need to sign-extend or zero-extend.
* Data should be word aligned (4 bytes). See the data sheet for
  details on how unaligned loads/stores behave.
* Function calls with more than 4 arguments will store additional
arguments on the stack.
* There are *NO* data or instruction caches on the ARM60.
* [Writing Efficient C for ARM](https://3dodev.com/_media/documentation/hardware/writing_efficient_c_for_arm.pdf)
* [More ARM60
details](https://3dodev.com/documentation/hardware/opera/arm)


### Memory

* 2MiB DRAM
* 1MiB VRAM
* Different parts of the system have limits on what RAM they can
  use. See the developer docs for specific functions for details.


### Audio

* No access to Redbook audio from Portfolio.
* If you want CD quality audio you need to stream from a file on the
  filesystem.
* You will read details about a "free running" mode for the DSP in the
  original developer docs but no such ability was made available in
  the original SDK. 
* There is currently no tooling to write DSP programs. There are
  remnants of a PForth based assembler in the Portfolio M2 repo but
  has not been reworked for modern systems.


### CEL Engine

* Only supports quads. No distinction between texture and polygon
  (no UV.)
* Can not run in parallel with the CPU.
* Call `DrawCels()` or `DrawScreenCels()` as few times as
  possible. Instead chain together CELs and call draw once if possible.
* `DrawCels()` is faster than `DrawScreenCels()`.
* Larger CELs are more expensive to draw than smaller even if scaled
  up. Use LODs / manual mipmaps if possible.
* Avoid VRAM as a source for CELs. Reduces performance.
* `MapCel()` is slower than `FastMapCel()` but the latter has
  limitation.
* No Z-buffer. Need to sort CELs for painters algo yourself.
* Uses forward rasteriation. Performance is directly related to CEL
  data size.
* No hardware mip-mapping.
* Does support backface culling and clipping.


### Math Engine

* Part of the MADAM chip. Memory mapped fixed point matrix arithmetic
  mostly. Technically asynchronous but practically can not run in
  parallel to the CPU.
* Wrapped by API due to different hardware support on different
  hardware variants.
* Supports arrays as input which should be used to reduce library
  call overhead.


## Portfolio OS

* [Portfolio](https://github.com/trapexit/portfolio_os) is a high
  level, preemptive multitasking operating system developed by the 3DO
  Company.
* Portfolio, leveraging a MMU within MADAM (referred to as memory
  fences), restricts direct access to hardware by the user. Direct
  hardware access requires a privileged driver.
* Usage of the Portfolio OS is currently *not* optional. An entire new
  OS and hardware abstractions would need to be created to replace it.
* Portfolio supports multiple processes and threading.
* Shared libraries are
  [supported](https://github.com/trapexit/3do-example-folio),
  including privileged calls, however multiprocess development is rare
  so of limited use except for getting privileged access for
  interacting with hardware.
* `CreateBitmap()` has a bug in it where it sets REGCTL1 (which
  controls the clipping width and height) to the bitmap width and
  height rather than the clip values if provided. When/if we rebuild
  the library this can be fixed.


### Programming Do's and Don'ts

https://3dodev.com/documentation/development/opera/pf25/ppgfldr/smmfldr/cdmfldr/07cdm002

* Don't assume a certain data delivery rate from an IO request.
* When using `DoIO()` be sure to check both the return value and on
  success `io_req->io_Error`.
* Don't alternate reads, rapidly, between two or more files. This will
  almost certainly require seeking and impose serious delay in IO
  requests.
* Issue read requests in 32KiB or larger chunks when possible.
* Use adequate buffering to survive a rotation of the CDROM (up to 150
  milliseconds).
* Use multiplex data streams rather than multiple open files.
* Concat files into a single file and split in memory to improve
  loading time.
* The current ISO builder app is not sophisticated as the original and
  have some bugs but plans exist to build a new one.


### Programming Etiquette

https://3dodev.com/documentation/development/opera/pf25/ppgfldr/smmfldr/gspfldr/06pgstoc

* [Don't Leave Unused Fields in DMA Structures
  Uninitialized](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/smmfldr/gspfldr/06pgs008)


### Threading

* As mentioned in the
  [docs](https://3dodev.com/documentation/development/opera/pf25/ppgfldr/pgsfldr/spg/02spg004)
  you must open any Devices, Drivers, Folios, etc. before using them
  in a thread. While memory may be shared between Tasks and Threads it
  does not appear Items share ownership or exist within one's scope.
* While it may appear that threads can't take arguments due to the
  CreateThread signature you can in fact pass two values via the
  CREATETASK_TAG_ARGC and CREATETASK_TAG_ARGV tags via
  CreateItem/CreateItemVA.
* When using CREATETASK_TAG_SP the ta_Arg value should be set to the
  bottom of the stack. malloc(stacksize) + stacksize.


## sysload

Uncomment the line in `takeme/AppStartup` regarding `sysload` to add a
CPU and DSP resource overlay to your app.


## ARM C 4.91 compiler

* ANSI C compatible
* Do *NOT* use `<>` angle brackets for `#include`ing headers as
  `armcc` will use builtin variants intended for ARM's libc.
* [User Guide](../compilers/usrguide.pdf)
* [Reference Guide](../compilers/refguide.pdf)


## C Standard Library (libc)

* Custom 3DO created (and community tweaked) libc.
* Not a complete library.
* Missing `FILE` based filesystem functions.
* Missing `scanf` family of functions.
* Plans exist to add missing features and generally improve the libc.


## ARM C++ 1.11 compiler

* ARM C++ 1.11 is a pre-standard compiler. From section 3.10 in the ref guide:
  * Exceptions are NOT supported
  * Namespaces are NOT supported
  * RTTI is only partially supported
  * C++ style casting is only partially supported
  * While technically mostly supported templates can be buggy and complex
    usage may crash the compiler
* [User Guide](../compilers/usrguide.pdf)
* [Reference Guide](../compilers/refguide.pdf)


## C++ Standard Template Library

* A version of RogueWave's STL was originally included but found to be
  troublesome to use and occasionally broken.
* `ttl` Trapexit's Template Library was created to replace it but is
  incomplete.


## Memory Management

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
