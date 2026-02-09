# ARM Library Format (ALF) — Complete File Format Specification

## Document Information

| Field | Value |
|-------|-------|
| **Format Name** | ARM Library Format (ALF), Acorn Library Format |
| **Primary Use** | Collections of AOF object files for linker input |
| **Key Features** | Chunk-based architecture, symbol tables, time stamps, member indexing |
| **Base Format** | Chunk File Format (same as AOF) |
| **Endianness** | Little-endian (target ARM byte order) |
| **Word Size** | 32 bits (4 bytes) |

---

## 1. Overview

ARM Library Format (ALF) is a container format for storing collections of ARM Object Format (AOF) files. ALF files, commonly called **object libraries** or **static libraries**, allow the linker (`armlink`) to selectively include only the object modules that resolve undefined symbols during linking.

ALF is built on the same Chunk File Format foundation as AOF, providing:
- Efficient random access to library members
- Symbol table for fast symbol resolution
- Time stamps for cache validation
- Support for both data and object code libraries

### 1.1 Design Goals

1. **Selective Linking**: Linker includes only needed modules
2. **Efficient Access**: Direct access to any member without scanning
3. **Symbol Discovery**: Rapid symbol lookup via external symbol table
4. **Cache Validation**: Time stamps for incremental builds
5. **Extensibility**: Support for both data and object code libraries

### 1.2 Relationship to AOF and the Linker

```
AOF Object Files (.o)
        ↓
   Librarian (armlib)
        ↓
   ALF Library File (.a, .lib)
        ↓
   Linker (armlink)
        ↓
   Resolved Symbols → Included Modules
   Unresolved → Continue search or error
```

### 1.3 Library Types

**Data Libraries:**
- Contain arbitrary data chunks
- No symbol table
- Accessed by position or named index

**Object Code Libraries:**
- Contain AOF object files
- Full external symbol table
- Selective linking via symbol resolution

---

## 2. Chunk File Format Foundation

Like AOF, ALF files use Chunk File Format as their underlying container. This section provides a quick reference for the shared foundation.

### 2.1 Chunk File Structure

```
+------------------+
| Chunk File      |
|     Header      |  Fixed: 3 words + 4 words per chunk
+------------------+
| Chunk 1         |  LIB_DIRY (directory)
+------------------+
| Chunk 2         |  LIB_TIME (timestamp)
+------------------+
| Chunk 3         |  LIB_VRSN (version)
+------------------+
| Chunk 4         |  LIB_DATA (member 1)
+------------------+
| Chunk 5         |  LIB_DATA (member 2)
+------------------+
| ...              |
+------------------+
| Chunk N         |  OFL_SYMT (object symbol table)
+------------------+
| Chunk N+1       |  OFL_TIME (symbol timestamp)
+------------------+
```

### 2.2 Chunk File Header

**Fixed Part (3 words):**

| Field | Description |
|-------|-------------|
| `ChunkFileId` | Magic number (0xC3CBC6C5) |
| `max_chunks` | Maximum chunks allocated |
| `num_chunks` | Currently used chunks |

**Chunk Entry (4 words per chunk):**

| Field | Description |
|-------|-------------|
| `chunkId` | 8-byte identifier |
| `file_offset` | Byte offset from file start |
| `size` | Chunk size in bytes |

---

## 3. ALF Chunk Types

### 3.1 Standard Library Chunks

| Chunk | Name | Required | Description |
|-------|------|----------|-------------|
| Directory | `LIB_DIRY` | Yes | Member directory |
| Version | `LIB_VRSN` | Yes | Library version |
| Time Stamp | `LIB_TIME` | Yes | Library modification time |
| Data | `LIB_DATA` | Yes | Library members (one or more) |
| Object Symbol | `OFL_SYMT` | Object only | External symbol table |
| Object Time | `OFL_TIME` | Object only | Symbol table timestamp |

### 3.2 Chunk Name Prefixes

| Prefix | File Type | Description |
|--------|-----------|-------------|
| `LIB_` | All libraries | Standard library chunks |
| `OFL_` | Object libraries | Object-specific chunks |

---

## 4. LIB_DIRY — Library Directory Chunk

The directory chunk contains a fixed-size index of all library members, enabling direct access to any member without scanning the entire file.

### 4.1 Directory Structure

