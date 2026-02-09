# ARM Object Library Format (ALF) - Complete File Format Specification

## Document Information

- **Format Name**: ARM Object Library Format (ALF) / ARM Library Format
- **File Extension**: .lib, .a
- **Byte Order**: Little-endian or Big-endian (matches target)
- **Version**: LIB_VRSN = 1
- **Copyright**: Based on ARM Limited and Acorn Computers documentation

---

## 1. Overview

ARM Object Library Format (ALF) is a library file format used by the ARM linker (armlink) and the ARM object librarian (armlib). It provides a container for multiple AOF (ARM Object Format) modules, allowing efficient selective linking.

### 1.1 Purpose and Design Goals

- **Library Storage**: Collect multiple AOF modules in a single file
- **Selective Linking**: Link only needed modules (by symbol references)
- **Efficient Access**: Quick lookup of symbols and modules
- **Tool Integration**: Standard format for ARM librarian and linker
- **Extensibility**: Chunk-based structure allows additions

### 1.2 Key Features

1. **Chunk-Based Structure**: Layered on Chunk File Format (like AOF)
2. **Directory**: Index of library members with names and timestamps
3. **Symbol Table**: Fast lookup of which member defines which symbol
4. **Versioning**: Version tracking for library format
5. **Timestamps**: Track modification times of members and library
6. **Multiple Members**: Each member is a complete AOF file

### 1.3 Relationship to Other Formats

```
Source Files (.c, .s)
    ↓
[Compiler/Assembler]
    ↓
Object Files (.o) - AOF Format
    ↓
[Librarian]
    ↓
Library Files (.lib) - ALF Format
    ↓
[Linker]
    ↓
Executable (.aif) - AIF Format
```

---

## 2. Terminology

### 2.1 Basic Data Types

| Type | Size | Alignment | Description |
|------|------|-----------|-------------|
| **Byte** | 8 bits | Any | Unsigned unless stated; stores flags or characters |
| **Half Word** | 16 bits | 2-byte | Usually unsigned; little-endian byte order |
| **Word** | 32 bits | 4-byte | Usually unsigned; little-endian byte order |
| **String** | Variable | Any | Byte sequence terminated by NUL (0x00); NUL included but not counted in length |

**Important**: For data in a file, "address" means offset from the start of the file.

### 2.2 Byte Order (Endianness)

ALF exists in two variants, matching AOF:

#### Little-Endian ALF
- **Byte Order**: Least significant byte at lowest address
- **Used By**: DEC, Intel, Acorn, ARM (default)
- **Example**: 0x12345678 stored as [78 56 34 12]

#### Big-Endian ALF
- **Byte Order**: Most significant byte at lowest address
- **Used By**: IBM, Motorola, Apple
- **Example**: 0x12345678 stored as [12 34 56 78]

**Rules**:
- ALF file endianness matches target ARM system endianness
- Cannot meaningfully mix endianness
- Linker accepts either but rejects mixed inputs
- All members in a library must have same endianness

### 2.3 Alignment Rules

- **Strings and Bytes**: Any byte boundary
- **Words**: 4-byte boundaries
- **ALF Fields**: Words only, 4-byte aligned
- **Within Members** (OBJ_AREA contents): Strict alignment (4-byte for words, 2-byte for halfwords)

---

## 3. Overall Structure of ALF Files

### 3.1 Chunk File Format Foundation

ALF is layered on Chunk File Format, providing:
- Simple access to distinct data chunks
- Efficient updating of individual chunks
- Extensibility for additional data
- File operations without chunk content knowledge

### 3.2 ALF Chunk File Header

Identical structure to AOF chunk files:

```
+----------------------------------+
| ChunkFileId (0xC3CBC6C5)         | 4 bytes - File signature
+----------------------------------+
| max_chunks                       | 4 bytes - Max chunks (fixed at creation)
+----------------------------------+
| num_chunks                       | 4 bytes - Current chunks used
+----------------------------------+
| Chunk Entry 1                    | 16 bytes (4 words)
|   - chunkId (8 bytes)            |   "LIB_????" identifiers
|   - offset (4 bytes)             |   Byte offset to chunk
|   - size (4 bytes)               |   Chunk size in bytes
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

### 3.3 Chunk Identification

The **chunkId** is 8 bytes split into two parts:

- **Bytes 0-3**: Universal unique name
  - For ALF (Library): `"LIB_"` (0x4C49425F)
  - For OFL (Object Library): `"OFL_"` (0x4F464C5F)
- **Bytes 4-7**: Component identifier
  - Examples: `"DIRY"`, `"TIME"`, `"VRSN"`, `"DATA"`, etc.

**Storage**: Characters in ascending address order (string order), independent of endianness.

### 3.4 ALF Chunk Types

#### Standard Library Chunks (All Libraries)

| Chunk Name | ChunkId | Required | Contents |
|------------|---------|----------|----------|
| **Directory** | `LIB_DIRY` | Yes | Directory of library members |
| **Time Stamp** | `LIB_TIME` | Yes | Library modification timestamp |
| **Version** | `LIB_VRSN` | Yes | Library format version (value = 1) |
| **Data** | `LIB_DATA` | Yes (multiple) | Individual library members (AOF files) |

#### Object Library Chunks (Object Code Libraries Only)

| Chunk Name | ChunkId | Required | Contents |
|------------|---------|----------|----------|
| **Symbol Table** | `OFL_SYMT` | Yes | External symbols defined in library |
| **Symbol Time Stamp** | `OFL_TIME` | Yes | Symbol table modification timestamp |

**Note**: There are typically many `LIB_DATA` chunks (one per library member), but only one of each other chunk type.

---

## 4. LIB_DIRY: Directory Chunk Format

The directory chunk contains an index of all library members.

### 4.1 Overall Structure

```
+----------------------------------+
| Directory Entry 1                | Variable length (multiple of 4)
+----------------------------------+
| Directory Entry 2                | Variable length (multiple of 4)
+----------------------------------+
...
| Directory Entry N                | Variable length (multiple of 4)
+----------------------------------+
```

### 4.2 Directory Entry Format

Each entry is variable length but always a multiple of 4 bytes:

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | **ChunkIndex** | Word | Index of corresponding LIB_DATA chunk (0 = unused) |
| +0x04 | **EntryLength** | Word | Total bytes in this directory entry (multiple of 4) |
| +0x08 | **DataLength** | Word | Bytes used in Data field (multiple of 4) |
| +0x0C | **Data** | Variable | Member name + optional data + timestamp |

### 4.3 Field Descriptions

#### ChunkIndex

- **Type**: Word (4 bytes)
- **Value**: Zero-origin index within chunk file header of corresponding `LIB_DATA` chunk
- **Convention**: First three chunks are typically `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN`, so ChunkIndex ≥ 3
- **Special Value**: 0 means directory entry is unused (available for reuse)
- **Purpose**: Direct lookup from directory entry to member data

#### EntryLength

- **Type**: Word (4 bytes)
- **Value**: Total number of bytes in this directory entry
- **Constraint**: Always multiple of 4
- **Includes**: ChunkIndex, EntryLength, DataLength, and all Data
- **Purpose**: Skip to next entry without parsing contents

#### DataLength

- **Type**: Word (4 bytes)
- **Value**: Number of bytes used in Data section
- **Constraint**: Multiple of 4
- **Purpose**: Actual data size (may be less than allocated space)

#### Data

Variable-length section containing (in order):

1. **Member Name**: Zero-terminated string
   - **Characters**: ISO-8859 non-control characters only (32-126, 160-255)
   - **Terminator**: NUL (0x00) byte
   - **Example**: `"module1.o\0"`

2. **Optional Information**: (Often empty)
   - **Purpose**: Library-specific metadata
   - **Typical**: Empty (just padding to timestamp)

3. **Padding**: Zero bytes to word-align timestamp
   - **Purpose**: Ensure timestamp starts on word boundary

4. **Time Stamp**: 8 bytes (2 words)
   - **Format**: Defined in section 8 (Time Stamps)
   - **Purpose**: Track when member was added/modified

### 4.4 Example Directory Entry

```
Offset  Content             Description
+0x00   03 00 00 00         ChunkIndex = 3
+0x04   20 00 00 00         EntryLength = 32 bytes
+0x08   20 00 00 00         DataLength = 32 bytes
+0x0C   6D 6F 64 75 6C 65   "module1.o\0" (member name)
+0x15   31 2E 6F 00
+0x18   00 00               Padding to word-align
+0x1A   xx xx xx xx         Timestamp (8 bytes)
+0x1E   xx xx xx xx
```

### 4.5 Directory Size

- **Fixed at Creation**: Directory size is set when library is created
- **Expansion**: Can accommodate new members if unused entries exist
- **Reuse**: Deleted members (ChunkIndex = 0) can be reused
- **Wholesale Copy**: Expanding beyond initial size requires recreating library

### 4.6 Robustness Considerations

For compatibility with earlier (now obsolete) ALF versions:

**When Creating Libraries**:
- Always include valid timestamps in directory entries
- Zero-pad after name string to word boundary

**When Reading Libraries**:
- Don't assume data beyond name string exists
- Check DataLength vs name string length
- Don't assume padding bytes have specific values (beyond first NUL)

---

## 5. LIB_VRSN: Version Chunk Format

### 5.1 Purpose

Identifies the ALF format version.

### 5.2 Format

Single word (4 bytes):

```
+----------------------------------+
| Version Number                   | 4 bytes
+----------------------------------+
```

### 5.3 Current Version

- **Value**: 1
- **Type**: Word (4 bytes)
- **Endianness**: Must match chunk file and all library members

### 5.4 Usage

```c
uint32_t version;
// Read from LIB_VRSN chunk
if (version != 1) {
    // Unknown or incompatible version
}
```

---

## 6. LIB_DATA: Data Chunk Format

### 6.1 Purpose

Contains one library member (typically a complete AOF file).

### 6.2 Format

```
+----------------------------------+
| Complete AOF File                | Variable length
+----------------------------------+
```

### 6.3 Member Contents

- **Type**: Complete file in ARM Object Format (AOF)
- **Structure**: Chunk file with OBJ_HEAD, OBJ_AREA, OBJ_SYMT, etc.
- **Endianness**: Must match containing library file
- **Self-Contained**: Each member is a complete, valid AOF file
- **No Interpretation**: Library tools don't examine member internals (beyond symbol extraction)

### 6.4 Alternative Member Types

While typically AOF files, a `LIB_DATA` chunk could theoretically contain:
- Another chunk file format
- Another library (nested libraries)
- Arbitrary data

**Convention**: Object code libraries contain only AOF files.

### 6.5 Multiple LIB_DATA Chunks

- **One Per Member**: Each library member gets its own `LIB_DATA` chunk
- **Indexed**: ChunkIndex in directory points to specific `LIB_DATA`
- **Order**: No required order; directory provides mapping

---

## 7. OFL_SYMT: Object Library Symbol Table

### 7.1 Purpose

Provides fast lookup of which library member defines each external symbol.

**Only Present in Object Code Libraries** (libraries of AOF files).

### 7.2 Overall Structure

Identical format to `LIB_DIRY`:

```
+----------------------------------+
| Symbol Entry 1                   | Variable length (multiple of 4)
+----------------------------------+
| Symbol Entry 2                   | Variable length (multiple of 4)
+----------------------------------+
...
| Symbol Entry N                   | Variable length (multiple of 4)
+----------------------------------+
```

### 7.3 Symbol Entry Format

| Offset | Field | Type | Description |
|--------|-------|------|-------------|
| +0x00 | **ChunkIndex** | Word | Index of `LIB_DATA` chunk defining this symbol |
| +0x04 | **EntryLength** | Word | Total bytes in this entry (multiple of 4) |
| +0x08 | **DataLength** | Word | Bytes used in Data field (multiple of 4) |
| +0x0C | **Data** | Variable | Symbol name + padding (NO timestamp) |

### 7.4 Key Differences from LIB_DIRY

1. **No Timestamp**: Symbol entries do NOT contain timestamps (unlike directory entries)
2. **Symbol Names Only**: Data contains only external symbol name and padding
3. **Multiple Symbols Per Member**: Many symbols can reference same ChunkIndex
4. **External Symbols Only**: Only exported/global symbols from AOF members

### 7.5 Data Field Contents

1. **External Symbol Name**: Zero-terminated string
   - **Characters**: Printable characters (implementation-defined for 128-255)
   - **Terminator**: NUL (0x00) byte
   - **Example**: `"printf\0"`

2. **Padding**: 1-4 NULL bytes
   - **Purpose**: Align entry to 4-byte boundary
   - **Values**: Should be 0x00, but only first NUL is required

### 7.6 Example Symbol Entry

```
Offset  Content             Description
+0x00   05 00 00 00         ChunkIndex = 5 (LIB_DATA chunk)
+0x04   10 00 00 00         EntryLength = 16 bytes
+0x08   10 00 00 00         DataLength = 16 bytes
+0x0C   70 72 69 6E 74 66   "printf\0"
+0x12   00
+0x13   00 00 00            Padding to word boundary
```

### 7.7 Symbol Table Creation

The librarian builds `OFL_SYMT` by:

1. **Scanning Each Member**: Read OBJ_SYMT chunk from each AOF member
2. **Extracting Exports**: Find symbols with "exported" flag set (bit 1 in symbol flags)
3. **Creating Entries**: Add entry for each exported symbol → member mapping
4. **Sorting** (optional): Sort entries alphabetically for faster lookup

### 7.8 Linker Usage

When linking with a library:

1. **Identify Undefined Symbols**: Symbols referenced but not defined in object files
2. **Search OFL_SYMT**: Look for symbol name in library's symbol table
3. **Load Member**: If found, load corresponding `LIB_DATA` chunk (AOF member)
4. **Recursive**: Newly loaded member may reference more symbols, repeat process
5. **Weak Symbols**: Ignored when deciding which members to load

---

## 8. Time Stamps

ALF uses a specific timestamp format for tracking modifications.

### 8.1 Time Stamp Format

A timestamp is **8 bytes (2 words)**:

```
+----------------------------------+
| High Word                        | 4 bytes
+----------------------------------+
| Low Word                         | 4 bytes
+----------------------------------+
```

### 8.2 Encoding

**Centisecond Count** (6 bytes total):
- **Epoch**: 00:00:00, 1st January 1900
- **Unit**: Centiseconds (1/100 second)
- **Range**: 0 to 2^48 - 1

**Microsecond Count** (2 bytes):
- **Unit**: Microseconds since last centisecond
- **Range**: 0 to 9999 (typically 0)

### 8.3 Word Layout

**High Word** (Most Significant 4 bytes):
```
Contains the most significant 4 bytes of the 6-byte centisecond count
```

**Low Word** (Least Significant 4 bytes):
```
Bits 31-16: Least significant 2 bytes of 6-byte centisecond count
Bits 15-0:  2-byte microsecond count (usually 0)
```

### 8.4 Byte Order

- **Endianness**: Must match chunk file endianness
- **Word Storage**: Each word in little-endian or big-endian as appropriate

### 8.5 Example

For a timestamp of **1st January 2000, 00:00:00.00**:

```
Centiseconds since 1900: approximately 3155673600 centiseconds
= 100 years * 365.25 days/year * 24 hours/day * 60 min/hour * 60 sec/min * 100 cs/sec
= 0x0000007526D67C00 (48-bit value)

