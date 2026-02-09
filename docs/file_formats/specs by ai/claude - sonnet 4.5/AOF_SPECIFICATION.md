# ARM Object Format (AOF) - Complete File Format Specification

## Document Information

- **Format Name**: ARM Object Format (AOF)
- **File Extension**: .o, .obj
- **Byte Order**: Little-endian or Big-endian (matches target)
- **Version**: AOF 310 (0x136)
- **Copyright**: Based on ARM Limited and Acorn Computers documentation

---

## 1. Overview

ARM Object Format (AOF) is the object file format used by ARM language processors (compilers, assemblers) and the ARM linker. It provides a generalized superset of Unix's a.out format capabilities while maintaining simplicity for generation and processing.

### 1.1 Purpose and Design Goals

- **Primary Use**: Output from compilers/assemblers (CC, ObjAsm)
- **Design Philosophy**: Simple to generate and process
- **Not Optimized For**: Maximum expressiveness or compactness
- **Linker Support**: Input to ARM linker (armlink) to produce AIF or other formats

### 1.2 Key Features

1. **Chunk-Based Structure**: Layered on Chunk File Format
2. **Separate Areas**: Code and data in named, attributed areas
3. **Relocation Support**: Comprehensive relocation directive system
4. **Symbol Tables**: Global and local symbol definitions
5. **Extensibility**: Additional chunks can be added
6. **Byte Sex Variants**: Little-endian and big-endian versions

---

## 2. Terminology

### 2.1 Basic Data Types

| Type | Size | Alignment | Description |
|------|------|-----------|-------------|
| **Byte** | 8 bits | Any | Unsigned unless stated; stores flags or characters |
| **Half Word** | 16 bits | 2-byte | Usually unsigned; little-endian byte order |
| **Word** | 32 bits | 4-byte | Usually unsigned; little-endian byte order |
| **String** | Variable | Any | Byte sequence terminated by NUL (0x00); NUL included but not counted |

**Important**: For data in a file, "address" means offset from the start of the file.

### 2.2 Byte Order (Endianness)

AOF exists in two variants:

#### Little-Endian AOF
- **Byte Order**: Least significant byte at lowest address
- **Used By**: DEC, Intel, Acorn, ARM (default)
- **Example**: 0x12345678 stored as [78 56 34 12]

#### Big-Endian AOF
- **Byte Order**: Most significant byte at lowest address
- **Used By**: IBM, Motorola, Apple
- **Example**: 0x12345678 stored as [12 34 56 78]

**Rules**:
- AOF file endianness matches target ARM system endianness
- Cannot mix endianness in a single link operation
- Linker accepts either but rejects mixed inputs

### 2.3 Alignment Rules

- **Strings and Bytes**: Any byte boundary
- **Words**: 4-byte boundaries
- **Half Words**: 2-byte boundaries (within area contents)
- **AOF Fields**: Words only, 4-byte aligned

---

## 3. Overall Structure of AOF Files

### 3.1 Chunk File Format Foundation

AOF is layered on Chunk File Format, which provides:
- Simple access to distinct data chunks
- Efficient updating of individual chunks
- Extensibility for custom chunks
- File copying without chunk content knowledge

### 3.2 Chunk File Header

Every chunk file begins with a header:

```
+----------------------------------+
| ChunkFileId (0xC3CBC6C5)         | 4 bytes - File signature
+----------------------------------+
| max_chunks                       | 4 bytes - Max number of chunks
+----------------------------------+
| num_chunks                       | 4 bytes - Current number used
+----------------------------------+
| Chunk Entry 1                    | 16 bytes (4 words)
|   - chunkId (8 bytes)            |
|   - offset (4 bytes)             |
|   - size (4 bytes)               |
+----------------------------------+
| Chunk Entry 2                    | 16 bytes
+----------------------------------+
...
| Chunk Entry N                    | 16 bytes
+----------------------------------+
| Chunk 1 Data                     |
+----------------------------------+
| Chunk 2 Data                     |
+----------------------------------+
...
+----------------------------------+
```

#### Chunk File Header Fields

| Field | Size | Description |
|-------|------|-------------|
| **ChunkFileId** | 4 bytes | Magic number: 0xC3CBC6C5. If reads as 0xC5C6CBC3, endianness reversed |
| **max_chunks** | 4 bytes | Maximum chunks allowed (fixed at file creation) |
| **num_chunks** | 4 bytes | Current number of chunks in use (0 to max_chunks, redundant) |

#### Chunk Entry Format (16 bytes each)

| Field | Size | Description |
|-------|------|-------------|
| **chunkId** | 8 bytes | Two 4-character names identifying chunk type |
| **offset** | 4 bytes | Byte offset to chunk start (must be divisible by 4); 0 = unused |
| **size** | 4 bytes | Exact byte size of chunk (need not be multiple of 4) |

### 3.3 Chunk Identification

The **chunkId** is 8 bytes split into two parts:

- **Bytes 0-3**: Universal unique name (allocated by Acorn/ARM)
  - For AOF: `"OBJ_"` (0x4F424A5F)
  - For ALF: `"LIB_"` (0x4C49425F)
- **Bytes 4-7**: Component identifier within domain
  - Examples: `"HEAD"`, `"AREA"`, `"SYMT"`, etc.

**Storage**: Characters stored in ascending address order (string order), independent of endianness.

