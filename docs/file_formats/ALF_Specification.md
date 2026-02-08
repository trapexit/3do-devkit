# ARM Object Library Format (ALF) Specification

**Consolidated Technical Reference**

---

## 1. Introduction

ARM Object Library Format (ALF), also referred to as Acorn Library Format, is the library file format used by the ARM linker (`armlink`) and the ARM librarian (`armlib`/`LibFile`). It provides a means of collecting multiple ARM Object Format (AOF) files into a single indexed archive for efficient linking.

Language processors such as the ARM C compiler (`armcc`) and the ARM Assembler (`armasm`/`ObjAsm`) generate processed code output as AOF files. An ALF file is constructed from a set of AOF files by the librarian tool. The ARM linker accepts both AOF and ALF files as input, producing an executable image in AIF (ARM Image Format) or another output format.

ALF is layered on **Chunk File Format**, the same container format used by AOF. This provides a simple and efficient means of accessing and updating distinct chunks of data within a single file.

The base library format defines four chunk classes. The **Object Library Format** (OFL) — the specialised form used for libraries of AOF object files — adds two additional chunks for symbol indexing, enabling the linker to quickly locate which library member defines a given external symbol without scanning all member files.

---

## 2. Terminology

| Term | Definition |
|------|-----------|
| **Byte** | 8 bits, unsigned unless stated otherwise. Usually used to store flag bits or characters. |
| **Half word** | 16 bits (2 bytes), usually unsigned. In little-endian format, the least significant byte has the lowest address. |
| **Word** | 32 bits (4 bytes), usually unsigned. In little-endian format, the least significant byte has the lowest address. Must be aligned on a 4-byte boundary. |
| **String** | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL is part of the string but is not counted in the string's length. May be aligned on any byte boundary. |

---

## 3. Endianness and Alignment

### 3.1 Byte Order

There are two sorts of ALF: **little-endian** and **big-endian**.

- **Little-endian** (DEC/Intel/Acorn): the least significant byte of a word or half word has the lowest address.
- **Big-endian** (IBM/Motorola/Apple): the most significant byte of a word or half word has the lowest address.

For data in a file, *address* means offset from the start of the file.

The endianness of an ALF file always matches the endianness of the target ARM system. There is no guarantee that the file endianness matches the host system's byte order.

The two byte orders cannot be meaningfully mixed (the target system cannot have mixed endianness). The ARM linker accepts inputs of either byte order and produces output of the same kind, but rejects inputs of mixed endianness.

### 3.2 Alignment

- Strings and bytes may be aligned on any byte boundary.
- ALF fields defined in this specification make no use of half words and align words on 4-byte boundaries.
- Within the contents of an ALF file (inside `LIB_DATA` chunks containing AOF data), the alignment of words and half words is defined by the contained data format. For all current ARM-based systems, alignment is strict: words on 4-byte boundaries, half words on 2-byte boundaries.

---

## 4. Chunk File Format

ALF uses the same Chunk File Format as AOF. A chunk file consists of a header and one or more named chunks of data. A chunk is accessed through the header, which contains the number, size, location, and identity of each chunk.

The size of the header may vary between different chunk files but is fixed for each file. Not all entries in a header need be used, allowing limited expansion of the number of chunks without a wholesale copy. A chunk file can be copied without knowledge of the contents of its chunks.

### 4.1 Chunk File Header

#### Fixed Part (3 words, 12 bytes)

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| 0 | ChunkFileId | 4 | Magic number: `0xC3CBC6C5`. The endianness can be deduced from this value. If it appears as `0xC5C6CBC3` when read as a word, each word value must be byte-reversed before use. |
| 4 | maxChunks | 4 | Maximum number of chunk entries in the header. Fixed when the file is created. |
| 8 | numChunks | 4 | Number of chunks currently used (0 to maxChunks). Redundant — can be computed by scanning the entries. |

#### Chunk Entries (4 words per entry, 16 bytes)