Microseconds: 0

High Word: 0x00000075
Low Word:  0x26D67C00
```

### 8.6 Practical Considerations

- **Resolution**: Centisecond resolution is 10 milliseconds
- **Microseconds**: Usually 0 (rarely used)
- **Comparison**: Compare as 64-bit integer for chronological order
- **Epoch**: 1900 epoch predates Unix epoch (1970)

---

## 9. LIB_TIME: Library Time Stamp

### 9.1 Purpose

Records when the library was last modified.

### 9.2 Format

```
+----------------------------------+
| Time Stamp                       | 8 bytes (2 words)
+----------------------------------+
```

### 9.3 Contents

- **Type**: Time stamp (8 bytes)
- **Format**: As defined in section 8
- **Meaning**: Date/time of last library modification
- **Updated When**:
  - Adding a member
  - Removing a member
  - Replacing a member
  - Modifying directory

### 9.4 Usage

```c
struct TimeStamp {
    uint32_t high;  // High 4 bytes of centisecond count
    uint32_t low;   // Low 2 bytes of cs count + microseconds
};

// Check if library is newer than member
if (lib_time > member_time) {
    // Library modified after member
}
```

---

## 10. OFL_TIME: Symbol Table Time Stamp

### 10.1 Purpose

Records when the object library symbol table (`OFL_SYMT`) was last modified.

**Only Present in Object Code Libraries**.

### 10.2 Format

Identical to `LIB_TIME`:

```
+----------------------------------+
| Time Stamp                       | 8 bytes (2 words)
+----------------------------------+
```

### 10.3 Contents

- **Type**: Time stamp (8 bytes)
- **Format**: As defined in section 8
- **Meaning**: Date/time symbol table was last updated
- **Updated When**:
  - Adding member with exported symbols
  - Removing member with exported symbols
  - Rebuilding symbol table

### 10.4 Usage

The librarian can use `OFL_TIME` to determine if symbol table needs rebuilding:

```c
if (ofl_time < lib_time) {
    // Symbol table out of date, rebuild needed
    rebuild_symbol_table();
}
```

---

## 11. Library Operations

### 11.1 Creating a Library

**Steps** (performed by armlib or similar tool):

1. **Create Chunk File Header**:
   - Set ChunkFileId = 0xC3CBC6C5
   - Allocate max_chunks (e.g., 100 for initial directory entries)
   - Set num_chunks = 0

2. **Create LIB_VRSN Chunk**:
   - Write version = 1

3. **Create Empty LIB_DIRY**:
   - Allocate fixed-size directory
   - Initialize all ChunkIndex values to 0 (unused)
   - Set EntryLength and DataLength appropriately

4. **Create LIB_TIME**:
   - Set to current time

5. **For Object Libraries, Create OFL_SYMT and OFL_TIME**:
   - Empty symbol table initially
   - Set OFL_TIME to current time

6. **Update Chunk File Header**:
   - Set num_chunks
   - Fill in chunk entries (chunkId, offset, size)

### 11.2 Adding a Member

**Steps**:

1. **Validate Member**:
   - Check it's a valid AOF file
   - Verify endianness matches library

2. **Find Unused Directory Entry**:
   - Scan LIB_DIRY for ChunkIndex = 0
   - If none, library is full (directory expansion needed)

3. **Create LIB_DATA Chunk**:
   - Write complete AOF file contents
   - Update chunk file header with new chunk

4. **Update Directory Entry**:
   - Set ChunkIndex to new LIB_DATA chunk index
   - Set member name
   - Set timestamp to member's modification time

5. **Update Symbol Table** (Object Libraries):
   - Extract exported symbols from AOF member
   - Add entries to OFL_SYMT for each symbol
   - Update OFL_TIME

6. **Update Library Timestamp**:
   - Set LIB_TIME to current time

### 11.3 Extracting a Member

**Steps**:

1. **Search Directory**:
   - Scan LIB_DIRY for matching member name

2. **Get ChunkIndex**:
   - Read ChunkIndex from directory entry

3. **Locate LIB_DATA Chunk**:
   - Use ChunkIndex to find chunk in chunk file header
   - Read offset and size

4. **Extract Data**:
   - Seek to offset
   - Read size bytes
   - Write to output file (complete AOF file)

### 11.4 Deleting a Member

**Steps**:

1. **Find Member**:
   - Search LIB_DIRY for member name

2. **Mark Directory Entry Unused**:
   - Set ChunkIndex = 0
   - Entry becomes available for reuse

3. **Update Symbol Table** (Object Libraries):
   - Remove all symbol entries pointing to deleted member's ChunkIndex
   - Or mark them for lazy deletion

4. **Optional: Reclaim Space**:
   - LIB_DATA chunk remains in file (wasted space)
   - Rebuild library to actually remove data

5. **Update Timestamps**:
   - Set LIB_TIME to current time
   - Set OFL_TIME to current time

**Note**: Actual chunk data not removed until library is packed/rebuilt.

### 11.5 Listing Members

**Steps**:

1. **Scan LIB_DIRY**:
   - Read each directory entry

2. **For Each Non-Zero ChunkIndex**:
   - Extract member name from Data field
   - Extract timestamp
   - Display to user

3. **Optional: Show Symbol Count**:
   - Count OFL_SYMT entries for this ChunkIndex

### 11.6 Searching for Symbol

**Steps** (Linker Operation):

1. **Scan OFL_SYMT**:
   - Search for matching symbol name
   - Optional: Binary search if sorted

2. **If Found**:
   - Get ChunkIndex
   - Load corresponding LIB_DATA chunk
   - Link in as if it were an object file

3. **If Not Found**:
   - Symbol undefined
   - Continue searching other libraries or report error

---

## 12. Library Types

### 12.1 Generic Library

**Characteristics**:
- Contains LIB_DIRY, LIB_TIME, LIB_VRSN
- LIB_DATA chunks can contain any data
- No OFL_SYMT or OFL_TIME
- Members not required to be AOF

**Use Cases**:
- Arbitrary file collections
- Archive format
- Resource bundles

### 12.2 Object Code Library

**Characteristics**:
- Contains LIB_DIRY, LIB_TIME, LIB_VRSN
- Contains OFL_SYMT and OFL_TIME
- LIB_DATA chunks contain AOF files
- Symbol table indexes exported symbols

**Use Cases**:
- Standard function libraries (C library, math library)
- Reusable code modules
- Framework libraries

**Primary Use**: Input to ARM linker for selective linking

---

## 13. Linker Processing

### 13.1 Library Search Process

When linker encounters undefined symbol:

1. **Search Object Files First**:
   - Check all input object files
   - If found, resolve immediately

2. **Search Libraries in Order**:
   - For each library on linker command line:
     a. Search OFL_SYMT for symbol
     b. If found, note ChunkIndex
     c. If not found, continue to next library

3. **Load Required Member**:
   - If symbol found in step 2:
     a. Load LIB_DATA chunk at ChunkIndex
     b. Parse AOF file
     c. Add areas, symbols, relocations to link state

4. **Recursive Processing**:
   - Newly loaded member may reference more symbols
   - Repeat process for each new undefined symbol

5. **Multiple Passes**:
   - May require multiple passes through libraries
   - Continue until no new symbols resolved

### 13.2 Member Loading Decision

Load a library member if:
- Defines at least one currently undefined symbol
- Symbol is NOT weak (weak references don't trigger loading)
- Member hasn't already been loaded

Don't load a member if:
- All its exported symbols already defined
- Only weak references to its symbols
- Already loaded from this or another library

### 13.3 Library Order Matters

```bash
# Correct order: object files first, then libraries in dependency order
armlink main.o module.o -L. -lmylib -lc

