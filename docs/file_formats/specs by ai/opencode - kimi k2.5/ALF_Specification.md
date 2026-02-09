# ARM Library Format (ALF) - Complete File Format Specification

## Overview

The **ARM Library Format (ALF)** is the archive format used by the ARM Software Development Toolkit (SDT) to store collections of ARM Object Format (AOF) files. ALF provides efficient access to library members through an indexed directory structure, supports symbol-based member lookup, and maintains timestamps for build system integration.

## Purpose and Usage

ALF files are:
- **Created by**: ARM librarian (armlib) from AOF object files
- **Consumed by**: ARM linker (armlink) during program linking
- **Used for**: Organizing related object files into reusable libraries
- **Inspected by**: decaof and armlib utilities

## Relationship to Other Formats

ALF builds upon the same **Chunk File Format** used by AOF, providing consistency in the ARM toolchain. The key difference is that ALF organizes multiple members (typically AOF files) within a single archive, while AOF represents a single compilation unit.

```
Source Files -> AOF Files -> ALF Library -> Linked Executable (AIF/ELF)
    (armcc)     (armasm)     (armlib)         (armlink)
```

## Byte Ordering and Alignment

### Endianness

ALF files follow the same endianness rules as AOF:
- **Little-endian**: Standard for most ARM systems
- **Big-endian**: Supported for big-endian ARM targets
- **Determination**: Chunk file ID 0xC3CBC6C5 indicates correct endianness

**Important**: The endianness of the ALF file must match the endianness of its contained members. Mixing endianness within a library is not meaningful and will be rejected by tools.

### Alignment

- **Words**: 4-byte aligned
- **Directory entries**: Word-aligned
- **Member data**: Preserves alignment of contained AOF files
- **Strings**: Byte-aligned

## Overall Structure

ALF uses the Chunk File Format with library-specific chunks:

```
+-----------------------------+  Offset 0
| Chunk File Header           |
| - ChunkFileId               |
| - maxChunks                 |
| - numChunks                 |
+-----------------------------+
| Chunk Directory             |
| (variable number of entries)|
+-----------------------------+
| LIB_DIRY - Library Directory|
+-----------------------------+
| LIB_TIME - Library Timestamp|
+-----------------------------+
| LIB_VRSN - Version Info     |
+-----------------------------+
| LIB_DATA - Member 1         |
+-----------------------------+
| LIB_DATA - Member 2         |
+-----------------------------+
| ...                         |
+-----------------------------+
| LIB_DATA - Member N         |
+-----------------------------+
| OFL_SYMT - Object Symbol    |
|            Table (optional) |
+-----------------------------+
| OFL_TIME - Symbol Table     |
|            Timestamp        |
+-----------------------------+
```

## ALF Chunks

ALF defines six chunk types:

| Chunk | Chunk ID | Required | Description |
|-------|----------|----------|-------------|
| Directory | LIB_DIRY | Yes | Index of library members |
| Time Stamp | LIB_TIME | Yes | Library modification time |
| Version | LIB_VRSN | Yes | Library format version |
| Data | LIB_DATA | Yes | Member contents (multiple instances) |
| Symbol Table | OFL_SYMT | No | External symbol index (object libraries) |
| Symbol Time | OFL_TIME | No | Symbol table modification time |

**Note**: An object code library (typical use case) includes all six chunk types.

## Chunk File Header

### Fixed Header (12 bytes)

| Offset | Field | Size | Value | Description |
|--------|-------|------|-------|-------------|
| 0x00 | ChunkFileId | 4 bytes | 0xC3CBC6C5 | Magic number identifying chunk file |
| 0x04 | maxChunks | 4 bytes | variable | Maximum chunk directory entries |
| 0x08 | numChunks | 4 bytes | variable | Currently used directory entries |

### Chunk Directory Entries

Each entry is 16 bytes:

