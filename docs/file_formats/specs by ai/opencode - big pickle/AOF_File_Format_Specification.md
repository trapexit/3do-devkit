# ARM Object Format (AOF) - Complete File Format Specification

## Overview

The ARM Object Format (AOF) is a relocatable object file format used by ARM assemblers and compilers. It serves as intermediate output for language processors and input to the ARM linker. AOF is designed to be simple to generate and process while providing comprehensive descriptive facilities for code and data organization.

### History

- **Late 1980s**: Developed for ARM development toolchain
- **Version 1.50**: Decimal 150 - early ARM SDT
- **Version 2.00**: Decimal 200 - enhanced features
- **Version 3.10**: Decimal 310 (`0x136`) - current version with Thumb and extended attribute support

### Design Principles

AOF was designed with these criteria in mind:
- Simple to generate and process
- Generalized superset of Unix a.out capabilities
- Support for ARM Procedure Call Standard (APCS)
- Extensible through chunk-based architecture
- Support for both little-endian and big-endian systems

## File Structure

### Chunk-Based Architecture

AOF is built on the Chunk File Format, which provides a simple and efficient means of accessing distinct data chunks within a single file.

#### Chunk File Header

```
┌─────────────────────────────────────┐
│ ChunkFileId (0xC3CBC6C5)           │  Word 0
├─────────────────────────────────────┤
│ maxChunks                           │  Word 1
├─────────────────────────────────────┤
│ numChunks                           │  Word 2
├─────────────────────────────────────┤
│ Chunk Entry 1                       │  4 words
├─────────────────────────────────────┤
│ Chunk Entry 2                       │  4 words
├─────────────────────────────────────┤
│ ... (maxChunks entries total)         │
└─────────────────────────────────────┘
```

#### Chunk Entry Format

Each chunk entry is 4 words (16 bytes):

| Word | Field | Description |
|------|--------|-------------|
| 0    | chunkId (8 bytes) | 8-character identifier, endian-independent |
| 1    | fileOffset | Byte offset from start of file (word-aligned) |
| 2    | size | Exact byte size of chunk contents |

### AOF Chunk Types

AOF defines five standard chunks:

| Chunk Name | Identifier | Description | Required |
|------------|-------------|-------------|-----------|
| OBJ_HEAD   | `OBJ_HEAD`  | AOF header and area declarations | Yes |
| OBJ_AREA   | `OBJ_AREA`  | Code, data, and debugging areas | Yes |
| OBJ_IDFN   | `OBJ_IDFN`  | Producer identification | No |
| OBJ_SYMT   | `OBJ_SYMT`  | Symbol table | No |
| OBJ_STRT   | `OBJ_STRT`  | String table | No |

Chunks may appear in any order within the file.

## OBJ_HEAD Chunk - AOF Header

The AOF header chunk contains a fixed-size section followed by variable-length area declarations.

### Fixed Header Section (6 words)

| Offset | Field | Description |
|--------|-------|-------------|
| 0x00   | Object File Type | Must be `0xC5E2D080` (identifies AOF) |
| 0x04   | Version Id | Version number: 150 (v1.50), 200 (v2.00), or 310 (`0x136`) for v3.10 |
| 0x08   | Number of Areas | Count of area declarations that follow |
| 0x0C   | Number of Symbols | Number of symbols in symbol table (0 if no OBJ_SYMT) |
| 0x10   | Entry Area Index | 1-origin index of area containing entry point (0 if none) |
| 0x14   | Entry Offset | Byte offset of entry point within entry area |

### Area Declarations (5 words each)

Following the fixed header are `Number of Areas` entries:

| Offset | Field | Description |
|--------|-------|-------------|
| 0x00   | Area Name | Offset into string table of area name |
| 0x04   | Attributes + Alignment | Attribute flags and alignment encoding |
| 0x08   | Area Size | Size in bytes (must be multiple of 4) |
| 0x0C   | Number of Relocations | Count of relocation directives for this area |
| 0x10   | Base Address | Absolute base address (used only for absolute areas) |

#### Area Attributes and Alignment

The `Attributes + Alignment` word encodes area properties:

- **Bits 0-7**: Alignment (power of 2, values 2-32 for 2^alignment)
- **Bits 8-31**: Attribute flags