# Incorrect: library before object that needs it
armlink -lmylib main.o  # May not find symbols from mylib
```

**Rule**: Libraries should be listed in order of decreasing dependency (most dependent first).

### 13.4 Optimization: Whole Library Loading

Some linkers support loading entire library:

```bash
armlink --whole-archive -lmylib main.o
```

- Loads ALL members regardless of symbol references
- Useful for initialization functions
- Larger output size

---

## 14. Example ALF File Structure

### 14.1 Simple Library with Two Members

```
Chunk File Header:
  ChunkFileId: 0xC3CBC6C5
  max_chunks: 20
  num_chunks: 7

  Chunk 0: LIB_DIRY at offset 100, size 128
  Chunk 1: LIB_TIME at offset 228, size 8
  Chunk 2: LIB_VRSN at offset 236, size 4
  Chunk 3: LIB_DATA at offset 240, size 500  (module1.o)
  Chunk 4: LIB_DATA at offset 740, size 600  (module2.o)
  Chunk 5: OFL_SYMT at offset 1340, size 64
  Chunk 6: OFL_TIME at offset 1404, size 8

LIB_DIRY (128 bytes):
  Entry 0:
    ChunkIndex: 3
    EntryLength: 32
    DataLength: 32
    Data: "module1.o\0" + padding + timestamp

  Entry 1:
    ChunkIndex: 4
    EntryLength: 32
    DataLength: 32
    Data: "module2.o\0" + padding + timestamp

  Entry 2-3: ChunkIndex = 0 (unused)