| Word | Field | Size | Description |
|------|-------|------|-------------|
| 0-1 | chunkId | 8 bytes | Chunk identifier (LIB_ or OFL_ prefix) |
| 2 | fileOffset | 4 bytes | Byte offset to chunk data (4-byte aligned, 0 = unused) |
| 3 | size | 4 bytes | Exact byte size of chunk content |

**Library Chunk IDs**:
- LIB_DIRY - Library directory chunk
- LIB_TIME - Library timestamp chunk  
- LIB_VRSN - Library version chunk
- LIB_DATA - Library member data chunk (multiple allowed)
- OFL_SYMT - Object file library symbol table chunk
- OFL_TIME - Object file library symbol table timestamp chunk

## Library Directory Chunk (LIB_DIRY)

The directory chunk provides a lookup table for library members. It maps member names to their locations within the file.

### Directory Structure

The directory consists of a sequence of variable-length entries, each an integral number of words long.

### Directory Entry Format

| Field | Size | Description |
|-------|------|-------------|
| ChunkIndex | 4 bytes | Zero-origin index in chunk file header of corresponding LIB_DATA chunk |
| EntryLength | 4 bytes | Number of bytes in this LIB_DIRY entry (always multiple of 4) |
| DataLength | 4 bytes | Number of bytes used in data section (always multiple of 4) |
| Data | variable | Entry data (name, info, timestamp) |

### Data Section Format

The Data field contains, in order:

1. **Member Name** (variable): Zero-terminated ISO-8859 string using only non-control characters
2. **Additional Info** (optional): Any other information relevant to the library module (often empty)
3. **Time Stamp** (8 bytes): Two-word timestamp recording when member was added/modified

### ChunkIndex Values

| Value | Meaning |
|-------|---------|
| 0 | Directory entry is unused (available for reuse) |
| 3 or greater | Index of LIB_DATA chunk containing member (convention: first three chunks are LIB_DIRY, LIB_TIME, LIB_VRSN) |

### Entry Example

```
+-----------------------------+
| ChunkIndex = 3              |  Points to first LIB_DATA
+-----------------------------+
| EntryLength = 32            |  Total entry size in bytes
+-----------------------------+
| DataLength = 24             |  Data section size
+-----------------------------+
| "stdio.o" + NUL             |  Member name (8 bytes)
+-----------------------------+
| (empty info)                |  No additional info (0 bytes)
+-----------------------------+
| Timestamp Word 1            |  4 bytes
+-----------------------------+
| Timestamp Word 2            |  4 bytes
+-----------------------------+
| Padding to word boundary    |  Align to 32 bytes
+-----------------------------+
```

### Directory Management

- **Fixed size**: Directory size is fixed when library is created
- **Expansion**: Limited expansion possible by pre-allocating unused entries
- **Fragmentation**: Removing members leaves unused entries (ChunkIndex = 0)
- **Convention**: Space for 8-16 directory entries typically allocated initially

## Library Time Stamp Chunk (LIB_TIME)

Records when the library was last modified.

### Format

Two words (8 bytes) encoding:

| Word | Content | Description |
|------|---------|-------------|
| 0 (MSW) | Upper 4 bytes | Most significant 4 bytes of centisecond count |
| 1 (LSW) | Lower 2 bytes + microseconds | Lower 2 bytes of centisecond count (upper 16 bits) + 2-byte microsecond count (lower 16 bits) |

### Time Stamp Encoding

A library time stamp encodes:
- **6-byte centisecond count**: Seconds since 00:00:00 1st January 1900, multiplied by 100
- **2-byte microsecond count**: Microseconds since last centisecond boundary (usually 0)

**First word**: Contains most significant 4 bytes of the 6-byte centisecond count

**Second word**: 
- Bits 31-16: Least significant 2 bytes of centisecond count
- Bits 15-0: Microsecond count (typically 0)

### Time Stamp Words

Stored in target system byte order. Must have same endianness as containing chunk file.

### Example Time Stamps

