# ARM Object Format (AOF) — Comprehensive Specification

**Format Name:** ARM Object Format (AOF)  
**Also Known As:** Acorn Object Format  
**Purpose:** Relocatable object files for ARM processors  
**Foundation:** Chunk File Format (shared with ALF)  
**Magic Numbers:** Chunk File ID `0xC3CBC6C5`, AOF Type `0xC5E2D080`, Version `310` (`0x136`)  
**File Extension:** `.o`

**Source References:**
- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide (1997-1998)
- RISC OS Programmer's Reference Manuals
- ARM Software Development Toolkit User Guide

---

## 1. Overview

ARM Object Format (AOF) is the standard relocatable object file format produced by ARM language tools (compilers, assemblers) and consumed by the ARM linker. It is a chunk-based format that contains compiled code, initialized data, symbol definitions, relocation directives, and optional debugging information.

### 1.1 Key Properties

- Based on the **Chunk File Format** (shared with ALF)
- Contains one or more **areas** — named, independent, indivisible chunks of code or data
- Supports **relocation** — all position-dependent references are recorded for adjustment at link time
- Contains a **symbol table** for inter-module references and definitions
- Supports both **26-bit and 32-bit ARM modes**, as well as **Thumb code**
- Supports **APCS** (ARM Procedure Call Standard) compliance marking

### 1.2 Toolchain Pipeline

```
Source Code → [armcc / armasm] → AOF (.o)
                                    ↓
                              [armlib] → ALF Library (.lib / .a)
                                    ↓
                              [armlink] → AIF / ELF Executable
```

**Producers:** ARM C Compiler (`armcc`), ARM C++ Compiler (`armcpp`), ARM Assembler (`armasm`/`ObjAsm`), Thumb compilers (`tcc`, `tcpp`)  
**Consumers:** ARM Linker (`armlink`), ARM Librarian (`armlib`/`arlib`), `decaof` utility

### 1.3 Historical Versions

| Version ID | Date | Key Features |
|------------|------|-------------|
| 150 | ~1989 | Initial version (v1.50) |
| 200 | ~1990 | Enhanced features (v2.00) |
| 310 | 1997-1998 | Current version (v3.10): 32-bit support, Thumb, interworking, Extended AIF |

---

## 2. Chunk File Format Foundation

AOF is layered on the Chunk File Format, which provides a generic container for multiple independent data chunks within a single file. Both AOF and ALF share this same foundation.

### 2.1 Chunk File Header (12 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 bytes | `ChunkFileId` | Magic number: **`0xC3CBC6C5`** |
| 0x04 | 4 bytes | `maxChunks` | Maximum number of chunk entries (fixed at creation) |
| 0x08 | 4 bytes | `numChunks` | Currently used entries (0 to `maxChunks`); redundant — derivable by scanning |

**Endianness detection:**
- `0xC3CBC6C5` → file matches host byte order
- `0xC5C6CBC3` → all word values must be byte-reversed

### 2.2 Chunk Directory Entries (16 bytes each)

Immediately following the file header are `maxChunks` entries:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 8 bytes | `chunkId` | 8-character identifier (endianness-independent string) |
| +0x08 | 4 bytes | `fileOffset` | Byte offset from file start (must be divisible by 4). **Value 0 = unused entry.** |
| +0x0C | 4 bytes | `size` | Exact byte size of chunk contents (need not be multiple of 4) |

**Total file header size:** `12 + (maxChunks × 16)` bytes

### 2.3 Chunk Identification

The 8-byte `chunkId` consists of two 4-character parts stored in ascending address order (like a string), independent of word endianness:

- **Bytes 0-3:** Namespace prefix (`"OBJ_"` for AOF)
- **Bytes 4-7:** Component identifier (`"HEAD"`, `"AREA"`, `"SYMT"`, `"STRT"`, `"IDFN"`)

Example: `"OBJ_HEAD"` stored as bytes: `4F 42 4A 5F 48 45 41 44`

---

## 3. AOF Chunk Types

| Chunk ID | Required | Description |
|----------|----------|-------------|
| `OBJ_HEAD` | **Yes** | AOF header: file type, version, area declarations |
| `OBJ_AREA` | **Yes** | Area contents: code/data bytes + relocation directives |
| `OBJ_IDFN` | No | Tool identification string |
| `OBJ_SYMT` | No | Symbol table: definitions and references |
| `OBJ_STRT` | No | String table: names for areas, symbols, etc. |

**Minimum viable AOF:** `OBJ_HEAD` + `OBJ_AREA`  
**Typical AOF:** All five chunks present  
**Conventional allocation:** 8 chunk slots (allows room for future expansion)  
**Order:** Chunks may appear in any order; the chunk directory provides random access

---

## 4. AOF Header (OBJ_HEAD)

The `OBJ_HEAD` chunk contains a fixed-size header followed by a variable number of area declarations.

