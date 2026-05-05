# ARM Object Format (AOF) Specification

**Consolidated Technical Reference**

---

## 1. Introduction

ARM Object Format (AOF) is the relocatable object file format used by ARM language processors (compilers, assemblers) and consumed by the ARM linker. It was designed by Acorn Computers Limited for ARM-based systems and later maintained by ARM Limited as part of the ARM Software Development Toolkit (SDT).

AOF is conceptually similar to Unix `a.out` but provides a generalised superset of `a.out`'s descriptive facilities (though this was neither an original design goal nor an influence on the early development of AOF). It was designed to be simple to generate and to process, rather than to be maximally expressive or maximally compact. AOF directly supports the ARM Procedure Call Standard (APCS) and the conventions of ARM-based systems.

An AOF file consists of named, attributed **areas** (code and data sections) with associated **relocation directives**, a **symbol table**, a **string table**, and an **identification string**. The format is layered on **Chunk File Format**, a simple container format that provides efficient access to distinct chunks of data within a single file.

---

## 2. Terminology

| Term | Definition |
|------|-----------|
| **Byte** | 8 bits, unsigned unless stated otherwise. Usually used to store flag bits or characters. |
| **Half word** | 16 bits (2 bytes), usually unsigned. In little-endian format, the least significant byte has the lowest address. Must be aligned on a 2-byte boundary. |
| **Word** | 32 bits (4 bytes), usually used to store a non-negative value. In little-endian format, the least significant byte has the lowest address. Must be aligned on a 4-byte boundary. |
| **String** | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL is part of the string but is not counted in the string's length. May be aligned on any byte boundary. |

Throughout this document, *object file* refers to a file in ARM Object Format, and *linker* refers to the ARM linker (`armlink`).

---

## 3. Endianness and Alignment

### 3.1 Byte Order

An AOF file can be produced in either **little-endian** or **big-endian** format:

- **Little-endian** (DEC/Intel/Acorn): the least significant byte of a word or half word has the lowest address.
- **Big-endian** (IBM/Motorola/Apple): the most significant byte of a word or half word has the lowest address.

The endianness of an AOF file always matches the endianness of the target ARM system. There is no guarantee that the file endianness matches the host system's endianness. The two byte orders cannot be mixed: the ARM linker accepts inputs of either byte order and produces output of the same order, but rejects inputs of mixed endianness.

For data in a file, *address* means offset from the start of the file.

### 3.2 Alignment

- Strings and bytes may be aligned on any byte boundary.
- AOF fields defined in this specification make no use of half words and align words on 4-byte boundaries.
- Within the contents of an AOF file (inside `OBJ_AREA` chunks), words are aligned on 4-byte boundaries and half words on 2-byte boundaries (for all current ARM-based systems).

### 3.3 Undefined Fields

Fields not explicitly defined by this specification are implicitly reserved. All such fields must be zeroed. New meaning may be ascribed to reserved fields in future versions, but will usually be done in a manner that gives no new meaning to zeroes.

---

## 4. Chunk File Format

AOF is layered on Chunk File Format, which provides a container for multiple named data chunks within a single file. A chunk file can be copied without knowledge of the contents of its chunks.

### 4.1 Chunk File Header

The chunk file header consists of two parts: a fixed-length part of three words and a variable-length array of chunk entries.

#### Fixed Header (3 words, 12 bytes)

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | ChunkFileId | 4 | Magic number: `0xC3CBC6C5`. The endianness of the chunk file can be deduced from this value. If it appears as `0xC5C6CBC3` when read as a word, each word value in the file must be byte-reversed before use. |
| 4 | maxChunks | 4 | Number of entries in the header. Fixed when the file is created. |
| 8 | numChunks | 4 | Number of chunks currently used (0 to maxChunks). This value is redundant — it can be computed by scanning the entries. |

#### Chunk Entries (4 words per entry, 16 bytes)

