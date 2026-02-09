# ARM Object Format (AOF) — Complete File Format Specification

## Document Information

| Field | Value |
|-------|-------|
| **Format Name** | ARM Object Format (AOF) |
| **Primary Use** | Intermediate object files produced by language processors (compilers, assemblers) |
| **Key Features** | Chunk-based architecture, area definitions, relocation directives, symbol tables |
| **Base Format** | Chunk File Format (layered format) |
| **Endianness** | Little-endian (target ARM byte order) |
| **Word Size** | 32 bits (4 bytes) |

---

## 1. Overview

ARM Object Format (AOF) is the standard intermediate file format produced by ARM language processors such as `armcc` (C compiler), `armasm` (assembler), and `tcc` (Thumb compiler). AOF files contain relocatable code and data that the linker (`armlink`) combines with other object files and library members to produce executable images.

AOF is built on top of **Chunk File Format**, a generic container format that provides:
- Efficient random access to file components
- Easy extensibility (new chunk types can be added)
- Self-describing structure
- Support for partial file updates

### 1.1 Design Goals

1. **Extensibility**: New chunk types can be added without breaking existing tools
2. **Efficiency**: Direct access to components without parsing entire file
3. **Modularity**: Each chunk contains one logical type of data
4. **Portability**: Format independent of host system byte order
5. **Symbolic Information**: Rich symbol table support for debugging

### 1.2 Relationship to Other Formats

```
Source Code (C, Assembly)
        ↓
   Language Processor (armcc, armasm)
        ↓
   AOF Object File (.o)
        ↓
   Linker (armlink)
        ↓
   Executable Image (AIF, ELF, etc.)
```

---

## 2. Chunk File Format Foundation

AOF files use Chunk File Format as their underlying container. Understanding this format is essential for working with AOF.

### 2.1 Chunk File Structure

```
+------------------+
| Chunk File      |
|     Header      |  Fixed size (3 words + 4 words per chunk)
+------------------+
| Chunk 1         |  Variable size
+------------------+
| Chunk 2         |
+------------------+
| ...              |
+------------------+
| Chunk N          |
+------------------+
```

### 2.2 Chunk File Header

The chunk file header consists of two parts:

**Fixed Part (3 words):**

| Field | Size | Description |
|-------|------|-------------|
| `ChunkFileId` | 4 bytes | Magic number identifying chunk files |
| `max_chunks` | 4 bytes | Maximum number of chunks (fixed at creation) |
| `num_chunks` | 4 bytes | Currently used chunks (0 to max_chunks) |

**Chunk Entry (4 words per chunk):**

| Field | Size | Description |
|-------|------|-------------|
| `chunkId` | 8 bytes | Chunk identifier (8 characters) |
| `file_offset` | 4 bytes | Byte offset from file start (divisible by 4) |
| `size` | 4 bytes | Exact byte size of chunk contents |

### 2.3 ChunkFileId Magic Number

```
C3 CB C6 C5  (hex)  = "chunk" in little-endian
C5 C6 CB C3  (hex)  = Byte-swapped (for big-endian detection)
```

**Endianness Detection:**
- If reading `ChunkFileId` as little-endian gives `0xC3CBC6C5`: File is little-endian
- If reading `ChunkFileId` as little-endian gives `0xC5C6CBC3`: File is big-endian, byte-swap all word values

### 2.4 Chunk Identification

The `chunkId` field is 8 bytes split into two 4-byte parts:

```
[AAAA BBBB]  (8 characters total)

AAAA = Universal name (allocated by Acorn/ARM)
BBBB = Component identifier (specific to chunk type)
```

**Character Storage:**
- Characters stored in ascending address order
- Independent of endianness
- First character stored first

**AOF Chunk Names:**

