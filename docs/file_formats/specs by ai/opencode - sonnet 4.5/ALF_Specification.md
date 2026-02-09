# ARM Library Format (ALF) - Complete Specification

## Document Information

**Format Name:** ARM Library Format (ALF) / Acorn Library Format  
**Version:** 1  
**Chunk Identifier:** LIB_  
**Additional Chunks (Object Libraries):** OFL_  
**Purpose:** Collection and management of ARM Object Format (AOF) files  
**File Extension:** .lib (object code libraries)  

---

## 1. Overview

ARM Library Format (ALF) is a container format for organizing and managing collections of ARM Object Format (AOF) files. ALF is used by:

- **ARM librarian (armlib)** - Creates and maintains libraries
- **ARM linker (armlink)** - Extracts members from libraries during linking

### 1.1 Types of Libraries

ALF supports two types of libraries:

1. **Generic Libraries:** Collections of arbitrary data members
2. **Object Code Libraries:** Collections of AOF object files with symbol indexing for efficient linking

---

## 2. Fundamental Architecture

ALF is built on **Chunk File Format**, the same foundation used by AOF.

### 2.1 Relationship to Chunk File Format

```
┌─────────────────────────────────────┐
│     Chunk File Format (Base)        │
│  ┌───────────────────────────────┐  │
│  │  ARM Library Format (ALF)     │  │
│  │                               │  │
│  │  - Directory (LIB_DIRY)      │  │
│  │  - Time Stamp (LIB_TIME)     │  │
│  │  - Version (LIB_VRSN)        │  │
│  │  - Data (LIB_DATA) [×N]      │  │
│  │                               │  │
│  │  Optional (Object Libraries): │  │
│  │  - Symbol Table (OFL_SYMT)   │  │
│  │  - Symbol Time (OFL_TIME)    │  │
│  └───────────────────────────────┘  │
└─────────────────────────────────────┘
```

---

## 3. Data Types and Byte Ordering

### 3.1 Endianness

ALF files have the same endianness as the target ARM system. This may differ from the host system processing the file.

**Key Properties:**
- Endianness determined by chunk file header ChunkFileId (0xC3CBC6C5)
- If appears as 0xC5C6CBC3, word values must be byte-reversed
- Two endiannesses cannot be meaningfully mixed
- ARM linker accepts either endianness but rejects mixed inputs

### 3.2 Basic Types

| Type | Size | Alignment | Byte Order |
|------|------|-----------|------------|
| **Byte** | 8 bits | 1 byte | N/A |
| **Word** | 32 bits | 4 bytes | Little or big endian (consistent throughout file) |
| **String** | Variable | 1 byte | Ascending address order (independent of endianness) |

### 3.3 Alignment Rules

- **Strings and bytes:** Any byte boundary
- **Words:** 4-byte boundaries
- **Halfwords:** Not used in ALF field definitions
- **Within AOF members:** Alignment defined by AOF specification

---

## 4. Chunk File Structure

ALF libraries use chunk file format with "LIB_" chunk identifier prefix.

### 4.1 Chunk File Header

Identical to AOF chunk file header:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| 0x00 | ChunkFileId | Word | 0xC3CBC6C5 (determines endianness) |
| 0x04 | max_chunks | Word | Maximum chunk entries (fixed at creation) |
| 0x08 | num_chunks | Word | Currently used chunks (0 to max_chunks) |

Followed by **max_chunks** chunk entries (4 words each):

| Field | Size | Description |
|-------|------|-------------|
| chunkId | 8 bytes | Chunk identifier (e.g., "LIB_DIRY") |
| file_offset | Word | Byte offset to chunk (0 = unused, else multiple of 4) |
| size | Word | Exact byte size of chunk |

### 4.2 Standard Chunk Layout

**For all libraries:**

