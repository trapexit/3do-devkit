# ARM Image Format (AIF) — Complete File Format Specification

## Document Information

| Field | Value |
|-------|-------|
| **Format Name** | ARM Image Format (AIF), formerly Arthur Image Format |
| **Primary Use** | Executable images for ARM-based systems (RISC OS, 3DO, embedded systems) |
| **Key Features** | Self-relocation, compression support, debugging tables, position independence |
| **Header Size** | 128 bytes (32 words) |
| **Endianness** | Little-endian (target ARM byte order) |
| **Word Size** | 32 bits (4 bytes) |

---

## 1. Overview

ARM Image Format (AIF) is a simple yet powerful executable file format designed for ARM processors. Originally developed by Acorn Computers Limited for their RISC OS operating system, AIF has been used extensively in ARM-based systems including the 3DO game console and various embedded platforms.

AIF files consist of a 128-byte header followed by the executable code and initialized data. The format supports several advanced features including:

- **Self-decompression**: Images can be compressed and include self-decompressing code
- **Self-relocation**: Images can relocate themselves to execute at their load address
- **Workspace allocation**: Support for specifying minimum workspace requirements
- **Debugging support**: Integration with ARM Symbolic Debugger (ASD/armsd)
- **Zero-initialization**: Built-in support for zero-initializing data areas

### 1.1 Design Philosophy

AIF was designed with the following principles:

1. **Simplicity**: The format is straightforward to parse and implement
2. **Position Independence**: Images can be loaded at arbitrary addresses
3. **Self-contained**: Images can prepare themselves for execution without external loaders
4. **Extensibility**: Support for debugging tables and optional features
5. **Efficiency**: Minimal overhead for simple executables

---

## 2. AIF Variants

AIF exists in three distinct variants, each serving different purposes:

### 2.1 Executable AIF

Executable AIF is the most common variant, used for standalone applications (such as RISC OS `!RunImage` files).

**Characteristics:**
- Header is part of the image itself
- Image is entered at the first word of the header
- Header contains code to prepare the image for execution
- Fourth word of header is `BL <entrypoint>` (MSB = 0xEB)
- Base address = address where header is loaded
- Code starts at `base + 0x80`

**Execution Flow:**
```
Load Image → Header Execution → Decompression (if needed) → 
Self-Relocation (if needed) → Zero-Init → Enter Application
```

### 2.2 Non-Executable AIF

Non-executable AIF requires an external image loader to prepare it for execution.

**Characteristics:**
- Header describes but is not part of the image
- Fourth word contains entry point offset (MSB nibble = 0x0)
- Base address = address where code should be loaded
- Loader performs decompression, relocation, and zero-init
- Header is discarded after preparation

**Use Cases:**
- ROM firmware images
- Bootloaders
- Custom loading scenarios

### 2.3 Extended AIF

Extended AIF is a special form of non-executable AIF for scatter-loaded images.

**Characteristics:**
- Contains a chain of load region descriptors
- Each descriptor specifies load address and size
- Image loader places regions at specified memory locations
- Header at offset 0x38 contains offset to first region header

---

## 3. File Structure

### 3.1 Overall Layout

```
+------------------+
| AIF Header       |  128 bytes (fixed)
+------------------+
| Read-Only Area   |  Code + constants (may include header if executable)
+------------------+
| Read-Write Area  |  Initialized data
+------------------+
| Debugging Data   |  Optional (low-level and/or source-level)
+------------------+
| Self-Reloc Code  |  Position-independent (if self-relocating)
+------------------+
| Relocation List  |  Words to relocate, terminated by -1
+------------------+
```

### 3.2 Compressed AIF Layout

When an AIF image is compressed, the layout changes:

```
+------------------+
| AIF Header       |  128 bytes (NOT compressed)
+------------------+
| Compressed Image |  Compressed code + data
+------------------+
| Decompression    |  Position-independent data
|     Data         |
+------------------+
| Decompression    |  Position-independent code
|     Code         |
+------------------+
```

The header is never compressed to allow loaders to read image parameters before decompression.

---

## 4. AIF Header Specification

The AIF header is exactly 128 bytes (32 32-bit words) in size. All multi-byte fields use little-endian byte order (least significant byte at lowest address).

### 4.1 Header Word Definitions

