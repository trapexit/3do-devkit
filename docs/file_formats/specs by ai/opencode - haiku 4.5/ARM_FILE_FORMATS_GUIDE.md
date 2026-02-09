# Complete ARM File Formats Guide

**Comprehensive Reference for AIF, AOF, and ALF Formats**

**Version:** 1.0  
**Last Updated:** February 2025  
**Documentation Status:** Complete

---

## Overview

This guide provides complete reference documentation for the three primary ARM file formats used in embedded systems, RISC OS, 3DO console development, and ARM-based systems:

1. **AIF (ARM Image Format)** - Executable images
2. **AOF (ARM Object Format)** - Compiled/assembled object files
3. **ALF (ARM Library Format)** - Object file archives

Together, these formats form the complete ARM toolchain pipeline:

```
Source Code
    ↓
[Compiler/Assembler]
    ↓
AOF (Object Files)
    ↓
[Librarian] → ALF (Libraries)
    ↓
[Linker]
    ↓
AIF (Executable Image)
    ↓
[Loader/Debugger]
    ↓
Running Program
```

---

## Document Reference

### AIF Comprehensive Specification (1,329 lines)

**File:** `AIF_COMPREHENSIVE_SPECIFICATION.md`

**Coverage:**
- Complete AIF format definition
- Header structure (128 bytes) with all 32 fields
- Three AIF types (executable, non-executable, extended)
- Compression and decompression mechanisms
- Self-relocation algorithms
- Zero-initialization code
- Debugging support
- Position-independent code techniques
- Entry veneers for shared libraries
- Reference C implementation

**Key Sections:**
- Format Characteristics
- AIF Header Specification (detailed field-by-field)
- Binary Image Section
- Relocation Mechanism
- Self-Relocation Code
- Compression Support
- Extended AIF Format
- Reference Implementation

**For Whom:**
- Developers creating loaders/bootloaders
- Debugger developers
- Firmware engineers
- 3DO game developers
- RISC OS application developers

### AOF Comprehensive Specification (1,092 lines)

**File:** `AOF_COMPREHENSIVE_SPECIFICATION.md`

**Coverage:**
- Chunk File Format fundamentals
- Complete AOF structure and organization
- Fixed and variable header formats
- Area declarations and attributes
- Code and data area handling
- Relocation directives (8 different types)
- Symbol table definitions (16-byte entries)
- String table organization
- Identification chunk
- Complete bit-level attribute definitions
- Linker concepts and processing
- Reference C implementation

**Key Sections:**
- Chunk File Format
- AOF Header Chunk (OBJ_HEAD)
- Areas Chunk (OBJ_AREA)
- Relocation Directives
- Symbol Table (OBJ_SYMT)
- String Table (OBJ_STRT)
- Identification Chunk (OBJ_IDFN)
- Attributes and Alignment
- Linker Concepts

**For Whom:**
- Compiler/assembler developers
- Linker developers
- Tool chain engineers
- Library creation utilities
- Build system implementers

### ALF Comprehensive Specification (907 lines)

**File:** `ALF_COMPREHENSIVE_SPECIFICATION.md`

**Coverage:**
- Library format fundamentals
- Chunk organization for libraries
- Directory chunk structure and entry format
- Timestamp encoding and interpretation
- Version and data chunks
- Object code library extensions
- External symbol table (OFL_SYMT)
- Library operations (create, add, remove, update)
- Build system integration
- Reference C implementation

**Key Sections:**
- Chunk File Structure
- Library File Format
- Directory Chunk (LIB_DIRY)
- Time Stamp Chunks
- Version Chunk (LIB_VRSN)
- Data Chunk (LIB_DATA)
- Object Code Libraries
- Symbol Table Chunk (OFL_SYMT)
- Library Operations

**For Whom:**
- Library creation tool developers
- Build system engineers
- Linker developers
- Archive utility developers
- Package management system designers

---

## Quick Reference

### File Format Relationships

