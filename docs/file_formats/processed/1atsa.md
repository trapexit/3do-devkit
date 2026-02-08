# Properties of ARM Image Format

Two variants of AIF exist:

- **Executable AIF** (in which the header is part of the image itself) can be executed by entering the header at its first word. Code in the header ensures the image is properly prepared for execution before being entered at its entry address.
- **Non-executable AIF** (in which the header is not part of the image, but merely describes it) is intended to be loaded by a program which interprets the header, and prepares the following image for execution.

The two variants of AIF are distinguished as follows:

- The fourth word of an executable AIF header is `BL entrypoint`. The most significant byte of this word (in the target byte order) is `0xEB`.
- The fourth word of a non-executable AIF image is the offset of its entry point from its base address. The most significant nibble of this word (in the target byte order) is `0x0`.

The base address of an executable AIF image is the address at which its header should be loaded; its code starts at *base* + `0x80`. The base address of a non-executable AIF image is the address at which its code should be loaded.

The remarks in the following subsection about executable AIF apply also to non-executable AIF, except that loader code must interpret the AIF header and perform any required uncompression, relocation, and creation of zero-initialised data. Compression and relocation are, of course, optional: AIF is often used to describe very simple absolute images.

## Executable AIF

It is assumed that on entry to a program in ARM Image Format (AIF), the general registers contain nothing of value to the program (the program is expected to communicate with its operating environment using SWI instructions or by calling functions at known, fixed addresses).

A program image in ARM Image Format is loaded into memory at its load address, and entered at its first word. The load address may be:

- An implicit property of the type of the file containing the image (as is usual with Unix executable file types, Acorn Absolute file types, etc.)
- Read by the program loader from offset `0x28` in the file containing the AIF image
- Given by some other means, e.g. by instructing an operating system or debugger to load the image at a specified address in memory

An AIF image may be compressed and can be self-decompressing (to support faster loading from slow peripherals, and better use of space in ROMs and delivery media such as floppy discs). An AIF image is compressed by a separate utility which adds self-decompression code and data tables to it.

If created with appropriate linker options, an AIF image may relocate itself at load time. Two kinds of self-relocation are supported:

- **Relocate to load address** - the image can be loaded anywhere and will execute where loaded
- **Self-move up memory** - leaving a fixed amount of workspace above, and relocate to this address (the image is loaded at a low address and will move to the highest address which leaves the required workspace free before executing there)

The second kind of self-relocation can only be used if the target system supports an operating system or monitor call which returns the address of the top of available memory. The ARM linker provides a simple mechanism for using a modified version of the self-move code illustrated in [Self-move and self-relocation code](1atsc.md#self-move-and-self-relocation-code), allowing AIF to be easily tailored to new environments.

AIF images support being debugged by the ARM Symbolic Debugger (armsd). Low-level and source-level support are orthogonal, and both, either, or neither kind of debugging support need be present in an AIF image.

Details of the format of the debugging tables are not available in this 3DO edition of this manual.

References from debugging tables to code and data are in the form of relocatable addresses. After loading an image at its load address these values are effectively absolute. References between debugger table entries are in the form of offsets from the beginning of the debugging data area. Thus, following relocation of a whole image, the debugging data area itself is position independent and may be copied or moved by the debugger.
