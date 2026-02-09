# ARM Object Format (AOF) - Comprehensive Specification

**Version:** 3.10  
**Last Updated:** February 2025  
**Based on:** ARM DUI 0041C, RISC OS PRMs

---

## Table of Contents

1. [Overview](#overview)
2. [Format Fundamentals](#format-fundamentals)
3. [Chunk File Format](#chunk-file-format)
4. [AOF Structure](#aof-structure)
5. [AOF Header Chunk](#aof-header-chunk)
6. [Areas Chunk](#areas-chunk)
7. [Relocation Directives](#relocation-directives)
8. [Symbol Table](#symbol-table)
9. [String Table](#string-table)
10. [Identification Chunk](#identification-chunk)
11. [Attributes and Alignment](#attributes-and-alignment)
12. [Linker Concepts](#linker-concepts)
13. [Reference Implementation](#reference-implementation)

---

## Overview

ARM Object Format (AOF) is the primary object file format produced by ARM language processors (compilers, assemblers) and consumed by ARM linkers. It is used on RISC OS, 3DO, and various other ARM-based systems.

**Key Characteristics:**
- Chunk-based extensible format
- Supports multiple areas (code, data, zero-initialized)
- Rich relocation directives for position-independent code
- Comprehensive symbol table for linking
- Endianness-aware structure
- Scalable from simple to complex modules

**Primary Producers:**
- ARM C Compiler (armcc)
- ARM Assembler (armasm/ObjAsm)
- C++ compilers with ARM backend

**Primary Consumers:**
- ARM Linker (armlink/link)
- ARM Object Librarian (arlib)
- ARM Object Analyzer tools

---

## Format Fundamentals

### Design Principles

1. **Modularity:** Format broken into independent chunks
2. **Extensibility:** New chunk types can be added without breaking existing readers
3. **Simplicity:** Straightforward structures with minimal redundancy
4. **Flexibility:** Supports various compilation modes and architectures
5. **Linkability:** Contains all information needed for linking

### Endianness

**Critical:** AOF file endianness must match target system endianness

- No automatic byte-swapping occurs
- Endianness determined from first chunk ID value (0xC3CBC6C5)
- All word values stored in target byte order
- Strings stored in ascending address order (independent of endianness)

**Detection:**
```
ChunkFileId value in file:
  0xC3CBC6C5 (as read) -> Little-endian (x86, ARM-LE)
  0xC5C6CBC3 (as read) -> Big-endian (Motorola, ARM-BE)
```

### Byte Alignment

- Strings and bytes: any alignment
- Halfwords: 2-byte alignment
- Words: 4-byte alignment
- Offsets in file must be divisible by 4 (for chunk offsets)

---

## Chunk File Format

### Purpose

Chunk File Format provides a container for multiple independent data structures within a single file. It allows:
- Easy addition/removal of chunks
- Fast random access to specific chunks
- In-place modification without complete file rewrite
- Forward compatibility (unknown chunks ignored)

### Chunk File Header Structure

**Fixed Part (3 words = 12 bytes):**

```
Offset  Size    Field           Description
------  ----    -----           -----------
0x00    4       ChunkFileId     0xC3CBC6C5 (identifies as chunk file)
0x04    4       max_chunks      Max number of chunk entries in header
0x08    4       num_chunks      Current number of used chunks
```

**ChunkFileId Interpretation:**

The value 0xC3CBC6C5 is stored as 4 separate bytes in order:
- Byte 0: 'C' (0x43)
- Byte 1: '#' (0x23)
- Byte 2: 'Ë' (0xCB)
- Byte 3: 'Å' (0xC5)

Reading this value as a 32-bit integer reveals endianness:
- 0xC3CBC6C5 = little-endian
- 0xC5C6CBC3 = big-endian

**Variable Part (4 words per chunk):**

For each chunk entry (max_chunks total):

```
Offset                Size    Field           Description
------                ----    -----           -----------
(3 + i*4)*4          8       chunkId         8-byte chunk identifier
(3 + i*4)*4 + 8      4       file_offset     Byte offset in file (must be divisible by 4)
(3 + i*4)*4 + 12     4       size            Exact byte size of chunk data

(where i = 0, 1, 2, ... max_chunks-1)
```

### Chunk Identifier Format

**8-Byte Identifier Structure:**

Divided into two 4-byte parts:
1. **First 4 bytes:** Universal type (allocated by central authority)
2. **Second 4 bytes:** Component name within that domain

**Examples:**
```
Chunk Name              First 4 bytes    Second 4 bytes
----------              -----------      -----------------
OBJ_HEAD               "OBJ_"          "HEAD"
OBJ_AREA               "OBJ_"          "AREA"
OBJ_SYMT               "OBJ_"          "SYMT"
OBJ_STRT               "OBJ_"          "STRT"
OBJ_IDFN               "OBJ_"          "IDFN"
LIB_DIRY               "LIB_"          "DIRY"
LIB_DATA               "LIB_"          "DATA"
LIB_TIME               "LIB_"          "TIME"
```

**Storage Order:**

Characters are stored in ascending address order, independent of endianness:

```c
struct {
    char id[4];   // "OBJ_"
    char type[4]; // "HEAD"
} chunk_id;
```

This means "OBJ_HEAD" is stored as:
- Byte 0: 'O'
- Byte 1: 'B'
- Byte 2: 'J'
- Byte 3: '_'
- Byte 4: 'H'
- Byte 5: 'E'
- Byte 6: 'A'
- Byte 7: 'D'

### Chunk Entry Status

**file_offset Field Meanings:**

- **0:** Chunk entry is unused; size field ignored
- **Non-zero:** Valid offset; size bytes of data at this offset

**Layout Constraints:**

1. All chunk offsets must be divisible by 4
2. Chunks may appear in any order in file
3. Chunks may not overlap
4. Unused entries have offset = 0

### Chunk File Structure Example

```
Offset   Content
------   -------
0x00     0xC3CBC6C5     (ChunkFileId)
0x04     0x00000008     (max_chunks = 8)
0x08     0x00000005     (num_chunks = 5)

0x0C     "OBJ_HEAD"     (chunk id, first chunk)
0x14     0x00000080     (file offset = 128)
0x18     0x00000180     (size = 384 bytes)

0x1C     "OBJ_AREA"     (chunk id, second chunk)
0x24     0x00000200     (file offset = 512)
0x28     0x00001000     (size = 4096 bytes)

0x2C     "OBJ_STRT"     (chunk id, third chunk)
0x34     0x00001200     (file offset = 4608)
0x38     0x00000400     (size = 1024 bytes)

0x3C     "OBJ_SYMT"     (chunk id, fourth chunk)
0x44     0x00001600     (file offset = 5632)
0x48     0x00000800     (size = 2048 bytes)

0x4C     "OBJ_IDFN"     (chunk id, fifth chunk)
0x54     0x00001E00     (file offset = 7680)
0x58     0x00000100     (size = 256 bytes)

0x5C     "    "         (chunk id, sixth entry - unused)
0x64     0x00000000     (file offset = 0, marks as unused)
0x68     0x00000000     (size = 0)

...

0x80     [Data: OBJ_HEAD chunk contents]
```

---

## AOF Structure

### AOF Chunk Organization

An AOF file contains these standard chunks:

| Chunk Name | Purpose | Required | Min Size |
|------------|---------|----------|----------|
| OBJ_HEAD | Object file header with area declarations | Yes | 24 bytes + area entries |
| OBJ_AREA | Code/data contents and relocation directives | Yes | 0 (if no areas) |
| OBJ_STRT | String table for area/symbol names | No | 4 bytes |
| OBJ_SYMT | Symbol table with definitions and references | No | 0 bytes |
| OBJ_IDFN | Tool identification string | No | Variable |

**Minimal Valid AOF:**
- OBJ_HEAD chunk (required)
- OBJ_AREA chunk (required, may be empty)

**Typical AOF:**
- All 5 chunks listed above
- May include additional tool-specific chunks

### Linker Processing

**Input:** One or more AOF files

**Processing:**
1. Read all AOF files
2. Merge symbol tables
3. Resolve external references
4. Compute final addresses
5. Apply relocation directives
6. Output AIF (executable) or ALF (library)

---

## AOF Header Chunk (OBJ_HEAD)

### Structure Overview

**Two Contiguous Parts:**

1. **Fixed Header (6 words, 24 bytes)**
2. **Variable Area Declarations (5 words each, N areas)**

### Fixed Header Format

```
Offset  Size    Field                   Description
------  ----    -----                   -----------
0x00    4       Object File Type        0xC5E2D080 (always)
0x04    4       Version ID              310 (0x136) for this version
0x08    4       Number of Areas         Count of area declarations below
0x0C    4       Number of Symbols       Count of symbols in OBJ_SYMT
0x10    4       Entry Area Index        1-based index of entry point area
0x14    4       Entry Offset            Byte offset within entry area
```

**Object File Type:**

The value 0xC5E2D080 is a magic number that:
1. Identifies the file as AOF
2. Reveals endianness (byte-order invariant encoding)
3. Must be checked before processing file

This value was chosen to make endianness detection possible.

**Version ID:**

- 150: AOF version 1.50 (early format)
- 200: AOF version 2.00 (RISC OS 2 era)
- 310: AOF version 3.10 (current format, used here)

Readers should handle multiple versions gracefully.

### Area Declarations

Following the fixed header, immediately after offset 0x18, the file contains `num_areas` area declarations:

**Area Declaration Format (5 words each):**

```
Offset  Size    Field                   Description
------  ----    -----                   -----------
0x00    4       Area Name               Offset into string table
0x04    4       Attributes+Alignment    Bit flags and alignment
0x08    4       Area Size               Size in bytes
0x0C    4       Number of Relocations   Relocation directives count
0x10    4       Base Address            (only for absolute areas)
```

**Area Name Field:**

- **0:** No string in table; area name defaults to "area##" (## = area number)
- **Non-zero:** Byte offset into OBJ_STRT chunk for area name string

**Attributes+Alignment Field:**

See detailed [Attributes and Alignment](#attributes-and-alignment) section.

**Area Size Field:**

- Size in bytes (must be multiple of 4)
- For common areas: minimum acceptable size
- For ZI areas: total size needed at runtime
- For regular areas: exact size of data in OBJ_AREA

**Number of Relocations:**

- Count of relocation directives for this area
- Relocations follow area data in OBJ_AREA chunk
- Value 0 = no relocations

**Base Address Field:**

- Present only if Absolute attribute (bit 8) is set
- Contains fixed address for absolute area
- Linker does not move absolute areas
- Used for memory-mapped I/O, fixed ROM locations

---

## Areas Chunk (OBJ_AREA)

### Structure and Layout

The OBJ_AREA chunk contains:

```
Area 1 Data         (size from header, or 0 if ZI flag set)
Area 1 Relocations  (num_relocs[1] entries, 8 bytes each)
Area 2 Data
Area 2 Relocations
...
Area N Data
Area N Relocations
```

**Alignment:** All areas and relocation tables 4-byte aligned

**Data Storage:** Only for initialized areas
- If ZI (zero-initialized) bit not set: area data included
- If ZI bit set: area data absent, linker zeros area at load time

### Data Organization

**Contiguous Storage:**
- Area data and relocations follow sequentially
- No gaps between areas
- Relocation table follows immediately after area data
- Next area begins 4-byte aligned after its relocations

**Example with 2 Areas:**

```
Offset      Content
------      -------
0x00        Area 1 data [0x100 bytes]
0x100       Area 1 relocation entry 1 [8 bytes]
0x108       Area 1 relocation entry 2 [8 bytes]
0x110       Area 1 relocation entry 3 [8 bytes]
0x118       [padding to 4-byte boundary - 0 bytes]
0x118       Area 2 data [0x200 bytes]
0x318       Area 2 relocation entry 1 [8 bytes]
0x320       Area 2 relocation entry 2 [8 bytes]
0x328       [padding to 4-byte boundary - 0 bytes]
```

---

## Relocation Directives

### Purpose

Relocation directives describe how the linker must modify code/data when areas are assigned their final addresses or when symbolic references are resolved.

### Relocation Directive Format

**Size:** 8 bytes per directive (2 words)

```
Word    Offset  Size    Field               Description
----    ------  ----    -----               -----------
0       0x00    4       Offset              Byte offset in area for relocation
1       0x04    4       Flags and SID       Relocation type and target
```

### Offset Field

**Value:** Byte offset from start of area to field requiring relocation

**Subject Field Types:**

- Byte (8-bit): At specified offset
- Halfword (16-bit): Starts at offset, alignment = 2
- Word (32-bit): Starts at offset, alignment = 4
- Instruction: ARM or Thumb instruction(s) at offset

### Flags and SID Word

**Bit Layout:**

```
Bits    Field               Description
----    -----               -----------
0-23    SID (24 bits)       Depends on bit 27:
                            A=1: Symbol table index
                            A=0: Area index
24-25   FT (2 bits)         Field type: 00=Byte, 01=Half, 10=Word, 11=Instr
26      R (1 bit)           Relocation type: 0=additive, 1=PC-relative
27      A (1 bit)           Address source: 0=area, 1=symbol
28      B (1 bit)           Based relocation flag
29-30   II (2 bits)         Instruction count constraint
31      (sign bit)          Must be 0 for most cases
```

### Field Type Encoding (Bits 24-25)

```
Value   Field Type          Description
-----   ----------          -----------
00      Byte (8-bit)        Single unsigned byte
01      Halfword (16-bit)   Two-byte value, 2-byte aligned
10      Word (32-bit)       Four-byte value, 4-byte aligned
11      Instruction/Seq     ARM or Thumb instruction(s)
```

### Relocation Type (Bits 26, 28)

**R=0, B=0: Plain Additive**
```
result = field + relocation_value
```
Most common case. Field contains constant addend.

**R=1, B=0: PC-Relative**
```
result = field + (relocation_value - subject_address)
```
For branch targets, relative references.

**R=0, B=1: Based Area**
```
result = field + (relocation_value - base_of_area_containing_relocation_value)
```
For based data areas (shared libraries).

### Instruction Count Constraint (Bits 29-30)

For instruction relocations, limits how many instructions may be modified:

```
Value   Constraint
-----   -----------
00      No constraint (all instructions in sequence may be modified)
01      At most 1 instruction
10      At most 2 instructions
11      At most 3 instructions
```

### Relocation Examples

**Example 1: Simple Additive Word Relocation**

```
Offset: 0x100
Flags:  0x28000000 (A=1, FT=10, SID=0)

Meaning: Add symbol[0] value to word at offset 0x100
result = *(word at 0x100) + symbol[0].value
```

**Example 2: PC-Relative Branch**

```
Offset: 0x044
Flags:  0x2C000002 (A=1, FT=11, R=1, SID=2)

Meaning: Relocate instruction at 0x044 (branch) to symbol[2]
result = (branch offset) + (symbol[2].address - 0x044)
```

**Example 3: Area-Based Relocation**

```
Offset: 0x200
Flags:  0x2A000005 (A=0, FT=10, SID=5)

Meaning: Add area[5].base to word at offset 0x200
result = *(word at 0x200) + area[5].base_address
```

---

## Symbol Table (OBJ_SYMT)

### Purpose

The symbol table contains all globally-visible symbols and external references in the object file.

### Symbol Entry Format

**Size:** 16 bytes per symbol (4 words)

```
Offset  Size    Field               Description
------  ----    -----               -----------
0x00    4       Name                Offset into OBJ_STRT
0x04    4       Attributes/Flags    Bit flags defining symbol properties
0x08    4       Value               Address offset or constant value
0x0C    4       Area Name           Offset into OBJ_STRT of area name
```

### Name Field

**0:** No string; symbol has no name (rare)

**Non-zero:** Byte offset into OBJ_STRT string table

### Attributes/Flags Field

**Bit Layout:**

```
Bits    Field               Description
----    -----               -----------
0       DEFINED             1 = defined in this file
1       GLOBAL              1 = visible globally
2       ABSOLUTE            1 = absolute value (not area-relative)
3       CASE_INSENSITIVE    1 = ignore case in symbol matching
4       WEAK                1 = weak definition (overridable)
5       (reserved)          must be 0
6       COMMON              1 = common block definition
7       STRONG              1 = strong definition (prefer over weak)
8-11    (reserved)          must be 0
12-15   SYMBOL_TYPE         Type-specific field
16-23   DATUM_TYPE          Data or register type info
24-27   EXTENDED_TYPE       Extended type code
28-31   (reserved)          must be 0
```

### Symbol Attributes Details

**Bit 0 (DEFINED):**
- 1 = Symbol defined in this object file
- 0 = Symbol is external reference

**Bit 1 (GLOBAL):**
- 1 = Defined globally; visible to other modules
- 0 = Local to this file

**Combinations (Bits 0-1):**

```
Bits 0-1    Meaning
--------    -------
00          Reserved (should not occur)
01          Local definition (defined here, not exported)
10          External reference (defined elsewhere)
11          Global definition (defined here, exported)
```

**Bit 2 (ABSOLUTE):**
- 1 = Value field is absolute constant
- 0 = Value field is offset from area base

**Bit 4 (WEAK):**
- 1 = Weak definition; linker can override with strong definition
- 0 = Strong definition

**Bit 6 (COMMON):**
- 1 = Symbol is common block (uninitialized shared area)
- 0 = Regular symbol

**Bit 7 (STRONG):**
- 1 = Strong definition; preferred over weak
- 0 = Not particularly strong

### Value Field

**For Defined Symbols:**
- If ABSOLUTE bit set: Contains absolute value (constant)
- If ABSOLUTE not set: Offset from area base address

**For Undefined References:**
- 0 (unused)

**For Common Symbols:**
- Size in bytes of common block

### Area Name Field

**For Area-Relative Symbols:**
- Offset into OBJ_STRT of area name
- Linker combines: `symbol.address = area[name].base + symbol.value`

**For Absolute Symbols:**
- 0 (unused)

### Extended Symbol Types (Bits 24-27)

**Type 0:** Undefined (not used)

**Types 1-8:** Reserved

**Type 9: FP Register**
- Bits 16-17: FP size (0=code, 1=1 word, 2=2 words, 3=3 words)
- Bits 18-23: FP register number

**Type 10: Scratch Register**
- Bits 16-23: Register number
- Value not preserved across procedure calls

**Type 11: Non-Scratch Register**
- Bits 16-23: Register number
- Value preserved across procedure calls

**Type 12: Coprocessor Register**
- Bits 16-23: Register encoding (CN*256 + REGNO)
- Scratch register (not preserved)

**Type 13: Coprocessor Register (Non-Scratch)**
- Same as type 12, but value preserved

---

## String Table (OBJ_STRT)

### Purpose

The string table stores all printable strings referenced by headers and symbol tables (area names, symbol names, etc.).

### Format

**Structure:**
```
Word 0:         Length field (4 bytes, includes self)
Bytes 4+:       Sequence of null-terminated strings
```

**String Encoding:**
- Characters: ASCII printable (32-126, 160-255)
- Terminator: NUL (0x00) byte
- Packing: No padding between strings

**Minimum Content:**
- At least 4 bytes (for length field only)
- First valid offset: 4 (after length word)

### Length Field

**Value:** Total size of string table including length word itself

**Example:**
```
Offset 0x00: 0x00000004  (length = 4, empty string table)

Offset 0x00: 0x00000020  (length = 32, includes 28 bytes of strings)
Offset 0x04: "area1\0" (6 bytes)
Offset 0x0A: "main\0" (5 bytes)
Offset 0x0F: "process\0" (8 bytes)
Offset 0x17: "var\0" (4 bytes)
Offset 0x1B: [5 unused bytes]
```

### Offset Usage

**In Headers:**
- Area Name field contains byte offset into string table
- If offset = 0: no name string (use default "area##")
- Otherwise: string at that offset

**In Symbol Table:**
- Name field: offset to symbol name string
- Area Name field: offset to area name string
- If offset = 0: no string

### Strings with Special Meanings

**Area Names:**
- Match between different files' areas (for linking)
- Case-sensitive comparison
- May include generated names like "SHL$$data"

**Symbol Names:**
- Identify functions and variables
- Usually match C/C++ identifiers after name mangling
- May be case-insensitive (if flag set)

---

## Identification Chunk (OBJ_IDFN)

### Purpose

Contains human-readable identification of the tool that generated the object file.

### Format

**Content:** Single null-terminated string (no length field)

**Allowed Characters:**
- Printable ASCII (0x20-0x7E)
- Control codes 0x0A (LF), 0x0D (CR)
- 8-bit characters (0x80-0xFF) discouraged

**Examples:**
```
"ARM C Compiler v4.1.0\0"
"ARM Assembler v2.50\0"
"ObjAsm 3.50\0"
"gcc-4.8.2 arm-none-eabi\0"
```

### Usage

Not used during linking, but useful for:
- Identifying object file origin in debuggers
- Detecting tool version-specific issues
- Archival and documentation

---

## Attributes and Alignment

### Overview

Area attributes define the nature and properties of code/data areas.

### Alignment Field (Bits 0-7)

**Encoding:** Power of 2 (typically 2-32)

```
Value   Alignment   Meaning
-----   ---------   -------
2       2^2 = 4     4-byte boundary (word alignment)
3       2^3 = 8     8-byte boundary
4       2^4 = 16    16-byte boundary
...
5       2^5 = 32    32-byte boundary (common for cache)
```

**Area Start Constraint:**
```
area_address % (2^alignment) == 0
```

### Type Bits (Bits 8-15)

**Bit 8: Absolute (0x00000100)**
- Area placed at fixed address (Base Address field)
- Linker does not move absolute areas

**Bit 9: Code (0x00000200)**
- 1 = Area contains executable code
- 0 = Area contains data

**Bit 10: Common Definition (0x00000400)**
- Area is common block definition
- Multiple common areas with same name overlaid
- Size = minimum required

**Bit 11: Common Reference (0x00000800)**
- Reference to common block
- No data in file (implies ZI)
- Size ≤ definition size

**Bit 12: Zero-Initialized (0x00001000)**
- Area uninitialized, must be zeroed
- No data in OBJ_AREA chunk
- Linker allocates and zeros at load time

**Bit 13: Read-Only (0x00002000)**
- Area not modified after relocation
- Code and debug areas must have this set
- Incompatible with ZI (bit 12)

**Bit 14: Position-Independent (0x00004000)**
- Code is position-independent
- Addresses use PC-relative or register-relative refs
- Essential for shared libraries

**Bit 15: Debugging Table (0x00008000)**
- Area contains symbolic debugging data
- Linker places in accessible section
- Usually also read-only (bit 13)

### Code Area Attributes (Bits 16-19)

**Only significant if Code bit (9) is set:**

**Bit 16: 32-bit APCS (0x00010000)**
- Code compiled for 32-bit ARM Procedure Call Standard
- May not run in 26-bit mode
- Affects register usage and call convention

**Bit 17: Reentrant (0x00020000)**
- Code follows reentrant coding rules
- Uses based addressing for data
- Can be shared (ROM) with private data

**Bit 18: Extended FP (0x00040000)**
- Uses ARM extended floating-point instructions
- Loads/stores FP state with LFM/SFM
- Incompatible with older ARM processors

**Bit 19: No Software Stack Check (0x00080000)**
- Code does not include software stack limit checks
- APCS variant without stack overflow detection
- May be incompatible with stack-checked code

### Data Area Attributes (Bits 20-21)

**Only significant if Code bit (9) is NOT set:**

**Bit 20: Based (0x00100000)**
- Data addressed via base register (SB)
- Used in shared libraries, position-independent code
- Bits 24-27 specify base register

**Bit 21: Shared Library Stub (0x00200000)**
- Data is stub for shared library call site
- Linker-internal; not set in source
- Treated similarly to common definition

### Base Register (Bits 24-27)

**Encoding:** Which base register for based addressing

```
Value   Register    Meaning
-----   --------    -------
0       (none)      Not based
1-15    R1-R15      Base register for addressing
(Typically: 9 = SB, 10 = SL, 11 = FP, etc.)
```

---

## Linker Concepts

### Area Ordering

Linker orders areas in output by:

1. **Attributes** (read-only before read-write)
2. **Lexicographic area name** (case-significant)
3. **Input file position** (from link list)

### Symbol Resolution

**External References:**
- Linker searches all loaded object files and libraries
- First definition wins (or strongest definition if multiple)
- Error if unresolved external remains

**Common Block Unification:**
- All common areas with same name treated as one
- Linker picks largest size requirement
- Multiple definitions must have identical contents

### Relocation Processing

**During Linking:**
1. Compute final base address for each area
2. For each relocation directive:
   - Determine relocation value (target address or constant)
   - Apply relocation formula to field
   - Store result in output

**Relocation Formulas:**

```
Plain Additive (R=0, B=0):
  output = field + relocation_value

PC-Relative (R=1, B=0):
  output = field + (relocation_value - field_address)

Based Area (R=0, B=1):
  output = field + (relocation_value - base_of_area_group)
```

---

## Reference Implementation

### C Structure Definitions

```c
/* Chunk File Header */
typedef struct {
    uint32_t chunk_file_id;         /* 0xC3CBC6C5 */
    uint32_t max_chunks;
    uint32_t num_chunks;
} ChunkFileHeader;

typedef struct {
    char     chunk_id[8];           /* e.g., "OBJ_HEAD" */
    uint32_t file_offset;
    uint32_t size;
} ChunkEntry;

/* AOF Header */
typedef struct {
    uint32_t object_file_type;      /* 0xC5E2D080 */
    uint32_t version_id;             /* 310 for version 3.10 */
    uint32_t num_areas;
    uint32_t num_symbols;
    uint32_t entry_area_index;       /* 1-based */
    uint32_t entry_offset;
} AOFHeaderFixed;

typedef struct {
    uint32_t area_name;             /* String table offset */
    uint32_t attributes;            /* Includes alignment in bits 0-7 */
    uint32_t area_size;
    uint32_t num_relocations;
    uint32_t base_address;          /* Only if absolute attribute set */
} AreaDeclaration;

/* Relocation Directive */
typedef struct {
    uint32_t offset;                /* Byte offset in area */
    uint32_t flags_and_sid;         /* Type and SID */
} RelocationDirective;

#define GET_RELOC_FT(flags)     (((flags) >> 24) & 0x03)
#define GET_RELOC_R(flags)      (((flags) >> 26) & 0x01)
#define GET_RELOC_A(flags)      (((flags) >> 27) & 0x01)
#define GET_RELOC_SID(flags)    ((flags) & 0x00FFFFFF)
#define GET_RELOC_B(flags)      (((flags) >> 28) & 0x01)

/* Symbol Table Entry */
typedef struct {
    uint32_t name;                  /* String table offset */
    uint32_t attributes;            /* Flags and type */
    uint32_t value;                 /* Address or constant */
    uint32_t area_name;             /* String table offset */
} SymbolEntry;

#define SYM_DEFINED             0x00000001
#define SYM_GLOBAL              0x00000002
#define SYM_ABSOLUTE            0x00000004
#define SYM_CASE_INSENSITIVE    0x00000008
#define SYM_WEAK                0x00000010
#define SYM_COMMON              0x00000040
#define SYM_STRONG              0x00000080

/* Area Attributes */
#define AREA_ABSOLUTE           0x00000100
#define AREA_CODE               0x00000200
#define AREA_COMMON_DEF         0x00000400
#define AREA_COMMON_REF         0x00000800
#define AREA_ZI                 0x00001000
#define AREA_RO                 0x00002000
#define AREA_PI                 0x00004000
#define AREA_DEBUG              0x00008000
#define AREA_32BIT_APCS         0x00010000
#define AREA_REENTRANT          0x00020000
#define AREA_EXTENDED_FP        0x00040000
#define AREA_NO_STACK_CHECK     0x00080000
#define AREA_BASED              0x00100000
#define AREA_SHL_STUB           0x00200000

#define GET_AREA_ALIGNMENT(attr) ((attr) & 0xFF)
#define GET_AREA_BASE_REG(attr)  (((attr) >> 24) & 0x0F)
```

### AOF Parser Skeleton

```c
#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    ChunkFileHeader header;
    ChunkEntry *entries;
    FILE *file;
} AOFContext;

int aof_open(const char *filename, AOFContext *ctx) {
    ctx->file = fopen(filename, "rb");
    if (!ctx->file) return -1;
    
    if (fread(&ctx->header, sizeof(ChunkFileHeader), 1, ctx->file) != 1) {
        fclose(ctx->file);
        return -1;
    }
    
    ctx->entries = malloc(ctx->header.max_chunks * sizeof(ChunkEntry));
    if (!ctx->entries) {
        fclose(ctx->file);
        return -1;
    }
    
    if (fread(ctx->entries, sizeof(ChunkEntry), 
              ctx->header.num_chunks, ctx->file) != ctx->header.num_chunks) {
        free(ctx->entries);
        fclose(ctx->file);
        return -1;
    }
    
    return 0;
}

ChunkEntry* aof_find_chunk(AOFContext *ctx, const char *chunk_id) {
    for (uint32_t i = 0; i < ctx->header.num_chunks; i++) {
        if (strncmp(ctx->entries[i].chunk_id, chunk_id, 8) == 0) {
            return &ctx->entries[i];
        }
    }
    return NULL;
}

int aof_read_chunk(AOFContext *ctx, ChunkEntry *chunk, void **data) {
    *data = malloc(chunk->size);
    if (!*data) return -1;
    
    if (fseek(ctx->file, chunk->file_offset, SEEK_SET) != 0) {
        free(*data);
        return -1;
    }
    
    if (fread(*data, chunk->size, 1, ctx->file) != 1) {
        free(*data);
        return -1;
    }
    
    return 0;
}

void aof_close(AOFContext *ctx) {
    if (ctx->entries) free(ctx->entries);
    if (ctx->file) fclose(ctx->file);
}
```

---

## Summary

ARM Object Format provides a robust container for compiled/assembled code with full relocation and symbol information needed by linkers. Its chunk-based design allows extensibility while maintaining simplicity.

Key strengths:
- Clean separation of code, data, symbols, relocations
- Full symbol and relocation information for linking
- Supports multiple compilation modes and architectures
- Extensible via additional chunks
- Efficient random access to chunks
- Endianness-aware design

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals, Appendix D
- ARM Object File Specification
- ARM Linker User Guide
