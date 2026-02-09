# ARM Object Format (AOF) - Complete Specification

## Document Information

**Format Name:** ARM Object Format (AOF)  
**Version:** 310 (0x136)  
**File Type Identifier:** 0xC5E2D080  
**Chunk Identifier:** OBJ_  
**Purpose:** Relocatable object files produced by ARM language processors (assemblers, compilers)  

---

## 1. Overview

ARM Object Format (AOF) is the standard object file format used by ARM development tools for storing compiled or assembled code before linking. AOF files are produced by:

- ARM C compilers (armcc)
- ARM C++ compilers (armcpp)
- ARM assemblers (armasm)
- Other ARM language processors

AOF files contain code, data, relocation information, symbol tables, and debugging data in a structured, extensible format.

---

## 2. Fundamental Data Types

All multi-byte values in AOF use **little-endian** byte ordering (LSB first, DEC/Intel byte sex).

### 2.1 Basic Types

| Type | Size | Alignment | Description |
|------|------|-----------|-------------|
| **Byte** | 8 bits | 1 byte | Unsigned unless stated otherwise; used for flags or characters |
| **Half word** | 16 bits | 2 bytes | Usually unsigned; LSB at lowest address |
| **Word** | 32 bits | 4 bytes | Usually unsigned; LSB at lowest address; address must be divisible by 4 |
| **String** | Variable | 1 byte | Sequence of bytes terminated by NUL (0x00); NUL is part of string but not counted in length |

**Critical Note:** A word consists of 32 bits, 4-byte aligned. Within a word, the least significant byte has the lowest address. This is DEC/Intel (little-endian) byte sex, **NOT** IBM/Motorola byte sex.

### 2.2 Endianness

An AOF file can be in either little-endian or big-endian format. The endianness is determined by reading the chunk file identifier (0xC3CBC6C5):

- If it reads as 0xC3CBC6C5: file is in native byte order
- If it reads as 0xC5C6CBC3: each word must be byte-reversed before use

The endianness of an AOF file matches the endianness of the target ARM system, which may differ from the host system processing the file.

---

## 3. File Structure

AOF uses a layered architecture built on **Chunk File Format**.

### 3.1 Overall Architecture

