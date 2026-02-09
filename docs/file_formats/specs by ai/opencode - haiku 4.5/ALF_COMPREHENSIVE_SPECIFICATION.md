# ARM Library Format (ALF) - Comprehensive Specification

**Version:** 3.10  
**Last Updated:** February 2025  
**Based on:** ARM DUI 0041C, RISC OS PRMs

---

## Table of Contents

1. [Overview](#overview)
2. [Format Fundamentals](#format-fundamentals)
3. [Chunk File Structure](#chunk-file-structure)
4. [Library File Format](#library-file-format)
5. [Directory Chunk (LIB_DIRY)](#directory-chunk-lib_diry)
6. [Time Stamp Chunks](#time-stamp-chunks)
7. [Version Chunk (LIB_VRSN)](#version-chunk-lib_vrsn)
8. [Data Chunk (LIB_DATA)](#data-chunk-lib_data)
9. [Object Code Libraries](#object-code-libraries)
10. [Symbol Table Chunk (OFL_SYMT)](#symbol-table-chunk-ofl_symt)
11. [Library Operations](#library-operations)
12. [Reference Implementation](#reference-implementation)

---

## Overview

ARM Library Format (ALF) is the standard format for archiving multiple object files into a single library file. It combines the flexibility of the Chunk File Format with library-specific metadata to enable efficient storage and linkage of object code.

**Key Characteristics:**
- Efficient archival of multiple object modules
- Optional symbol table for fast lookup
- Timestamp tracking for incremental builds
- Support for both generic and object code libraries
- Fully compatible with ARM linker (link/armlink)
- Position of chunks irrelevant; random access fast

**Primary Producers:**
- ARM Object Librarian (arlib/armlib)
- Build systems and library creation tools
- Manual library construction

**Primary Consumers:**
- ARM Linker (link/armlink)
- Archive viewers and tools
- Library maintenance utilities

---

## Format Fundamentals

### Design Philosophy

ALF is built on the Chunk File Format, using it as a container for library metadata and member objects. This provides:

1. **Efficiency:** Only needed chunks accessed
2. **Extensibility:** New chunk types can be added
3. **Modularity:** Members are independent objects
4. **Fast Lookup:** Directory enables quick member access
5. **Compatibility:** Unknown chunks ignored gracefully

### Chunk File Format Review

ALF uses the standard Chunk File Format as its base:

**Header (12 bytes):**
```
Offset  Size    Field
------  ----    -----
0x00    4       ChunkFileId: 0xC3CBC6C5
0x04    4       max_chunks: Maximum number of chunk entries
0x08    4       num_chunks: Current number of chunks used
```

**Chunk Entries (16 bytes per chunk):**
```
Offset  Size    Field
------  ----    -----
0x00    8       chunkId: 8-byte identifier
0x08    4       file_offset: Byte offset in file
0x0C    4       size: Exact byte size of chunk
```

### Endianness Considerations

- ALF endianness must match target system
- Endianness detection via ChunkFileId value
- All words stored in target byte order
- Strings independent of byte order

**Detection:**
```
0xC3CBC6C5 (as read) = little-endian
0xC5C6CBC3 (as read) = big-endian
```

---

## Chunk File Structure

### Standard ALF Chunk Organization

**Required Chunks:**
- `LIB_DIRY`: Directory of library members
- `LIB_TIME`: Library modification timestamp
- `LIB_VRSN`: Version information
- `LIB_DATA`: At least one data chunk (member)

**Optional Chunks (for Object Code Libraries):**
- `OFL_SYMT`: External symbol table
- `OFL_TIME`: Symbol table timestamp

**Conventional Layout:**
```
Offset          Content
------          -------
0x00-0x2F       Chunk file header (12 bytes) + entry for 5 chunks
                Entries for LIB_DIRY, LIB_TIME, LIB_VRSN, OFL_SYMT, OFL_TIME

0x30+           LIB_DIRY chunk
                LIB_TIME chunk
                LIB_VRSN chunk
                LIB_DATA chunks (multiple)
                OFL_SYMT chunk (object libraries only)
                OFL_TIME chunk (object libraries only)
```

**Chunk Naming Convention:**

| Purpose | Prefix | Component |
|---------|--------|-----------|
| Generic Library | LIB_ | DIRY, TIME, VRSN, DATA |
| Object Library | OFL_ | SYMT, TIME |

---

## Library File Format

### Directory Chunk (LIB_DIRY)

**Purpose:** Index of all library members

**Structure:**
```
Variable-length entries, each describing one member

Entry Format (variable length, multiple of 4 bytes):
Offset  Size    Field
------  ----    -----
0x00    4       ChunkIndex
0x04    4       EntryLength
0x08    4       DataLength
0x0C+   var     Data (member info)
```

**Fixed Header per Entry:**

```c
struct {
    uint32_t chunk_index;       /* Zero-origin index of corresponding LIB_DATA chunk */
    uint32_t entry_length;      /* Total bytes in this entry (multiple of 4) */
    uint32_t data_length;       /* Bytes of actual data (variable content) */
    /* Followed by (data_length) bytes of data */
    /* Padded to entry_length with zeros if data_length < entry_length - 12 */
}
```

**ChunkIndex Field:**

- **0:** Directory entry is unused/deleted
- **Non-zero:** 1-origin index in chunk header (ChunkIndex+2 in 0-origin, accounting for DIRY and TIME chunks being typically first)

**EntryLength vs DataLength:**

```
EntryLength = 12 + data_length + padding
DataLength = actual content size (must be multiple of 4)
Padding = EntryLength - 12 - DataLength (must be ≥ 0)
```

**Entry Data Contents:**

```
Member Name (null-terminated string)
Additional info (often empty)
Timestamp (2 words, if present)
```

**Member Name:**
- Null-terminated string (e.g., "module1.o\0")
- Stored at beginning of data area
- Can be up to 32+ characters

**Timestamp (optional):**
- Last 8 bytes of data area (if present)
- Two 32-bit words encoding library time format
- Format described in [Time Stamps](#time-stamps)

### Alignment and Padding

**Padding Rules:**
1. Each directory entry is word-aligned (multiple of 4 bytes)
2. Data length specifies only used bytes (not padding)
3. All padding bytes must be 0x00
4. Readers should verify this for robustness

**Example:**

```
Entry Length: 20 bytes
String "module1.o" = 9 bytes
Null terminator = 1 byte
Timestamp = 8 bytes
Total data = 18 bytes
Padding = 20 - 12 - 18 = -10 bytes... ERROR

Actually:
Entry Length: 24 bytes
Data Length: 18 bytes
Padding: 24 - 12 - 18 = -6 bytes... Still error

Correct:
Entry Length: 32 bytes
Data Length: 20 bytes (string 9 + null 1 + timestamp 8 + 2 padding in data)
Padding: 32 - 12 - 20 = 0 bytes
```

### Version Chunk (LIB_VRSN)

**Purpose:** Encode library format version

**Content:** Single 32-bit word

```
uint32_t version = 1;
```

**Current Value:** Always 1 for ALF 3.10

**Future Versions:** If format changes incompatibly, version incremented

### Data Chunks (LIB_DATA)

**Purpose:** Store actual library member data

**Structure:**
```
One LIB_DATA chunk per library member

Content: Raw member data (usually an AOF file, but arbitrary)
Size: Specified in chunk header entry
Byte Order: Same as containing library file
```

**Member Organization:**

```
LIB_DATA[0]   ->  Member 0 (indexed as ChunkIndex=3 if after DIRY, TIME, VRSN)
LIB_DATA[1]   ->  Member 1
...
LIB_DATA[N]   ->  Member N
```

**Indexing:**
- ChunkIndex in directory = position in chunk header, 0-origin
- Typically: LIB_DIRY=0, LIB_TIME=1, LIB_VRSN=2, LIB_DATA[0]=3, LIB_DATA[1]=4, etc.

**Member Content Types:**

- **Object File:** AOF file (most common)
- **Sub-library:** Another ALF file
- **Binary:** Raw binary data
- **Custom:** Tool-specific format

The library treats members as opaque data; no interpretation is performed.

---

## Directory Chunk (LIB_DIRY)

### Detailed Format Specification

**Entry Sequence:**

```
Entry 0: 12 bytes header + data_length_0 bytes + padding_0
Entry 1: 12 bytes header + data_length_1 bytes + padding_1
...
Entry N: 12 bytes header + data_length_N bytes + padding_N
```

**Constraint:** Total directory chunk size = sum of all entry lengths

### Entry Field Definitions

#### ChunkIndex

**Value Range:**
- 0: Entry unused (deleted or placeholder)
- 1 to max_chunks: Points to corresponding chunk in file

**Interpretation:**
- Direct index into chunk header entry array (0-origin)
- Note: This is 1-origin in user documentation but 0-origin in implementation
- If value is 0, member has been deleted (space reclaimed)

#### EntryLength

**Value Range:** Must be multiple of 4

**Meaning:** Total bytes in this directory entry, including 12-byte header

**Constraint:** EntryLength ≥ 12 + data_length

#### DataLength

**Value Range:** Variable, typically 16-256 bytes per entry

**Meaning:** Actual bytes of data present (name, info, timestamp)

**Constraint:** Must be multiple of 4

### Data Area Contents

**Required:**
- Member name (null-terminated string, ISO-8859 non-control characters)

**Optional:**
- Descriptive information (tool-specific)
- Timestamp (2 words)
- Custom metadata

**String Constraints:**
- Only ISO-8859 non-control characters (0x20-0x7E, 0xA0-0xFF)
- NUL terminator required
- No embedded NULs

**Padding:**
- Fill unused bytes (from data_length to entry_length) with 0x00
- Readers must not rely on padding values beyond first NUL

### Example Directory Entry

**Member: "mymodule.o" linked on 2024-01-15**

```
Offset  Size    Content
------  ----    -------
0x00    4       ChunkIndex = 3 (points to LIB_DATA[0])
0x04    4       EntryLength = 28
0x08    4       DataLength = 20
0x0C    10      "mymodule.o\0"
0x16    4       Padding (0x00000000)
0x1A    8       Timestamp (2 words)
```

---

## Time Stamp Chunks

### Timestamp Format

**Structure:** Two 32-bit words (8 bytes total)

```
Word 0 (Most Significant):  Upper 4 bytes of centisecond count
Word 1 (Least Significant): Lower 2 bytes of centisecond count (upper half)
                           + 2 bytes of microsecond count (lower half)
```

**Encoding:**

```
Centiseconds since 00:00:00 UTC, January 1, 1900
= 6-byte value split across two 32-bit words

Word 0: MS 4 bytes of centisecond count
Word 1: LS 2 bytes of centisecond count (bits 16-31)
      : 2 bytes of microsecond offset (bits 0-15, usually 0)
```

**Examples:**

```c
/* January 1, 1900, 00:00:00.000000 */
uint32_t word0 = 0x00000000;
uint32_t word1 = 0x00000000;

/* January 1, 1900, 00:00:01.000000 (100 centiseconds) */
uint32_t word0 = 0x00000000;
uint32_t word1 = 0x00640000;  /* 100 in bits 16-31 */

/* January 1, 1970, 00:00:00 (2,208,988,800 centiseconds) */
uint32_t word0 = 0x83B9B580;
uint32_t word1 = 0x00000000;
```

### LIB_TIME Chunk

**Purpose:** Timestamp of library creation/modification

**Content:** Single 8-byte timestamp

```c
struct {
    uint32_t timestamp_high;
    uint32_t timestamp_low;
}
```

**Usage:**
- Records when library was last modified
- Useful for incremental builds
- Helps with caching in build systems

### OFL_TIME Chunk

**Purpose:** Timestamp of symbol table (OFL_SYMT) modification

**Content:** Single 8-byte timestamp (same format as LIB_TIME)

**Usage:**
- Tracks when symbol table was last updated
- Allows rebuilding symbol index if needed
- Distinct from library modification time

### Timestamp Comparison

**To determine if update needed:**

```c
if (ofl_time > cache_time) {
    /* Symbol table is newer than cached index */
    /* Rebuild or refresh cache */
}
```

---

## Version Chunk (LIB_VRSN)

### Purpose and Format

**Content:** Single 32-bit word

```
uint32_t version = 1;
```

**Current Value:** 1

**Future Compatibility:**
- If ALF format evolves, version incremented
- Readers can validate expected version
- Undefined behavior if version ≠ 1

### Version Field Interpretation

**Value 1:** Current ALF format (ARM DUI 0041C)

**Other Values:** Reserved for future formats

**Recommended Reader Behavior:**
```c
if (version != 1) {
    fprintf(stderr, "Unsupported ALF version: %u\n", version);
    return ERROR;
}
```

---

## Data Chunk (LIB_DATA)

### General Structure

**Purpose:** Store member data (usually AOF object files)

**Constraints:**
- Each member in separate LIB_DATA chunk
- Indexed by ChunkIndex in directory
- Opaque to library manager; no interpretation
- Byte order must match containing file

### Member Type Handling

**For Object Code Members (AOF files):**
- Endianness must match library file
- Can be mixed in same library only if endianness identical
- Linker verifies and rejects mixed endianness

**For Other Member Types:**
- Binary data treated as-is
- No validation by library tools
- May require tool-specific knowledge

### Member Size Considerations

**Typical Sizes:**
- Small object: 1-10 KB
- Medium object: 10-100 KB
- Large object: 100 KB - 1 MB
- Very large: > 1 MB (rare, better as separate files)

---

## Object Code Libraries

### Extension for Object Code

**For libraries containing AOF object files, two additional chunks:**

1. **OFL_SYMT:** External symbol table (cross-library references)
2. **OFL_TIME:** Symbol table modification timestamp

### Symbol Table Chunk (OFL_SYMT)

**Purpose:** Fast lookup of symbols exported by library members

**Format:** Similar to LIB_DIRY but for symbols

```c
struct SymbolEntry {
    uint32_t chunk_index;       /* Chunk containing definition */
    uint32_t entry_length;      /* Total entry size */
    uint32_t data_length;       /* Data bytes (must be multiple of 4) */
    char     symbol_name[];     /* Null-terminated symbol name */
    uint8_t  padding[];         /* Zeros to align to 4-byte boundary */
}
```

**Key Differences from LIB_DIRY:**

| Feature | LIB_DIRY | OFL_SYMT |
|---------|----------|----------|
| Purpose | Index members | Index symbols |
| ChunkIndex | Points to LIB_DATA | Points to AOF containing symbol |
| Time stamps | Optional | Not included |
| Symbol info | Member name | External symbol name |

**ChunkIndex Interpretation:**
- Points to LIB_DATA chunk containing AOF that exports this symbol
- Linker loads only needed members during linking

### Symbol Entry Details

**Entry Structure:**

```
Field           Size    Meaning
-----           ----    -------
chunk_index     4       Index in chunk header of containing member
entry_length    4       Total bytes in this entry (multiple of 4)
data_length     4       Bytes of name + padding
symbol_name     var     Null-terminated symbol name
padding         var     Zeros to entry_length
```

**Name Field:**
- Null-terminated ASCII string
- No timestamp (unlike LIB_DIRY)
- Actual name length = strlen(symbol_name) + 1

### Symbol Lookup Example

```c
/* To find where symbol "main" is defined: */
1. Read OFL_SYMT chunk
2. Iterate through entries
3. For each entry:
   - Read symbol_name
   - Compare with "main"
   - If match, chunk_index tells which member to load
4. Load appropriate member (LIB_DATA[chunk_index - offset])
5. Linker resolves "main" from that object file
```

**Benefits:**
- Linker doesn't need to load all members
- Only pulls in modules providing needed symbols
- Faster linking of large libraries

---

## Library Operations

### Creating a Library

**Steps:**

1. **Prepare object files:** Compile/assemble source modules
2. **Create directory:**
   - List each member
   - Assign index to corresponding LIB_DATA chunk
   - Create directory entries with names and timestamps
3. **Organize chunks:**
   - Chunk header with entries for: LIB_DIRY, LIB_TIME, LIB_VRSN, LIB_DATA[0..N]
   - Optional: OFL_SYMT, OFL_TIME for object libraries
4. **Write file:**
   - Chunk header
   - All chunks in any order
   - Library tool typically places metadata first

### Adding/Removing Members

**Adding:**
1. Allocate new LIB_DATA chunk in chunk header
2. Write member data
3. Add directory entry with ChunkIndex pointing to new chunk
4. Update timestamps

**Removing:**
1. Set directory ChunkIndex to 0 (marks unused)
2. Leave data chunk in place (wastes space) or reorganize file
3. Update timestamps

**Note:** Many implementations don't actually reclaim space; just mark unused

### Updating Symbol Table (Object Libraries)

**After adding/removing members:**

1. Scan all AOF members
2. Extract exported symbols (global definitions)
3. Build symbol table (OFL_SYMT)
4. Update symbol table timestamp (OFL_TIME)
5. Write new OFL_SYMT and OFL_TIME chunks

---

## Reference Implementation

### C Structure Definitions

```c
/* ALF Library Header Structure */
typedef struct {
    uint32_t chunk_file_id;
    uint32_t max_chunks;
    uint32_t num_chunks;
} ALFHeader;

/* Chunk Entry */
typedef struct {
    char     chunk_id[8];
    uint32_t file_offset;
    uint32_t size;
} ChunkEntry;

/* Directory Entry (variable size) */
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    /* Followed by data_length bytes of:
       - Member name (null-terminated string)
       - Optional info
       - Optional 8-byte timestamp
       - Padding zeros to entry_length
    */
} DirectoryEntry;

/* Timestamp Structure */
typedef struct {
    uint32_t centiseconds_high;
    uint32_t centiseconds_low;  /* In bits 16-31; microseconds in bits 0-15 */
} TimeStamp;

/* Symbol Table Entry (for OFL_SYMT) */
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    /* Followed by data_length bytes of:
       - Symbol name (null-terminated string)
       - Padding zeros to entry_length
    */
} SymbolTableEntry;
```

### Library Reading Function

```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    FILE *file;
    ALFHeader header;
    ChunkEntry *entries;
} ALFLibrary;

int alf_open(const char *filename, ALFLibrary *lib) {
    lib->file = fopen(filename, "rb");
    if (!lib->file) return -1;
    
    /* Read header */
    if (fread(&lib->header, sizeof(ALFHeader), 1, lib->file) != 1) {
        fclose(lib->file);
        return -1;
    }
    
    /* Read chunk entries */
    lib->entries = malloc(lib->header.max_chunks * sizeof(ChunkEntry));
    if (!lib->entries) {
        fclose(lib->file);
        return -1;
    }
    
    if (fread(lib->entries, sizeof(ChunkEntry),
              lib->header.num_chunks, lib->file) != lib->header.num_chunks) {
        free(lib->entries);
        fclose(lib->file);
        return -1;
    }
    
    return 0;
}

ChunkEntry* alf_find_chunk(ALFLibrary *lib, const char *chunk_id) {
    for (uint32_t i = 0; i < lib->header.num_chunks; i++) {
        if (strncmp(lib->entries[i].chunk_id, chunk_id, 8) == 0) {
            return &lib->entries[i];
        }
    }
    return NULL;
}

/* Read entire chunk into memory */
int alf_read_chunk(ALFLibrary *lib, const char *chunk_id, void **data) {
    ChunkEntry *chunk = alf_find_chunk(lib, chunk_id);
    if (!chunk) return -1;
    
    *data = malloc(chunk->size);
    if (!*data) return -1;
    
    if (fseek(lib->file, chunk->file_offset, SEEK_SET) != 0) {
        free(*data);
        return -1;
    }
    
    if (fread(*data, chunk->size, 1, lib->file) != 1) {
        free(*data);
        return -1;
    }
    
    return 0;
}

/* Extract member name from directory entry */
const char* alf_member_name(DirectoryEntry *entry) {
    /* Member name is immediately after the header */
    return (const char*)(entry + 1);
}

/* List all library members */
int alf_list_members(ALFLibrary *lib) {
    void *diry_data;
    if (alf_read_chunk(lib, "LIB_DIRY", &diry_data) != 0) {
        return -1;
    }
    
    DirectoryEntry *entry = (DirectoryEntry *)diry_data;
    uint32_t offset = 0;
    
    while (offset < ((ChunkEntry *)diry_data)->size) {
        if (entry->chunk_index != 0) {
            printf("Member: %s\n", alf_member_name(entry));
        }
        offset += entry->entry_length;
        entry = (DirectoryEntry *)((uint8_t *)entry + entry->entry_length);
    }
    
    free(diry_data);
    return 0;
}

void alf_close(ALFLibrary *lib) {
    if (lib->entries) free(lib->entries);
    if (lib->file) fclose(lib->file);
}
```

---

## Compatibility and Robustness

### Handling Older Format Versions

**Backward Compatibility:**
- ALF v1.0 readers should validate version field
- Unknown chunks should be safely ignored
- Directory entries beyond declared size should be ignored

**Forward Compatibility:**
- New chunk types can be added without breaking old readers
- Old readers skip unknown chunks
- Chunk order irrelevant; fast random access via header

### String Table Handling

**In Directory Entries:**
1. Read data_length bytes
2. Look for first NUL character
3. Use as member name
4. Ignore padding bytes beyond first NUL

**Robustness:**
- Don't assume well-formed strings
- Check for NUL within data_length bytes
- Handle missing NUL gracefully (truncate at data_length)

### Timestamp Validation

**When reading timestamps:**
1. Read both words
2. Interpret as 6-byte centisecond count
3. Handle overflow gracefully
4. Reasonable range: 1900-2200+ (covers any real archive)

---

## Use Cases

### Build System Integration

**Incremental Linking:**

```
1. Check LIB_TIME vs build cache time
2. If library unchanged, use cached link
3. If library updated, rebuild link
```

**Symbol Tracking:**

```
1. Extract OFL_SYMT from library
2. Build dependency graph
3. Only link needed members
4. Reduce link time and binary size
```

### Library Maintenance

**Adding to Library:**
```
arlib mylibrary.a mymodule.o newmodule.o
```

**Listing Members:**
```
arlib -t mylibrary.a
```

**Extracting Member:**
```
arlib -x mylibrary.a mymodule.o
```

### Development Workflow

**Create library from objects:**
```bash
cc module1.c -c -o module1.o
cc module2.c -c -o module2.o
arlib -cr mylib.a module1.o module2.o
```

**Link against library:**
```bash
cc main.c -c -o main.o
link -o program main.o mylib.a
```

---

## Summary

ARM Library Format provides an efficient, extensible container for object file libraries. Its layering on Chunk File Format enables fast random access while maintaining simplicity.

**Key Strengths:**
- Efficient archival of object modules
- Optional symbol index for fast lookup
- Version-independent processing
- Forward/backward compatible
- Tool-neutral design
- Supports mixed object types

**Key Characteristics:**
- Fixed metadata (directory, timestamp, version)
- Flexible member count and size
- Robust handling of additions/removals
- No size limits on members or libraries
- Suitable for very large libraries

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals, Appendix D
- ARM Library File Format Specification
- ARM Linker User Guide
- ARM Object Librarian (arlib) Documentation