Starting at offset 12, there are `maxChunks` entries:

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| +0 | chunkId | 8 | An 8-byte field identifying the chunk contents. This is an 8-byte field (not a 2-word field), so it has the same byte order independent of endianness. The 8 characters are stored in ascending address order, as if forming part of a NUL-terminated string, independently of endianness. The first 4 bytes contain a unique authority-allocated domain name; the remaining 4 bytes identify the component. |
| +8 | fileOffset | 4 | Byte offset within the file of the start of the chunk. Must be divisible by 4. A value of **0** indicates the entry is **unused**. |
| +12 | size | 4 | Exact byte size of the chunk's contents (which need not be a multiple of 4). |

### 4.2 ChunkId Domain Names

| Domain | Used By |
|--------|---------|
| `LIB_` | Base library format chunks |
| `OFL_` | Additional chunks specific to Object Code Libraries |
| `OBJ_` | ARM Object Format (AOF) chunks (within library member data) |

---

## 5. Library File Structure

### 5.1 Chunk Types

Each piece of a library file is stored in a separate, identifiable chunk:

| Chunk | chunkId | Required | Description |
|-------|---------|----------|-------------|
| Directory | `LIB_DIRY` | Yes | Index of all library members |
| Time Stamp | `LIB_TIME` | Yes | Last modification time of the library |
| Version | `LIB_VRSN` | Yes | Library format version number |
| Data | `LIB_DATA` | Yes (×N) | One chunk per library member |
| Symbol Table | `OFL_SYMT` | OFL only | External symbol index |
| Symbol Table Time Stamp | `OFL_TIME` | OFL only | Last modification time of the symbol table |

There may be many `LIB_DATA` chunks in a library, one for each library member.

In all chunks, word values are stored with the same byte order as the target system. Strings are stored in ascending address order, which is independent of target byte order.

### 5.2 Conventional Chunk Ordering

The first three chunks of an OFL file are conventionally:

| Index | Chunk | Notes |
|-------|-------|-------|
| 0 | `LIB_DIRY` | Library directory |
| 1 | `LIB_TIME` | Library modification timestamp |
| 2 | `LIB_VRSN` | Version (value = 1) |
| 3+ | `LIB_DATA` | One per member |
| N+1 | `OFL_SYMT` | External symbol table |
| N+2 | `OFL_TIME` | Symbol table timestamp |

Because the first three indices (0, 1, 2) are occupied by the directory, timestamp, and version chunks, `ChunkIndex` values in `LIB_DIRY` and `OFL_SYMT` entries are conventionally ≥ 3.

---

## 6. LIB_DIRY — Library Directory

The `LIB_DIRY` chunk contains a directory of all modules (members) in the library, each of which is stored in a `LIB_DATA` chunk. The directory size is fixed when the library is created. The directory consists of a sequence of variable-length entries, each an integral number of words long. The number of directory entries is determined by the size of the `LIB_DIRY` chunk.

### 6.1 Directory Entry Format

| Field | Size | Description |
|-------|------|-------------|
| ChunkIndex | 4 | Zero-origin index within the chunk file header of the corresponding `LIB_DATA` chunk. A value of **0** means the directory entry is **unused** (available for reuse). Conventionally ≥ 3. |
| EntryLength | 4 | Total number of bytes in this `LIB_DIRY` entry. Always a multiple of 4. |
| DataLength | 4 | Number of bytes used in the Data section of this entry. Also a multiple of 4. |
| Data | variable | See section 6.2 for contents. |

### 6.2 Data Section Contents

The Data section of each `LIB_DIRY` entry contains, in order:

1. **Member Name** — a NUL-terminated string (the name of the library member). Should contain only ISO-8859 non-control characters (codes 0-31, 127, and 128+0-31 are excluded). Typically this is the name of the file from which the library member was created.

2. **Optional Additional Information** — any other data relevant to the library module. Often empty.

3. **Time Stamp** — a two-word (8 bytes), word-aligned time stamp. Its value is an encoded version of the last-modified time of the file from which the library member was created. See section 8 for the time stamp format.

### 6.3 Directory Entry Layout Diagram