| Chunk | Name | Purpose |
|-------|------|---------|
| Header | `OBJ_HEAD` | Fixed header + area declarations |
| Areas | `OBJ_AREA` | Code/data contents + relocations |
| Identification | `OBJ_IDFN` | Tool that created this file |
| Symbol Table | `OBJ_SYMT` | Symbol definitions |
| String Table | `OBJ_STRT` | All symbol/area names |

---

## 3. AOF Chunk Specifications

### 3.1 Required and Optional Chunks

| Chunk | Required | Description |
|-------|----------|-------------|
| `OBJ_HEAD` | Yes | Fixed header + area declarations |
| `OBJ_AREA` | Yes | Code, data, and relocations |
| `OBJ_IDFN` | No | Identification string |
| `OBJ_SYMT` | No | Symbol table |
| `OBJ_STRT` | No | String table (usually required if symbols exist) |

**Note:** The header and areas chunks are mandatory; other chunks are optional but typically present.

---

## 4. OBJ_HEAD — AOF Header Chunk

The header chunk contains metadata about the object file and definitions of all areas within the file.

### 4.1 Header Structure

```
+---------------------------+
| Fixed Part (6 words)     |  Common to all AOF files
+---------------------------+
| Area Header 1 (5 words)  |  One per area in file
+---------------------------+
| Area Header 2 (5 words)  |
+---------------------------+
| ...                      |
+---------------------------+
| Area Header N (5 words)  |
+---------------------------+
```

### 4.2 Fixed Part (First 6 Words)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `file_type` | Object file type identifier |
| 0x04 | 4 | `version_id` | AOF version number |
| 0x08 | 4 | `num_areas` | Number of code/data areas |
| 0x0C | 4 | `num_symbols` | Number of symbols in symbol table |
| 0x10 | 4 | `entry_area` | Area containing entry point (1-based index) |
| 0x14 | 4 | `entry_offset` | Byte offset within entry area |

#### 4.2.1 File Type

```
C5 E2 D0 80  (hex)  = AOF identifier (little-endian)
```

This magic number identifies the file as an AOF object file. Must match target byte order.

#### 4.2.2 Version ID

| Value | Meaning |
|-------|---------|
| 310 (0x136) | Current AOF version (SDT 2.50) |

The version number allows tools to handle legacy formats.

#### 4.2.3 Entry Point

| Field | Description |
|-------|-------------|
| `entry_area` | 1-based index into area headers; 0 = no entry defined |
| `entry_offset` | Byte offset from base of entry area to entry point |

---

## 5. Area Declarations

Following the fixed header are area declarations, one for each area in the object file.

### 5.1 Area Header Format (5 words)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `area_name` | Offset into string table of area name |
| 0x04 | 4 | `attributes` | Area attributes and alignment |
| 0x08 | 4 | `area_size` | Size in bytes (multiple of 4) |
| 0x0C | 4 | `num_relocs` | Number of relocation directives |
| 0x10 | 4 | `base_address` | Absolute address (if absolute area) |

### 5.2 Area Name

A byte offset into the `OBJ_STRT` string table. An offset of zero means no symbolic name exists.

### 5.3 Area Attributes

The attributes word encodes multiple properties:

```
+------------------------------------------+
|    Unused   | Alignment | Attribute Bits |
|  (bits 31-24)| (bits 8-15)| (bits 0-7)   |
+------------------------------------------+
```

#### 5.3.1 Attribute Bits (bits 0-7)

At most one type bit may be set:

| Bit | Value | Name | Description |
|-----|-------|------|-------------|
| 0 | 0x01 | ABS | Absolute area (fixed address) |
| 1 | 0x02 | REL | Relocatable area |
| 2 | 0x04 | CODE | Code area |
| 3 | 0x08 | COMMON | Common area (uninitialized) |
| 4 | 0x10 | COMBLK | Common block (block storage) |
| 5 | 0x20 | — | Reserved (linker use) |
| 6 | 0x40 | DEBUG | Debugging tables |
| 7 | 0x80 | ZINIT | Zero-initialized data |

