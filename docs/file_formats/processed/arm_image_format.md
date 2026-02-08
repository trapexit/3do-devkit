# ARM Image Format

**ARM DUI 0041C** — Copyright © 1997 and 1998 ARM Limited. All rights reserved.

---

## Chapter 13: ARM Image Format

This chapter describes the ARM Image Format (AIF). It contains the following sections:

- Overview of the ARM Image Format
- AIF variants
- The layout of AIF

---

## 13.1 Overview of the ARM Image Format

ARM Image Format (AIF) is a simple format for ARM executable images, consisting of:

- a 128-byte header
- the image code
- the image initialized static data.

An AIF image is capable of self-relocation if it is created with the appropriate linker options. The image can be loaded anywhere and it will execute where it is loaded. After an AIF image has been relocated, it can create its own zero-initialized area. Finally, the image is entered at the unique entry point.

---

## 13.2 AIF Variants

There are three variants of AIF:

### Executable AIF

Executable AIF can be loaded at its load address and entered at the same point (at the first word of the AIF header). It prepares itself for execution by relocating itself if required and setting to zero its own zero-initialized data.

The header is part of the image itself. Code in the header ensures that the image is properly prepared for execution before being entered at its entry address.

The fourth word of an executable AIF header is:

```
BL entrypoint
```

The most significant byte of this word (in the target byte order) is `0xeb`.

The base address of an executable AIF image is the address at which its header should be loaded. Its code starts at `base + 0x80`.

### Non-executable AIF

Non-executable AIF must be processed by an image loader that loads the image at its load address and prepares it for execution as detailed in the AIF header. The header is then discarded. The header is not part of the image, it only describes the image.

The fourth word of a non-executable AIF image is the offset of its entry point from its base address. The most significant nibble of this word (in the target byte order) is `0x0`.

The base address of a non-executable AIF image is the address at which its code should be loaded.

### Extended AIF

Extended AIF is a special type of non-executable AIF. It contains a scatter-loaded image. It has an AIF header that points to a chain of load region descriptors within the file. The image loader should place each region at the location in memory specified by the load region descriptor.

---

## 13.3 The Layout of AIF

This section describes the layout of AIF images.

### 13.3.1 AIF Image Layout

An AIF image has the following layout:

- Header
- Read-only area
- Read-write area
- Debugging data (optional)
- Self-relocation code (position-independent)
- Relocation list — a list of byte offsets from the beginning of the AIF header, of words to be relocated, followed by a word containing `-1`. The relocation of non-word values is not supported.

> **Note:** An AIF image is restartable if, and only if, the program it contains is restartable (an AIF image is not reentrant). Following self-relocation, the second word of the header must be reset to NOP. This causes no additional problems with the read-only nature of the code section.

On systems with memory protection, the self-relocation code must be bracketed by system calls to change the access status of the read-only section (first to writable, then back to read-only).

### 13.3.2 Debugging Data

After the execution of the self-relocation code, or if the image is not self-relocating, the image has the following layout:

- Header
- Read-only area
- Read-write area
- Debugging data (optional)

AIF images support being debugged by an ARM debugger. Low-level and source-level support are orthogonal. An AIF image can have both, either, or neither kind of debugging support.

References from debugging tables to code and data are in the form of relocatable addresses. After loading an image at its load address these values are effectively absolute.

References between debugger table entries are in the form of offsets from the beginning of the debugging data area. Following relocation of a whole image, the debugging data area itself is position-independent and may be copied or moved by the debugger.

### 13.3.3 AIF Header

The following table shows the layout of the AIF header.

> **Note:** In all cases, NOP is encoded as `MOV r0,r0`.