```
+-------------------+
| ChunkIndex        |  4 bytes — 0-origin index to LIB_DATA chunk (0 = unused)
+-------------------+
| EntryLength       |  4 bytes — total entry size in bytes (multiple of 4)
+-------------------+
| DataLength        |  4 bytes — used data size in bytes (multiple of 4)
+-------------------+
| Member Name       |  NUL-terminated string (ISO-8859 non-control chars)
| (NUL padding)     |  Padding bytes to maintain alignment
+-------------------+
| Optional Info     |  Additional module-specific data (often empty)
+-------------------+
| Time Stamp        |  8 bytes (2 words) — see Time Stamps section
+-------------------+
```

### 6.4 Compatibility Notes

To ensure maximum robustness with respect to earlier, now obsolete, versions of the ARM Object Library Format:

- **Writers:** Applications which create libraries or library members should ensure that the `LIB_DIRY` entries they create contain valid time stamps.
- **Readers:** Applications which read `LIB_DIRY` entries should not rely on any data beyond the end of the name string being present, unless the difference between the `DataLength` field and the name-string length allows for it. Even then, the contents of a time stamp should be treated cautiously and not assumed to be sensible.
- **Padding:** Applications which write `LIB_DIRY` or `OFL_SYMT` entries should ensure that padding is done with NUL (`0x00`) bytes. Applications that read these entries should make no assumptions about the values of padding bytes beyond the first, string-terminating NUL byte.

---

## 7. LIB_VRSN — Version Chunk

The version chunk contains a single word:

| Field | Size | Value | Description |
|-------|------|-------|-------------|
| Version | 4 | `1` | Library format version number |

---

## 8. Time Stamps

### 8.1 Time Stamp Format

A library time stamp is a pair of words (8 bytes total) encoding:

- A **6-byte count** of centiseconds (hundredths of a second) since 00:00:00, 1st January 1900
- A **2-byte count** of microseconds since the last centisecond (usually 0)

### 8.2 Word Layout

| Word | Contents |
|------|----------|
| **First word** (most significant) | Most significant 4 bytes of the 6-byte centisecond count |
| **Second word** (least significant) | Upper half: least significant 2 bytes of the 6-byte centisecond count. Lower half: 2-byte microsecond count (usually 0). |

### 8.3 Byte-Level Layout

```
First Word:                              Second Word:
+--------+--------+--------+--------+   +--------+--------+--------+--------+
| CS[5]  | CS[4]  | CS[3]  | CS[2]  |   | CS[1]  | CS[0]  | us[1]  | us[0]  |
+--------+--------+--------+--------+   +--------+--------+--------+--------+
  MSB of centisecond count                 LSB of CS       microseconds
```

Where:
- `CS[5]` is the most significant byte of the centisecond count
- `CS[0]` is the least significant byte of the centisecond count
- `us[1]` is the most significant byte of the microsecond count
- `us[0]` is the least significant byte of the microsecond count

Time stamp words are stored in target system byte order. They must have the same endianness as the containing chunk file.

### 8.4 LIB_TIME — Library Time Stamp

The `LIB_TIME` chunk contains a single two-word (8-byte) time stamp recording when the library was last modified.

---

## 9. LIB_DATA — Data Chunks

Each `LIB_DATA` chunk contains one library member, identified by the corresponding `LIB_DIRY` entry. The endianness of the member data is, by assumption, the same as the byte order of the containing library/chunk file.

No interpretation is placed on the contents of a member by the library management tools. A member could be:

- A file in chunk file format
- An AOF object file
- Another library file
- Any other opaque data

For an **Object Code Library** (OFL), each member is an AOF file.

---

## 10. Object Code Libraries (OFL)

An Object Code Library is a library file whose `LIB_DATA` members are files in ARM Object Format (AOF). Object code libraries contain two additional chunks beyond the base library format:

- `OFL_SYMT` — external symbol table
- `OFL_TIME` — symbol table modification timestamp

### 10.1 OFL_SYMT — External Symbol Table

The external symbol table contains an entry for each external symbol defined by members of the library, together with the index of the chunk containing the member that defines that symbol. This enables the linker to quickly locate which library member defines a given symbol without scanning all member AOF files.