#### 5.3.2 Area Type Bits (bits 8-15)

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 8 | 0x0100 | NO_CODE | Area must not contain code |
| 9 | 0x0200 | PI_CODE | Position independent code |
| 10 | 0x0400 | REENT | Reentrant / common block |
| 11 | 0x0800 | READONLY | Read-only data |
| 12 | 0x1000 | DEBUG_TBL | Debugging table |
| 13-14 | — | ALIGN | Alignment (log2) |
| 15 | 0x8000 | BASE_VALID | Base address is valid |

**Alignment (bits 13-14):**

| Value | Meaning |
|-------|---------|
| 0 | Byte aligned (2^0 = 1) |
| 1 | Halfword aligned (2^1 = 2) |
| 2 | Word aligned (2^2 = 4) |

#### 5.3.3 Extended Attributes (bits 16-22)

For code areas (bit 2 set in type bits):

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 16 | 0x010000 | APCS_32 | 32-bit APCS compliant |
| 17 | 0x020000 | REENT | Reentrant code |
| 18 | 0x040000 | FPU | Uses extended FP instructions |
| 19 | 0x080000 | NOSTCHK | No software stack checking |
| 20 | 0x100000 | THUMB | Thumb code area |
| 21 | 0x200000 | HALFWORD | May contain ARM halfword instructions |
| 22 | 0x400000 | INTERWORK | ARM/Thumb interworking |

#### 5.3.4 Stub Bits (bits 24-31)

Used and updated by the linker. Initial value undefined except:
- For common block areas (bit 4 set): bits 24-27 define max alignment

### 5.4 Area Size

Size in bytes of the area:
- For normal areas: Actual data size in `OBJ_AREA` chunk
- For common areas (bits 3/4 set): Minimum acceptable size
- For zero-init areas (bit 7 set): Size must be multiple of 4
- For uninitialized areas (bit 12 set): No data in `OBJ_AREA`

### 5.5 Number of Relocations

Count of relocation directives in `OBJ_AREA` chunk following this area's data.

### 5.6 Absolute Area Base Address

Only present for absolute areas (bit 0 set in type bits):

| Field | Description |
|-------|-------------|
| `base_address` | Fixed address where area must be placed |

Absolute areas cannot contain relocatable references to non-absolute areas.

---

## 6. OBJ_AREA — Areas Chunk

The areas chunk contains the actual contents of all areas and their associated relocation directives.

### 6.1 Structure

```
+---------------------------+
| Area 1 Data              |  area_size[0] bytes
+---------------------------+
| Area 1 Relocations       |  num_relocs[0] × 8 bytes
+---------------------------+
| Area 2 Data              |
+---------------------------+
| Area 2 Relocations       |
+---------------------------+
| ...                      |
+---------------------------+
| Area N Data              |
+---------------------------+
| Area N Relocations       |
+---------------------------+
```

Both area data and relocation tables are aligned to 4-byte boundaries.

### 6.2 Area Data

Raw contents of the area:
- **Code areas**: ARM/Thumb instructions
- **Data areas**: Initialized data values
- **Debug areas**: Debugging tables
- **Uninitialized areas**: No data present (BIT 12 set)

All words and halfwords use target byte order (little-endian for ARM).

---

## 7. Relocation Directives

Relocation directives describe values that cannot be fixed until link time or load time.

### 7.1 Relocation Directive Format

Each relocation directive is 2 words (8 bytes):

```
+------------------------------------------+
| Offset (word)                            |
+------------------------------------------+
| Flags and SID (word)                     |
+------------------------------------------+
```

### 7.2 Offset Field

Byte offset from the start of the **preceding area** to the subject field (word, halfword, byte, or instruction) that needs relocation.

### 7.3 Flags and SID Field

```
+------------------------------------------+
|  II  | B | A | R |  FT   | SID (24-bit)  |
+------------------------------------------+
|31 30|29 |28 |27 |26  25 | 24            0|
+------------------------------------------+
```