### 4.1 Fixed Part (6 words = 24 bytes)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | Object File Type | **`0xC5E2D080`** — identifies file as AOF |
| 0x04 | 4 | Version ID | **`310`** (`0x136`) for current version |
| 0x08 | 4 | Number of Areas | Count of area declarations following this header (recommended ≤ 255) |
| 0x0C | 4 | Number of Symbols | Count of symbol entries in `OBJ_SYMT` (0 if no symbol table) |
| 0x10 | 4 | Entry Area Index | **1-origin** index into area headers. 0 = no entry point defined. |
| 0x14 | 4 | Entry Offset | Byte offset within the entry area. Entry address = `base(entry_area) + entry_offset` |

### 4.2 Area Headers (5 words = 20 bytes each)

Immediately following the fixed part are `Number_of_Areas` area headers:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | Area Name | Byte offset into `OBJ_STRT` string table |
| +0x04 | 4 | Attributes + Alignment | Bit-mapped attributes and alignment (see section 5) |
| +0x08 | 4 | Area Size | Size in bytes (must be multiple of 4) |
| +0x0C | 4 | Number of Relocations | Count of relocation directives for this area |
| +0x10 | 4 | Base Address | Base address for absolute areas; 0 or reserved otherwise |

**Total OBJ_HEAD size:** `24 + (Number_of_Areas × 20)` bytes

---

## 5. Area Attributes

The Attributes + Alignment word in each area header contains both alignment specification and attribute flags.

### 5.1 Alignment (Bits 0-7)

The least significant byte specifies the required alignment as a power of 2:

| Value | Alignment |
|-------|-----------|
| 2 | 4-byte (word) — minimum and most common |
| 3 | 8-byte (doubleword) |
| 4 | 16-byte |
| ... | ... |
| 32 | 4 GiB |

### 5.2 Attribute Flags (Bits 8-31)

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 8 | `0x00000100` | **Absolute** | Area has a fixed base address (value in Base Address field) |
| 9 | `0x00000200` | **Code** | Area contains executable code (not set = data area) |
| 10 | `0x00000400` | **Common Definition** | Common area definition (Fortran COMMON, C tentative) |
| 11 | `0x00000800` | **Common Reference** | Reference to common block defined elsewhere |
| 12 | `0x00001000` | **Zero-Initialized** | No initializing data; area filled with zeros at link time. Incompatible with Read-Only. |
| 13 | `0x00002000` | **Read-Only** | Area is not modified after relocation. Typically set with Code. |
| 14 | `0x00004000` | **Position Independent** | All references are PC-relative or base-register-relative |
| 15 | `0x00008000` | **Debugging Tables** | Contains symbolic debugging data (ASD format) |
| 16 | `0x00010000` | **32-bit APCS** | Code area uses 32-bit APCS variant |
| 17 | `0x00020000` | **Reentrant** | Code area uses reentrant APCS variant |
| 18 | `0x00040000` | **Extended FP** | Code area uses LFM/SFM floating-point instructions |
| 19 | `0x00080000` | **No SW Stack Check** | Code area has no software stack limit checking |
| 20 | `0x00100000` | **Thumb** (code) / **Based** (data) | Code: contains Thumb instructions. Data: addressed via base register. |
| 21 | `0x00200000` | **Halfword** (code) / **Stub** (data) | Code: uses ARM halfword/signed transfer instructions. Data: shared library stub data. |
| 22 | `0x00400000` | **Interworking** | Code area supports ARM/Thumb interworking |
| 23 | — | Reserved | Must be 0 |
| 24-27 | `0x0F000000` | **Base Register** (data) / **Stub Bits** (common) | Data: base register number (R0-R15). Common: max alignment (log₂). |
| 28-31 | — | Reserved | Must be 0 |

### 5.3 Attribute Combination Rules

- **Code + Read-Only** are typically set together
- **Zero-Initialized + Read-Only** are incompatible (cannot both be set)
- **Common areas** should have Reentrant set
- **Common areas** must NOT have Read-Only set
- **Absolute areas** must have the Base Address field populated
- **Zero-Initialized areas** have no data in `OBJ_AREA`; their size contributes to runtime memory only
- **Debugging areas** ignore the Code bit
- Code attributes (bits 16-22) are only meaningful when the Code bit (bit 9) is set

### 5.4 RISC OS PRM Encoding (Alternative, versions ≤ 2.00)

Older versions used a different attribute layout with bits 0-7 as type flags rather than alignment:

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | `0x01` | Absolute | Fixed base address |
| 1 | `0x02` | Relocatable | May be relocated |
| 2 | `0x04` | Code | Contains executable code |
| 3 | `0x08` | Common | Uninitialized common data |
| 4 | `0x10` | Common Block | Must be contiguous block |
| 5 | `0x20` | Reserved | Linker use |
| 6 | `0x40` | Debug | Debugging tables |
| 7 | `0x80` | Zero-Init | Zero-initialized data |
| 8-15 | — | AT flags | Additional type flags |
| 13-14 | `0x6000` | Alignment | log₂ encoded |

