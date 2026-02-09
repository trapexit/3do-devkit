# ARM File Format Specifications - Complete Index

**Documentation Version:** 1.0
**Last Updated:** 2026-02-07
**Total Documentation:** 92KB across 3 comprehensive specifications

---

## Overview

This directory contains complete, detailed specifications for three interrelated file formats used in ARM software development:

1. **AIF** - ARM Image Format (executable format)
2. **AOF** - ARM Object Format (compiler/assembler output)
3. **ALF** - ARM Object Library Format (archive format)

These formats work together in a complete compilation pipeline:

```
Source Code
    ↓
Compiler/Assembler → AOF Object Files
    ↓
Librarian → ALF Libraries (optional)
    ↓
Linker → AIF Executable Image
    ↓
Execution
```

---

## Format Specifications

### 1. AIF_FORMAT_COMPLETE.md (ARM Image Format)

**File Size:** 37 KB
**Purpose:** Executable image format for ARM processors
**Used By:** RISC OS, 3DO, embedded systems

**Key Topics Covered:**
- ✓ Overview and history (versions 0.03 through 3.00+)
- ✓ Three AIF variants (Executable, Non-Executable, Extended)
- ✓ Complete 128-byte header specification with bit-level detail
- ✓ All image sections (RO, RW, ZI, Debug)
- ✓ Self-relocation mechanisms (one-time, self-move)
- ✓ Decompression support with examples
- ✓ Zero-initialization code and procedures
- ✓ Debug information support (ASD format)
- ✓ C structure definitions
- ✓ Detailed examples and reference tables

**Quick Facts:**
- Header: Exactly 128 bytes (32 words)
- Code starts at: 0x80 (executable AIF) or base + 0x80
- Base address on RISC OS: 0x8000 (virtual)
- Support for compression, relocation, debugging
- Position-independent code capable

**Key Sections:**
- File Layout (uncompressed, compressed, post-relocation)
- Header Specification (32 words with detailed explanation)
- Image Sections (RO, RW, ZI, Debug, relocation code)
- Relocation Code Examples
- Decompression Overview
- Zero-Init Algorithm with step-by-step breakdown

---

### 2. AOF_FORMAT_COMPLETE.md (ARM Object Format)

**File Size:** 31 KB
**Purpose:** Object file format (compiler and assembler output)
**Used By:** All ARM language processors, linkers, librarians

**Key Topics Covered:**
- ✓ Chunk File Format foundation (required for understanding)
- ✓ AOF structure and chunk requirements
- ✓ Header chunk (OBJ_HEAD) with area declarations
- ✓ Areas chunk (OBJ_AREA) with data and relocations
- ✓ Complete relocation directive specification
- ✓ Symbol table chunk (OBJ_SYMT) with attributes
- ✓ String table chunk (OBJ_STRT)
- ✓ Identification chunk (OBJ_IDFN)
- ✓ Symbol types and attributes (15+ types)
- ✓ Relocation formulas and types
- ✓ C structure definitions with helpers
- ✓ Complete processing examples

**Quick Facts:**
- Header: 6 fixed words + 5 words per area
- Minimum: OBJ_HEAD + OBJ_AREA required
- Typical: 5 chunks (includes IDFN, SYMT, STRT)
- Chunk count: Usually 8 slots reserved
- Endianness: Supports both little and big-endian
- Symbol count: Typically 10-1000+ per file

**Key Sections:**
- Chunk File Format (essential foundation)
- AOF Structure (required/optional chunks)
- Header Chunk with area declarations
- Area Types (absolute, relocatable, code, common, zero-init, debug)
- Relocation Directives (format, types, formulas)
- Symbol Table (attributes, scope, types)
- String Table (layout and references)
- C Structure definitions
- Processing examples

---

### 3. ALF_FORMAT_COMPLETE.md (ARM Object Library Format)

**File Size:** 24 KB
**Purpose:** Archive format for organizing multiple object files
**Used By:** ARM librarian tools, linkers

**Key Topics Covered:**
- ✓ Library file format overview
- ✓ Chunk File Format foundation (shared with AOF)
- ✓ Required chunks (LIB_DIRY, LIB_TIME, LIB_VRSN, LIB_DATA)
- ✓ Directory chunk (LIB_DIRY) with entries
- ✓ Time stamp encoding (centiseconds since 1900)
- ✓ Version chunk (always = 1)
- ✓ Data chunks (one per member)
- ✓ Object Library extensions (OFL_SYMT, OFL_TIME)
- ✓ Robustness and compatibility guidelines
- ✓ C structure definitions
- ✓ Library creation and scanning examples

**Quick Facts:**
- Directory entries: Variable-length, 4-byte aligned
- Time base: 1900-01-01 00:00:00 (centiseconds)
- Version: Always 1 in current format
- Member format: Usually complete AOF files
- Minimum: 1 member (4 chunks minimum)
- Typical: 5-50 members per library

**Key Sections:**
- Library File Format
- Directory Chunk (variable-length entries)
- Time Stamp Format (6-byte centisecond count)
- Version Chunk (single word = 1)
- Data Chunks (raw member data)
- Object Library Extensions (for symbol indexing)
- C Structure definitions
- Library creation and scanning examples
- Robustness guidelines