```
┌─────────────────────────────────────────┐
│          Source Code Files              │
│  (C, C++, Assembly, etc.)               │
└─────────────┬───────────────────────────┘
              │
         [COMPILE/ASSEMBLE]
              │
         Produces: AOF Files
┌─────────────┴───────────────────────────┐
│     AOF Object Files (1 per module)     │
│  - Code areas (RO, PI, etc.)            │
│  - Data areas (RW, ZI, etc.)            │
│  - Relocation directives                │
│  - Symbol table (exported & imported)   │
└─────────────┬───────────────────────────┘
              │
         [LIBRARIAN / ARCHIVER]
              │
         Produces: ALF Files
┌─────────────┴───────────────────────────┐
│    ALF Library File (Archive)           │
│  - Multiple AOF members                 │
│  - Directory of members                 │
│  - Optional symbol table (OFL_SYMT)     │
│  - Timestamps and metadata              │
└─────────────┬───────────────────────────┘
              │
         [LINKER]
              │
    Processes: AOF files and ALF libraries
    Performs:
    - Symbol resolution
    - Relocation processing
    - Memory layout
    - Dead code elimination
              │
         Produces: AIF File
┌─────────────┴───────────────────────────┐
│      AIF Executable Image               │
│  - 128-byte header                      │
│  - Read-only area (code + constants)    │
│  - Read-write area (initialized data)   │
│  - Optional debugging data              │
│  - Optional relocation/decompression    │
│  - Self-contained, executable           │
└─────────────┬───────────────────────────┘
              │
         [LOADER / DEBUGGER]
              │
              ↓
         Running Program
```

### Format Sizes and Constraints

| Aspect | AIF | AOF | ALF |
|--------|-----|-----|-----|
| **Header Size** | 128 bytes (fixed) | 24+ bytes (variable) | 12 bytes (chunk file) |
| **Areas per File** | N/A (linked) | Multiple (RO, RW, ZI, debug, etc.) | Many (one per member) |
| **Max File Size** | 2^32-1 bytes | 2^32-1 bytes | 2^32-1 bytes |
| **Max Area Size** | 2^32-1 bytes | 2^32-1 bytes | 2^32-1 bytes |
| **Max Symbol Count** | N/A | 2^32-1 | 2^32-1 (via members) |
| **Alignment** | 4-byte (words) | 4-byte (words) | 4-byte (chunks) |
| **Endianness** | Target-specific | Target-specific | Target-specific |

### Typical File Sizes

| File Type | Typical Size | Example |
|-----------|--------------|---------|
| Small AOF | 2-10 KB | Simple module |
| Medium AOF | 10-100 KB | Average module with debug info |
| Large AOF | 100-500 KB | Complex module with symbols |
| Small ALF | 10-50 KB | Simple utility library |
| Medium ALF | 50-500 KB | Standard library |
| Large ALF | 500 KB - 5 MB | System library |
| Small AIF | 5-20 KB | Bootloader, minimal executable |
| Medium AIF | 20-200 KB | Typical application |
| Large AIF | 200 KB - 5 MB | Complex application |

---

## Processing Pipeline Details

### Compilation to Object Code (Compiler/Assembler → AOF)

**Input:** Source code (C, C++, Assembly)

**Processing:**
1. Lexical analysis and tokenization
2. Syntax parsing
3. Semantic analysis
4. Code generation
5. Assembly to binary
6. Segment organization (code, data, zero-init)
7. Generate symbol table and export declarations
8. Generate relocation directives for external references

**Output:** AOF file with:
- OBJ_HEAD: Header and area declarations
- OBJ_AREA: Compiled code and data
- OBJ_SYMT: Symbols (exports and imports)
- OBJ_STRT: String table
- OBJ_IDFN: Tool identification

### Library Creation (Librarian ALF)

**Input:** One or more AOF files

**Processing:**
1. Read all AOF files
2. Create directory entries for each member
3. Assign chunk indices to LIB_DATA sections
4. If object library: extract external symbols, build OFL_SYMT
5. Create timestamps (LIB_TIME, OFL_TIME)
6. Write chunk file structure

**Output:** ALF file with:
- LIB_DIRY: Member directory
- LIB_TIME: Library timestamp
- LIB_VRSN: Format version (=1)
- LIB_DATA: Member AOF files
- OFL_SYMT: Symbol table (if object library)
- OFL_TIME: Symbol table timestamp (if object library)

### Linking (Linker: AOF+ALF → AIF)

**Input:** AOF files and/or ALF libraries