| Offset | Size | Name | Description |
|--------|------|------|-------------|
| 0x00 | 4 | `BL DecompressCode` | Branch to decompression code; NOP if not compressed |
| 0x04 | 4 | `BL SelfRelocCode` | Branch to self-relocation code; NOP if not self-relocating |
| 0x08 | 4 | `BL ZeroInitCode` | Branch to zero-initialization code; NOP if none |
| 0x0C | 4 | `BL EntryPoint` | Branch to application entry point (executable AIF) |
| 0x0C | 4 | `Entry Offset` | Entry point offset from base (non-executable AIF) |
| 0x10 | 4 | `SWI Exit` | Exit instruction (usually `SWI 0x11`) |
| 0x14 | 4 | `ImageROSize` | Size of read-only area (including header if executable) |
| 0x18 | 4 | `ImageRWSize` | Size of read-write area (multiple of 4 bytes) |
| 0x1C | 4 | `ImageDebugSize` | Size of debugging data (multiple of 4 bytes) |
| 0x20 | 4 | `ImageZISize` | Size of zero-init area (multiple of 4 bytes) |
| 0x24 | 4 | `ImageDebugType` | Debugging type (0-3) |
| 0x28 | 4 | `ImageBase` | Address where image was linked |
| 0x2C | 4 | `WorkSpace` | Minimum workspace (obsolete in modern AIF) |
| 0x30 | 4 | `AddressMode` | 26-bit/32-bit mode flags + separate data base flag |
| 0x34 | 4 | `DataBase` | Address where image data was linked (if separate) |
| 0x38 | 8 | `Reserved[2]` | Reserved (used by Extended AIF) |
| 0x40 | 4 | `DebugInit` | Debug initialization instruction (NOP if unused) |
| 0x44 | 60 | `ZeroInitCode[15]` | Zero-initialization code (15 words) |

### 4.2 Detailed Field Specifications

#### 4.2.1 Decompression Code Pointer (0x00)

For compressed images: `BL <decompression_routine>`

For uncompressed images: `MOV R0, R0` (NOP = 0xE1A00000)

**NOP Encoding:**
```
E1 A0 00 00
↑   ↑   ↑   ↑
↑   ↑   ↑   └─ Rn = 0
↑   ↑   └────── Rd = 0
↑   └────────── shifter = 0 (MOV)
└────────────── Condition = EQ (14)
```

#### 4.2.2 Self-Relocation Code Pointer (0x04)

For self-relocating images: `BL <relocation_routine>`

For non-relocating images: `MOV R0, R0` (NOP)

#### 4.2.3 Zero-Initialization Code Pointer (0x08)

For images with zero-init data: `BL <zeroinit_routine>`

For images without zero-init: `MOV R0, R0` (NOP)

#### 4.2.4 Entry Point (0x0C)

**Executable AIF:**
```
BL <entry_point_address>
MSB = 0xEB (ARM BL instruction encoding)
```

**Non-Executable AIF:**
Offset from base address to entry point
MSB nibble = 0x0

#### 4.2.5 Program Exit Instruction (0x10)

Default: `SWI 0x11` (RISC OS `OS_Exit`)

Alternative: Branch to self (`B .`) for systems without exit SWI

**SWI Encoding:**
```
EF 00 00 11
↑   ↑   ↑   ↑
↑   ↑   ↑   └─ SWI number = 0x11
↑   ↑   └────── Ignored
↑   └────────── Ignored
└────────────── SWI instruction
```

#### 4.2.6 Image Read-Only Size (0x14)

Size in bytes of the read-only section (code + constants).

**Executable AIF:** Includes the 128-byte header
**Non-Executable AIF:** Excludes the header

Must be a multiple of 4 bytes.

#### 4.2.7 Image Read-Write Size (0x18)

Size in bytes of the initialized read-write data section.

Must be a multiple of 4 bytes. Zero if no read-write data exists.

#### 4.2.8 Image Debug Size (0x1C)

Size in bytes of all debugging data.

**Format:**
```
Bits 0-3:   Debug type (same as offset 0x24)
Bits 4-31:  Size of low-level debugging data
High-level debugging data follows immediately after
```

Must be a multiple of 4 bytes. Zero if no debugging data.

#### 4.2.9 Image Zero-Init Size (0x20)

Size in bytes of the zero-initialized data section.

This area is filled with zeros at runtime. Must be a multiple of 4 bytes. Zero if no zero-init data.

#### 4.2.10 Image Debug Type (0x24)

| Value | Meaning | Description |
|-------|---------|-------------|
| 0 | None | No debugging data present |
| 1 | Low-level | Low-level debugging data only (armsd) |
| 2 | Source-level | Source-level debugging data only (ASD) |
| 3 | Both | Both low-level and source-level debugging |