---

## Relationships Between Formats

### Compilation Pipeline

```
┌─────────────────┐
│  Source Code    │  (C, Pascal, Fortran, ASM)
│  (*.c, *.asm)   │
└────────┬────────┘
         │
         ↓
┌─────────────────┐
│ Compiler/ASM    │
│   (CC, ObjAsm)  │
└────────┬────────┘
         │
         ↓
┌─────────────────────────────────┐
│  AOF Object File                │  ← AOF_FORMAT_COMPLETE.md
│  (*.o)                          │
│  • OBJ_HEAD (header)            │
│  • OBJ_AREA (code, data)        │
│  • OBJ_SYMT (symbols)           │
│  • OBJ_STRT (strings)           │
│  • OBJ_IDFN (identification)    │
└────────┬────────────────────────┘
         │
         ├─────────────────────────────────────┐
         │                                     │
         ↓                                     ↓
    ┌─────────────────┐               ┌──────────────────┐
    │  Single File    │               │ Multiple Files   │
    │  Direct Link    │               │ Group Via Lib    │
    └────────┬────────┘               └────────┬─────────┘
             │                                 │
             │                        ┌────────↓───────────┐
             │                        │ Librarian (armar)  │
             │                        │ (LibFile tool)     │
             │                        └────────┬───────────┘
             │                                 │
             │                     ┌───────────↓────────────────┐
             │                     │  ALF Library File          │
             │                     │  (*.a, *.lib)              │
             │                     │  • LIB_DIRY (directory)    │
             │                     │  • LIB_TIME (mod time)     │
             │                     │  • LIB_VRSN (version)      │
             │                     │  • LIB_DATA (members)      │
             │                     │  • OFL_SYMT (optional)     │
             │                     │  • OFL_TIME (optional)     │
             │                     └────────┬──────────────────┘
             │                              │
             └──────────────┬───────────────┘
                            │
                            ↓
                 ┌──────────────────────┐
                 │  Linker (armlink)    │
                 │  (Link tool)         │
                 └──────────┬───────────┘
                            │
                            ↓
            ┌────────────────────────────────────┐
            │  AIF Executable Image              │
            │  (AIF file, !RunImage, etc.)       │
            │  • 128-byte header                 │  ← AIF_FORMAT_COMPLETE.md
            │  • Read-only section (code)        │
            │  • Read-write section (data)       │
            │  • Debug info (optional)           │
            │  • Relocation code (optional)      │
            │  • Relocation list (optional)      │
            └────────────┬───────────────────────┘
                         │
                         ↓
                   ┌──────────────┐
                   │  Execution   │
                   │  (OS/Runtime) │
                   └──────────────┘
```

### Data Flow

**AOF → ALF:**
- Tool: Librarian (LibFile, armar)
- Input: Multiple AOF files
- Output: Single ALF library
- Purpose: Organize object files

**AOF → AIF:**
- Tool: Linker (armlink, Link)
- Input: AOF files and/or ALF libraries
- Output: Single AIF executable
- Purpose: Resolve symbols, generate executable

**ALF → AIF:**
- Tool: Linker
- Input: ALF library (extracts needed members)
- Output: AIF executable
- Purpose: Selective linking from library

---

## Cross-Format References

### Chunk File Format (Used by AOF and ALF)

**Definition:** AOF/ALF header structure found in both formats

| Aspect | Location |
|--------|----------|
| Specification | AOF_FORMAT_COMPLETE.md → "Chunk File Format Foundation" |
| Magic Number | 0xC3CBC6C5 |
| Purpose | Enable modular data storage |
| Chunks | Named data sections (LIB_*, OBJ_*) |
| Directory | Offset/size table at file start |

### Symbol Resolution (AOF ↔ Linking ↔ AIF)

| Step | In Format | Details |
|------|-----------|---------|
| 1. Define | AOF | OBJ_SYMT entries marked as "defined" |
| 2. Reference | AOF | OBJ_SYMT entries marked as "undefined" |
| 3. Collect | ALF | OFL_SYMT indexes all symbols (if present) |
| 4. Resolve | Linker | Matches definitions to references |
| 5. Relocate | AOF | Relocation directives applied |
| 6. Finalize | AIF | Static addresses in executable |

### String Storage (All Formats)

| Format | Chunk | Field |
|--------|-------|-------|
| AOF | OBJ_STRT | Area names, symbol names |
| ALF | LIB_DIRY | Member names |
| AIF | Header | (none - uses offsets directly) |

---

## Quick Reference Tables

### Format Comparison

| Aspect | AOF | ALF | AIF |
|--------|-----|-----|-----|
| **Purpose** | Compiler output | Archive | Executable |
| **Chunk-based** | Yes | Yes | No |
| **Relocatable** | Yes | Yes (members) | Optional |
| **Contains Code** | Yes | Yes (in members) | Yes |
| **Symbols** | Yes (OBJ_SYMT) | Yes (OFL_SYMT) | No (resolved) |
| **Typical Size** | 1-500 KB | 100 KB - MB | 10-500 KB |
| **Endianness** | Flexible | Flexible | Fixed |
| **Executable** | No | No | Yes |

