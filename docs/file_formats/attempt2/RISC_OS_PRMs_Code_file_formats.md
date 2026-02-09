# Appendix D: Code file formats

---

This appendix defines three file formats used to store processed code and the format of debugging data used by debuggers:

- AOF - Arm Object Format
- ALF - Acorn Library Format
- AIF - RISC OS Application Image Format
- ASD - ARM Symbolic Debugging Format.

Language processors such as CC and ObjAsm generate processed code output as AOF files. An ALF file is a collection of AOF files constructed from a set of AOF files by the LibFile tool. The Link tool accepts a set of AOF and ALF files as input, and by default produces an executable program file as output in AIF.

## Terminology

Throughout this appendix the terms *byte*, *half word*, *word*, and *string* are used to mean the following:

*Byte*: 8 bits, considered unsigned unless otherwise stated, usually used to store flag bits or characters.

*Half word*: 16 bits, or 2 bytes, usually unsigned. The least significant byte has the lowest address (DEC/Intel *byte sex*, sometimes called *little endian*). The address of a half word (ie of its least significant byte) must be divisible by 2.

*Word*: 32 bits, or 4 bytes, usually used to store a non-negative value. The least significant byte has the lowest address (DEC/Intel byte sex, sometimes called little endian). The address of a word (ie of its least significant byte) must be divisible by 4.

*String*: A sequence of bytes terminated by a NUL (0X00) byte. The NUL is part of the string but is not counted in the string's length. Strings may be aligned on any byte boundary.

For emphasis: a word consists of 32 bits, 4-byte aligned; within a word, the least significant byte has the lowest address. This is DEC/Intel, or little endian, byte sex, **not** IBM/Motorola byte sex.

## Undefined Fields

Fields not explicitly defined by this appendix are implicitly reserved to Acorn. It is required that all such fields be zeroed. Acorn may ascribe meaning to such fields at any time, but will usually do so in a manner which gives no new meaning to zeroes.

## Overall structure of AOF and ALF files

An object or library file contains a number of separate but related pieces of data. In order to simplify access to these data, and to provide for a degree of extensibility, the object and library file formats are themselves layered on another format called **Chunk File Format**, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file. The object file format defines five chunks:

- header
- areas
- identification
- symbol table
- string table.

The library file format defines four chunks:

- directory
- time-stamp
- version
- data.

There may be many data chunks in a library.

The minimum size of a piece of data in both formats is four bytes or one word. Each word is stored in a file in little-endian format; that is the least significant byte of the word is stored first.

## Chunk file format

A chunk is accessed via a header at the start of the file. The header contains the number, size, location and identity of each chunk in the file. The size of the header may vary between different chunk files but is fixed for each file. Not all entries in a header need be used, thus limited expansion of the number of chunks is permitted without a wholesale copy. A chunk file can be copied without knowledge of the contents of the individual chunks.

Graphically, the layout of a chunk file is as follows:

![Chunk file format diagram](objectformat-2.png)

`ChunkFileId` marks the file as a chunk file. Its value is C3CBC6C5 hex. The `maxChunks` field defines the number of the entries in the header, fixed when the file is created. The `numChunks` field defines how many chunks are currently used in the file, which can vary from 0 to `maxChunks`. The value of `numChunks` is redundant as it can be found by scanning the entries.

Each entry in the header comprises four words in the following order:

| Field | Description |
|-------|-------------|
| `chunkId` | a two word field identifying what data the chunk file contains |
| `Offset` | a one word field defining the byte offset within the file of the chunk (which must be divisible by four); an entry of zero indicates that the corresponding chunk is unused |
| `size` | a one word field defining the exact byte size of the chunk (which need not be a multiple of four). |

The `chunkId` field provides a conventional way of identifying what type of data a chunk contains. It is split into two parts. The first four characters (in the first word) contain a universally unique name allocated by a central authority (Acorn). The remaining four characters (in the second word) can be used to identify component chunks within this universal domain. In each part, the first character of the name is stored first in the file, and so on.

For AOF files, the first part of each chunk's name is `OBJ_`; the second components are defined later. For ALF files, the first part is `LIB_`.

# AOF

ARM object format files are output by language processors such as CC and ObjAsm.

## Object file format

Each piece of an object file is stored in a separate, identifiable, chunk. AOF defines five chunks as follows:

| Chunk | Chunk Name |
|-------|------------|
| Header | OBJ_HEAD |
| Areas | OBJ_AREA |
| Identification | OBJ_IDFN |
| Symbol Table | OBJ_SYMT |
| String Table | OBJ_STRT |

Only the `header` and `areas` chunks must be present, but a typical object file will contain all five of the above chunks.

A feature of chunk file format is that chunks may appear in any order in the file. However, language processors which must also generate other object formats - such as UNIX's `a.out` format - should use this flexibility cautiously.

A language translator or other system utility may add additional chunks to an object file, for example a language-specific symbol table or language-specific debugging data, so it is conventional to allow space in the chunk header for additional chunks; space for eight chunks is conventional when the AOF file is produced by a language processor which generates all five chunks described here.

The `header` chunk should not be confused with the chunk file's header.

### Format of the AOF header chunk

The AOF header is logically in two parts, though these appear contiguously in the header chunk. The first part is of fixed size and describes the contents and nature of the object file. The second part is variable in length (specified in the fixed part) and is a sequence of `area` declarations defining the code and data areas within the OBJ_AREA chunk.

#### The fixed part

| Field | Description |
|-------|-------------|
| `file_type` | word |
| `version_id` | word |
| `num_areas` | number of code/data areas (should be <= 255) |
| `num_symbols` | number of symbols |
| `entry_area` | area containing the entry point |
| `entry_offset` | byte offset of entry point within entry_area |

The two words `file_type` and `version_id` identify this as an AOF file and distinguish its version. For the object file format defined here, `file_type` is `0xC5E2D080` and `version_id` is `310`, the latter chosen to allow a vague association to be made with the version of Assembler or C which first produced files containing these fields.

#### Area declarations

The second part of the header contains `num_areas` entries each of which describes a code or data area. The format of an entry is:

| Field | Description |
|-------|-------------|
| `area_name` | an offset into the string table of the name of the area |
| `area_attributes` | word |
| `area_size` | size of area in bytes |
| `num_relocs` | number of relocations |
| `reserved` | word containing 0 |

The fields have the following interpretations:

`area_name`: This is a byte offset into the string table. An offset of zero means there is no symbol-table string for the area name (in which case it may, by convention, be taken to be something like *areaxx* where xx is the 0-origin number of the area). See on page 1036 and on page 1042.

`area_attributes`: This is a bit-map specifying various area attributes, some of which may or may not be set in an object file produced by a language processor. The meanings of the bits of the word are:

| Bit | Meaning |
|-----|---------|
| 0-7 | Type bits. At most one can be set |
| 8 | = 1 ⇒ area cannot contain code |
| 9 | = 1 ⇒ area is position independent code |
| 10 | = 1 ⇒ area is reentrant (common) |
| 11 | = 1 ⇒ area contains only readable data (data which is never written to in the course of normal execution of the program - the converse is **not** true) |
| 12 | = 1 ⇒ area is a debugging table |
| 13-14 | Alignment bits: n ⇒ area is aligned to 2^n (0 is byte aligned, 1 is short aligned, 2 is word aligned) |
| 15 | = 1 ⇒ the area base field is valid (area base is set by the linker) |
| 16-23 | Reserved, undefined bits. Must be zero |
| 24-31 | Stub bits. Bits set depending on the area type |

**Type bits** (bits 0-7): In an object file, at most one of these bits may be set, though clearly it makes sense to output areas to object files with no type bit set. The meanings of the type bits are:

| Bit | Meaning if set |
|-----|---------------|
| 0 | absolute area |
| 1 | relocatable area |
| 2 | code area |
| 3 | common area, ie uninitialised data |
| 4 | common area which must be a block |
| 5 | for use by the linker |
| 6 | debugging tables area |
| 7 | zero-initialised data |

An absolute area has the property that the area's base remains constant through relocation and linking; a relocatable area may be relocated through any convenient amount by the linker; a code area contains something likely to be executed as ARM instructions; a common area is uninitialised and may share its address space with other common areas having the same area name. Common data areas permit the easy implementation of Fortran named common; common definition areas used as an aid to separately compiling modules within the scope of a compilation system (the modules can be linked 'for free' within the compilation system or with the aid of the system linker). Common definition areas should not have any real data associated with them (like areas marked as uninitialised in bit 3) but can have initialisation relocation directives and symbols associated with them. The linker creates a real area from the common definition area and the common block area only if it detects references to symbols defined relative to the common definition area or to the common block area. See under on page 1028. Zero initialised areas are like common areas except that storage is guaranteed to be initialised to zero.