LIB_TIME (8 bytes):
  High: 0x00000075
  Low:  0x26D67C00

LIB_VRSN (4 bytes):
  Version: 1

LIB_DATA (chunk 3, 500 bytes):
  [Complete AOF file for module1.o]

LIB_DATA (chunk 4, 600 bytes):
  [Complete AOF file for module2.o]

OFL_SYMT (64 bytes):
  Entry 0:
    ChunkIndex: 3
    EntryLength: 16
    DataLength: 16
    Data: "func1\0" + padding

  Entry 1:
    ChunkIndex: 4
    EntryLength: 16
    DataLength: 16
    Data: "func2\0" + padding

  Entry 2:
    ChunkIndex: 4
    EntryLength: 20
    DataLength: 20
    Data: "global_var\0" + padding

OFL_TIME (8 bytes):
  High: 0x00000075
  Low:  0x26D67C00
```

### 14.2 Hexdump Excerpt

```
00000000: C5 C6 CB C3 14 00 00 00 07 00 00 00 4C 49 42 5F  ............LIB_
00000010: 44 49 52 59 64 00 00 00 80 00 00 00 4C 49 42 5F  DIRYd.......LIB_
00000020: 54 49 4D 45 E4 00 00 00 08 00 00 00 4C 49 42 5F  TIME........LIB_
00000030: 56 52 53 4E EC 00 00 00 04 00 00 00 4C 49 42 5F  VRSN........LIB_
00000040: 44 41 54 41 F0 00 00 00 F4 01 00 00 4C 49 42 5F  DATA........LIB_
00000050: 44 41 54 41 E4 02 00 00 58 02 00 00 4F 46 4C 5F  DATA....X...OFL_
00000060: 53 59 4D 54 3C 05 00 00 40 00 00 00 4F 46 4C 5F  SYMT<...@...OFL_
00000070: 54 49 4D 45 7C 05 00 00 08 00 00 00 ...
```

---

## 15. Validation and Error Checking

### 15.1 Library Validation

**Check at Load Time**:

1. **Chunk File Header**:
   - ChunkFileId = 0xC3CBC6C5 (or reversed)
   - max_chunks reasonable (< 1000)
   - num_chunks ≤ max_chunks

2. **Required Chunks Present**:
   - LIB_DIRY exists
   - LIB_TIME exists
   - LIB_VRSN exists

3. **Version Compatible**:
   - LIB_VRSN value = 1

4. **Directory Consistency**:
   - All ChunkIndex values valid (< num_chunks)
   - All referenced chunks exist and have correct type (LIB_DATA)

5. **Symbol Table** (if object library):
   - OFL_SYMT and OFL_TIME present together
   - All ChunkIndex values point to valid LIB_DATA chunks

6. **Endianness Consistent**:
   - All chunks same endianness
   - All AOF members same endianness

### 15.2 Member Validation

**When Loading LIB_DATA Chunk**:

1. **AOF File Valid**:
   - ChunkFileId = 0xC3CBC6C5
   - OBJ_HEAD chunk present
   - file_type = 0xC5E2D080

2. **Endianness Matches**:
   - Member endianness = library endianness

3. **Complete File**:
   - All chunks referenced in header exist
   - File size matches expectations

### 15.3 Common Errors

**Invalid ChunkIndex**:
- Value = 0 is special (unused entry)
- Value ≥ num_chunks is invalid
- Referenced chunk doesn't exist

**Endianness Mismatch**:
- Library little-endian, member big-endian (or vice versa)
- Results in corrupted data

**Corrupted Directory**:
- EntryLength not multiple of 4
- DataLength > EntryLength
- Name string not NUL-terminated

**Missing Symbol Table**:
- Library used for linking but no OFL_SYMT
- Can't perform selective linking

**Out-of-Date Symbol Table**:
- OFL_TIME < LIB_TIME
- Symbols may not reflect current members
- Rebuild needed

---

## 16. Tool Usage Examples

### 16.1 Creating a Library (armlib)

```bash
# Create new library
armlib -c mylib.lib module1.o module2.o module3.o