Implementors should check the Version ID to determine which encoding is in use.

---

## 6. Area Data (OBJ_AREA)

The `OBJ_AREA` chunk contains the concatenated data for all areas, each followed by its relocation directives. Areas appear in the order declared in `OBJ_HEAD`.

### 6.1 Layout

```
For each area (in declaration order):
+----------------------------+
| Area Data                  |  area_size bytes (omitted if zero-initialized)
+----------------------------+
| Relocation Directives      |  num_relocs × 8 bytes
+----------------------------+
```

```
Area 0 Data        (area_size[0] bytes, 4-byte aligned)
Area 0 Relocations (num_relocs[0] × 8 bytes)
Area 1 Data
Area 1 Relocations
...
Area N-1 Data
Area N-1 Relocations
```

### 6.2 Properties

- All area data and relocation tables are aligned to **4-byte boundaries**
- Zero-initialized areas (attribute bit 12 set) have **no data** — only their relocation directives (if any)
- Endianness of words/halfwords within area data matches the AOF file endianness
- Area size must be a multiple of 4 bytes

---

## 7. Relocation Directives

Each relocation directive is 8 bytes (2 words) and describes how to adjust one field within an area when the final addresses are determined at link time.

### 7.1 Directive Format

```
Word 0: Offset
+-----------------------------------------------------------+
| Byte offset within area of field to relocate (32 bits)    |
+-----------------------------------------------------------+

Word 1: Flags and SID
+------+-------+------+------+------+--------+-------------+
| Bit  | Bits  | Bit  | Bit  | Bit  | Bits   | Bits        |
| 31   | 30-29 | 28   | 27   | 26   | 25-24  | 23-0        |
|      |  II   |  B   |  A   |  R   |  FT    |    SID      |
+------+-------+------+------+------+--------+-------------+
```

### 7.2 Field Descriptions

#### SID — Symbol/Area Identifier (Bits 0-23)

| A Bit | SID Interpretation |
|-------|--------------------|
| A = 0 (bit 27 clear) | SID is a **0-origin area index**. Relocation value = base address of that area. |
| A = 1 (bit 27 set) | SID is a **0-origin symbol table index**. Relocation value = value of that symbol. |

#### FT — Field Type (Bits 24-25)

| Value | Name | Size | Description |
|-------|------|------|-------------|
| 00 | Byte | 1 byte | 8-bit field |
| 01 | Halfword | 2 bytes | 16-bit field |
| 10 | Word | 4 bytes | 32-bit field (most common) |
| 11 | Instruction | Variable | ARM or Thumb instruction sequence |

For instruction relocations (FT = 11):
- If bit 0 of the Offset word is set → **Thumb** instruction
- If bit 0 of the Offset word is clear → **ARM** instruction

Bytes, halfwords, and instructions may only be relocated by values of suitably small size. Overflow is faulted by the linker.

#### R — PC-Relative Flag (Bit 26)

| R | Meaning | Formula |
|---|---------|---------|
| 0 | Plain additive (absolute) | `field = field + relocation_value` |
| 1 | PC-relative | `field = field + relocation_value - base_of_area_containing(field)` |

When R = 1, A = 0, and the relocation value equals the base of the area containing the subject field: only the subtraction is applied (`field = field - area_base`). This handles PC-relative branches to fixed addresses within the same area.

#### A — Symbol/Area Selector (Bit 27)

| A | Meaning |
|---|---------|
| 0 | Relocate by area base (SID = area index) |
| 1 | Relocate by symbol value (SID = symbol index) |

#### B — Based Area Relocation (Bit 28)

| B | Meaning |
|---|---------|
| 0 | Normal relocation |
| 1 | Based area relocation: `field = field + (relocation_value - base_of_area_group)` |

When B = 1:
- Bits 29-30 (II) must be 0
- Bit 31 must be 1
- Used for `sb`-relative addressing in reentrant code (static base register)

If both R = 1 and B = 1: inter-link-unit branch for tail-call optimization in reentrant code.

#### II — Instruction Sequence Constraint (Bits 29-30)

For instruction relocations (FT = 11), constrains how many instructions the linker may modify:

| Value | Constraint |
|-------|------------|
| 00 | No constraint — linker may modify as many instructions as needed |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

#### Bit 31

Must be 1 for based area relocations (B = 1). Otherwise varies by context.

### 7.3 Instruction Sequence Relocation Patterns

The linker recognizes these instruction patterns for sequence relocation (FT = 11):

- **Branch:** `B` or `BL` instruction (always relocated alone, II ignored)
- **Load/Store:** `LDR` or `STR` instruction
- **ADD/SUB sequence:** 1-3 `ADD`/`SUB` instructions with a common destination register, optionally followed by `LDR`/`STR`