Starting at offset 12, there are `maxChunks` entries:

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| +0 | chunkId | 8 | An 8-byte field identifying the chunk contents. This is an 8-byte field (not a 2-word field), so it has the same byte order independent of endianness. The 8 characters are stored in ascending address order, as if they formed part of a NUL-terminated string. The first 4 bytes contain a unique authority-allocated domain name; the remaining 4 bytes identify the component within that domain. |
| +8 | fileOffset | 4 | Byte offset within the file of the start of the chunk. Must be divisible by 4 (all chunks are word-aligned). A value of **0** indicates the chunk entry is **unused**. |
| +12 | size | 4 | Exact byte size of the chunk's contents (which need not be a multiple of 4). |

### 4.2 Chunk File Layout Diagram

```
+-------------------------------+
| ChunkFileId (0xC3CBC6C5)     |  12 bytes (3 words)
| maxChunks                     |
| numChunks                     |
+-------------------------------+
| Chunk Entry 0                 |  16 bytes per entry (4 words)
|   chunkId (8 bytes)           |
|   fileOffset (4 bytes)        |
|   size (4 bytes)              |
+-------------------------------+
| Chunk Entry 1                 |
|   ...                         |
+-------------------------------+
| ...                           |
+-------------------------------+
| Chunk Entry maxChunks-1       |
+-------------------------------+
| Chunk Data (word-aligned)     |
| ...                           |
+-------------------------------+
```

### 4.3 ChunkId Domain Names

| Domain | Used By |
|--------|---------|
| `OBJ_` | ARM Object Format (AOF) files |
| `LIB_` | ARM Object Library Format (ALF) files |
| `OFL_` | Additional chunks in Object Code Libraries |

---

## 5. AOF File Structure

For AOF files, the first 4 bytes of each chunkId are `OBJ_`. AOF defines five chunks:

| Chunk | chunkId | Required | Description |
|-------|---------|----------|-------------|
| AOF Header | `OBJ_HEAD` | Yes | Fixed header and area declarations |
| Areas | `OBJ_AREA` | Yes | Code, data, and relocation directives |
| Identification | `OBJ_IDFN` | No | Tool identification string |
| Symbol Table | `OBJ_SYMT` | No | Symbol definitions and references |
| String Table | `OBJ_STRT` | No | All print names (area names, symbol names) |

Only `OBJ_HEAD` and `OBJ_AREA` must be present. A typical object file contains all five chunks.

Chunks may appear in any order within the file. The ARM C compiler and the ARM Assembler produce their AOF chunks in different orders.

A language translator or other utility may add additional chunks to an object file (e.g., a language-specific symbol table or debugging data). Space for **8 chunk entries** in the chunk file header is conventional when the AOF file is produced by a language processor generating all five standard chunks, leaving room for three additional chunks.

The AOF header chunk (`OBJ_HEAD`) should not be confused with the chunk file header.

---

## 6. The AOF Header Chunk (OBJ_HEAD)

The AOF header consists of two contiguous parts within the `OBJ_HEAD` chunk: a fixed-size part and a variable-length sequence of area headers.

### 6.1 Fixed Part (6 words, 24 bytes)

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | Object File Type | 4 | `0xC5E2D080` — marks the file as a relocatable object. The endianness of this value must be identical to the endianness of the containing chunk file. |
| 4 | Version Id | 4 | AOF version number. See version table below. |
| 8 | Number of Areas | 4 | Number of areas in the file. Equivalently, the number of area headers that follow the fixed part. Should be ≤ 255. |
| 12 | Number of Symbols | 4 | Number of symbols in the symbol table (`OBJ_SYMT`). 0 if no symbol table chunk is present. |
| 16 | Entry Area Index | 4 | 1-origin index in the area header array of the area containing the entry point. A value of **0** signifies that no program entry address is defined by this AOF file. |
| 20 | Entry Offset | 4 | Byte offset of the entry point within the entry area. The entry address is defined as: `base_address_of_entry_area + Entry Offset`. |

#### AOF Version Numbers

| Version Id | AOF Version | Notes |
|-----------|-------------|-------|
| 150 | 1.50 | Earliest documented version |
| 200 | 2.00 | Intermediate version |
| 310 (`0x136`) | 3.10 | Current version. Loosely associated with the version of Assembler/C that first produced files with these fields. |

### 6.2 Area Headers (5 words each, 20 bytes)

