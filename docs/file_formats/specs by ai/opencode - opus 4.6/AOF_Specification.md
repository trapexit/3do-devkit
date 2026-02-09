# ARM Object Format (AOF) Specification

## 1. Overview

ARM Object Format (AOF) is the relocatable object file format used by ARM language processors (compilers, assemblers) and consumed by the ARM linker (`armlink`). It is similar in complexity and expressive power to Unix's `a.out` format, providing a generalised superset of `a.out`'s descriptive facilities.

AOF was designed to be simple to generate and to process, rather than to be maximally expressive or maximally compact.

An object file written in AOF consists of any number of named, attributed **areas**. Each area has attributes such as read-only, reentrant, code, data, position-independent, etc. Typically, a compiled AOF file contains a read-only code area and a read-write data area (zero-initialised data areas and based areas for address constants are also common).

AOF directly supports the ARM Procedure Call Standard (APCS).

---

## 2. Terminology

| Term      | Definition |
|-----------|------------|
| byte      | 8 bits, considered unsigned unless otherwise stated |
| half word | 16 bits (2 bytes), usually considered unsigned |
| word      | 32 bits (4 bytes), usually considered unsigned |
| string    | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL byte is part of the string but is not counted in the string's length |
| object file | A file in ARM Object Format |
| address   | For data in a file, this means offset from the start of the file |

---

## 3. Byte Sex / Endianness

There are two sorts of AOF: **little-endian** and **big-endian**.

- **Little-endian:** The least significant byte of a word or half-word has the lowest address. Used by DEC, Intel, and Acorn.
- **Big-endian:** The most significant byte of a word or half-word has the lowest address. Used by IBM, Motorola, and Apple.

The endianness of an AOF file always matches the endianness of the target ARM system. There is no guarantee it matches the host system processing the file.

The two sorts of AOF cannot be mixed. The ARM linker accepts inputs of either endianness and produces output of the same endianness, but rejects inputs of mixed endianness.

---

## 4. Alignment

- Strings and bytes may be aligned on any byte boundary.
- AOF header fields do not use half-words and align words on 4-byte boundaries.
- Within area contents, words are aligned on 4-byte boundaries and half-words on 2-byte boundaries (for all current ARM-based systems).

---

## 5. Overall Structure of an AOF File

An AOF file is layered on **Chunk File Format**, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file.

### 5.1 Chunk File Format

A chunk file consists of a header at the start of the file, followed by one or more chunks. The header contains the number, size, location, and identity of each chunk.

#### 5.1.1 Chunk File Header

The header has two parts:

**Part 1 - Fixed Header (3 words):**

| Field         | Size   | Description |
|---------------|--------|-------------|
| ChunkFileId   | 1 word | Magic number: `0xC3CBC6C5`. If this appears as `0xC5C6CBC3` when read as a word, each word value must be byte-reversed before use. This determines the endianness of the entire file. |
| maxChunks     | 1 word | Number of entries in the header. Fixed when the file is created. |
| numChunks     | 1 word | Number of chunks currently in use (0 to maxChunks). Redundant -- can be determined by scanning entries. |

**Part 2 - Chunk Entries (one per chunk):**

Each entry consists of 4 words:

| Field      | Size    | Description |
|------------|---------|-------------|
| chunkId    | 8 bytes | Identifying tag for the chunk contents. This is an **8-byte field** (not a 2-word field), so it has the same byte order regardless of endianness. Characters are stored in ascending address order. |
| fileOffset | 1 word  | Byte offset within the file to the start of this chunk. Must be divisible by 4. A value of 0 indicates the chunk entry is unused. |
| size       | 1 word  | Exact byte size of the chunk's contents (need not be a multiple of 4). |

#### 5.1.2 Chunk Identification

The 8-byte `chunkId` field is split into two 4-character parts:

1. **First 4 characters:** A universally unique domain name allocated by a central authority.
2. **Last 4 characters:** Component chunk identifiers within that domain.

Characters are stored in ascending address order, as if part of a NUL-terminated string, independent of endianness.

For AOF files, the domain name is `"OBJ_"`.

### 5.2 AOF Chunks

AOF defines five chunks:

| Chunk            | chunkId    | Required? | Description |
|------------------|------------|-----------|-------------|
| AOF Header       | `OBJ_HEAD` | Yes       | Fixed header + area declarations |
| Areas            | `OBJ_AREA` | Yes       | Area contents + relocation directives |
| Identification   | `OBJ_IDFN` | No        | Tool identification string |
| Symbol Table     | `OBJ_SYMT` | No        | Symbol definitions and references |
| String Table     | `OBJ_STRT` | No        | All print names referenced from headers/symbols |

Only the AOF Header and Areas chunks must be present. A typical object file contains all five chunks.

Chunks may appear in **any order** in the file. Language translators may add additional chunks (e.g. language-specific symbol tables or debugging data). Space for 8 chunks in the chunk file header is conventional.

**Important:** The AOF header chunk (`OBJ_HEAD`) must not be confused with the chunk file header.

---

## 6. The AOF Header Chunk (OBJ_HEAD)

The AOF header chunk consists of two contiguous parts.

### 6.1 Part 1 - Fixed Header (6 words)

| Field             | Size   | Description |
|-------------------|--------|-------------|
| Object File Type  | 1 word | `0xC5E2D080` marks the file as relocatable object format. The endianness must match the containing chunk file. |
| Version Id        | 1 word | AOF version number: `150` for version 1.50, `200` for version 2.00, `310` (`0x136`) for version 3.10 (current). |
| Number of Areas   | 1 word | Number of areas (equivalently, number of area header declarations following). Should be <= 255 in early versions. |
| Number of Symbols | 1 word | Number of symbols in the symbol table (`OBJ_SYMT`), if present. |
| Entry Area Index  | 1 word | 1-origin index into the area headers of the area containing the entry point. 0 = no entry point defined. |
| Entry Offset      | 1 word | The entry address = base address of the Entry Area + this offset. |

### 6.2 Part 2 - Area Headers

Immediately following the fixed header are `Number of Areas` area header entries. Each is 5 words:

| Field                  | Size   | Description |
|------------------------|--------|-------------|
| Area Name              | 1 word | Offset into the string table (`OBJ_STRT`) of the area's name. Each area within an object file must have a unique name. |
| Attributes + Alignment | 1 word | Bit flags for attributes (upper 24 bits) and alignment (lower 8 bits). See Section 7. |
| Area Size              | 1 word | Size of the area in bytes. Must be a multiple of 4. Unless the Not Initialised bit (bit 12) is set, there must be this many bytes in the `OBJ_AREA` chunk. |
| Number of Relocations  | 1 word | Number of relocation directives that apply to this area. |
| Base Address            | 1 word | Used only if the area has the absolute attribute (bit 8). Records the fixed base address. Otherwise 0. |