Example of a 3-instruction sequence:
```arm
ADD  Rb, Rx, #V1        ; First part of large offset
ADD  Rb, Rb, #V2        ; Second part
LDR  Ry, [Rb, #V3]      ; Final load with remaining offset
; Total relocatable value: V = V1 + V2 + V3
```

### 7.4 Common Relocation Patterns

| Pattern | R | A | B | FT | Description |
|---------|---|---|---|----|-------------|
| Simple data pointer | 0 | 0 | 0 | 10 | `field += area_base[SID]` |
| Symbol reference | 0 | 1 | 0 | 10 | `field += symbol_value[SID]` |
| PC-relative branch | 1 | 0 | 0 | 11 | `field += area_base[SID] - this_area_base` |
| PC-relative to symbol | 1 | 1 | 0 | 11 | `field += symbol_value[SID] - this_area_base` |
| Based area reference | 0 | 1 | 1 | 10 | `field += symbol_value[SID] - group_base` |

### 7.5 RISC OS PRM Relocation Format (Version 1.50, alternative)

Older versions used a different bit layout:

| Bits | Field | Description |
|------|-------|-------------|
| 0 | PC-relative flag | If set, A-field is PC-relative |
| 8-11 | A field specifier | How to extract addend; 0 = no A-field |
| 12-15 | R-type specifier | Always 0 in v1.50 (word-wide) |
| 16-22 | SID number | Symbol index (if bit 23 = 0) |
| 23 | Area/Symbol flag | 1 = remaining 7 bits specify area number |

---

## 8. Symbol Table (OBJ_SYMT)

The symbol table contains `Number_of_Symbols` entries (as declared in `OBJ_HEAD`). Each entry is 16 bytes (4 words).

### 8.1 Symbol Entry Format

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 | Name | Byte offset into `OBJ_STRT` string table |
| +0x04 | 4 | Attributes | Bit-mapped symbol attributes (see section 8.2) |
| +0x08 | 4 | Value | Symbol value, offset, or common block size (see section 8.4) |
| +0x0C | 4 | Area Name | Byte offset into `OBJ_STRT` of the defining area name |

### 8.2 Symbol Attribute Flags

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | `0x00000001` | **Defined** | Symbol is defined in this object file |
| 1 | `0x00000002` | **Global / Exported** | Symbol is visible outside this object file |
| 2 | `0x00000004` | **Absolute** | Symbol has a constant (absolute) value, not an address |
| 3 | `0x00000008` | **Case-Insensitive** | Match this symbol name case-insensitively (external refs only) |
| 4 | `0x00000010` | **Weak** | Weak definition/reference; may remain unsatisfied at link time |
| 5 | `0x00000020` | **Strong** | Strong definition; overrides non-strong definitions from other files |
| 6 | `0x00000040` | **Common** | Reference to common area; Value field = byte length of common block |
| 7 | — | Reserved | Must be 0 |
| 8 | `0x00000100` | **Code Datum** | Symbol identifies a datum (not code) within a code area |
| 9 | `0x00000200` | **FP Args in FP Regs** | Function entry point accepting FP arguments in FP registers |
| 10 | — | Reserved | Must be 0 |
| 11 | `0x00000800` | **Simple Leaf Function** | Leaf function: intra-link-unit and inter-link-unit entry are the same |
| 12 | `0x00001000` | **Thumb** | Symbol refers to Thumb code (version 3.10) |
| 13-15 | — | Reserved | Must be 0 |
| 16-23 | `0x00FF0000` | **Datum / Type** | Type-dependent interpretation (see section 8.3) |
| 24-27 | `0x0F000000` | **Symbol Type** | Type classification (see section 8.3) |
| 28-31 | — | Reserved | Must be 0 |

### 8.3 Scope Encoding (Bits 0-1 combined)

| Bit 1 (Global) | Bit 0 (Defined) | Meaning |
|-----------------|-----------------|---------|
| 0 | 0 | Reserved (should not occur) |
| 0 | 1 | **Local definition** — defined in this file, not exported |
| 1 | 0 | **External reference** — defined in another file, imported |
| 1 | 1 | **Global definition** — defined in this file, exported |

### 8.3 Symbol Type Values (Bits 24-27)

| Type | Description | Bits 16-23 Interpretation |
|------|-------------|---------------------------|
| 0 | Not used | — |
| 1-8 | Reserved | — |
| 9 | FP Register | Bits 16-17: FP size (1=1 word, 2=2 words, 3=3 words). Bits 18-23: register number (255=none). |
| 10 | Scratch Register | Bits 16-23: register number (255=none). Not preserved across calls. |
| 11 | Non-Scratch Register | Bits 16-23: register number. Preserved across calls. |
| 12 | Coprocessor Register (scratch) | R = 256×CP_number + register_number. Not preserved. |
| 13 | Coprocessor Register (preserved) | Same encoding as type 12. Preserved across calls. |