#### 7.3.1 SID (bits 0-23): Symbol/Area Identifier

**A=0 (A bit, bit 28 = 0):**
- SID = Area index in header (0-based)
- Relocate by base of specified area

**A=1 (A bit, bit 28 = 1):**
- SID = Symbol table index
- Relocate by symbol value

**Most significant bit (bit 23) set:**
- Remaining 7 bits specify area number (0-127)

#### 7.3.2 FT (bits 24-25): Field Type

| Value | Meaning |
|-------|---------|
| 00 | Byte (8 bits) |
| 01 | Halfword (16 bits) |
| 10 | Word (32 bits) |
| 11 | Instruction or sequence |

#### 7.3.3 R (bit 26): Relocation Type

| R | Meaning |
|---|---------|
| 0 | Plain additive: `field = field + value` |
| 1 | PC-relative: `field = field + (value - area_base)` |

#### 7.3.4 A (bit 28): Address Source

| A | Meaning |
|---|---------|
| 0 | Relocate by area base (SID = area index) |
| 1 | Relocate by symbol value (SID = symbol index) |

#### 7.3.5 B (bit 29): Based Area Relocation

| B | Meaning |
|---|---------|
| 0 | Normal relocation |
| 1 | Based on area group base (bits 29-30 must be 0, bit 31 = 1) |

#### 7.3.6 II (bits 30-31): Instruction Constraints

For instruction relocations (FT = 11):

| Value | Constraint |
|-------|------------|
| 00 | No constraint |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

---

## 8. OBJ_SYMT — Symbol Table Chunk

The symbol table contains definitions and references for all symbols in the object file.

### 8.1 Symbol Table Structure

```
+---------------------------+
| Symbol Entry 1 (16 bytes) |  4 words per entry
+---------------------------+
| Symbol Entry 2 (16 bytes) |
+---------------------------+
| ...                       |
+---------------------------+
| Symbol Entry N (16 bytes) |
+---------------------------+
```

### 8.2 Symbol Entry Format (4 words)

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `name` | Offset in string table |
| 0x04 | 4 | `attributes` | Symbol attributes |
| 0x08 | 4 | `value` | Symbol value or size |
| 0x0C | 4 | `area_name` | Defining area (offset in string table) |

### 8.3 Symbol Attributes

```
+------------------------------------------+
| Unused | Type | Flags | Definition       |
+------------------------------------------+
|31    27|26  24|23   16|15              0|
+------------------------------------------+
```

#### 8.3.1 Definition Bits (bits 0-1)

| Bits | Value | Meaning |
|------|-------|---------|
| 00 | 0x00 | Reserved |
| 01 | 0x01 | Defined in this file (local scope) |
| 10 | 0x02 | Reference (defined elsewhere) |
| 11 | 0x03 | Global scope (defined and exported) |

#### 8.3.2 Flag Bits

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 2 | 0x04 | ABS | Absolute attribute (not relocatable) |
| 3 | 0x08 | CASE_INSENS | Case-insensitive match |
| 4 | 0x10 | WEAK | Weak definition |
| 5 | 0x20 | — | Reserved |
| 6 | 0x40 | COMMON | Common symbol reference |
| 7 | 0x80 | — | Reserved |

#### 8.3.3 Symbol Type (bits 24-26)

| Type | Meaning | Description |
|------|---------|-------------|
| 0 | — | Not used |
| 1-8 | — | Reserved |
| 9 | FP_ARGS | Floating-point args in FP registers |
| 10 | REG_SCR | Scratch register value |
| 11 | REG_PRES | Preserved register value |
| 12 | COPROC | Coprocessor register (scratch) |
| 13 | COP_PRES | Coprocessor register (preserved) |

### 8.4 Symbol Value Interpretation

