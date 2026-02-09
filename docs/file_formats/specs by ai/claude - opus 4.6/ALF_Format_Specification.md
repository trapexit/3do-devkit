# ARM Object Library Format (ALF) — Complete File Format Specification

## Overview

ARM Object Library Format (ALF) is the library file format used by the ARM linker (`armlink`) and the ARM object librarian (`armlib`). An ALF file is a collection of object files (in ARM Object Format, AOF) bundled together with a directory and symbol index for efficient linking. The linker can selectively extract individual object modules from a library based on unresolved symbol references.

ALF is layered on top of **Chunk File Format**, the same container format used by AOF. This shared foundation means that library files and object files have a consistent structure and can be processed with common tooling.

### Document Sources

This specification is synthesized from:

- ARM Software Development Toolkit Reference Guide (ARM DUI 0041C, Chapter 14, 1997/1998)
- RISC OS Programmer's Reference Manuals, Appendix D
- 3DO SDK Technical Documentation (ARM Technical Specifications)

---

## 1. Terminology

| Term       | Definition |
|------------|------------|
| **Byte**   | 8 bits, unsigned unless otherwise stated. Used to store flag bits or characters. |
| **Half Word** | 16 bits (2 bytes), usually unsigned. |
| **Word**   | 32 bits (4 bytes), usually unsigned. |
| **String** | A sequence of bytes terminated by a NUL (0x00) byte. The NUL is part of the string but is not counted in its length. Strings may be aligned on any byte boundary. |

---

## 2. Endianness (Byte Sex)

There are two sorts of ALF: **little-endian** and **big-endian**.

- **Little-endian ALF**: The least significant byte of a word or half-word has the lowest address. Used by DEC, Intel, and Acorn.
- **Big-endian ALF**: The most significant byte of a word or half-word has the lowest address. Used by IBM, Motorola, and Apple.

For data in a file, *address* means offset from the start of the file.

There is no guarantee that the endianness of an ALF file will be the same as the endianness of the system used to process it — the endianness of the file is always the same as the endianness of the target ARM system.

The two sorts of ALF cannot be meaningfully mixed. The ARM linker accepts inputs of either sex and produces an output of the same sex, but rejects inputs of mixed endianness.

---

## 3. Alignment

- Strings and bytes may be aligned on any byte boundary.
- ALF fields defined in this specification do not use half-words, and align words on 4-byte boundaries.
- Within the contents of an ALF file (within the data contained in `OBJ_AREA` chunks of member AOF files), the alignment of words and half-words is defined by the use to which ALF is being put. For all current ARM-based systems, alignment is strict (words on 4-byte boundaries, half-words on 2-byte boundaries).

---

## 4. Underlying Container — Chunk File Format

An ALF file is stored in Chunk File Format, the same container format used by AOF files. The chunk file structure is described here for completeness and as it applies specifically to ALF.

### 4.1 Chunk File Layout

```
+---------------------------+
| Chunk File Header         |
|   ChunkFileId (1 word)    |
|   maxChunks   (1 word)    |
|   numChunks   (1 word)    |
+---------------------------+
| Chunk Entry 0 (4 words)   |
| Chunk Entry 1 (4 words)   |
| ...                       |
| Chunk Entry N (4 words)   |
+---------------------------+
| Chunk Data 0              |
| Chunk Data 1              |
| ...                       |
+---------------------------+
```

### 4.2 Chunk File Header (Fixed Part — 3 words)

| Offset | Size   | Field         | Description |
|--------|--------|---------------|-------------|
| 0x00   | 1 word | ChunkFileId   | Magic number: `0xC3CBC6C5`. The endianness of the chunk file can be deduced from this value. If it reads as `0xC5C6CBC3`, each word value must be byte-reversed before use. |
| 0x04   | 1 word | maxChunks     | Number of chunk entries in the header. Fixed when the file is created. For an ALF file, this must be large enough to accommodate the directory, time stamp, version, all data chunks, and (for object libraries) the symbol table and its time stamp. |
| 0x08   | 1 word | numChunks     | Number of chunks currently in use (0 to maxChunks). Redundant — can be computed by scanning entries. |

### 4.3 Chunk Entry Format (4 words each)

Each entry in the chunk file header consists of four words:

| Offset | Size    | Field      | Description |
|--------|---------|------------|-------------|
| +0x00  | 8 bytes | chunkId    | An 8-byte identifier for the chunk's data type. This is an 8-byte field (not a 2-word field), so it has the same byte order independent of endianness. The 8 characters are stored in ascending address order, as if they formed part of a NUL-terminated string. |
| +0x08  | 1 word  | fileOffset | Byte offset within the file of the start of the chunk. Must be divisible by 4 (all chunks are word-aligned). A value of 0 indicates the chunk entry is unused. |
| +0x0C  | 1 word  | size       | Exact byte size of the chunk's contents (need not be a multiple of 4). |

### 4.4 Chunk Identification

The `chunkId` field is split into two 4-character parts:

- **First 4 characters**: A unique namespace identifier.
  - `LIB_` for library-specific chunks
  - `OFL_` for object-file-library-specific chunks
- **Last 4 characters**: Identify the specific chunk within the namespace.

---

## 5. ALF Chunks

An ALF file defines two sets of chunks: the base library chunks and optional object-code-library chunks.

### 5.1 Base Library Chunks

| Chunk      | Chunk Name  | Required     | Count    | Description |
|------------|-------------|--------------|----------|-------------|
| Directory  | `LIB_DIRY`  | Yes          | 1        | Directory of library members. |
| Time Stamp | `LIB_TIME`  | Yes          | 1        | Last modification time of the library. |
| Version    | `LIB_VRSN`  | Yes          | 1        | Library format version number. |
| Data       | `LIB_DATA`  | Yes (1+)     | 1 per member | Contains one library member each. |

### 5.2 Object Code Library Chunks (Additional)

| Chunk              | Chunk Name  | Required for OFL | Count | Description |
|--------------------|-------------|-------------------|-------|-------------|
| Symbol Table       | `OFL_SYMT`  | Yes               | 1     | External symbol index. |
| Symbol Table Time  | `OFL_TIME`  | Yes               | 1     | Last modification time of `OFL_SYMT`. |

### 5.3 Conventional Chunk Ordering

Conventionally, the first three chunks of an object library file are:

| Index | Chunk     |
|-------|-----------|
| 0     | `LIB_DIRY` |
| 1     | `LIB_TIME` |
| 2     | `LIB_VRSN` |
| 3+    | `LIB_DATA` (one per library member) |

For object code libraries, `OFL_SYMT` and `OFL_TIME` are also present (often at the end or interleaved, but the specific position is determined by the chunk index).

In all chunks, word values are stored with the same byte order as the target system. Strings are stored in ascending address order, which is independent of target byte order.

---

## 6. The Directory Chunk (`LIB_DIRY`)

The `LIB_DIRY` chunk contains a directory of the modules (members) in the library. Each member is stored in its own `LIB_DATA` chunk. The directory size is fixed when the library is created.

### 6.1 Directory Structure

The directory consists of a sequence of variable-length entries, each an integral number of words long. The number of entries is determined by the total size of the `LIB_DIRY` chunk.

### 6.2 Directory Entry Format

```
+---------------------------+
| ChunkIndex    (1 word)    |
+---------------------------+
| EntryLength   (1 word)    |
+---------------------------+
| DataLength    (1 word)    |
+---------------------------+
| Data          (variable)  |
+---------------------------+
```

| Offset | Size     | Field        | Description |
|--------|----------|--------------|-------------|
| +0x00  | 1 word   | ChunkIndex   | 0-origin index within the chunk file header of the corresponding `LIB_DATA` chunk. A value of **0** means the directory entry is **unused**. Conventionally ≥ 3 for OFL files (since indices 0-2 are `LIB_DIRY`, `LIB_TIME`, `LIB_VRSN`). |
| +0x04  | 1 word   | EntryLength  | Total size of this `LIB_DIRY` entry in bytes. Always a multiple of 4. Includes the 3 header words and the Data section. |
| +0x08  | 1 word   | DataLength   | Size of the Data section in bytes. Always a multiple of 4. |
| +0x0C  | variable | Data         | Contains (in order): a NUL-terminated string (the member name), optional additional data, and a 2-word time stamp (word-aligned). |

### 6.3 Data Section Contents

The Data section of each directory entry contains, in order:

1. **Member Name**: A NUL-terminated string identifying the library member. Should contain only ISO-8859 non-control characters (codes 0-31, 127, and 128+0-31 are excluded). Typically this is the filename from which the library member was created.

2. **Additional Data**: Any other information relevant to the library module. Often empty.

3. **Time Stamp**: A 2-word (8-byte), word-aligned time stamp. Its value is an encoded version of the last-modified time of the file from which the library member was created. See section 8 for the time stamp format.

### 6.4 Robustness Notes

For compatibility with earlier (now obsolete) versions of the ARM object library format:

- **Writers** should ensure that `LIB_DIRY` entries contain valid time stamps.
- **Readers** should not rely on any data beyond the end of the name string being present, unless the difference between `DataLength` and the name-string length allows for it. Even then, time stamp contents should be treated cautiously.
- **Writers** should pad with NUL (0) bytes.
- **Readers** should make no assumptions about the values of padding bytes beyond the first string-terminating NUL byte.

---

## 7. The Version Chunk (`LIB_VRSN`)

The version chunk contains a single word whose value is **1**.

```
+---------------------------+
| Version = 1   (1 word)    |
+---------------------------+
```

This identifies the library format version. The current and only defined version is 1.

---

## 8. Time Stamps

A library time stamp is a pair of words (8 bytes) encoding:

- A **6-byte count of centiseconds** since 00:00:00, 1st January 1900.
- A **2-byte count of microseconds** since the last centisecond (usually 0).

### 8.1 Time Stamp Layout

```
+-------------------------------------------+
| Word 0 (Most Significant)                 |
|   Bytes 5-2 of the centisecond count      |
|   (most significant 4 bytes of 6)         |
+-------------------------------------------+
| Word 1 (Least Significant)                |
|   Upper half: Bytes 1-0 of centisecond    |
|     count (least significant 2 bytes of 6)|
|   Lower half: Microsecond count           |
|     (usually 0)                           |
+-------------------------------------------+
```

#### Bit-Level Detail

**Word 0** (4 bytes): Contains the most significant 4 bytes (bytes 5, 4, 3, 2) of the 6-byte centisecond count.

**Word 1** (4 bytes):
- **Most significant half** (upper 16 bits): Contains the least significant 2 bytes (bytes 1, 0) of the 6-byte centisecond count.
- **Least significant half** (lower 16 bits): Contains the 2-byte microsecond count since the last centisecond (usually 0).

### 8.2 Byte Order

Time stamp words are stored in target system byte order. They must have the same endianness as the containing chunk file.

### 8.3 Epoch

The time stamp epoch is **00:00:00, 1st January 1900**. This is the same epoch used by RISC OS.

---

## 9. The Library Time Stamp Chunk (`LIB_TIME`)

The `LIB_TIME` chunk contains a single 2-word (8-byte) time stamp recording when the library was last modified.

```
+---------------------------+
| Time Stamp Word 0         |   (centiseconds MSW)
+---------------------------+
| Time Stamp Word 1         |   (centiseconds LSW + microseconds)
+---------------------------+
```

---

## 10. The Data Chunks (`LIB_DATA`)

Each `LIB_DATA` chunk contains one library member, as indexed by the `LIB_DIRY` chunk. There is one `LIB_DATA` chunk per library member.

### 10.1 Content

The content of a `LIB_DATA` chunk is the raw bytes of the library member. The endianness of this data is, by assumption, the same as the byte order of the containing library/chunk file.

No other interpretation is placed on the contents of a member by the library management tools. A member could itself be:

- A file in Chunk File Format
- An AOF object file
- Another library
- Any other data format

For **object code libraries**, each `LIB_DATA` chunk contains a complete AOF file (an ARM Object Format file, with its own chunk file header, `OBJ_HEAD`, `OBJ_AREA`, etc.).

---

## 11. Object Code Libraries

An **object code library** is a library file whose members are files in ARM Object Format (AOF). In addition to the base library chunks, an object code library contains two additional chunks that provide an external symbol index for efficient linking.

### 11.1 Additional Chunks

| Chunk                    | Chunk Name  | Description |
|--------------------------|-------------|-------------|
| External Symbol Table    | `OFL_SYMT`  | Index of external symbols defined by library members. |
| Symbol Table Time Stamp  | `OFL_TIME`  | Records when `OFL_SYMT` was last modified. |