### 8.4 Value Field Interpretation

| Condition | Value Field Contains |
|-----------|---------------------|
| Absolute (bit 2 set) | Absolute constant value |
| Common (bit 6 set) | Size of common block in bytes |
| Defined, non-absolute (bit 0 set, bit 2 clear) | Byte offset from base of area named in Area Name field |
| Undefined (bit 0 clear) | 0 (meaningless) |

### 8.5 Area Name Field Interpretation

- **Non-absolute defined symbol** (bit 0 set, bit 2 clear): Byte offset into `OBJ_STRT` of the area name where the symbol is defined
- **Absolute or undefined symbol**: 0

---

## 9. String Table (OBJ_STRT)

The string table stores all names used by area headers and symbol entries.

### 9.1 Format

```
+----------------------------+
| Length (4 bytes)            |  Total size of table including this word
+----------------------------+
| String 0 (NUL-terminated)  |  First string (offset = 4)
+----------------------------+
| String 1 (NUL-terminated)  |
+----------------------------+
| ...                        |
+----------------------------+
```

### 9.2 Properties

- **First 4 bytes:** Total length of the string table **including this length word**
- **Minimum size:** 4 bytes (empty table with only the length word)
- **Minimum valid string offset:** 4 (offsets 0-3 fall within the length word)
- **String format:** NUL-terminated. Valid characters: non-control characters (codes 32-126, 160-255).
- **No padding** between strings
- Strings are identified by **byte offset** from the start of the `OBJ_STRT` chunk
- The length word's endianness matches the AOF file

---

## 10. Identification Chunk (OBJ_IDFN)

Optional single NUL-terminated string identifying the tool that created the file.

### 10.1 Properties

- No length prefix (unlike `OBJ_STRT`)
- Valid characters: codes 10-13 (CR, LF, etc.) and 32-126
- Codes 128-255 are discouraged (host-system-dependent)

### 10.2 Examples

```
"ARM C Compiler v2.50 (ARM Ltd.)"
"ARM Assembler version 2.50"
"ObjAsm 3.50"
```

---

## 11. Endianness and Alignment

### 11.1 Endianness

- AOF files exist in **little-endian** or **big-endian** variants
- **Little-endian:** LSB at lowest address (DEC/Intel/Acorn/ARM standard)
- **Big-endian:** MSB at lowest address (IBM/Motorola)
- File endianness matches the **target ARM system**, not the host
- Detection via `ChunkFileId`: `0xC3CBC6C5` = native; `0xC5C6CBC3` = reversed
- Cannot mix endianness in a single link operation
- Chunk IDs (8-byte character strings) are endianness-independent

### 11.2 Alignment Requirements

| Data Type | Alignment |
|-----------|-----------|
| Strings / bytes | Any byte boundary |
| Half-words | 2-byte boundary |
| Words | 4-byte boundary |
| Chunk file offsets | Divisible by 4 |
| Area data | 4-byte boundary (or as specified by area alignment attribute) |
| Relocation directives | 4-byte boundary (each is 8 bytes = 2 words) |
| Symbol entries | 4-byte boundary (each is 16 bytes = 4 words) |
| AOF header fields | 4-byte boundary |

---

## 12. Complete File Structure Example

```
CHUNK FILE HEADER (12 bytes):
  ChunkFileId: 0xC3CBC6C5
  maxChunks:   8
  numChunks:   5

CHUNK DIRECTORY (8 × 16 = 128 bytes):
  Entry 0: id="OBJ_HEAD"  offset=140   size=64
  Entry 1: id="OBJ_AREA"  offset=204   size=256
  Entry 2: id="OBJ_IDFN"  offset=460   size=32
  Entry 3: id="OBJ_SYMT"  offset=492   size=48
  Entry 4: id="OBJ_STRT"  offset=540   size=56
  Entry 5: (unused)
  Entry 6: (unused)
  Entry 7: (unused)

OBJ_HEAD (64 bytes):
  Fixed Part (24 bytes):
    Object File Type:  0xC5E2D080
    Version ID:        310 (0x136)
    Number of Areas:   2
    Number of Symbols: 3
    Entry Area Index:  1  (first area, 1-origin)
    Entry Offset:      0

  Area Header 0 (20 bytes):
    Name:        offset 4 → "$$Code"
    Attributes:  0x00002302  (Code + ReadOnly + 32-bit APCS, align=4)
    Size:        128
    Relocations: 2
    Base:        0

  Area Header 1 (20 bytes):
    Name:        offset 12 → "$$Data"
    Attributes:  0x00000002  (Data, align=4)
    Size:        64
    Relocations: 1
    Base:        0

OBJ_AREA (256 bytes):
  Area 0 Data:        128 bytes of code
  Area 0 Relocations: 2 × 8 = 16 bytes
  Area 1 Data:        64 bytes of data
  Area 1 Relocations: 1 × 8 = 8 bytes
  (remaining bytes: padding if any)

OBJ_IDFN (32 bytes):
  "ARM C Compiler v2.50\0"

OBJ_SYMT (48 bytes = 3 × 16):
  Symbol 0: name=offset_20 ("main"), attr=0x03, value=0, area=offset_4
  Symbol 1: name=offset_25 ("_data"), attr=0x03, value=0, area=offset_12
  Symbol 2: name=offset_31 ("printf"), attr=0x02, value=0, area=0

OBJ_STRT (56 bytes):
  Length: 56
  offset  4: "$$Code\0"
  offset 12: "$$Data\0"
  offset 20: "main\0"
  offset 25: "_data\0"
  offset 31: "printf\0"
```