```
+---------------------------+
| Directory Entry 1         |  Variable length (multiple of 4)
+---------------------------+
| Directory Entry 2         |
+---------------------------+
| ...                       |
+---------------------------+
| Directory Entry N         |
+---------------------------+
```

### 4.2 Directory Entry Format

Each entry contains information about one library member:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `chunk_index` | Chunk file header index of LIB_DATA |
| 0x04 | 4 | `entry_length` | Total entry size (multiple of 4) |
| 0x08 | 4 | `data_length` | Data section size (multiple of 4) |
| 0x0C | N | `data_section` | Member name + metadata + timestamp |

#### 4.2.1 Chunk Index

0-based index into the chunk file header:
- Conventionally, first three chunks are `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN`
- Value of 0 means directory entry is unused (deleted member)

#### 4.2.2 Entry Length

Total size of this directory entry in bytes:
- Must be a multiple of 4
- Includes all fields and padding

#### 4.2.3 Data Length

Number of bytes used in the data section:
- Also a multiple of 4
- May be less than entry_length (indicating padding)

### 4.3 Data Section Format

The variable-length data section contains:

```
+---------------------------+
| Member Name               |  Null-terminated string
+---------------------------+
| Metadata                  |  Optional additional info
+---------------------------+
| Timestamp (8 bytes)       |  Two-word time stamp
+---------------------------+
| Padding                   |  To make entry_length multiple of 4
+---------------------------+
```

#### 4.3.1 Member Name

- Null-terminated string
- ISO-8859 non-control characters only
- Maximum length: Limited by entry_length
- Uniquely identifies the member within the library

#### 4.3.2 Metadata

Optional field containing library-member-specific information:
- Often empty for simple libraries
- May contain version, type, or other identification
- Interpretation is library-specific

#### 4.3.3 Time Stamp

8-byte (two-word) time stamp recording when the member was added/modified.

---

## 5. Time Stamps

ALF uses a sophisticated time stamp format for precise modification tracking.

### 5.1 Time Stamp Format

```
+---------------------------+
| Word 0 (MSW)             |  Centiseconds since 1900-01-01 00:00:00
+---------------------------+
| Word 1 (LSW)             |  Lower 16 bits: lower centiseconds
|                           |  Upper 16 bits: microseconds
+---------------------------+
```

### 5.2 Encoding Details

**First Word (Most Significant):**
- Contains the upper 4 bytes of a 6-byte centisecond counter
- Centiseconds = 1/100th of a second
- Base date: 00:00:00 on January 1, 1900

**Second Word (Least Significant):**
```
+---------------------------+
| Upper 16 bits            |  Lower 2 bytes of centisecond count
+---------------------------+
| Lower 16 bits            |  Microseconds (usually 0)
+---------------------------+
```

### 5.3 Time Stamp Calculation

**Example: Converting to Unix time stamp**

```
UNIX_EPOCH = 1970-01-01 00:00:00 UTC
1900_EPOCH = 1900-01-01 00:00:00 UTC
DIFF_SECONDS = (1970 - 1900) × 365.25 × 24 × 60 × 60
             ≈ 2208988800 seconds
UNIX_TIME = (centiseconds / 100) - DIFF_SECONDS
```

### 5.4 Range and Precision

| Property | Value |
|----------|-------|
| Start date | January 1, 1900 |
| End date | Approximately 2036 (wraps at 2^48 centiseconds) |
| Resolution | 10 microseconds |
| Typical usage | 1 second precision (microseconds often 0) |

---

## 6. LIB_VRSN — Version Chunk

The version chunk identifies the ALF format version used by the library.

### 6.1 Structure

```
+---------------------------+
| Version Number (4 bytes)  |  Single word
+---------------------------+
```

### 6.2 Version Value

| Value | Meaning |
|-------|---------|
| 1 | Current ALF version |

The version number allows tools to handle legacy formats and backward compatibility.

---

## 7. LIB_DATA — Data Chunk

Data chunks contain the actual library members. Multiple LIB_DATA chunks may exist, one for each member.

### 7.1 Structure

```
+---------------------------+
| Member Contents           |  Variable size
+---------------------------+
```

### 7.2 Content Interpretation

| Library Type | Member Contents |
|--------------|----------------|
| Data library | Raw data bytes |
| Object library | Complete AOF object file |

### 7.3 Member Access

