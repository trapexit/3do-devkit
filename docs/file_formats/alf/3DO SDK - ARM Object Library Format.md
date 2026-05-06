# ARM Object Library Format

This document defines a file format called *ARM Object Library
Format*, or *ALF*, which is used by the ARM linker, *armlink*, and the
ARM object librarian, *armlib*.

# About the library format

A library file contains a number of separate but related pieces of
data.  In order to simplify access to these data, and to provide for a
degree of extensibility, the library file format is itself layered on
another format called *Chunk File Format*. This provides a simple and
efficient means of accessing and updating distinct chunks of data
within a single file. For a description of the Chunk File Format, see
[Chunk file
format](https://ext.3dodev.com/3DO/Portfolio_2.5/OnLineDoc/DevDocs/tktfldr/atsfldr/2atsb.html#XREF31035).

The Library format defines four chunk classes: Directory, Time stamp,
Version and Data. There may be many Data chunks in a library.

The Object Library Format defines two additional chunks: Symbol table
and Symbol table time stamp.

These chunks are described in detail later in this document.


## Terminology

The terms *byte, half word, word,* and *string* are used to mean:

| Type      | Description |
| --------- | ----------- |
| byte      | 8 bits, considered unsigned unless otherwise stated, usually used to store flag bits or characters. |
| half word | 16 bits, or 2 bytes, usually considered unsigned. |
| word      | 32 bits, or 4 bytes, usually considered unsigned. |
| string    | A sequence of bytes terminated by a NUL (0x00) byte. The NUL byte is part of the string but is not counted in the string's length. |


## Byte sex or endian-ness

There are two sorts of ALF: *little-endian* and *big-endian**.*

- In little-endian ALF, the least significant byte of a word or
  half-word has the lowest address of any byte in the
  (half-)word. Used by DEC, Intel and Acorn, amongst others.
- In big-endian ALF, the most significant byte of a (half-)word has
  the lowest address. This byte sex is used by IBM, Motorola and
  Apple, amongst others.

For data in a file, address means offset from the start of the file.

There is no guarantee that the endian-ness of an ALF file will be the
same as the endian-ness of the system used to process it, (the
endian-ness of the file is always the same as the endian-ness of the
target ARM system).

The two sorts of ALF cannot, meaningfully, be mixed (the target system
cannot have mixed endian-ness: it must have one or the other). Thus
the ARM linker will accept inputs of either sex and produce an output
of the same sex, but will reject inputs of mixed endian-ness.


## Alignment

Strings and bytes may be aligned on any byte boundary.

ALF fields defined in this document do not use half-words, and align
words on 4-byte boundaries.

Within the contents of an ALF file (within the data contained in
OBJ_AREA chunks - see below), the alignment of words and half-words is
defined by the use to which ALF is being put. For all current
ARM-based systems, alignment is strict, as described immediately
above.


# Library File Format

For library files, the first part of each chunk's name is "LIB_"; for
object libraries, the names of the additional two chunks begin with
"OFL_".

Each piece of a library file is stored in a separate, identifiable
chunk, named as follows:

| Chunk        | Chunk Name                            |
| ------------ | ------------------------------------- |
| Directory    | LIB_DIRY                              |
| Time stamp   | LIB_TIME                              |
| Version      | LIB_VSRN                              |
| Data         | LIB_DATA                              |
| Symbol table | OFL_SYMT (object code libraries only) |
| Time stamp   | OFL_TIME (object code libraries only) |

There may be many LIB_DATA chunks in a library, one for each library
member. In all chunks, word values are stored with the same byte order
as the target system; strings are stored in ascending address order,
which is independent of target byte order.

## LIB_DIRY

The LIB_DIRY chunk contains a directory of the modules in the library,
each of which is stored in a LIB_DATA chunk. The directory size is
fixed when the library is created. The directory consists of a
sequence of variable length entries, each an integral number of words
long. The number of directory entries is determined by the size of the
LIB_DIRY chunk.

| ChunkIndex  | Description                                                    |
| ----------- | -------------------------------------------------------------- |
| EntryLength | The size of this LIB_DIRY chunk (an integral number of words). |
| DataLength  | The size of the Data (an integral number of words).            |
| Data        |                                                                |

ChunkIndex is a word containing the 0-origin index within the chunk
 file header of the corresponding LIB_DATA chunk. Conventionally, the
 first 3 chunks of an OFL file are LIB_DIRY, LIB_TIME and LIB_VSRN, so
 *ChunkIndex* is at least 3. A ChunkIndex of 0 means the directory
 entry is unused.

The corresponding LIB_DATA chunk entry gives the offset and size of
the library module in the library file.

EntryLength is a word containing the number of bytes in this LIB_DIRY
entry, always a multiple of 4.

DataLength is a word containing the number of bytes used in the data
section of this LIB_DIRY entry, also a multiple of 4.

The Data section consists of, in order:

- a 0-terminated string (the name of the library member);
- any other information relevant to the library module (often empty);
- a 2-word, word-aligned time stamp.

Strings should contain only ISO-8859 non-control characters (codes
[0-31], 127 and 128+[0-31] are excluded).

The string field is the name used to identify this library module.
Typically it is the name of the file from which the library member was
created.

The format of the time stamp is described in [Time
stamps](https://ext.3dodev.com/3DO/Portfolio_2.5/OnLineDoc/DevDocs/tktfldr/atsfldr/3atsb.html#XREF15659). Its
value is an encoded version of the last-modified time of the file from
which the library member was created.

To ensure maximum robustness with respect to earlier, now obsolete,
versions of the ARM object library format:

- Applications which create libraries or library members should ensure
  that the LIB_DIRY entries they create contain valid time stamps.
- Applications which read LIB_DIRY entries should not rely on any data
  beyond the end of the name string being present, unless the
  difference between the DataLength field and the name-string length
  allows for it. Even then, the contents of a time stamp should be
  treated cautiously and not assumed to be sensible.

Applications which write LIB_DIRY or OFL_SYMT entries should ensure
that padding is done with NUL (0) bytes; applications which read
LIB_DIRY or OFL_SYMT entries should make no assumptions about the
values of padding bytes beyond the first, string-terminating NUL byte.


## Time stamps

A library time stamp is a pair of words encoding the following:

- a 6-byte count of centi-seconds since the start of the 20th century;
- a 2-byte count of microseconds since the last centi-second (usually 0).

The first word stores the most significant 4 bytes of the 6-byte
count; the least significant 2 bytes of the count are in the most
significant half of the second word.

The least significant half of the second word contains the microsecond
count and is usually 0.

Time stamp words are stored in target system byte order: they must
have the same *endianness* as the containing chunk file.


## LIB_TIME

The LIB_TIME chunk contains a 2-word time stamp recording when the
library was last modified. It is, hence, 8 bytes long.


## LIB_VSRN

The version chunk contains a single word whose value is 1.


## LIB_DATA

A LIB_DATA chunk contains one of the library members indexed by the
LIB_DIRY chunk. The endian-ness or byte order of this data is, by
assumption, the same as the byte order of the containing library/chunk
file.

No other interpretation is placed on the contents of a member by the
library management tools. A member could itself be a file in chunk
file format or even another library.


# Object code libraries

An object code library is a library file whose members are files in
ARM Object Format (see [ARM Object
Format](https://ext.3dodev.com/3DO/Portfolio_2.5/OnLineDoc/DevDocs/tktfldr/atsfldr/ats2frst.html#XREF16187)
for details).

An object code library contains two additional chunks: an external
symbol table chunk named OFL_SYMT; and a time stamp chunk named
OFL_TIME.


## OFL_SYMT

The external symbol table contains an entry for each external symbol
defined by members of the library, together with the index of the
chunk containing the member defining that symbol.

The OFL_SYMT chunk has exactly the same format as the LIB_DIRY chunk
except that the Data section of each entry contains only a string, the
name of an external symbol, and between 1 and 4 bytes of NUL padding,
as follows:

| ChunkIndex           | Description                                                                     |
| -------------------- | ------------------------------------------------------------------------------- |
| EntryLength          | The size of this OFL_SYMT chunk (an integral number of words).                  |
| DataLength           | The size of the External Symbol Name and Padding (an integral number of words). |
| External Symbol Name |                                                                                 |
| Padding              |                                                                                 |

OFL_SYMT entries do not contain time stamps.


## OFL_TIME

The OFL_TIME chunk records when the OFL_SYMT chunk was last modified
and has the same format as the LIB_TIME chunk (see [Time
stamps](https://ext.3dodev.com/3DO/Portfolio_2.5/OnLineDoc/DevDocs/tktfldr/atsfldr/3atsb.html#XREF15659)).