Example: `"OBJ_HEAD"` stored as:
```
Offset: 0  1  2  3  4  5  6  7
Bytes:  4F 42 4A 5F 48 45 41 44
Chars:  O  B  J  _  H  E  A  D
```

### 3.4 AOF Chunk Types

AOF defines five standard chunks:

| Chunk Name | ChunkId | Required | Contents |
|------------|---------|----------|----------|
| **Header** | `OBJ_HEAD` | Yes | Object file header and area declarations |
| **Areas** | `OBJ_AREA` | Yes | Area contents and relocation directives |
| **Identification** | `OBJ_IDFN` | No | Tool identification string |
| **Symbol Table** | `OBJ_SYMT` | Typical | Symbol definitions and references |
| **String Table** | `OBJ_STRT` | Typical | Name strings for symbols and areas |

**Ordering**: Chunks may appear in any order, but processors should preserve order for compatibility with other formats (e.g., Unix a.out).

**Extensibility**: Additional chunks (e.g., language-specific debugging data) may be added. Convention: reserve space for 8 chunks when creating AOF files.

---

## 4. OBJ_HEAD: Header Chunk Format

The header chunk consists of two contiguous parts:

1. **Fixed Header** (6 words = 24 bytes)
2. **Area Declarations** (5 or 6 words per area)

### 4.1 Fixed Header (24 bytes)

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x00 | **file_type** | Word | 0xC5E2D080 - marks file as AOF |
| 0x04 | **version_id** | Word | 310 (0x136) for current AOF version |
| 0x08 | **num_areas** | Word | Number of areas (should be ≤ 255) |
| 0x0C | **num_symbols** | Word | Number of symbols in OBJ_SYMT |
| 0x10 | **entry_area** | Word | 1-origin index of area containing entry point (0 = none) |
| 0x14 | **entry_offset** | Word | Byte offset of entry point within entry_area |

#### Field Details

**file_type** (0xC5E2D080):
- Identifies file as ARM Object Format
- Endianness must match chunk file header
- If appears byte-reversed, entire file uses opposite endianness

**version_id** (310 = 0x136):
- Current version number
- Chosen to vaguely associate with C/Assembler version that first produced it

**num_areas**:
- Number of code/data areas in the file
- Should be ≤ 255 (convention, not hard limit)
- Determines number of area declarations following fixed header

**num_symbols**:
- Count of symbols in OBJ_SYMT chunk
- 0 if no symbol table chunk present

**entry_area**:
- 1-origin index into area declarations array
- 0 means no entry point defined
- If non-zero, specifies which area contains program entry

**entry_offset**:
- Byte offset from base of entry_area to first instruction
- Only meaningful if entry_area ≠ 0
- Entry address = base(entry_area) + entry_offset

### 4.2 Area Declarations (5 words each, 6 for absolute areas)

Following the fixed header are **num_areas** area declarations:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | **area_name** | Word | Offset into OBJ_STRT of area name |
| +0x04 | **area_attributes** | Word | Attribute and alignment bit flags |
| +0x08 | **area_size** | Word | Size of area in bytes |
| +0x0C | **num_relocs** | Word | Number of relocation directives |
| +0x10 | **reserved** | Word | Must be 0 |
| +0x14 | **area_base** | Word | Only for absolute areas (bit 8 set) |

#### Area Name

- **Type**: Offset into OBJ_STRT chunk
- **Value 0**: No name string (convention: use "areaNN" where NN is 0-origin area number)
- **Uniqueness**: Each area within an object file must have a unique name

#### Area Attributes and Alignment

The area_attributes word is a complex bit field:

```
31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
[  Stub Bits    ][             Reserved              ][AT Bits ][Alignment ] [     Type Bits     ]
                                                        15-8       7-0          7   6 5 4 3 2 1 0
```

**Alignment** (Bits 0-7):
- Power of 2 alignment requirement: 2^n
- Common values:
  - 0 = byte aligned
  - 1 = halfword aligned (2^1 = 2)
  - 2 = word aligned (2^2 = 4)
  - 3 = doubleword aligned (2^3 = 8)

**Type Bits** (Bits 0-7, mutually exclusive in AOF):

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 0 | 0x00000001 | Absolute | Area base address is fixed |
| 1 | 0x00000002 | Relocatable | Area can be relocated |
| 2 | 0x00000004 | Code | Area contains executable code |
| 3 | 0x00000008 | Common Block | Uninitialised, shares space with same-named areas |
| 4 | 0x00000010 | Common Definition | Common area that must be a block |
| 5 | 0x00000020 | Linker Use | For linker internal use |
| 6 | 0x00000040 | Debugging Tables | Area contains debugging data |
| 7 | 0x00000080 | Zero-Initialised | BSS - storage initialized to 0 |

**AT (Area Type) Bits** (Bits 8-15):

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 8 | 0x00000100 | Absolute | Must use specified base address |
| 9 | 0x00000200 | Code | Contains executable instructions |
| 10 | 0x00000400 | Common Definition | Overlay with same-named areas |
| 11 | 0x00000800 | Common Reference | Reference to common area |
| 12 | 0x00001000 | Not Initialised | No data in OBJ_AREA chunk |
| 13 | 0x00002000 | Read-Only | Not modified after relocation |
| 14 | 0x00004000 | Position Independent | No external address refs |
| 15 | 0x00008000 | Debugging Table | Contains debug information |