| Chunk | Chunk ID | Required | Count | Description |
|-------|----------|----------|-------|-------------|
| Directory | LIB_DIRY | Yes | 1 | Directory of library members |
| Time Stamp | LIB_TIME | Yes | 1 | Library modification time |
| Version | LIB_VRSN | Yes | 1 | Library format version |
| Data | LIB_DATA | Yes | N | Library members (one chunk per member) |

**Additional for object code libraries:**

| Chunk | Chunk ID | Required | Count | Description |
|-------|----------|----------|-------|-------------|
| Symbol Table | OFL_SYMT | Yes | 1 | External symbol index |
| Symbol Time | OFL_TIME | Yes | 1 | Symbol table modification time |

**Typical layout:**

```
Chunk 0: LIB_DIRY
Chunk 1: LIB_TIME
Chunk 2: LIB_VRSN
Chunk 3-N: LIB_DATA (one per member)
Chunk N+1: OFL_SYMT (object libraries only)
Chunk N+2: OFL_TIME (object libraries only)
```

---

## 5. LIB_DIRY - Directory Chunk

The directory indexes all members in the library.

### 5.1 Structure

Sequence of variable-length entries, each an integral number of words:

```
┌─────────────────────────────────┐
│ Directory Entry 0               │
├─────────────────────────────────┤
│ Directory Entry 1               │
├─────────────────────────────────┤
│ ...                             │
├─────────────────────────────────┤
│ Directory Entry N               │
└─────────────────────────────────┘
```

### 5.2 Directory Entry Format

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | ChunkIndex | Word | Zero-origin index of LIB_DATA chunk (0 = unused entry) |
| +0x04 | EntryLength | Word | Total bytes in this entry (multiple of 4) |
| +0x08 | DataLength | Word | Bytes used in data section (multiple of 4) |
| +0x0C | Data | Variable | Entry data (see below) |

### 5.3 Data Section Format

The Data section contains (in order):

1. **Member Name:** NUL-terminated string (ISO-8859 non-control characters only)
2. **Additional Info:** Optional data (often empty)
3. **Padding:** Zero bytes to word-align time stamp
4. **Time Stamp:** 2 words (8 bytes), word-aligned

**Layout:**
```
Offset  Content
+0x0C   Member name string\0
+???    Additional information (optional, often empty)
+???    Padding (0x00 bytes to word boundary)
+???    Time stamp word 1 (4 bytes)
+???    Time stamp word 2 (4 bytes)
```

### 5.4 ChunkIndex Field

- **Value 0:** Directory entry is **unused** (deleted member)
- **Value ≥ 3:** Zero-origin index into chunk file header
  - Points to corresponding LIB_DATA chunk
  - Conventionally ≥ 3 because chunks 0-2 are LIB_DIRY, LIB_TIME, LIB_VRSN
  - Actual minimum depends on library structure

### 5.5 Entry Management

**Adding a member:**
1. Find unused entry (ChunkIndex = 0) or add new entry
2. Create LIB_DATA chunk
3. Set ChunkIndex to new chunk's index
4. Fill in name, time stamp, lengths

**Deleting a member:**
1. Set ChunkIndex to 0
2. Optionally reclaim space during library compaction
3. LIB_DATA chunk becomes unreferenced (may be removed)

### 5.6 Compatibility Notes

**For robustness with earlier ALF versions:**

- **Creating entries:** Ensure time stamps are valid and present
- **Reading entries:** Don't assume data beyond name string exists unless DataLength indicates it
- **Padding:** Write NULL (0) bytes for padding; don't assume padding values when reading beyond first NULL
- **Name access:** Calculate name length, don't rely on fixed offsets

---

## 6. LIB_TIME - Library Time Stamp

Records when the library was last modified.

### 6.1 Format

Single 2-word (8-byte) time stamp.

### 6.2 Time Stamp Encoding

ALF uses a specific time stamp format:

**Word 1 (Most Significant):**
- Contains most significant 4 bytes of 6-byte centisecond count