**Processing:**
1. Read all AOF files
2. For symbol references:
   - Look up in loaded AOF files
   - If not found, query ALF symbol tables
   - Load required ALF members
3. Resolve all external references
4. Check for unresolved symbols (error if found)
5. Assign memory addresses to areas
6. Apply relocation directives:
   - Calculate relocation value for each directive
   - Update referenced fields
7. Organize into AIF sections:
   - Read-only (code, constants, debug data)
   - Read-write (initialized data)
   - Zero-initialized (size only)
8. Generate self-relocation code if image is relocatable
9. Generate compression wrapper if compression requested

**Output:** AIF file with:
- Header (128 bytes, with linked addresses)
- Read-only area (all code and constants)
- Read-write area (initialized data)
- Optional: Debug section
- Optional: Self-relocation code and list
- Optional: Compression header and data

### Loading and Execution (Loader → Running Program)

**Input:** AIF file, target memory

**Processing:**
1. Read AIF header
2. Check for compression:
   - If compressed: decompress image
3. Load image at target address
4. Check for self-relocation:
   - If self-relocating: execute relocation code
5. Execute zero-init code to clear ZI areas
6. Execute any initialization code (C runtime, constructors)
7. Jump to entry point

**Output:** Running program with:
- Code and constants at correct addresses
- Data areas properly initialized
- Entry point executed

---

## Design Pattern Comparisons

### AIF vs AOF vs ALF

| Characteristic | AIF | AOF | ALF |
|---|---|---|---|
| **Primary Use** | Executable | Intermediate | Archive |
| **Entry Point** | Yes | Possibly (one per file) | No (inherited from members) |
| **Relocation** | Optional | Required | Optional (per member) |
| **Symbols** | No | Yes (full table) | No (but members have symbols) |
| **Debugging** | Optional | Possibly | Depends on members |
| **Reusable** | No (specific to app) | Yes (can be relinked) | Yes (standard archive) |
| **Single File** | Yes | Yes | Yes (multiple members) |
| **Compression** | Optional | No | No (but members may be) |
| **Position Independent** | Optional | Optional | Optional (members) |
| **Header Info** | Fixed 128 bytes | Variable | Chunk-based |

### Chunk File Format Usage

Both AOF and ALF use Chunk File Format as container:

**AOF Chunks:**
- OBJ_HEAD: Metadata
- OBJ_AREA: Code/data
- OBJ_SYMT: Symbols
- OBJ_STRT: Names
- OBJ_IDFN: Tool info

**ALF Chunks:**
- LIB_DIRY: Member directory
- LIB_DATA: Member files (multiple)
- LIB_TIME: Archive timestamp
- LIB_VRSN: Format version
- OFL_SYMT: Symbol index (optional)
- OFL_TIME: Symbol timestamp (optional)

---

## Architecture-Specific Considerations

### ARM Instruction Modes

**32-bit vs 26-bit:**
- Address Mode field (0x30) specifies mode
- Affects relocation and procedure calls
- Modern systems: 32-bit

**Thumb vs ARM:**
- Thumb code areas marked in attributes
- Different instruction encoding
- Mixed ARM/Thumb requires interworking code

**Position Independence:**
- PC-relative branches
- Data via base registers
- Essential for ROM/shared code

### APCS Variants

**ARM Procedure Call Standard:**
- Defines register usage, calling conventions
- Multiple variants for different scenarios
- Reentrant vs non-reentrant
- With/without stack checks

### Endianness

All three formats are endianness-aware:
- AIF: Matches target processor
- AOF: Detectable from magic number
- ALF: Inherits from contained AOF files

---

## Tool Support

### Standard ARM Toolchain

**Compiler (armcc):**
- Input: C/C++ source
- Output: AOF
- Creates: Multiple AOF files for separate compilation units

**Assembler (armasm/ObjAsm):**
- Input: Assembly source
- Output: AOF
- Can be mixed with compiler output

**Linker (armlink/link):**
- Input: AOF files + ALF libraries
- Output: AIF executable
- Performs: Symbol resolution, relocation, layout

**Librarian (arlib/armlib):**
- Input: AOF files
- Output: ALF library
- Operations: Create, add, remove, list members

**Debugger (DDE, Vigil, etc.):**
- Input: AIF files (with debug info)
- Operations: Set breakpoints, examine variables, step code