1. **Read LIB_DIRY** to get chunk_index for each member
2. **Look up chunk** in chunk file header using index
3. **Read LIB_DATA** at file_offset

### 7.4 Nested Chunks

A library member can itself be a chunk file:
- Enables libraries of libraries
- Recursive structure supported
- Nested chunk file header must be parsed separately

---

## 8. Object Code Library Extensions

Object code libraries (containing AOF files) include two additional chunks for symbol management.

### 8.1 OFL_SYMT — Object Symbol Table Chunk

The external symbol table enables fast symbol lookup during linking.

#### 8.1.1 Structure

```
+---------------------------+
| Symbol Entry 1            |  Variable length (multiple of 4)
+---------------------------+
| Symbol Entry 2            |
+---------------------------+
| ...                       |
+---------------------------+
| Symbol Entry N            |
+---------------------------+
```

#### 8.1.2 Symbol Entry Format

Identical to LIB_DIRY entry format:

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 | `chunk_index` | LIB_DATA chunk containing definition |
| 0x04 | 4 | `entry_length` | Total entry size (multiple of 4) |
| 0x08 | 4 | `data_length` | Symbol name length + padding |
| 0x0C | N | `symbol_name` | Null-terminated string |
| 0x0C+N| P | `padding` | NULL bytes to align entry_length |

#### 8.1.3 Symbol Name

- External symbol name as it appears in object files
- Null-terminated
- May include leading underscores or other linker-specific prefixes

#### 8.1.4 Symbol Lookup Process

1. **Linker receives** undefined symbol reference
2. **Searches OFL_SYMT** for matching name
3. **If found**, retrieves chunk_index
4. **Locates LIB_DATA** chunk using chunk_index
5. **Extracts AOF** member from library
6. **Adds to link** with other input objects

### 8.2 OFL_TIME — Object Symbol Time Stamp Chunk

Records when the symbol table was last modified.

#### 8.2.1 Structure

```
+---------------------------+
| Time Stamp (8 bytes)      |  Same format as LIB_TIME
+---------------------------+
```

#### 8.2.2 Update Semantics

- Updated whenever OFL_SYMT is modified
- Separate from LIB_TIME (library modification)
- Allows incremental rebuild of dependent libraries

---

## 9. Linking with Object Libraries

### 9.1 Linker Process

When the linker processes an object library:

```
1. Parse LIB_DIRY to build member index
2. Parse OFL_SYMT to build symbol index
3. For each undefined symbol:
   a. Search symbol index
   b. If found, note chunk_index
   c. Add member to link set
4. For each included member:
   a. Extract from LIB_DATA
   b. Parse as AOF
   c. Add to output image
```

### 9.2 Selective Inclusion

Only modules that resolve undefined symbols are included:

```c
// Module A defines: int foo();
void bar() { foo(); }

// Module B uses: extern int foo();
int baz() { return foo(); }

// If linking with only Module B:
//   foo is undefined → error or continue search

// If linking with both A and B:
//   foo is resolved by A → both A and B included
```

### 9.3 Symbol Resolution Order

When multiple libraries define the same symbol:
- First definition wins (link order dependent)
- Typically resolved by library order on command line
- Weak symbols can be overridden by strong definitions

---

## 10. Complete Library Structure