| Symbol Type | Value Field Contains |
|-------------|---------------------|
| Defining (bit 0 set) | Offset from area base |
| Absolute (bit 2 set) | Absolute value |
| Common (bit 6 set) | Size of common area |

### 8.5 Area Name Field

Offset into string table of the area containing the symbol definition. Zero if symbol is absolute.

---

## 9. OBJ_STRT — String Table Chunk

The string table contains all printable names referenced in the object file.

### 9.1 Structure

```
+---------------------------+
| Table Length (4 bytes)    |  Includes this word
+---------------------------+
| String 1                  |  Null-terminated
+---------------------------+
| String 2                  |
+---------------------------+
| ...                       |
+---------------------------+
| String N                  |
+---------------------------+
```

### 9.2 String Format

- Sequence of non-control characters (ASCII 32-126, 160-255)
- Terminated by NULL (0x00) byte
- No alignment padding between strings

### 9.3 Length Word

First 4 bytes contain total table length including the length word itself:
- No valid offset is less than 4
- Minimum table size is 4 bytes (empty table)

### 9.4 String Table Offsets

All name references (area names, symbol names) use offsets from the start of the string table:
- Offset 0 is reserved (means "no name")
- Offset 4 points to first actual string

---

## 10. OBJ_IDFN — Identification Chunk

Identifies the tool that created the object file.

### 10.1 Structure

```
+---------------------------+
| Identification String     |  Null-terminated
+---------------------------+
```

### 10.2 Content Format

- Printable characters (ASCII 10-13, 32-126)
- Null-terminated
- Describes tool name and version
- Example: `"ARM Ltd. Assembler 2.50"`

---

## 11. Common Area Support

### 11.1 Common Block Definition

Areas with bit 4 set (COMBLK) define common blocks:

```
area_name:      ; Name of common block
    AREA    |Common|COMBLK|,ALIGN=0
    ; No initialization data
    DCD     0       ; Just establishes block
```

### 11.2 Common Block Reference

Areas with bit 3 set (COMMON) reference a common block:

```
area_name:
    AREA    |Common|,ALIGN=0
    ; References to symbols in this area
    ; get resolved to definition block
```

### 11.3 Linker Behavior

1. Finds all common definitions with same name
2. Creates single area with maximum required size
3. Resolves all references to this consolidated area
4. Addresses stub bits for alignment information

---

## 12. Complete Header Map

```
Chunk File Header:
+---------------------------+
| ChunkFileId (8 bytes)     |  "OBJ_HEAD" + chunk type
+---------------------------+
| Header Offset (4)         |  Offset of OBJ_HEAD chunk
+---------------------------+
| Header Size (4)           |  Size of OBJ_HEAD chunk
+---------------------------+
| Areas Offset (4)          |  Offset of OBJ_AREA chunk
+---------------------------+
| Areas Size (4)            |  Size of OBJ_AREA chunk
+---------------------------+
| ... (other chunks)        |
+---------------------------+

OBJ_HEAD Chunk:
+---------------------------+
| file_type (4)             |  0xC5E2D080
+---------------------------+
| version_id (4)            |  310 (0x136)
+---------------------------+
| num_areas (4)             |  Number of areas
+---------------------------+
| num_symbols (4)           |  Number of symbols
+---------------------------+
| entry_area (4)            |  Entry area index (1-based)
+---------------------------+
| entry_offset (4)          |  Offset in entry area
+---------------------------+
| Area 1 Name Offset (4)    |  String table offset
+---------------------------+
| Area 1 Attributes (4)     |  Area type + flags
+---------------------------+
| Area 1 Size (4)           |  Bytes
+---------------------------+
| Area 1 Relocs (4)         |  Number of relocations
+---------------------------+
| Area 1 Base (4)           |  (Absolute areas only)
+---------------------------+
| ... (Area 2...)           |
+---------------------------+

OBJ_AREA Chunk:
+---------------------------+
| Area 1 Data               |  area_size[0] bytes
+---------------------------+
| Area 1 Reloc 1 Offset (4) |
+---------------------------+
| Area 1 Reloc 1 Flags (4)  |
+---------------------------+
| Area 1 Reloc 2 Offset (4) |
+---------------------------+
| Area 1 Reloc 2 Flags (4)  |
+---------------------------+
| ...                       |
+---------------------------+

OBJ_SYMT Chunk:
+---------------------------+
| Symbol 1 Name (4)         |  String table offset
+---------------------------+
| Symbol 1 Attr (4)         |  Attributes
+---------------------------+
| Symbol 1 Value (4)        |  Value/size
+---------------------------+
| Symbol 1 Area (4)          |  Area name offset
+---------------------------+
| ... (Symbol 2...)          |
+---------------------------+

OBJ_STRT Chunk:
+---------------------------+
| Table Length (4)           |  Includes this word
+---------------------------+
| "\0"                      |  Offset 0 = empty string
+---------------------------+
| "area_name\0"             |  Offset 4
+---------------------------+
| "symbol_name\0"           |
+---------------------------+
| ...                       |
+---------------------------+

OBJ_IDFN Chunk:
+---------------------------+
| "ARM Ltd. armcc 2.50\0"  |  Tool identification
+---------------------------+
```

