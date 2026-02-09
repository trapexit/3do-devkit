# ARM Object Format (AOF) - Complete File Format Specification

## Overview

The ARM Object Format (AOF) is the relocatable object file format used by ARM Software Development Toolkit (SDT) compilers and assemblers. AOF serves as the intermediate format between language processors (compilers, assemblers) and the linker. It provides comprehensive support for code areas, data areas, symbol tables, relocation directives, and debugging information.

## Purpose and Usage

AOF files are:
- **Produced by**: ARM C/C++ compilers (armcc, armcpp), Thumb compilers (tcc, tcpp), ARM assembler (armasm), and other language processors
- **Consumed by**: ARM linker (armlink) to produce executable images
- **Stored in**: ARM Library Format (ALF) files via the librarian (armlib)
- **Decoded by**: decaof utility for inspection and debugging

## Byte Ordering and Alignment

### Endianness

AOF files can be produced in either endianness:
- **Little-endian**: Least significant byte at lowest address (standard for most ARM systems)
- **Big-endian**: Most significant byte at lowest address

The endianness of the AOF file matches the endianness of the target ARM system, not necessarily the host system that created it.

**Determining Endianness**:
The chunk file ID at offset 0 can detect endianness:
- Little-endian: 0xC3CBC6C5
- Big-endian (byte-swapped read): 0xC5C6CBC3

If the ID appears byte-swapped, all word values must be reversed before use.

### Alignment

- **Strings and bytes**: May align on any byte boundary
- **Words**: Must be 4-byte aligned (address divisible by 4)
- **Halfwords**: Must be 2-byte aligned (address divisible by 2)
- **AOF internal fields**: Align on word boundaries only (no halfword fields)

## Overall Structure

AOF is built on top of Chunk File Format, which provides a flexible, extensible container for distinct data sections.

### Chunk File Structure

```
+-----------------------------+  Offset 0
| Chunk File Header           |
| - ChunkFileId (4 bytes)     |
| - maxChunks (4 bytes)       |
| - numChunks (4 bytes)       |
+-----------------------------+
| Chunk Directory Entries     |
| (4 words per entry)         |
| - chunkId (8 bytes)         |
| - fileOffset (4 bytes)      |
| - size (4 bytes)            |
+-----------------------------+
| Chunk Data                  |
| (variable size per chunk)   |
+-----------------------------+
```

### AOF Chunks

An AOF file contains exactly five chunk types:

| Chunk | Chunk ID | Required | Description |
|-------|----------|----------|-------------|
| Header | OBJ_HEAD | Yes | Object file metadata and area declarations |
| Areas | OBJ_AREA | Yes | Code/data content and relocation directives |
| Identification | OBJ_IDFN | No | Producer identification string |
| Symbol Table | OBJ_SYMT | No | Symbol definitions and references |
| String Table | OBJ_STRT | No | String storage for names |

**Note**: While only Header and Areas chunks are strictly required, typical AOF files contain all five chunks.

## Chunk File Header

### Fixed Header (12 bytes)

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0x00 | ChunkFileId | 4 bytes | Magic number: 0xC3CBC6C5 |
| 0x04 | maxChunks | 4 bytes | Maximum number of chunk entries in directory |
| 0x08 | numChunks | 4 bytes | Currently used chunk entries (0 to maxChunks) |

### Chunk Directory Entries

Each entry is 16 bytes (4 words):

| Word | Field | Size | Description |
|------|-------|------|-------------|
| 0-1 | chunkId | 8 bytes | Two-part identifier (stored in address order, endianness-independent) |
| 2 | fileOffset | 4 bytes | Byte offset to chunk data (must be 4-byte aligned; 0 = unused entry) |
| 3 | size | 4 bytes | Exact byte size of chunk content (need not be multiple of 4) |

### Chunk ID Format

Chunk IDs are 8-byte identifiers split into two parts:
- **First 4 bytes**: Universal domain name (e.g., OBJ_ for object files)
- **Last 4 bytes**: Component identifier within domain

Characters are stored in ascending address order (first character first), independent of endianness.

**AOF Chunk IDs**:
- OBJ_HEAD - Header chunk
- OBJ_AREA - Areas chunk
- OBJ_IDFN - Identification chunk
- OBJ_SYMT - Symbol table chunk
- OBJ_STRT - String table chunk