| Date/Time | Centiseconds | Word 0 | Word 1 |
|-----------|--------------|--------|--------|
| 1900-01-01 00:00:00 | 0 | 0x00000000 | 0x00000000 |
| 2000-01-01 00:00:00 | 315576000000 | 0x0000498B | 0xA5400000 |

## Library Version Chunk (LIB_VRSN)

Contains a single word identifying the library format version.

### Format

| Size | Value | Meaning |
|------|-------|---------|
| 4 bytes | 1 | Current ALF version |

**Note**: Earlier versions of ARM tools used different version numbers. Version 1 is the current standard.

## Library Data Chunk (LIB_DATA)

Contains the actual content of a library member. Each member is stored in its own LIB_DATA chunk.

### Content

- **Object code library**: AOF file content
- **General library**: Any data (could be another library, text file, etc.)

### Properties

- **No interpretation**: Library management tools do not interpret member contents
- **Endianness**: Assumed to match containing library
- **Alignment**: Member data is word-aligned within the chunk
- **Multiple chunks**: Many LIB_DATA chunks allowed per library (one per member)

### Member Types

| Member Type | Typical Content | Usage |
|-------------|-----------------|-------|
| Object file | Complete AOF file | Code libraries |
| Text | Source code, documentation | General archives |
| Archive | Nested ALF file | Hierarchical libraries |
| Binary | Raw data | Resource libraries |

## Object File Library Symbol Table Chunk (OFL_SYMT)

Object code libraries include an additional symbol table chunk that indexes external symbols defined by library members. This enables the linker to efficiently locate members that satisfy external references.

### Purpose

The external symbol table provides:
- **Fast lookup**: Direct mapping from symbol name to containing member
- **Selective loading**: Linker loads only members that define needed symbols
- **Dependency resolution**: Automatic inclusion of required modules

### Symbol Table Entry Format

Same structure as LIB_DIRY entries:

| Field | Size | Description |
|-------|------|-------------|
| ChunkIndex | 4 bytes | Index of LIB_DATA chunk containing member defining this symbol |
| EntryLength | 4 bytes | Size of this OFL_SYMT entry (integral number of words) |
| DataLength | 4 bytes | Size of symbol name + padding (integral number of words) |
| External Symbol Name | variable | NUL-terminated symbol name |
| Padding | 0-3 bytes | NULL bytes to word boundary |

### Key Differences from LIB_DIRY

- **No timestamp**: OFL_SYMT entries do not include time stamps
- **No additional info**: Only symbol name is present
- **Multiple entries per member**: One entry per exported symbol
- **Alphabetical order**: Typically sorted for binary search efficiency

### Symbol Entry Example

```
+-----------------------------+
| ChunkIndex = 5              |  Member defining printf
+-----------------------------+
| EntryLength = 16            |  Total size
+-----------------------------+
| DataLength = 12             |  Name + padding
+-----------------------------+
| "printf" + NUL + NUL + NUL  |  Symbol name (padded to 12 bytes)
+-----------------------------+
```

### Building the Symbol Table

1. **Extract symbols**: For each AOF member, identify exported symbols
2. **Create entries**: Map each symbol to its containing member's chunk index
3. **Sort**: Optionally sort by symbol name for faster lookup
4. **Update timestamp**: Record when symbol table was built

## Object File Library Symbol Table Time Stamp (OFL_TIME)

Records when the OFL_SYMT chunk was last modified.

### Format

Identical to LIB_TIME:

| Size | Description |
|------|-------------|
| 4 bytes | Timestamp word 1 (centiseconds high) |
| 4 bytes | Timestamp word 2 (centiseconds low + microseconds) |

### Usage

- **Consistency checking**: Verify symbol table is newer than member modifications
- **Rebuild detection**: Tools check if symbol table needs regeneration
- **Build systems**: Dependency tracking for makefiles

## Object Code Library Structure

A complete object code library contains all six chunk types in the typical order:

```
Chunk 0: LIB_DIRY - Directory of all members
Chunk 1: LIB_TIME - When library was last modified  
Chunk 2: LIB_VRSN - Version number (1)
Chunk 3: LIB_DATA - Member 1 (AOF file)
Chunk 4: LIB_DATA - Member 2 (AOF file)
...
Chunk N: LIB_DATA - Member N (AOF file)
Chunk N+1: OFL_SYMT - External symbol index
Chunk N+2: OFL_TIME - When symbol table was built
```

### Directory and Symbol Table Relationship

```
LIB_DIRY Entry 0: ChunkIndex=3 -> LIB_DATA 3 = "stdio.o"
LIB_DIRY Entry 1: ChunkIndex=4 -> LIB_DATA 4 = "stdlib.o"

OFL_SYMT Entry 0: ChunkIndex=3 -> Symbol "printf" in stdio.o
OFL_SYMT Entry 1: ChunkIndex=3 -> Symbol "scanf" in stdio.o
OFL_SYMT Entry 2: ChunkIndex=3 -> Symbol "fopen" in stdio.o
OFL_SYMT Entry 3: ChunkIndex=4 -> Symbol "malloc" in stdlib.o
OFL_SYMT Entry 4: ChunkIndex=4 -> Symbol "free" in stdlib.o
```

## C Structure Definitions

```c
/* Library format version */
#define ALF_VERSION             1

/* Library Directory Entry */
typedef struct {
    uint32_t chunk_index;         /* Index of LIB_DATA chunk */
    uint32_t entry_length;        /* Total entry size in bytes */
    uint32_t data_length;         /* Data section size in bytes */
    /* Followed by: name (NUL-terminated), info (optional), timestamp (8 bytes) */
} ALFDirEntry;

/* Library Time Stamp (also used for OFL_TIME) */
typedef struct {
    uint32_t centisecs_high;      /* Upper 4 bytes of centisecond count */
    uint32_t centisecs_low_usec;  /* Lower 2 bytes centisecs + 2 bytes usecs */
} ALFTimeStamp;

/* Macro to extract microseconds from timestamp word 2 */
#define ALF_GET_USECS(ts) ((ts).centisecs_low_usec & 0xFFFF)

/* Macro to extract low centiseconds from timestamp word 2 */
#define ALF_GET_CENTIS_LOW(ts) (((ts).centisecs_low_usec >> 16) & 0xFFFF)

/* External Symbol Table Entry (OFL_SYMT) */
typedef struct {
    uint32_t chunk_index;         /* Index of LIB_DATA chunk defining symbol */
    uint32_t entry_length;        /* Total entry size in bytes */
    uint32_t data_length;         /* Symbol name + padding size */
    /* Followed by: symbol name (NUL-terminated, padded) */
} ALFSymEntry;

/* Chunk File structures (same as AOF) */
#define CHUNK_FILE_MAGIC        0xC3CBC6C5

typedef struct {
    uint32_t chunk_file_id;       /* 0xC3CBC6C5 */
    uint32_t max_chunks;          /* Maximum directory entries */
    uint32_t num_chunks;          /* Used directory entries */
} ChunkFileHeader;

typedef struct {
    char     chunk_id[8];         /* 8-byte identifier (e.g., "LIB_DIRY") */
    uint32_t file_offset;         /* Offset to chunk data (4-byte aligned) */
    uint32_t size;                /* Size of chunk content */
} ChunkDirEntry;

/* Chunk ID values for ALF */
#define ALF_CHUNK_DIRY          "LIB_DIRY"
#define ALF_CHUNK_TIME          "LIB_TIME"
#define ALF_CHUNK_VRSN          "LIB_VRSN"
#define ALF_CHUNK_DATA          "LIB_DATA"
#define ALF_CHUNK_SYMT          "OFL_SYMT"
#define ALF_CHUNK_SYMT_TIME     "OFL_TIME"
```

## ALF Processing Guidelines

### Creating ALF Files

1. **Initialize chunk file**:
   - Set ChunkFileId = 0xC3CBC6C5
   - Allocate directory (typically 8-16 entries)
   - Reserve chunks 0-2 for LIB_DIRY, LIB_TIME, LIB_VRSN

