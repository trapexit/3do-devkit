# ARM Object Format (AOF) - Complete File Format Specification

**Version:** 310 (0x136)
**Last Updated:** 2026-02-07
**Based on:** ARM DUI 0041C, RISC OS Programmer's Reference Manuals

---

## Table of Contents

1. [Overview](#overview)
2. [Chunk File Format Foundation](#chunk-file-format-foundation)
3. [AOF Structure](#aof-structure)
4. [Header Chunk (OBJ_HEAD)](#header-chunk-objhead)
5. [Areas Chunk (OBJ_AREA)](#areas-chunk-objarea)
6. [Symbol Table Chunk (OBJ_SYMT)](#symbol-table-chunk-objsymt)
7. [String Table Chunk (OBJ_STRT)](#string-table-chunk-objstrt)
8. [Identification Chunk (OBJ_IDFN)](#identification-chunk-objidfn)
9. [Relocation Directives](#relocation-directives)
10. [C Structure Definitions](#c-structure-definitions)

---

## Overview

**ARM Object Format (AOF)** is the standard object file format used by ARM language processors (compilers, assemblers) such as CC, ObjAsm, and the ARM Assembler. AOF is designed to be a flexible, extensible format that supports:

- Multiple code and data areas
- Symbol tables for linking
- Relocation directives for address calculation
- Debugging information
- Multiple target platforms

### Key Characteristics

- **Modular Design:** Separate chunks for different data types
- **Extensible:** Additional chunks can be added by tools
- **Endianness Support:** Supports both little-endian and big-endian targets
- **Relocatable:** All references are relative, suitable for linking
- **Symbol Management:** Complete symbol tables with attributes
- **Debug Support:** Can carry debugging information with code

### Typical Processing Pipeline

```
Source Code
    ↓
Compiler/Assembler
    ↓
AOF Object File ← (This specification)
    ↓
Linker
    ↓
AIF/Executable Image
```

---

## Chunk File Format Foundation

### Overview

AOF files are built on top of **Chunk File Format**, a general-purpose format for storing multiple independent data sections in a single file.

### Chunk File Header Structure

The chunk file header has two parts:

**Part 1: Fixed Header (3 words)**

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x00 | ChunkFileId | DWord | Marks file as chunk format: `0xC3CBC6C5` |
| 0x04 | max_chunks | Word | Number of chunk table entries (fixed at file creation) |
| 0x08 | num_chunks | Word | Number of chunks currently used (0 ≤ num_chunks ≤ max_chunks) |

**ChunkFileId Details:**
- Value: `0xC3CBC6C5` (ASCII: "C3 CB C6 C5")
- Endianness detection: If appears as `0xC5C6CBC3` when read, file is opposite endianness
- Must be first thing in every chunk file

**Part 2: Chunk Directory (4 words per chunk entry)**

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x0C + N×16 | chunkId | 8 bytes | 8-character chunk identifier |
| 0x0C + N×16 + 8 | file_offset | Word | Byte offset of chunk data (must be 4-byte aligned) |
| 0x0C + N×16 + 12 | size | Word | Exact byte size of chunk (need not be 4-byte aligned) |

### Chunk Identification

The 8-character `chunkId` is split into two 4-character parts:

**First 4 characters:** Universal domain name
- `OBJ_` = ARM Object Format
- `LIB_` = Library Format
- `OFL_` = Object File Library extensions

**Next 4 characters:** Component type within domain
- `HEAD` = Header chunk
- `AREA` = Areas chunk
- `SYMT` = Symbol table
- `STRT` = String table
- `IDFN` = Identification
- `DIRY` = Directory (library)
- `TIME` = Time stamp

**Storage:** Characters stored in ascending address order, independent of endianness

### Example: Chunk File Header

```
Offset  Hex Value   Description
──────────────────────────────────────────────────────
0x00    C3CBC6C5    ChunkFileId (chunk file magic)
0x04    00000008    max_chunks = 8
0x08    00000005    num_chunks = 5

Chunk 0 (Header):
0x0C    4F424A5F    "OBJ_" (first part)
0x10    48454144    "HEAD" (second part)
0x14    0000000C    file_offset = 0x0C (immediately after directory)
0x18    00000018    size = 24 bytes

Chunk 1 (Areas):
0x1C    4F424A5F    "OBJ_"
0x20    41524541    "AREA"
0x24    00000024    file_offset = 0x24
0x28    00001000    size = 4096 bytes

... and so on for remaining chunks
```

---

## AOF Structure

### Required and Optional Chunks

| Chunk Name | Status | Purpose |
|-----------|--------|---------|
| OBJ_HEAD | **Required** | Object file header with area declarations |
| OBJ_AREA | **Required** | Code, data, and relocation directives |
| OBJ_SYMT | Optional | Symbol table with definitions and references |
| OBJ_STRT | Optional | String table for names |
| OBJ_IDFN | Optional | Identification string (tool that created file) |

**Minimum Viable AOF:** OBJ_HEAD + OBJ_AREA

**Typical Complete AOF:** All five chunks

### Chunk Ordering

Chunks can appear in any order in the file. However:

- The ChunkFile header must come first (always at offset 0)
- Chunks are typically ordered by type (HEAD, AREA, IDFN, SYMT, STRT)
- Space convention: AOF created by language processors reserves 8 chunk slots

### Standard AOF File Layout

```
Offset      Contents
───────────────────────────────────────────────
0x0000      Chunk File Header (12 + 8×16 = 140 bytes for 8 chunks)
0x008C
            OBJ_HEAD chunk
            ├─ Fixed header (6 words)
            └─ Area declarations (5 words each)

            OBJ_AREA chunk
            ├─ Area 1 data
            ├─ Area 1 relocations
            ├─ Area 2 data
            ├─ Area 2 relocations
            └─ ...

            OBJ_IDFN chunk (optional)

            OBJ_SYMT chunk (optional)

            OBJ_STRT chunk (optional)
```

---

## Header Chunk (OBJ_HEAD)

### Overview

The AOF header chunk contains metadata about the object file and declarations of all code/data areas.

**Structure:**
1. Fixed part (6 words)
2. Variable part (5 words per area)

### Fixed Header Part (6 words)

| Offset | Word# | Field | Type | Description |
|--------|-------|-------|------|-------------|
| 0x00 | 0 | Object File Type | DWord | File type identifier: `0xC5E2D080` |
| 0x04 | 1 | Version ID | Word | Version number (current: 310 = 0x136) |
| 0x08 | 2 | Number of Areas | Word | Count of area declarations following |
| 0x0C | 3 | Number of Symbols | Word | Count of symbol table entries (if OBJ_SYMT present) |
| 0x10 | 4 | Entry Area Index | Word | 1-origin index of area containing entry point (0 = none) |
| 0x14 | 5 | Entry Offset | Word | Byte offset within entry area of entry point |

**Field Details:**

**Object File Type (0x00):**
- Value: `0xC5E2D080` (unique identifier for AOF)
- Byte order: Must match chunk file endianness
- Fixed: Always this value for AOF files

**Version ID (0x04):**
- Current value: 310 (0x00000136)
- Chosen for association with ARM assembler/compiler version
- Future versions use higher numbers
- Format: Single 32-bit unsigned integer

**Number of Areas (0x08):**
- Typical range: 1-20
- Common: 2-4 (code, data, debug info)
- Must match number of area declarations below

**Number of Symbols (0x0C):**
- Set only if OBJ_SYMT chunk present
- Can be 0 (object file without symbol table)
- If non-zero, OBJ_SYMT chunk must exist with this many entries

**Entry Area Index (0x10):**
- 1-origin index into area array
- Value 0: No entry point defined
- Value 1: Entry in first area
- Value N: Entry in Nth area
- Must be ≤ Number of Areas

**Entry Offset (0x14):**
- Byte offset from area base to entry point
- Only meaningful if Entry Area Index ≠ 0
- Value ignored if Entry Area Index = 0

### Area Declarations (5 words each)

Immediately following the fixed header are area declarations:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x18 + N×20 | Area Name | Word | Offset in OBJ_STRT (0 = unnamed, "areaXX" convention) |
| 0x1C + N×20 | Attributes | Word | Bit-mapped attributes and alignment |
| 0x20 + N×20 | Area Size | Word | Size in bytes (must be multiple of 4) |
| 0x24 + N×20 | Number of Relocs | Word | Count of relocation directives for this area |
| 0x28 + N×20 | Area Base / Reserved | Word | Base address (for absolute) or 0 (reserved) |

**Area Name (Offset in String Table):**
- Byte offset into OBJ_STRT chunk
- Value 0: Area has no string name (use "area00", "area01", etc.)
- Valid offsets: ≥ 4 (after string table length word)

**Attributes & Alignment Word:**

Bit layout:
```
Bits 0-7:   Type bits (at most one set)
Bits 8-15:  Area Type attributes
Bits 16-23: Additional attributes (code areas only)
Bits 24-31: Stub bits (linker use)
Bits 0-7:   Alignment bits (deprecated, see bits 13-14)
```

**Type Bits (0-7) - At most one set:**

| Bit | Mask | Type | Description |
|-----|------|------|-------------|
| 0 | 0x00000001 | Absolute | Area has fixed base address |
| 1 | 0x00000002 | Relocatable | Area can be relocated (default) |
| 2 | 0x00000004 | Code | Area contains executable code |
| 3 | 0x00000008 | Common | Uninitialized data (can share space) |
| 4 | 0x00000010 | Common Block | Definition area for common blocks |
| 5 | 0x00000020 | Linker-only | Reserved for linker use |
| 6 | 0x00000040 | Debug | Area contains debugging tables |
| 7 | 0x00000080 | Zero-Init | Zero-initialized data (BSS) |

**Area Type Attributes (8-15):**

| Bit | Mask | Attribute | Meaning |
|-----|------|-----------|---------|
| 8 | 0x00000100 | No Code | 1 = area cannot contain executable code |
| 9 | 0x00000200 | PIC | 1 = position-independent code (PC-relative only) |
| 10 | 0x00000400 | Reentrant | 1 = reentrant/common block |
| 11 | 0x00000800 | Read-only | 1 = read-only data (never written) |
| 12 | 0x00001000 | Debug | 1 = debugging table area |
| 13-14 | 0x00006000 | Alignment | Alignment in log₂ form (0-3 = 2⁰ to 2³) |
| 15 | 0x00008000 | Base Valid | 1 = area base field is valid (absolute area) |

**Additional Code Area Attributes (16-23):**

| Bit | Mask | Attribute | Meaning |
|-----|------|-----------|---------|
| 16 | 0x00010000 | 32-bit PC | 1 = complies with 32-bit APCS (ARM Procedure Calling Standard) |
| 17 | 0x00020000 | Reentrant | 1 = reentrant code |
| 18 | 0x00040000 | FP Extend | 1 = uses extended FP instruction set (LFM/SFM) |
| 19 | 0x00080000 | No Stack Check | 1 = no software stack checking |
| 20 | 0x00100000 | Thumb | 1 = Thumb code area (not ARM) |
| 21 | 0x00200000 | Halfword | 1 = may contain ARM halfword instructions |
| 22 | 0x00400000 | Interwork | 1 = suitable for ARM/Thumb interworking |

**Stub Bits (24-31) - Linker Use:**
- Initialized as 0 or undefined by compiler
- Modified and used by linker
- Should not be set in file for library inclusion

**Area Size:**
- Size in bytes
- Common areas: Minimum acceptable size
- Zero-init areas: Must be multiple of 4
- Other areas: Must match data in OBJ_AREA chunk

**Number of Relocations:**
- Count of relocation directives for this area
- Relocations follow area data in OBJ_AREA chunk
- Can be 0 (no relocations)

**Area Base / Reserved:**
- For absolute areas (bit 0 set): Contains base address
- For other areas: Must be 0 (reserved)
- Used by linker to ensure fixed placement

### Example: Header Chunk

```
Offset  Hex Value   Description
──────────────────────────────────────────────────────
Fixed Header:
0x00    C5E2D080    Object file type identifier
0x04    00000136    Version 310
0x08    00000003    3 areas
0x0C    00000005    5 symbols
0x10    00000001    Entry area = 1 (first area)
0x14    00000080    Entry offset = 0x80

Area 0 (Code):
0x18    00000000    Area name offset = 0 (unnamed: "area0")
0x1C    00002204    Attributes: code (bit 9), 4-byte align (bits 13-14)
0x20    00000200    Size = 512 bytes
0x24    00000004    4 relocation directives
0x28    00000000    Reserved (not absolute)

Area 1 (Data):
0x2C    0000000A    Area name offset = 10 ("data")
0x30    00000008    Attributes: read-only
0x34    00000100    Size = 256 bytes
0x38    00000000    No relocations
0x3C    00000000    Reserved

Area 2 (Zero-init):
0x40    00000010    Area name offset = 16 ("bss")
0x44    00001100    Attributes: zero-init (bit 7), 4-byte align
0x48    00001000    Size = 4096 bytes
0x4C    00000000    No relocations
0x50    00000000    Reserved
```

---

## Areas Chunk (OBJ_AREA)

### Overview

The OBJ_AREA chunk contains the actual code, data, and relocation information for each area declared in the header.

### Layout

For each area (in order):
1. Area data (if size > 0 or uninitialized bit set)
2. Relocation directives (count specified in header)

```
Area 0 data
Area 0 relocations (if any)
Area 1 data
Area 1 relocations (if any)
...
Area N data
Area N relocations (if any)
```

**Alignment:** 4-byte boundaries between areas and relocations

### Area Data

**Format:** Raw binary code or initialized data

**Properties:**
- Bytes stored in target endianness
- Size must match "Area Size" from header
- Uninitialized areas (bit 12 of attributes) have no data
- Alignment depends on area attributes

**Example (512-byte code area):**
```
Offset          Contents
────────────────────────────────
0x0000-0x01FF   512 bytes of ARM code
```

### Relocation Directives

Each relocation is exactly 8 bytes (2 words):

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0 | Offset | Word | Byte offset within area of field to relocate |
| +4 | Flags & SID | Word | Bit-mapped relocation information |

**Relocation Offset:**
- Byte offset from start of area data
- Must point to valid field type (byte, halfword, word, instruction)

**Flags & SID Word (bits):**

```
Bit 0:          R bit (relocation type)
Bits 1-7:       Reserved
Bits 8-11:      Field specifier (A-field extraction)
Bits 12-15:     R-field specifier (usually 0 = word-wide)
Bits 16-23:     SID (Symbol ID or Area index)
Bit 24:         B bit (branch type)
Bits 25-26:     Reserved
Bit 27:         A bit (address type selector)
Bits 28-29:     II bits (instruction count constraint)
Bit 30:         Reserved
Bit 31:         Instruction bit
```

**Bit Fields Detailed:**

| Field | Bits | Type | Meaning |
|-------|------|------|---------|
| Field Type | 24-25 | FT | 00=byte, 01=halfword, 10=word, 11=instruction |
| Instruction Bits | 29-30 | II | Instruction constraint (00=none, 01=≤1, 10=≤2, 11=≤3) |
| R Bit | 26 | - | 1=PC-relative, 0=additive |
| B Bit | 28 | - | 1=branch form (rarely used) |
| A Bit | 27 | - | 1=symbol, 0=area |
| SID | 0-23 | - | Symbol index (if A=1) or area index (if A=0) |

**Relocation Types:**

| R | B | Effect |
|---|---|--------|
| 0 | 0 | Plain additive: `field = field + reloc_value` |
| 1 | 0 | PC-relative: `field = field + (reloc_value - area_base)` |
| 0 | 1 | Based area: `field = field + (reloc_value - base_group)` |

---

## Symbol Table Chunk (OBJ_SYMT)

### Overview

The symbol table contains definitions and references to symbols that may be linked with other object files.

### Symbol Table Entry Format

Each entry is exactly 16 bytes (4 words):

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0 | Name | Word | Offset into OBJ_STRT (0 = unnamed) |
| +4 | Attributes | Word | Bit-mapped symbol attributes |
| +8 | Value | Word | Symbol value (context-dependent) |
| +12 | Area Name | Word | Offset into OBJ_STRT for area name |

### Name Field

- Byte offset into string table
- 0 = symbol has no string name
- Otherwise points to null-terminated string

### Attributes Field (32 bits)

| Bit | Mask | Attribute | Meaning |
|-----|------|-----------|---------|
| 0 | 0x00000001 | Defined | 1 = symbol defined in this file |
| 1 | 0x00000002 | Global | 1 = symbol has global scope |
| 2 | 0x00000004 | Absolute | 1 = symbol has absolute value |
| 3 | 0x00000008 | Case-Insensitive | 1 = case-insensitive matching |
| 4 | 0x00000010 | Weak | 1 = weak definition (can be overridden) |
| 5 | 0x00000020 | Reserved | Must be 0 |
| 6 | 0x00000040 | Common | 1 = common symbol (size in value) |
| 7 | 0x00000080 | Strong | 1 = strong definition |
| 8-15 | 0x0000FF00 | Reserved | Must be 0 |
| 16-23 | 0x00FF0000 | Type/Datum | Interpretation-dependent |
| 24-27 | 0x0F000000 | Symbol Type | Type code (0-15, codes 0-8 reserved) |
| 28-31 | 0xF0000000 | Reserved | Must be 0 |

**Scope Bits (0-1):**

| Bits | Meaning |
|------|---------|
| 00 | Reserved |
| 01 | Defined locally (bits 1=0, 0=1) |
| 10 | Undefined reference (bits 1=1, 0=0) |
| 11 | Defined globally (bits 1=1, 0=1) |

**Symbol Type Values (bits 24-27):**

| Value | Type | Description |
|-------|------|-------------|
| 0 | - | Not used |
| 1-8 | - | Reserved |
| 9 | FP Argument | FP number size in bits 16-17, register in bits 18-23 |
| 10 | Register | Register number in bits 16-23 (scratch) |
| 11 | Register | Register number in bits 16-23 (preserved) |
| 12 | Coprocessor | Coprocessor register (R = 256×CN + REGNO) |
| 13 | Coprocessor | Coprocessor register (preserved) |
| 14-15 | - | Reserved |

### Value Field

**For Absolute Symbols (bit 2 set):**
- Contains absolute constant value
- Not relative to any area

**For Area-Relative Symbols (bit 2 clear):**
- Contains byte offset from area base
- Combined with area base by linker
- Area name given in Area Name field

**For Common Symbols (bit 6 set):**
- Contains size of common block in bytes
- All definitions with same name must have same size
- Linker allocates single block shared by all references

### Area Name Field

- Byte offset into string table
- Only meaningful if symbol is non-absolute defined (bits 0=1, 2=0)
- Points to name of area containing symbol
- 0 if symbol is absolute or undefined

### Symbol Table Example

```
Symbol 0 (function entry):
Offset  Hex Value   Description
──────────────────────────────────────────────────────
0x00    00000004    Name offset = 4 ("main")
0x04    00000003    Attributes: defined (1), global (1), local base ref
0x08    00000000    Value = 0 (at start of area)
0x0C    00000000    Area name offset = 0 (area 0)

Symbol 1 (external variable):
0x10    0000000A    Name offset = 10 ("count")
0x14    00000002    Attributes: undefined (global ref)
0x18    00000000    Value = 0 (undefined, will be resolved)
0x1C    00000000    Area name = 0 (undefined)

Symbol 2 (constant):
0x20    00000010    Name offset = 16 ("MAX_SIZE")
0x24    00000005    Attributes: defined, absolute
0x28    00001000    Value = 0x1000 (4096)
0x2C    00000000    Area name = 0 (absolute, no area)

Symbol 3 (common):
0x30    0000001A    Name offset = 26 ("workspace")
0x34    00000043    Attributes: defined, global, common
0x38    00000100    Value = 256 (size of common block)
0x3C    00000000    Area name = 0 (common, no area)
```

---

## String Table Chunk (OBJ_STRT)

### Overview

The string table contains all print names referenced from other chunks.

### Layout

```
Offset  Contents
────────────────────────────────
0x00    Length word (4 bytes, includes itself)
0x04    String 1 (null-terminated)
        String 2 (null-terminated)
        String 3 (null-terminated)
        ...
        Padding (if needed)
```

### Format

**Length Word (first 4 bytes):**
- 32-bit unsigned integer
- Contains total length of string table (including length word)
- Minimum value: 4 (empty table)
- Byte order: Must match AOF endianness

**Strings:**
- Null-terminated (0x00 terminator included)
- Non-control characters only (codes 32-126, 160-255)
- Aligned on byte boundaries (no padding between strings)
- Stored in ascending address order

**String References:**
- All references are byte offsets from start of string table (offset 0)
- No valid offset < 4 (reserved for length word)
- Offset points to first character of string (not length)

### String Table Rules

**Minimum Requirements:**
- Must exist if any other chunk references strings
- Length field must be ≥ 4 (even if empty)
- Valid offsets: ≥ 4

**Identification of Strings:**
- By offset, not by count
- Tools must scan table to find end of string

**Example String Table:**

```
Hex Offset  ASCII       Description
────────────────────────────────────────────
0x00        0000001C    Length word = 28 bytes
0x04        00          Start of first string
0x05        6D 61 69 6E "main" + null
            00
0x0A        64 61 74 61 "data" + null
            00
0x0F        62 73 73    "bss" + null
            00
0x13        69 6E 69    "init" + null
            74 00
0x19        58          Padding/unused
            XX XX XX
0x1C        ...         Next chunk
```

---

## Identification Chunk (OBJ_IDFN)

### Overview

The identification chunk contains a string identifying the tool that created the object file.

### Format

Simple null-terminated string:

```
Contents
────────────────────────
Tool name/version string (null-terminated)
```

**Example values:**
- `ARM Assembler version 2.57`
- `ARM C Compiler version 3.00`
- `ObjAsm version 1.20`
- `Custom Assembler v1.0`

**Properties:**
- Single string, no length prefix
- Null-terminated
- Printable characters (codes 10-13, 32-126)
- Use of codes 128-255 discouraged (host-dependent interpretation)
- Typically 20-50 characters

---

## Relocation Directives

### Complete Relocation Formula

Relocation adjusts field values based on:
1. Field type (byte, halfword, word, instruction)
2. Relocation type (additive, PC-relative, based area)
3. Target (symbol or area)
4. Instruction constraints

### Basic Relocation Types

**Plain Additive (R=0, B=0):**
```
new_value = old_value + relocation_amount
relocation_amount = symbol_value (if A=1) OR area_base (if A=0)
```

**PC-Relative (R=1, B=0):**
```
new_value = old_value + (relocation_amount - containing_area_base)
relocation_amount = symbol_value (if A=1) OR area_base (if A=0)
Special case: If area references itself, relocation = 0
```

**Based Area (R=0, B=1):**
```
new_value = old_value + (relocation_amount - base_of_group)
Only valid for register-based addressing
```

### Field Types

| FT | Type | Size | Constraints |
|----|----|------|-------------|
| 00 | Byte | 1 | Only small relocations (byte range) |
| 01 | Halfword | 2 | Only small relocations (halfword range) |
| 10 | Word | 4 | Standard relocation (full 32-bit) |
| 11 | Instruction | 4 | ARM/Thumb instruction, special constraints |

**Instruction Type Constraints (II bits):**

| II | Constraint | Meaning |
|----|------------|---------|
| 00 | None | Can use multiple instructions |
| 01 | At most 1 | Only 1 instruction can be modified |
| 10 | At most 2 | At most 2 instructions can be modified |
| 11 | At most 3 | At most 3 instructions can be modified |

### Common Relocation Patterns

**Loading Address (MOV/MOVT):**
```
Pattern: MOV r0, #addr_bits (or MOVW/MOVT)
Type: Instruction, II=01 or II=10
Effect: Place address constant in register
```

**Branch Target (B/BL):**
```
Pattern: B/BL offset
Type: Instruction, R=1 (PC-relative)
Effect: Jump to address with PC-relative offset
```

**Data Reference:**
```
Pattern: [r0, r1, r2...] = address (in data section)
Type: Word, R=0 (additive)
Effect: Store absolute address in memory
```

---

## C Structure Definitions

### Chunk File Header

```c
#ifndef AOF_CHUNK_H
#define AOF_CHUNK_H

#include <stdint.h>

/* Chunk file magic number */
#define CHUNK_FILE_ID 0xC3CBC6C5

/* Chunk file header - 3 words */
typedef struct {
    uint32_t chunk_file_id;     /* 0x00: Chunk file identifier */
    uint32_t max_chunks;        /* 0x04: Max number of chunk entries */
    uint32_t num_chunks;        /* 0x08: Currently used chunks */
} ChunkFileHeader;

_Static_assert(sizeof(ChunkFileHeader) == 12, "Chunk header must be 12 bytes");

/* Chunk directory entry - 4 words */
typedef struct {
    char chunk_id[8];           /* 0x00: 8-char chunk identifier */
    uint32_t file_offset;       /* 0x08: Byte offset (4-byte aligned) */
    uint32_t size;              /* 0x0C: Exact byte size */
} ChunkDirectoryEntry;

_Static_assert(sizeof(ChunkDirectoryEntry) == 16, "Chunk entry must be 16 bytes");

#endif /* AOF_CHUNK_H */
```

### AOF Header Chunk

```c
#ifndef AOF_HEADER_H
#define AOF_HEADER_H

#include <stdint.h>

/* AOF file type identifier */
#define AOF_FILE_TYPE 0xC5E2D080

/* AOF version */
#define AOF_VERSION 310

/* AOF fixed header - 6 words */
typedef struct {
    uint32_t file_type;         /* 0x00: File type (0xC5E2D080) */
    uint32_t version_id;        /* 0x04: Version (310) */
    uint32_t num_areas;         /* 0x08: Number of areas */
    uint32_t num_symbols;       /* 0x0C: Number of symbols (if OBJ_SYMT present) */
    uint32_t entry_area;        /* 0x10: Entry area index (1-origin, 0=none) */
    uint32_t entry_offset;      /* 0x14: Entry offset within entry area */
} AOFHeader;

_Static_assert(sizeof(AOFHeader) == 24, "AOF header must be 24 bytes");

/* Area attributes bits */
#define AOF_AREA_ABSOLUTE   0x00000001
#define AOF_AREA_RELOCAT    0x00000002
#define AOF_AREA_CODE       0x00000004
#define AOF_AREA_COMMON     0x00000008
#define AOF_AREA_COMMON_BLK 0x00000010
#define AOF_AREA_DEBUG      0x00000040
#define AOF_AREA_ZI         0x00000080

#define AOF_ATTR_NO_CODE    0x00000100
#define AOF_ATTR_PIC        0x00000200
#define AOF_ATTR_REENTRANT  0x00000400
#define AOF_ATTR_READONLY   0x00000800
#define AOF_ATTR_DEBUG      0x00001000
#define AOF_ATTR_ALIGN_MASK 0x00006000
#define AOF_ATTR_BASE_VALID 0x00008000

#define AOF_ATTR_32BIT      0x00010000
#define AOF_ATTR_REENT_CODE 0x00020000
#define AOF_ATTR_FP_EXT     0x00040000
#define AOF_ATTR_NO_STACK_CK 0x00080000
#define AOF_ATTR_THUMB      0x00100000
#define AOF_ATTR_HALFWORD   0x00200000
#define AOF_ATTR_INTERWORK  0x00400000

/* Area declaration - 5 words */
typedef struct {
    uint32_t area_name;         /* Offset in string table */
    uint32_t attributes;        /* Bit-mapped attributes */
    uint32_t area_size;         /* Size in bytes */
    uint32_t num_relocs;        /* Number of relocation directives */
    uint32_t area_base;         /* Base (for absolute) or reserved */
} AOFAreaDeclaration;

_Static_assert(sizeof(AOFAreaDeclaration) == 20, "Area declaration must be 20 bytes");

#endif /* AOF_HEADER_H */
```

### AOF Area and Relocation

```c
#ifndef AOF_AREA_H
#define AOF_AREA_H

#include <stdint.h>

/* Relocation directive - 2 words */
typedef struct {
    uint32_t offset;            /* Offset within area to relocate */
    uint32_t reloc_info;        /* Flags and SID */
} AOFRelocationDirective;

_Static_assert(sizeof(AOFRelocationDirective) == 8, "Relocation must be 8 bytes");

/* Relocation info field bits */
#define AOF_RELOC_FT_MASK    0x03000000   /* Field type (bits 24-25) */
#define AOF_RELOC_FT_BYTE    0x00000000
#define AOF_RELOC_FT_HALF    0x01000000
#define AOF_RELOC_FT_WORD    0x02000000
#define AOF_RELOC_FT_INSTR   0x03000000

#define AOF_RELOC_R_BIT      0x04000000   /* PC-relative bit */
#define AOF_RELOC_B_BIT      0x10000000   /* Branch bit */
#define AOF_RELOC_A_BIT      0x08000000   /* Address type (1=symbol, 0=area) */

#define AOF_RELOC_II_MASK    0x60000000   /* Instruction constraint */
#define AOF_RELOC_II_NONE    0x00000000
#define AOF_RELOC_II_1       0x20000000
#define AOF_RELOC_II_2       0x40000000
#define AOF_RELOC_II_3       0x60000000

#define AOF_RELOC_SID_MASK   0x00FFFFFF   /* Symbol or Area ID */

/* Helper to extract fields */
static inline int aof_reloc_is_symbol(uint32_t reloc) {
    return (reloc & AOF_RELOC_A_BIT) != 0;
}

static inline int aof_reloc_is_pcrel(uint32_t reloc) {
    return (reloc & AOF_RELOC_R_BIT) != 0;
}

static inline uint32_t aof_reloc_get_sid(uint32_t reloc) {
    return reloc & AOF_RELOC_SID_MASK;
}

static inline uint32_t aof_reloc_get_field_type(uint32_t reloc) {
    return (reloc >> 24) & 0x03;
}

#endif /* AOF_AREA_H */
```

### AOF Symbol Table

```c
#ifndef AOF_SYMBOL_H
#define AOF_SYMBOL_H

#include <stdint.h>

/* Symbol table entry - 4 words */
typedef struct {
    uint32_t name;              /* Offset in string table */
    uint32_t attributes;        /* Bit-mapped attributes */
    uint32_t value;             /* Value (context-dependent) */
    uint32_t area_name;         /* Area name offset (or 0) */
} AOFSymbolEntry;

_Static_assert(sizeof(AOFSymbolEntry) == 16, "Symbol entry must be 16 bytes");

/* Symbol attribute bits */
#define AOF_SYM_DEFINED     0x00000001   /* Symbol defined in this file */
#define AOF_SYM_GLOBAL      0x00000002   /* Global scope */
#define AOF_SYM_ABSOLUTE    0x00000004   /* Absolute value */
#define AOF_SYM_CASE_INSENS 0x00000008   /* Case-insensitive */
#define AOF_SYM_WEAK        0x00000010   /* Weak definition */
#define AOF_SYM_COMMON      0x00000040   /* Common symbol */
#define AOF_SYM_STRONG      0x00000080   /* Strong definition */

#define AOF_SYM_TYPE_MASK   0x0F000000   /* Symbol type (bits 24-27) */
#define AOF_SYM_TYPE_SHIFT  24

/* Helper functions */
static inline int aof_sym_is_defined(uint32_t attr) {
    return (attr & AOF_SYM_DEFINED) != 0;
}

static inline int aof_sym_is_global(uint32_t attr) {
    return (attr & AOF_SYM_GLOBAL) != 0;
}

static inline int aof_sym_is_absolute(uint32_t attr) {
    return (attr & AOF_SYM_ABSOLUTE) != 0;
}

static inline int aof_sym_is_common(uint32_t attr) {
    return (attr & AOF_SYM_COMMON) != 0;
}

static inline uint32_t aof_sym_get_scope(uint32_t attr) {
    return attr & 0x03;
}

#endif /* AOF_SYMBOL_H */
```

---

## Complete AOF Processing Example

### Example AOF File Structure

```
Chunk File Header (at 0x00):
  ChunkFileId: 0xC3CBC6C5
  max_chunks: 5
  num_chunks: 5

  Directory Entry 0: OBJ_HEAD at 0x2C, size 0x48
  Directory Entry 1: OBJ_AREA at 0x74, size 0x600
  Directory Entry 2: OBJ_IDFN at 0x674, size 0x20
  Directory Entry 3: OBJ_SYMT at 0x694, size 0x40
  Directory Entry 4: OBJ_STRT at 0x6D4, size 0x50

OBJ_HEAD (at 0x2C):
  file_type: 0xC5E2D080
  version: 310
  num_areas: 2
  num_symbols: 3
  entry_area: 1
  entry_offset: 0

  Area 0 (code):
    name: 4 ("text")
    attrs: 0x2204 (code, 4-byte align)
    size: 0x200
    relocs: 2
    base: 0

  Area 1 (data):
    name: 10 ("data")
    attrs: 0x0800 (read-only)
    size: 0x100
    relocs: 0
    base: 0

OBJ_AREA (at 0x74):
  [512 bytes of code]
  [2 relocation directives]
  [256 bytes of data]

OBJ_IDFN (at 0x674):
  "ARM Compiler v2.5\0"

OBJ_SYMT (at 0x694):
  Symbol 0: "main" at offset 0 in area 0
  Symbol 1: "printf" (external)
  Symbol 2: "version" = 0x100 (absolute)

OBJ_STRT (at 0x6D4):
  Length: 0x50
  "text\0"
  "data\0"
  "main\0"
  "printf\0"
  "version\0"
  [padding]
```

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals
- ARM Tools User Guide
- ARM Linker documentation