---

## 13. Symbol Attribute Reference

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | 0x0001 | DEFINED | Symbol defined in this file |
| 1 | 0x0002 | GLOBAL | Symbol exported (visible externally) |
| 2 | 0x0004 | ABS | Absolute (not relocatable) |
| 3 | 0x0008 | CASE_INS | Case-insensitive match |
| 4 | 0x0010 | WEAK | Weak definition |
| 5 | 0x0020 | — | Reserved |
| 6 | 0x0040 | COMMON | Common symbol reference |
| 7 | 0x0080 | — | Reserved |
| 8 | 0x0100 | CODE_DATUM | Datum in code area (not code) |
| 9 | 0x0200 | FP_ARGS | FP args in FP registers |
| 10-11 | — | — | Reserved |
| 12 | 0x1000 | THUMB | Thumb symbol |

---

## 14. Area Attribute Reference

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | 0x01 | ABS | Absolute area |
| 1 | 0x02 | REL | Relocatable area |
| 2 | 0x04 | CODE | Code area |
| 3 | 0x08 | COMMON | Common area (uninitialized) |
| 4 | 0x10 | COMBLK | Common block |
| 5 | 0x20 | — | Reserved |
| 6 | 0x40 | DEBUG | Debugging tables |
| 7 | 0x80 | ZINIT | Zero-initialized |
| 8 | 0x0100 | NO_CODE | Must not contain code |
| 9 | 0x0200 | PI_CODE | Position independent |
| 10 | 0x0400 | REENT | Reentrant/common |
| 11 | 0x0800 | READONLY | Read-only |
| 12 | 0x1000 | DEBUG_TBL | Debugging table |
| 13-14 | — | ALIGN | Alignment (log2) |
| 15 | 0x8000 | BASE_VALID | Base address valid |
| 16 | 0x010000 | APCS_32 | 32-bit APCS |
| 17 | 0x020000 | REENT | Reentrant code |
| 18 | 0x040000 | FPU | Uses FP instructions |
| 19 | 0x080000 | NOSTCHK | No stack check |
| 20 | 0x100000 | THUMB | Thumb code |
| 21 | 0x200000 | HALFWORD | Halfword allowed |
| 22 | 0x400000 | INTERWORK | ARM/Thumb interwork |

---

## 15. Implementation Guidelines

### 15.1 Reading AOF Files

1. **Detect endianness** from ChunkFileId
2. **Parse chunk header** to locate chunks
3. **Read OBJ_HEAD** to get area count and locations
4. **Read OBJ_AREA** to get code/data and relocations
5. **Read OBJ_STRT** if names needed
6. **Read OBJ_SYMT** if symbols needed
7. **Apply relocations** when linking