## AOF Header Chunk (OBJ_HEAD)

The header chunk has two contiguous parts:
1. **Fixed Header** (6 words): Object file metadata
2. **Area Headers** (5 words each per area): Area declarations

### Fixed Header Fields

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0x00 | ObjectFileType | 4 bytes | Magic number: 0xC5E2D080 (endianness matches chunk file) |
| 0x04 | VersionId | 4 bytes | AOF version number (310 = 0x136 for current version) |
| 0x08 | NumAreas | 4 bytes | Number of code/data areas (should be less than or equal to 255) |
| 0x0C | NumSymbols | 4 bytes | Number of symbols in symbol table |
| 0x10 | EntryAreaIndex | 4 bytes | 1-origin index of area containing entry point (0 = no entry point) |
| 0x14 | EntryOffset | 4 bytes | Byte offset of entry point within entry area |

### Area Header Fields

Each area declaration is 20 bytes (5 words):

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0x00 | AreaName | 4 bytes | Offset into string table of area name (0 = no name, use areaXX) |
| 0x04 | Attributes | 4 bytes | Bit flags specifying area attributes |
| 0x08 | AreaSize | 4 bytes | Size of area in bytes |
| 0x0C | NumRelocs | 4 bytes | Number of relocation directives for this area |
| 0x10 | BaseAddress | 4 bytes | Base address for absolute areas (0 for non-absolute) |

## Area Attributes

The Attributes field is a 32-bit value with the following bit definitions:

### Type Bits (Bits 0-7)

At most one type bit may be set in an object file:

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | 0x00000001 | Absolute | Area has fixed, unchanging base address |
| 1 | 0x00000002 | Relocatable | Area may be relocated by linker |
| 2 | 0x00000004 | Code | Area contains executable code |
| 3 | 0x00000008 | Common | Uninitialized data (Fortran common block style) |
| 4 | 0x00000010 | CommonBlock | Common area must be a single contiguous block |
| 5 | 0x00000020 | LinkerUse | Reserved for linker use |
| 6 | 0x00000040 | DebugTable | Area contains debugging tables |
| 7 | 0x00000080 | ZeroInit | Zero-initialized data area |

### Attribute Bits (Bits 8-15)

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 8 | 0x00000100 | NoCode | Area cannot contain code |
| 9 | 0x00000200 | PositionIndependent | Area contains position-independent code (PIC) |
| 10 | 0x00000400 | Reentrant | Area is reentrant or common block |
| 11 | 0x00000800 | ReadOnly | Area contains only readable data |
| 12 | 0x00001000 | Debug | Area is a debugging table |
| 13-14 | 0x00006000 | Alignment | Area alignment: n = 2^n bytes (0=byte, 1=halfword, 2=word) |
| 15 | 0x00008000 | BaseValid | Area base field is valid (set by linker) |

### Extended Attributes (Bits 16-22)

Additional code area attributes (only valid if Code bit set):

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 16 | 0x00010000 | APCS32 | Complies with 32-bit APCS |
| 17 | 0x00020000 | ReentrantCode | Reentrant code attribute |
| 18 | 0x00040000 | ExtendedFP | Uses extended FP instruction set (LFM/SFM) |
| 19 | 0x00080000 | NoStackCheck | No software stack checking |
| 20 | 0x00100000 | ThumbCode | All relocations are Thumb code |
| 21 | 0x00200000 | Halfword | Area may contain ARM halfword instructions |
| 22 | 0x00400000 | Interwork | Suitable for ARM/Thumb interworking |

### Reserved (Bits 23-31)

Must be zero in object files.

## Area Types in Detail

### Absolute Areas (Bit 0)

- Base address remains constant through relocation and linking
- Area declaration includes valid BaseAddress field (bit 15 set)
- Cannot contain relocatable references to non-absolute areas
- Linker combines absolute areas without moving their bases

### Relocatable Areas (Bit 1)

- Linker may relocate by any convenient amount
- Most common type for code and data
- References resolved at link time

### Code Areas (Bit 2)

- Contains executable ARM or Thumb instructions
- Must have ReadOnly attribute (bit 11)
- PositionIndependent (bit 9) indicates no PC-relative references outside area

