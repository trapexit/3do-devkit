# ARM Object Format (AOF) — Complete File Format Specification

## Overview

ARM Object Format (AOF) is the object file format used by language processors (compilers, assemblers) for ARM-based systems. AOF files are the primary input to the ARM linker (`armlink`), which combines them to produce executable images (typically in AIF format). AOF is a simple object format, similar in complexity and expressive power to Unix's `a.out` format, designed to be straightforward to generate and process.

AOF directly supports the ARM Procedure Call Standard (APCS) through area attributes that encode APCS compliance information.

### Document Sources

This specification is synthesized from:

- ARM Software Development Toolkit Reference Guide (ARM DUI 0041C, Chapter 15, 1997/1998)
- RISC OS Programmer's Reference Manuals, Appendix D
- 3DO SDK Technical Documentation (ARM Technical Specifications)

---

## 1. Terminology

| Term       | Definition |
|------------|------------|
| **Byte**   | 8 bits, unsigned unless otherwise stated. Used to store flag bits or characters. |
| **Half Word** | 16 bits (2 bytes), usually unsigned. |
| **Word**   | 32 bits (4 bytes), usually unsigned. |
| **String** | A sequence of bytes terminated by a NUL (0x00) byte. The NUL is part of the string but is not counted in its length. Strings may be aligned on any byte boundary. |

---

## 2. Endianness (Byte Sex)

There are two sorts of AOF: **little-endian** and **big-endian**.

- **Little-endian AOF**: The least significant byte of a word or half-word has the lowest address. Used by DEC, Intel, and Acorn.
- **Big-endian AOF**: The most significant byte of a word or half-word has the lowest address. Used by IBM, Motorola, and Apple.

For data in a file, *address* means offset from the start of the file.

There is no guarantee that the endianness of an AOF file will be the same as the endianness of the system used to process it — the endianness of the file is always the same as the endianness of the target ARM system.

The two sorts of AOF cannot be mixed. The ARM linker accepts inputs of either sex and produces an output of the same sex, but rejects inputs of mixed endianness.

---

## 3. Alignment

- Strings and bytes may be aligned on any byte boundary.
- AOF header fields make no use of half-words and align words on 4-byte boundaries.
- Within the contents of an AOF file (in `OBJ_AREA` chunks), words are aligned on 4-byte boundaries and half-words on 2-byte boundaries for all current ARM-based systems.

---

## 4. Overall Structure — Chunk File Format

An AOF file is layered on **Chunk File Format**, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file.

### 4.1 Chunk File Layout

```
+---------------------------+
| Chunk File Header         |
|   ChunkFileId (1 word)    |
|   maxChunks   (1 word)    |
|   numChunks   (1 word)    |
+---------------------------+
| Chunk Entry 0 (4 words)   |
| Chunk Entry 1 (4 words)   |
| ...                       |
| Chunk Entry N (4 words)   |
+---------------------------+
| Chunk Data 0              |
| Chunk Data 1              |
| ...                       |
+---------------------------+
```

### 4.2 Chunk File Header (Fixed Part)

| Offset | Size   | Field         | Description |
|--------|--------|---------------|-------------|
| 0x00   | 1 word | ChunkFileId   | Magic number identifying this as a chunk file. Value: `0xC3CBC6C5`. The endianness can be deduced from this value — if it reads as `0xC5C6CBC3`, each word value must be byte-reversed. |
| 0x04   | 1 word | maxChunks     | Number of chunk entries in the header. Fixed when the file is created. |
| 0x08   | 1 word | numChunks     | Number of chunks currently used (0 to maxChunks). Redundant — can be determined by scanning entries. |

### 4.3 Chunk Entry Format (4 words each)

Each entry in the chunk file header consists of four words:

| Offset | Size    | Field      | Description |
|--------|---------|------------|-------------|
| +0x00  | 8 bytes | chunkId    | An 8-byte identifier for the chunk's data type. This is an 8-byte field (not a 2-word field), so it has the same byte order independent of endianness. The 8 characters are stored in ascending address order, as if they formed part of a NUL-terminated string. |
| +0x08  | 1 word  | fileOffset | Byte offset within the file of the start of the chunk. Must be divisible by 4 (all chunks are word-aligned). A value of 0 indicates the chunk entry is unused. |
| +0x0C  | 1 word  | size       | Exact byte size of the chunk's contents (need not be a multiple of 4). |