| Offset | Field | Description |
|--------|-------|-------------|
| `0x00` | NOP | — |
| `0x04` | `BL SelfRelocCode` | NOP if the image is not self-relocating. |
| `0x08` | `BL ZeroInit` | NOP if the image has none. |
| `0x0C` | `BL ImageEntryPoint` or `EntryPoint Offset` | BL to make the header addressable via r14 (the application will not return). Non-executable AIF uses an offset, not BL. BL is used to make the header addressable via r14 in a position-independent manner, and to ensure that the header will be position-independent. |
| `0x10` | Program Exit Instruction | Last attempt in case of return. Usually a SWI causing program termination. On systems that do not implement a SWI for this purpose, a branch-to-self is recommended. Applications are expected to exit directly and not to return to the AIF header. The ARM linker sets this field to `SWI 0x11` by default, but it may be set to any desired value by providing a template for the AIF header in an area called `AIF_HDR` in the first object file in the input list to `armlink`. |
| `0x14` | Image ReadOnly Size | Includes the size of the AIF header only if the AIF type is executable (i.e., if the header itself is part of the image). |
| `0x18` | Image ReadWrite Size | Exact size (a multiple of 4 bytes). |
| `0x1C` | Image Debug Size | Exact size (a multiple of 4 bytes). Includes high-level and low-level debug size. Bits 0–3 hold the type. Bits 4–31 hold the low level debug size. |
| `0x20` | Image Zero-Init Size | Exact size (a multiple of 4 bytes). |
| `0x24` | Image Debug Type | Valid values: **0** = No debugging data present; **1** = Low-level debugging data present; **2** = Source level debugging data present; **3** = 1 and 2 are present together. All other values are reserved. |
| `0x28` | Image Base | Address where the image (code) was linked. |
| `0x2C` | Work Space | Obsolete. |
| `0x30` | Address Mode (26/32 + 3 flag bytes) | 0, or contains in its least significant byte: **26** = image linked for 26-bit ARM mode (obsolete); **32** = image linked for 32-bit ARM mode. A value of 0 indicates an old-style 26-bit AIF header. If bit 8 is set, the image was linked with separate code and data bases; the word at `0x34` contains the data base address. |
| `0x34` | Data Base | Address where the image data was linked. |
| `0x38` | Two Reserved Words (initially 0) | In Extended AIF images, the word at `0x38` is non-zero. It contains the byte offset within the file of the header for the first non-root load region. See below for the region header format. |
| `0x40` | NOP | — |
| `0x44` | Zero-Init Code | 15 words. Header is 32 words long. |

**Extended AIF Region Header Format** (44 bytes):

| Word | Description |
|------|-------------|
| 0 | File offset of header of next region (0 if none) |
| 1 | Load address |
| 2 | Size in bytes (a multiple of 4) |
| 3–10 | Region name (32 chars, padded with zeros) |

The initializing data for the region follows the header.

---

---

# ARM Object Library Format

## Chapter 14: ARM Object Library Format

This chapter describes the ARM Object Library Format (ALF). It contains the following sections:

- Overview of ARM Object Library Format
- Endianness and alignment
- Library file format
- Time stamps
- Object code libraries

---

## 14.1 Overview of ARM Object Library Format

ARM Object Library Format (ALF) is used by the ARM linker and the ARM object librarian.

A library file contains a number of separate but related pieces of data. The library file format is layered on another format called Chunk File Format, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file. Refer to the Chunk File Format section for a description.

The Library format defines four chunk classes: Directory, Time stamp, Version, and Data. There may be many Data chunks in a library.

The Object Library Format defines two additional chunks: Symbol table and Symbol table time stamp.

---

## 14.2 Endianness and Alignment

For data in a file, *address* means offset from the start of the file.

There is no guarantee that the endianness of an ALF file will be the same as the endianness of the system used to process it (the endianness of the file is always the same as the endianness of the target ARM system).

The two sorts of ALF cannot meaningfully be mixed. The ARM linker accepts inputs of either sex and produces an output of the same sex, but rejects inputs of mixed endianness.

### 14.2.1 Alignment

Strings and bytes may be aligned on any byte boundary. ALF fields defined in this document do not use halfwords, and align words on 4-byte boundaries.

Within the contents of an ALF file (within the data contained in `OBJ_AREA` chunks), the alignment of words and halfwords is defined by the use to which ALF is being put. For all current ARM-based systems, alignment is strict.

---

## 14.3 Library File Format

For library files, the first part of each chunk name is `LIB_`. For object libraries, the names of the additional two chunks begin with `OFL_`.

Each piece of a library file is stored in a separate, identifiable chunk.