Following the fixed part, there are `Number of Areas` area headers, each containing:

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| +0 | Area Name | 4 | Byte offset into the string table (`OBJ_STRT`) of the area's name. Each area within an object file must have a unique name. An offset of 0 means there is no string table entry for the name (by convention, such an area may be named `areaNN` where NN is the 0-origin area number). |
| +4 | Attributes + Alignment | 4 | Bit field encoding area attributes and alignment. See section 6.3. |
| +8 | Area Size | 4 | Size of the area in bytes. Must be a multiple of 4. Unless the Zero-Initialised bit (bit 12) is set, this many bytes of data must be present for this area in `OBJ_AREA`. For common areas (bits 3 or 4 in RISC OS format, or bit 10/11 in SDT format), this is the minimum acceptable size. |
| +12 | Number of Relocations | 4 | Number of relocation directives that apply to this area (equivalently, the number of 8-byte relocation records following the area's data in `OBJ_AREA`). |
| +16 | Base Address | 4 | Used only for absolute areas (bit 8 set in attributes). Records the fixed base address at which the area must be placed. 0 if unused. Giving an area a base address prior to linking will generally cause problems unless only a single object file is involved. |

### 6.3 Attributes and Alignment Word

The least significant 8 bits encode the alignment of the area start as a power of 2. The upper 24 bits encode area attributes.

#### Alignment (Bits 0-7)

The alignment value must be between 2 and 32 inclusive. The area start address must be divisible by 2^alignment. Examples:

| Alignment Value | Resulting Alignment |
|----------------|-------------------|
| 2 | 4-byte (word) boundary |
| 3 | 8-byte boundary |
| 4 | 16-byte boundary |
| 12 | 4096-byte (page) boundary |

#### Attribute Bits (SDT / Version 3.10 Format)

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 8 | `0x00000100` | Absolute | Area has a fixed address; Base Address field is valid. Not usually set by language processors. |
| 9 | `0x00000200` | Code | Area contains code (1) or data (0). |
| 10 | `0x00000400` | Common Block Definition | Common areas with the same name are overlaid by the linker. The Area Size field defines the common block size; all other references must specify a size ≤ the definition size. Multiple definitions of the same common area must have identical contents. Can be used with bit 9 for common code blocks. |
| 11 | `0x00000800` | Common Block Reference | Precludes the area having initialising data. Effectively implies bit 12 (zero-initialised). If both bits 10 and 11 are set, bit 11 is ignored. |
| 12 | `0x00001000` | Zero-Initialised | Area has no initialising data in `OBJ_AREA`; its contents are missing from the areas chunk. The linker either includes zero-filled data or maps a zero-initialised region. Incompatible with Read-Only (bit 13). Whether a zero-initialised area is re-zeroed on image re-entry is a property of the image format and/or target system. A combination of bit 10 and bit 12 has the same meaning as bit 11. |
| 13 | `0x00002000` | Read-Only | Area will not be modified following relocation. The linker groups read-only areas together for write protection. Code areas and debugging tables must have this set. Incompatible with bit 12. |
| 14 | `0x00004000` | Position Independent | All memory address references must be link-time-fixed offsets from a base register (e.g., PC-relative branch offsets). Usually significant only for code areas. |
| 15 | `0x00008000` | Debugging Tables | Area contains symbolic debugging tables. The linker groups these together for extraction by debuggers. Bit 9 (Code) is ignored. Usually has bit 13 (Read-Only) set. |
| 16 | `0x00010000` | 32-bit PC | Code complies with a 32-bit variant of the APCS. Code areas only (bit 9 must be set). May be incompatible with 26-bit APCS code. |
| 17 | `0x00020000` | Reentrant | Code complies with a reentrant variant of the APCS. Code areas only. |
| 18 | `0x00040000` | Extended FP | Uses the ARM extended floating-point instruction set (LFM/SFM for save/restore). Code areas only. Code with this attribute may not execute on older ARM systems. |
| 19 | `0x00080000` | No SW Stack Check | Code complies with a variant of the APCS without software stack-limit checking. Code areas only. May be incompatible with limit-checked code. |
| 20 | `0x00100000` | Based / Thumb | **Data areas:** Based attribute — area is addressed via link-time-fixed offsets from a base register (encoded in bits 24-27). Based areas are treated specially by the linker for shared libraries and ROM-able code. **Code areas (SDT 2.50):** All relocations are of Thumb code. |
| 21 | `0x00200000` | Shared Library Stub Data / Halfword | **Data areas:** Shared library stub data attribute. Set by the linker, not by language processors. **Code areas (SDT 2.50):** Area may contain ARM halfword instructions (LDRH/STRH). |
| 22 | `0x00400000` | Interworking | Area is suitable for ARM/Thumb interworking. (SDT 2.50 addition.) |
| 23 | | Reserved | Must be 0. |
| 24-27 | `0x0F000000` | Base Register | For based data areas: encodes the base register number. Must be 0 if the area is not based. |
| 28-31 | | Reserved | Must be 0. |

#### Attribute Combination Rules

- Code areas (bit 9 set): bits 16-19 may be set; bits 20-22 may be set in SDT 2.50+.
- Debugging table areas (bit 15 set): bit 9 is ignored; bit 13 should be set.
- Common definition (bit 10) with bit 11 also set: bit 11 is ignored.
- Read-only (bit 13) and zero-initialised (bit 12) are mutually exclusive.
- Absolute areas (bit 8) must have a valid Base Address.
- Common areas (bit 11) must not have bit 13 (read-only) set.

#### RISC OS 2 (Earlier Format) Area Attributes

In the older RISC OS Programmer's Reference Manual format, area attributes were structured differently:

| Bits | Meaning |
|------|---------|
| 0-7 | Type bits. At most one may be set: 0=absolute, 1=relocatable, 2=code, 3=common (uninitialised), 4=common block, 5=linker use, 6=debugging tables, 7=zero-initialised |
| 8 | Area cannot contain code |
| 9 | Position independent code |
| 10 | Reentrant / common |
| 11 | Read-only data |
| 12 | Debugging table |
| 13-14 | Alignment in log2 form (0=byte, 1=halfword, 2=word) |
| 15 | Area base field is valid |
| 16-23 | Reserved, must be zero |
| 24-31 | Stub bits (used by linker) |

### 6.4 Area Ordering in Linked Images

The linker orders areas in a generated image by:

1. **Attributes** — grouping like areas together
2. **Lexicographic order of area names** — case-significant
3. **Position of the containing object module in the link list** — the position of a module loaded from a library is not predictable

---

## 7. The Areas Chunk (OBJ_AREA)

The `OBJ_AREA` chunk contains the actual area contents (code, data, debugging data) together with associated relocation directives. Its chunkId is `"OBJ_AREA"`.

### 7.1 Layout

An area is simply a sequence of byte values. The endianness of words and half words within it must agree with the containing AOF file.

```
+---------------------------+
| Area 1 Data               |  area_size bytes (from area header)
+---------------------------+
| Area 1 Relocation         |  num_relocs × 8 bytes (2 words each)
+---------------------------+
| Area 2 Data               |
+---------------------------+
| Area 2 Relocation         |
+---------------------------+
| ...                       |
+---------------------------+
| Area N Data               |
+---------------------------+
| Area N Relocation         |
+---------------------------+
```

- Each area is immediately followed by its associated table of relocation directives (if any).
- An area is either completely initialised by the values from the file, or is initialised to zero (as specified by bit 12 of its area attributes). Zero-initialised areas have **no data** in the `OBJ_AREA` chunk.
- Both area contents and relocation tables are aligned to 4-byte boundaries.
- There is data for each area whose size > 0 and which is not marked as uninitialised/zero-initialised.

---

## 8. Relocation Directives

A relocation directive describes a value that is computed at link time or load time but cannot be fixed when the object module is created. In the absence of applicable relocation directives, the value of a byte, halfword, word, or instruction from the preceding area is exactly the value that will appear in the final image.

A field may be subject to more than one relocation directive.

### 8.1 Relocation Directive Format (SDT / Version 3.10)

Each relocation directive is 2 words (8 bytes):

```
Word 0: Offset (byte offset in the preceding area of the subject field)

Word 1: Flags + SID

  31  30  29  28  27  26  25 24  23                          0
+----+-------+----+----+----+-----+---------------------------+
| 1  |  II   | B  | A  | R  | FT  |        SID (24 bits)     |
+----+-------+----+----+----+-----+---------------------------+
```

**Note:** Bit 31 must be 1 in the SDT format.

### 8.2 Field Descriptions

#### Offset (Word 0)

Byte offset within the preceding area of the subject field to be relocated.

#### SID — Subject Identifier (Bits 0-23)

The interpretation depends on the A bit (bit 27):

| A | SID Interpretation |
|---|-------------------|
| 0 | SID is the **0-origin index** of an area in the array of area headers. The relocation value is the base address of that area. |
| 1 | SID is the **0-origin index** of a symbol in the symbol table chunk (`OBJ_SYMT`). The relocation value is the value of that symbol. |

If a symbol is marked as being in a common area, then a relocation directive based on that symbol is effectively based on the area.

#### FT — Field Type (Bits 24-25)

| Value | Type | Description |
|-------|------|-------------|
| 00 | Byte | 8-bit field to be relocated |
| 01 | Halfword | 16-bit field (2 bytes) to be relocated |
| 10 | Word | 32-bit field (4 bytes) to be relocated |
| 11 | Instruction | ARM or Thumb instruction (or instruction sequence). If bit 0 of the Offset word is set, the instruction is Thumb; otherwise ARM. An ARM branch or branch-with-link instruction is always a suitable subject. |

Bytes, halfwords, and instructions may only be relocated by values of suitably small magnitude. Overflow is faulted by the linker.

#### R — PC-Relative Flag (Bit 26)

| R | Meaning |
|---|---------|
| 0 | Additive relocation |
| 1 | PC-relative relocation |

#### A — Symbol/Area Flag (Bit 27)

Selects whether SID is a symbol index (A=1) or an area index (A=0). See SID description above.

#### B — Based Area Flag (Bit 28)

| B | Meaning |
|---|---------|
| 0 | Normal relocation |
| 1 | Based area relocation |

If R=1, B is usually 0. A B value of 1 with R=1 denotes that the inter-link-unit value of a branch destination is to be used (allows compilers to perform the tail-call optimisation on reentrant code).

#### II — Instruction Sequence Constraint (Bits 29-30)

For instruction-type relocations (FT = 11), constrains how many contiguous instructions the linker may modify:

| Value | Constraint |
|-------|-----------|
| 00 | No constraint (linker may modify as many as needed) |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

### 8.3 Relocation Types

#### 8.3.1 Plain Additive Relocation (R=0, B=0)

The relocation value (symbol value or area base address) is added to the existing subject field content:

```c
subject_field = subject_field + relocation_value
```

#### 8.3.2 PC-Relative Relocation (R=1, B=0)

The subject field is adjusted by the difference between the relocation value and the base of the area containing the subject field:

```c
subject_field = subject_field + (relocation_value - base_of_area_containing(subject_field))
```

**Special case:** If A=0 and the relocation value is the base of the area containing the subject field, then it is not added:

```c
subject_field = subject_field - base_of_area_containing(subject_field)
```

This caters for relocatable PC-relative branches to fixed target addresses.

#### 8.3.3 Based Area Relocation (R=0, B=1)

The relocation value must be an address within a based data area. The subject field is incremented by the difference between this value and the base address of the consolidated based area group (the linker consolidates all areas based on the same base register into a single, contiguous region):

```c
subject_field = subject_field + (relocation_value - base_of_area_group_containing(relocation_value))
```

This is used for based-area addressing where data is accessed via a base register (e.g., `sb`). When generating reentrant code, the C compiler places address constants in an adcon area based on register `sb`, and loads them using `sb`-relative LDR instructions. At link time, separate adcon areas are merged and `sb` no longer points where presumed at compile time. Based area relocation of the LDR instructions corrects for this.

For based area relocations: bits 29-30 must be 0; bit 31 must be 1.

### 8.4 RISC OS 2 (Earlier Format) Relocation Directive

In the older format, the flags word was structured differently:

| Bits | Meaning |
|------|---------|
| 0 | PC-relative relocation flag |
| 8-11 | A-field specifier: describes how to extract the A-field from the word being relocated. 0 = no A-field (addend is in the following word). |
| 12-15 | R-type field specifier (always 0 = word-wide in version 1.50) |
| 16-23 | SID number. Either the symbol table index, or if bit 23 is set, the remaining 7 bits specify the area number. |

---

## 9. Symbol Table Chunk (OBJ_SYMT)

The `Number of Symbols` field in the AOF header fixed part defines the entry count. Each entry is **4 words (16 bytes)**.

### 9.1 Symbol Table Entry Format

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| +0 | Name | 4 | Byte offset in the string table (`OBJ_STRT`) of the symbol name. An offset of 0 means there is no name string. |
| +4 | Attributes | 4 | Symbol attribute flags. See section 9.3. |
| +8 | Value | 4 | Interpretation depends on symbol type; see section 9.2. |
| +12 | Area Name | 4 | Byte offset in the string table of the name of the area defining this symbol. Meaningful only for non-absolute defining occurrences. Must be 0 otherwise. The linker combines the base of the named area with the symbol's Value to produce the symbol's absolute value at link time. |

### 9.2 Value Field Semantics

| Condition | Value Interpretation |
|-----------|---------------------|
| Absolute symbol (bits 0, 2 set) | The absolute value of the symbol (e.g., a constant). |
| Common symbol (bit 6 set) | The byte length of the referenced common area. |
| Non-absolute defining occurrence (bit 0 set, bit 2 unset) | Offset from the base address of the area named by Area Name. |
| External reference (bit 0 unset) | Value and Area Name should both be 0. |

### 9.3 Symbol Attributes (SDT / Version 3.10 Format)

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 0 | `0x00000001` | Defined | Symbol is defined in this object file. |
| 1 | `0x00000002` | Global | Symbol has global scope; can be matched by the linker to a similarly named symbol from another object file. |
| 2 | `0x00000004` | Absolute | Symbol has an absolute value (e.g., a constant). Not relative to any area base. Meaningful only if bit 0 is set. |
| 3 | `0x00000008` | Case-Insensitive | Linker ignores case when matching this symbol. Only meaningful for external references (bit 0 unset). |
| 4 | `0x00000010` | Weak | Acceptable for the reference to remain unsatisfied; unrelocated fields remain as-is. Linker ignores weak references when selecting library members. Meaningful only for external references (bits 1,0 = 10). |
| 5 | `0x00000020` | Strong | Enables link-time indirection. A strong definition of symbol S causes all non-strong definitions of S to be resolved to the strong definition for references from outside the non-strong definition's file. Within the file containing the strong definition, references resolve to the non-strong definition. Usually, a strong definition is absolute. Meaningful only for external defining occurrences (bits 1,0 = 11). |
| 6 | `0x00000040` | Common | Symbol is a reference to a common area with the symbol's name. The Value field gives the byte length. All common symbols with the same name are assigned the same base address by the linker. If the name matches a common area, they are merged. |
| 7 | | Reserved | Must be 0. |
| 8 | `0x00000100` | Code Datum | Symbol identifies a datum (usually read-only) in a code area, not an executable instruction. Meaningful only for symbols in code areas. |
| 9 | `0x00000200` | FP Args in FP Regs | Symbol identifies a function entry point that passes FP arguments in FP registers. A reference with this attribute cannot be matched by the linker to a definition lacking it. |
| 10-11 | | Reserved / Leaf | Must be 0. Bit 11 encodes the simple leaf function attribute in some implementations: the function's inter-link-unit entry point equals its intra-link-unit entry point. |
| 12 | `0x00001000` | Thumb | Symbol is a Thumb symbol. (SDT 2.50 addition.) |
| 13-31 | | Reserved | Must be 0. |

### 9.4 Scope Combinations (Bits 0-1)

| Bit 1 | Bit 0 | Meaning |
|-------|-------|---------|
| 0 | 0 | Reserved (not permitted). |
| 0 | 1 | **Local definition** — defined with scope limited to this object file. The linker only matches this symbol to references from within the same object file. |
| 1 | 0 | **External reference** — reference to a symbol defined in another object file. If no defining instance is found, the linker attempts to match the name to common block names. |
| 1 | 1 | **Global definition** — defined with global scope. The linker matches this definition to references from other object files. |

### 9.5 RISC OS 2 (Earlier Format) Symbol Attributes

In the older RISC OS format, symbol attributes were arranged differently:

| Bits | Meaning |
|------|---------|
| 0 | Defined in this file (LL0) |
| 1 | Exported/global (GBLL1) |
| 2 | Weak override definition (WKLL2). Must not have both bits 1 and 2 set. |
| 3 | Common (CMLL3) |
| 4 | Constant value / common size (CSLL4). Must be set if bit 3 is set. |
| 5 | Case-insensitive (CISL5) |
| 6 | Weak (WKLL6). Can be weak and global. Cannot be weak and case-insensitive. |
| 7 | Strong (STLL7) |
| 8-15 | Not used |
| 16-23 | Datum and type field. Interpretation is overlaid depending on symbol type. |
| 24-27 | Symbol type (architecture-specific). Values 0-8 reserved. 9=FP size/register, 10=scratch register, 11=non-scratch register, 12-13=coprocessor register. |
| 28-31 | Reserved |

---

## 10. The String Table Chunk (OBJ_STRT)

The string table contains all print names referenced from the AOF header and symbol table chunks. This separation factors out the variable-length nature of names from the primary data formats.

### 10.1 Format

- A print name is stored as a sequence of non-control characters (codes 32-126 and 160-255) terminated by a NUL (0) byte.
- Names are identified by their byte offset from the start of the table.
- The **first 4 bytes** of the string table contain its length in bytes, **including the length word itself**.
- No valid offset is less than 4.
- No table has length less than 4.
- The endianness of the length word must match the containing AOF and chunk file.

### 10.2 Layout

```
+-------------------+
| Length (4 bytes)   |  Total size of table including this word
+-------------------+
| "name1\0"         |  First string (at offset 4)
+-------------------+
| "name2\0"         |  Second string
+-------------------+
| ...               |
+-------------------+
```

Strings are packed end-to-end with no alignment padding between them. No alignment restrictions apply.

---

## 11. The Identification Chunk (OBJ_IDFN)

This chunk contains a NUL-terminated string of printable characters (codes 10-13 and 32-126) identifying the name and version of the tool that generated the object file.

Use of character codes 128-255 is discouraged as their interpretation is host-dependent.

---

## 12. Linker Symbol Table Output Format

The linker can write its internal symbol table to a file for debugging and browsing purposes. The format is a table of NUL-terminated strings with no padding:

```
name,value,class[,definingmodulename]
```

| Field | Description |
|-------|-------------|
| name | Symbol name (at least one printable character). |
| value | Hex value prefixed by `&` (BBC BASIC style), e.g., `&00008000`. |
| class | One of: `Abs` (absolute), `RelocDef` (defined but unreferenced), `RelocRef` (referenced in one or more modules), `ExtDef` (defined and referenced across modules), `ExtRef` (referenced but not defined). |
| definingmodulename | Optional. Name of a module in which the symbol is defined or referenced. |

---

## 13. Summary of Constants

| Constant | Value | Description |
|----------|-------|-------------|
| ChunkFileId | `0xC3CBC6C5` | Chunk file magic number |
| ChunkFileId (byte-reversed) | `0xC5C6CBC3` | Indicates opposite endianness |
| Object File Type | `0xC5E2D080` | Relocatable object file marker |
| AOF Version (v1.50) | `150` | Earliest documented version |
| AOF Version (v2.00) | `200` | Intermediate version |
| AOF Version (v3.10, current) | `310` (`0x136`) | Current version |
| AOF chunk prefix | `OBJ_` | First 4 bytes of all AOF chunkIds |

### Chunk Identifiers

| chunkId (8 bytes) | Description |
|-------------------|-------------|
| `OBJ_HEAD` | AOF header (fixed part + area declarations) |
| `OBJ_AREA` | Area data and relocation directives |
| `OBJ_IDFN` | Tool identification string |
| `OBJ_SYMT` | Symbol table |
| `OBJ_STRT` | String table |

---

## 14. References

- ARM SDT Reference Guide, ARM DUI 0041C, Chapter 15: "ARM Object Format", ARM Limited, 1997-1998
- RISC OS Programmer's Reference Manuals, Volume 4, Appendix D: "Code file formats"
- ARM Technical Specifications (3DO edition), Chapter 2: "ARM Object Format"