### 4.4 Chunk Identification

The `chunkId` field is split into two 4-character parts:

- **First 4 characters**: A universally unique name allocated by a central authority (Acorn/ARM).
- **Last 4 characters**: Identify component chunks within this domain.

For AOF files, the first part of each chunk name is `OBJ_`.

---

## 5. AOF Chunks

AOF defines five chunks:

| Chunk           | Chunk Name  | Required | Description |
|-----------------|-------------|----------|-------------|
| AOF Header      | `OBJ_HEAD`  | Yes      | Fixed and variable-length header describing the object file. |
| Areas           | `OBJ_AREA`  | Yes      | Code and data areas with their relocation directives. |
| Identification  | `OBJ_IDFN`  | No       | Identifies the tool that created the file. |
| Symbol Table    | `OBJ_SYMT`  | No       | External and local symbol definitions and references. |
| String Table    | `OBJ_STRT`  | No       | All print names referenced from the header and symbol table. |

Only the Header and Areas chunks must be present. A typical object file contains all five chunks.

Chunks may appear in any order within the file. A language translator or other utility may add additional chunks (e.g., language-specific symbol tables or debugging data). Space for eight chunks is conventional when the AOF file is produced by a language processor that generates all five standard chunks.

**Important**: The AOF header chunk (`OBJ_HEAD`) is distinct from the chunk file header.

---

## 6. The AOF Header Chunk (`OBJ_HEAD`)

The AOF header consists of two contiguous parts within the `OBJ_HEAD` chunk:

1. **Fixed Part** (6 words / 24 bytes)
2. **Variable Part** (5 words per area header)

### 6.1 Fixed Part (6 words)

| Offset | Size   | Field            | Description |
|--------|--------|------------------|-------------|
| +0x00  | 1 word | Object File Type | `0xC5E2D080` — marks the file as relocatable object format. The endianness must be identical to the containing chunk file. |
| +0x04  | 1 word | Version Id       | AOF version number. Version 1.50 = `150` (0x96). Version 2.00 = `200` (0xC8). Version 3.10 = `310` (0x136, current). |
| +0x08  | 1 word | Number of Areas  | Number of code/data areas in the file. Equivalently, the number of area headers in the variable part. Should be ≤ 255. |
| +0x0C  | 1 word | Number of Symbols | Number of symbols in the symbol table (`OBJ_SYMT` chunk). 0 if no symbol table is present. |
| +0x10  | 1 word | Entry Area Index | 1-origin index into the array of area headers of the area containing the program entry point. 0 if no entry point is defined by this AOF file. |
| +0x14  | 1 word | Entry Offset     | Byte offset of the entry point within the entry area. The entry address = base address of the entry area + Entry Offset. |

### 6.2 Variable Part — Area Headers (5 words each)

The variable part immediately follows the fixed part. It contains `Number of Areas` entries, each describing one code or data area.

| Offset | Size   | Field                  | Description |
|--------|--------|------------------------|-------------|
| +0x00  | 1 word | Area Name              | Offset of the area name in the string table (`OBJ_STRT`). Each area within an object file must have a unique name. |
| +0x04  | 1 word | Attributes + Alignment | Bit flags encoding area attributes (bits 8-31) and alignment (bits 0-7). See section 7. |
| +0x08  | 1 word | Area Size              | Size of the area in bytes. Must be a multiple of 4. Unless the zero-initialized bit (bit 12) is set, there must be this many bytes of data for this area in the `OBJ_AREA` chunk. |
| +0x0C  | 1 word | Number of Relocations  | Number of relocation directives that apply to this area. |
| +0x10  | 1 word | Base Address           | Unused (should be 0) unless the area has the absolute attribute (bit 8). Records the fixed base address for absolute areas. |

---

## 7. Area Attributes and Alignment

The `Attributes + Alignment` word encodes area attributes in the most significant 24 bits and alignment in the least significant 8 bits.

### 7.1 Alignment (Bits 0-7)

The least significant 8 bits encode the alignment of the start of the area as a power of 2. Valid values range from 2 to 32, meaning the area should start at an address divisible by 2^alignment. For example:

| Value | Alignment |
|-------|-----------|
| 2     | 4-byte aligned (word) |
| 3     | 8-byte aligned |
| 4     | 16-byte aligned |
| ...   | ... |

### 7.2 Attribute Bits (Bits 8-31)

| Bit(s)  | Mask           | Attribute                    | Applicability |
|---------|----------------|------------------------------|---------------|
| 8       | `0x00000100`   | Absolute                     | All areas |
| 9       | `0x00000200`   | Code                         | All areas |
| 10      | `0x00000400`   | Common block definition      | All areas |
| 11      | `0x00000800`   | Common block reference       | All areas |
| 12      | `0x00001000`   | Zero-initialized (uninit.)   | All areas |
| 13      | `0x00002000`   | Read-only                    | All areas |
| 14      | `0x00004000`   | Position independent         | All areas |
| 15      | `0x00008000`   | Debugging tables             | All areas |
| 16      | `0x00010000`   | 32-bit APCS compliant        | Code areas only |
| 17      | `0x00020000`   | Reentrant code               | Code areas only |
| 18      | `0x00040000`   | Extended FP instruction set  | Code areas only |
| 19      | `0x00080000`   | No software stack checking   | Code areas only |
| 20      | `0x00100000`   | Based area / Thumb code      | Data / Code areas |
| 21      | `0x00200000`   | Shared library stub data / ARM halfword instructions | Data / Code areas |
| 22      | `0x00400000`   | ARM/Thumb interworking       | Code areas |
| 23      | —              | Reserved (must be 0)         | — |
| 24-27   | `0x0F000000`   | Base register (for based data areas) | Data areas only |
| 28-31   | —              | Reserved (must be 0)         | — |

### 7.3 Detailed Attribute Descriptions

**Bit 8 — Absolute**: The area must be placed at its Base Address (from the area header). Not usually set by language processors. An absolute area has the property that its base remains constant through relocation and linking. Absolute areas cannot contain relocatable references to non-absolute areas.

**Bit 9 — Code**: If set, the area contains code; otherwise, it contains data.

**Bit 10 — Common Block Definition**: The area is a common definition. Common areas with the same name are overlaid on each other by the linker. The Area Size field defines the size of the common block. All other references to this common block must specify a size ≤ the definition size. If there is more than one definition, each must have exactly the same contents. If there is no definition, the size is the maximum of all common references. Can be used in conjunction with bit 9 for common code blocks.

**Bit 11 — Common Block Reference**: Defines the area as a reference to a common block. Precludes the area having initializing data (implies bit 12). If both bits 10 and 11 are set, bit 11 is ignored.

**Bit 12 — Zero-Initialized (Uninitialized)**: The area has no initializing data in this object file; its contents are missing from the `OBJ_AREA` chunk. The linker creates a read-write area of binary zeroes of appropriate size, or maps a read-write area that will be zeroed at image start-up. Incompatible with bit 13 (read-only). A combination of bit 10 and bit 12 has the same meaning as bit 11 alone.

**Bit 13 — Read-Only**: The area will not be modified following relocation. The linker groups read-only areas together for write-protection at run-time. Code areas and debugging tables should have this bit set. Incompatible with bit 12.

**Bit 14 — Position Independent (PI)**: Any reference to a memory address from this area must be in the form of a link-time-fixed offset from a base register (e.g., a PC-relative branch offset). Usually only significant for code areas.

**Bit 15 — Debugging Tables**: The area contains symbolic debugging tables. The linker groups these together for access as a single continuous chunk. Usually has bit 13 set also. Bit 9 (code) is ignored in debugging table areas.

**Bit 16 — 32-bit APCS**: (Code areas only.) Code complies with a 32-bit variant of the ARM Procedure Call Standard. May be incompatible with 26-bit APCS code.

**Bit 17 — Reentrant**: (Code areas only.) Code complies with a reentrant variant of the APCS.

**Bit 18 — Extended FP Instruction Set**: (Code areas only.) Code uses the ARM's extended floating-point instruction set (LFM/SFM save/restore rather than multiple LDFEs/STFEs). May not execute on older ARM-based systems.

**Bit 19 — No Software Stack Checking**: (Code areas only.) Code complies with a variant of the APCS without software stack-limit checking. May be incompatible with limit-checked APCS code.

