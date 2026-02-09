# ARM Object Library Format (ALF) Specification

## 1. Overview

ARM Object Library Format (ALF) is the library file format used by the ARM linker (`armlink`) and the ARM object librarian (`armlib`). A library file contains a collection of separate but related pieces of data -- specifically, a set of AOF (ARM Object Format) object files -- stored together with indexing and metadata to allow efficient access.

The library file format is layered on **Chunk File Format**, the same container format used by AOF files. This provides a simple and efficient means of accessing and updating distinct chunks of data within a single file.

Language processors such as C compilers and assemblers generate AOF files. The `armlib` tool collects sets of AOF files into ALF libraries. The `armlink` linker accepts ALF libraries (alongside standalone AOF files) as input, loading only those library members needed to satisfy symbolic references.

---

## 2. Terminology

| Term      | Definition |
|-----------|------------|
| byte      | 8 bits, considered unsigned unless otherwise stated |
| half word | 16 bits (2 bytes), usually considered unsigned |
| word      | 32 bits (4 bytes), usually considered unsigned |
| string    | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL byte is part of the string but not counted in the string's length |
| address   | For data in a file, this means offset from the start of the file |

---

## 3. Byte Sex / Endianness

There are two sorts of ALF: **little-endian** and **big-endian**.

- **Little-endian:** The least significant byte of a word or half-word has the lowest address. Used by DEC, Intel, and Acorn.
- **Big-endian:** The most significant byte of a word or half-word has the lowest address. Used by IBM, Motorola, and Apple.

The endianness of an ALF file always matches the endianness of the target ARM system. There is no guarantee it matches the host system processing the file.

The two sorts of ALF cannot meaningfully be mixed. The ARM linker accepts inputs of either endianness and produces output of the same endianness, but rejects inputs of mixed endianness.

---

## 4. Alignment

- Strings and bytes may be aligned on any byte boundary.
- ALF header fields do not use half-words and align words on 4-byte boundaries.
- Within the contents of an ALF file (within the data contained in `OBJ_AREA` chunks of member AOF files), the alignment of words and half-words is defined by the use to which ALF is being put. For all current ARM-based systems, alignment is strict: words on 4-byte boundaries, half-words on 2-byte boundaries.

---

## 5. Chunk File Format

ALF files use the same Chunk File Format as AOF files. See the AOF Specification, Section 5.1, for full details of the chunk file header and entry format.

Key points:

- **ChunkFileId:** `0xC3CBC6C5` (determines endianness).
- **maxChunks:** Number of header entries, fixed at file creation.
- **numChunks:** Number of chunks currently in use (redundant, derivable by scanning).
- Each chunk entry: 8-byte `chunkId` + 1-word `fileOffset` + 1-word `size`.
- chunkId characters stored in ascending address order, independent of endianness.

For ALF (library) files, the chunk domain name is `"LIB_"`. For object library-specific additional chunks, the domain name is `"OFL_"`.

---

## 6. Library File Chunks

An ALF library file contains the following chunks:

| Chunk           | chunkId    | Required?       | Description |
|-----------------|------------|-----------------|-------------|
| Directory       | `LIB_DIRY` | Yes             | Directory of library members |
| Time stamp      | `LIB_TIME` | Yes             | Last modification timestamp of the library |
| Version         | `LIB_VSRN` | Yes             | Library format version |
| Data            | `LIB_DATA` | Yes (multiple)  | One per library member; contains the actual member data |
| Symbol table    | `OFL_SYMT` | Object libs only | External symbol index for the library |
| Symbol timestamp| `OFL_TIME` | Object libs only | Last modification time of the symbol table |

There may be **many** `LIB_DATA` chunks in a library, one for each library member.

In all chunks, word values are stored with the same byte order as the target system. Strings are stored in ascending address order, independent of target byte order.

### 6.1 Conventional Chunk Ordering

Conventionally, the first three chunks in an OFL (Object Format Library) file are:

1. `LIB_DIRY` (chunk index 0)
2. `LIB_TIME` (chunk index 1)
3. `LIB_VSRN` (chunk index 2)

Therefore, `LIB_DATA` chunks have indices starting at 3 or higher. An object format library additionally contains `OFL_SYMT` and `OFL_TIME` chunks.

---

## 7. LIB_DIRY - Library Directory

The `LIB_DIRY` chunk contains a directory of the modules (members) in the library. Its size is fixed when the library is created. The directory consists of a sequence of variable-length entries, each an integral number of words long. The number of entries is determined by the total size of the `LIB_DIRY` chunk.

### 7.1 Directory Entry Format

Each entry consists of:

| Field       | Size    | Description |
|-------------|---------|-------------|
| ChunkIndex  | 1 word  | 0-origin index within the chunk file header of the corresponding `LIB_DATA` chunk. A value of 0 indicates the directory entry is unused. Conventionally >= 3 for OFL files. |
| EntryLength | 1 word  | Total number of bytes in this directory entry. Always a multiple of 4. |
| DataLength  | 1 word  | Number of bytes used in the Data section of this entry. Also a multiple of 4. |
| Data        | variable| Contains (in order): a NUL-terminated member name string, optional additional information, and a 2-word word-aligned time stamp. |

### 7.2 Data Section Contents

The Data section of each directory entry contains, in order:

1. **Member name:** A zero-terminated string containing only ISO-8859 non-control characters (codes [0-31], 127, and 128+[0-31] are excluded). This is typically the name of the file from which the library member was created.
2. **Additional information:** Any other information relevant to the library module. Often empty.
3. **Time stamp:** A 2-word, word-aligned time stamp (see Section 9) encoding the last-modified time of the file from which the member was created.

### 7.3 Padding

- Applications creating `LIB_DIRY` entries must ensure padding is done with NUL (0) bytes.
- Applications reading entries should make no assumptions about the values of padding bytes beyond the first string-terminating NUL.
- Applications reading entries should not rely on data beyond the end of the name string being present, unless the difference between DataLength and the name string length allows for it.

---

## 8. LIB_VSRN - Library Version

The version chunk contains a single word whose value is `1`.

---

## 9. Time Stamps

A library time stamp is a pair of words (8 bytes) encoding:

- A **6-byte count of centiseconds** since 00:00:00, 1st January 1900.
- A **2-byte count of microseconds** since the last centisecond (usually 0).

### 9.1 Time Stamp Word Layout

```
Word 0 (most significant):
+------------------------------------------+
| MS 4 bytes of 6-byte centisecond count   |
+------------------------------------------+

Word 1 (least significant):
+---------------------+---------------------+
| LS 2 bytes of       | 2-byte microsecond  |
| centisecond count    | count (usually 0)   |
| (MSH of word)        | (LSH of word)       |
+---------------------+---------------------+
```

**Word 0:** Contains the most significant 4 bytes of the 6-byte centisecond count.

**Word 1:** The most significant half of this word contains the least significant 2 bytes of the 6-byte centisecond count. The least significant half contains the microsecond count (usually 0).

Time stamp words are stored in target system byte order and must have the same endianness as the containing chunk file.

---

## 10. LIB_TIME - Library Modification Timestamp

The `LIB_TIME` chunk contains a single 2-word (8-byte) time stamp recording when the library was last modified. Its format is as described in Section 9.

---

## 11. LIB_DATA - Library Member Data

Each `LIB_DATA` chunk contains one library member, indexed by the corresponding entry in the `LIB_DIRY` chunk.

### 11.1 Properties

- The endianness of the member data is, by assumption, the same as the byte order of the containing library/chunk file.
- No other interpretation is placed on the contents of a member by the library management tools.
- A member could itself be a file in chunk file format, or even another library.
- For object code libraries, each member is an AOF file.

---

## 12. Object Code Libraries

An **object code library** is an ALF library file whose members are files in ARM Object Format (AOF). Object code libraries contain two additional chunks beyond the base library chunks:

| Chunk            | chunkId    | Description |
|------------------|------------|-------------|
| Symbol table     | `OFL_SYMT` | External symbol index |
| Symbol timestamp | `OFL_TIME` | When the symbol table was last modified |

### 12.1 OFL_SYMT - External Symbol Table

The external symbol table contains an entry for each external symbol defined by members of the library, together with the index of the chunk containing the member that defines that symbol. This enables the linker to quickly locate which library member to load for a given unresolved symbol.

#### 12.1.1 OFL_SYMT Entry Format

The `OFL_SYMT` chunk has exactly the same format as the `LIB_DIRY` chunk, except that:

- The Data section of each entry contains **only** a string (the name of an external symbol) and between 1 and 4 bytes of NUL padding to reach a word boundary.
- OFL_SYMT entries do **not** contain time stamps.

| Field                | Size     | Description |
|----------------------|----------|-------------|
| ChunkIndex           | 1 word   | 0-origin index of the `LIB_DATA` chunk containing the member that defines this symbol. |
| EntryLength          | 1 word   | Size of this `OFL_SYMT` entry in bytes (an integral number of words). |
| DataLength           | 1 word   | Size of the External Symbol Name + Padding (an integral number of words). |
| External Symbol Name | variable | NUL-terminated string naming the external symbol. |
| Padding              | 1-4 bytes| NUL bytes to reach next word boundary. |

#### 12.1.2 Usage by the Linker