| Bit | Mask | Attribute | Description | Scope |
|-----|------|-----------|-------------|--------|
| 8   | `0x00000100` | Absolute | Area must be placed at Base Address |
| 9   | `0x00000200` | Code | Area contains executable code |
| 10  | `0x00000400` | Common Definition | Common area definition |
| 11  | `0x00000800` | Common Reference | Reference to common block |
| 12  | `0x00001000` | Uninitialized | Zero-initialized data |
| 13  | `0x00002000` | Read Only | Area will not be modified after relocation |
| 14  | `0x00004000` | Position Independent | PC-relative addressing only |
| 15  | `0x00008000` | Debugging Tables | Contains symbolic debugging information |
| 16  | `0x00010000` | 32-bit PC | Complies with 32-bit APCS |
| 17  | `0x00020000` | Reentrant | Reentrant code variant |
| 18  | `0x00040000` | Extended FP | Uses ARM floating-point instruction set |
| 19  | `0x00080000` | No Software Stack Check | Disables stack checking |
| 20  | `0x00100000` | Based | Addressed via base register |
| 21  | `0x00200000` | Shared Library Stub | Shared library stub data |
| 22-23| `0x00C00000` | Reserved | Must be 0 |
| 24-27| `0x0F000000` | Base Register | Base register number for based areas |
| 28-31| `0xF0000000` | Reserved | Must be 0 |

### Attribute Combinations

#### Code Areas (Bit 9 set)
- Bits 16-19 can be non-zero for APCS variants
- Bit 20 cannot be set (based areas are data-only)
- Bit 21 typically set by linker for shared libraries
- Bit 13 should be set (read-only)
- Bit 12 incompatible (code cannot be uninitialized)

#### Data Areas (Bit 9 clear)
- Bit 10-11 for common block handling
- Bit 12 for zero-initialized data
- Bit 13 for read-only data
- Bit 20-21 for shared library and based areas

#### Common Areas
- **Definition (Bit 10)**: Defines size and contents of common block
- **Reference (Bit 11)**: References common block defined elsewhere
- Multiple definitions must have identical contents
- Size of common is maximum of all definitions

## OBJ_AREA Chunk - Areas Data

The areas chunk contains the actual code and data for all areas, followed by relocation information.

### Layout

```
┌─────────────────────────────────────┐
│ Area 1 Data                         │ (Area Size bytes)
├─────────────────────────────────────┤
│ Area 1 Relocation Directives        │ (Number of Relocations * 8 bytes)
├─────────────────────────────────────┤
│ Area 2 Data                         │ (Area Size bytes)
├─────────────────────────────────────┤
│ Area 2 Relocation Directives        │ (Number of Relocations * 8 bytes)
├─────────────────────────────────────┤
│ ... (continues for all areas)        │
└─────────────────────────────────────┘
```

### Area Data

- Present only if Not Initialized bit (12) is clear
- Size exactly as specified in area header
- Word-aligned (4-byte boundaries)
- Contains actual machine code, initialized data, or debugging tables

### Relocation Directives

Each relocation directive is 2 words (8 bytes):

| Word | Field | Description |
|------|--------|-------------|
| 0    | Offset | Byte offset from area base of field to relocate |
| 1    | Flags and SID | Relocation type and target identifier |

#### Relocation Format (Word 1)

```
┌─────────────────────────────────────────────────────────────┐
│31    30   29   28   27   26   25   24   23-0            │
│ II    B    A    R    FT=00 FT=01 FT=10 FT=11   SID (24 bits) │
└─────────────────────────────────────────────────────────────┘
```

- **SID (bits 0-23)**: Symbol ID or Area ID
  - If A bit (27) = 1: Symbol table index
  - If A bit (27) = 0: Area index (bit 23 = 1 for area)
- **FT (bits 24-25)**: Field Type
  - 00: Byte (8 bits)
  - 01: Halfword (16 bits)  
  - 10: Word (32 bits)
  - 11: Instruction (ARM or Thumb based on offset LSB)
- **B (bit 28)**: Based area relocation
- **R (bit 26)**: PC-relative relocation
- **II (bits 29-30)**: Instruction count constraint
  - 00: No constraint
  - 01: At most 1 instruction
  - 10: At most 2 instructions
  - 11: At most 3 instructions

#### Relocation Calculations

The relocation value depends on flags:

1. **Plain Additive (R=0, B=0)**:
   ```
   new_value = old_value + relocation_symbol
   ```

2. **PC-Relative (R=1, B=0)**:
   ```
   new_value = old_value + (relocation_symbol - area_base)
   ```