| Chunk | Chunk Name |
|-------|-----------|
| Directory | `LIB_DIRY` |
| Time stamp | `LIB_TIME` |
| Version | `LIB_VRSN` |
| Data | `LIB_DATA` |
| Symbol table | `OFL_SYMT` (object code) |
| Time stamp | `OFL_TIME` (object code) |

There may be many `LIB_DATA` chunks in a library, one for each library member. In all chunks, word values are stored with the same byte order as the target system. Strings are stored in ascending address order, which is independent of target byte order.

### 14.3.1 Earlier Versions of ARM Object Library Format

Notes for robustness with earlier, now obsolete, versions:

- Applications which create libraries or library members should ensure that the `LIB_DIRY` entries they create contain valid time stamps.
- Applications which read `LIB_DIRY` entries should not rely on any data beyond the end of the name string being present, unless the difference between the DataLength field and the name-string length allows for it.
- Applications which write `LIB_DIRY` or `OFL_SYMT` entries should ensure that padding is done with NULL (0) bytes. Applications that read these entries should make no assumptions about the values of padding bytes beyond the first, string-terminating NULL byte.

### 14.3.2 LIB_DIRY

The `LIB_DIRY` chunk contains a directory of the modules in the library, each of which is stored in a `LIB_DATA` chunk. The directory size is fixed when the library is created. The directory consists of a sequence of variable length entries, each an integral number of words long.

| Field | Description |
|-------|-------------|
| ChunkIndex | Zero-origin index within the chunk file header of the corresponding `LIB_DATA` chunk. Conventionally, the first three chunks of an OFL file are `LIB_DIRY`, `LIB_TIME` and `LIB_VRSN`, so ChunkIndex is at least 3. A ChunkIndex of 0 means the directory entry is unused. |
| EntryLength | Number of bytes in this `LIB_DIRY` entry, always a multiple of 4. |
| DataLength | Number of bytes used in the data section of this `LIB_DIRY` entry, also a multiple of 4. |
| Data | Contains (in order): a zero-terminated string (the name of the library member, using only ISO-8859 non-control characters); any other information relevant to the library module (often empty); a two-word, word-aligned time stamp. |

### 14.3.3 LIB_VRSN

The version chunk contains a single word whose value is 1.

### 14.3.4 LIB_DATA

A `LIB_DATA` chunk contains one of the library members indexed by the `LIB_DIRY` chunk. The endianness or byte order of this data is, by assumption, the same as the byte order of the containing library/chunk file.

No other interpretation is placed on the contents of a member by the library management tools. A member could itself be a file in chunk file format or even another library.

---

## 14.4 Time Stamps

A library time stamp is a pair of words that encode:

- a six byte count of centiseconds since 00:00:00 1st January 1900
- a two byte count of microseconds since the last centisecond

**First (most significant) word:** Contains the most significant 4 bytes of the 6 byte centisecond count.

**Second (least significant) word:** Contains the least significant two bytes of the six byte centisecond count in the most significant half of the word and the two byte count of microseconds since the last centisecond in the least significant half of the word. This is usually 0.

Time stamp words are stored in target system byte order. They must have the same endianness as the containing chunk file.

### 14.4.1 LIB_TIME

The `LIB_TIME` chunk contains a two-word (eight-byte) time stamp recording when the library was last modified.

---

## 14.5 Object Code Libraries

An object code library is a library file whose members are files in ARM Object Format. An object code library contains two additional chunks: an external symbol table chunk named `OFL_SYMT`, and a time stamp chunk named `OFL_TIME`.

### 14.5.1 OFL_SYMT

The external symbol table contains an entry for each external symbol defined by members of the library, together with the index of the chunk containing the member defining that symbol.

The `OFL_SYMT` chunk has exactly the same format as the `LIB_DIRY` chunk except that the Data section of each entry contains only a string (the name of an external symbol) and between one and four bytes of NULL padding:

| Field | Description |
|-------|-------------|
| ChunkIndex | — |
| EntryLength | Size of this `OFL_SYMT` chunk (an integral number of words). |
| DataLength | Size of the External Symbol Name and Padding (an integral number of words). |
| External Symbol Name | — |
| Padding | — |

`OFL_SYMT` entries do not contain time stamps.

