# About the Library Format

A library file contains a number of separate but related pieces of data. In order to simplify access to these data, and to provide for a degree of extensibility, the library file format is itself layered on another format called *Chunk File Format*. This provides a simple and efficient means of accessing and updating distinct chunks of data within a single file. For a description of the Chunk File Format, see [Chunk File Format](2atsb.md#chunk-file-format).

The Library format defines four chunk classes: Directory, Time stamp, Version and Data. There may be many Data chunks in a library.

The Object Library Format defines two additional chunks: Symbol table and Symbol table time stamp.

These chunks are described in detail later in this document.

## Terminology

The terms *byte*, *half word*, *word*, and *string* are used to mean:

| Term      | Definition                                                                                                      |
|-----------|-----------------------------------------------------------------------------------------------------------------|
| byte      | 8 bits, considered unsigned unless otherwise stated, usually used to store flag bits or characters               |
| half word | 16 bits, or 2 bytes, usually considered unsigned                                                                |
| word      | 32 bits, or 4 bytes, usually considered unsigned                                                                |
| string    | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL byte is part of the string but is not counted in the string's length |

## Byte Sex or Endian-ness

There are two sorts of ALF: *little-endian* and *big-endian*.

- In **little-endian** ALF, the least significant byte of a word or half-word has the lowest address of any byte in the (half-)word. Used by DEC, Intel and Acorn, amongst others.
- In **big-endian** ALF, the most significant byte of a (half-)word has the lowest address. This byte sex is used by IBM, Motorola and Apple, amongst others.

For data in a file, address means offset from the start of the file.

There is no guarantee that the endian-ness of an ALF file will be the same as the endian-ness of the system used to process it (the endian-ness of the file is always the same as the endian-ness of the target ARM system).

The two sorts of ALF cannot, meaningfully, be mixed (the target system cannot have mixed endian-ness: it must have one or the other). Thus the ARM linker will accept inputs of either sex and produce an output of the same sex, but will reject inputs of mixed endian-ness.

## Alignment

Strings and bytes may be aligned on any byte boundary.

ALF fields defined in this document do not use half-words, and align words on 4-byte boundaries.

Within the contents of an ALF file (within the data contained in OBJ_AREA chunks - see below), the alignment of words and half-words is defined by the use to which ALF is being put. For all current ARM-based systems, alignment is strict, as described immediately above.