**Bit 20 — Based Area / Thumb Code**:
- *Data areas*: The area is addressed via link-time-fixed offsets from a base register (encoded in bits 24-27). Based areas have a special role in shared libraries and ROM-able code.
- *Code areas* (version 3.10): All relocations are of Thumb code.

**Bit 21 — Shared Library Stub Data / ARM Halfword Instructions**:
- *Data areas*: Shared Library Stub Data attribute. Areas with both bit 21 and bit 12 are treated similarly to common reference areas.
- *Code areas* (version 3.10): Area may contain ARM halfword instructions.

**Bit 22 — ARM/Thumb Interworking**: (Version 3.10, code areas.) Area is suitable for ARM/Thumb interworking.

**Bits 24-27 — Base Register**: (Data areas only.) Encode the base register used to address a based area. Must be 0 if the area is not based.

### 7.4 Area Ordering by the Linker

The linker orders areas in a generated image by:

1. Attributes (areas with like attributes are grouped together)
2. Lexicographic order of area names (case-significant)
3. Position of the containing object module in the link list

The position of an object module loaded from a library is not predictable.

### 7.5 Attribute Compatibility Rules

| Combination | Rule |
|-------------|------|
| Code + Common | Common areas are overlaid; multiple definitions must have identical contents. |
| Zero-initialized + Read-only | Incompatible — cannot both be set. |
| Common definition + Zero-initialized | Equivalent to Common block reference (bit 11). |
| Debugging tables | Bit 9 (code) is ignored in debugging table areas. |

---

## 8. The Areas Chunk (`OBJ_AREA`)

The `OBJ_AREA` chunk contains the actual area contents (code, data, debugging data) together with their associated relocation directives.

### 8.1 Layout

```
+----------------------------+
| Area 1 Data                |   (Area Size bytes, 4-byte aligned)
+----------------------------+
| Area 1 Relocation Table    |   (Number of Relocations × 8 bytes)
+----------------------------+
| Area 2 Data                |
+----------------------------+
| Area 2 Relocation Table    |
+----------------------------+
| ...                        |
+----------------------------+
| Area N Data                |
+----------------------------+
| Area N Relocation Table    |
+----------------------------+
```

- An area is a sequence of byte values. The endianness of words and half-words must agree with the containing AOF file.
- An area is either completely initialized by the data in the file, or is zero-initialized (bit 12 set, in which case no data is present).
- Both area contents and relocation tables are aligned to 4-byte boundaries.
- An area is followed immediately by its associated table of relocation directives (if any).

---

## 9. Relocation Directives

A relocation directive describes a value that is computed at link time or load time but cannot be fixed when the object module is created. In the absence of applicable relocation directives, the value of a byte, halfword, word, or instruction is exactly the value that will appear in the final image.

A field may be subject to more than one relocation.

### 9.1 Relocation Directive Format (8 bytes / 2 words)

```
Word 0:
+------------------------------------------+
|              Offset (32 bits)            |
+------------------------------------------+

Word 1:
+--+--+--+--+--+--+--+------------------------+
|31|30|29|28|27|26|25|24|23                   0|
| 1| II  | B| A| R| FT |       SID (24 bits)  |
+--+--+--+--+--+--+--+--+---------------------+
```

### 9.2 Field Descriptions

**Offset** (Word 0, 32 bits): Byte offset within the preceding area of the subject field to be relocated.

**SID** (Bits 0-23): Subject IDentifier. Interpretation depends on the A bit:
- **A=0**: SID is the 0-origin index of an area in the array of areas. The subject field is relocated by the base of that area.
- **A=1**: SID is the 0-origin index of a symbol in the symbol table chunk. The subject field is relocated by the value of that symbol.

**FT — Field Type** (Bits 24-25):

| Value | Meaning |
|-------|---------|
| 00    | The field to be relocated is a **byte** (1 byte). |
| 01    | The field to be relocated is a **half-word** (2 bytes). |
| 10    | The field to be relocated is a **word** (4 bytes). |
| 11    | The field to be relocated is an **instruction or instruction sequence**. If bit 0 of the Offset is set, the instruction is Thumb; otherwise it is ARM. |

Bytes, halfwords, and instructions may only be relocated by values of suitably small size. Overflow is faulted by the linker.

**R — PC-Relative** (Bit 26): See section 9.3.

**A — Symbol/Area Selector** (Bit 27): See SID above.