---

## 13. C Structure Definitions

```c
#ifndef AOF_FORMAT_H
#define AOF_FORMAT_H

#include <stdint.h>

/* ===== Chunk File Format ===== */

#define CHUNK_FILE_ID           0xC3CBC6C5
#define CHUNK_FILE_ID_REVERSED  0xC5C6CBC3

typedef struct {
    uint32_t chunk_file_id;     /* 0xC3CBC6C5 */
    uint32_t max_chunks;
    uint32_t num_chunks;
} ChunkFileHeader;              /* 12 bytes */

typedef struct {
    char     chunk_id[8];       /* e.g., "OBJ_HEAD" */
    uint32_t file_offset;       /* 4-byte aligned; 0 = unused */
    uint32_t size;              /* Exact byte size */
} ChunkDirEntry;                /* 16 bytes */

/* ===== AOF Header (OBJ_HEAD) ===== */

#define AOF_FILE_TYPE           0xC5E2D080
#define AOF_VERSION_150         150
#define AOF_VERSION_200         200
#define AOF_VERSION_310         310     /* 0x136 — current version */

typedef struct {
    uint32_t object_file_type;  /* 0xC5E2D080 */
    uint32_t version_id;        /* 310 (0x136) */
    uint32_t num_areas;         /* Count of area headers following */
    uint32_t num_symbols;       /* Count of symbols in OBJ_SYMT */
    uint32_t entry_area;        /* 1-origin index (0 = no entry point) */
    uint32_t entry_offset;      /* Byte offset within entry area */
} AOFHeader;                    /* 24 bytes */

typedef struct {
    uint32_t name;              /* Offset into OBJ_STRT */
    uint32_t attributes;        /* Bit flags + alignment */
    uint32_t size;              /* Area size in bytes (multiple of 4) */
    uint32_t num_relocs;        /* Count of relocation directives */
    uint32_t base_address;      /* Base address (absolute areas); 0 otherwise */
} AOFAreaHeader;                /* 20 bytes */

/* ===== Area Attribute Flags (SDT 2.50 encoding) ===== */

/* Bits 0-7: Alignment (power of 2, minimum value 2 = word-aligned) */
#define AOF_ALIGN_MASK          0x000000FF

/* Bits 8+: Attribute flags */
#define AOF_ATTR_ABSOLUTE       0x00000100  /* Bit 8:  Fixed base address */
#define AOF_ATTR_CODE           0x00000200  /* Bit 9:  Executable code */
#define AOF_ATTR_COMDEF         0x00000400  /* Bit 10: Common area definition */
#define AOF_ATTR_COMREF         0x00000800  /* Bit 11: Common block reference */
#define AOF_ATTR_ZEROINIT       0x00001000  /* Bit 12: Zero-initialized (no data) */
#define AOF_ATTR_READONLY       0x00002000  /* Bit 13: Read-only after relocation */
#define AOF_ATTR_PIC            0x00004000  /* Bit 14: Position-independent code */
#define AOF_ATTR_DEBUG          0x00008000  /* Bit 15: Debugging tables */
#define AOF_ATTR_APCS32         0x00010000  /* Bit 16: 32-bit APCS (code only) */
#define AOF_ATTR_REENTRANT      0x00020000  /* Bit 17: Reentrant APCS (code only) */
#define AOF_ATTR_EXTENDEDFP     0x00040000  /* Bit 18: Extended FP (code only) */
#define AOF_ATTR_NOSTACKCHECK   0x00080000  /* Bit 19: No SW stack check (code only) */
#define AOF_ATTR_THUMB          0x00100000  /* Bit 20: Thumb code (code) / Based (data) */
#define AOF_ATTR_HALFWORD       0x00200000  /* Bit 21: Halfword instrs (code) / Stub (data) */
#define AOF_ATTR_INTERWORK      0x00400000  /* Bit 22: ARM/Thumb interworking (code) */
#define AOF_ATTR_BASED          0x00100000  /* Bit 20: Based area (data) — same as THUMB */
#define AOF_ATTR_STUB           0x00200000  /* Bit 21: SHL stub (data) — same as HALFWORD */
#define AOF_ATTR_BASEREG_MASK   0x0F000000  /* Bits 24-27: Base register number */
#define AOF_ATTR_BASEREG_SHIFT  24

/* Helper macros */
#define AOF_GET_ALIGNMENT(attr) ((attr) & AOF_ALIGN_MASK)
#define AOF_GET_BASEREG(attr)   (((attr) >> AOF_ATTR_BASEREG_SHIFT) & 0x0F)

/* ===== Relocation Directives ===== */

typedef struct {
    uint32_t offset;            /* Byte offset in area of field to relocate */
    uint32_t flags;             /* Flags and SID (see bit layout) */
} AOFReloc;                     /* 8 bytes */

/* Relocation flags */
#define AOF_RELOC_SID_MASK      0x00FFFFFF  /* Bits 0-23:  SID */
#define AOF_RELOC_FT_MASK       0x03000000  /* Bits 24-25: Field Type */
#define AOF_RELOC_FT_SHIFT      24
#define AOF_RELOC_R             0x04000000  /* Bit 26: PC-relative */
#define AOF_RELOC_A             0x08000000  /* Bit 27: Symbol (1) vs Area (0) */
#define AOF_RELOC_B             0x10000000  /* Bit 28: Based area relocation */
#define AOF_RELOC_II_MASK       0x60000000  /* Bits 29-30: Instruction constraint */
#define AOF_RELOC_II_SHIFT      29

/* Field Type values */
#define AOF_FT_BYTE             0   /* 8-bit field */
#define AOF_FT_HALFWORD         1   /* 16-bit field */
#define AOF_FT_WORD             2   /* 32-bit field */
#define AOF_FT_INSTRUCTION      3   /* ARM or Thumb instruction */

/* Helper macros */
#define AOF_GET_SID(f)          ((f) & AOF_RELOC_SID_MASK)
#define AOF_GET_FT(f)           (((f) >> AOF_RELOC_FT_SHIFT) & 0x03)
#define AOF_IS_PCREL(f)         (((f) & AOF_RELOC_R) != 0)
#define AOF_IS_SYMBOL(f)        (((f) & AOF_RELOC_A) != 0)
#define AOF_IS_BASED(f)         (((f) & AOF_RELOC_B) != 0)
#define AOF_GET_II(f)           (((f) >> AOF_RELOC_II_SHIFT) & 0x03)
#define AOF_IS_THUMB_RELOC(r)   (((r).offset & 1) != 0)  /* For FT=instruction */

/* ===== Symbol Table (OBJ_SYMT) ===== */

typedef struct {
    uint32_t name;              /* Offset into OBJ_STRT */
    uint32_t attributes;        /* Bit-mapped attributes */
    uint32_t value;             /* Value / offset / common size */
    uint32_t area_name;         /* Offset into OBJ_STRT of defining area */
} AOFSymbol;                    /* 16 bytes */

/* Symbol attribute flags */
#define AOF_SYM_DEFINED         0x00000001  /* Bit 0:  Defined in this file */
#define AOF_SYM_GLOBAL          0x00000002  /* Bit 1:  Exported / global */
#define AOF_SYM_ABSOLUTE        0x00000004  /* Bit 2:  Absolute value */
#define AOF_SYM_CASEINSENS      0x00000008  /* Bit 3:  Case-insensitive */
#define AOF_SYM_WEAK            0x00000010  /* Bit 4:  Weak definition/reference */
#define AOF_SYM_STRONG          0x00000020  /* Bit 5:  Strong definition */
#define AOF_SYM_COMMON          0x00000040  /* Bit 6:  Common block reference */
#define AOF_SYM_CODEDATUM       0x00000100  /* Bit 8:  Datum in code area */
#define AOF_SYM_FPARGS          0x00000200  /* Bit 9:  FP args in FP regs */
#define AOF_SYM_LEAF            0x00000800  /* Bit 11: Simple leaf function */
#define AOF_SYM_THUMB           0x00001000  /* Bit 12: Thumb symbol */
#define AOF_SYM_TYPE_MASK       0x0F000000  /* Bits 24-27: Symbol type */
#define AOF_SYM_TYPE_SHIFT      24
#define AOF_SYM_DATUM_MASK      0x00FF0000  /* Bits 16-23: Datum/type-dependent */
#define AOF_SYM_DATUM_SHIFT     16

/* Scope determination */
#define AOF_SYM_IS_LOCAL(attr)  (((attr) & 0x03) == 0x01)  /* defined, not global */
#define AOF_SYM_IS_EXTERN(attr) (((attr) & 0x03) == 0x02)  /* global, not defined */
#define AOF_SYM_IS_EXPORT(attr) (((attr) & 0x03) == 0x03)  /* defined and global */

/* Symbol type values (bits 24-27) */
#define AOF_SYMTYPE_NONE        0
#define AOF_SYMTYPE_FPREG       9   /* FP register */
#define AOF_SYMTYPE_SCRATCH     10  /* Scratch register */
#define AOF_SYMTYPE_NONSCRATCH  11  /* Non-scratch register */
#define AOF_SYMTYPE_COPROC_S    12  /* Coprocessor register (scratch) */
#define AOF_SYMTYPE_COPROC_P    13  /* Coprocessor register (preserved) */

/* ===== String Table (OBJ_STRT) ===== */

/* First word is the total length including itself.
 * Strings follow, each NUL-terminated.
 * Minimum valid string offset = 4.
 */

/* ===== Validation Helpers ===== */

static inline int aof_is_valid_header(const AOFHeader *hdr) {
    return hdr->object_file_type == AOF_FILE_TYPE;
}

static inline int aof_has_entry_point(const AOFHeader *hdr) {
    return hdr->entry_area != 0;
}

static inline int aof_area_is_code(const AOFAreaHeader *area) {
    return (area->attributes & AOF_ATTR_CODE) != 0;
}

static inline int aof_area_is_zeroinit(const AOFAreaHeader *area) {
    return (area->attributes & AOF_ATTR_ZEROINIT) != 0;
}

static inline int aof_area_is_readonly(const AOFAreaHeader *area) {
    return (area->attributes & AOF_ATTR_READONLY) != 0;
}

static inline int aof_area_is_debug(const AOFAreaHeader *area) {
    return (area->attributes & AOF_ATTR_DEBUG) != 0;
}

#endif /* AOF_FORMAT_H */
```

