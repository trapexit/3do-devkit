# ARM Object Library Format (ALF) - Complete File Format Specification

**Version:** 1.00+
**Last Updated:** 2026-02-07
**Based on:** ARM DUI 0041C, RISC OS Programmer's Reference Manuals

---

## Table of Contents

1. [Overview](#overview)
2. [Library File Format](#library-file-format)
3. [Chunk File Foundation](#chunk-file-foundation)
4. [Required Chunks](#required-chunks)
5. [Directory Chunk (LIB_DIRY)](#directory-chunk-libdiry)
6. [Time Stamp Chunk (LIB_TIME)](#time-stamp-chunk-libtime)
7. [Version Chunk (LIB_VRSN)](#version-chunk-libvrsn)
8. [Data Chunk (LIB_DATA)](#data-chunk-libdata)
9. [Object Library Extensions](#object-library-extensions)
10. [C Structure Definitions](#c-structure-definitions)

---

## Overview

**ARM Object Library Format (ALF)** is a standardized archive format for managing collections of object files. ALF libraries are created from one or more ARM Object Format (AOF) files using the ARM librarian tool (typically called `LibFile` or `armar`).

### Key Characteristics

- **Modular Storage:** Multiple independent object files in one container
- **Efficient Access:** Directory chunk enables quick member lookup
- **Time Tracking:** Records when library was last modified
- **Extensibility:** Additional chunks can be added
- **Chunk-Based:** Built on same foundation as AOF files

### Typical Usage Pipeline

```
AOF Object Files
    ↓
LibFile/armar Tool
    ↓
ALF Library File ← (This specification)
    ↓
Linker
    ↓
Executable Image (AIF)
```

### Library vs Compiler

**Standard Library:**
- Container for multiple related object modules
- Linker pulls in only needed members
- Typical content: utility functions, standard library code

**Object Library (OFL):**
- Library with symbol table index
- Faster member resolution
- OFL_SYMT and OFL_TIME chunks added
- See Object Library Extensions section

---

## Library File Format

### Overall Structure

An ALF library file is structured as a chunk file (same format as AOF) with specialized chunks:

```
Byte 0          Chunk File Header
                ├─ ChunkFileId
                ├─ max_chunks
                └─ num_chunks

                Chunk Directory (one per chunk)

                LIB_DIRY chunk (Directory)
                LIB_TIME chunk (Last modified time)
                LIB_VRSN chunk (Version = 1)
                LIB_DATA chunk (First member)
                LIB_DATA chunk (Second member)
                ... (more LIB_DATA chunks)
                LIB_DATA chunk (Last member)

                [Optional OFL extensions for object libraries]
```

### Chunk Order Convention

Standard ALF libraries have chunks in this order:
1. **Chunk File Header** (at offset 0x00)
2. **LIB_DIRY** - Directory of members
3. **LIB_TIME** - Library modification time
4. **LIB_VRSN** - Version number (always 1)
5. **LIB_DATA** - First member
6. **LIB_DATA** - Second member
7. ... (additional members)
8. **LIB_DATA** - Last member

**Other Chunks (for Object Libraries):**
- OFL_SYMT - External symbol table
- OFL_TIME - Symbol table modification time

### Endianness

- All numeric values use target system byte order
- String data order is independent of endianness
- All chunks must have same endianness as chunk file header

---

## Chunk File Foundation

### Chunk File Structure

ALF uses the same chunk file format as AOF (see AOF specification for detailed chunk file format).

**Quick Reference:**

| Offset | Field | Size | Value |
|--------|-------|------|-------|
| 0x00 | ChunkFileId | 4 | 0xC3CBC6C5 |
| 0x04 | max_chunks | 4 | 8 (typical) |
| 0x08 | num_chunks | 4 | 3+ (header + time + version + members) |

**Library-Specific Chunk IDs:**

| Chunk | Name | Purpose |
|-------|------|---------|
| LIB_DIRY | Directory | Directory of all members |
| LIB_TIME | Time stamp | When library was last modified |
| LIB_VRSN | Version | Library format version (always 1) |
| LIB_DATA | Data | One library member per chunk |

**Chunk ID Format:**
- First 4 bytes: `LIB_` (0x4C49425F)
- Next 4 bytes: Component name (DIRY, TIME, VRSN, DATA)
- Total: 8 bytes, stored in ascending address order

### Chunk Directory Layout

```
Offset 0x00:    Chunk File Header (12 bytes)
Offset 0x0C:    Directory Entry 0 (16 bytes)
                - chunk_id: "LIB_DIRY"
                - file_offset: offset to LIB_DIRY
                - size: size of LIB_DIRY

Offset 0x1C:    Directory Entry 1 (16 bytes)
                - chunk_id: "LIB_TIME"
                - file_offset: offset to LIB_TIME
                - size: 8 bytes

Offset 0x2C:    Directory Entry 2 (16 bytes)
                - chunk_id: "LIB_VRSN"
                - file_offset: offset to LIB_VRSN
                - size: 4 bytes

Offset 0x3C:    Directory Entry 3+ (16 bytes each)
                - chunk_id: "LIB_DATA"
                - file_offset: offset to member
                - size: size of member
                (one entry per member)
```

---

## Required Chunks

### Minimum Library Requirements

Every ALF library **MUST** contain:
1. **LIB_DIRY** - Directory chunk
2. **LIB_TIME** - Time stamp chunk
3. **LIB_VRSN** - Version chunk
4. At least one **LIB_DATA** - Member data

### Optional Elements

- **OFL_SYMT** - Symbol table (for object libraries)
- **OFL_TIME** - Symbol table time (for object libraries)
- Additional custom chunks (not standard)

---

## Directory Chunk (LIB_DIRY)

### Purpose

The directory chunk contains a listing of all members in the library, enabling quick lookup and access to specific members.

### Overall Structure

The directory is a sequence of variable-length entries, each an integral number of words (4-byte aligned).

```
Directory Entry 0
├─ Fixed fields (ChunkIndex, EntryLength, DataLength)
└─ Data section (member name, metadata, time stamp)

Directory Entry 1
├─ Fixed fields
└─ Data section

... (one entry per member)
```

### Directory Entry Format

Each entry has a fixed part (3 words) followed by variable-length data:

| Offset | Field | Type | Size | Description |
|--------|-------|------|------|-------------|
| +0 | ChunkIndex | Word | 4 | Chunk file directory index of member |
| +4 | EntryLength | Word | 4 | Total bytes in this entry (multiple of 4) |
| +8 | DataLength | Word | 4 | Bytes used in data section (multiple of 4) |
| +12 | Data | Variable | varies | Member name + metadata + time stamp |

### Fixed Field Details

**ChunkIndex (offset +0):**
- Index into chunk file header of corresponding LIB_DATA chunk
- Typically 3 or higher (first three chunks are DIRY, TIME, VRSN)
- Value 0: Entry is unused/deleted
- Index N: Corresponds to Nth entry in chunk directory

**EntryLength (offset +4):**
- Total size of this directory entry in bytes
- Always multiple of 4 (4-byte aligned)
- Includes fixed part (12 bytes) + data section
- Used to locate next entry: next_entry = current + EntryLength

**DataLength (offset +8):**
- Bytes actually used in data section
- Multiple of 4
- Can be less than (EntryLength - 12) due to padding
- Includes name string, metadata, and time stamp

### Data Section

The data section contains (in order):

1. **Member Name String**
   - Null-terminated string
   - ISO-8859 non-control characters
   - Immediately follows fixed header (offset +12)

2. **Optional Metadata**
   - Purpose-defined by application
   - Often empty
   - Between name string end and time stamp

3. **Time Stamp (2 words)**
   - 8 bytes (2×4 bytes)
   - Records when member was added/modified
   - Word-aligned
   - Last part of data section

### Time Stamp Format (in Data Section)

The directory entry time stamp is at the end of the data section:

| Offset | Field | Description |
|--------|-------|-------------|
| -8 | First Word | High 4 bytes of 6-byte centisecond count |
| -4 | Second Word | Low 2 bytes of centisecond count + microseconds |

**Detailed Breakdown (Second Word):**
```
Bits 0-15:   Microseconds since last centisecond (usually 0)
Bits 16-31:  Low 2 bytes of centisecond count
```

Time base: Centiseconds since 00:00:00 1st January 1900 (Unix-like but different epoch)

### Directory Entry Example

```
Offset      Hex Value   Description
────────────────────────────────────────────────────
0x00        00000004    ChunkIndex = 4 (fourth directory entry = first data)
0x04        00000020    EntryLength = 32 bytes (includes this entry)
0x08        00000018    DataLength = 24 bytes (name + time stamp)
0x0C        6C 69 62 31 "lib1" (null-terminated member name)
            00 00 00 00
0x14        (metadata:  0-4 bytes of optional extra data)
            00 00 00 00
0x18        01 23 45 67 Time stamp (first word, centiseconds)
0x1C        89 AB 00 00 Time stamp (second word, microseconds)

Next Entry at 0x00 + 0x20 = 0x20
```

### Multiple Entries Example

```
Directory Offset 0x00:
  ChunkIndex = 4, EntryLength = 32, DataLength = 24
  Name: "vector.o" (8 chars + null = 9 bytes)
  Padding: 7 bytes
  Time: 2 words

Directory Offset 0x20:
  ChunkIndex = 5, EntryLength = 28, DataLength = 20
  Name: "math.o" (6 chars + null = 7 bytes)
  Padding: 9 bytes
  Time: 2 words

Directory Offset 0x38:
  ChunkIndex = 6, EntryLength = 24, DataLength = 16
  Name: "io.o" (4 chars + null = 5 bytes)
  Padding: 7 bytes
  Time: 2 words

... and so on
```

### Unused/Deleted Entries

An entry can be marked as unused (for "deletion"):
- Set ChunkIndex = 0
- EntryLength and DataLength still valid
- Entry is skipped by linker
- Space can be reused on re-library

### Directory Size Calculation

Total directory size depends on:
- Number of members
- Length of member names
- Amount of optional metadata
- Padding to 4-byte alignment

**Minimum entry:** 12 (fixed) + 5 (name "a.o") + 8 (time) = 25 → 28 bytes (padded to 4-byte)
**Typical entry:** 12 + 20 (name) + 8 (time) = 40 bytes
**Maximum entry:** Depends on longest member name

---

## Time Stamp Chunk (LIB_TIME)

### Purpose

Records when the library file was last modified.

### Format

Exactly **8 bytes (2 words)**:

| Offset | Field | Description |
|--------|-------|-------------|
| 0 | First Word | High 4 bytes of centisecond count |
| 4 | Second Word | Low 2 bytes of centisecond count + microseconds |

### Time Encoding

**First Word (bits 0-31):**
```
Most significant 4 bytes of 6-byte centisecond count
Valid range: 0x00000000 to 0xFFFFFFFF
```

**Second Word:**
```
Bits 0-15:    Microseconds since last centisecond (usually 0)
Bits 16-31:   Least significant 2 bytes of centisecond count
```

### Time Base

- **Epoch:** 00:00:00 1st January 1900
- **Units:** Centiseconds (0.01 second precision)
- **Range:** 6 bytes = 48 bits, covers ~8,921 years

### Byte Order

- Stored in target system byte order
- Must match chunk file endianness
- If chunk file is little-endian, time is little-endian

### Example Time Values

```
1900-01-01 00:00:00 → 0x00000000 0x00000000
1970-01-01 00:00:00 → 0x002CDF55 0x0EE30000 (Unix epoch)
2000-01-01 00:00:00 → 0x00F71CE0 0x14150000
2020-01-01 00:00:00 → 0x01B2F234 0x9AA10000
```

### Purpose in Library

Records when the library was created or last updated. Useful for:
- Determining if cached linker data is stale
- Comparing against member times
- Tracking library modification history

---

## Version Chunk (LIB_VRSN)

### Purpose

Identifies the version of the ALF library format used.

### Format

Exactly **4 bytes (1 word)**:

| Offset | Field | Type | Value |
|--------|-------|------|-------|
| 0 | Version | Word | 1 |

### Version Information

**Current Version:** 1
- First and only version documented
- Any other value is reserved
- Future versions would use higher numbers

**Interpretation:**
- Version 1 = ALF format as specified in this document
- Unknown versions should be rejected by tools

### Future Extensibility

If ALF format changes:
- New version number introduced (e.g., 2)
- Old tools can detect incompatible version
- New tools can handle multiple versions

---

## Data Chunk (LIB_DATA)

### Purpose

Contains the actual member data (typically an AOF object file).

### Format

Binary data without interpretation by ALF format:

```
Offset 0x00:    AOF file or other member data
                (exactly as stored, no processing)

Size:           As specified in chunk directory
```

### Member Content Rules

**Typical Member:** Complete AOF object file
- Valid chunk file format
- Contains OBJ_HEAD, OBJ_AREA, etc.
- Can be linked by ARM linker

**Alternative Members:**
- Could be another ALF library (nested)
- Could be data files (unlikely in practice)
- Interpretation left to application

### Endianness

- Member endianness must match library endianness
- Linker will verify compatibility
- Error if endianness mismatch detected

### Member Retrieval

To extract a member:

1. Look up member name in LIB_DIRY
2. Get ChunkIndex from directory entry
3. Find chunk in chunk file directory
4. Read file_offset and size from chunk directory
5. Read `size` bytes from file at `file_offset`
6. Resulting data is valid member

### Example Member Extraction

```c
// Given library file and member name "main.o"
// 1. Search LIB_DIRY for "main.o"
// Found at offset 0x100 in LIB_DIRY
// ChunkIndex = 5

// 2. Look up chunk directory entry 5
// Chunk 5 at offset 0x3C in chunk file header
// file_offset = 0x1000, size = 0x800

// 3. Read bytes from file[0x1000:0x1800]
// Result: Complete AOF object file "main.o"
```

---

## Object Library Extensions

### Object File Library (OFL)

An **Object File Library** is an ALF library optimized for object code with additional chunks:

- **OFL_SYMT** - External symbol table index
- **OFL_TIME** - Symbol table modification time

### OFL_SYMT Chunk (Symbol Table)

**Purpose:** Fast lookup of symbols defined in library members

**Format:** Similar to LIB_DIRY but for symbols

| Field | Description |
|-------|-------------|
| ChunkIndex | Index of chunk containing symbol definition |
| EntryLength | Total entry size (multiple of 4) |
| DataLength | Data section size (multiple of 4) |
| Data | Symbol name string + padding |

**Differences from LIB_DIRY:**
- Symbol name instead of member name
- No time stamp in data section
- Padding to maintain 4-byte alignment
- ChunkIndex points to LIB_DATA containing the symbol definition

### OFL_TIME Chunk

**Purpose:** Records when OFL_SYMT was last updated

**Format:** Same as LIB_TIME (8 bytes)

### Symbol Table Building

Linker can build symbol table by:
1. Reading OFL_SYMT chunk
2. Looking up symbol in table
3. Getting ChunkIndex
4. Loading only necessary members from library

**Benefits:**
- Faster linking (no need to scan all members)
- Reduced symbol resolution time
- Better performance on large libraries

### Creating Object Libraries

Standard ALF library can be converted to OFL:

1. Scan all member AOF files
2. Extract external symbols from OBJ_SYMT chunks
3. Build OFL_SYMT with all symbols
4. Add OFL_TIME chunk
5. Write back to library file

---

## C Structure Definitions

### Library Directory Entry

```c
#ifndef ALF_LIBRARY_H
#define ALF_LIBRARY_H

#include <stdint.h>

/* Library directory entry (variable-length, 4-byte aligned) */
typedef struct {
    uint32_t chunk_index;       /* Chunk directory index of member */
    uint32_t entry_length;      /* Total bytes in this entry (multiple of 4) */
    uint32_t data_length;       /* Bytes in data section (multiple of 4) */
    /* Data section follows:
     * - Member name (null-terminated string)
     * - Optional metadata
     * - Time stamp (2 words, 8 bytes)
     */
} ALFDirectoryEntry;

/* Time stamp structure - appears at end of directory entry data */
typedef struct {
    uint32_t time_high;         /* High 4 bytes of centisecond count */
    uint32_t time_low;          /* Low 2 bytes of cs + microseconds */
} ALFTimeStamp;

_Static_assert(offsetof(ALFTimeStamp, time_low) == 4, "Time stamp layout");

/* Library header chunks */
typedef struct {
    uint32_t time_high;         /* High bytes of modification time */
    uint32_t time_low;          /* Low bytes + microseconds */
} ALFTimeChunk;

_Static_assert(sizeof(ALFTimeChunk) == 8, "Time chunk must be 8 bytes");

typedef struct {
    uint32_t version;           /* Version = 1 */
} ALFVersionChunk;

_Static_assert(sizeof(ALFVersionChunk) == 4, "Version chunk must be 4 bytes");

/* Symbol table entry (for OFL libraries) */
typedef struct {
    uint32_t chunk_index;       /* Chunk containing symbol definition */
    uint32_t entry_length;      /* Total entry size (multiple of 4) */
    uint32_t data_length;       /* Data section size (multiple of 4) */
    /* Data section:
     * - Symbol name (null-terminated)
     * - Padding to 4-byte alignment
     */
} ALFSymbolEntry;

/* Helper functions */

/* Calculate next directory entry offset */
static inline uint32_t alf_next_entry(uint32_t current_offset, uint32_t entry_length) {
    return current_offset + entry_length;
}

/* Get pointer to data in directory entry */
static inline const char* alf_entry_data(const ALFDirectoryEntry *entry) {
    return (const char*)(entry + 1);  /* Data follows fixed part */
}

/* Get time stamp from end of data section */
static inline const ALFTimeStamp* alf_entry_timestamp(
    const ALFDirectoryEntry *entry, const char *data) {
    return (const ALFTimeStamp*)(data + entry->data_length - sizeof(ALFTimeStamp));
}

#endif /* ALF_LIBRARY_H */
```

### Library File Operations

```c
/* Example: Search directory for member name */
int alf_find_member(const void *lib_file, const void *diry_chunk,
                    uint32_t diry_size, const char *member_name,
                    uint32_t *out_chunk_index) {
    const ALFDirectoryEntry *entry = (const ALFDirectoryEntry *)diry_chunk;
    uint32_t offset = 0;

    while (offset < diry_size) {
        if (entry->chunk_index != 0) {  /* Check if entry is used */
            const char *data = alf_entry_data(entry);
            if (strcmp(data, member_name) == 0) {
                *out_chunk_index = entry->chunk_index;
                return 0;  /* Found */
            }
        }

        offset += entry->entry_length;
        entry = (const ALFDirectoryEntry *)((char*)diry_chunk + offset);
    }

    return -1;  /* Not found */
}

/* Example: Extract member from library */
int alf_extract_member(const void *lib_file, const void *chunk_dir,
                       uint32_t chunk_index,
                       void **out_member, uint32_t *out_size) {
    const ChunkDirectoryEntry *chunk = get_chunk_entry(chunk_dir, chunk_index);
    if (!chunk) return -1;

    *out_member = (void*)((char*)lib_file + chunk->file_offset);
    *out_size = chunk->size;
    return 0;
}
```

---

## Library Creation Example

### Creating a New Library

```c
int alf_create_library(const char *lib_filename,
                       const char **member_names,
                       const void **member_data,
                       const uint32_t *member_sizes,
                       uint32_t num_members) {
    FILE *out = fopen(lib_filename, "wb");
    if (!out) return -1;

    /* 1. Write chunk file header */
    ChunkFileHeader chunk_hdr = {
        .chunk_file_id = 0xC3CBC6C5,
        .max_chunks = num_members + 4,  /* +4 for DIRY, TIME, VRSN, DATA... */
        .num_chunks = num_members + 3
    };
    fwrite(&chunk_hdr, sizeof(chunk_hdr), 1, out);

    /* 2. Build chunk directory entries */
    /* Calculate offsets for all chunks */
    uint32_t chunk_dir_size = (num_members + 3) * 16;
    uint32_t diry_offset = 0x0C + chunk_dir_size;
    uint32_t time_offset = diry_offset + calculate_diry_size(...);
    uint32_t vrsn_offset = time_offset + 8;
    uint32_t data_offset = vrsn_offset + 4;

    /* 3. Write chunk directory */
    ChunkDirectoryEntry diry_entry = {
        .chunk_id = {'L', 'I', 'B', '_', 'D', 'I', 'R', 'Y'},
        .file_offset = diry_offset,
        .size = diry_size
    };
    /* ... similar for TIME and VRSN ... */

    /* 4. Write LIB_DIRY chunk */
    /* Build directory from member info */

    /* 5. Write LIB_TIME chunk */
    ALFTimeChunk time_chunk = {
        .time_high = get_current_time_high(),
        .time_low = get_current_time_low()
    };

    /* 6. Write LIB_VRSN chunk */
    uint32_t version = 1;
    fwrite(&version, 4, 1, out);

    /* 7. Write member data (LIB_DATA chunks) */
    for (int i = 0; i < num_members; i++) {
        fwrite(member_data[i], member_sizes[i], 1, out);
    }

    fclose(out);
    return 0;
}
```

---

## Library Processing Example

### Scanning Library Members

```c
typedef struct {
    const char *name;
    const void *data;
    uint32_t size;
} ALFMember;

int alf_scan_members(FILE *lib_file, ALFMember **out_members,
                     uint32_t *out_count) {
    /* 1. Read chunk file header */
    ChunkFileHeader chunk_hdr;
    fread(&chunk_hdr, sizeof(chunk_hdr), 1, lib_file);

    /* 2. Read chunk directory */
    ChunkDirectoryEntry chunk_dir[chunk_hdr.num_chunks];
    for (int i = 0; i < chunk_hdr.num_chunks; i++) {
        fread(&chunk_dir[i], 16, 1, lib_file);
    }

    /* 3. Find LIB_DIRY chunk */
    ChunkDirectoryEntry *diry_entry = NULL;
    for (int i = 0; i < chunk_hdr.num_chunks; i++) {
        if (strncmp(chunk_dir[i].chunk_id, "LIB_DIRY", 8) == 0) {
            diry_entry = &chunk_dir[i];
            break;
        }
    }

    if (!diry_entry) return -1;

    /* 4. Read directory chunk */
    fseek(lib_file, diry_entry->file_offset, SEEK_SET);
    void *diry_data = malloc(diry_entry->size);
    fread(diry_data, diry_entry->size, 1, lib_file);

    /* 5. Parse directory entries */
    ALFMember *members = malloc(sizeof(ALFMember) * 256);
    uint32_t member_count = 0;

    uint32_t offset = 0;
    while (offset < diry_entry->size) {
        ALFDirectoryEntry *entry = (ALFDirectoryEntry*)((char*)diry_data + offset);

        if (entry->chunk_index != 0) {  /* Skip deleted entries */
            /* Find member data chunk */
            ChunkDirectoryEntry *member_chunk = &chunk_dir[entry->chunk_index];

            members[member_count].name = alf_entry_data(entry);
            members[member_count].size = member_chunk->size;
            members[member_count].data = /* memory-mapped or read from file */
                malloc(member_chunk->size);

            fseek(lib_file, member_chunk->file_offset, SEEK_SET);
            fread((void*)members[member_count].data, member_chunk->size, 1, lib_file);

            member_count++;
        }

        offset += entry->entry_length;
    }

    *out_members = members;
    *out_count = member_count;
    free(diry_data);
    return 0;
}
```

---

## Robustness and Compatibility

### Version 1.0 Robustness Notes

For compatibility with potentially older/newer versions:

**When Creating LIB_DIRY Entries:**
- Ensure all time stamps are valid
- Pad with NULL (0) bytes after name string
- Pad before time stamp for alignment
- Keep total entry size as multiple of 4

**When Reading LIB_DIRY Entries:**
- Don't assume padding values (could be garbage)
- Validate chunk index (should be ≥ 3 typically)
- Find name string by scanning for null terminator
- Check DataLength, not EntryLength, for data extent
- Find time stamp at end of data section

**When Writing Directory/Symbol Entries:**
- Zero-pad unused space
- Use consistent padding method
- Test with real linkers

**Linker Expectations:**
- LIB_DIRY always present
- LIB_TIME present (records last modification)
- LIB_VRSN always version 1
- LIB_DATA chunks in order (index 3, 4, 5, ...)
- Member names unique (recommended)
- All chunk offsets and sizes valid

---

## Summary Table

| Aspect | Detail |
|--------|--------|
| **File Type** | Chunk File with library chunks |
| **Magic** | ChunkFileId = 0xC3CBC6C5 |
| **Endianness** | Matches chunk file endianness |
| **Minimum Chunks** | LIB_DIRY, LIB_TIME, LIB_VRSN, LIB_DATA (1+) |
| **Member Storage** | One LIB_DATA per member |
| **Member Access** | Via LIB_DIRY lookup |
| **Time Base** | Centiseconds since 1900-01-01 |
| **Typical Use** | Linking with ARM linker |
| **Extensions** | OFL_SYMT, OFL_TIME for object libraries |

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals
- ARM Tools User Guide
- Chunk File Format specification (in AOF documentation)

