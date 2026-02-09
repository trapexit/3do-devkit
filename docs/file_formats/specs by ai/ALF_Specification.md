# ARM Library Format (ALF) — Comprehensive Specification

**Format Name:** ARM Library Format (ALF)  
**Also Known As:** Acorn Library Format, ARM Object Library Format (OFL)  
**Purpose:** Static library container for ARM Object Format (AOF) files  
**Foundation:** Chunk File Format (shared with AOF)  
**Magic Number:** `0xC3CBC6C5` (Chunk File identifier)  
**File Extensions:** `.lib`, `.a`

**Source References:**
- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide (1997-1998)
- RISC OS Programmer's Reference Manuals
- ARM Software Development Toolkit User Guide

---

## 1. Overview

ARM Library Format (ALF) is a container format for storing collections of ARM Object Format (AOF) files as a static library. It enables the ARM linker (`armlink`) to selectively include only the object modules needed to resolve undefined symbols during linking, reducing executable size.

### 1.1 Key Properties

- Based on the same **Chunk File Format** used by AOF
- Contains one or more AOF object files as opaque data members
- Provides a **directory** for locating members by name
- Provides an **external symbol table** mapping exported symbols to their defining members
- Supports **selective linking** — only members that satisfy undefined references are included

### 1.2 Library Types

ALF supports two library types:

1. **Generic/Data Libraries** — contain arbitrary data chunks; no symbol table; accessed by position or named index
2. **Object Code Libraries** (most common) — contain AOF object files; include an external symbol table (`OFL_SYMT`) for selective linking

### 1.3 Toolchain Pipeline

```
Source Files → [armcc/armasm] → AOF (.o) → [armlib] → ALF Library (.lib/.a)
                                                              ↓
                                              [armlink] → Executable (AIF/ELF)
```

**Producers:** ARM Librarian (`armlib`, also `arlib`, `LibFile`, `armar` across SDK versions)  
**Consumers:** ARM Linker (`armlink`), library inspection utilities (`decaof`)

---

## 2. Chunk File Format Foundation

ALF is layered on the Chunk File Format, which provides a generic container for multiple independent data chunks within a single file. Both ALF and AOF share this same foundation.

### 2.1 Chunk File Header

The file begins with a 12-byte fixed header:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 bytes | `ChunkFileId` | Magic number: **`0xC3CBC6C5`** |
| 0x04 | 4 bytes | `maxChunks` | Maximum number of chunk entries (fixed at file creation) |
| 0x08 | 4 bytes | `numChunks` | Currently used chunk entries (`0` to `maxChunks`) |

**Endianness detection:** If `ChunkFileId` reads as `0xC5C6CBC3`, all word values in the file must be byte-reversed before use.

### 2.2 Chunk Directory Entries

Immediately following the file header are `maxChunks` chunk directory entries, each 16 bytes:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 8 bytes | `chunkId` | 8-character identifier (see section 2.3) |
| +0x08 | 4 bytes | `fileOffset` | Byte offset from file start to chunk data. Must be divisible by 4 (word-aligned). **Value 0 = entry is unused.** |
| +0x0C | 4 bytes | `size` | Exact byte size of chunk contents (need not be a multiple of 4) |

**Total file header size:** `12 + (maxChunks × 16)` bytes

### 2.3 Chunk Identification

The 8-byte `chunkId` consists of two 4-character parts stored in ascending address order (like a string), independent of word endianness:

- **Bytes 0-3:** Namespace prefix — identifies the format domain
- **Bytes 4-7:** Component identifier — identifies the chunk type within the domain

| Prefix | Domain |
|--------|--------|
| `LIB_` | Standard library management chunks |
| `OFL_` | Object-file-library-specific chunks |
| `OBJ_` | AOF object file chunks (within embedded member files) |

**Example:** `"LIB_DIRY"` is stored as bytes `4C 49 42 5F 44 49 52 59`.

---

## 3. ALF Chunk Types

### 3.1 Standard Library Chunks (Required for ALL libraries)

| Chunk ID (8 bytes) | Required | Count | Description |
|-------------------|----------|-------|-------------|
| `LIB_DIRY` | Yes | 1 | Directory of library members |
| `LIB_TIME` | Yes | 1 | Library modification timestamp |
| `LIB_VRSN` | Yes | 1 | Library format version |
| `LIB_DATA` | Yes | 1 per member | Individual library member data |