---

## 14. Validation Requirements

### 14.1 Chunk File Level

- `ChunkFileId` must be `0xC3CBC6C5` (or byte-swapped `0xC5C6CBC3`)
- `numChunks` ≤ `maxChunks`
- All `fileOffset` values divisible by 4
- Chunks must not overlap
- All chunks must be within file bounds

### 14.2 AOF Header Level

- `Object File Type` must be `0xC5E2D080`
- `Version ID` should be 150, 200, or 310
- `Number of Areas` should be > 0 and ≤ 255
- `Entry Area Index` must be 0 or 1-`Number_of_Areas`
- `Entry Offset` must be within the entry area's size

### 14.3 Area Level

- All area sizes must be multiples of 4
- Zero-initialized areas must not have Read-Only set
- Absolute areas must have a valid base address
- Alignment value must be ≥ 2

### 14.4 Symbol Level

- Bits 0-1 must not both be 0 (reserved combination)
- Defined non-absolute symbols must have valid area name offsets
- Common symbols must have non-zero value (size)
- String offsets must be within `OBJ_STRT` bounds

### 14.5 Relocation Level

- Offset values must be within the area size
- SID values must be valid area indices (A=0) or symbol indices (A=1)
- Word offsets must be divisible by 4
- Halfword offsets must be divisible by 2