### 14.5.2 OFL_TIME

The `OFL_TIME` chunk records when the `OFL_SYMT` chunk was last modified and has the same format as the `LIB_TIME` chunk.

---

---

# ARM Object Format

## Chapter 15: ARM Object Format

This chapter describes the ARM Object Format (AOF). It contains the following sections:

- ARM Object Format
- Overall structure of an AOF file
- The AOF header chunk (OBJ_HEAD)
- The AREAS chunk (OBJ_AREA)
- Relocation directives
- Symbol Table Chunk Format (OBJ_SYMT)
- The String Table Chunk (OBJ_STRT)
- The Identification Chunk (OBJ_IDFN)

---

## 15.1 ARM Object Format

The following terms apply throughout this section:

- **object file** — refers to a file in ARM Object Format.
- **address** — for data in a file, this means offset from the start of the file.

### 15.1.1 Areas

An object file written in AOF consists of any number of named, attributed areas. Attributes include: read-only, reentrant, code, data, and position-independent. See section on Attributes and Alignment for details.

Typically, a compiled AOF file contains a read-only code area, and a read-write data area (a zero-initialized data area is also common, and reentrant code uses a separate based area for address constants).

### 15.1.2 Relocation Directives

Associated with each area is a (possibly empty) list of relocation directives which describe locations that the linker will have to update when a non-zero base address is assigned to the area, or a symbolic reference is resolved.

Each relocation directive may be given relative to the (not yet assigned) base address of an area in the same AOF file, or relative to a symbol in the symbol table. Each symbol may:

- have a definition within its containing object file which is local to the object file
- have a definition within the object file which is visible globally (to all object files in the link step)
- be a reference to a symbol defined in some other object file

### 15.1.3 Byte Sex or Endianness

An AOF file can be produced in either little-endian or big-endian format.

There is no guarantee that the endianness of an AOF file will be the same as the endianness of the system used to process it (the endianness of the file is always the same as the endianness of the target ARM system).

### 15.1.4 Alignment

Strings and bytes may be aligned on any byte boundary. AOF fields defined in this document make no use of halfwords and align words on 4-byte boundaries.

Within the contents of an AOF file, the alignment of words and halfwords is defined by the use to which AOF is being put. For all current ARM-based systems, words are aligned on 4-byte boundaries and halfwords on 2-byte boundaries.

---

## 15.2 Overall Structure of an AOF File

An AOF file contains a number of separate pieces of data. The object file format is layered on Chunk File Format, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file.

### 15.2.1 Chunk File Format

A file written in chunk file format consists of a header, and one or more chunks. The header is always positioned at the beginning of the file. A chunk is accessed through the header. The header contains the number, size, location, and identity of each chunk in the file.

The size of the header may vary between different chunk files, but it is fixed for each file. Not all entries in a header need be used, thus limited expansion of the number of chunks is permitted without a wholesale copy. A chunk file can be copied without knowledge of the contents of its chunks.

#### Chunk File Header

The chunk file header consists of two parts: a fixed length part of three words, and a four word entry for each chunk in the file.

**First part (3 words):**

| Field | Description |
|-------|-------------|
| ChunkFileId | Marks the file as a chunk file. Its value is `0xC3CBC6C5`. The endianness can be determined from this value (if it appears to be `0xC5C6CBC3` when read as a word, each word value must be byte-reversed before use). |
| max_chunks | Defines the number of entries in the header, fixed when the file is created. |
| num_chunks | Defines how many chunks are currently used in the file (0 to max_chunks). Redundant — can be found by scanning the entries. |

**Second part (4 words per chunk entry):**

| Field | Description |
|-------|-------------|
| chunkId | An 8-byte field identifying what data the chunk contains. This is an 8-byte field (not a 2-word field), so it has the same byte order independent of endianness. |
| file_offset | A one-word field defining the byte offset within the file of the start of the chunk. Must be divisible by four. A value of zero indicates the chunk entry is unused. |
| size | A one-word field defining the exact byte size of the chunk's contents (which need not be a multiple of four). |

#### Identifying Data Types

The chunkId field provides a conventional way of identifying what type of data a chunk contains. It has eight characters, split into two parts: the first four characters contain a unique name allocated by a central authority; the remaining four characters identify component chunks within this domain.