```
┌─────────────────────────────────────┐
│     Chunk File Format (Base)        │
│  ┌───────────────────────────────┐  │
│  │   ARM Object Format (AOF)     │  │
│  │                               │  │
│  │  - Header Chunk (OBJ_HEAD)   │  │
│  │  - Areas Chunk (OBJ_AREA)    │  │
│  │  - Symbol Table (OBJ_SYMT)   │  │
│  │  - String Table (OBJ_STRT)   │  │
│  │  - Identification (OBJ_IDFN) │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

### 3.2 Chunk File Format

Chunk File Format provides a container for storing multiple named data segments (chunks) with efficient random access.

#### 3.2.1 Chunk File Header

The chunk file begins with a header containing:

**Fixed Part (3 words):**

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x00 | ChunkFileId | Word | Magic number: 0xC3CBC6C5 (identifies file as chunk format and determines endianness) |
| 0x04 | max_chunks | Word | Maximum number of chunk entries in header (fixed at file creation) |
| 0x08 | num_chunks | Word | Number of chunks currently in use (0 to max_chunks); redundant, can be found by scanning entries |

**Chunk Entries (4 words each, max_chunks entries):**

Each chunk entry contains:

| Field | Size | Description |
|-------|------|-------------|
| chunkId | 8 bytes | Two-part identifier for chunk type (see below) |
| file_offset | Word | Byte offset within file to start of chunk data; must be divisible by 4; 0 = unused entry |
| size | Word | Exact size in bytes of chunk data; need not be multiple of 4 |

#### 3.2.2 Chunk Identifier Format

The 8-byte chunkId is split into two 4-character parts:

- **First 4 characters:** Universal domain identifier (allocated by central authority, originally Acorn)
  - For AOF: "OBJ_"
  - For ALF: "LIB_"
- **Second 4 characters:** Component identifier within that domain

Characters are stored in **ascending address order** (as if in a string), independent of endianness.

**Example:** OBJ_HEAD
- Bytes: 'O', 'B', 'J', '_', 'H', 'E', 'A', 'D'
- At addresses: 0, 1, 2, 3, 4, 5, 6, 7

#### 3.2.3 Chunk File Properties

- Chunks may appear in **any order** in the file
- Not all header entries need be used (allows limited expansion)
- File can be copied without knowledge of chunk contents
- Header size is fixed per file but may vary between files
- Convention: allocate space for 8 chunks when creating AOF with all 5 standard chunks

---

## 4. AOF Chunks

AOF defines five standard chunks:

| Chunk Name | Chunk ID | Required | Description |
|------------|----------|----------|-------------|
| Header | OBJ_HEAD | **Yes** | File metadata and area declarations |
| Areas | OBJ_AREA | **Yes** | Code, data, and relocation directives |
| Identification | OBJ_IDFN | No | Tool identification string |
| Symbol Table | OBJ_SYMT | No | Symbol definitions and references |
| String Table | OBJ_STRT | No | String storage for names |

### 4.1 Undefined Fields Policy

Fields not explicitly defined are **reserved to Acorn/ARM**. All undefined fields **must be zero**. ARM may assign meaning to such fields in future versions, typically in a manner that preserves zero as "default" behavior.

---

## 5. OBJ_HEAD - Header Chunk

The AOF header chunk consists of two contiguous parts:

### 5.1 Fixed Part (6 words)

| Offset | Field | Type | Value | Description |
|--------|-------|------|-------|-------------|
| 0x00 | file_type | Word | 0xC5E2D080 | Marks file as AOF; must match chunk file endianness |
| 0x04 | version_id | Word | 310 (0x136) | AOF version number |
| 0x08 | num_areas | Word | 0-255 | Number of code/data areas (should be ≤ 255) |
| 0x0C | num_symbols | Word | ≥ 0 | Number of symbols in symbol table (0 if no OBJ_SYMT chunk) |
| 0x10 | entry_area | Word | 0 or 1-origin | Area containing entry point (0 = no entry point defined) |
| 0x14 | entry_offset | Word | ≥ 0 | Byte offset of entry point within entry_area |

**Entry Point Calculation:**
```
entry_address = base_of_area[entry_area] + entry_offset
```

### 5.2 Area Declarations (5 or 6 words each)

Following the fixed part are **num_areas** area declarations. Each contains:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | area_name | Word | Byte offset into OBJ_STRT of area name (0 = no name, use default like "area00") |
| +0x04 | area_attributes | Word | Bit-mapped area attributes (see below) |
| +0x08 | area_size | Word | Size in bytes (for common areas: minimum acceptable size; for zero-init: must be multiple of 4) |
| +0x0C | num_relocs | Word | Number of relocation directives for this area |
| +0x10 | reserved | Word | Must be 0 (for library files) |
| +0x14 | area_base | Word | **Only if absolute (bit 8 set):** base address for this area |

---

## 6. Area Attributes

The area_attributes word is a complex bitmap encoding type, alignment, and various flags.

### 6.1 Attribute Bits Layout

```
Bits:  31-24    23-16    15-14  13-12  11   10   9    8    7-0
      [STUB]  [Reserved] [AL]  [DBG] [RO] [RE] [PI] [NC] [TYPE]