# Add members to existing library
armlib -r mylib.lib newmodule.o

# Replace member
armlib -r mylib.lib module1.o
```

### 16.2 Listing Library Contents

```bash
# List members
armlib -t mylib.lib

# Verbose listing with symbols
armlib -tv mylib.lib

# List symbols only
armlib -s mylib.lib
```

### 16.3 Extracting Members

```bash
# Extract specific member
armlib -x mylib.lib module1.o

# Extract all members
armlib -x mylib.lib
```

### 16.4 Deleting Members

```bash
# Delete member
armlib -d mylib.lib module1.o

# Delete multiple members
armlib -d mylib.lib module1.o module2.o
```

### 16.5 Linking with Libraries

```bash
# Link with library
armlink main.o -L. -lmylib -o output.aif

# Link with multiple libraries
armlink main.o -L/path/to/libs -lmylib -lutils -lc

# Explicit library path
armlink main.o /path/to/mylib.lib -o output.aif
```

### 16.6 Rebuilding Symbol Table

```bash
# Rebuild symbol table
armlib -s mylib.lib

# Rebuild entire library (pack/cleanup)
armlib -c newlib.lib $(armlib -t oldlib.lib)
```

---

## 17. Advanced Topics

### 17.1 Library Versioning

**Problem**: Library updates may break existing code

**Solutions**:

1. **Weak Symbols**:
   - Provide weak definitions in library
   - Allow user to override

2. **Multiple Versions**:
   - Keep multiple library versions
   - Different library names (libfoo.1.lib, libfoo.2.lib)

3. **Version Symbols**:
   - Encode version in symbol names
   - `func_v1`, `func_v2`

### 17.2 Library Ordering

**Problem**: Circular dependencies between libraries

**Solution**: List libraries multiple times or combine:

```bash
# Circular dependency
armlink main.o -la -lb -la