The eight characters are stored in ascending address order, as if they formed part of a NULL-terminated string, independent of endianness.

For AOF files, the first part of each chunk name is `OBJ_`.

### 15.2.2 ARM Object Format

Each piece of an object file is stored in a separate, identifiable chunk. AOF defines five chunks:

| Chunk | Chunk Name |
|-------|-----------|
| AOF Header | `OBJ_HEAD` |
| Areas | `OBJ_AREA` |
| Identification | `OBJ_IDFN` |
| Symbol Table | `OBJ_SYMT` |
| String Table | `OBJ_STRT` |

Only the AOF Header and AREAS chunks must be present, but a typical object file contains all five.

Each name in an object file is encoded as an offset into the string table, stored in the `OBJ_STRT` chunk. This allows the variable-length nature of names to be factored out from primary data formats.

A feature of ARM Object Format is that chunks may appear in any order in the file. A language translator or other utility may add additional chunks to an object file. Space for eight chunks is conventional when the AOF file is produced by a language processor which generates all five chunks.

> **Note:** The AOF header chunk should not be confused with the chunk file header.

---

## 15.3 The AOF Header Chunk (OBJ_HEAD)

The AOF header consists of two contiguous parts: a fixed size part of six words, and a variable length part consisting of a sequence of area headers.

### Part One — Fixed Header (6 words)

| Field | Description |
|-------|-------------|
| Object File Type | The value `0xC5E2D080` marks the file as relocatable object format. The endianness must be identical to the endianness of the containing chunk file. |
| Version Id | Encodes the AOF version number. The current version number is 310 (`0x136`). |
| Number of Areas | The number of areas in the file, equivalently the number of AREA declarations that follow the fixed part of the AOF header. |
| Number of Symbols | If the object file contains a symbol table chunk (`OBJ_SYMT`), this records the number of symbols in the symbol table. |
| Entry Area Index | 1-origin index in the array of area headers of the area containing the entry point. A value of 0 signifies that no program entry address is defined by this AOF file. |
| Entry Offset | The entry address is defined to be the base address of the entry area plus Entry Offset. |

### Part Two — Area Headers (5 words each)

| Field | Description |
|-------|-------------|
| Area Name | Offset of the name in the string table (`OBJ_STRT`). Each area within an object file must have a unique name. |
| Attributes and Alignment | Bit flags specifying the attributes and alignment of the area. |
| Area Size | Size of the area in bytes (must be a multiple of 4). Unless the Not Initialised bit (bit 12) is set, there must be this number of bytes in the `OBJ_AREA` chunk. |
| Number of Relocations | Number of relocation directives that apply to this area. |
| Base Address | Unused unless the area has the absolute attribute; records the base address. An unused Base Address is denoted by the value 0. |

### 15.3.1 Attributes and Alignment

Each area has a set of attributes encoded in the most significant 24 bits of the Attributes + Alignment word. The least significant eight bits encode the alignment of the start of the area as a power of 2 (value between 2 and 32).

| Bit | Mask | Attribute |
|-----|------|-----------|
| 8 | `0x00000100` | Absolute attribute |
| 9 | `0x00000200` | Code attribute |
| 10 | `0x00000400` | Common block definition |
| 11 | `0x00000800` | Common block reference |
| 12 | `0x00001000` | Uninitialized (zero-initialized) |
| 13 | `0x00002000` | Read-only |
| 14 | `0x00004000` | Position independent |
| 15 | `0x00008000` | Debugging tables |
| 16 | `0x00010000` | Complies with the 32-bit APCS |
| 17 | `0x00020000` | Reentrant code |
| 18 | `0x00040000` | Uses extended FP instruction set |
| 19 | `0x00080000` | No software stack checking |
| 20 | `0x00100000` | All relocations are of Thumb code |
| 21 | `0x00200000` | Area may contain ARM halfword instructions |
| 22 | `0x00400000` | Area suitable for ARM/Thumb interworking |

Some combinations of attributes are meaningless (e.g., read-only and zero-initialized).

The linker orders areas in a generated image by: attributes, then lexicographic order of area names (case-significant), then position of the containing object module in the link list.