**Code Attributes** (Bits 16-22, only if bit 9 set):

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 16 | 0x00010000 | 32-bit APCS | Complies with 32-bit procedure call standard |
| 17 | 0x00020000 | Reentrant | Code is reentrant |
| 18 | 0x00040000 | Extended FP | Uses ARM FP instruction set (LFM/SFM) |
| 19 | 0x00080000 | No SW Stack Check | Software stack checking disabled |
| 20 | 0x00100000 | Thumb Code | All relocations are Thumb code |
| 21 | 0x00200000 | Halfword Instructions | May contain ARM halfword instructions |
| 22 | 0x00400000 | Interworking | Suitable for ARM/Thumb interworking |

**Stub Bits** (Bits 24-31):
- Used and updated by linker
- Initial value undefined except for common blocks (bits 24-27 = max alignment in log2 form)
- Should not be set in files to be placed in libraries

**Reserved** (Bits 23, 5):
- Must be 0

#### Area Attribute Combinations

**Code Area** (Bit 9 set):
- Only bits 14, 16-22 may be set in addition
- Bit 14 = position independent
- Read-only (bit 13) typically set but not required
- Common flags (bits 10, 11) invalid

**Common Area** (Bits 10 or 11 set):
- Bit 11 in AT bits must be set
- Bit 13 (read-only) must NOT be set
- For common blocks (type bit 4), stub bits encode max alignment

**Absolute Area** (Type bit 0 or AT bit 8 set):
- Bit 15 in AT bits must be set
- area_base field must be present
- Base address cannot be relocated

**Debugging Table** (Type bit 6 or AT bit 15 set):
- Both bits should be set
- Bit 9 (code) is ignored
- Typically read-only (bit 13)

**Zero-Initialised** (Type bit 7 set):
- Like common area but storage guaranteed zero
- Bit 12 in AT bits should be set
- Incompatible with read-only (bit 13)
- Size must be multiple of 4

**Relocatable Data** (Type bit 1, not code, not common):
- May have bit 13 (read-only) set

#### Area Size

- **Units**: Bytes
- **Common/Zero-Init**: Minimum size acceptable to defining module
- **Zero-Init**: Must be multiple of 4 (typically multiple of 16)
- **Other Areas**: Exact size of data in OBJ_AREA chunk
- **Uninitialised** (bit 12 set): No data in OBJ_AREA

#### Number of Relocations

- Count of relocation directives in OBJ_AREA for this area
- Follows immediately after area's data in OBJ_AREA chunk

#### Reserved Word

- **Must be 0** in files to be put in libraries
- Future expansion field

#### Area Base (Absolute Areas Only)

- **Present When**: Type bit 0 or AT bit 8 set
- **Size**: 1 word (4 bytes)
- **Content**: Fixed address where area must be located
- **Relocation**: Area is "linked" but base not moved
- **Restriction**: Cannot contain relocatable references to non-absolute areas

---

## 5. OBJ_AREA: Areas Chunk Format

The areas chunk contains actual code/data and associated relocation directives.

### 5.1 Overall Layout

```
+---------------------------+
| Area 1 Data               |
+---------------------------+
| Area 1 Relocations        |
+---------------------------+
| Area 2 Data               |
+---------------------------+
| Area 2 Relocations        |
+---------------------------+
...
| Area N Data               |
+---------------------------+
| Area N Relocations        |
+---------------------------+
```

### 5.2 Area Data

- **Size**: As specified in area declaration's area_size field
- **Alignment**: 4-byte boundary
- **Endianness**: Words and halfwords must match chunk file endianness
- **Omitted When**: Area has "not initialised" attribute (bit 12)
- **Content**: Raw bytes of code or data

### 5.3 Relocation Directives

Each relocation directive is **2 words (8 bytes)**:

```
+----------------------------------+
| Offset                           | Word 1
+----------------------------------+
| Flags and SID                    | Word 2
+----------------------------------+
```

#### Word 1: Offset

- **Type**: Word (4 bytes)
- **Value**: Byte offset from area base to field to be relocated
- **Alignment**: Must be divisible by 4 (word-aligned)

#### Word 2: Flags and SID

```
Bit Layout:
31 30 29 28 27 26 25 24 23 22 21 20 ... 16 15 14 13 12 ... 8  7  6  5  4  3  2  1  0
[  II ][B][A][ R][  FT  ][              SID (24 bits)                              ]
```

| Bits | Field | Description |
|------|-------|-------------|
| 0-23 | **SID** | Symbol/Area ID (interpretation depends on A bit) |
| 24-25 | **FT** | Field Type |
| 26 | **R** | PC-Relative flag |
| 27 | **A** | Area/Symbol flag |
| 28 | **B** | Based relocation flag |
| 29-30 | **II** | Instruction count constraint |
| 31 | **(unused in v1.50)** | Must be 0 |

#### SID Field (Bits 0-23)

**When A=0** (Area-based relocation):
- SID is area index (0-origin)
- Relocate by base of specified area

**When A=1** (Symbol-based relocation):
- SID is symbol table index (0-origin)
- Relocate by value of specified symbol
- If symbol is in common area, treated as area-based

#### FT Field (Bits 24-25): Field Type

| Value | Field Type | Size | Description |
|-------|------------|------|-------------|
| 00 | Byte | 1 byte | 8-bit value |
| 01 | Halfword | 2 bytes | 16-bit value |
| 10 | Word | 4 bytes | 32-bit value |
| 11 | Instruction | Variable | ARM or Thumb instruction(s) |