# Or combine libraries
armlib -c combined.lib $(armlib -t liba.lib) $(armlib -t libb.lib)
armlink main.o -lcombined
```

### 17.3 Incremental Library Updates

**Efficient Updates**:

1. **Add Members**: Just append new LIB_DATA chunks
2. **Delete Members**: Mark directory entry unused (ChunkIndex = 0)
3. **Replace**: Delete + Add (leaves gap)
4. **Periodic Rebuild**: Pack library to remove gaps

**Tradeoff**: File size vs. rebuild time

### 17.4 Cross-Platform Libraries

**Endianness**:
- Maintain separate libraries for little/big endian
- Or convert at link time (slower)

**Naming Convention**:
```
mylib_le.lib  (little-endian)
mylib_be.lib  (big-endian)
```

### 17.5 Library Search Paths

**Linker Search Order**:

1. Current directory
2. Paths from `-L` options (in order)
3. Default system library paths

**Example**:
```bash
armlink -L/usr/local/lib -L/opt/arm/lib main.o -lmylib
```

Searches:
1. `./libmylib.lib`
2. `/usr/local/lib/libmylib.lib`
3. `/opt/arm/lib/libmylib.lib`

---

## 18. Performance Considerations

### 18.1 Symbol Table Performance

**Linear Search**:
- Simple implementation
- O(n) lookup time
- Acceptable for small libraries (< 100 symbols)

**Sorted Symbol Table**:
- Binary search possible
- O(log n) lookup time
- Better for large libraries

**Hash Table** (not in ALF):
- Separate hash table file
- O(1) lookup time
- Extra complexity

### 18.2 Library Size Management

**Growth Over Time**:
- Deleted members leave gaps
- File size increases even if members removed

**Mitigation**:
```bash
# Rebuild library periodically
armlib -c packed.lib $(armlib -t old.lib)
```

### 18.3 Member Count Limits

**Theoretical Limit**:
- max_chunks in chunk file header
- Practical limit: hundreds to thousands

**Large Libraries**:
- Split into multiple libraries by functionality
- Or increase max_chunks when creating library

---

## 19. Format Constants and Magic Numbers

### 19.1 Key Constants

| Name | Value | Description |
|------|-------|-------------|
| CHUNK_FILE_ID | 0xC3CBC6C5 | Chunk file magic number |
| LIB_DIRY | "LIB_DIRY" | Directory chunk ID |
| LIB_TIME | "LIB_TIME" | Library timestamp chunk ID |
| LIB_VRSN | "LIB_VRSN" | Version chunk ID |
| LIB_DATA | "LIB_DATA" | Member data chunk ID |
| OFL_SYMT | "OFL_SYMT" | Object library symbol table ID |
| OFL_TIME | "OFL_TIME" | Symbol table timestamp ID |
| ALF_VERSION | 1 | Current ALF version number |
| EPOCH_YEAR | 1900 | Timestamp epoch year |

### 19.2 C Structure Definitions

```c
#pragma pack(push, 1)