In the RISC OS PRM version, the 5th word is listed as `reserved` (must be zero), and an optional 6th word `area_base` is added only for absolute areas (bit 0 set in that version's type bits).

---

## 7. Area Attributes and Alignment

The Attributes + Alignment word encodes area attributes in the most significant 24 bits and alignment in the least significant 8 bits.

### 7.1 Alignment (bits 0-7)

The alignment value is a power of 2, between 2 and 32 inclusive. The area start address must be divisible by 2^alignment. Most commonly, alignment is 2 (requiring word alignment, i.e. 4-byte boundaries).

**Note:** In the RISC OS PRM, bits 0-7 encode type bits, and bits 13-14 encode alignment in log2 form. The SDT 2.50 specification uses the encoding described here.

### 7.2 Attribute Bits (SDT 2.50 / AOF 3.10)

| Bit   | Mask           | Attribute                          | Notes |
|-------|----------------|------------------------------------|-------|
| 8     | `0x00000100`   | Absolute                           | Area must be placed at its Base Address. |
| 9     | `0x00000200`   | Code                               | 1 = code, 0 = data. |
| 10    | `0x00000400`   | Common block definition            | Common areas with the same name are overlaid by the linker. |
| 11    | `0x00000800`   | Common block reference             | Implies bit 12. If both bits 10 and 11 are set, bit 11 is ignored. |
| 12    | `0x00001000`   | Zero-initialised (uninitialised)   | No initialising data in `OBJ_AREA`. Incompatible with bit 13. |
| 13    | `0x00002000`   | Read-only                          | Area will not be modified after relocation. Code and debug areas must set this. Incompatible with bit 12. |
| 14    | `0x00004000`   | Position independent               | All memory references must be link-time-fixed offsets from a base register. |
| 15    | `0x00008000`   | Debugging tables                   | Area contains symbolic debugging tables. Bit 9 is ignored. Usually has bit 13 set. |
| 16    | `0x00010000`   | 32-bit APCS                        | Code area only. Complies with 32-bit variant of APCS. |
| 17    | `0x00020000`   | Reentrant                          | Code area only. Complies with reentrant APCS variant. |
| 18    | `0x00040000`   | Extended FP instruction set        | Code area only. Uses LFM/SFM instructions. |
| 19    | `0x00080000`   | No software stack checking         | Code area only. Complies with non-stack-checked APCS variant. |
| 20    | `0x00100000`   | Thumb / Based area                 | SDT 2.50: Thumb code (code areas). Earlier: Based area attribute (data areas). |
| 21    | `0x00200000`   | ARM halfword instructions / SHL stub data | SDT 2.50: May contain halfword instructions. Earlier: Shared library stub data. |
| 22    | `0x00400000`   | ARM/Thumb interworking             | SDT 2.50 only. |
| 24-27 | `0x0F000000`   | Base register                      | For based areas: encodes the base register number (data areas only). |
| 23, 28-31 |            | Reserved                           | Must be 0. |

### 7.3 RISC OS PRM Attribute Encoding

The RISC OS PRM uses a different encoding for the lower bits:

| Bits  | Meaning |
|-------|---------|
| 0     | Absolute area |
| 1     | Relocatable area |
| 2     | Code area |
| 3     | Common area (uninitialised data) |
| 4     | Common block area |
| 5     | For use by the linker |
| 6     | Debugging tables area |
| 7     | Zero-initialised data |
| 8     | Area cannot contain code |
| 9     | Position independent code |
| 10    | Reentrant / common |
| 11    | Read-only data |
| 12    | Debugging table |
| 13-14 | Alignment (log2: 0=byte, 1=short, 2=word) |
| 15    | Area base field is valid |
| 24-31 | Stub bits (used by linker) |

### 7.4 Common Areas

Common areas with the same name are overlaid by the linker. The Area Size field of a common definition area defines the size of a common block. All other references must specify a size less than or equal to the definition size. If there is more than one definition, they must have exactly the same contents. If there is no definition, the size is the largest common reference.

Common areas conventionally hold data, but bit 10 with bit 9 can define a common code block (useful for code that must be generated in several compilation units but included once).

### 7.5 Linker Area Ordering

The linker orders areas in a generated image by:

1. **Attributes** (read-only code first, then read-only based data, read-only data, read-write code, based data, initialised data, zero-initialised data, debugging tables).
2. **Lexicographic order of area names** (case-significant, ASCII collation).
3. **Position of the containing object module** in the link list.

---

## 8. The Areas Chunk (OBJ_AREA)

The areas chunk contains the actual area contents (code, data, debugging data) together with associated relocation data.

### 8.1 Layout

```
Area 1 contents
Area 1 relocation directives
Area 2 contents
Area 2 relocation directives
...
Area N contents
Area N relocation directives
```

Each area is a sequence of bytes. The endianness of words and half-words within it must agree with the containing AOF file. An area is followed by its table of relocation directives (if any). Both area contents and relocation tables are aligned to 4-byte boundaries.

An area is either completely initialised by values from the file or is entirely zero-initialised (as specified by bit 12 of its area attributes). Zero-initialised areas have no data in the `OBJ_AREA` chunk.

---

## 9. Relocation Directives

A relocation directive describes a value that is computed at link time or load time but cannot be fixed when the object module is created. In the absence of relocation directives, values in the area appear exactly as they will in the final image.

A field may be subject to more than one relocation.

### 9.1 Relocation Directive Format (2 words)

```
Word 0: Offset
Word 1: Flags + SID

Bit layout of Word 1:
+----+----+----+----+----+------+------------------------+
| 31 | 30 | 29 | 28 | 27 | 26  | 25 | 24 | 23 ... 0     |
|  1 | II      | B  | A  | R   | FT       | SID (24 bits)|
+----+----+----+----+----+------+------------------------+
```

### 9.2 Field Descriptions

**Offset (Word 0):** Byte offset in the preceding area of the subject field to be relocated.

**SID (bits 0-23):** Subject identification. Interpretation depends on the A bit:

| A bit | Interpretation |
|-------|---------------|
| A = 1 (bit 27 set) | SID is the 0-origin index into the symbol table. The subject field is relocated by the value of that symbol. |
| A = 0 (bit 27 clear) | SID is the 0-origin index into the array of areas. The subject field is relocated by the base of that area. |

**FT - Field Type (bits 24-25):**

| Value | Field Type |
|-------|------------|
| 00    | Byte (1 byte) |
| 01    | Half-word (2 bytes) |
| 10    | Word (4 bytes) |
| 11    | Instruction or instruction sequence |

For field type `11`, bit 0 of the Offset determines instruction set: if set = Thumb, otherwise ARM (SDT 2.50).

Bytes, half-words, and instructions may only be relocated by values of suitably small size. Overflow is faulted by the linker.

**II - Instruction count constraint (bits 29-30):** Only meaningful for instruction sequence relocations (FT = 11):

| Value | Constraint |
|-------|------------|
| 00    | No constraint; linker may modify as many contiguous instructions as needed |
| 01    | At most 1 instruction |
| 10    | At most 2 instructions |
| 11    | At most 3 instructions |

**Bit 31:** Must be 1 for based area relocations. Otherwise varies.

### 9.3 Relocation Types

The relocation type is determined by the R bit (bit 26) and B bit (bit 28):

#### 9.3.1 Plain Additive Relocation (R=0, B=0)

```c
subject_field = subject_field + relocation_value
```

The relocation value is simply added to the subject field.

#### 9.3.2 PC-Relative Relocation (R=1, B=0)

```c
subject_field = subject_field + (relocation_value - base_of_area_containing(subject_field))
```

**Special case:** If A=0 and the relocation value is the base of the area containing the subject field, the value is not added:

```c
subject_field = subject_field - base_of_area_containing(subject_field)
```

This handles PC-relative branches to fixed target addresses.

If R=1 and B=1, the inter-link-unit value of a branch destination is used rather than the intra-link-unit value (supports tail-call optimisation on reentrant code).

#### 9.3.3 Based Area Relocation (R=0, B=1)

```c
subject_field = subject_field + (relocation_value - base_of_area_group_containing(relocation_value))
```

Used for sb-relative addressing in reentrant code. The linker consolidates all areas based on the same base register into a single contiguous region. Bits 29-30 must be 0; bit 31 must be 1.

### 9.4 Instruction Sequence Relocation

The linker recognises the following instruction sequences as defining a relocatable value:

- A `B` or `BL` instruction (always relocated alone)
- An `LDR` or `STR` instruction
- 1 to 3 `ADD` or `SUB` instructions with a common destination and intermediate source register, optionally followed by an `LDR` or `STR` using that register as base

Example relocatable sequence:
```arm
ADD    Rb, Rx, #V1
ADD    Rb, Rb, #V2
LDR    Ry, [Rb, #V3]
```
Relocatable value: `V = V1 + V2 + V3`

After relocation, the new value is distributed: the LDR/STR offset is preserved if possible; the remainder is split among the ADD/SUB instructions using 8-bit rotated immediates.

---

## 10. Symbol Table Chunk (OBJ_SYMT)

The Number of Symbols field in the AOF header defines the number of entries. Each entry is 4 words (16 bytes):

| Field      | Size   | Description |
|------------|--------|-------------|
| Name       | 1 word | Offset in the string table (`OBJ_STRT`) of the symbol name. |
| Attributes | 1 word | See Section 10.1. |
| Value      | 1 word | See Section 10.2. |
| Area Name  | 1 word | Offset in the string table of the defining area name. Meaningful only for non-absolute defining occurrences. |

### 10.1 Symbol Attributes

| Bit  | Mask           | Attribute                         | Notes |
|------|----------------|-----------------------------------|-------|
| 0    | `0x00000001`   | Defined in this file              | |
| 1    | `0x00000002`   | Global scope                      | Can be matched to a similarly named symbol from another object file. |
| 2    | `0x00000004`   | Absolute                          | Symbol has an absolute value (e.g. a constant). |
| 3    | `0x00000008`   | Case-insensitive reference         | Linker ignores case when matching. Meaningful only for external references. |
| 4    | `0x00000010`   | Weak                              | Reference may remain unsatisfied. Linker ignores weak references when loading library members. |
| 5    | `0x00000020`   | Strong                            | SDT 2.50: Strong definition overrides non-strong definitions. |
| 6    | `0x00000040`   | Common                            | Reference to a common area. Value field = byte length. |
| 7    |                | Reserved                          | Must be 0. |
| 8    | `0x00000100`   | Code datum                        | Symbol identifies a datum (not code) within a code area. |
| 9    | `0x00000200`   | FP args in FP regs                | Symbol is a function entry point. Reference with this attribute cannot match a definition lacking it. |
| 10   |                | Reserved                          | Must be 0. |
| 11   | `0x00000800`   | Simple leaf function              | SDT 2.50: Leaf function whose inter-LU and intra-LU entry points are the same. |
| 12   | `0x00001000`   | Thumb symbol                      | SDT 2.50 only. |
| 13-31 |               | Reserved                          | Must be 0. |

### 10.2 Bits 1,0 Combinations

| Bits 1,0 | Meaning |
|----------|---------|
| 01       | Defined in this file, local scope (linker only matches within same object file). |
| 10       | External reference to a symbol defined in another object file. If unresolved, linker tries to match to common block names. |
| 11       | Defined in this file with global scope. Linker matches to references from other files. |
| 00       | Reserved. |

### 10.3 Value Field Interpretation

| Condition | Value contains |
|-----------|----------------|
| Absolute symbol (bits 0,2 set) | The absolute value of the symbol. |
| Common symbol (bit 6 set) | The byte length of the referenced common area. |
| Otherwise (non-absolute defining occurrence) | Offset from the base address of the area named by Area Name. |

### 10.4 RISC OS PRM Symbol Attribute Encoding

The RISC OS PRM uses a different bit layout:

| Bits  | Meaning |
|-------|---------|
| 0     | Defined in this file |
| 1     | Exported (global) |
| 2     | Weak override |
| 3     | Common |
| 4     | Constant value (not address) |
| 5     | Case-insensitive |
| 6     | Weak definition |
| 7     | Strong definition |
| 16-23 | Datum and type field |
| 24-27 | Symbol type (architecture-specific) |

Symbol types 9-13 encode register information for floating-point and coprocessor registers.

---

## 11. The String Table Chunk (OBJ_STRT)

The string table contains all print names referenced from the header and symbol table chunks. This separation factors out the variable-length nature of names from primary data structures.

### 11.1 Format

- The first 4 bytes contain the total length of the string table (including the length word itself). No valid offset is less than 4; no table has length less than 4.
- The endianness of the length word matches the containing AOF and chunk files.
- Each print name is stored as a sequence of non-control characters (codes 32-126 and 160-255) terminated by a NUL (0) byte.
- Names are identified by their byte offset from the start of the table.

### 11.2 RISC OS PRM Variant

In the RISC OS PRM, the string table is described as a simple sequence of NUL-terminated strings packed end to end starting at byte offset 0, with no leading length word and no alignment restrictions.

---

## 12. The Identification Chunk (OBJ_IDFN)

This chunk contains a string of printable characters (codes 10-13 and 32-126) terminated by a NUL (0) byte, giving information about the name and version of the tool that generated the object file.

Use of codes in the range 128-255 is discouraged as interpretation is host-dependent.

---

## 13. Endianness Detection

The endianness of an AOF file can be determined from two independent mechanisms:

1. **ChunkFileId:** If `0xC3CBC6C5` when read as a word, the file matches the host byte order. If `0xC5C6CBC3`, all word values must be byte-reversed.
2. **Object File Type:** `0xC5E2D080` marks relocatable object format. Its endianness must match the containing chunk file.

---

## 14. AOF as Linker Output

When AOF is used as an output format (partial linking), the linker:

- Merges similarly named and attributed areas.
- Performs PC-relative relocations between merged areas.
- Re-writes symbol-relative relocation directives as area-based directives.
- Minimises the symbol table.

Unresolved references remain unresolved, and the output may be used as input to a subsequent link step.

---

## 15. Version History

| Version | Value | Notes |
|---------|-------|-------|
| 1.50    | 150   | Original specification |
| 2.00    | 200   | Introduced zero-init bit 7, common definition areas |
| 3.10    | 310   | SDT 2.50; Thumb support; strong symbols; halfword instructions; interworking |