**AT (Area Type) bits** (bit 8-15): The meanings of these bits are:

| Bit | Meaning if set |
|-----|---------------|
| 8 | area must not contain code |
| 9 | area contains position independent code |
| 10 | area is reentrant or common block |
| 11 | area is read-only |
| 12 | area is a debugging table |
| 13-14 | area alignment in log2 form |
| 15 | area base is valid |

These bits were designed to be an upward compatible superset of those defined in the RISC OS 2 Programmers Reference Manual which only defined bits 8-10. They are used in the following combinatoric interpretations:

A code area (bit 2 set in the type bits or an area in which code is expected) is necessarily non-common data.

In a code area, only bits 9, 11, 13, 14 and 15 can be set. Bit 9 specifies that no PC-relative reference within the area (other than PC-relative branches) is made outside of the area and bit 11 says that the code is execute-only.

A common area (bits 3 or 4 set in the type bits) must have bit 10 set.

A common area must not have bit 11 set: a common area is never read-only.

For areas that are common blocks (bit 4 in the type bits is set) the stub bits (24-31) are used by the linker.

An absolute area must have bit 15 set and the area base field present (see Area declarations).

A debugging tables area (bit 6 set in the type bits) must have bit 12 set.

A relocatable data area (ie not code, not common) may have bit 11 set to indicate that it is read-only.

A zero-initialised data area (bit 7 set in the type bits) is like a common area, but storage is guaranteed to be initialised to 0. In RISC OS 2, data areas which are uninitialised and hence require zero-initialisation (bit 3 set in the type bits) were distinguished from true common data areas (bit 10 set) because they never share their address space with other areas with the same name. In AOF version 200, AIF version 3.00 and Link version 3.00, bit 7 is set to explicitly indicate zero-initialised data.

**STUB (Stub Type) bits** (bits 24-31): These bits are used and updated by the linker; their initial value is undefined, except in common block areas for which bits 24-27 define the maximum permitted alignment in log2 form. These bits should not be set in a file to be put in a library.

`area_size`: In an object file, the size of an area is its size in bytes. For a common area (bits 3 or 4 set in the type bits), this is the minimum value acceptable to the defining module. For a zero-initialised data area (bit 7 set), the size must be a whole number of words. For all other kinds of area, there is this much data in the areas chunk.

`num_relocs`: This is the number of relocation directives associated with this area, present in the areas chunk following the code or data for this area.

`reserved`: This field must be zero in a file to be put in a library.

**Absolute areas** (bit 0 set in the type bits): These differ from ordinary object file areas in that they define their addresses explicitly. The area declaration has an additional field which follows immediately after the `reserved` field:

| Field | Description |
|-------|-------------|
| `area_base` | word |

`area_base`: The address at which the area is to be located.

Absolute areas are 'linked' together by the linker, but do not have their base moved. Absolute areas cannot contain relocatable references to areas which are not absolute.

### Format of the areas chunk

The format of the areas chunk is:

```
area 1 data
area 1 relocation directives
area 2 data
area 2 relocation directives
...
```

There is data for each area with size > 0 (or marked as being uninitialised).

The relocation directives for an area immediately follow the area's data. The number of relocation directives is given in the area's declaration in the header chunk. Each relocation directive comprises two words:

| Field | Description |
|-------|-------------|
| `offset` | word |
| `flags and SID` | word |

The relocation directive specifies that at some offset relative to the base of the area, there is a word (ie 4 bytes) containing a field which should be relocated (adjusted) as a consequence of the base addresses of other areas varying during link time. There is a large degree of freedom in what can be encoded, though most common cases are efficiently supported. If the base of area N is denoted by AN, and the contents of the word at the offset to be relocated is denoted by W, then the common cases which are supported efficiently are shown below:

| Relocation Type | Description |
|----------------|-------------|
| `W + AN` | Word contains PC-relative offset, to be relocated by the base of area N |
| `W + AN + a` | Word contains offset plus a constant, to be relocated by base of area N |
| `((W + AN) mod 2^24) or (W and 0xFF000000)` | Field containing a 24-bit PC-relative branch target. The top 8 bits of the instruction encoding are preserved |

The meanings of some of the bits in the flags word are:

| Bit | Meaning |
|-----|---------|
| 0 | Relevant to A-type relocations: if set, then the A-field of the relocation directive is a PC-relative value |
| 8-11 | A field specifier. Bits specify how to extract the A-field from the word to be relocated. If the field specification is 0, there is no A-field and any addend must be in the following word |
| 12-15 | R-type field specifier. Always set to 0 in Version 1.50 (meaning word-wide) |
| 16-23 | SID number. Either the symbol table index number of the symbol to be relocated against, or if the most significant bit (bit 23) is set, then the remaining 7 bits specify the area number to be relocated by |

If a symbol is marked as being in a common area, then a relocation directive based on that symbol is based on the area.

### Format of the identification chunk

The identification chunk identifies the program which created the object file. It contains only a variable length string which names the file creation agent:

```
string
```

### Format of the symbol table chunk

Each symbol table entry comprises four words in the following format:

| Field | Description |
|-------|-------------|
| `name` | offset of symbol name in the string table (word) |
| `flags` | word |
| `value` | word |
| `area_name` | offset of name of defining area in the string table (word) |

The interpretation of the four words is:

`name`: A byte offset into the string table. An offset of zero means that there is no name string.

`flags`: This field contains several bit fields. The bit field definitions are as follows (all other bits are reserved):

| Bits | Meaning |
|------|---------|
| 0 | The symbol is defined in this object file (LL0) |
| 1 | The symbol is exported to other object files (GBLL1) |
| 2 | The symbol's definition may override a previous weak definition in another object file (WKLL2). A symbol must not have both bits 1 and 2 set |
| 3 | The symbol is common (CMLL3). This is an alternative method of defining external common. The symbol should not be defined in terms of an area base (field four). Symbol-relative relocations are converted by the linker |
| 4 | Set if the value field contains a constant (or common size) rather than an address. This bit must be set if bit 3 (common) is set (CSLL4) |
| 5 | Set if the symbol is a case-insensitive match against other symbols (e.g. all symbols in a Basic program) (CISL5). Only code producing languages which are case-insensitive can use this |
| 6 | Set if the symbol is weak (defined here but may be undefined by some other definition) (WKLL6). A symbol can be weak and global. A symbol cannot be weak and case-insensitive |
| 7 | Set if the symbol is a strong definition (STLL7). This is the opposite sense to bit 2 |
| 8-15 | Not used |
| 16-23 | A datum and type field. Different interpretations are overlaid onto this field |
| 24-27 | A symbol type. Interpretation is architecture specific. Values 0-8 are reserved |
| 28-31 | Reserved |

Bit 0 indicates whether a symbol is defined in this object file. A symbol can be referenced from a module but actually defined elsewhere. More than one separately compiled module can reference the same external symbol, but only one can define it. If a symbol is not defined within an object file, its 'value' and 'area_name' fields should both be 0.

Bit 1 indicates that a symbol defined in a module may be referenced from other modules. An error should normally be signalled if an external reference is made to a symbol which is not marked as global. However, for pragmatic reasons, it is conventional to accept such references but issue a warning (if warnings are enabled).

Bit 2 indicates that this symbol definition can be overridden by another definition. This enables libraries to export weak definitions of symbols which can be used if other definitions are not found, but which are discarded if those definitions are found.

Bit 3 indicates that the symbol is common, rather than referring to some location in an area. In this case the value field should contain the length of the common block required. All definitions of the common symbol must have the same value (or common size). A common symbol can also be defined in an area by referring to a common definition area or a common block area (bits 3 or 4 of the type bits in the area declaration). A common symbol must have bit 3 set.

Bit 4 indicates that the symbol definition is a constant expression, not an address. Relocatable references relative to a constant expression are not meaningful. A constant symbol may be defined in an absolute area. A common symbol must have bit 4 set (and its value field will encode the size of the common).

Bit 5 indicates a case-insensitive symbol (which performs case-insensitive matching). Only code producing languages which are case-insensitive themselves can create this.

Bits 6 and 7 should not both be set. Bit 6 indicates that the linker should use some other definition of the symbol if one is present. Bit 7 indicates that the linker should use this definition of the symbol in preference to any weak definition of it. A symbol can be weak and exported (bits 1 and 6 set). A symbol cannot be weak and case-insensitive (bits 5 and 6 should not both be set). These attributes are mutually exclusive in sensible use.

The symbol datum and type fields can be interpreted in several ways depending on the symbol type encoded in bits 24-27:

Symbol type 0: Not used.