All other values reserved to ARM Limited.

#### 4.2.11 Image Base (0x28)

The address at which the image was linked.

For relocatable images, this is the preferred load address. The actual load address may differ, requiring relocation.

#### 4.2.12 WorkSpace (0x2C)

**Obsolete field** — not used in modern AIF implementations.

Historically specified minimum workspace in bytes to be reserved by self-moving relocatable images.

#### 4.2.13 Address Mode (0x30)

```
+---+---+---+---+
| Flags     | Mode|
+---+---+---+---+
   8 bits   8 bits
```

**Mode (LSB):**
| Value | Meaning |
|-------|---------|
| 0 | Old-style 26-bit AIF header |
| 26 | Image linked for 26-bit ARM mode |
| 32 | Image linked for 32-bit ARM mode |

**Flags:**
| Bit | Mask | Meaning |
|-----|------|---------|
| 8 | 0x100 | Separate code/data base (use DataBase field) |

**Interpretation:**
- 26-bit mode: PSR bits in R15 are significant
- 32-bit mode: Full 32-bit addressing
- Bit 8 set: Image uses separate data base address

#### 4.2.14 Data Base (0x34)

Address where the image data was linked (when bit 8 of AddressMode is set).

Used for images with separate code and data segments.

#### 4.2.15 Reserved (0x38-0x3C)

Two reserved words, initialized to zero.

**Extended AIF Usage:**
In Extended AIF images, word at 0x38 contains byte offset to first load region header.

#### 4.2.16 Debug Init (0x40)

Debug initialization instruction, usually `MOV R0, R0` (NOP).

If used, typically a SWI to alert a debugger that a debuggable image is starting.

#### 4.2.17 Zero-Init Code (0x44-0x7C)

15 words of zero-initialization code.

This position-independent code zeros the zero-init area. The standard implementation is:

```arm
ZeroInit:
    SUB    ip, lr, pc         ; Calculate header base
    ADD    ip, pc, ip
    LDMIB  ip, {r0,r1,r2,r4} ; Load sizes
    SUB    ip, ip, #16        ; -> image base
    ADD    ip, ip, r0         ; + RO size
    ADD    ip, ip, r1         ; + RW size = ZI base
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMPS   r4, #0
00  MOVLE  pc, lr             ; Done
    STMIA  ip!, {r0,r1,r2,r3} ; Zero 16 bytes
    SUBS   r4, r4, #16
    B      %B00
```

---

## 5. Extended AIF Region Headers

Extended AIF images contain region descriptors following the main header.

### 5.1 Region Header Format (44 bytes)

| Word | Description |
|------|-------------|
| 0 | File offset of next region header (0 = no more) |
| 1 | Load address for this region |
| 2 | Size in bytes (multiple of 4) |
| 3-10 | Region name (32 characters, null-padded) |

### 5.2 Region Data Layout

```
Region Header (44 bytes)
+----------------------+
| Next Offset (4)      |
+----------------------+
| Load Address (4)     |
+----------------------+
| Size (4)             |
+----------------------+
| Name (32)            |
+----------------------+
| Region Data          |  Follows header
| ...                  |
+----------------------+
```

---

## 6. Position Independence and Relocation

### 6.1 Relocation Concepts

AIF supports two types of position independence:

1. **Load-time relocation**: Image relocates itself before execution
2. **Position-independent code (PIC)**: Code uses PC-relative addressing

### 6.2 Self-Relocation Mechanism

The self-relocation code is position-independent and handles:

1. **Determining load address**: Calculates actual load address from PC
2. **Relocation offset**: Computes difference between load and linked addresses
3. **Applying relocations**: Updates all absolute addresses in the image
4. **Self-moving (optional)**: Moves image to higher memory if required

### 6.3 Relocation List Format

Following the self-relocation code is a list of byte offsets:

```
+----------------------+
| Offset 1             |  Byte offset from header start
+----------------------+
| Offset 2             |
+----------------------+
| ...                  |
+----------------------+
| -1 (0xFFFFFFFF)      |  Terminator
+----------------------+
```

Each offset points to a word in the image that needs relocation by adding the relocation offset.

### 6.4 Self-Relocation Code Example