```

### 6.2 Type Bits (bits 0-7)

**At most ONE type bit may be set in an object file.**

| Bit | Mask | Meaning if Set |
|-----|------|----------------|
| 0 | 0x01 | **Absolute area** - base address fixed, not relocated |
| 1 | 0x02 | **Relocatable area** - may be relocated by linker |
| 2 | 0x04 | **Code area** - contains executable instructions |
| 3 | 0x08 | **Common area** - uninitialized, may overlay with same-named areas |
| 4 | 0x10 | **Common block area** - must be contiguous block |
| 5 | 0x20 | **Reserved for linker use** |
| 6 | 0x40 | **Debugging tables area** |
| 7 | 0x80 | **Zero-initialized data** - guaranteed initialized to zero |

### 6.3 AT (Area Type) Bits (bits 8-15)

| Bit | Mask | Meaning if Set |
|-----|------|----------------|
| 8 | 0x0100 | **No code** - area must not contain executable code |
| 9 | 0x0200 | **Position independent** - code/data is PC-relative or base-register relative |
| 10 | 0x0400 | **Reentrant/Common block** - can be shared or is common |
| 11 | 0x0800 | **Read-only** - not modified during normal execution |
| 12 | 0x1000 | **Debugging table** - contains symbolic debug info |
| 13-14 | 0x6000 | **Alignment** - log₂ alignment: 0=byte, 1=halfword, 2=word (bits encode power of 2) |
| 15 | 0x8000 | **Area base valid** - area_base field is present and meaningful |

**Alignment Encoding:**
- Bits 13-14 = 00: byte aligned (2⁰ = 1)
- Bits 13-14 = 01: halfword aligned (2¹ = 2)
- Bits 13-14 = 10: word aligned (2² = 4)
- Bits 13-14 = 11: doubleword aligned (2³ = 8)

### 6.4 Reserved Bits (bits 16-23)

Must be zero. Reserved for future use.

### 6.5 STUB Bits (bits 24-31)

Used and updated by the linker; initial value undefined except:

- **For common block areas (bit 4 set):** bits 24-27 define maximum permitted alignment in log₂ form
- **Should not be set** in files to be put in a library

### 6.6 Attribute Combination Rules

Certain bit combinations have special meanings or restrictions:

1. **Code areas** (bit 2 set):
   - Only bits 9, 11, 13-14, 15 can be set
   - Bit 9 = position independent code
   - Bit 11 = execute-only (read-only code)
   - NOT common data

2. **Common areas** (bit 3 or 4 set):
   - Bit 10 MUST be set
   - Bit 11 MUST NOT be set (common is never read-only)
   - For bit 4 (common block): STUB bits encode max alignment

3. **Absolute areas** (bit 0 set):
   - Bit 15 MUST be set
   - area_base field MUST be present
   - Cannot contain relocatable references to non-absolute areas

4. **Debugging areas** (bit 6 set):
   - Bit 12 MUST be set

5. **Zero-initialized areas** (bit 7 set):
   - Like common but storage guaranteed zero-initialized
   - May not share address space with other areas
   - Size must be multiple of 4

---

## 7. OBJ_AREA - Areas Chunk

Contains actual area contents (code, data, debug tables) and associated relocation directives.

### 7.1 Layout

```
┌─────────────────────────┐
│ Area 1 data             │
├─────────────────────────┤
│ Area 1 relocations      │
├─────────────────────────┤
│ Area 2 data             │
├─────────────────────────┤
│ Area 2 relocations      │
├─────────────────────────┤
│ ...                     │
├─────────────────────────┤
│ Area N data             │
├─────────────────────────┤
│ Area N relocations      │
└─────────────────────────┘
```

### 7.2 Area Data

- Present for each area with size > 0 (unless marked uninitialized)
- Exact byte sequence as in memory
- Endianness of words/halfwords must match AOF file endianness
- Aligned to 4-byte boundary

### 7.3 Relocation Directives

Each relocation directive is **2 words (8 bytes)**:

#### 7.3.1 Relocation Directive Format

```
Word 1: offset (byte offset in area)
Word 2: flags and SID
```

**Word 2 Bit Layout:**
```
Bits:  31-24     23        22-16      15-12     11-8      7-0
      [unused]  [SID bit] [SID area] [R-type]  [A-field] [flags]