### Common Areas (Bit 3)

- Uninitialized data
- Multiple common areas with same name share address space
- Enables Fortran named common blocks
- Must have Reentrant attribute (bit 10)
- Must NOT have ReadOnly attribute

### Common Block Areas (Bit 4)

- Like common areas but must be single contiguous block
- Bits 24-27 of stub bits define maximum permitted alignment (log2)

### Zero-Initialized Areas (Bit 7)

- Like common areas but storage guaranteed initialized to zero
- Size must be whole number of words
- No initializing data in OBJ_AREA chunk
- Incompatible with ReadOnly attribute

### Debugging Table Areas (Bit 6)

- Contains symbolic debugging information
- Must have Debug attribute (bit 12)
- Code bit (bit 9) ignored

## Areas Chunk (OBJ_AREA)

The Areas chunk contains the actual content of all areas followed by their relocation directives.

### Layout

```
+-----------------------------+
| Area 1 Data                 |
| (size = AreaSize from       |
|  area header)               |
+-----------------------------+
| Area 1 Relocation Directives|
| (NumRelocs entries)         |
+-----------------------------+
| Area 2 Data                 |
+-----------------------------+
| Area 2 Relocation Directives|
+-----------------------------+
| ...                         |
+-----------------------------+
| Area N Data                 |
+-----------------------------+
| Area N Relocation Directives|
+-----------------------------+
```

### Area Data

- Aligned to 4-byte boundaries
- For initialized areas: raw code or data bytes
- For uninitialized/zero-init areas: omitted from chunk
- Endianness matches containing AOF file

## Relocation Directives

Relocation directives describe locations that must be updated when area bases are assigned or symbols resolved.

### Relocation Directive Format (8 bytes)

```
+-----------------------------+  Word 0
| Offset (32 bits)            |
| - Byte offset within area   |
+-----------------------------+  Word 1
| Flags and SID (32 bits)     |
| - Bits 0-23: SID            |
| - Bits 24-25: Field Type    |
| - Bit 26: R flag            |
| - Bit 27: A flag            |
| - Bit 28: B flag            |
| - Bits 29-30: II field      |
| - Bit 31: Reserved          |
+-----------------------------+
```

### Offset Field

- **Size**: 32 bits
- **Purpose**: Byte offset within the area where relocation applies
- **Alignment**: Subject field must be aligned according to field type

### SID Field (Bits 0-23)

**When A flag = 1**: Symbol index
- Bits 0-22: Symbol table index
- Bit 23: 0 = symbol index, 1 = reserved

**When A flag = 0**: Area index
- Bits 0-6: Area number (1-origin index in area headers array)
- Bits 7-23: Reserved (0)

**Note**: If bit 23 set, bits 0-6 contain area number to relocate by

### Field Type (Bits 24-25)

| Value | Type | Description |
|-------|------|-------------|
| 00 | Byte | 8-bit field |
| 01 | Halfword | 16-bit field |
| 10 | Word | 32-bit field |
| 11 | Instruction | ARM or Thumb instruction sequence |

### R Flag (Bit 26) - Relocation Type

- **0**: Plain additive relocation
  - subject = subject + relocation_value
  
- **1**: PC-relative relocation
  - subject = subject + (relocation_value - base_of_area_containing_subject)
  - Special case: if A=0 and relocation value equals base of containing area, no adjustment

### A Flag (Bit 27) - Address Mode

- **0**: Relocate by base of area (SID field contains area number)
- **1**: Relocate by symbol value (SID field contains symbol index)

### B Flag (Bit 28) - Based Area Relocation

- **0**: Normal relocation
- **1**: Based area relocation (inter-link-unit branch destination)
  - subject = subject + (relocation_value - base_of_area_group_containing_relocation_value)
  - Bits 29-30 must be 0, bit 31 must be 1

### II Field (Bits 29-30) - Instruction Count

For instruction field types (FT=11), constrains number of instructions that may be modified:

| Value | Meaning |
|-------|---------|
| 00 | No constraint |
| 01 | At most 1 instruction |
| 10 | At most 2 instructions |
| 11 | At most 3 instructions |

### Common Relocation Cases