**Word 2 (Least Significant):**
- Bits 16-31: Least significant 2 bytes of centisecond count
- Bits 0-15: Microseconds since last centisecond (usually 0)

**Time Base:** 00:00:00, 1st January 1900

**Precision:**
- Primary: Centiseconds (0.01 second) since epoch
- Secondary: Microseconds (0.000001 second) within centisecond

### 6.3 Centisecond Count Structure

```
6-byte centisecond count breakdown:
  Byte 5 (MSB) ┐
  Byte 4       ├─ Word 1 (MS word)
  Byte 3       │
  Byte 2       ┘
  Byte 1       ┐
  Byte 0 (LSB) ┴─ Word 2, bits 16-31 (LS word upper half)
  
  Microseconds ─── Word 2, bits 0-15 (LS word lower half)
```

### 6.4 Endianness

Time stamp words stored in target system byte order (same as chunk file endianness).

### 6.5 Calculation Example

Given date: 1st January 2000, 12:00:00.00

1. Calculate days from 1900-01-01 to 2000-01-01: 36,525 days
2. Add 12 hours: 36,525 days + 0.5 days = 36,525.5 days
3. Convert to seconds: 36,525.5 × 86,400 = 3,156,604,800 seconds
4. Convert to centiseconds: 3,156,604,800 × 100 = 315,660,480,000 cs
5. Express as 6 bytes: 0x0000004980D04000
   - Word 1: 0x00000049
   - Word 2: 0x80D04000 (upper 16 bits = 0x80D0, lower 16 bits = 0x4000 = microseconds)

---

## 7. LIB_VRSN - Version Chunk

Identifies the library format version.

### 7.1 Format

Single word value: **1**

### 7.2 Purpose

- Allows future ALF format revisions
- Tools can check compatibility
- Current version: 1

---

## 8. LIB_DATA - Data Chunks

Each library member is stored in its own LIB_DATA chunk.

### 8.1 Properties

- **Count:** One LIB_DATA chunk per library member
- **Content:** Arbitrary data (for generic libraries) or AOF file (for object libraries)
- **Endianness:** Same as containing chunk file (by assumption)
- **Interpretation:** Determined by member type

### 8.2 Object Code Library Members

For object code libraries:
- Each LIB_DATA chunk contains a complete AOF file
- AOF may itself use chunk file format
- Member can be another library (nested libraries possible)
- No additional interpretation by library management tools

### 8.3 Access

Members accessed through:
1. Read LIB_DIRY to find member by name
2. Get ChunkIndex for desired member
3. Use ChunkIndex to locate LIB_DATA chunk in chunk file header
4. Read chunk at specified file_offset

---

## 9. OFL_SYMT - External Symbol Table (Object Libraries Only)

Indexes external symbols defined by library members for efficient linking.

### 9.1 Purpose

Enables linker to quickly find which library member defines a needed symbol without scanning all members.

### 9.2 Format

Identical structure to LIB_DIRY, but data section contains only:
- External symbol name (NUL-terminated string)
- Padding (1-4 NULL bytes to word-align entry)

**Entry Structure:**

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | ChunkIndex | Word | Index of LIB_DATA chunk defining this symbol |
| +0x04 | EntryLength | Word | Total entry size in bytes (multiple of 4) |
| +0x08 | DataLength | Word | Size of symbol name + padding (multiple of 4) |
| +0x0C | SymbolName | String | NUL-terminated external symbol name |
| +??? | Padding | Bytes | NULL bytes (0x00) to reach word boundary |

**Note:** No time stamp in OFL_SYMT entries (unlike LIB_DIRY).

### 9.3 Symbol Table Properties

- One entry per external symbol defined in library
- Multiple symbols may map to same ChunkIndex (same member defines multiple symbols)
- Symbols from all library members aggregated
- Updated when members added/removed/modified

### 9.4 Example