### 15.2 Writing AOF Files

1. **Determine chunks** to include
2. **Build string table** with all names
3. **Calculate offsets** for all chunks
4. **Write chunk header**
5. **Write chunk contents** in any order
6. **Ensure 4-byte alignment** of chunk offsets

### 15.3 Chunk Order

Chunks may appear in **any order** in the file. The chunk header provides all necessary navigation information.

### 15.4 Memory Allocation

When reading an AOF file:
- Allocate buffer for entire file or chunks
- Validate all size fields
- Check for integer overflow in offset calculations
- Verify chunk offsets are within file bounds

---

## 16. Example AOF File

### 16.1 Sample Assembly Source

```arm
    AREA    |.text|, CODE, READONLY
    ENTRY

start
    MOV     r0, #42
    B       start

    AREA    |.data|, DATA, READWRITE
value   DCD     0x12345678

    AREA    |.bss|, ZINIT
buffer  SPACE   256
```

### 16.2 Resulting AOF Structure

```
Chunk File Header:
  ChunkFileId:     "OBJ_HEAD"  -> offset 28, size = 80
  OBJ_AREA offset: 108
  OBJ_STRT offset: 300

OBJ_HEAD (80 bytes):
  file_type:       0xC5E2D080
  version_id:      0x136 (310)
  num_areas:       3
  num_symbols:     5
  entry_area:      1
  entry_offset:    0

  Area 0 (.text):
    name:          4 (offset in string table)
    attributes:    0x0400A4 (CODE | READONLY | ALIGN=4)
    size:          8
    relocs:        0

  Area 1 (.data):
    name:          11
    attributes:    0x0800A4 (DATA | READWRITE | ALIGN=4)
    size:          4
    relocs:        0

  Area 2 (.bss):
    name:          18
    attributes:    0x8800A4 (ZINIT | ALIGN=4)
    size:          256
    relocs:        0

OBJ_AREA (12 bytes):
  .text:           E3A0002A E3A0FFFA (MOV #42, B start)
  .data:           12345678
  .bss:            (no data - zero-init)

OBJ_STRT (35 bytes):
  Length:          35
  [0]:             ""
  [4]:             ".text"
  [11]:            ".data"
  [18]:            ".bss"
  [23]:            "start"
  [29]:            "value"
  [36]:            "buffer"

OBJ_IDFN (21 bytes):
  "ARM Ltd. armasm 2.50\0"
```

---

## 17. References

1. ARM Software Development Toolkit Reference Guide (DUI 0041C)
2. ARM Software Development Toolkit User Guide (DUI 0040)
3. RISC OS Programmer's Reference Manuals
4. ARM Architectural Reference Manual (ARM DUI 0100)

---

## Appendix A: Magic Numbers

| Value | Hex | Description |
|-------|-----|-------------|
| ChunkFileId (LE) | C3 CB C6 C5 | Chunk file identifier |
| ChunkFileId (BE) | C5 C6 CB C3 | Byte-swapped chunk file |
| AOF File Type | C5 E2 D0 80 | AOF object file |
| AOF Version | 00 00 01 36 | Version 310 |

---

## Appendix B: Attribute Masks

```
Area Attributes Word:
  Bits 0-7:   Type (exactly one set)
  Bits 8-15:  Type modifiers
  Bits 16-22: Code attributes
  Bits 23:    Reserved (0)
  Bits 24-31: Stub bits

Symbol Attributes Word:
  Bits 0-1:   Definition scope
  Bits 2-7:   Flags
  Bits 8-11:  Reserved
  Bits 12:    Thumb
  Bits 13-23: Reserved
  Bits 24-26: Symbol type
  Bits 27-31: Reserved
```

---

*Document Version: 1.0*
*Last Updated: Based on ARM SDT 2.50 (1998) and RISC OS PRMs documentation*