**B — Based** (Bit 28): See section 9.3.

**II — Instruction Count Constraint** (Bits 29-30): Only meaningful for instruction sequence relocations (FT=11):

| Value | Meaning |
|-------|---------|
| 00    | No constraint — linker may modify as many contiguous instructions as needed. |
| 01    | At most 1 instruction may be modified. |
| 10    | At most 2 instructions may be modified. |
| 11    | At most 3 instructions may be modified. |

**Bit 31**: Must be 1 for based relocations (R=0, B=1). Otherwise see the applicable relocation mode.

### 9.3 Relocation Modes

The way the relocation value modifies the subject field is determined by the R (PC-relative) and B (based) bits:

#### 9.3.1 Plain Additive Relocation (R=0, B=0)

The relocation value is added to the subject field:

```
subject_field = subject_field + relocation_value
```

#### 9.3.2 PC-Relative Relocation (R=1, B=0)

The difference between the relocation value and the base of the area containing the subject field is added:

```
subject_field = subject_field + (relocation_value - base_of_area_containing(subject_field))
```

**Special case**: If A=0 and the relocation value is the base of the area containing the subject field, only the subtraction is performed:

```
subject_field = subject_field - base_of_area_containing(subject_field)
```

This caters for relocatable PC-relative branches to fixed target addresses.

If R=1, B is usually 0. A B value of 1 with R=1 denotes that the inter-link-unit value of a branch destination should be used, rather than the intra-link-unit value. This allows compilers to perform the tail-call optimization on reentrant code.

#### 9.3.3 Based Area Relocation (R=0, B=1)

The relocation value must be an address within a based data area. The subject field is incremented by the difference between the relocation value and the base address of the consolidated based area group:

```
subject_field = subject_field + (relocation_value - base_of_area_group_containing(relocation_value))
```

For based area relocation:
- Bits 29-30 must be 0.
- Bit 31 must be 1.

This is used when generating reentrant code. The C compiler places address constants in an adcon area based on register `sb`, and loads them using sb-relative LDRs. At link time, separate adcon areas are merged and `sb` no longer points where assumed at compile time — the B-type relocation of the LDR instructions corrects for this.

### 9.4 RISC OS PRMs Relocation Format (Version 1.50)

In the older RISC OS PRMs version of AOF, the relocation directive format is simpler:

| Bits   | Field      | Description |
|--------|------------|-------------|
| 0      | PC-relative flag | If set, the A-field is PC-relative. |
| 8-11   | A field specifier | How to extract the A-field from the word to be relocated. 0 = no A-field. |
| 12-15  | R-type field specifier | Always 0 in version 1.50 (word-wide). |
| 16-23  | SID number | Symbol table index, or if bit 23 is set, remaining 7 bits specify the area number. |

---

## 10. Symbol Table Chunk (`OBJ_SYMT`)

The `Number of Symbols` field in the fixed part of the AOF header defines the number of entries in the symbol table. Each entry is 4 words (16 bytes) long.

### 10.1 Symbol Table Entry Format

| Offset | Size   | Field     | Description |
|--------|--------|-----------|-------------|
| +0x00  | 1 word | Name      | Offset in the string table (`OBJ_STRT`) of the symbol name. |
| +0x04  | 1 word | Attributes | Bit flags encoding symbol attributes. See section 10.2. |
| +0x08  | 1 word | Value     | Depends on attributes (see below). |
| +0x0C  | 1 word | Area Name | Offset in the string table of the defining area name. See below. |

**Value field interpretation**:
- If the symbol is **absolute** (bits 0,2 set): Contains the absolute value of the symbol.
- If the symbol is **common** (bit 6 set): Contains the byte length of the common area.
- Otherwise: Offset from the base address of the area named by `Area Name`.

**Area Name field**: Meaningful only if the symbol is a non-absolute defining occurrence (bit 0 set, bit 2 unset). Gives the string table offset for the name of the area in which the symbol is defined. Must refer to an area defined in this object file. The linker combines the base of the relevant area with the symbol's Value to produce the symbol's absolute value at link time.

### 10.2 Symbol Attributes