**For Instruction**:
- Bit 0 of offset field distinguishes:
  - Offset bit 0 = 0: ARM instruction(s)
  - Offset bit 0 = 1: Thumb instruction(s)

#### R Bit (Bit 26): PC-Relative

**R=0**: Plain additive relocation
```
subject_field = subject_field + relocation_value
```

**R=1**: PC-relative relocation
```
subject_field = subject_field + (relocation_value - base_of_area_containing(subject_field))
```

**Special Case**: If A=0 and relocation value is base of area containing subject field, relocation value is not added.

**Typical Use**: R=1 for PC-relative branches, R=0 for data pointers

#### B Bit (Bit 28): Based Area

**B=0**: Normal relocation

**B=1**: Based area relocation
```
subject_field = subject_field + (relocation_value - base_of_area_group_containing(relocation_value))
```

**Restrictions when B=1**:
- II bits (29-30) must be 0
- Bit 31 must be 1

**Use**: For based addressing in reentrant code

#### II Bits (Bits 29-30): Instruction Count Constraint

For instruction sequences (FT=11), constrains how many instructions may be modified:

| Value | Constraint |
|-------|-----------|
| 00 | No constraint (any number of instructions) |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

**Purpose**: Limit code expansion for relocations requiring instruction sequences

### 5.4 Common Relocation Cases

#### Case 1: Simple Data Pointer

```
W + AN
```
- W = word contents at offset
- AN = base of area N
- Flags: R=0, A=0, FT=10, SID=N

#### Case 2: Data Pointer with Offset

```
W + AN + a
```
- a = addend (stored in subject word W)
- Flags: R=0, A=0, FT=10, SID=N

#### Case 3: PC-Relative Branch

```
((W + AN) mod 2^24) | (W & 0xFF000000)
```
- 24-bit field in branch instruction
- Top 8 bits (condition/opcode) preserved
- Flags: R=1, A=0, FT=11, SID=N

#### Case 4: Symbol-Relative Data

```
W + symbol_value
```
- Flags: R=0, A=1, FT=10, SID=symbol_index

---

## 6. OBJ_SYMT: Symbol Table Chunk Format

The symbol table contains **num_symbols** entries, each **4 words (16 bytes)**.

### 6.1 Symbol Table Entry Structure

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | **name** | Word | Offset in OBJ_STRT of symbol name (0 = no name) |
| +0x04 | **flags** | Word | Attribute bit flags |
| +0x08 | **value** | Word | Symbol value or common size |
| +0x0C | **area_name** | Word | Offset in OBJ_STRT of defining area name |

### 6.2 Symbol Flags Field

```
Bit Layout:
31 30 29 28 27 26 ... 24 23 22 ... 16 15 14 ... 12 11 10  9  8  7  6  5  4  3  2  1  0
[  Rsvd  ][SymType][ Rsvd  ][Datum][  Rsvd  ][Code Attr][ Rsvd ][Cm][Wk][CI][CS][Wk][Ex][Df]
```

#### Core Symbol Attributes (Bits 0-7)

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | 0x00000001 | **Defined** | Symbol defined in this file |
| 1 | 0x00000002 | **Exported** | Symbol has global scope |
| 2 | 0x00000004 | **Absolute** | Symbol value is absolute (not area-relative) |
| 3 | 0x00000008 | **Case-Insensitive** | Match symbol case-insensitively |
| 4 | 0x00000010 | **Weak** | Weak definition (can remain unsatisfied) |
| 5 | 0x00000020 | **Strong** | Strong definition (opposite of weak) |
| 6 | 0x00000040 | **Common** | Symbol is common (not area-relative) |
| 7 | 0x00000080 | **Reserved** | Must be 0 |

#### Symbol Definition States (Bits 0-1)

| Bits 1:0 | Binary | State | Description |
|----------|--------|-------|-------------|
| 00 | 00 | Reserved | Invalid state |
| 01 | 01 | Local Definition | Defined, local scope |
| 10 | 10 | External Reference | Referenced, defined elsewhere |
| 11 | 11 | Global Definition | Defined, global scope |

#### Additional Symbol Attributes

**Bit 2 - Absolute**:
- Value field contains absolute value (constant)
- Not relative to any area base
- Can be defined in absolute area
- Incompatible with area-relative addressing

**Bit 3 - Case Insensitive**:
- Linker ignores case when matching
- Only for external references
- Only case-insensitive languages can use

**Bit 4 - Weak**:
- Reference can remain unsatisfied
- Linker ignores when deciding library member loading
- Cannot be both weak and case-insensitive (bits 3 and 4)

**Bit 5 - Strong**:
- Opposite of weak (redundant information)
- Should not both be set with bit 4

**Bit 6 - Common**:
- Symbol refers to common area
- Value field contains common block size
- All definitions must have same size
- Must have bit 4 set (common symbols are constants)

#### Code Area Symbol Attributes (Bits 8-12)

Valid only when symbol defined in code area (area has bit 9 set):

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 8 | 0x00000100 | **Code Datum** | Symbol identifies data (not executable code) in code area |
| 9 | 0x00000200 | **FP Args in FP Regs** | Function uses FP args in FP registers |
| 10-11 | 0x00000C00 | **Reserved** | Must be 0 |
| 12 | 0x00001000 | **Thumb** | Symbol is Thumb code symbol |

#### Symbol Type and Datum Fields (Bits 16-27)

