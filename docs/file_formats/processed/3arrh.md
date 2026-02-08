# ARM Image Format

At its simplest, a file in ARM Image Format (AIF) is a plain binary image preceded by a small (128 byte) header which describes what follows.

At its most sophisticated, AIF can be considered to be a collection of envelopes which enwrap a plain binary image, as follows:

- **Compression**: The outer wrapper allows the inner layers to be compressed using any compression algorithm to which you have access which supports efficient decompression at image load time, either by the loader or by the loaded image itself. In particular, AIF defines a simple structure for images which decompress themselves, consisting of: AIF header; compressed contents; decompression tables; decompression code.
- **Relocation**: The next layer of wrapping deals with relocating the image to its load address. Three options are supported: link-time relocation; load-time relocation to whatever address the image has been loaded at; load-time relocation to a fixed offset from the top of memory. In particular, an AIF image is capable of self-relocation or self-location (to the high address end of memory), followed by self-relocation.
- **Zero-initialisation**: Once an AIF image has been decompressed and relocated, it can create its own zero-initialised area.
- **Entry**: Finally, the enwrapped image is entered at the (unique) entry point found by the linker in the set of input object modules.

## AIF Flavours

Two flavours of AIF are supported:

- **Executable AIF**: Can be loaded at its load address and entered at the same point (at the first word of the AIF header). It prepares itself for execution by relocating itself, zeroing its own 0-initialised data, etc.
- **Non-executable AIF**: Must be processed by an image loader, which loads the image at its load address and prepares it for execution as detailed in the AIF header. The header is then discarded.

An executable AIF image is loaded at its load address (which may be arbitrary if the image is relocatable), and entered at the same address. Eventually, control passes to a branch to the image's entry point.

## Requirements

In order to produce an AIF output there must be:

- No unresolved symbolic references between the input objects (each reference must resolve directly or via an input library).
- Exactly one input object containing a program entry point (or no input area containing an entry point, and the entry point given using an `-Entry` option).
- Either an absolute load address or the relocatable option given to the linker (the self-location option is system-dependent).

## Customisation

The AIF header is specified fully in the Layout of AIF technical specification.

The contents of some fields of the AIF header (such as the program exit instruction) can be customised by providing a template for the header in an area with the name `AIF_HDR` and a length of 128 bytes, in the *first* object module in the list of object modules given to *armlink*.

Similarly, the self-move and self-relocation code appended by the linker to a relocatable AIF image can be customised by providing an area with the name `AIF_RELOC`, also in the *first* object module in the input list.