```

#### 7.3.2 Fields

| Field | Bits | Description |
|-------|------|-------------|
| **offset** | Word 1 | Byte offset from area base of word to relocate |
| **SID** | 16-23 | Symbol/area identifier (see below) |
| **R-type** | 12-15 | Relocation field type (always 0 in v1.50 = word-wide) |
| **A-field** | 8-11 | A-field specifier (0 = addend in following word) |
| **PC-rel** | bit 0 | 1 = A-field value is PC-relative |

#### 7.3.3 SID (Symbol/Area Identifier) - Bits 16-23

**Bit 23 determines interpretation:**

- **Bit 23 = 0:** Bits 16-22 = symbol table index (0-127)
- **Bit 23 = 1:** Bits 16-22 = area number (0-127)

If symbol is in a common area, relocation is based on the area, not the symbol.

#### 7.3.4 A-Field Specifier (bits 8-11)

Determines how to extract addend from the word being relocated:

- **0:** No A-field in word; addend follows in next word
- **Non-zero:** Specifies bit-field extraction from word (implementation-specific)

#### 7.3.5 Common Relocation Cases

Notation:
- **W** = contents of word at offset being relocated
- **A_N** = base address of area N
- **a** = addend constant

| Type | Description | Formula |
|------|-------------|---------|
| Simple offset | Word contains offset to be relocated by area base | W + A_N |
| Offset + addend | Word contains offset plus constant | W + A_N + a |
| PC-relative branch | 24-bit branch target (ARM BL instruction) | ((W + A_N) mod 2²⁴) OR (W AND 0xFF000000) |

### 7.4 Relocation Processing

At link time:

1. For each relocation directive:
   - Read word at **area_base + offset**
   - Extract A-field (addend) if present, or read following word
   - Determine relocation value from SID (symbol value or area base)
   - Apply relocation formula based on flags
   - Write updated value back to **area_base + offset**

---

## 8. OBJ_SYMT - Symbol Table Chunk

Contains symbol definitions and references for linking and debugging.

### 8.1 Symbol Table Entry Format

Each entry is **4 words (16 bytes)**:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | name | Word | Offset into OBJ_STRT of symbol name (0 = no name) |
| +0x04 | flags | Word | Symbol attributes (see below) |
| +0x08 | value | Word | Symbol value/offset/size (interpretation depends on flags) |
| +0x0C | area_name | Word | Offset into OBJ_STRT of defining area name (0 if absolute) |

### 8.2 Symbol Flags (bits 0-31)

#### 8.2.1 Basic Flags (bits 0-7)

| Bit | Mask | Name | Meaning |
|-----|------|------|---------|
| 0 | 0x00000001 | **Defined** | Symbol is defined in this object file |
| 1 | 0x00000002 | **Global** | Symbol exported to other object files |
| 2 | 0x00000004 | **Weak extern** | Definition may override previous weak definition (bits 1 and 2 mutually exclusive) |
| 3 | 0x00000008 | **Common** | Symbol is common (alternative external common method) |
| 4 | 0x00000010 | **Constant** | Value is constant or common size, not address (must be set if bit 3 set) |
| 5 | 0x00000020 | **Case-insensitive** | Match ignoring case (case-insensitive languages like Basic) |
| 6 | 0x00000040 | **Weak** | Weak definition (may be undefined by strong definition) |
| 7 | 0x00000080 | **Strong** | Strong definition (opposite of bit 2) |

**Flag Combinations:**

| Bits 1:0 | Meaning |
|----------|---------|
| 00 | Reserved/unused |
| 01 | Defined, local scope only |
| 10 | External reference (not defined here) |
| 11 | Defined with global scope |

**Mutual Exclusions:**
- Bits 1 and 2 cannot both be set
- Bits 5 and 6 cannot both be set (weak and case-insensitive)
- Bits 6 and 7 have opposite meanings

#### 8.2.2 Datum and Type Fields (bits 16-23)

Multiple interpretations overlaid onto these bits based on symbol type (bits 24-27).

#### 8.2.3 Symbol Type (bits 24-27)

| Type | Bits 16-23 Interpretation | Description |
|------|---------------------------|-------------|
| 0 | Unused | Not used |
| 1-8 | Reserved | Reserved for future use |
| 9 | Bits 16-17: FP size in words (0=reserved, 1=1 word, 2=2 words, 3=3 words)<br>Bits 18-23: Register number (255=no register) | Floating-point number |
| 10 | Bits 16-23: Register number (255=no register) | Scratch register (not preserved across calls) |
| 11 | Bits 16-23: Register number (255=no register) | Non-scratch register (preserved across calls) |
| 12 | Bits 16-23: R = 256×CN + REGNO | Coprocessor register (scratch) |
| 13 | Bits 16-23: R = 256×CN + REGNO | Coprocessor register (non-scratch) |

#### 8.2.4 Reserved Bits (bits 8-15, 28-31)

Must be zero.

### 8.3 Symbol Value Field

Interpretation depends on flag bits:

| Condition | value Field Contains |
|-----------|---------------------|
| Absolute (bit 2 flags) | Absolute value of symbol |
| Common (bit 3 flags) | Size of common block in bytes |
| Defined (bit 0 flags), not absolute | Offset from base of area named in area_name field |
| Not defined | Meaningless (should be 0) |

### 8.4 Symbol Area Name Field

| Condition | area_name Field Contains |
|-----------|-------------------------|
| Defined (bit 0 set), not absolute (bit 2 clear) | Offset into OBJ_STRT of area name defining this symbol |
| Absolute or not defined | 0 (meaningless) |

---

## 9. OBJ_STRT - String Table Chunk

Stores all printable names referenced from header and symbol table.

### 9.1 Format

```
┌─────────────────────────┐
│ Length (4 bytes)        │  ← Word 0: total size including this length field
├─────────────────────────┤
│ String 1\0              │
├─────────────────────────┤
│ String 2\0              │
├─────────────────────────┤
│ ...                     │
├─────────────────────────┤
│ String N\0              │
└─────────────────────────┘
```

### 9.2 Properties

- **First 4 bytes:** Table length in bytes (including length field itself)
- **Minimum length:** 4 bytes
- **Minimum valid offset:** 4 (offsets 0-3 invalid)
- **Strings:** Sequence of non-control characters (codes 32-126, 160-255) terminated by NUL (0x00)
- **Alignment:** Strings may start on any byte boundary
- **Endianness:** Length word must match AOF/chunk file endianness

### 9.3 String Encoding

Each string:
- Contains only printable characters (ASCII 32-126 or extended 160-255)
- Terminated by NUL byte (0x00)
- NUL is part of string but not counted in length
- No alignment padding between strings

### 9.4 Referencing Strings

Strings are referenced by **byte offset** from start of chunk:
- Offset 0-3: invalid (within length word)
- Offset 4+: valid string positions

---

## 10. OBJ_IDFN - Identification Chunk

### 10.1 Purpose

Identifies the tool that created the object file.

### 10.2 Format

Single NUL-terminated string containing:
- Name and version of creating tool
- Printable characters only (codes 10-13, 32-126)
- NUL terminator (0x00)

### 10.3 Recommendations

- Use codes 10-13 (CR, LF, FF, VT) and 32-126 only
- Avoid codes 128-255 (interpretation is host-dependent)

### 10.4 Example

```
"ARM Assembler version 5.06\0"
```

---

## 11. Linker Symbol Table Format

The linker can export its internal symbol table to a text file for debugging and browsing.

### 11.1 Format

Plain text file of NUL-terminated lines, each with comma-separated fields:

```
name,value,class[,definingmodulename]
```

### 11.2 Fields

| Field | Required | Format | Description |
|-------|----------|--------|-------------|
| **name** | Yes | String | Symbol name (≥1 printable character) |
| **value** | Yes | &XXXXXXXX | Symbol value as hex prefixed with '&' (BBC Basic style) |
| **class** | Yes | Keyword | One of: Abs, RelocDef, RelocRef, ExtDef, ExtRef |
| **definingmodulename** | No | String | Module defining or referencing symbol |

### 11.3 Class Values

| Class | Meaning |
|-------|---------|
| **Abs** | Absolute value |
| **RelocDef** | Defined in a module, not referenced elsewhere |
| **RelocRef** | Referenced in ≥1 module, not defined |
| **ExtDef** | Defined in ≥1 module, referenced in ≥1 module |
| **ExtRef** | Referenced but not defined anywhere |

### 11.4 Example

```
main,&00008080,ExtDef,myapp
_stack_base,&00020000,Abs
printf,&00000000,ExtRef,myapp
```

---

## 12. Compatibility and Versioning

### 12.1 Version History

| Version | file_type | version_id | Changes |
|---------|-----------|------------|---------|
| 1.50 | 0xC5E2D080 | 310 (0x136) | Original documented version |

### 12.2 Forward Compatibility

- Undefined fields must be zero
- Future versions may define new meanings for reserved fields
- Zero will typically mean "default" or "not used"
- New chunks may be added
- Chunk file format allows ignoring unknown chunks

### 12.3 Backward Compatibility

- Tools should handle max_chunks > actual chunks needed
- Tools should ignore unknown chunks
- Tools should validate version_id and warn if newer than supported

---

## 13. Common Use Cases

### 13.1 Simple Object File

Minimal AOF for "hello world":

1. **Chunk file header:** 8 chunk slots
2. **OBJ_HEAD:**
   - 1 code area
   - 1 symbol (entry point)
3. **OBJ_AREA:** 
   - Code area data
   - No relocations
4. **OBJ_SYMT:** Entry point symbol
5. **OBJ_STRT:** Area and symbol names
6. **OBJ_IDFN:** Compiler identification

### 13.2 Relocatable Object with External References

1. **OBJ_HEAD:**
   - Multiple areas (code, data, zero-init)
   - Entry point
2. **OBJ_AREA:**
   - Code and data
   - Relocation directives for:
     - Internal references between areas
     - External symbol references
3. **OBJ_SYMT:**
   - Local symbols (defined, not global)
   - Global symbols (defined, global)
   - External references (not defined, global)
4. **OBJ_STRT:** All names
5. **OBJ_IDFN:** Tool identification

### 13.3 Library Member

For inclusion in ALF library:

1. **Reserved field** in area declarations must be 0
2. **STUB bits** should not be set
3. File should be complete and valid standalone
4. Typically contains:
   - Single function or related functions
   - External symbols for library interface
   - Local symbols for internal implementation

---

## 14. Validation and Error Checking

### 14.1 Required Validation

1. **Chunk file header:**
   - ChunkFileId = 0xC3CBC6C5 (or byte-swapped)
   - num_chunks ≤ max_chunks
   - All file_offsets divisible by 4

2. **OBJ_HEAD:**
   - file_type = 0xC5E2D080
   - version_id recognized
   - num_areas ≤ 255
   - entry_area = 0 or 1 ≤ entry_area ≤ num_areas

3. **Area attributes:**
   - Only one type bit (0-7) set
   - Alignment bits (13-14) encode valid power of 2
   - Combinations follow rules (e.g., common → bit 10 set)

4. **Relocation directives:**
   - offset < area_size
   - SID references valid symbol or area
   - Word to relocate is within area bounds

5. **Symbol table:**
   - name offset either 0 or ≥ 4 and < string table length
   - area_name offset valid if defined and not absolute
   - Flag combinations valid

6. **String table:**
   - First word = chunk size
   - Minimum size = 4
   - All referenced offsets within bounds
   - Strings properly NUL-terminated

### 14.2 Recommended Validation

1. Verify all offsets into string table point to valid string starts
2. Check symbol value ranges for reasonableness
3. Verify area sizes match data present
4. Check that absolute areas have area_base field
5. Validate debugging data if present

---

## 15. Example File Layout

### 15.1 Minimal AOF File

```
File Offset  Content
===========  =======
0x00000000   Chunk File Header
             - ChunkFileId: 0xC3CBC6C5
             - max_chunks: 8
             - num_chunks: 5
             
             Chunk Entry 0 (OBJ_HEAD)
             - chunkId: "OBJ_HEAD"
             - offset: 0x00000050
             - size: 0x00000030
             
             Chunk Entry 1 (OBJ_AREA)
             - chunkId: "OBJ_AREA"
             - offset: 0x00000080
             - size: 0x00000020
             
             Chunk Entry 2 (OBJ_IDFN)
             - chunkId: "OBJ_IDFN"
             - offset: 0x000000A0
             - size: 0x0000001C
             
             Chunk Entry 3 (OBJ_SYMT)
             - chunkId: "OBJ_SYMT"
             - offset: 0x000000C0
             - size: 0x00000010
             
             Chunk Entry 4 (OBJ_STRT)
             - chunkId: "OBJ_STRT"
             - offset: 0x000000D0
             - size: 0x00000020
             
             Chunk Entries 5-7: unused (offset=0)