| Bit(s) | Mask           | Attribute                        | Description |
|--------|----------------|----------------------------------|-------------|
| 0      | `0x00000001`   | Defined in this file             | Symbol has a definition in this object file. |
| 1      | `0x00000002`   | Global scope                     | Symbol can be matched by the linker to symbols from other files. |
| 2      | `0x00000004`   | Absolute                         | Symbol has an absolute value (a constant). |
| 3      | `0x00000008`   | Case-insensitive reference       | Linker ignores case when matching (external references only). |
| 4      | `0x00000010`   | Weak                             | Reference may remain unsatisfied; linker ignores for library loading. |
| 5      | `0x00000020`   | Strong                           | This definition overrides non-strong definitions from other files. |
| 6      | `0x00000040`   | Common                           | Reference to a common area; Value field = common area byte length. |
| 7      | —              | Reserved (must be 0)             | — |
| 8      | `0x00000100`   | Code datum                       | Symbol identifies a datum (not code) within a code area. |
| 9      | `0x00000200`   | FP args in FP regs               | Symbol identifies a function entry point with FP args in FP regs. |
| 10     | —              | Reserved (must be 0)             | — |
| 11     | `0x00000800`   | Simple leaf function             | Entry point of a simple leaf function (intra = inter LU entry). |
| 12     | `0x00001000`   | Thumb symbol                     | Symbol is a Thumb code symbol (version 3.10). |
| 13-31  | —              | Reserved (must be 0)             | — |

### 10.3 Scope Encoding (Bits 0-1)

| Bit 1 | Bit 0 | Meaning |
|-------|-------|---------|
| 0     | 0     | Reserved. |
| 0     | 1     | Symbol defined in this file with **local** scope (only matched within this file). |
| 1     | 0     | Symbol is a **reference** to a symbol defined in another file. If no definition is found, the linker attempts to match to common block names. |
| 1     | 1     | Symbol defined in this file with **global** scope (can be matched to references from other files). |

### 10.4 Detailed Attribute Descriptions

**Bit 2 — Absolute**: Meaningful only for defining occurrences (bit 0 set). The symbol has an absolute value (e.g., a constant), not relative to any area base.

**Bit 3 — Case-Insensitive**: Meaningful only for external references (bit 0 unset). The linker ignores case when matching.

**Bit 4 — Weak**: Meaningful only for external references (bits 1,0 = 10). It is acceptable for the reference to remain unsatisfied and for any fields relocated via it to remain unrelocated. The linker ignores weak references when deciding which members to load from an object library.

**Bit 5 — Strong**: Meaningful only for external defining occurrences (bits 1,0 = 11). If there is a non-strong external definition of the same symbol in another file, references from outside the file containing the strong definition resolve to the strong definition, while those within that file resolve to the non-strong definition. Allows link-time indirection. Usually the strong definition is absolute and implements an OS entry vector.

**Bit 6 — Common**: Meaningful only for external references (bits 1,0 = 10). The symbol references a common area with the symbol's name. The Value field gives the length. All common symbols with the same name are assigned the same base address; the allocated length is the maximum of all specified lengths. If the name matches a common area, they are merged. Unmatched common symbols are collected into an anonymous linker-created pseudo-area.

**Bit 8 — Code Datum**: (Code area symbols only.) The symbol identifies a datum (usually read-only data) within a code area, not an executable instruction.

**Bit 9 — FP Args in FP Regs**: (Code area symbols only.) The symbol identifies a function entry point that passes floating-point arguments in floating-point registers. A reference with this attribute cannot be matched to a definition lacking it.

**Bit 11 — Simple Leaf Function**: (Code area symbols only.) The symbol defines the entry point of a sufficiently simple leaf function. For reentrant code, this means the function's inter-link-unit entry point is the same as its intra-link-unit entry point.

**Bit 12 — Thumb**: (Version 3.10.) The symbol is a Thumb code symbol.

---

## 11. The String Table Chunk (`OBJ_STRT`)

The string table chunk contains all the print names referred to from the AOF header and symbol table chunks. This separation factors out the variable-length nature of names from the primary data structures.

### 11.1 Format

```
+---------------------------+
| Length (4 bytes)           |   Length of the entire string table
+---------------------------+   including this length word.
| String 0 (NUL-terminated) |
| String 1 (NUL-terminated) |
| ...                       |
+---------------------------+
```