### 11.2 The External Symbol Table (`OFL_SYMT`)

The `OFL_SYMT` chunk contains an entry for each external symbol defined by members of the library, together with the index of the `LIB_DATA` chunk containing the member that defines that symbol.

The `OFL_SYMT` chunk has exactly the same format as the `LIB_DIRY` chunk, except that:

- The Data section of each entry contains **only** a string (the name of an external symbol) and between 1 and 4 bytes of NUL padding.
- Entries do **not** contain time stamps.

#### OFL_SYMT Entry Format

```
+---------------------------+
| ChunkIndex    (1 word)    |
+---------------------------+
| EntryLength   (1 word)    |
+---------------------------+
| DataLength    (1 word)    |
+---------------------------+
| External Symbol Name      |   NUL-terminated string
| + NUL Padding (1-4 bytes) |   Pad to word boundary
+---------------------------+
```

| Offset | Size     | Field               | Description |
|--------|----------|---------------------|-------------|
| +0x00  | 1 word   | ChunkIndex          | 0-origin index within the chunk file header of the `LIB_DATA` chunk containing the AOF member that defines this symbol. |
| +0x04  | 1 word   | EntryLength         | Total size of this `OFL_SYMT` entry in bytes (an integral number of words). |
| +0x08  | 1 word   | DataLength          | Size of the External Symbol Name + Padding in bytes (an integral number of words). |
| +0x0C  | variable | External Symbol Name | NUL-terminated string naming the external symbol. |
| ...    | 1-4 bytes | Padding            | NUL bytes to pad to a word boundary. |

### 11.3 Symbol Table Resolution

When the linker encounters an unresolved external symbol reference during linking, it searches the `OFL_SYMT` chunk for a matching symbol name. If found, the corresponding `ChunkIndex` identifies which `LIB_DATA` chunk (and hence which AOF member) to load to satisfy the reference. Loading that member may introduce additional unresolved references, which triggers further searching.

### 11.4 The Symbol Table Time Stamp (`OFL_TIME`)

The `OFL_TIME` chunk records when the `OFL_SYMT` chunk was last modified. It has the same format as the `LIB_TIME` chunk — a single 2-word (8-byte) time stamp.

```
+---------------------------+
| Time Stamp Word 0         |
+---------------------------+
| Time Stamp Word 1         |
+---------------------------+
```

---

## 12. Complete Binary Layout Summary

### 12.1 Overall ALF File Structure

```
Offset   Content
------   -------
0x00     Chunk File Header
           ChunkFileId = 0xC3CBC6C5     (1 word)
           maxChunks                     (1 word)
           numChunks                     (1 word)
         Chunk Entries (4 words each × maxChunks)
           Entry 0: LIB_DIRY
           Entry 1: LIB_TIME
           Entry 2: LIB_VRSN
           Entry 3: LIB_DATA (member 0)
           Entry 4: LIB_DATA (member 1)
           ...
           Entry N: OFL_SYMT            (object libraries only)
           Entry N+1: OFL_TIME          (object libraries only)

...      Chunk Data (at offsets specified by chunk entries)
           LIB_DIRY chunk data
           LIB_TIME chunk data (8 bytes)
           LIB_VRSN chunk data (4 bytes)
           LIB_DATA chunk 0 (complete AOF file)
           LIB_DATA chunk 1 (complete AOF file)
           ...
           OFL_SYMT chunk data          (object libraries only)
           OFL_TIME chunk data (8 bytes) (object libraries only)
```

### 12.2 Chunk File Header

```
Offset   Size      Field
------   ----      -----
0x00     4 bytes   ChunkFileId = 0xC3CBC6C5
0x04     4 bytes   maxChunks
0x08     4 bytes   numChunks
0x0C     16×N      Chunk Entries (4 words each, N = maxChunks)
```

### 12.3 Per Chunk Entry

```
Offset   Size      Field
------   ----      -----
+0x00    8 bytes   chunkId (e.g., "LIB_DIRY", "LIB_DATA", "OFL_SYMT")
+0x08    4 bytes   fileOffset (byte offset in file, multiple of 4)
+0x0C    4 bytes   size (exact byte size of chunk data)
```