2. **Add members**:
   - For each AOF file:
     - Create LIB_DATA chunk with file content
     - Add LIB_DIRY entry pointing to chunk
     - Record timestamp

3. **Build symbol table** (object libraries):
   - Scan all members for exported symbols
   - Create OFL_SYMT entries
   - Set OFL_TIME timestamp

4. **Finalize**:
   - Update LIB_TIME
   - Set all directory offsets and sizes

### Reading ALF Files

1. **Verify header**:
   - Check ChunkFileId
   - Determine endianness
   - Validate version

2. **Parse directory**:
   - Read LIB_DIRY chunk
   - Build member name-to-index map

3. **Access members**:
   - Lookup name in directory
   - Read LIB_DATA chunk via ChunkIndex

4. **Symbol resolution** (linkers):
   - Read OFL_SYMT chunk
   - Build symbol name-to-member map
   - Load members as symbols are referenced

### Modifying ALF Files

1. **Add member**:
   - Find unused directory entry (ChunkIndex = 0) or append
   - Create new LIB_DATA chunk
   - Update LIB_DIRY entry
   - Update LIB_TIME

2. **Remove member**:
   - Set LIB_DIRY ChunkIndex to 0
   - Note: LIB_DATA chunk not removed (fragmentation)
   - Update LIB_TIME

3. **Replace member**:
   - Remove old member
   - Add new member (may use different LIB_DATA chunk)
   - Update OFL_SYMT if symbol table present

4. **Rebuild symbol table**:
   - Required after any member modification
   - Rescan all members
   - Update OFL_SYMT and OFL_TIME

## Linker Library Processing

### Member Selection Algorithm

1. **Initial state**: No members loaded
2. **Process undefined symbols**:
   - For each undefined symbol in link:
     - Search OFL_SYMT for matching symbol
     - If found, load corresponding LIB_DATA member
     - Add member's exported symbols to defined set
     - Add member's undefined symbols to search set
3. **Repeat**: Continue until no new symbols can be resolved
4. **Result**: Minimal set of members required

### Symbol Resolution Example

```
Program references: printf, fopen

Library OFL_SYMT:
  printf -> Chunk 3 (stdio.o)
  fopen -> Chunk 3 (stdio.o)
  malloc -> Chunk 4 (stdlib.o)
  
Linker actions:
1. Look up "printf" -> Found in Chunk 3
2. Load LIB_DATA Chunk 3 (stdio.o)
3. stdio.o defines "printf" and "fopen", references "malloc"
4. "fopen" now satisfied
5. Look up "malloc" -> Found in Chunk 4
6. Load LIB_DATA Chunk 4 (stdlib.o)
7. stdio.o + stdlib.o loaded
```

## Version History and Compatibility

### Current Version (1)

- **Standard format**: Used by SDT 2.50 and later
- **Features**: Full symbol table support, timestamps
- **Compatibility**: Backward compatible with reading older versions

### Earlier Versions

Applications creating or reading ALF files should:

1. **Ensure valid timestamps**: LIB_DIRY entries should contain valid time stamps
2. **Graceful degradation**: Don't rely on data beyond name string unless space permits
3. **NULL padding**: Pad with 0 bytes, make no assumptions about padding values

## Best Practices

### For Library Creators

1. **Include symbol table**: Always build OFL_SYMT for object libraries
2. **Consistent endianness**: All members must match library endianness
3. **Meaningful names**: Use clear, consistent member naming conventions
4. **Update timestamps**: Maintain accurate modification times
5. **Compact libraries**: Periodically rebuild to eliminate fragmentation
6. **Version tracking**: Increment library version when format changes

### For Library Users (Linkers)

1. **Validate format**: Check magic numbers and version
2. **Handle missing symbols**: Provide meaningful error messages
3. **Selective loading**: Use OFL_SYMT to minimize loaded members
4. **Circular dependencies**: Detect and report circular symbol references
5. **Weak symbols**: Handle weak definitions correctly
6. **Multiple libraries**: Search libraries in specified order