// Chunk file header
typedef struct {
    uint32_t chunk_file_id;  // 0xC3CBC6C5
    uint32_t max_chunks;
    uint32_t num_chunks;
} ChunkFileHeader;

// Chunk entry (16 bytes)
typedef struct {
    char chunk_id[8];        // e.g., "LIB_DIRY"
    uint32_t offset;
    uint32_t size;
} ChunkEntry;

// Directory entry (variable)
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    // Followed by: name string, padding, timestamp (8 bytes)
} DirEntry;

// Timestamp (8 bytes)
typedef struct {
    uint32_t high;  // High 4 bytes of centisecond count
    uint32_t low;   // Low 2 bytes cs + 2 bytes microseconds
} TimeStamp;

// Symbol entry (variable)
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    // Followed by: symbol name string, padding
} SymbolEntry;

#pragma pack(pop)
```

---

## 20. Comparison with Other Library Formats

### 20.1 ALF vs. Unix ar Format

| Feature | ALF | Unix ar |
|---------|-----|---------|
| **Structure** | Chunk-based | Sequential archive |
| **Symbol Table** | Separate chunk (OFL_SYMT) | __.SYMDEF or ar index |
| **Member Deletion** | Mark unused, gap left | Remove from archive |
| **Timestamps** | Special format (1900 epoch) | Unix time (1970 epoch) |
| **Extensibility** | Easy (add chunks) | Limited |
| **Random Access** | Via chunk header | Via member headers |

### 20.2 ALF vs. Windows .lib Format

| Feature | ALF | Windows .lib |
|---------|-----|---------|
| **Format Base** | Chunk file | COFF/PE |
| **Symbol Index** | OFL_SYMT | Import library symbols |
| **Object Format** | AOF | COFF |
| **Toolchain** | ARM-specific | Microsoft/LLVM |

---

## 21. Conclusion

ARM Object Library Format (ALF) is a well-designed library format that:

**Strengths**:
- Simple chunk-based structure
- Efficient symbol lookup via OFL_SYMT
- Supports selective linking
- Flexible directory structure
- Extensible via additional chunks

**Use Cases**:
- Standard C library for ARM
- Math and utility libraries
- Framework libraries
- Code reuse across projects

**Modern Status**:
- Largely superseded by ELF archives on modern systems
- Still relevant for:
  - Legacy ARM systems (RISC OS, 3DO)
  - Embedded ARM development
  - Historical software preservation
  - Understanding ARM toolchain evolution

---

## Appendix A: Complete Example

### A.1 Creating and Using a Library

**Step 1: Create Object Files**

```c
// math_utils.c
int add(int a, int b) { return a + b; }
int multiply(int a, int b) { return a * b; }
```

```c
// string_utils.c
int string_length(const char *s) {
    int len = 0;
    while (*s++) len++;
    return len;
}
```

Compile:
```bash
armcc -c math_utils.c -o math_utils.o
armcc -c string_utils.c -o string_utils.o
```

**Step 2: Create Library**

```bash
armlib -c utils.lib math_utils.o string_utils.o
```

**Step 3: Use Library**

```c
// main.c
extern int add(int, int);
extern int string_length(const char *);

int main(void) {
    int result = add(5, 3);
    int len = string_length("hello");
    return result + len;
}
```

Link:
```bash
armcc -c main.c -o main.o
armlink main.o -L. -lutils -o program.aif
```

**Result**: Only `math_utils.o` and `string_utils.o` linked (not other library members if present).

---

## Appendix B: References

### Primary Documentation

1. **ARM DUI 0041C** - ARM Software Development Toolkit
   - Chapter 14: ARM Object Library Format
   - ARM Limited, 1997-1998

2. **RISC OS PRM** - Programmer's Reference Manual
   - Appendix D: Code File Formats
   - 3QD Developments Ltd, 2015

3. **ARM Technical Specifications**
   - 3DO Edition
   - Library Format Specification

### Related Formats

- **AOF**: ARM Object Format (object files)
- **AIF**: ARM Image Format (executables)
- **Chunk File Format**: Base format for AOF and ALF

### Tools

- **armlib**: ARM Object Librarian (creates/maintains ALF files)
- **armlink**: ARM Linker (uses ALF files)
- **armcc**: ARM C Compiler (generates AOF, uses ALF)

---

**Document Version**: 1.0
**Date**: 2026-02-07
**Status**: Complete Specification