#### OFL_SYMT Entry Format

The `OFL_SYMT` chunk has exactly the same structure as the `LIB_DIRY` chunk, with two differences:

1. The Data section contains **only** a NUL-terminated symbol name string plus 1-4 bytes of NUL padding (to word-align the entry).
2. Entries do **not** contain time stamps.

| Field | Size | Description |
|-------|------|-------------|
| ChunkIndex | 4 | Zero-origin index of the `LIB_DATA` chunk containing the member that defines this symbol. |
| EntryLength | 4 | Total size of this `OFL_SYMT` entry in bytes (must be a multiple of 4). |
| DataLength | 4 | Size of the External Symbol Name + NUL padding in bytes (must be a multiple of 4). |
| External Symbol Name | variable | NUL-terminated string naming the external symbol. |
| Padding | 1-4 | NUL bytes to align the entry to a word boundary. |

#### OFL_SYMT Entry Layout Diagram

```
+-------------------+
| ChunkIndex        |  4 bytes — index to LIB_DATA chunk of defining member
+-------------------+
| EntryLength       |  4 bytes — total entry size (multiple of 4)
+-------------------+
| DataLength        |  4 bytes — name + padding size (multiple of 4)
+-------------------+
| Symbol Name       |  NUL-terminated string
| NUL Padding       |  1-4 NUL bytes for word alignment
+-------------------+
```

### 10.2 OFL_TIME — Symbol Table Time Stamp

The `OFL_TIME` chunk records when the `OFL_SYMT` chunk was last modified. It has the same format as the `LIB_TIME` chunk: a single pair of words (8 bytes) encoding a time stamp as described in section 8.

---

## 11. Complete OFL File Structure

The following diagram shows the complete structure of a typical Object Code Library file:

```
+-------------------------------------------+
| Chunk File Header                         |
|   ChunkFileId = 0xC3CBC6C5               |
|   maxChunks                               |
|   numChunks                               |
|   Chunk Entry 0: LIB_DIRY                |
|     chunkId = "LIB_DIRY"                 |
|     fileOffset, size                      |
|   Chunk Entry 1: LIB_TIME                |
|     chunkId = "LIB_TIME"                 |
|     fileOffset, size                      |
|   Chunk Entry 2: LIB_VRSN                |
|     chunkId = "LIB_VRSN"                 |
|     fileOffset, size                      |
|   Chunk Entry 3: LIB_DATA (member 0)     |
|     chunkId = "LIB_DATA"                 |
|     fileOffset, size                      |
|   Chunk Entry 4: LIB_DATA (member 1)     |
|     ...                                   |
|   ...                                     |
|   Chunk Entry N: LIB_DATA (member N-3)   |
|   Chunk Entry N+1: OFL_SYMT              |
|     chunkId = "OFL_SYMT"                 |
|     fileOffset, size                      |
|   Chunk Entry N+2: OFL_TIME              |
|     chunkId = "OFL_TIME"                 |
|     fileOffset, size                      |
|   (unused entries: fileOffset = 0)        |
+-------------------------------------------+
| LIB_DIRY Data                             |
|   Entry for member 0:                     |
|     ChunkIndex = 3                        |
|     EntryLength, DataLength               |
|     "member_name_0\0" + [optional] + TS   |
|   Entry for member 1:                     |
|     ChunkIndex = 4                        |
|     EntryLength, DataLength               |
|     "member_name_1\0" + [optional] + TS   |
|   ...                                     |
|   (unused entries: ChunkIndex = 0)        |
+-------------------------------------------+
| LIB_TIME Data                             |
|   8 bytes: library modification timestamp |
+-------------------------------------------+
| LIB_VRSN Data                             |
|   4 bytes: version = 1                    |
+-------------------------------------------+
| LIB_DATA (member 0)                       |
|   Complete AOF file contents:             |
|     Chunk File Header (0xC3CBC6C5)       |
|     OBJ_HEAD, OBJ_AREA, OBJ_IDFN,       |
|     OBJ_SYMT, OBJ_STRT chunks            |
+-------------------------------------------+
| LIB_DATA (member 1)                       |
|   Complete AOF file contents              |
+-------------------------------------------+
| ...                                       |
+-------------------------------------------+
| LIB_DATA (member N-3)                     |
|   Complete AOF file contents              |
+-------------------------------------------+
| OFL_SYMT Data                             |
|   Entry for symbol "func_a":             |
|     ChunkIndex = 3 (member 0 defines it) |
|     EntryLength, DataLength               |
|     "func_a\0\0"                          |
|   Entry for symbol "func_b":             |
|     ChunkIndex = 4 (member 1 defines it) |
|     EntryLength, DataLength               |
|     "func_b\0\0\0"                        |
|   Entry for symbol "global_var":          |
|     ChunkIndex = 3 (member 0 defines it) |
|     EntryLength, DataLength               |
|     "global_var\0\0"                      |
|   ...                                     |
+-------------------------------------------+
| OFL_TIME Data                             |
|   8 bytes: symbol table mod timestamp     |
+-------------------------------------------+
```