3. **Based Area (R=0, B=1)**:
   ```
   new_value = old_value + (relocation_symbol - area_group_base)
   ```

## OBJ_SYMT Chunk - Symbol Table

The symbol table chunk contains information about global symbols defined or referenced by the object file.

### Symbol Entry Format (4 words each)

| Word | Field | Description |
|------|--------|-------------|
| 0    | Name | Offset into string table of symbol name |
| 1    | Attributes | Symbol attribute flags |
| 2    | Value | Symbol value or size |
| 3    | Area Name | Offset of defining area name (for non-absolute symbols) |

### Symbol Attributes

| Bit | Mask | Attribute | Description |
|-----|------|-----------|-------------|
| 0   | `0x00000001` | Defined | Symbol is defined in this object file |
| 1   | `0x00000002` | Global | Symbol is visible to other object files |
| 2   | `0x00000004` | Absolute | Symbol has absolute value |
| 3   | `0x00000008` | Case Insensitive | Case-insensitive matching |
| 4   | `0x00000010` | Weak | Weak definition (can be overridden) |
| 5   | `0x00000020` | Reserved | Must be 0 |
| 6   | `0x00000040` | Common | Symbol is common block reference |
| 7   | `0x00000080` | Reserved | Must be 0 |
| 8   | `0x00000100` | Code Datum | Symbol identifies code data, not instruction |
| 9   | `0x00000200` | FP Args | Function uses FP registers for arguments |
| 10-11| `0x00000C00` | Reserved | Must be 0 |
| 12  | `0x00001000` | Thumb | Symbol is Thumb code |
| 13-31| `0xFFFFE000` | Reserved | Must be 0 |

### Symbol Types and Values

#### Defined Symbols (Bit 0 = 1)
- **Absolute (Bit 2 = 1)**: Value field contains absolute address
- **Non-Absolute**: Value = offset from base of area named by Area Name field

#### Undefined Symbols (Bit 0 = 0)
- **Global (Bit 1 = 1)**: External reference to be resolved by linker
- **Local (Bit 1 = 0)**: Reference within object file
- Value and Area Name fields are 0

#### Common Symbols (Bit 6 = 1)
- Value field contains size of common block
- Area Name field is 0
- Symbol refers to uninitialized data area

## OBJ_STRT Chunk - String Table

The string table chunk contains all string data referenced by other chunks.

### Format

```
┌─────────────────────────────────────┐
│ Length (including this word)       │  Word 0
├─────────────────────────────────────┤
│ String 1 (NUL-terminated)       │
├─────────────────────────────────────┤
│ String 2 (NUL-terminated)       │
├─────────────────────────────────────┤
│ ... (continues)                   │
└─────────────────────────────────────┘
```

### Properties

- First word contains total length including itself
- All strings are ASCII (ISO-8859 non-control characters)
- Strings are NUL-terminated (`0x00`)
- No alignment requirements between strings
- Valid offsets start at 4 (after length word)

## OBJ_IDFN Chunk - Identification

The identification chunk contains information about the tool that created the object file.

### Format

```
┌─────────────────────────────────────┐
│ Tool Identification String          │ NUL-terminated
└─────────────────────────────────────┘
```

### Content

- Contains printable characters (codes 10-13, 32-126)
- Typically includes tool name, version, and build information
- Example: "ARM Assembler 2.50"
- NUL-terminated, no fixed length

## Data Types and Alignment

### Basic Data Types

| Type | Size | Alignment | Description |
|-------|-------|------------|-------------|
| byte   | 1 byte | Any boundary | Unsigned 8-bit value |
| halfword | 2 bytes | 2-byte boundary | Unsigned 16-bit value |
| word   | 4 bytes | 4-byte boundary | Unsigned 32-bit value |
| string | variable | Any boundary | NUL-terminated sequence |

### Endianness

Two variants of AOF exist:

#### Little-Endian AOF
- Least significant byte at lowest address
- Used by DEC, Intel, Acorn systems
- Default for most ARM development

#### Big-Endian AOF  
- Most significant byte at lowest address
- Used by IBM, Motorola, Apple systems
- Detected from Object File Type field

### Compatibility Rules

- Object files must have consistent endianness throughout
- Linker accepts either endian format but rejects mixed inputs
- Output file endianness matches input files
- Target system endianness determines required file format

## Usage Examples

### Reading AOF Header