**Bits 16-23**: Datum field (interpretation depends on symbol type)

**Bits 24-27**: Symbol Type

| Type | Description | Datum Interpretation |
|------|-------------|---------------------|
| 0 | Not used | - |
| 1-8 | Reserved | - |
| 9 | Floating Point | Bits 16-17: FP size (0=0, 1=1 word, 2=2 words, 3=3 words)<br>Bits 18-23: Register number (255=no register) |
| 10 | Scratch Register | Bits 16-23: Register number (255=no register)<br>Need not be preserved across calls |
| 11 | Non-Scratch Register | Bits 16-23: Register number<br>Must be preserved across calls |
| 12 | Coprocessor Register | Bits 16-23: R = 256*CN + REGNO |
| 13 | Coprocessor Register (Preserved) | As type 12, must be preserved |

### 6.3 Symbol Value Field

**When Defined (bit 0 set)**:
- **Absolute** (bit 2 set): Contains absolute value
- **Common** (bit 6 set): Contains size of common block
- **Area-Relative**: Offset from base of defining area

**When Undefined** (bit 0 clear):
- Must be 0

### 6.4 Area Name Field

**When Non-Absolute Definition** (bit 0 set, bit 2 clear):
- Offset into OBJ_STRT of defining area's name
- Used by linker to find area base

**Otherwise**:
- Must be 0

---

## 7. OBJ_STRT: String Table Chunk Format

### 7.1 Structure

The string table is a simple concatenation of NUL-terminated strings:

```
+----------------------------------+
| Length (4 bytes)                 | Total size including this word
+----------------------------------+
| String 1                         | "text\0"
+----------------------------------+
| String 2                         | "data\0"
+----------------------------------+
| String 3                         | "main\0"
+----------------------------------+
...
+----------------------------------+
```

### 7.2 Format Details

**First 4 Bytes**:
- Word containing string table length
- Includes the length word itself
- Minimum value: 4 (empty table)
- Endianness matches AOF file

**Strings**:
- Non-control characters (codes 32-126, 160-255)
- Terminated by NUL (0x00) byte
- No alignment restrictions
- Identified by offset from table start

**Valid Offsets**:
- No offset < 4 (reserved for length word)
- Offset 4 refers to first byte after length

**Example**:
```
Offset  Content
0-3     0x00000020  (32 bytes total)
4       'a'
5       'r'
6       'e'
7       'a'
8       '1'
9       0x00 (NUL)
10      'm'
11      'a'
12      'i'
13      'n'
14      0x00 (NUL)
...
```

---

## 8. OBJ_IDFN: Identification Chunk Format

### 8.1 Purpose

Identifies the tool that created the object file.

### 8.2 Format

Simple variable-length string:

```
+----------------------------------+
| Printable string                 |
+----------------------------------+
```

**Characters**:
- Codes 10-13 (newlines)
- Codes 32-126 (printable ASCII)
- Terminated by NUL (0x00)
- Codes 128-255 discouraged (host-dependent interpretation)

**Example**:
```
"ARM C Compiler vsn 5.06 [June 1995]\0"
"ARM Assembler vsn 2.00\0"
```

---

## 9. Areas and Memory Model

### 9.1 What is an Area?

An **area** is a named, attributed region of code or data:

- **Named**: Each area has a unique name within the object file
- **Attributed**: Properties like read-only, code, position-independent
- **Relocatable**: Can be relocated as a unit by the linker
- **Self-Contained**: Relocation directives describe internal and external references

### 9.2 Typical Area Layout

A compiled AOF file typically contains:

1. **Read-Only Code Area**
   - Contains executable instructions
   - Attributes: Code (bit 9), Read-Only (bit 13)
   - Name example: "C$$code"

2. **Read-Write Data Area**
   - Initialized data
   - Attributes: Data (not code), writable
   - Name example: "C$$data"

3. **Zero-Initialised Data Area**
   - BSS segment
   - Attributes: Zero-Init (bit 7), Data
   - Name example: "C$$zidata"

4. **Based Area** (for reentrant code)
   - Address constants
   - Attributes: Read-Only, separate base
   - Name example: "C$$constdata"

### 9.3 Common Areas

**Purpose**: Implement Fortran COMMON or C tentative definitions

**Definition Area** (bit 10 set):
- Declares common block
- Can have initialization data
- Can have symbols defined relative to it
- Linker creates real area from all definitions

**Reference Area** (bit 11 set):
- References common block
- No initialization data
- Size is minimum acceptable

**Combination**:
- Same-named common areas overlay each other
- Linker uses largest size
- All must agree on attributes

### 9.4 Absolute Areas

**Purpose**: Place code/data at specific addresses (e.g., interrupt vectors)

**Properties**:
- Base address specified in area declaration
- Not relocated by linker
- Can only reference other absolute areas or symbols
- Cannot contain relocatable references

**Use Cases**:
- ROM-based code at fixed addresses
- Memory-mapped I/O
- Hardware interrupt vectors

---

## 10. Relocation Processing

### 10.1 Relocation Concepts

**Relocation** adjusts address references when:
- Areas are assigned non-zero base addresses
- Symbolic references are resolved
- Code is positioned at runtime address

**Relocation Directive** specifies:
- **Where**: Offset of field to modify
- **What**: Field type (byte, halfword, word, instruction)
- **How**: Additive, PC-relative, based
- **By What**: Area base or symbol value

### 10.2 Relocation Calculation