### 3.2 Object Code Library Chunks (Additional, required for OFL)

| Chunk ID (8 bytes) | Required for OFL | Count | Description |
|-------------------|-------------------|-------|-------------|
| `OFL_SYMT` | Yes | 1 | External symbol index |
| `OFL_TIME` | Yes | 1 | Symbol table modification timestamp |

### 3.3 Conventional Chunk Ordering

While chunks may appear in any order (the header provides random access), the conventional arrangement is:

```
Chunk 0:   LIB_DIRY      (Directory)
Chunk 1:   LIB_TIME      (Library timestamp)
Chunk 2:   LIB_VRSN      (Version)
Chunk 3:   LIB_DATA      (Member 0 — first AOF file)
Chunk 4:   LIB_DATA      (Member 1)
...
Chunk N:   LIB_DATA      (Member N-3)
Chunk N+1: OFL_SYMT      (External symbol table — object libraries only)
Chunk N+2: OFL_TIME      (Symbol table timestamp — object libraries only)
```

---

## 4. LIB_DIRY — Directory Chunk

The directory is a single chunk containing a sequence of variable-length entries. Each entry describes one library member. The total number of entries is determined by the total size of the `LIB_DIRY` chunk.

### 4.1 Directory Entry Format

Each entry is an integral number of words (multiple of 4 bytes):

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 bytes | `ChunkIndex` | 0-origin index into the chunk file header of the corresponding `LIB_DATA` chunk. **Value 0 = entry is unused/deleted.** |
| +0x04 | 4 bytes | `EntryLength` | Total size of this directory entry in bytes. **Always a multiple of 4.** |
| +0x08 | 4 bytes | `DataLength` | Number of bytes used in the Data section below. **Always a multiple of 4.** |
| +0x0C | variable | Data | Member name + optional info + padding + timestamp |

### 4.2 Data Section Layout

The Data section within each directory entry contains, in order:

1. **Member Name** — NUL-terminated string. Valid characters: ISO-8859 non-control characters (codes 32-126, 160-255). Typically the original source filename.

2. **Additional Information** — Optional, tool-specific metadata. Often empty.

3. **Padding** — Zero bytes (`0x00`) to word-align the following timestamp.

4. **Timestamp** — 2 words (8 bytes), word-aligned. Records the last-modified time of the file from which the member was created. See section 7 for timestamp encoding.

### 4.3 Field Relationships

```
EntryLength ≥ 12 + DataLength
DataLength  ≤ EntryLength - 12
Both EntryLength and DataLength are multiples of 4
```

The next entry starts at `current_entry_offset + EntryLength`.

### 4.4 ChunkIndex Details

- **Value 0:** Entry is unused (deleted member, available for reuse)
- **Value ≥ 3:** Typical range, since chunks 0-2 are conventionally `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN`
- The index is **0-origin** into the chunk directory entry array

### 4.5 Directory Management

- **Fixed size:** The directory chunk size is established when the library is created
- **Expansion:** Limited by pre-allocating unused entries (`ChunkIndex = 0`)
- **Deletion:** Set `ChunkIndex` to 0; the corresponding `LIB_DATA` chunk remains (causes fragmentation)
- **Compaction:** Requires rebuilding the entire library file

### 4.6 Robustness Notes

- **Writers** must ensure valid timestamps are present and pad with NUL (`0x00`) bytes
- **Readers** should not rely on data beyond the member name string unless `DataLength` permits it; make no assumptions about padding byte values beyond the first NUL terminator

---

## 5. LIB_DATA — Member Data Chunks

### 5.1 Properties

- **One `LIB_DATA` chunk per library member**
- Content is the raw bytes of the member file
- **For object code libraries:** Each `LIB_DATA` contains a **complete, self-contained AOF file** (with its own chunk file header, `OBJ_HEAD`, `OBJ_AREA`, `OBJ_SYMT`, `OBJ_STRT`, etc.)
- Library management tools treat member contents as opaque — no interpretation is performed
- Endianness of member content must match the containing library
- A member could itself be a chunk file, or even another ALF library (nested libraries are supported but uncommon)

### 5.2 Access Pattern

```
1. Read LIB_DIRY to find member by name
2. Get ChunkIndex from directory entry
3. Use ChunkIndex to locate chunk in chunk file header's directory
4. Read data at file_offset for size bytes
```