When the linker encounters an unresolved external reference, it searches the `OFL_SYMT` chunk for a matching symbol name. If found, the ChunkIndex identifies the `LIB_DATA` chunk containing the AOF member that defines the symbol. The linker then loads that member and processes it as if it were a standalone object file.

### 12.2 OFL_TIME - Symbol Table Timestamp

The `OFL_TIME` chunk records when the `OFL_SYMT` chunk was last modified. It has the same format as the `LIB_TIME` chunk: a 2-word (8-byte) time stamp as described in Section 9.

This timestamp allows tools to determine whether the symbol table is stale (i.e. whether library members have been added, removed, or modified since the symbol table was last rebuilt).

---

## 13. Library Module Inclusion by the Linker

The linker processes libraries differently from standalone object files:

- **Object files** in the input list appear in the output unconditionally, whether or not anything references them (though unused areas may be eliminated in some output formats).
- **Library members** are included if, and only if, some object file or some already-included library member makes a **non-weak** reference to a symbol they define.

### 13.1 Processing Order

1. First, the standalone object files are linked together, ignoring libraries. This typically produces a set of unresolved symbol references. Some may be weak references.
2. Then, libraries are processed in the order they appear in the input list:
   - The library's `OFL_SYMT` is searched for members containing definitions matching currently unsatisfied non-weak references.
   - Each matching member is loaded, satisfying some references and potentially creating new ones.
   - The search repeats until no further members are loaded from that library.
3. Each library is processed in turn. A reference from a member of a later library to a member of an earlier library cannot be satisfied. Circular dependencies between libraries are forbidden.

It is an error if any non-weak reference remains unsatisfied at the end of a link step (except when producing partially-linked relocatable AOF output).

---

## 14. Robustness Notes

For maximum robustness with earlier (now obsolete) versions of ALF:

1. **Writers** of libraries should ensure that `LIB_DIRY` entries contain valid time stamps.
2. **Readers** of `LIB_DIRY` entries should not rely on data beyond the end of the name string being present unless DataLength permits it. Even then, time stamp contents should be treated cautiously.
3. **Writers** of `LIB_DIRY` or `OFL_SYMT` entries must pad with NUL (0) bytes.
4. **Readers** should make no assumptions about padding byte values beyond the first string-terminating NUL.

---

## 15. Complete ALF File Structure Example

A typical object code library file has the following chunk file layout:

```
Chunk File Header:
  ChunkFileId:  0xC3CBC6C5
  maxChunks:    N (e.g. 50, to allow for many LIB_DATA members)
  numChunks:    varies

  Entry 0:  chunkId="LIB_DIRY"  offset=...  size=...
  Entry 1:  chunkId="LIB_TIME"  offset=...  size=8
  Entry 2:  chunkId="LIB_VSRN"  offset=...  size=4
  Entry 3:  chunkId="LIB_DATA"  offset=...  size=...   (member 1: AOF file)
  Entry 4:  chunkId="LIB_DATA"  offset=...  size=...   (member 2: AOF file)
  ...
  Entry M:  chunkId="LIB_DATA"  offset=...  size=...   (member N: AOF file)
  Entry M+1: chunkId="OFL_SYMT" offset=...  size=...
  Entry M+2: chunkId="OFL_TIME" offset=...  size=8

LIB_DIRY chunk:
  [Entry for member 1: ChunkIndex=3, name="module1.o", timestamp]
  [Entry for member 2: ChunkIndex=4, name="module2.o", timestamp]
  ...
  [Unused entries: ChunkIndex=0]

LIB_TIME chunk:
  [2-word timestamp: last library modification]

LIB_VSRN chunk:
  [1 word: value 1]

LIB_DATA chunks:
  [Complete AOF file for member 1]
  [Complete AOF file for member 2]
  ...

OFL_SYMT chunk:
  [Entry: ChunkIndex=3, symbol="function_a"]
  [Entry: ChunkIndex=3, symbol="global_var_x"]
  [Entry: ChunkIndex=4, symbol="function_b"]
  ...

OFL_TIME chunk:
  [2-word timestamp: last symbol table modification]
```

---

## 16. Endianness Detection

As with AOF files, the endianness of an ALF file is determined from the ChunkFileId magic number:

- `0xC3CBC6C5` when read as a word: file matches host byte order.
- `0xC5C6CBC3` when read as a word: all word values must be byte-reversed.

---

## 17. Relationship to AOF

ALF is a container format for AOF files. Each `LIB_DATA` chunk contains a complete, valid AOF file. The `OFL_SYMT` chunk provides a pre-built index of externally visible symbols from all member AOF files, enabling the linker to resolve references without scanning every member.

The AOF specification (including Chunk File Format, area headers, symbol tables, and relocation directives) applies in full to each library member.