```
+---------------------------+
| Chunk File Header         |
|                           |
| ChunkFileId: 0xC3CBC6C5   |
| max_chunks:     8         |
| num_chunks:     6         |
+---------------------------+
| chunkId: "LIB_DIRY"       |  Index 0
| offset:       28          |
| size:         ~100       |
+---------------------------+
| chunkId: "LIB_TIME"       |  Index 1
| offset:       128        |
| size:         8          |
+---------------------------+
| chunkId: "LIB_VRSN"       |  Index 2
| offset:       136        |
| size:         4          |
+---------------------------+
| chunkId: "LIB_DATA "      |  Index 3 (Member 1)
| offset:       140        |
| size:         ~1000      |
+---------------------------+
| chunkId: "LIB_DATA "      |  Index 4 (Member 2)
| offset:       1140       |
| size:         ~800       |
+---------------------------+
| chunkId: "OFL_SYMT"       |  Index 5 (Object only)
| offset:       1940       |
| size:         ~500       |
+---------------------------+
| chunkId: "OFL_TIME"       |  Index 6 (Object only)
| offset:       2440       |
| size:         8          |
+---------------------------+

Chunk Contents:

LIB_DIRY:
+---------------------------+
| Entry 1: chunk_idx=3      |
|   entry_len=24            |
|   data_len=16             |
|   "module1.o\0"           |
|   timestamp               |
+---------------------------+
| Entry 2: chunk_idx=4      |
|   entry_len=24            |
|   data_len=16             |
|   "module2.o\0"           |
|   timestamp               |
+---------------------------+

LIB_TIME:
+---------------------------+
| 0xXXXXXXXX               |  MSW centiseconds
+---------------------------+
| 0x00000000               |  LSW (centi + microseconds)
+---------------------------+

LIB_VRSN:
+---------------------------+
| 0x00000001               |  Version 1
+---------------------------+

LIB_DATA (Index 3):
+---------------------------+
| AOF Object File Contents   |
| module1.o                 |
| (Complete AOF structure)   |
+---------------------------+

LIB_DATA (Index 4):
+---------------------------+
| AOF Object File Contents   |
| module2.o                 |
| (Complete AOF structure)   |
+---------------------------+

OFL_SYMT (Index 5):
+---------------------------+
| Symbol Entry: "foo"       |
|   chunk_idx=3             |
|   entry_len=20            |
|   data_len=8              |
|   "foo\0\0\0"             |
+---------------------------+
| Symbol Entry: "bar"       |
|   chunk_idx=3             |
|   entry_len=20            |
|   data_len=8              |
|   "bar\0\0\0"             |
+---------------------------+

OFL_TIME (Index 6):
+---------------------------+
| 0xXXXXXXXX               |  MSW centiseconds
+---------------------------+
| 0x00000000               |  LSW
+---------------------------+
```

---

## 11. Directory Entry Examples

### 11.1 Minimal Entry (16 bytes)

```
Offset  Content (hex)
------  --------------------------------
00-03   chunk_index = 3
04-07   entry_length = 16
08-0B   data_length = 8
0C-0F   "mod\0\0" + timestamp MSW
10-13   timestamp LSW
```

### 11.2 Entry with Metadata (24 bytes)

```
Offset  Content (hex)
------  --------------------------------
00-03   chunk_index = 4
04-07   entry_length = 24
08-0B   data_length = 16
0C-13   "utils\0\0\0" (member name)
14-15   metadata flags = 0x0001
16-17   reserved = 0
18-1B   timestamp MSW
1C-1F   timestamp LSW
```

---

## 12. Backward Compatibility

### 12.1 Robust Reading

When reading ALF files:

1. **Don't assume** data beyond name string exists
2. **Use data_length** to determine actual content size
3. **Handle deleted entries** (chunk_index = 0)
4. **Skip unknown chunks** gracefully
5. **Validate timestamps** are reasonable

### 12.2 Robust Writing

When creating ALF files:

1. **Always include** valid timestamps
2. **Pad with NULL** bytes (0x00)
3. **Use consistent** chunk ordering
4. **Update LIB_TIME** when modifying
5. **Update OFL_TIME** when symbol table changes

### 12.3 Version Handling

| Version | Changes |
|---------|---------|
| 1 | Initial ALF specification |
| Future | Reserved for extensions |

---

## 13. Library Management Tools

### 13.1 ARM Librarian (`armlib`)

**Command Syntax:**
```
armlib [options] -o library lib1 [lib2] ...
armlib [options] -t library [member...]
armlib [options] -x library member...
armlib [options] -r library member...
```

**Common Options:**

| Option | Description |
|--------|-------------|
| `-o file` | Output library file |
| `-t` | List table of contents |
| `-x` | Extract member |
| `-r` | Replace/add member |
| `-d` | Delete member |
| `-v` | Verbose output |

### 13.2 Creating a Library

```bash
# Compile source files
armcc -c module1.c -o module1.o
armcc -c module2.c -o module2.o

# Create library
armlib -o mylib.a module1.o module2.o

# List contents
armlib -t mylib.a

# Output:
# module1.o
# module2.o
```

### 13.3 Using Library in Linking

```bash
# Link with library
armlink main.o mylib.a -o program

# Library searched for undefined symbols
# Only needed modules are extracted
```

---

## 14. Comparison with Other Library Formats