---

## 6. OFL_SYMT — External Symbol Table

The external symbol table is present only in object code libraries. It indexes all externally-visible symbols defined by library members, enabling the linker to quickly locate which member defines a needed symbol without scanning all members.

### 6.1 Entry Format

The symbol table has an entry structure identical to `LIB_DIRY`, but with different semantics:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 bytes | `ChunkIndex` | 0-origin index of the `LIB_DATA` chunk containing the AOF member that defines this symbol. **Value 0 = entry unused.** |
| +0x04 | 4 bytes | `EntryLength` | Total size of this entry in bytes (multiple of 4) |
| +0x08 | 4 bytes | `DataLength` | Size of symbol name + padding in bytes (multiple of 4) |
| +0x0C | variable | Symbol Name | NUL-terminated string |
| ... | 1-4 bytes | Padding | NUL bytes to reach next word boundary |

### 6.2 Key Differences from LIB_DIRY Entries

- **No timestamp** — symbol table entries contain only the symbol name and padding
- **No additional info** — just the symbol name
- **Multiple entries per member** — one entry per exported symbol; many symbols can map to the same `ChunkIndex`
- Entries may optionally be **sorted alphabetically** for binary search efficiency

### 6.3 Symbol Extraction Process

When building the symbol table from AOF members:

1. For each member, locate its `OBJ_SYMT` chunk
2. For each symbol entry in `OBJ_SYMT`:
   - Check attributes: bit 0 (defined) AND bit 1 (global/exported) must both be set
   - If both are set, the symbol is an external definition
3. Extract the symbol name from the member's `OBJ_STRT`
4. Add an entry to `OFL_SYMT` with the member's `ChunkIndex`

### 6.4 Example

```
OFL_SYMT contents:

Entry 0: ChunkIndex=3, EntryLength=16, DataLength=4
         SymbolName: "sin\0"   Padding: 0x00 0x00 0x00

Entry 1: ChunkIndex=3, EntryLength=16, DataLength=4
         SymbolName: "cos\0"   Padding: 0x00 0x00 0x00

Entry 2: ChunkIndex=5, EntryLength=20, DataLength=8
         SymbolName: "printf\0" Padding: 0x00
```

In this example, `sin` and `cos` are both defined in the same member (chunk 3), while `printf` is in a different member (chunk 5).

---

## 7. Timestamps

### 7.1 LIB_TIME — Library Modification Timestamp

Exactly **8 bytes (2 words)**. Records when the library was last modified.

### 7.2 OFL_TIME — Symbol Table Timestamp

Exactly **8 bytes (2 words)**, same encoding as `LIB_TIME`. Records when `OFL_SYMT` was last updated.

Separate from `LIB_TIME` to allow tools to detect a stale symbol table: if `OFL_TIME < LIB_TIME`, the symbol table needs rebuilding.

### 7.3 Timestamp Encoding

All timestamps in ALF use the same format — a 6-byte centisecond count since epoch, plus 2 bytes of microseconds:

```
Word 0 (4 bytes):
+--------------------------------------------------+
| Most significant 4 bytes of 6-byte centisecond   |
| count (bytes 5, 4, 3, 2 of the count)            |
+--------------------------------------------------+

Word 1 (4 bytes):
+-------------------------+-------------------------+
| Least significant 2     | Microsecond count       |
| bytes of centisecond    | (usually 0)             |
| count (bits 31-16)      | (bits 15-0)             |
+-------------------------+-------------------------+
```

**Epoch:** 00:00:00, 1st January 1900 (same as RISC OS)  
**Primary resolution:** Centisecond (10 milliseconds)  
**Secondary resolution:** Microsecond (usually 0)  
**Range:** 6 bytes = 48 bits of centiseconds ≈ 8,900+ years

### 7.4 Timestamp Extraction

```c
/* Extract centisecond count (48-bit value) */
uint64_t centisecs = ((uint64_t)word0 << 16) | ((word1 >> 16) & 0xFFFF);

/* Extract microsecond count */
uint16_t microsecs = word1 & 0xFFFF;
```

---

## 8. LIB_VRSN — Version Chunk

Exactly **4 bytes (1 word)**. Contains the library format version number.

**Current value: `1`**

Readers should reject files with unknown version numbers. Future versions would use higher values.

---