### For Tools

1. **Preserve unknown chunks**: Forward-compatible with new chunk types
2. **Rebuild symbol tables**: Automatically regenerate when stale
3. **List utilities**: Display member names, sizes, and timestamps
4. **Extract functionality**: Allow extraction of individual members
5. **Consistency checks**: Verify all referenced chunks exist

## Common Operations

### Creating a Library

```bash
armlib -c library.alf file1.o file2.o file3.o
```

### Adding Members

```bash
armlib -a library.alf file4.o file5.o
```

### Listing Contents

```bash
armlib -t library.alf
```

### Extracting Members

```bash
armlib -x library.alf file1.o
```

### Deleting Members

```bash
armlib -d library.alf file2.o
```

### Rebuilding Symbol Table

```bash
armlib -s library.alf
```

## Error Handling

### Common Errors

| Error | Cause | Resolution |
|-------|-------|------------|
| Invalid magic | Corrupted file or wrong format | Verify file is valid ALF |
| Version mismatch | Created by newer/older tools | Rebuild with current tools |
| Missing chunk | Corrupted or truncated file | Restore from backup |
| Orphaned symbol | Symbol table references removed member | Rebuild symbol table |
| Endianness mismatch | Members have different byte order | Recreate library with consistent endianness |
| Directory full | Too many members | Rebuild with larger directory |

### Recovery Strategies

1. **Corrupted directory**: May be able to scan LIB_DATA chunks directly
2. **Missing symbol table**: Can be regenerated by scanning members
3. **Orphaned members**: Scan for LIB_DATA chunks not referenced by directory
4. **Timestamp mismatch**: Rebuild to synchronize timestamps

## Performance Considerations

### Directory Organization

- **Sequential search**: O(n) for directory lookup
- **Sorted directory**: Enables binary search O(log n)
- **Hash table**: External index for O(1) lookup (not in ALF format)

### Symbol Table Organization

- **Alphabetical sort**: Enables binary search
- **Hash index**: Some linkers build external hash for faster lookup
- **Bloom filter**: Can quickly determine symbol absence

### Memory Mapping

- **Direct access**: Memory-map ALF for zero-copy member access
- **Lazy loading**: Load members only when referenced
- **Caching**: Cache frequently accessed members

## Comparison with Other Archive Formats

| Feature | ALF | Unix ar | Windows LIB |
|---------|-----|---------|-------------|
| Chunk-based | Yes | No | No |
| Symbol index | Yes | Optional (ranlib) | Yes |
| Timestamps | Yes | Yes | Yes |
| Cross-platform | ARM-specific | Universal | Windows-specific |
| Member alignment | Word | 2-byte | 2-byte |
| Nested archives | Yes | No | No |
| Compression | No | No | No |

## Related Formats

- **AOF (ARM Object Format)**: Members contained in ALF
- **AIF (ARM Image Format)**: Output of linking ALF members
- **Chunk File Format**: Underlying container format

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- ARM DUI 0061: ARM Target Development System User Guide
- RISC OS Programmer's Reference Manuals

## Summary

ALF provides an efficient, well-structured archive format optimized for ARM development workflows. Its chunk-based design allows easy extension while maintaining compatibility. The built-in symbol table enables efficient selective linking, reducing executable size by including only required modules. Understanding ALF is essential for developing custom linkers, library management tools, and build systems targeting ARM platforms.

### Key Points

1. **Chunk-based structure**: Consistent with AOF, extensible design
2. **Indexed access**: Fast member lookup via directory
3. **Symbol indexing**: OFL_SYMT enables selective linking
4. **Timestamp tracking**: Build system integration
5. **Endianness awareness**: Matches target ARM system
6. **Object code focus**: Optimized for AOF member storage

ALF remains the standard library format for legacy ARM toolchains and RISC OS development, though modern ARM development has largely transitioned to ELF and standard archive formats.
