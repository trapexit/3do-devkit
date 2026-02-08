# About the ARM Linker

The ARM linker, *armlink*, accepts as input:

- One or more separately compiled or assembled object files written in ARM Object Format (see [ARM Object Format](./3arrf.md) for a synopsis, and the ARM Object Format technical specification for details).
- Optionally, one or more object libraries in ARM Object Library Format (see ARM Object Library Format for details).

## Functions

The ARM linker performs the following functions:

- It resolves symbolic references between object files.
- It extracts from object libraries the object modules needed to satisfy otherwise unsatisfied symbolic references.
- It sorts object fragments (AOF areas) according to their attributes and names, and consolidates similarly attributed and named fragments into contiguous chunks.
- It relocates (possibly partially) relocatable values.
- It generates an output *image* possibly comprising several files (or a partially linked object file instead).

## Output Formats

The ARM linker can produce output in any of the following formats:

- **ARM Object Format** (see [ARM Object Format](./3arrf.md) for a synopsis, and the ARM Object Format technical specification for details).
- **Plain binary format**, relocated to a fixed address (see [Plain Binary Format](./3arrg.md)).
- **ARM Image Format** (see [ARM Image Format](./3arrh.md), and also the ARM Image Format technical specification).
- **Extended Intellec Hex Format**, suitable for driving the Compass integrated circuit design tools (see [Extended Intellec Hex Format (IHF)](./3arri.md)).
- **ARM Shared Library Format**: a read-only position-independent reentrant sharable code segment (or *shared library*), written as a plain binary file, together with a *stub* containing read-write data, entry veneers, etc., written in ARM Object Format. See [ARM Shared Library Format](./3arrj.md) for details.
- **ARM Overlay Format**: a *root segment* written in ARM Image Format, together with a collection of *overlay segments*, each written as a plain binary file. A system of overlays may be static (each segment bound to a fixed address at link time), or dynamic (each segment may be relocated when it is loaded).

For details of how to use the options available on the linker command line, see The ARM Linker (armlink) user guide.