---

## 15. Linker Interaction

### 15.1 How the Linker Processes AOF Files

1. **Read chunk file header** and locate all chunks
2. **Parse `OBJ_HEAD`** to get area declarations and symbol counts
3. **Parse `OBJ_STRT`** to resolve all name offsets
4. **Parse `OBJ_SYMT`** to collect symbol definitions and references
5. **For each area:**
   a. Allocate memory at appropriate address (based on attributes)
   b. Copy area data from `OBJ_AREA`
   c. Merge with same-named areas from other objects (if not absolute)
6. **Process relocations:**
   a. For each relocation directive, compute the relocation value
   b. Apply the adjustment to the target field
7. **Resolve symbols** against other object files and libraries
8. **Generate output** (AIF or ELF executable)

### 15.2 Area Merging Rules

- Areas with the same name are concatenated (placed adjacent in memory)
- Common areas with the same name are merged (overlaid at the same address)
- Absolute areas are placed at their specified base address
- Non-absolute areas are relocated to their final addresses

### 15.3 Entry Point

The program entry point is determined by:
- `Entry Area Index` (1-origin) identifies which area
- `Entry Offset` gives the byte offset within that area
- Entry address = `final_base(entry_area) + entry_offset`

Only one object file in a link should define an entry point. If multiple do, the linker reports an error.

---

## 16. Related Formats

| Format | Purpose | Relationship |
|--------|---------|-------------|
| **ALF** (ARM Library Format) | Static library container | Contains multiple AOF files as members |
| **AIF** (ARM Image Format) | Executable images | Linker output produced from AOF inputs |
| **Chunk File Format** | Generic container | Shared foundation used by both AOF and ALF |
| **ELF** | Modern replacement | Supersedes AOF/AIF on modern ARM systems |

---

**End of ARM Object Format (AOF) Specification**