**Bit descriptions:**

- **Bit 8** — Absolute attribute: area must be placed at its Base Address. Not usually set by language processors.
- **Bit 9** — Code attribute: 1 = code, 0 = data.
- **Bit 10** — Common definition: common areas with the same name are overlaid by the linker. All other references must specify a size ≤ the definition size. Can be used with bit 9 for common code blocks.
- **Bit 11** — Common block reference: precludes the area having initializing data (implies bit 12). If both bits 10 and 11 are set, bit 11 is ignored.
- **Bit 12** — Zero-initialized attribute: area has no initializing data in this object file; contents are missing from `OBJ_AREA`. Incompatible with read-only (bit 13).
- **Bit 13** — Read-only: area will not be modified following relocation. Code areas and debugging tables must have this bit set. Incompatible with bit 12.
- **Bit 14** — Position independent (PI): any memory address reference must be a link-time-fixed offset from a base register (e.g., pc-relative branch offset).
- **Bit 15** — Debugging table: area contains symbolic debugging tables. Bit 9 is ignored in debugging table areas. Usually has bit 13 set also.
- **Bits 16–22** — Additional attributes of code areas (must be non-zero only if bit 9 is set). Bits 20–22 can be non-zero for data areas.
  - **Bit 16** — 32-bit PC attribute.
  - **Bit 17** — Reentrant attribute.
  - **Bit 18** — Uses ARM floating-point instruction set (LFM/SFM).
  - **Bit 19** — No Software Stack Check attribute.
  - **Bit 20** — Thumb code area.
  - **Bit 21** — Area may contain ARM halfword instructions.
  - **Bit 22** — Suitable for ARM/Thumb interworking.
- **Bits 23–31** — Reserved, set to 0.

---

## 15.4 The AREAS Chunk (OBJ_AREA)

The AREAS chunk contains the actual area contents (code, data, debugging data) together with their associated relocation data. An area is simply a sequence of bytes. The endianness of the words and halfwords within it must agree with that of the containing AOF file.

Layout:

```
Area 1
Area 1 Relocation
...
Area n
Area n Relocation
```

An area is followed by its associated table of relocation directives (if any). An area is either completely initialized by the values from the file or is initialized to zero (as specified by bit 12 of its area attributes). Both area contents and table of relocation directives are aligned to 4-byte boundaries.

---

## 15.5 Relocation Directives

A relocation directive describes a value which is computed at link time or load time, but which cannot be fixed when the object module is created. In the absence of applicable relocation directives, the value of a byte, halfword, word or instruction from the preceding area is exactly the value that will appear in the final image.

A field may be subject to more than one relocation.

### Relocation Directive Format

```
┌────────────────────────────────────────────────┐
│  Offset (word)                                 │
├──┬──┬──┬──┬──┬──┬──────────────────────────────┤
│31│30│29│28│27│26│25│24│23        ...          0 │
│ II  │ B│ A│ R│  FT │       SID (24 bits)       │
└──┴──┴──┴──┴──┴──┴──┴──────────────────────────-┘
```

**Offset** — byte offset in the preceding area of the subject field to be relocated.

**SID (bits 0–23):** Depends on the A bit (bit 27):

- **A=1:** Subject field is relocated by the value of the symbol at index SID in the symbol table chunk.
- **A=0:** Subject field is relocated by the base of the area at index SID in the array of areas.

**FT (bits 24–25):** Describes the subject field type:

| Value | Field Type |
|-------|------------|
| 00 | Byte |
| 01 | Halfword (two bytes) |
| 10 | Word (four bytes) |
| 11 | Instruction or instruction sequence (bit 0 of offset set = Thumb, otherwise ARM) |

Bytes, halfwords, and instructions may only be relocated by values of small size. Overflow is faulted by the linker.

**II (bits 29–30):** For instruction sequences, constrains how many instructions may be modified:

| Value | Constraint |
|-------|-----------|
| 00 | No constraint |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

**R (bit 26) and B (bit 28):** Determine how the relocation value modifies the subject field:

- **R=0, B=0** — Plain additive relocation: `subject_field = subject_field + relocation_value`
- **R=1, B=0** — PC-relative relocation: `subject_field = subject_field + (relocation_value - base_of_area_containing(subject_field))`. Special case: if A=0 and the relocation value is the base of the area containing the subject field, it is not added. If R=1, B is usually 0. B=1 denotes the inter-link-unit value of a branch destination is to be used.
- **R=0, B=1** — Based area relocation: `subject_field = subject_field + (relocation_value - base_of_area_group_containing(relocation_value))`. Bits 29–30 must be 0. Bit 31 must be 1.

---

## 15.6 Symbol Table Chunk Format (OBJ_SYMT)

The Number of Symbols field in the fixed part of the AOF header defines how many entries there are in the symbol table. Each symbol table entry is four words long:

| Field | Description |
|-------|-------------|
| Name | Offset in the string table (`OBJ_STRT`) of the symbol name. |
| Attributes | See Symbol Attributes below. |
| Value | Meaningful only if the symbol is a defining occurrence (bit 0 set) or a common symbol (bit 6 set). If absolute: contains the symbol value. If common: contains the byte length. Otherwise: offset from the base address of the area named by Area Name. |
| Area Name | Meaningful only if the symbol is a non-absolute defining occurrence (bit 0 set, bit 2 unset). Gives the string table index for the name of the area in which the symbol is defined. |

### 15.6.1 Symbol Attributes

| Bit | Mask | Attribute |
|-----|------|-----------|
| 0 | `0x00000001` | Symbol is defined in this file |
| 1 | `0x00000002` | Symbol has global scope |
| 2 | `0x00000004` | Absolute attribute |
| 3 | `0x00000008` | Case-insensitive attribute |
| 4 | `0x00000010` | Weak attribute |
| 6 | `0x00000040` | Common attribute |
| 8 | `0x00000100` | Code area datum attribute |
| 9 | `0x00000200` | FP args in FP regs attribute |
| 12 | `0x00001000` | Thumb symbol |

**Bit interpretations:**

- **Bit 0** — Symbol is defined in this object file.
- **Bit 1** — Symbol has global scope; can be matched by the linker to a similarly named symbol from another object file.
  - **01** (bit 1 unset, bit 0 set) — Defined with scope limited to this object file.
  - **10** (bit 1 set, bit 0 unset) — Reference to a symbol defined in another object file. If no defining instance is found, the linker attempts to match to common block names.
  - **11** — Defined with global scope.
  - **00** — Reserved.
- **Bit 2** — Absolute attribute: the symbol has an absolute value (e.g., a constant).
- **Bit 3** — Case insensitive reference: the linker will ignore case when matching (only meaningful for external references).
- **Bit 4** — Weak attribute: acceptable for the reference to remain unsatisfied. The linker ignores weak references when deciding which members to load from an object library.
- **Bit 5** — Reserved, must be 0.
- **Bit 6** — Common attribute: the symbol is a reference to a common area. The length is given by the symbol's Value field.
- **Bit 7** — Reserved, must be 0.
- **Bits 8–11** — Additional attributes of symbols defined in code areas.
  - **Bit 8** — Code datum: symbol identifies a datum (usually read-only), not an executable instruction.
  - **Bit 9** — FP arguments in FP registers: symbol identifies a function entry point. A reference with this attribute cannot match a definition lacking it.
  - **Bits 10–11** — Reserved, must be 0.
- **Bit 12** — Thumb attribute: symbol is a Thumb symbol.

---

## 15.7 The String Table Chunk (OBJ_STRT)

The string table chunk contains all the print names referred to from the header and symbol table chunks. A print name is stored as a sequence of non-control characters (codes 32–126 and 160–255) terminated by a NULL (0) byte, and is identified by an offset from the start of the table.

The first four bytes of the string table contain its length (including the length word itself), so no valid offset is less than four, and no table has length less than four.

The endianness of the length word must be identical to the endianness of the AOF and chunk files containing it.

---

## 15.8 The Identification Chunk (OBJ_IDFN)

This chunk should contain a string of printable characters (codes 10–13 and 32–126) terminated by a NULL (0) byte, which gives information about the name and version of the tool which generated the object file.

Use of codes in the range 128–255 is discouraged, as the interpretation of these values is host-dependent.