0x00000050   OBJ_HEAD Chunk
             - file_type: 0xC5E2D080
             - version_id: 310
             - num_areas: 1
             - num_symbols: 1
             - entry_area: 1
             - entry_offset: 0
             Area Declaration 0:
             - area_name: 4 (offset in string table)
             - area_attributes: 0x00000204 (code, relocatable)
             - area_size: 0x00000010
             - num_relocs: 0
             - reserved: 0

0x00000080   OBJ_AREA Chunk
             - Area 0 data: 16 bytes of code
             - Area 0 relocations: none

0x000000A0   OBJ_IDFN Chunk
             - "ARM Assembler 5.06\0"

0x000000C0   OBJ_SYMT Chunk
             - Symbol 0:
               - name: 16 (offset in string table)
               - flags: 0x00000003 (defined, global)
               - value: 0x00000000
               - area_name: 4

0x000000D0   OBJ_STRT Chunk
             - Length: 0x00000020
             - Offset 4: "code\0"
             - Offset 9: padding
             - Offset 16: "main\0"
             - Offset 21: padding
```

---

## 16. Implementation Notes

### 16.1 Creating AOF Files

1. Allocate chunk file header with adequate max_chunks (8 recommended)
2. Create OBJ_HEAD with area declarations
3. Generate code/data into memory buffer
4. Record relocation directives during code generation
5. Build symbol table from labels and references
6. Collect strings into string table, recording offsets
7. Write identification string
8. Write all chunks to file
9. Update chunk file header with actual offsets and sizes

### 16.2 Reading AOF Files

1. Read chunk file header
2. Detect endianness from ChunkFileId
3. Byte-swap if necessary
4. Locate required chunks (OBJ_HEAD, OBJ_AREA)
5. Parse header to get area information
6. Read area data and relocations
7. Load symbol table and string table if present
8. Validate consistency

### 16.3 Performance Considerations

- Chunk file format allows random access to any chunk
- String table allows sharing of common strings
- Area data is contiguous for efficient loading
- Relocation directives grouped by area

---

## 17. Related Formats

### 17.1 ALF (ARM Library Format)

- Collection of AOF files
- Uses same chunk file format with "LIB_" prefix
- Contains directory, symbol table, and AOF members

### 17.2 AIF (ARM Image Format)

- Executable image format
- Output of linker from AOF files
- Contains header, code, data, optional debug info

### 17.3 Debugging Formats

- ASD (ARM Symbolic Debugger format)
- DWARF 1 and DWARF 2 (later ARM tools)
- Stored in debugging areas within AOF

---

## 18. References

### 18.1 Source Documents

1. RISC OS Programmer's Reference Manual, Appendix D: Code file formats
2. ARM Software Development Toolkit Reference Guide (DUI0041C), Chapter 15
3. Acorn technical documentation PLG-* series

### 18.2 Related Standards

- ARM Architecture Reference Manual
- ARM Procedure Call Standard (APCS)
- ELF (Executable and Linkable Format) - successor to AOF

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024 | Initial comprehensive specification compiled from multiple ARM documentation sources |

---

**End of ARM Object Format (AOF) Specification**