```arm
RelocCode:
    SUB    ip, lr, pc         ; Calculate header address
    ADD    ip, pc, ip
    SUB    ip, ip, #12
    LDR    r0, [ip, #4]       ; Reset second word to NOP
    STR    r0, [ip, #4]
    LDR    r9, [ip, #0x2C]    ; Get workspace requirement
    CMPS   r9, #0
    BEQ    RelocateOnly

    ; Self-move code here if workspace needed

RelocateOnly:
    LDR    r1, [ip, #0x28]    ; Linked base address
    SUBS   r1, ip, r1         ; Relocation offset
    MOVEQ  pc, lr             ; No relocation needed
    STR    ip, [ip, #0x28]   ; Store actual base
    ADR    r2, RelocList
01  LDR    r0, [r2], #4
    CMNS   r0, #1
    MOVEQ  pc, lr
    LDR    r3, [ip, r0]
    ADD    r3, r3, r1
    STR    r3, [ip, r0]
    B      %B01

RelocList:
    ; Offsets to relocate, -1 terminator
```

---

## 7. Zero-Initialization Code

### 7.1 Purpose

The zero-init code creates the zero-initialized data area (ZI section). This area:

- Contains data that must start as zero
- Is typically used for:
  - Uninitialized static variables
  - BSS segment
  - Stack space (in simple systems)

### 7.2 Algorithm

1. Load the four size values from header
2. Calculate ZI base address
3. Zero memory in 16-byte chunks
4. Return to caller when complete

### 7.3 Memory Layout After Zero-Init

```
Image Base
    +------------------+
    | Header           |  128 bytes
    +------------------+
    | RO (Code+Const)  |  ImageROSize - 128
    +------------------+
    | RW (Data)        |  ImageRWSize
    +------------------+
    | Debug            |  ImageDebugSize (optional)
    +------------------+
    | ZI (Zero-Init)   |  ImageZISize
    +------------------+
```

---

## 8. Debugging Support

### 8.1 Debug Data Types

AIF supports two levels of debugging information:

**Low-Level Debugging (Type 1):**
- Raw memory maps
- Symbol information
- Register state
- Memory examination

**Source-Level Debugging (Type 2):**
- Line number information
- Type information
- Variable information
- Source file mapping

### 8.2 Debug Type Encoding

| Bits | Field | Description |
|------|-------|-------------|
| 0 | Low-level present | Set if low-level debug data exists |
| 1 | Source-level present | Set if source-level debug data exists |

### 8.3 Debugger Interaction

Debuggers can intercept image execution by:

1. Copying debugging data before program use
2. Modifying the entry point in header word 3
3. Setting breakpoints in the image

---

## 9. C Header Definition

```c
#pragma once
#include <stdint.h>

#define AIF_NOOP         0xE1A00000    /* MOV R0, R0 */
#define AIF_BL           0xEB000000
#define OS_EXIT          0xEF000011    /* SWI OS_Exit */
#define AIF_IMAGEBASE    0x00008000    /* Default load address */

typedef struct AIFHeader {
    uint32_t blDecompress;     /* 0x00: Decompression code branch */
    uint32_t blSelfReloc;       /* 0x04: Self-relocation code branch */
    uint32_t blZeroInit;        /* 0x08: Zero-init code branch */
    uint32_t blEntry;           /* 0x0C: Entry point branch/offset */
    uint32_t swiExit;          /* 0x10: Exit instruction */
    int32_t  imageROSize;      /* 0x14: Read-only section size */
    int32_t  imageRWSize;      /* 0x18: Read-write section size */
    int32_t  debugSize;        /* 0x1C: Debugging data size */
    int32_t  ziSize;           /* 0x20: Zero-init section size */
    int32_t  debugType;        /* 0x24: Debug type (0-3) */
    uint32_t imageBase;        /* 0x28: Linked base address */
    int32_t  workspace;        /* 0x2C: Workspace requirement */
    uint32_t addressMode;      /* 0x30: 26/32-bit mode flags */
    uint32_t dataBase;         /* 0x34: Data base address */
    uint32_t reserved[2];      /* 0x38: Reserved */
    uint32_t debugInit;        /* 0x40: Debug init instruction */
    uint32_t zeroInitCode[15]; /* 0x44: Zero-init code */
} AIFHeader;

/* Address Mode Flags */
#define AIF_ADDRMODE_26BIT    26
#define AIF_ADDRMODE_32BIT   32
#define AIF_SEPARATE_DATABASE 0x100

/* Debug Types */
#define AIF_DBG_NONE         0
#define AIF_DBG_LOWLEVEL     1
#define AIF_DBG_SOURCELEVEL  2
#define AIF_DBG_BOTH         3
```