General form:
```
new_value = old_value + f(relocation_value, constraints)
```

Where:
- **old_value**: Original field contents
- **relocation_value**: Area base or symbol value
- **constraints**: R bit, B bit, field type

### 10.3 Example Relocations

#### Example 1: Absolute Data Pointer

Source:
```c
extern int global_var;
int *ptr = &global_var;
```

AOF:
```
Area: data
Offset 0: 00 00 00 00    ; ptr initialized to 0

Relocation:
  Offset: 0
  Flags: A=1, R=0, FT=10 (word)
  SID: symbol_index(global_var)
```

Linker action:
```
word[0] = 0 + value(global_var)
```

#### Example 2: PC-Relative Branch

Source:
```arm
        BL      function
```

AOF:
```
Area: code
Offset 100: EB 00 00 00   ; BL with offset 0

Relocation:
  Offset: 100
  Flags: A=1, R=1, FT=11 (instruction)
  SID: symbol_index(function)
```

Linker action:
```
instruction = (0xEB000000 + ((value(function) - (base + 100)) >> 2)) & 0x00FFFFFF
instruction |= 0xEB000000
```

#### Example 3: Area-Relative Data

Source:
```arm
        LDR     r0, =area_base
```

AOF:
```
Area: code
Offset 200: 00 00 00 00   ; Literal pool entry

Relocation:
  Offset: 200
  Flags: A=0, R=0, FT=10 (word)
  SID: area_index
```

Linker action:
```
word[200] = 0 + base(area_index)
```

---

## 11. Linker Symbol Table Format (Optional Output)

The linker can write its internal symbol table to a file for debugging/browsing.

### 11.1 Format

String table format (NUL-terminated lines):

```
name,value,class[,definingmodulename]
```

### 11.2 Field Descriptions

**name**: Symbol name (at least one printable character)

**value**: Hex number prefixed by '&' (BBC Basic style)
- Example: `&00008000`

**class**: One of:
- **Abs**: Absolute value
- **RelocDef**: Defined, not referenced
- **RelocRef**: Referenced, not defined
- **ExtDef**: Defined and referenced
- **ExtRef**: Referenced, not defined anywhere

**definingmodulename**: (Optional)
- Module defining/referencing symbol
- From linker `-md` option

### 11.3 Example

```
main,&00008000,ExtDef,program.o
_printf,&00009000,ExtDef,printf.o
buffer,&0000A000,RelocDef,data.o
undefined_sym,&00000000,ExtRef
PI,&40490FDB,Abs
```

---

## 12. AOF File Creation and Processing

### 12.1 Typical Creation Process

1. **Compiler/Assembler**:
   - Parses source code
   - Generates code and data areas
   - Creates symbol table entries
   - Produces relocation directives
   - Writes AOF file

2. **Chunk File Creation**:
   - Allocate header (convention: 8 chunks)
   - Write each chunk to file
   - Update chunk file header

3. **Area Generation**:
   - Emit code/data to memory buffer
   - Track relocatable references
   - Create relocation directive for each reference
   - Write area data and relocations to OBJ_AREA

### 12.2 Linker Processing

1. **Read all input AOF files**
2. **Resolve symbols**:
   - Match external references to definitions
   - Handle weak symbols
   - Process common areas
3. **Assign area bases**:
   - Group areas by attributes
   - Sort by name within groups
   - Assign contiguous addresses
4. **Apply relocations**:
   - Process each relocation directive
   - Update target fields
5. **Generate output**:
   - AIF format (executable)
   - AOF format (partial link)
   - Other formats (binary, ELF, etc.)

### 12.3 Library Processing

Libraries (ALF format) contain multiple AOF members:

1. **Initial scan**: Build external symbol table
2. **Member loading**: Load members defining unresolved symbols
3. **Recursive loading**: Load additional members as new references appear
4. **Weak symbols**: Ignored when deciding member loading

---

## 13. Compatibility and Portability

### 13.1 Endianness Handling

**Detecting Endianness**:
```c
uint32_t chunk_id;
fread(&chunk_id, 4, 1, file);
if (chunk_id == 0xC3CBC6C5) {
    // Native endianness
} else if (chunk_id == 0xC5C6CBC3) {
    // Opposite endianness - byte swap all words
}
```

**Processing Different Endianness**:
- Read entire file
- Byte-reverse all word values
- String data unchanged (byte order independent)
- ChunkIds unchanged (defined as byte sequence)

### 13.2 Version Compatibility

**Checking Version**:
```c
if (header->version_id != 310) {
    // Potentially incompatible version
}
```

**Forward Compatibility**:
- Newer features use reserved bits
- Old tools should zero reserved fields
- Unknown chunks can be preserved

### 13.3 Extension Mechanisms

1. **Custom Chunks**: Add new chunk types with unique names
2. **Reserved Bits**: Future attributes use reserved bit fields
3. **Extended Attributes**: Higher bits in attribute words
4. **Version Field**: Increment for incompatible changes

---

## 14. Example AOF File Structure

### 14.1 Simple AOF File

A minimal AOF with one code area and one symbol:

```
Chunk File Header:
  ChunkFileId: 0xC3CBC6C5
  max_chunks: 8
  num_chunks: 5

  Chunk 0: OBJ_HEAD at offset 48, size 44
  Chunk 1: OBJ_AREA at offset 92, size 20
  Chunk 2: OBJ_IDFN at offset 112, size 20
  Chunk 3: OBJ_SYMT at offset 132, size 16
  Chunk 4: OBJ_STRT at offset 148, size 20

OBJ_HEAD (44 bytes):
  file_type: 0xC5E2D080
  version_id: 310
  num_areas: 1
  num_symbols: 1
  entry_area: 1
  entry_offset: 0

  Area 0:
    area_name: 4 (offset into STRT)
    area_attributes: 0x00000202 (code, relocatable, word-aligned)
    area_size: 12
    num_relocs: 0
    reserved: 0

OBJ_AREA (20 bytes):
  Area 0 data (12 bytes):
    E3A00000    ; MOV r0, #0
    E3A01001    ; MOV r1, #1
    E12FFF1E    ; BX lr
  Area 0 relocations (8 bytes):
    (none - aligned to word boundary with zeros)

OBJ_IDFN (20 bytes):
  "ARM Assembler\0" + padding

OBJ_SYMT (16 bytes):
  Symbol 0:
    name: 9 (offset into STRT)
    flags: 0x00000003 (defined, exported)
    value: 0
    area_name: 4

OBJ_STRT (20 bytes):
  length: 20
  "code\0main\0" + padding
```

### 14.2 Hexdump of Example

```
00000000: C5 C6 CB C3 08 00 00 00 05 00 00 00 4F 42 4A 5F  ............OBJ_
00000010: 48 45 41 44 30 00 00 00 2C 00 00 00 4F 42 4A 5F  HEAD0...,...OBJ_
00000020: 41 52 45 41 5C 00 00 00 14 00 00 00 4F 42 4A 5F  AREA\.......OBJ_
00000030: 49 44 46 4E 70 00 00 00 14 00 00 00 4F 42 4A 5F  IDFNp.......OBJ_
00000040: 53 59 4D 54 84 00 00 00 10 00 00 00 4F 42 4A 5F  SYMT........OBJ_
00000050: 53 54 52 54 94 00 00 00 14 00 00 00 80 D0 E2 C5  STRT............
00000060: 36 01 00 00 01 00 00 00 01 00 00 00 01 00 00 00  6...............
00000070: 00 00 00 00 04 00 00 00 02 02 00 00 0C 00 00 00  ................
00000080: 00 00 00 00 00 00 00 00 00 00 A0 E3 01 10 A0 E3  ................
00000090: 1E FF 2F E1 00 00 00 00 41 52 4D 20 41 73 73 65  ../.....ARM Asse
000000A0: 6D 62 6C 65 72 00 00 00 04 00 00 00 09 00 00 00  mbler...........
000000B0: 03 00 00 00 00 00 00 00 04 00 00 00 14 00 00 00  ................
000000C0: 63 6F 64 65 00 6D 61 69 6E 00 00 00              code.main...
```

---

## 15. Common Use Cases

### 15.1 Separate Compilation

**Source Files**:
```c
// module1.c
extern int shared_var;
void func1(void) { shared_var = 1; }

// module2.c
int shared_var = 0;
void func2(void) { shared_var = 2; }
```

**AOF Generation**:
1. Compile module1.c → module1.o
   - Symbol: func1 (defined, exported)
   - Symbol: shared_var (undefined, external reference)
   - Relocation: shared_var reference in func1 code

2. Compile module2.c → module2.o
   - Symbol: func2 (defined, exported)
   - Symbol: shared_var (defined, exported)
   - Area: data containing shared_var

**Linking**:
1. Read both AOF files
2. Resolve shared_var reference to definition
3. Apply relocation in func1 code
4. Generate combined AIF

### 15.2 Position-Independent Code

**Source**:
```arm
        ; Position-independent function
        MOV     ip, pc
        LDR     r0, [ip, #offset_to_data]
        BX      lr
```

**AOF**:
- Area attributes: bit 14 set (position-independent)
- Relocation: PC-relative for data access
- No absolute address references

**Benefits**:
- Code can execute at any address
- No relocation required at load time
- Suitable for shared libraries, ROMs

### 15.3 Common Block (Fortran COMMON)

**Source**:
```fortran
      COMMON /VARS/ X, Y, Z
      X = 1.0
```

**AOF**:
- Area: VARS (common definition, size 12)
- Symbol: X (offset 0 in VARS)
- Symbol: Y (offset 4 in VARS)
- Symbol: Z (offset 8 in VARS)

**Linking Multiple Modules**:
- All reference same VARS area
- Overlay at single address
- Largest size used

---

## 16. Debugging and Analysis

### 16.1 Examining AOF Files

**Using armsd (ARM Symbolic Debugger)**:
```
armsd> load file.o
armsd> info areas
armsd> info symbols
```

**Using DecAOF (if available)**:
```
$ decaof file.o
```

**Manual Examination**:
1. Check chunk file header magic: 0xC3CBC6C5
2. Identify chunks by chunkId
3. Parse OBJ_HEAD for area count
4. Examine each area's attributes
5. Check symbol table entries
6. Validate relocation directives

### 16.2 Common Issues

**Undefined Symbols**:
- Symbol referenced but not defined in any input
- Check symbol table for external references
- Verify all needed object files/libraries included

**Relocation Overflow**:
- Relocation value too large for field type
- Common with byte/halfword relocations
- Check branch distances in code

**Endianness Mismatch**:
- Mixing little-endian and big-endian AOF
- Chunk file ID appears byte-reversed
- All word values incorrect