## 9. Linker Interaction

### 9.1 Processing Model

The ARM linker (`armlink`) processes libraries differently from standalone object files:

- **Object files** in the input list are included **unconditionally**
- **Library members** are included **if and only if** some object file or already-included library member makes a **non-weak** reference to a symbol they define

### 9.2 Processing Order

1. **First pass:** All standalone object files are linked together. This produces a set of unresolved (non-weak) symbol references.

2. **Library pass:** Libraries are processed in command-line order:
   a. The library's `OFL_SYMT` is searched for members defining currently-unsatisfied non-weak symbols
   b. Each matching member is loaded, satisfying some references and potentially creating new ones
   c. The search **repeats** within the same library until no further members are loaded

3. **Each library is processed in turn.** A reference from a member of a later library to a member of an earlier library cannot be satisfied. Circular dependencies between libraries are not supported.

4. Any remaining unsatisfied non-weak references are link errors (unless producing partially-linked relocatable output).

### 9.3 Symbol Resolution Algorithm

```
function resolve_symbols(libraries):
    for each library in command_line_order:
        build symbol_map from OFL_SYMT   // symbol_name → ChunkIndex
        repeat:
            changed = false
            for each undefined_symbol in linker_state:
                if undefined_symbol is weak: continue
                if symbol_map contains undefined_symbol:
                    chunk_index = symbol_map[undefined_symbol]
                    load LIB_DATA[chunk_index] as AOF file
                    add all symbols/areas to linker state
                    changed = true
        until not changed
```

### 9.4 Key Behaviors

- **First definition wins** — resolution is link-order dependent
- **Weak symbols** are ignored when deciding which members to load
- Libraries are searched in command-line order; no backtracking
- Some linkers support `--whole-archive` to load all members unconditionally

---

## 10. Endianness and Alignment

### 10.1 Endianness

- ALF files exist in **little-endian** or **big-endian** variants
- File endianness always matches the **target ARM system**, not the host
- Detection: Read `ChunkFileId` as a word:
  - `0xC3CBC6C5` = file matches host byte order
  - `0xC5C6CBC3` = all word values must be byte-reversed
- **All members must have the same endianness** as the containing library
- The linker rejects mixed-endianness inputs
- Strings (chunk IDs, member names, symbol names) are stored in ascending address order and are endianness-independent

### 10.2 Alignment

| Data Type | Alignment |
|-----------|-----------|
| Strings / bytes | Any byte boundary |
| Half-words | 2-byte boundary |
| Words | 4-byte boundary |
| Chunk file offsets | Divisible by 4 |
| Directory entries | Integral number of words (multiple of 4 bytes) |
| `EntryLength` | Multiple of 4 |
| `DataLength` | Multiple of 4 |
| Timestamps within entries | Word-aligned |

---

## 11. Library Creation Process

### 11.1 Steps to Create an Object Code Library

1. **Initialize chunk file header:**
   - Set `ChunkFileId` = `0xC3CBC6C5`
   - Set `maxChunks` to accommodate all planned chunks plus growth room
   - Reserve entries 0-2 for `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN`

2. **For each AOF member file:**
   - Create a `LIB_DATA` chunk containing the complete AOF file
   - Add a `LIB_DIRY` entry with member name, timestamp, and `ChunkIndex` pointing to the new `LIB_DATA`

3. **Build the symbol table (`OFL_SYMT`):**
   - Scan each member's `OBJ_SYMT` for exported symbols (defined + global)
   - Create an `OFL_SYMT` entry for each, referencing the member's `ChunkIndex`

4. **Set timestamps:**
   - `LIB_TIME` = current time
   - `OFL_TIME` = current time

5. **Write `LIB_VRSN`** = `1`

6. **Finalize:** Update chunk file header with final offsets and sizes

### 11.2 Tool Commands

```bash
# Create library from object files
armlib -c mylib.lib module1.o module2.o module3.o

# Add or replace members
armlib -r mylib.lib newmodule.o

# List library contents
armlib -t mylib.lib

# Extract a member
armlib -x mylib.lib module1.o

# Delete a member
armlib -d mylib.lib module2.o

# Rebuild symbol table
armlib -s mylib.lib
```

---

## 12. Validation

### 12.1 Chunk File Level