```c
typedef struct {
    uint32_t object_type;        // 0xC5E2D080
    uint32_t version_id;         // 310 for v3.10
    uint32_t num_areas;
    uint32_t num_symbols;
    uint32_t entry_area_index;
    uint32_t entry_offset;
} AOFSHeader;

typedef struct {
    uint32_t area_name;         // String table offset
    uint32_t attributes;        // Attribute flags
    uint32_t area_size;         // Size in bytes
    uint32_t num_relocations;    // Count of relocations
    uint32_t base_address;       // For absolute areas
} AOFAreaHeader;

// Read header
AOFSHeader* header = (AOFSHeader*)file_data;
AOFAreaHeader* areas = (AOFAreaHeader*)(header + 1);

for (int i = 0; i < header->num_areas; i++) {
    printf("Area %d: name_offset=0x%08x, size=%d\n", 
           i, areas[i].area_name, areas[i].area_size);
}
```

### Processing Relocations

```c
typedef struct {
    uint32_t offset;
    uint32_t flags_sid;
} AOFRelocation;

void process_relocations(uint8_t* area_data, AOFRelocation* relocs, 
                     int count, uint32_t area_base) {
    for (int i = 0; i < count; i++) {
        uint32_t offset = relocs[i].offset;
        uint32_t flags = relocs[i].flags_sid;
        
        uint32_t* field = (uint32_t*)(area_data + offset);
        uint32_t sid = flags & 0x00FFFFFF;
        bool is_symbol = (flags & 0x08000000) != 0;
        uint32_t field_type = (flags >> 24) & 0x03;
        
        // Calculate relocation value based on symbol/area
        uint32_t reloc_value = get_relocation_value(is_symbol, sid);
        
        // Apply relocation based on type
        if ((flags & 0x04000000) && (flags & 0x01000000)) {
            // PC-relative
            *field = *field + (reloc_value - area_base);
        } else {
            // Plain additive
            *field = *field + reloc_value;
        }
    }
}
```

### Symbol Resolution

```c
typedef struct {
    uint32_t name_offset;
    uint32_t attributes;
    uint32_t value;
    uint32_t area_name_offset;
} AOFSymbol;

const char* get_symbol_name(AOFSymbol* sym, char* string_table) {
    return string_table + sym->name_offset;
}

bool is_symbol_defined(AOFSymbol* sym) {
    return (sym->attributes & 0x00000001) != 0;
}

bool is_symbol_global(AOFSymbol* sym) {
    return (sym->attributes & 0x00000002) != 0;
}

bool is_symbol_common(AOFSymbol* sym) {
    return (sym->attributes & 0x00000040) != 0;
}
```

## Linker Processing

### Area Ordering

The linker sorts areas by:
1. Attributes (priority order: absolute, code, data, common)
2. Lexicographic order of area names (case-sensitive)
3. Position of object module in link list

### Common Block Resolution

1. Collect all common areas with same name
2. Find definition areas (bit 10 set)
3. Verify all definitions have identical contents
4. Use size from largest definition
5. Overlay all common areas at same location

### Symbol Resolution

1. Collect all global symbols from all object files
2. Resolve undefined symbols against definitions
3. Handle weak symbols (can be overridden)
4. Process common symbol references
5. Generate final symbol table for executable

### Relocation Processing

1. Assign base addresses to all areas
2. Calculate relocation offsets for each area
3. Process all relocation directives:
   - Symbol-based: add symbol absolute address
   - Area-based: add area base address
   - PC-relative: adjust for target location
4. Apply relocations to area data

## Implementation Notes

### Version Compatibility

- **Version 1.50**: Basic AOF format
- **Version 2.00**: Enhanced attribute support
- **Version 3.10**: Thumb instruction support, extended attributes

### Extensions

Additional chunks can be added for:
- Language-specific symbol tables
- Custom debugging information
- Source code correlation data
- Optimization metadata

### Toolchain Integration

- **armasm**: Generates AOF from assembly source
- **armcc/armcpp**: Generate AOF from C/C++ source
- **armlink**: Consumes AOF, produces executables or libraries
- **armlib**: Manages AOF libraries

## Limitations

- No built-in compression
- No implicit shared library support
- Limited to 32-bit address spaces
- No dynamic relocation support
- Fixed-size data structures (no 64-bit support)

## References

- ARM DUI 0041C Reference Guide (1997-1998)
- ARM Software Development Toolkit User Guide
- RISC OS Programmer's Reference Manuals
- ARM Architecture Reference Manual
- AOF and ALF file format specifications