1. **Word contains PC-relative offset**:
   - R=0, B=0, FT=10 (word)
   - Value: W + AN (offset relocated by base of area N)

2. **Word contains offset plus constant**:
   - Same as above with constant already in word

3. **24-bit PC-relative branch target**:
   - R=1, FT=11 (instruction)
   - Top 8 bits preserved, lower 24 bits modified

## Symbol Table Chunk (OBJ_SYMT)

The symbol table contains entries for all defined and referenced symbols.

### Symbol Entry Format (16 bytes)

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0x00 | Name | 4 bytes | Offset in string table of symbol name (0 = no name) |
| 0x04 | Attributes | 4 bytes | Bit flags defining symbol properties |
| 0x08 | Value | 4 bytes | Symbol value or common size (see below) |
| 0x0C | AreaName | 4 bytes | Offset in string table of defining area name |

### Symbol Attributes

| Bit | Mask | Name | Description |
|-----|------|------|-------------|
| 0 | 0x00000001 | Defined | Symbol is defined in this object file |
| 1 | 0x00000002 | Global | Symbol is exported to other object files |
| 2 | 0x00000004 | Absolute | Symbol has absolute (constant) value |
| 3 | 0x00000008 | CaseInsensitive | Symbol matching is case-insensitive |
| 4 | 0x00000010 | Weak | Weak definition (may be overridden) |
| 5 | 0x00000020 | Reserved | Must be 0 |
| 6 | 0x00000040 | Common | Symbol is a reference to common area |
| 7 | 0x00000080 | Reserved | Must be 0 |
| 8 | 0x00000100 | CodeDatum | Symbol identifies datum in code area |
| 9 | 0x00000200 | FPArgsInRegs | FP args passed in FP registers |
| 10 | 0x00000400 | Reserved | Must be 0 |
| 11 | 0x00000800 | Reserved | Must be 0 |
| 12 | 0x00001000 | Thumb | Thumb symbol |
| 16-23 | 0x00FF0000 | DatumType | Type and datum field (type-dependent) |
| 24-27 | 0x0F000000 | SymbolType | Symbol type classification |
| 28-31 | 0xF0000000 | Reserved | Must be 0 |

### Symbol Type Values (Bits 24-27)

| Value | Type | Description |
|-------|------|-------------|
| 0 | Unused | Not used |
| 1-8 | Reserved | Reserved for future use |
| 9 | FPRegister | Floating-point register (bits 16-17 = size, bits 18-23 = reg) |
| 10 | ScratchRegister | Scratch register (bits 16-23 = reg number, 255 = none) |
| 11 | PreservedRegister | Preserved register (bits 16-23 = reg number) |
| 12 | CoproScratch | Coprocessor scratch (R = 256*CN + REGNO) |
| 13 | CoproPreserved | Coprocessor preserved register |

### Symbol Definition Combinations

| Bit 1 (Global) | Bit 0 (Defined) | Meaning |
|----------------|-----------------|---------|
| 0 | 0 | Reserved |
| 0 | 1 | Defined, local to this file |
| 1 | 0 | Reference to external symbol |
| 1 | 1 | Defined with global scope |

### Value Field Interpretation

- **Absolute symbol** (bit 2 set): Contains absolute constant value
- **Common symbol** (bit 6 set): Contains size of common block in bytes
- **Defined non-absolute**: Offset from base of area specified by AreaName
- **Undefined symbol**: Should be 0

### Weak Symbol Rules

- Bit 4 (Weak) and bit 7 (Strong) should not both be set
- Weak symbol: Linker uses other definition if present
- Strong symbol: This definition preferred over weak ones
- Symbol can be both weak and global (bits 1 and 4 both set)
- Symbol cannot be both weak and case-insensitive (bits 3 and 4 both set)

## String Table Chunk (OBJ_STRT)

The string table stores all names referenced by the header and symbol table.

### Format

```
+-----------------------------+  Offset 0
| Length (4 bytes)            |
| - Total size including this |+-----------------------------+  Offset 4
| String 0 (NUL-terminated)   |
+-----------------------------+
| String 1 (NUL-terminated)   |
+-----------------------------+
| ...                         |
+-----------------------------+
```

### Properties