### 12.4 LIB_DIRY Entry

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   ChunkIndex (0-origin, 0 = unused)
+0x04    4 bytes   EntryLength (total entry size, multiple of 4)
+0x08    4 bytes   DataLength (data section size, multiple of 4)
+0x0C    variable  Member Name (NUL-terminated string)
         variable  Optional additional data
         8 bytes   Time Stamp (2 words, word-aligned)
         variable  NUL padding to word boundary
```

### 12.5 LIB_VRSN

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Version = 1
```

### 12.6 LIB_TIME / OFL_TIME

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   Time Stamp Word 0 (MSW of centisecond count)
+0x04    4 bytes   Time Stamp Word 1 (LSW of centisecond + microseconds)
```

### 12.7 LIB_DATA

```
Offset   Size      Field
------   ----      -----
+0x00    variable  Raw member data (typically a complete AOF file)
```

### 12.8 OFL_SYMT Entry

```
Offset   Size      Field
------   ----      -----
+0x00    4 bytes   ChunkIndex (0-origin index of defining LIB_DATA chunk)
+0x04    4 bytes   EntryLength (total entry size, multiple of 4)
+0x08    4 bytes   DataLength (symbol name + padding size, multiple of 4)
+0x0C    variable  External Symbol Name (NUL-terminated string)
         1-4 bytes NUL padding to word boundary
```

---

## 13. Identifying an ALF File

An ALF file can be identified by:

1. **Chunk File Magic**: First word = `0xC3CBC6C5` (or byte-swapped equivalent).
2. **Chunk Names**: Look for `LIB_DIRY`, `LIB_TIME`, and `LIB_VRSN` chunk IDs. The presence of `LIB_` prefixed chunks (rather than `OBJ_` prefixed chunks) distinguishes an ALF file from an AOF file.
3. **Object Library**: The presence of `OFL_SYMT` and `OFL_TIME` chunks identifies the library as an object code library.
4. **Version**: The `LIB_VRSN` chunk should contain the value 1.

### 13.1 Distinguishing ALF from AOF

Both ALF and AOF files use Chunk File Format with the same `ChunkFileId` magic number. They are distinguished by their chunk names:

| File Type | Chunk Name Prefix |
|-----------|-------------------|
| AOF       | `OBJ_` (e.g., `OBJ_HEAD`, `OBJ_AREA`) |
| ALF       | `LIB_` (e.g., `LIB_DIRY`, `LIB_DATA`) plus optionally `OFL_` |

---

## 14. Library Construction and Usage

### 14.1 Creating a Library

Libraries are created using the ARM object librarian (`armlib`). The typical workflow is:

1. Compile/assemble source files to produce AOF object files.
2. Use `armlib` to create a new ALF file, adding the AOF files as members.
3. `armlib` creates the `LIB_DIRY` directory, populates `LIB_DATA` chunks with AOF file contents, builds the `OFL_SYMT` external symbol index, and sets time stamps.

### 14.2 Linking with a Library

When the ARM linker (`armlink`) processes an ALF library:

1. The linker reads the `OFL_SYMT` chunk to build a symbol-to-member mapping.
2. For each unresolved external symbol reference, the linker searches the symbol table.
3. If a match is found, the corresponding `LIB_DATA` chunk is loaded and processed as an AOF file.
4. This may introduce new unresolved references, causing additional library members to be loaded.
5. The process repeats until no more references can be resolved.

### 14.3 Library Member Independence

Each `LIB_DATA` chunk contains a complete, self-contained AOF file. Library members have no direct dependencies on each other within the library structure — all inter-module dependencies are resolved through the symbol table during linking.

---

## 15. Notes on the Chunk File Format Shared by AOF and ALF

The Chunk File Format is a shared container:

- **A chunk file can be copied without knowledge of its chunk contents.** Only the chunk file header needs to be understood.
- **Chunks may appear in any order.** The chunk file header provides random access to any chunk by its index.
- **Limited extensibility is built in.** The `maxChunks` field can be set larger than `numChunks` to allow additional chunks to be added without restructuring the file.
- **Chunk names are 8 bytes, stored as raw characters (not words).** This makes chunk names endianness-independent.
- **The endianness of the file can be determined from `ChunkFileId`.** If it reads as `0xC3CBC6C5`, the file matches the host byte order; if it reads as `0xC5C6CBC3`, all word values must be byte-swapped.