Symbol types 1-8: Reserved.

Symbol type 9: The value of bits 16-17 encodes a floating point number size in words (0 ⇒ 0 words (reserved for code), 1 ⇒ 1 word, 2 ⇒ 2 words and 3 ⇒ 3 words). Bits 18-23 are the register number (which register contains the value of the symbol). A register number of 255 is special and indicates that there is no appropriate register.

Symbol type 10: Value of bits 16-23 is a register number (which register contains the value of the symbol) and need not be preserved between procedure calls (scratch register). A register number of 255 is special and means that there is no appropriate register.

Symbol type 11: As for type 10 but the register value must be preserved between procedure calls (non-scratch register).

Symbol type 12: Coprocessor register - as for type 10. The coprocessor number is encoded in the register field using R = 256*CN + REGNO.

Symbol type 13: Coprocessor register - as for type 12 but the register value must be preserved between procedure calls.

`value`: This field usually gives the offset of the symbol's value relative to the base of its defining area. If the symbol is absolute, its absolute value is given (see common and data field of the flags word). If the symbol is common, value gives the size of the symbol's common area.

`area_name`: This field is the offset into the string table of the name of the area (or the name of the base of the area) which defines the symbol's value. If the symbol is absolute this field is zero.

The linker combines the base of the relevant area with the symbol's value to produce the symbol's absolute value at link time.

### Format of the string table

The OBJ_STRT chunk is simply a sequence of NUL-terminated strings packed end to end and starting at byte offset 0. No alignment restrictions apply.

## Linker symbol table format

The linker reads and writes OBJ_SYMT symbol table chunks and creates internal data structures from them. For debugging and browsing purposes, the linker can write its internal symbol table to a file. The format of this file is similar to that of a string table. It is simply a table of zero-terminated strings with no padding. Each line consists of up to four comma-separated fields: `name,value,class[,definingmodulename]`.

name: The name of the symbol, at least one printable character long.

value: The value of the symbol; either a constant, an absolute address, or an offset from a base. The string encodes a hex number prefixed by '&' (in the style of BBC Basic).

class: One of Abs, RelocDef, RelocRef, ExtDef or ExtRef; Abs means the value is absolute, ExtRef means the symbol is referenced in some module but not defined anywhere, ExtDef means it is defined in one or more modules and also referenced in one or more modules (not counting the definitions), RelocDef means the symbol is defined in some module but not referenced in any, and RelocRef means the symbol is referenced in one or more modules.

definingmodulename: Gives the name of a module in which the symbol is defined, or from which it is referenced if the class is ExtRef. The string is optional (and is a name specified by a command line -mddname option to the linker).

## ASD

### ARM symbolic debugging tables

The symbolic information embedded in an AOF file consists of a sequence of data items describing the names and types of procedures, variables and so on. Each item consists of a four-byte header, defining the class and length, followed by data appropriate to the class. The length in the header is the total length of the item in bytes including the header; the shortest data item is four bytes long (just a header).

The header word format is:

| Field | Size | Description |
|-------|------|-------------|
| Class | 8 bits | Item class |
| Data | 8 bits | Class-specific data |
| Length | 16 bits | Total length in bytes (minimum 4) |

The symbolic information for a program is stored in one or more sections. Each section corresponds to one area of code in the executable program. Within a section the items are grouped to minimise redundant context information. For example, the function containing a variable is recorded in the item which introduces the function, rather than being stated for each variable defined in the function.

### Section

This item starts a new section and usually appears only once in a chunk. The format is as follows:

| Field | Description |
|-------|-------------|
| EndPC | word: size of section in bytes (PC extent) |
| language | a four-character language name field (eg 'C   ' or 'BCPL') |
| section name | a string (or NUL if the section has no name) |

A section is either resident or relinked. It is resident if its name is a nonempty string; it is relinked if the string is a NUL. Resident sections are typically loaded from the disc file into memory only once, and may then be shared by several tasks; relinked sections are loaded every time a task starts up, and appear at different addresses in each task.

The EndPC is significant mainly for resident sections where there may be no other means of finding the section length. It should be set, even though it is likely to be ignored for relinked sections.

### Function

This item introduces a new function, ending the previous one (if any). The format is as follows:

| Field | Description |
|-------|-------------|
| startpc | word: address of the function entry-point |
| endpc | word: address of the first byte beyond the function code |
| name | string |

### Variable