- **First 4 bytes**: Length of entire string table (including length word)
- **Minimum size**: 4 bytes (just the length word)
- **Minimum valid offset**: 4 (no valid offset is less than 4)
- **Strings**: NUL-terminated (0x00), printable characters (codes 32-126, 160-255)
- **Padding**: No specific alignment requirements
- **Endianness**: Length word matches AOF endianness

### Usage

- Area names referenced by offset from start of table
- Symbol names referenced by offset from start of table
- Offset 0 means "no name string"

## Identification Chunk (OBJ_IDFN)

Contains a single string identifying the tool that created the object file.

### Format

- Single NUL-terminated string
- Printable characters only (codes 10-13, 32-126)
- Codes 128-255 discouraged (host-dependent interpretation)

### Example Content

```
ARM C Compiler v2.50 (ARM Ltd.)
ObjAsm v3.00
```

## C Structure Definitions

```c
/* AOF Header Chunk */
#define AOF_MAGIC       0xC5E2D080
#define AOF_VERSION     310

typedef struct {
    uint32_t object_file_type;    /* 0xC5E2D080 */
    uint32_t version_id;          /* 310 */
    uint32_t num_areas;           /* Number of areas */
    uint32_t num_symbols;         /* Number of symbols */
    uint32_t entry_area;          /* 1-origin area index (0 = no entry) */
    uint32_t entry_offset;        /* Offset within entry area */
    /* Followed by area headers */
} AOFFixedHeader;

/* Area Header */
typedef struct {
    uint32_t area_name;           /* String table offset */
    uint32_t area_attributes;     /* Attribute flags */
    uint32_t area_size;           /* Size in bytes */
    uint32_t num_relocs;          /* Number of relocations */
    uint32_t area_base;           /* Base address (absolute areas only) */
} AOFAreaHeader;

/* Area Attribute Bits */
#define AOF_AREA_ABSOLUTE       0x00000001
#define AOF_AREA_RELOCATABLE    0x00000002
#define AOF_AREA_CODE           0x00000004
#define AOF_AREA_COMMON         0x00000008
#define AOF_AREA_COMMONBLOCK    0x00000010
#define AOF_AREA_DEBUGTABLE     0x00000040
#define AOF_AREA_ZERODATA       0x00000080
#define AOF_AREA_NOCODE         0x00000100
#define AOF_AREA_POSITIONINDEP  0x00000200
#define AOF_AREA_REENTRANT      0x00000400
#define AOF_AREA_READONLY       0x00000800
#define AOF_AREA_DEBUG          0x00001000
#define AOF_AREA_BASEVALID      0x00008000
#define AOF_AREA_APCS32         0x00010000
#define AOF_AREA_THUMB          0x00100000
#define AOF_AREA_INTERWORK      0x00400000

/* Relocation Directive */
typedef struct {
    uint32_t offset;              /* Byte offset in area */
    uint32_t flags_sid;           /* Flags and symbol/area ID */
} AOFRelocation;

/* Relocation Flags */
#define AOF_RELOC_FIELD_BYTE    0x00000000
#define AOF_RELOC_FIELD_HALF    0x01000000
#define AOF_RELOC_FIELD_WORD    0x02000000
#define AOF_RELOC_FIELD_INSTR   0x03000000
#define AOF_RELOC_FIELD_MASK    0x03000000

#define AOF_RELOC_RFLAG         0x04000000
#define AOF_RELOC_AFLAG         0x08000000
#define AOF_RELOC_BFLAG         0x10000000

#define AOF_RELOC_II_MASK       0x60000000
#define AOF_RELOC_II_SHIFT      29

#define AOF_RELOC_SID_MASK      0x00FFFFFF

/* Symbol Table Entry */
typedef struct {
    uint32_t name;                /* String table offset */
    uint32_t attributes;          /* Symbol attributes */
    uint32_t value;               /* Symbol value or size */
    uint32_t area_name;           /* String table offset of area */
} AOFSymbol;

/* Symbol Attribute Bits */
#define AOF_SYM_DEFINED         0x00000001
#define AOF_SYM_GLOBAL          0x00000002
#define AOF_SYM_ABSOLUTE        0x00000004
#define AOF_SYM_CASEINSENS      0x00000008
#define AOF_SYM_WEAK            0x00000010
#define AOF_SYM_COMMON          0x00000040
#define AOF_SYM_CODEDATUM       0x00000100
#define AOF_SYM_FPARGSREGS      0x00000200
#define AOF_SYM_THUMB           0x00001000

#define AOF_SYM_TYPE_MASK       0x0F000000
#define AOF_SYM_TYPE_SHIFT      24

/* Chunk File Header */
#define CHUNK_FILE_MAGIC        0xC3CBC6C5

typedef struct {
    uint32_t chunk_file_id;       /* 0xC3CBC6C5 */
    uint32_t max_chunks;          /* Max directory entries */
    uint32_t num_chunks;          /* Used directory entries */
} ChunkFileHeader;

/* Chunk Directory Entry */
typedef struct {
    char     chunk_id[8];         /* 8-byte identifier */
    uint32_t file_offset;         /* Offset to chunk data */
    uint32_t size;                /* Size of chunk content */
} ChunkDirEntry;
```