---

## 10. Implementation Considerations

### 10.1 Memory Protection Systems

On systems with memory protection:

1. Decompression code must change header to writable before modifying
2. Relocation code must change RO section to writable
3. Restore read-only protection after modification

### 10.2 Restartability

AIF images are restartable if:

1. The contained program is restartable
2. Following decompression, word 0 is reset to NOP
3. Following relocation, word 1 is reset to NOP

### 10.3 Endianness Handling

- File byte order matches target ARM byte order (little-endian)
- Loader byte-swaps if host endianness differs
- ChunkFileId (`0xC5C6CBC3` when swapped) indicates endianness

### 10.4 Alignment Requirements

| Data Type | Alignment |
|-----------|-----------|
| Word | 4 bytes |
| Halfword | 2 bytes |
| Byte | 1 byte |

---

## 11. Historical Notes

### 11.1 Version History

| Version | Date | Changes |
|---------|------|---------|
| 0.03 | 1987 | Initial AIF specification |
| 0.04 | 1987 | Added symbolic debugger support |
| 1.00 | 1989 | Improved relocatable image support |
| 3.00 | ~1993 | 32-bit support, enhanced relocations |

### 11.2 Name Evolution

- **Arthur Image Format**: Original name (Arthur = early RISC OS)
- **ARM Image Format**: Current name, reflects ARM Ltd. ownership

### 11.3 Usage Timeline

```
1987: Acorn Archimedes (Arthur OS)
      ↓
1990s: RISC OS, Acorn computers
      ↓
1990s: 3DO game console
      ↓
1990s: Embedded ARM systems
      ↓
Present: Mostly replaced by ELF, but still used in legacy systems
```

---

## 12. Comparison with Other Formats

| Feature | AIF | ELF | PE |
|---------|-----|-----|-----|
| Header Size | 128 bytes | 52 bytes | Varies |
| Debugging | Yes | Yes | Yes |
| Relocation | Internal | External | External |
| Dynamic Linking | Limited | Full | Full |
| Position Independence | Good | Good | Poor |
| Simplicity | High | Medium | Low |
| Extensibility | Low | High | High |

---

## 13. References

1. ARM Software Development Toolkit Reference Guide (DUI 0041C)
2. RISC OS Programmer's Reference Manuals
3. Acorn Technical Memorandum: RISC OS Application Image Format (PLG-AIF)
4. 3DO SDK Documentation (aif.h)
5. RISC OS Open Limited (ROOL) Documentation

---

## Appendix A: Byte Patterns

### A.1 Key Header Values

| Offset | Pattern | Meaning |
|--------|---------|---------|
| 0x00 | `E1 A0 00 00` | NOP (no compression) |
| 0x00 | `EB xx xx xx` | BL to decompressor |
| 0x04 | `E1 A0 00 00` | NOP (no relocation) |
| 0x0C | `EB xx xx xx` | BL entry (executable) |
| 0x0C | `00 xx xx xx` | Entry offset (non-executable) |
| 0x10 | `EF 00 00 11` | SWI OS_Exit |

### A.2 Chunk File Identifier

```
C3 CB C6 C5  = "chunk" in little-endian
C5 C6 CB C3  = Same when byte-swapped (big-endian detection)
```

---

## Appendix B: Complete Header Map

```
Offset  Content
------  -------------------------------------------------------
0x00    Decompression code branch (BL) or NOP
0x04    Self-relocation code branch (BL) or NOP
0x08    Zero-init code branch (BL) or NOP
0x0C    Entry point branch (BL) or entry offset
0x10    Exit instruction (SWI or branch)
0x14    Image Read-Only size (bytes)
0x18    Image Read-Write size (bytes)
0x1C    Image Debug size (bytes)
0x20    Image Zero-Init size (bytes)
0x24    Image Debug Type (0-3)
0x28    Image Base (linked address)
0x2C    WorkSpace (obsolete)
0x30    Address Mode (26/32 + flags)
0x34    Data Base (if separate)
0x38    Reserved [0]
0x3C    Reserved [1]
0x40    Debug Init instruction
0x44    Zero-Init Code [0]
...
0x7C    Zero-Init Code [14]
------  -------------------------------------------------------
Total:  128 bytes (32 words)
```

---

*Document Version: 1.0*
*Last Updated: Based on ARM SDT 2.50 (1998), RISC OS AIF (1989), and 3DO SDK documentation*