```
Entry 0:
  ChunkIndex: 5        (member at chunk 5)
  EntryLength: 16      (4+4+4+3+1 padded to 16)
  DataLength: 4        (name "sin\0" = 4 bytes including padding)
  SymbolName: "sin\0"
  Padding: 0x00, 0x00, 0x00

Entry 1:
  ChunkIndex: 5        (same member)
  EntryLength: 16
  DataLength: 4
  SymbolName: "cos\0"
  Padding: 0x00, 0x00, 0x00

Entry 2:
  ChunkIndex: 7        (different member)
  EntryLength: 20
  DataLength: 8
  SymbolName: "printf\0"
  Padding: 0x00
```

---

## 10. OFL_TIME - Symbol Table Time Stamp (Object Libraries Only)

Records when OFL_SYMT was last modified.

### 10.1 Format

Identical to LIB_TIME: 2-word (8-byte) time stamp.

### 10.2 Purpose

- Tracks symbol table freshness
- Allows incremental updates
- Detects stale symbol indices

### 10.3 Update Conditions

OFL_TIME should be updated when:
- Member added to library
- Member deleted from library  
- Member modified (symbols changed)
- Library reorganized

---

## 11. Creating Object Code Libraries

### 11.1 Process Overview

1. **Initialize library:**
   - Create chunk file header
   - Allocate LIB_DIRY chunk
   - Create LIB_TIME, LIB_VRSN chunks
   - Reserve space for OFL_SYMT, OFL_TIME

2. **Add members:**
   - For each AOF file:
     - Create LIB_DATA chunk with AOF contents
     - Add directory entry with member name and time
     - Extract external symbols from AOF OBJ_SYMT
     - Add symbol entries to OFL_SYMT

3. **Finalize:**
   - Update LIB_TIME with current time
   - Update OFL_TIME with current time
   - Write chunk file header with final offsets

### 11.2 Symbol Extraction

From each member AOF file:

1. Locate OBJ_SYMT chunk
2. For each symbol entry:
   - Check flags: bit 0 (defined) AND bit 1 (global)
   - If both set: symbol is external definition
   - Extract symbol name from member's OBJ_STRT
   - Add to OFL_SYMT with ChunkIndex of member

### 11.3 Directory Size Planning

- Directory entries variable length (name + overhead)
- Pre-allocate directory space based on:
  - Expected member count
  - Average member name length
  - Growth allowance (unused entries)

---

## 12. Using Libraries with the Linker

### 12.1 Selective Linking

Linker includes only required members:

1. **Initial scan:**
   - Identify undefined symbols in object files
   
2. **Library search:**
   - For each library in search path:
     - Search OFL_SYMT for undefined symbols
     - If symbol found, note ChunkIndex
     
3. **Member extraction:**
   - Load LIB_DATA for required ChunkIndex
   - Parse AOF member
   - Add to link (may introduce new undefined symbols)
   
4. **Iteration:**
   - Repeat until no new undefined symbols

### 12.2 Search Order

- Libraries processed in command-line order
- Within library, symbols located via OFL_SYMT hash/index
- If symbol multiply defined, first found wins (or error)

### 12.3 Efficiency

OFL_SYMT enables:
- Direct symbol lookup without scanning all members
- Loading only required members
- Fast linking with large libraries

---

## 13. Library Maintenance Operations

### 13.1 Adding a Member

```
1. Read existing library
2. Find unused directory entry or append new entry
3. Create new LIB_DATA chunk with member data
4. Update directory entry:
   - ChunkIndex → new chunk index
   - EntryLength, DataLength
   - Member name, time stamp
5. Extract external symbols (if object library)
6. Update OFL_SYMT with new symbols
7. Update LIB_TIME and OFL_TIME
8. Write modified library
```

### 13.2 Deleting a Member