- The first 4 bytes contain the length of the string table (including the length word itself).
- No valid offset into the table is less than 4.
- No table has a length less than 4.
- A print name is a sequence of non-control characters (codes 32-126 and 160-255) terminated by a NUL (0) byte.
- Names are identified by byte offsets from the start of the table.
- The endianness of the length word must be identical to the endianness of the AOF and chunk files containing it.

---

## 12. The Identification Chunk (`OBJ_IDFN`)

This chunk contains a string of printable characters (codes 10-13 and 32-126) terminated by a NUL (0) byte. It identifies the name and version of the tool that generated the object file.

Use of codes in the range 128-255 is discouraged as their interpretation is host-dependent.

---

## 13. Linker Symbol Table Output Format

The linker can write its internal symbol table to a file for debugging and browsing purposes. This file is a table of zero-terminated strings with no padding. Each line consists of up to four comma-separated fields:

```
name,value,class[,definingmodulename]
```

| Field                | Description |
|----------------------|-------------|
| `name`               | Symbol name (at least one printable character). |
| `value`              | Symbol value as a hex number prefixed by `&` (BBC Basic style). May be a constant, absolute address, or offset from a base. |
| `class`              | One of: `Abs`, `RelocDef`, `RelocRef`, `ExtDef`, `ExtRef`. |
| `definingmodulename` | Optional. Name of a module in which the symbol is defined or referenced. |

**Class values**:

| Class      | Meaning |
|------------|---------|
| `Abs`      | The value is absolute. |
| `ExtRef`   | The symbol is referenced but not defined anywhere. |
| `ExtDef`   | The symbol is defined in one or more modules and referenced from other modules. |
| `RelocDef` | The symbol is defined but not referenced from any other module. |
| `RelocRef` | The symbol is referenced in one or more modules. |

---

## 14. Complete Binary Layout Summary

### 14.1 Chunk File Header

```
Offset   Size      Field
------   ----      -----
0x00     4 bytes   ChunkFileId = 0xC3CBC6C5
0x04     4 bytes   maxChunks
0x08     4 bytes   numChunks
0x0C     16×N      Chunk Entries (4 words each, N = maxChunks)
```

### 14.2 OBJ_HEAD Chunk

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Object File Type = 0xC5E2D080
+0x04    4 bytes   Version Id (e.g., 310)
+0x08    4 bytes   Number of Areas
+0x0C    4 bytes   Number of Symbols
+0x10    4 bytes   Entry Area Index (1-origin, or 0)
+0x14    4 bytes   Entry Offset
+0x18    20×N      Area Headers (5 words each, N = Number of Areas)
```

### 14.3 Area Header

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Area Name (string table offset)
+0x04    4 bytes   Attributes + Alignment
+0x08    4 bytes   Area Size (bytes, multiple of 4)
+0x0C    4 bytes   Number of Relocations
+0x10    4 bytes   Base Address (0 if not absolute)
```

### 14.4 OBJ_AREA Chunk — Per-Area Content

```
Offset   Size             Field
------   ----             -----
+0x00    Area Size bytes  Area Data (omitted if zero-initialized)
+N       8×R bytes        Relocation Directives (R = Number of Relocations)
```

### 14.5 Relocation Directive

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Offset (byte offset within area)
+0x04    4 bytes   Flags: [1|II|B|A|R|FT|SID(24)]
```

### 14.6 OBJ_SYMT Chunk — Per-Symbol Entry

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Name (string table offset)
+0x04    4 bytes   Attributes
+0x08    4 bytes   Value
+0x0C    4 bytes   Area Name (string table offset)
```

### 14.7 OBJ_STRT Chunk

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Table Length (including this word)
+0x04    variable  NUL-terminated strings, packed end-to-end
```

### 14.8 OBJ_IDFN Chunk

```
Offset   Size      Field
------   ----      -----
+0x00    variable  NUL-terminated identification string
```

---

## 15. Identifying an AOF File

An AOF file can be identified by:

1. **Chunk File Magic**: First word = `0xC3CBC6C5` (or byte-swapped equivalent).
2. **Chunk Names**: Look for `OBJ_HEAD` and `OBJ_AREA` chunk IDs.
3. **Object File Type**: The first word of the `OBJ_HEAD` chunk = `0xC5E2D080`.
4. **Version**: The second word of `OBJ_HEAD` should be a recognized AOF version (150, 200, or 310).