This item describes a local or static variable. The format is as follows:

| Field | Description |
|-------|-------------|
| address | (interpretation as described below) |
| type | a type word (interpretation as described below) |
| name | string |

The address field can be interpreted in several ways, depending on the 'data' byte of the header. If zero it is the address of a static variable. If one, it is the stack offset of a variable on the stack. If two, it is the register number holding the value of the variable.

### Type

This item defines a type whose name can be associated with it. This is needed in languages (like Pascal or C) where the user can explicitly name a type (to avoid repetitive specification of composite types). The format is as follows:

| Field | Description |
|-------|-------------|
| type | a type word (interpretation as described below) |
| name | string |

### Struct

This item specifies a structure. The format is the 'length' field from the header giving the length of the item, followed by a number of fields, each of which has the following structure:

| Field | Description |
|-------|-------------|
| offset | byte offset of this field relative to the start of the structure |
| type | a type word (interpretation as described earlier) |
| name | string |

Union types are described by struct items in which all fields have 0 offsets.

C bit fields are not treated in full detail: a bit field is simply represented by an integer starting on the appropriate word boundary (so that the word contains the whole field).

### Array

This item is used to describe a one-dimensional array. Multi-dimensional arrays are described as arrays of arrays. Which dimension comes first is dependent on the source language (different for C and Fortran). The format is as follows:

| Field | Description |
|-------|-------------|
| size | total byte size of each element |
| arrayflags | (see below) |
| basetype | a type word |
| lowerbound | constant value or stack offset of variable |
| upperbound | constant value or stack offset of variable |

If the size field is zero, debugger operations affecting the whole array, rather than individual elements of it, are forbidden.

The following bit numbers in the arrayflags field are defined:

| Bit | Meaning |
|-----|---------|
| 0 | lower bound is undefined |
| 1 | lower bound is a constant |
| 2 | upper bound is undefined |
| 3 | upper bound is a constant |

If a bound is defined and not constant then it is an integer variable on the stack and the boundvalue field contains the stack offset of the variable (from the frame-pointer).

### Subrange

This item is used to describe subrange typed in Pascal. It also serves to describe enumerated types in C and scalars in Pascal (in which case the base type is understood to be an unsigned integer of appropriate size). Its format is as follows:

| Field | Description |
|-------|-------------|
| size | half-word: 1, 2, or 4 to indicate byte size of object |
| typecode | half-word: simple type code |
| lwb | lower bound of subrange |
| upb | upper bound of subrange |

### Set

This item is used to describe a Pascal set type. Currently, the description is only partial. The format is:

| Field | Description |
|-------|-------------|
| size | byte size of the object |

### Fileinfo

This item appears once per section after all other debugging data items. The half of the header word which would usually give the item length is not required and should be set to 0.

Each source file is described by a sequence of 'fragments', each of which describes a contiguous region of the file within which the addresses of compiled code increase monotonically with source-file position. The order in which fragments appear in the sequence is not necessarily related to the source file positions to which they refer.

Note that for compilations that make no use of the #include facility, the list of fragments will have only one entry and all line-number information will be contiguous.

The item is a list of entries each with the following format:

| Field | Description |
|-------|-------------|
| length | length of this entry in bytes (0 marks the final entry) |
| date | date and time when the file was last modified |
| filename | string (or null if the name is not known) |
| n | number of fragments following |
| fragments... | n fragments with the following structure... |
| | fragmentsize: length of this entry in bytes |
| | firstline: linenumber |
| | lastline: linenumber |
| | codeaddr: pointer to the start of the fragment's executab |
| | codesize: byte size of the code in the fragment |
| | lineinfo...: a variable number of line number data |

There is one lineinfo half-word for each statement of the source file fragment which gives rise to executable code. Exactly what constitutes an executable statement may be defined by the language implementation; the definition may for instance include some declarations. The half-word can be regarded as 2 bytes: the first contains the number of bytes of code generated from the statement and cannot be zero; the second contains the number of source lines occupied by the statement (ie the difference between the line number of the start of the statement and the line number of the next statement). This may be zero if there are multiple statements on the same source line.

If the whole half-word is zero, this indicates that one of the quantities is too large to fit into a byte and that the following 2 half-words contain (in order) the number of lines followed by the number of bytes of code generated from the statement.

---

*This edition Copyright © 3QD Developments Ltd 2015  
Last Edit: Tue,03 Nov 2015*