| Feature | ALF | COFF | ELF |
|---------|-----|------|-----|
| Symbol table | Yes (OFL_SYMT) | Yes | Yes |
| Time stamps | Yes | Yes | Optional |
| Selective linking | Yes | Yes | Yes |
| Chunk-based | Yes | No | No |
| AOF integration | Native | Requires conversion | Requires conversion |
| RISC OS primary | Yes | No | Modern systems |

---

## 15. Implementation Guidelines

### 15.1 Reading ALF Files

1. **Read chunk file header**
2. **Detect endianness** from ChunkFileId
3. **Locate LIB_DIRY** (usually index 0)
4. **Build member index** from directory
5. **Locate OFL_SYMT** for object libraries
6. **Build symbol index** for linking

### 15.2 Writing ALF Files

1. **Collect members** to include
2. **Build string table** of member names
3. **Create LIB_DIRY** entries
4. **Write LIB_TIME** with current time
5. **Write LIB_VRSN** (= 1)
6. **Write LIB_DATA** chunks for members
7. **Write OFL_SYMT** for object libraries
8. **Write OFL_TIME** after symbol updates

### 15.3 Member Extraction

```c
// Pseudocode for extracting a member
ALFDirEntry *entry = find_member("module.o");
if (entry == NULL) error("not found");

ChunkHeader *chunk = get_chunk_header(entry->chunk_index);
FILE *member = fopen("module.o", "wb");

fseek(library_file, chunk->file_offset, SEEK_SET);
fread(buffer, 1, chunk->size, library_file);
fwrite(buffer, 1, chunk->size, member);

fclose(member);
```

---

## 16. Time Stamp Reference

### 16.1 Centisecond Calculation

```c
// Calculate centiseconds since 1900-01-01
uint64_t centiseconds_since_1900(int year, int month, int day,
                                   int hour, int minute, int second) {
    // Days from 1900 to given date
    uint64_t days = ...;
    
    // Convert to centiseconds
    uint64_t centis = days * 24 * 60 * 60 * 100;
    centis += hour * 60 * 60 * 100;
    centis += minute * 60 * 100;
    centis += second * 100;
    
    return centis;
}
```

### 16.2 Sample Time Stamps

| Date/Time | Centiseconds (hex) |
|-----------|-------------------|
| 1990-01-01 00:00:00 | 0x02C32660 |
| 1995-01-01 00:00:00 | 0x04F5C1E0 |
| 2000-01-01 00:00:00 | 0x07133800 |
| 2005-01-01 00:00:00 | 0x08CB8680 |
| 2010-01-01 00:00:00 | 0x0A6CDE00 |

---

## 17. References

1. ARM Software Development Toolkit Reference Guide (DUI 0041C)
2. ARM Software Development Toolkit User Guide (DUI 0040)
3. RISC OS Programmer's Reference Manuals
4. Chunk File Format specification

---

## Appendix A: Chunk Names Summary

| Chunk | 8-byte ID | Purpose |
|-------|-----------|---------|
| LIB_DIRY | `LIB_DIRY` | Member directory |
| LIB_TIME | `LIB_TIME` | Library timestamp |
| LIB_VRSN | `LIB_VRSN` | Version (= 1) |
| LIB_DATA | `LIB_DATA ` | Member data |
| OFL_SYMT | `OFL_SYMT` | Object symbol table |
| OFL_TIME | `OFL_TIME` | Symbol timestamp |

**Note:** LIB_DATA has trailing space to make 8 characters.

---

## Appendix B: Directory Entry Fields

```
Entry Layout:

Offset  Size  Content
------  -----  ----------------------------------------
0x00    4     chunk_index (0 = deleted/unused)
0x04    4     entry_length (multiple of 4)
0x08    4     data_length (≤ entry_length - 12)
0x0C    N     member name + NULL + metadata + timestamp
             (N = data_length - 8)
0x0C+N  P     padding to align entry_length (P bytes, P ≥ 0)
```

---

## Appendix C: Symbol Entry Fields

```
Symbol Entry Layout:

Offset  Size  Content
------  -----  ----------------------------------------
0x00    4     chunk_index (LIB_DATA containing definition)
0x04    4     entry_length (multiple of 4)
0x08    4     data_length (symbol name + NULL + padding)
0x0C    N     symbol name + NULL + padding
```

---

*Document Version: 1.0*
*Last Updated: Based on ARM SDT 2.50 (1998) and RISC OS PRMs documentation*