### Open Source Alternatives

- **GCC (arm-none-eabi-gcc):** Can produce AOF-compatible code (with configuration)
- **LLVM:** ARM backend can target AOF
- **Rust for ARM:** Cross-compilation support

### 3DO-Specific Tools

- **modbin:** Converts AIF to 3DO bootable format
- **celtcc:** 3DO C compiler (produces AOF)
- **CeltAsm:** 3DO assembler (produces AOF)
- **BroadcastShell:** Linking and build environment

---

## Common Issues and Solutions

### Issue: "Undefined External Reference"

**Cause:**
- Symbol referenced in AOF but not defined
- Not linked with library containing definition
- Symbol exported from wrong library

**Solution:**
1. Check which library exports symbol (search OFL_SYMT)
2. Add library to link command
3. Check symbol spelling and case sensitivity
4. Verify symbol is global (not static/local)

### Issue: "Address Overflow"

**Cause:**
- Address constant doesn't fit in field
- PC-relative branch offset too large
- Linked at wrong address

**Solution:**
1. Check linked base addresses
2. Use PC-relative addressing if available
3. Use long branches (multiple instructions)
4. Enable position independence

### Issue: "Relocation Overflow"

**Cause:**
- Relocation value doesn't fit in field
- 16-bit field can't hold 32-bit address
- Byte field can't hold multi-byte value

**Solution:**
1. Check field size in relocation directive
2. Use different instruction sequence
3. Reorganize code/data layout
4. Contact toolchain vendor

### Issue: "Symbol Table Mismatch"

**Cause:**
- OFL_SYMT out of sync with members
- Library corrupted or partially updated
- Endianness mismatch

**Solution:**
1. Rebuild library from members
2. Use standard librarian tool
3. Check endianness consistency
4. Verify library integrity

---

## Performance Optimization

### Linking Performance

**Factors:**
- Number of objects/libraries
- Symbol table size
- Relocation count
- Output size

**Optimization:**
- Use libraries (ALF) for large codebase
- Symbol index (OFL_SYMT) speeds lookup
- Incremental linking (rebuild only changed modules)
- Link-time optimization (whole program)

### Execution Performance

**Factors:**
- Self-relocation overhead
- Decompression overhead
- ZI initialization time
- Startup code complexity

**Optimization:**
- Pre-relocated images (load address = link address)
- Compression for slow media only
- Parallel ZI init (multi-core)
- Minimal startup code

### File Size Optimization

**Techniques:**
- Compression (reduces by 30-60%)
- Dead code elimination (linker feature)
- Section merging (combine similar data)
- Strip debug info for release

---

## Security Considerations

**AIF Files:**
- No inherent security checks
- Verify source before loading
- Use digital signatures if needed
- Implement loader security

**AOF Files:**
- No validation of internal structure
- Trust compiler/assembler output
- Linker validates relocation compatibility

**ALF Files:**
- Verify member integrity
- Check symbol table for tampering
- Implement archive signing

---

## Conclusion

The AIF, AOF, and ALF formats together provide a complete, efficient system for compiling, archiving, linking, and executing code on ARM systems. Understanding these formats is essential for:

- Developers using ARM toolchains
- Tool chain engineers and maintainers
- System programmers and bootloader developers
- 3DO console game developers
- RISC OS application developers
- Embedded systems engineers

Each format has a specific role in the pipeline, and together they enable modern ARM software development while remaining compatible with legacy systems.

For complete details, refer to the individual comprehensive specifications:

1. **AIF_COMPREHENSIVE_SPECIFICATION.md** - Full AIF reference
2. **AOF_COMPREHENSIVE_SPECIFICATION.md** - Full AOF reference
3. **ALF_COMPREHENSIVE_SPECIFICATION.md** - Full ALF reference

---

## Related Resources

- ARM Software Development Toolkit Reference Guide (DUI 0041C)
- RISC OS Programmer's Reference Manuals
- ARM Linker User Guide
- ARM Assembler User Guide
- 3DO Developer Documentation
- gcc-arm-none-eabi documentation

---

**Last Updated:** February 8, 2025  
**Total Documentation:** 3,328 lines across 4 files  
**Status:** Complete and Comprehensive