### Header Sizes

| Format | Component | Size | Details |
|--------|-----------|------|---------|
| AOF | Fixed header | 24 bytes | 6 words |
| AOF | Per area | 20 bytes | 5 words each |
| AIF | Complete header | 128 bytes | 32 words fixed |
| ALF | Directory entry | Variable | 12 bytes + data + padding |
| ALF | Time stamp | 8 bytes | 2 words |
| ALF | Version | 4 bytes | 1 word = 1 |

### Chunk Type Summary

| Chunk Name | Format | Required | Purpose |
|-----------|--------|----------|---------|
| OBJ_HEAD | AOF | Yes | Object header + areas |
| OBJ_AREA | AOF | Yes | Code, data, relocations |
| OBJ_SYMT | AOF | No | Symbol definitions |
| OBJ_STRT | AOF | No | String table |
| OBJ_IDFN | AOF | No | Tool identification |
| LIB_DIRY | ALF | Yes | Member directory |
| LIB_TIME | ALF | Yes | Library mod time |
| LIB_VRSN | ALF | Yes | Version (=1) |
| LIB_DATA | ALF | Yes | Member data (1+) |
| OFL_SYMT | ALF | No | Symbol index |
| OFL_TIME | ALF | No | Symbol mod time |

---

## Documentation Statistics

### Content Summary

| Document | Lines | Words | Key Sections |
|----------|-------|-------|--------------|
| AIF_FORMAT_COMPLETE.md | 1,200+ | 8,000+ | 10 major sections |
| AOF_FORMAT_COMPLETE.md | 1,100+ | 7,500+ | 10 major sections |
| ALF_FORMAT_COMPLETE.md | 850+ | 5,500+ | 10 major sections |
| **TOTAL** | **3,150+** | **21,000+** | **30 major sections** |

### Coverage Details

**Specification Completeness:**
- ✓ Header layouts (byte-by-byte, bit-by-bit)
- ✓ Chunk structures (all types)
- ✓ Bit field definitions (all flags)
- ✓ Data formats (all encoding methods)
- ✓ C structure definitions (ready to use)
- ✓ Example values (real-world cases)
- ✓ Processing examples (step-by-step)
- ✓ Helper functions (code snippets)

**Cross-References:**
- ✓ Format relationships documented
- ✓ Data flow diagrams provided
- ✓ Summary tables for comparison
- ✓ Quick reference sections
- ✓ Common patterns explained

---

## How to Use These Documents

### For Understanding the Formats

1. **Start with AIF_FORMAT_COMPLETE.md** if interested in executable images
2. **Start with AOF_FORMAT_COMPLETE.md** if interested in compilation
3. **Start with ALF_FORMAT_COMPLETE.md** if interested in archives

### For Implementation

1. **Read the "C Structure Definitions" sections** for data layout
2. **Study the "Examples" sections** for typical usage
3. **Reference the detailed tables** for field meanings
4. **Use the helper functions** as starting points

### For Troubleshooting

1. **Check the format tables** for valid values
2. **Review the examples** for typical patterns
3. **Verify constraints** (alignment, size, ranges)
4. **Check cross-references** between formats

### For Reference

1. **Keep the quick reference tables handy**
2. **Use table of contents** for quick navigation
3. **Search for specific field names** or concepts
4. **Cross-check format relationships**

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-02-07 | Initial comprehensive specification |

---

## Additional Resources

### Related Documentation in Repository

- `RISC_OS_ARM_AIF_Format.md` - Overview article on AIF
- `AIF-1989.md` - Original 1989 AIF specification
- `AIF-1993.md` - 1993 extended AIF specification
- `RISC_OS_PRMs_Code_file_formats.md` - Original RISC OS PRMs
- `arm_image_format.md` - ARM DUI documentation

### External References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals (PLG-* documents)
- ARM Linker documentation
- ARM Assembler documentation

---

## Notes

### Terminology

- **Word:** 32-bit (4-byte) value
- **Halfword:** 16-bit (2-byte) value
- **Byte:** 8-bit (1-byte) value
- **Offset:** Byte position from start of section/file
- **Base Address:** Starting address of image/area in memory
- **Relocation:** Adjustment of address values

### Conventions

- All offsets and addresses are in bytes unless stated otherwise
- All sizes are in bytes unless stated otherwise
- Hex values shown as 0xHHHHHHHH (32-bit) or 0xHH (8-bit)
- Bit numbering: Bit 0 is least significant
- Byte order: Little-endian unless otherwise specified

### Standards

- References: ARM DUI 0041C (primary), RISC OS PRMs (secondary)
- Compatibility: These specifications cover current standard versions
- Extensions: Tools may add custom chunks/fields while maintaining compatibility

---

**Documentation prepared for:** 3DO Devkit Project
**Format versions covered:** AIF 3.00+, AOF 310, ALF 1.0
**Endianness:** Both little-endian and big-endian documented
**Platform support:** RISC OS, 3DO, ARM embedded systems