**Version Incompatibility**:
- version_id not 310
- Unknown attribute bits set
- Extended features not supported

---

## 17. Format Constants and Magic Numbers

### 17.1 Key Constants

| Name | Value | Description |
|------|-------|-------------|
| CHUNK_FILE_ID | 0xC3CBC6C5 | Chunk file magic number |
| CHUNK_FILE_ID_REV | 0xC5C6CBC3 | Reversed (wrong endianness) |
| AOF_MAGIC | 0xC5E2D080 | AOF file type marker |
| AOF_VERSION | 310 (0x136) | Current AOF version |
| OBJ_HEAD | "OBJ_HEAD" | Header chunk ID |
| OBJ_AREA | "OBJ_AREA" | Areas chunk ID |
| OBJ_SYMT | "OBJ_SYMT" | Symbol table chunk ID |
| OBJ_STRT | "OBJ_STRT" | String table chunk ID |
| OBJ_IDFN | "OBJ_IDFN" | Identification chunk ID |

### 17.2 Attribute Bit Masks

```c
// Area Type Bits
#define AOF_AREA_ABS    0x00000001
#define AOF_AREA_CODE   0x00000004
#define AOF_AREA_COMDEF 0x00000008
#define AOF_AREA_COMREF 0x00000010
#define AOF_AREA_DEBUG  0x00000040
#define AOF_AREA_ZEROINIT 0x00000080

// Area Attribute Bits
#define AOF_AREA_AT_ABS   0x00000100
#define AOF_AREA_AT_CODE  0x00000200
#define AOF_AREA_AT_COMDEF 0x00000400
#define AOF_AREA_AT_COMREF 0x00000800
#define AOF_AREA_AT_NOINIT 0x00001000
#define AOF_AREA_AT_READONLY 0x00002000
#define AOF_AREA_AT_PIC   0x00004000
#define AOF_AREA_AT_DEBUG 0x00008000

// Code Attributes
#define AOF_AREA_32BIT  0x00010000
#define AOF_AREA_REENT  0x00020000
#define AOF_AREA_FPINST 0x00040000
#define AOF_AREA_NOSWST 0x00080000
#define AOF_AREA_THUMB  0x00100000
#define AOF_AREA_HALFWORD 0x00200000
#define AOF_AREA_INTERWORK 0x00400000

// Symbol Flags
#define AOF_SYM_DEFINED 0x00000001
#define AOF_SYM_EXPORT  0x00000002
#define AOF_SYM_ABS     0x00000004
#define AOF_SYM_CASEINS 0x00000008
#define AOF_SYM_WEAK    0x00000010
#define AOF_SYM_COMMON  0x00000040
#define AOF_SYM_DATUM   0x00000100
#define AOF_SYM_FPARGS  0x00000200
#define AOF_SYM_THUMB   0x00001000

// Relocation Flags
#define AOF_RELOC_R     0x04000000  // Bit 26: PC-relative
#define AOF_RELOC_A     0x08000000  // Bit 27: Symbol (vs area)
#define AOF_RELOC_B     0x10000000  // Bit 28: Based
#define AOF_RELOC_FT_MASK 0x03000000 // Bits 24-25: Field type
#define AOF_RELOC_II_MASK 0x60000000 // Bits 29-30: Instr constraint
#define AOF_RELOC_SID_MASK 0x00FFFFFF // Bits 0-23: SID
```

---

## 18. Conclusion

ARM Object Format (AOF) is a well-designed, simple object file format that effectively supports:

- **Modular Compilation**: Separate compilation and linking
- **Flexible Relocation**: Comprehensive relocation mechanisms
- **Symbol Management**: Global and local symbols with rich attributes
- **Code/Data Separation**: Multiple attributed areas per file
- **Extensibility**: Chunk-based structure allows additions
- **Tool Integration**: Standard format for ARM development tools

While simpler than modern formats like ELF, AOF provides all essential features for ARM software development and remains important for:
- Legacy ARM systems (RISC OS, 3DO)
- Embedded development
- Understanding ARM toolchain evolution
- Retro computing and preservation

---

## Appendix A: AOF vs. ELF Comparison

| Feature | AOF | ELF |
|---------|-----|-----|
| **Complexity** | Simple | Complex |
| **Sections** | Areas | Sections |
| **Relocation** | Simple directives | REL/RELA |
| **Symbol Table** | Single flat table | Multiple tables |
| **Dynamic Linking** | Not supported | Full support |
| **Debug Info** | Custom chunks | DWARF |
| **Extensibility** | Custom chunks | Sections, segments |
| **Toolchain** | ARM-specific | Cross-platform |

---

## Appendix B: References

### Primary Documentation

1. **ARM DUI 0041C** - ARM Software Development Toolkit
   - Chapter 15: ARM Object Format
   - ARM Limited, 1997-1998

2. **RISC OS PRM** - Programmer's Reference Manual
   - Appendix D: Code File Formats
   - 3QD Developments Ltd, 2015

3. **ARM Technical Specifications**
   - 3DO Edition
   - Acorn Computers / ARM

### Tools

- **ARM Compiler** (armcc): Generates AOF
- **ARM Assembler** (armasm/objasm): Generates AOF
- **ARM Linker** (armlink): Processes AOF → AIF
- **ARM Librarian** (armlib): Creates ALF from AOF

---

**Document Version**: 1.0
**Date**: 2026-02-07
**Status**: Complete Specification