- `ChunkFileId` must be `0xC3CBC6C5` (or byte-swapped `0xC5C6CBC3`)
- `numChunks` ≤ `maxChunks`
- All `fileOffset` values divisible by 4
- Chunks must not overlap
- All chunks must be within file bounds

### 12.2 Library Level

- `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN` must all be present
- `LIB_VRSN` must contain value `1`
- `LIB_TIME` must be exactly 8 bytes
- All referenced `LIB_DATA` chunks must exist

### 12.3 Directory Entries

- `ChunkIndex` must be 0 (unused) or a valid index to a `LIB_DATA` chunk
- `EntryLength` ≥ 12
- `DataLength` ≤ `EntryLength` - 12
- Member name must be properly NUL-terminated
- Both `EntryLength` and `DataLength` must be multiples of 4

### 12.4 Object Code Libraries

- `OFL_SYMT` and `OFL_TIME` must both be present (or both absent)
- Symbol entries must reference valid `LIB_DATA` chunks
- Referenced members must be valid AOF files

---

## 13. Complete File Structure Example

```
CHUNK FILE HEADER (12 bytes):
  ChunkFileId: 0xC3CBC6C5
  maxChunks:   10
  numChunks:   8

CHUNK DIRECTORY (10 × 16 = 160 bytes):
  Entry 0: id="LIB_DIRY"  offset=172   size=80
  Entry 1: id="LIB_TIME"  offset=252   size=8
  Entry 2: id="LIB_VRSN"  offset=260   size=4
  Entry 3: id="LIB_DATA"  offset=264   size=1024  (math_sin.o)
  Entry 4: id="LIB_DATA"  offset=1288  size=960   (math_cos.o)
  Entry 5: id="LIB_DATA"  offset=2248  size=2048  (string_printf.o)
  Entry 6: id="OFL_SYMT"  offset=4296  size=64
  Entry 7: id="OFL_TIME"  offset=4360  size=8
  Entry 8: (unused — fileOffset=0)
  Entry 9: (unused — fileOffset=0)

LIB_DIRY (80 bytes):
  Entry: ChunkIndex=3, EntryLength=28, DataLength=16
         Name: "math_sin.o\0" + padding + timestamp(8 bytes)
  Entry: ChunkIndex=4, EntryLength=28, DataLength=16
         Name: "math_cos.o\0" + padding + timestamp(8 bytes)
  Entry: ChunkIndex=5, EntryLength=24, DataLength=12
         Name: "string_printf.o\0" + padding + timestamp(8 bytes)

LIB_TIME (8 bytes):
  [2-word timestamp — library modification time]

LIB_VRSN (4 bytes):
  0x00000001

LIB_DATA (chunk 3, 1024 bytes):
  [Complete AOF file for math_sin.o, with its own ChunkFileId header]

LIB_DATA (chunk 4, 960 bytes):
  [Complete AOF file for math_cos.o]

LIB_DATA (chunk 5, 2048 bytes):
  [Complete AOF file for string_printf.o]

OFL_SYMT (64 bytes):
  Entry: ChunkIndex=3, EntryLength=16, DataLength=4,  Name="sin\0"+pad
  Entry: ChunkIndex=4, EntryLength=16, DataLength=4,  Name="cos\0"+pad
  Entry: ChunkIndex=5, EntryLength=20, DataLength=8,  Name="printf\0"+pad
  Entry: ChunkIndex=5, EntryLength=20, DataLength=8,  Name="sprintf\0"+pad

OFL_TIME (8 bytes):
  [2-word timestamp — symbol table modification time]
```

---

## 14. C Structure Definitions