## Linker Symbol Table Format

For debugging and browsing, the linker can output its internal symbol table. This is a text file format (not AOF).

### Format

Each line: name,value,class[,definingmodulename]

**Fields**:
- **name**: Symbol name (at least 1 printable character)
- **value**: Symbol value as hex number prefixed by '&' (BBC Basic style)
- **class**: One of:
  - Abs - Absolute value
  - RelocDef - Defined but not referenced
  - RelocRef - Referenced but not defined
  - ExtDef - Defined and referenced externally
  - ExtRef - Referenced but not defined
- **definingmodulename**: Optional module name (from -mddname option)

### Example

```
main,&00008000,ExtDef,main.o
printf,&00000000,ExtRef,libc.o
_buffer,&00009000,RelocDef,data.o
```

## AOF Processing Guidelines

### Reading AOF Files

1. **Read chunk file header**: Verify magic number, determine endianness
2. **Parse chunk directory**: Build index of available chunks
3. **Read string table**: Required before processing names
4. **Read header chunk**: Get area declarations and symbol count
5. **Process areas**: Read code/data and apply relocations
6. **Read symbol table**: Resolve external references

### Writing AOF Files

1. **Prepare chunks**: Create all five chunk types in memory
2. **Calculate offsets**: Determine final file layout
3. **Write header**: Chunk file header and directory
4. **Write chunks**: In any order (conventionally: HEAD, AREA, IDFN, SYMT, STRT)
5. **Update directory**: Set correct offsets and sizes

### Creating Libraries

AOF files are collected into ALF (ARM Library Format) files by the librarian. See ALF specification for details.

## Version History

| Version | Value | Notes |
|---------|-------|-------|
| 200 | 0xC8 | Original RISC OS version |
| 310 | 0x136 | Current version (SDT 2.50) |

**Version 310 Changes**:
- Explicit zero-initialized data areas (bit 7)
- Extended attribute bits
- Thumb support

## Best Practices

### For Producers (Compilers/Assemblers)

1. Always set version_id to 310
2. Include identification chunk for debugging
3. Set appropriate area attributes accurately
4. Generate position-independent code where possible
5. Use common areas correctly for Fortran-style blocks
6. Verify relocation directives cover all necessary locations

### For Consumers (Linkers)

1. Handle both endiannesses correctly
2. Validate all magic numbers and version IDs
3. Check for reserved bits being zero
4. Support all defined relocation types
5. Handle weak symbol resolution correctly
6. Report meaningful errors for malformed files

### For Tools

1. Preserve unknown chunks when copying/modifying
2. Maintain chunk alignment requirements
3. Update chunk directory when modifying
4. Use string table consistently

## Related Formats

- **ALF (ARM Library Format)**: Container for AOF files
- **AIF (ARM Image Format)**: Output format from linker
- **ASD (ARM Symbolic Debug)**: Debugging data format

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- ARM DUI 0061: ARM Target Development System User Guide
- RISC OS Programmer's Reference Manuals

## Summary

AOF provides a comprehensive, flexible object file format supporting modern ARM development needs including 32-bit addressing, Thumb code, position-independent code, and extended floating-point. Its chunk-based structure allows easy extension while maintaining backward compatibility. Understanding AOF is essential for developing tools that interface with the ARM toolchain or for implementing custom linkers and loaders for ARM systems.