```
1. Read existing library
2. Locate directory entry by name
3. Set ChunkIndex to 0 (marks unused)
4. Remove member's symbols from OFL_SYMT
5. Update LIB_TIME and OFL_TIME
6. Optionally compact library (remove unreferenced chunks)
7. Write modified library
```

### 13.3 Replacing a Member

```
1. Delete old member
2. Add new member with same name
3. Atomic operation preferred to avoid inconsistency
```

### 13.4 Listing Members

```
1. Read LIB_DIRY chunk
2. For each entry where ChunkIndex ≠ 0:
   - Extract member name from data section
   - Extract time stamp
   - Display to user
```

### 13.5 Extracting a Member

```
1. Read LIB_DIRY to find member by name
2. Get ChunkIndex from directory entry
3. Locate LIB_DATA chunk in chunk header
4. Read chunk data at file_offset
5. Write to output file
```

---

## 14. Compatibility Considerations

### 14.1 Earlier ALF Versions

**When reading:**
- Don't assume directory entry data beyond DataLength exists
- Handle missing time stamps gracefully
- Verify padding is NULL but don't rely on specific padding values

**When writing:**
- Always include valid time stamps in directory entries
- Pad with NULL (0x00) bytes
- Set all required fields

### 14.2 Endianness Mixing

- **Not supported:** Cannot mix little and big-endian in same library
- **Tools must reject:** Mixed endianness inputs
- **Tools may support:** Reading either endianness, writing same endianness

### 14.3 Nested Libraries

- LIB_DATA may contain another chunk file (including ALF)
- Nesting depth not limited by format
- Tools may impose practical limits
- Use with caution (complexity)

---

## 15. Validation and Error Checking

### 15.1 Required Validation

**Chunk file level:**
1. ChunkFileId = 0xC3CBC6C5 (or byte-swapped)
2. num_chunks ≤ max_chunks
3. All file_offsets divisible by 4
4. Chunks don't overlap
5. All chunks within file bounds

**Library level:**
1. LIB_DIRY, LIB_TIME, LIB_VRSN chunks present
2. LIB_VRSN contains 1
3. LIB_TIME is 8 bytes
4. All referenced LIB_DATA chunks exist

**Directory entries:**
1. ChunkIndex = 0 or valid chunk index
2. EntryLength ≥ 12 (minimum: ChunkIndex, EntryLength, DataLength)
3. DataLength ≤ EntryLength - 12
4. Entry size = EntryLength
5. Member name properly NUL-terminated

**Object libraries:**
1. OFL_SYMT and OFL_TIME present
2. Symbol entries reference valid LIB_DATA chunks
3. Referenced members are valid AOF files

### 15.2 Recommended Validation

1. Time stamps reasonable (not in future, not before epoch)
2. Member names use only printable characters
3. ChunkIndex values form valid set (no gaps preferred)
4. LIB_DATA chunks contain valid data (AOF for object libraries)
5. OFL_SYMT symbols match actual exports from members

---

## 16. Performance Optimization

### 16.1 Symbol Table Organization

- **Sorted entries:** Binary search for symbols
- **Hash table:** Even faster lookups (tool-specific)
- **Compact storage:** Minimal padding

### 16.2 Directory Organization

- **Pre-allocated space:** Avoid frequent reorganization
- **Sorted names:** Faster member searches
- **Compact unused entries:** Mark with ChunkIndex=0, don't remove

### 16.3 Chunk Placement

- **Cluster data:** Place LIB_DATA chunks together
- **Metadata first:** Directory, tables before data for faster header reads
- **Alignment:** Align chunks to page boundaries for memory mapping

---

## 17. Example Library Structure

### 17.1 Small Object Library