---

## 12. Linker Usage

The ARM linker uses ALF/OFL libraries as follows:

### 12.1 Symbol Resolution

1. When the linker encounters an unresolved external symbol reference, it searches the `OFL_SYMT` chunk of each library in the link order.
2. When a matching symbol is found, the `ChunkIndex` in the `OFL_SYMT` entry identifies the `LIB_DATA` chunk containing the defining AOF member.
3. The linker loads the member (a complete AOF file) from the corresponding `LIB_DATA` chunk and processes it as if it were a separate input object file.
4. Loading a library member may introduce additional unresolved references, which may in turn cause further library members to be loaded.
5. **Weak** symbol references (bit 4 of symbol attributes in AOF) do **not** trigger library member inclusion. The linker ignores weak references when deciding which members to load from a library.

### 12.2 Library Management

The ARM librarian (`armlib`/`LibFile`) creates and maintains ALF/OFL files:

- **Adding members:** Insert new AOF files as `LIB_DATA` chunks, add `LIB_DIRY` entries, and update `OFL_SYMT` entries for all external symbols defined by the new member.
- **Replacing members:** Replace the `LIB_DATA` chunk contents, update the `LIB_DIRY` entry's time stamp, and rebuild `OFL_SYMT` entries.
- **Removing members:** Mark the `LIB_DIRY` entry as unused (ChunkIndex = 0), remove associated `OFL_SYMT` entries, and optionally reclaim the `LIB_DATA` chunk.
- **Rebuilding the symbol table:** Scan all members and regenerate the `OFL_SYMT` chunk.
- **Updating time stamps:** Update `LIB_TIME` when the library is modified; update `OFL_TIME` when the symbol table is modified.

---

## 13. Summary of Constants

| Constant | Value | Description |
|----------|-------|-------------|
| ChunkFileId | `0xC3CBC6C5` | Chunk file magic number |
| ChunkFileId (byte-reversed) | `0xC5C6CBC3` | Indicates opposite endianness |
| LIB_VRSN value | `1` | Library format version |
| Library chunk prefix | `LIB_` | First 4 bytes of base library chunkIds |
| OFL chunk prefix | `OFL_` | First 4 bytes of object library chunkIds |

### Chunk Identifiers

| chunkId (8 bytes) | Description |
|-------------------|-------------|
| `LIB_DIRY` | Library directory — index of all members |
| `LIB_TIME` | Library modification time stamp |
| `LIB_VRSN` | Library format version (always 1) |
| `LIB_DATA` | Library member data (one chunk per member) |
| `OFL_SYMT` | Object library external symbol table |
| `OFL_TIME` | Symbol table modification time stamp |

---

## 14. References

- ARM SDT Reference Guide, ARM DUI 0041C, Chapter 14: "ARM Object Library Format", ARM Limited, 1997-1998
- RISC OS Programmer's Reference Manuals, Volume 4, Appendix D: "Code file formats"
- ARM Technical Specifications (3DO edition), Chapter 3: "ARM Object Library Format"