```c
#ifndef ALF_FORMAT_H
#define ALF_FORMAT_H

#include <stdint.h>

/* ===== Chunk File Format ===== */

#define CHUNK_FILE_ID           0xC3CBC6C5
#define CHUNK_FILE_ID_REVERSED  0xC5C6CBC3
#define ALF_VERSION             1

/* Chunk File Header (12 bytes) */
typedef struct {
    uint32_t chunk_file_id;     /* 0xC3CBC6C5 */
    uint32_t max_chunks;        /* Maximum chunk directory entries */
    uint32_t num_chunks;        /* Currently used entries */
} ChunkFileHeader;

/* Chunk Directory Entry (16 bytes) */
typedef struct {
    char     chunk_id[8];       /* 8-character identifier (e.g., "LIB_DIRY") */
    uint32_t file_offset;       /* Byte offset from file start (4-byte aligned; 0 = unused) */
    uint32_t size;              /* Exact byte size of chunk contents */
} ChunkDirEntry;

/* ===== Library Directory Entry (variable-length) ===== */
typedef struct {
    uint32_t chunk_index;       /* 0-origin index of LIB_DATA chunk; 0 = unused */
    uint32_t entry_length;      /* Total entry size in bytes (multiple of 4) */
    uint32_t data_length;       /* Data section size in bytes (multiple of 4) */
    /* Followed by variable-length data:
     *   - Member name (NUL-terminated string)
     *   - Optional additional info
     *   - NUL padding (0x00) to word-align timestamp
     *   - Timestamp (2 words, 8 bytes — word-aligned)
     */
} ALFDirEntry;

/* ===== Symbol Table Entry (OFL_SYMT) ===== */
typedef struct {
    uint32_t chunk_index;       /* Index of LIB_DATA chunk defining this symbol */
    uint32_t entry_length;      /* Total entry size in bytes (multiple of 4) */
    uint32_t data_length;       /* Symbol name + padding size (multiple of 4) */
    /* Followed by:
     *   - Symbol name (NUL-terminated string)
     *   - NUL padding (1-4 bytes to next word boundary)
     *   NOTE: No timestamp (unlike LIB_DIRY entries)
     */
} ALFSymEntry;

/* ===== Timestamp (8 bytes) ===== */
/* Used by LIB_TIME, OFL_TIME, and timestamps within directory entries */
typedef struct {
    uint32_t centisecs_high;        /* MS 4 bytes of 6-byte centisecond count */
    uint32_t centisecs_low_usec;    /* Bits 31-16: LS 2 bytes of centisecond count
                                       Bits 15-0:  Microsecond count (usually 0) */
} ALFTimestamp;

/* Timestamp epoch: 00:00:00, 1st January 1900 */
/* Resolution: centisecond (10 ms) primary; microsecond secondary */

/* Timestamp extraction macros */
#define ALF_GET_CENTISECS(ts) \
    (((uint64_t)(ts).centisecs_high << 16) | (((ts).centisecs_low_usec >> 16) & 0xFFFF))

#define ALF_GET_MICROSECS(ts) \
    ((ts).centisecs_low_usec & 0xFFFF)

/* ===== Version Chunk (4 bytes) ===== */
typedef struct {
    uint32_t version;           /* Must be 1 */
} ALFVersion;

/* ===== Chunk ID Constants ===== */
/* Standard library chunks */
#define ALF_ID_DIRY     "LIB_DIRY"  /* Directory */
#define ALF_ID_TIME     "LIB_TIME"  /* Library timestamp */
#define ALF_ID_VRSN     "LIB_VRSN"  /* Version */
#define ALF_ID_DATA     "LIB_DATA"  /* Member data */

/* Object code library chunks */
#define ALF_ID_SYMT     "OFL_SYMT"  /* Symbol table */
#define ALF_ID_SYTIME   "OFL_TIME"  /* Symbol table timestamp */

/* ===== Helper Functions ===== */

/* Compare chunk IDs (8-byte comparison) */
static inline int chunk_id_equals(const char id[8], const char *name) {
    return memcmp(id, name, 8) == 0;
}

/* Check if a directory entry is active (not deleted) */
static inline int alf_entry_active(const ALFDirEntry *entry) {
    return entry->chunk_index != 0;
}

/* Get pointer to member name within a directory entry */
static inline const char *alf_entry_name(const ALFDirEntry *entry) {
    return (const char *)entry + 12;  /* Offset past fixed header */
}

/* Get pointer to next directory entry */
static inline const ALFDirEntry *alf_next_entry(const ALFDirEntry *entry) {
    return (const ALFDirEntry *)((const uint8_t *)entry + entry->entry_length);
}

#endif /* ALF_FORMAT_H */
```

---

## 15. Related Formats

| Format | Purpose | Relationship |
|--------|---------|-------------|
| **AOF** (ARM Object Format) | Relocatable object files | Members stored inside ALF `LIB_DATA` chunks |
| **AIF** (ARM Image Format) | Executable images | Linker output; produced from AOF + ALF inputs |
| **Chunk File Format** | Generic container | Shared foundation used by both ALF and AOF |

---

**End of ARM Library Format (ALF) Specification**