```
File Layout:
============

Chunk File Header (84 bytes = 3 words + 5 entries × 4 words)
  ChunkFileId: 0xC3CBC6C5
  max_chunks: 5
  num_chunks: 5
  
  Entry 0: LIB_DIRY, offset=0x0054, size=96
  Entry 1: LIB_TIME, offset=0x00B4, size=8
  Entry 2: LIB_VRSN, offset=0x00BC, size=4
  Entry 3: LIB_DATA, offset=0x00C0, size=512  (member "sin.o")
  Entry 4: LIB_DATA, offset=0x02C0, size=480  (member "cos.o")
  Entry 5: OFL_SYMT, offset=0x04A0, size=32
  Entry 6: OFL_TIME, offset=0x04C0, size=8

LIB_DIRY (96 bytes)
  Entry 0:
    ChunkIndex: 3
    EntryLength: 32
    DataLength: 20
    Data: "sin.o\0" + padding + timestamp
  Entry 1:
    ChunkIndex: 4
    EntryLength: 32
    DataLength: 20
    Data: "cos.o\0" + padding + timestamp
  (Remaining space unused)

LIB_TIME (8 bytes)
  0x00000049, 0x80D04000

LIB_VRSN (4 bytes)
  0x00000001

LIB_DATA (chunk 3, 512 bytes)
  [Complete AOF file for sin.o]

LIB_DATA (chunk 4, 480 bytes)
  [Complete AOF file for cos.o]

OFL_SYMT (32 bytes)
  Entry 0:
    ChunkIndex: 3
    EntryLength: 16
    DataLength: 4
    SymbolName: "sin\0"
  Entry 1:
    ChunkIndex: 4
    EntryLength: 16
    DataLength: 4
    SymbolName: "cos\0"

OFL_TIME (8 bytes)
  0x00000049, 0x80D04000
```

---

## 18. Tool Implementation Notes

### 18.1 Librarian (armlib) Operations

**Create:**
```
armlib -create mylib.lib file1.o file2.o file3.o
```

**Add:**
```
armlib -insert mylib.lib newfile.o
```

**Delete:**
```
armlib -delete mylib.lib file2.o
```

**List:**
```
armlib -list mylib.lib
```

**Extract:**
```
armlib -extract mylib.lib file1.o
```

### 18.2 Linker (armlink) Usage

```
armlink main.o -lib mylib.lib -lib stdlib.lib -o output.axf
```

Process:
1. Load main.o, record undefined symbols
2. Search mylib.lib OFL_SYMT for undefined symbols
3. Load required members from mylib.lib
4. Repeat for stdlib.lib
5. Resolve all symbols
6. Generate output

---

## 19. Limitations and Extensions

### 19.1 Format Limitations

- **Member count:** Limited by directory size (practical: thousands)
- **Member size:** Limited by chunk size field (4GB per member)
- **Total library size:** Limited by file system (typically >4GB on modern systems)
- **Name length:** No explicit limit (practical: <256 chars)

### 19.2 Possible Extensions

- **Compression:** LIB_DATA could contain compressed AOF
- **Indices:** Additional chunks for faster searching
- **Metadata:** Extended directory information
- **Security:** Digital signatures, encryption

---

## 20. Related Formats

### 20.1 AOF (ARM Object Format)

- Member format for object code libraries
- ALF is a container for AOF files
- See AOF specification for member structure

### 20.2 AIF (ARM Image Format)

- Executable output from linking
- Not stored in libraries (but could be in generic library)

### 20.3 Archive Formats

- **Unix ar:** Similar concept, different format
- **COFF libraries:** Microsoft format
- **ELF archives:** Modern alternative to ALF

---

## 21. References

### 21.1 Source Documents

1. ARM Software Development Toolkit Reference Guide (DUI0041C), Chapter 14
2. RISC OS Programmer's Reference Manual, Library File Format sections
3. ARM Object Library Format specification (ALF)

### 21.2 Related Specifications

- ARM Object Format (AOF)
- Chunk File Format
- ARM Image Format (AIF)
- ARM Procedure Call Standard (APCS)

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024 | Initial comprehensive specification compiled from ARM documentation sources |

---

**End of ARM Library Format (ALF) Specification**
