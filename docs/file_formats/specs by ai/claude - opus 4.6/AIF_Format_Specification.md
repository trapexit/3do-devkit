# ARM Image Format (AIF) — Complete File Format Specification

## Overview

ARM Image Format (AIF) is a simple executable image format for ARM processors. It consists of a fixed 128-byte (32-word) header followed by the image's code, initialized static data, and optional debugging and relocation information. AIF was designed by Acorn Computers Limited for use on RISC OS systems and was subsequently adopted by ARM Ltd for the ARM Software Development Toolkit (SDT). It has also been used on other ARM-based platforms including the 3DO Interactive Multiplayer console.

### Document Sources

This specification is synthesized from:

- PLG-AIF Issue 1.00 (Acorn Computers Limited, 1989)
- ARM Software Development Toolkit Reference Guide (ARM DUI 0041C, 1997/1998)
- RISC OS Programmer's Reference Manuals, Appendix D
- 3DO SDK Technical Documentation (ARM Technical Specifications)
- 3DO Portfolio OS `aif.h` header file (Revision 1.21, 1994)

---

## 1. Terminology

| Term       | Definition |
|------------|------------|
| **Byte**   | 8 bits, unsigned unless otherwise stated. |
| **Half Word** | 16 bits (2 bytes), usually unsigned. Aligned on 2-byte boundaries. Least significant byte at lowest address (little-endian) or most significant byte at lowest address (big-endian), matching the target system. |
| **Word**   | 32 bits (4 bytes), usually unsigned. Aligned on 4-byte boundaries. Byte ordering matches the target system. |
| **String** | A sequence of bytes terminated by a NUL (0x00) byte. The NUL is part of the string but is not counted in the string's length. |
| **NOP**    | No Operation instruction, encoded as `MOV r0, r0` (`0xE1A00000`). In older toolchains, NOP was sometimes encoded as `BLNV 0` (`0xFB000000` — a branch-with-link using the NV (never) condition, causing it to never execute). |
| **BL**     | Branch with Link instruction. The most significant byte of a BL instruction word is `0xEB`. |

---

## 2. Endianness (Byte Sex)

An AIF image can be produced in either **little-endian** or **big-endian** format. The endianness of the AIF file is always the same as the endianness of the target ARM system.

- **Little-endian**: The least significant byte of a word has the lowest address (DEC/Intel byte order).
- **Big-endian**: The most significant byte of a word has the lowest address (IBM/Motorola byte order).

The endianness can be inferred from the content of the header — specifically, by examining the instruction encoding at known offsets.

---

## 3. AIF Variants

Three variants of AIF exist:

### 3.1 Executable AIF

- The header is part of the image itself.
- The image can be loaded at its load address and entered (executed) at its first word (offset 0x00 of the header).
- Code within the header ensures the image is properly prepared for execution (decompression, self-relocation, zero-initialization) before branching to the entry point.
- The fourth word (offset 0x0C) of an executable AIF header is a `BL <entrypoint>` instruction. The most significant byte of this word (in the target byte order) is `0xEB`.
- The base address of an executable AIF image is the address at which the header is loaded. The actual code begins at `base + 0x80` (after the 128-byte header).
- The Image ReadOnly Size field (offset 0x14) **includes** the size of the 128-byte header.

**Distinguishing feature**: `(header[0x0C] >> 24) == 0xEB`

### 3.2 Non-Executable AIF

- The header is not part of the image; it merely describes the image.
- The image must be processed by an image loader, which interprets the header, performs any required decompression, relocation, and zero-initialization, then discards the header.
- The fourth word (offset 0x0C) is the byte offset of the entry point from the image's base address. The most significant nibble of this word (in the target byte order) is `0x0`.
- The base address of a non-executable AIF image is the address at which the code (not the header) should be loaded.
- The Image ReadOnly Size field (offset 0x14) **excludes** the header size.

**Distinguishing feature**: `(header[0x0C] >> 28) == 0x0`

### 3.3 Extended AIF

- A special type of non-executable AIF.
- Contains a scatter-loaded image.
- The AIF header points to a chain of load region descriptors within the file.
- An image loader should place each region at the location in memory specified by the load region descriptor.
- Identified by a non-zero value in the first reserved word at offset 0x38, which contains the byte offset within the file of the header for the first non-root load region.

---

## 4. Properties of AIF

1. An AIF image is loaded into memory at its load address and entered at its first word.

2. An AIF image may be **compressed** and can be **self-decompressing**. Compression is performed by a separate utility which adds self-decompression code and data tables. In a compressed image, the header itself is **not** compressed.

3. If created with appropriate linker options, an AIF image may **relocate itself** at load time. Two kinds of self-relocation are supported:
   - **Relocate to load address** (one-time position independence): The image can be loaded at any address and will execute where loaded.
   - **Self-move up memory**: The image moves itself to the highest address that leaves the required workspace free, then relocates to that address. This requires an OS/monitor call that returns the address of the top of available memory.

4. An AIF image is **re-startable** if and only if the program it contains is re-startable. An AIF image is **not** reentrant.

5. AIF images support being debugged by the ARM Symbolic Debugger (armsd/ASD). Low-level and source-level debugging support are orthogonal — both, either, or neither kind may be present.

6. On entry to an AIF program, general registers contain nothing of value to the program. The program communicates with its operating environment using SWI instructions or by calling functions at known, fixed addresses.

---

## 5. Image Layout

### 5.1 Compressed AIF Image

```
+----------------------------+
| Header (128 bytes)         |   NOT compressed
+----------------------------+
| Compressed image           |
+----------------------------+
| Decompression data         |   Position-independent
+----------------------------+
| Decompression code         |   Position-independent
+----------------------------+
```

### 5.2 Uncompressed AIF Image

```
+----------------------------+
| Header (128 bytes)         |
+----------------------------+
| Read-Only area             |   Code
+----------------------------+
| Read-Write area            |   Initialized data
+----------------------------+
| Debugging data             |   (optional)
+----------------------------+
| Self-relocation code       |   Position-independent
+----------------------------+
| Relocation list            |   List of word offsets, terminated by -1
+----------------------------+
```

- Debugging data is present only if the image was linked with the `-d` option and/or compiled with the `-g` option.
- The relocation list is a list of byte offsets from the beginning of the AIF header, identifying words to be relocated. The list is terminated by a word containing `-1` (0xFFFFFFFF). Relocation of non-word values is not supported.

### 5.3 Post-Relocation / Final Layout

After self-relocation (or if the image is not self-relocating):

```
+----------------------------+
| Header (128 bytes)         |
+----------------------------+
| Read-Only area             |
+----------------------------+
| Read-Write area            |
+----------------------------+
| Debugging data             |   (optional)
+----------------------------+
```

At this stage, a debugger should copy debugging data to a safe location, as it will be overwritten by zero-initialized data, heap, or stack. A debugger seizes control by copying then modifying the third word of the header (offset 0x08).

---

## 6. AIF Header Layout

The AIF header is exactly **128 bytes (32 words)** long.

### 6.1 Header Field Table

| Offset | Size   | Field Name                  | Description |
|--------|--------|-----------------------------|-------------|
| `0x00` | 1 word | BL DecompressCode / NOP     | Branch-with-link to decompression code. NOP (`MOV r0, r0` = `0xE1A00000`) if the image is not compressed. In the 1989 spec, `BLNV 0` was used for NOP. |
| `0x04` | 1 word | BL SelfRelocCode / NOP      | Branch-with-link to self-relocation code. NOP if the image is not self-relocating. |
| `0x08` | 1 word | BL DBGInit/ZeroInit / NOP   | Branch-with-link to the zero-initialization code (located at offset 0x44 within the header). NOP if the image has no zero-initialized data. In the 1989 spec, this branched directly to `ZeroInitCode`. |
| `0x0C` | 1 word | BL ImageEntryPoint / EntryPoint Offset | **Executable AIF**: `BL <entrypoint>` — branch-with-link to the image entry point. BL is used to make the header addressable via r14 in a position-independent manner. The application must not return from this call. **Non-executable AIF**: The byte offset of the entry point from the image's base address. The most significant nibble is `0x0`. |
| `0x10` | 1 word | Program Exit Instruction    | A last-resort instruction in case the application returns. Usually `SWI 0x11` (`0xEF000011`, OS_Exit). On systems without such a SWI, a branch-to-self (`B .`) is recommended. Applications should exit directly and never return to the AIF header. |
| `0x14` | 1 word | Image ReadOnly Size         | Size of the read-only (code) area in bytes, including any padding. **Includes** the 128-byte header size if the AIF is executable; **excludes** the header size if non-executable. |
| `0x18` | 1 word | Image ReadWrite Size        | Exact size of the read-write (initialized data) area in bytes. Must be a multiple of 4. |
| `0x1C` | 1 word | Image Debug Size            | Exact size of the debugging data area in bytes. Must be a multiple of 4. |
| `0x20` | 1 word | Image Zero-Init Size        | Exact size of the zero-initialized data area in bytes. Must be a multiple of 4. This area is not stored in the image file; it is created at run-time by the zero-initialization code. |
| `0x24` | 1 word | Image Debug Type            | Indicates the type of debugging data present. See section 6.2. |
| `0x28` | 1 word | Image Base                  | The address at which the image (code) was linked. Set by the linker. For RISC OS applications, this is typically `0x00008000`. |
| `0x2C` | 1 word | Work Space                  | Minimum work space in bytes to be reserved by a self-moving relocatable image. A value of 0 means no self-move is required (only relocate in place). See also section 6.4 for 3DO extensions. |
| `0x30` | 1 word | Address Mode + Flags        | See section 6.3. |
| `0x34` | 1 word | Data Base                   | The address at which the image data was linked. Only meaningful if the Address Mode word has bit 8 set (separate data base). |
| `0x38` | 1 word | Reserved Word 1             | Must be zero. In **Extended AIF**, this word is non-zero and contains the byte offset within the file of the header for the first non-root load region. |
| `0x3C` | 1 word | Reserved Word 2             | Must be zero. |
| `0x40` | 1 word | Debug Init Instruction      | NOP if unused. If used, expected to be a SWI instruction alerting a resident debugger that a debuggable image is commencing execution. Can be customized via an `AIF_HDR` area template. |
| `0x44` | 15 words | Zero-Init Code            | The built-in zero-initialization subroutine. Together with the Debug Init Instruction at 0x40, these 16 words complete the 32-word header. |

### 6.2 Image Debug Type Values

| Value | Meaning |
|-------|---------|
| 0     | No debugging data are present. |
| 1     | Low-level debugging data are present. |
| 2     | Source-level (ASD) debugging data are present. |
| 3     | Both low-level and source-level debugging data are present. |

All other values are reserved.

### 6.3 Address Mode Word (Offset 0x30)

The least significant byte (in the target byte order) contains one of:

| Value | Meaning |
|-------|---------|
| 0     | Old-style 26-bit AIF header (no explicit address mode). |
| 26    | Image was linked for a 26-bit ARM mode. May not execute correctly in a 32-bit mode. |
| 32    | Image was linked for a 32-bit ARM mode. May not execute correctly in a 26-bit mode. |

**Bit 8** (`0x100`): If set, the image was linked with separate code and data bases. The word at offset 0x34 contains the base address of the image's data.

**Bits 9-31**: Reserved, should be zero.

### 6.4 Work Space Field — 3DO Extensions (Offset 0x2C)

On the 3DO platform, the Work Space field has additional bit-field encoding:

| Bits     | Mask         | Meaning |
|----------|--------------|---------|
| 31-28    | `0xF0000000` | Flags (see below) |
| 23-0     | `0x00FFFFFF` | Stack size (max 16 MiB) |

**Flag bits (31-28)**:

| Value        | Mask         | Meaning |
|--------------|--------------|---------|
| Bit 30       | `0x40000000` | `AIF_3DOHEADER` — A 128-byte 3DO Binary Header (`_3DOBinHeader`) immediately follows the AIF header. |

When `AIF_3DOHEADER` is set, the system takes all task parameters from the `_3DOBinHeader` structure rather than from the encoded stack/workspace values.

### 6.5 Extended AIF Region Header Format

For Extended AIF images (non-zero value at offset 0x38), each load region is described by a 44-byte region header:

| Word   | Size    | Description |
|--------|---------|-------------|
| 0      | 1 word  | File offset of the header of the next region (0 if this is the last region). |
| 1      | 1 word  | Load address for this region. |
| 2      | 1 word  | Size in bytes (must be a multiple of 4). |
| 3-10   | 8 words | Region name (32 characters, padded with zero bytes). |

The initializing data for the region immediately follows its header.

---

## 7. AIF Header as a C Structure

```c
#define AIF_NOOP       0xE1A00000    /* MOV r0, r0 */
#define AIF_BLAL       0xEB000000    /* BL (always) base opcode */
#define OS_EXIT        0xEF000011    /* SWI OS_Exit */
#define OS_GETENV      0xEF000010    /* SWI OS_GetEnv */
#define AIF_IMAGEBASE  0x00008000    /* Default load address (RISC OS) */
#define AIF_BLZINIT    0xEB00000C    /* BL to ZeroInit at +0x40 */
#define DEBUG_TASK     0xEF041D41    /* SWI DDE_Debug (RISC OS) */

typedef struct AIFHeader {
    uint32_t aif_blDecompress;     /* 0x00: NOP if image not compressed */
    uint32_t aif_blSelfReloc;      /* 0x04: NOP if image not self-relocating */
    uint32_t aif_blZeroInit;       /* 0x08: NOP if image has no zero-init data */
    uint32_t aif_blEntry;          /* 0x0C: BL entry (executable) or offset (non-exec) */
    uint32_t aif_SWIexit;          /* 0x10: Program exit instruction */
    uint32_t aif_ImageROsize;      /* 0x14: Read-only size (includes header if exec) */
    uint32_t aif_ImageRWsize;      /* 0x18: Read-write size (exact, multiple of 4) */
    uint32_t aif_DebugSize;        /* 0x1C: Debug data size (exact, multiple of 4) */
    uint32_t aif_ZeroInitSize;     /* 0x20: Zero-init size (exact, multiple of 4) */
    uint32_t aif_ImageDebugType;   /* 0x24: Debug type (0, 1, 2, or 3) */
    uint32_t aif_ImageBase;        /* 0x28: Address image was linked at */
    uint32_t aif_WorkSpace;        /* 0x2C: Min workspace / 3DO flags+stack */
    uint32_t aif_AddressMode;      /* 0x30: 0, 26, or 32 + flag bits */
    uint32_t aif_DataBaseAddr;     /* 0x34: Data base address (if separate) */
    uint32_t aif_Reserved[2];      /* 0x38: Reserved (Extended AIF uses [0]) */
    uint32_t aif_DebugInit;        /* 0x40: Debug init instruction or NOP */
    uint32_t aif_ZeroInitCode[15]; /* 0x44: Zero-initialization subroutine */
} AIFHeader;                       /* Total: 32 words = 128 bytes */
```

---

## 8. Relationship Between Header Sizes and Linker Symbols

The following relationships hold between the header size fields and the linker pre-defined symbols:

```
Image$$RO$$Base  = AIFHeader.ImageBase

Image$$RW$$Base  = AIFHeader.ImageBase + AIFHeader.ROSize

Image$$ZI$$Base  = AIFHeader.ImageBase + AIFHeader.ROSize + AIFHeader.RWSize

Image$$RW$$Limit = AIFHeader.ImageBase + AIFHeader.ROSize + AIFHeader.RWSize
                   + AIFHeader.ZeroInitSize
```

---

## 9. Zero-Initialization Code

The zero-initialization code occupies the last 16 words of the header (offsets 0x40-0x7C). It is entered via the `BL` instruction at offset 0x08. On entry, `r14` (LR) points to `AIFHeader + 0x0C`.

The code computes the base address of the zero-initialized area (at the end of the read-write area), then fills it with zeroes in 16-byte blocks.

### 9.1 Algorithm

1. Calculate the base of the AIF header from the current PC and LR values (position-independent computation).
2. Load the ReadOnly size, ReadWrite size, Debug size, and ZeroInit size from the header.
3. If ZeroInit size is zero, return immediately.
4. Compute the start address of the zero-init area: `base + ROSize + RWSize`.
5. Zero 16 bytes at a time (4 registers x 4 bytes) using `STMIA`, decrementing the remaining count until complete.
6. Return via `MOV PC, LR`.

### 9.2 Assembly Listing

```arm
; Offset 0x40:
ZeroInit
        NOP                         ; or <Debug Init Instruction>
; Offset 0x44:
        SUB     ip, lr, pc          ; base+12+[PSR]-(ZeroInit+12+[PSR])
                                    ; = base - ZeroInit
        ADD     ip, pc, ip          ; base - ZeroInit + ZeroInit + 16
                                    ; = base + 16
        LDMIB   ip, {r0,r1,r2,r4}  ; Load ROSize, RWSize, DebugSize, ZISize
        SUB     ip, ip, #16         ; ip = image base
        ADD     ip, ip, r0          ; + RO size
        ADD     ip, ip, r1          ; + RW size = base of 0-init area
        MOV     r0, #0
        MOV     r1, #0
        MOV     r2, #0
        MOV     r3, #0
        CMPS    r4, #0
00      MOVLE   pc, lr              ; nothing left to do — return
        STMIA   ip!, {r0,r1,r2,r3} ; zero 16 bytes, post-increment
        SUBS    r4, r4, #16
        B       %B00
; Total: 16 words (including the Debug Init instruction at 0x40)
```

**Note**: The SUB/ADD sequence for computing the header base address works correctly in both 26-bit and 32-bit ARM modes because the PSR bits cancel out in the subtraction.

---

## 10. Self-Relocation and Self-Move Code

This code is appended to the end of an AIF image by the linker, immediately before the relocation list. It is entered via the `BL` instruction at offset 0x04 of the header. On entry, `r14` -> `AIFHeader + 0x08`. In 26-bit ARM modes, `r14` also contains PSR flags.

### 10.1 Phase 1: Calculate Header Address

```arm
RelocCode
        NOP                         ; used by ensure_byte_order() and below
        SUB     ip, lr, pc          ; base+8+[PSR]-(RelocCode+12+[PSR])
                                    ; = base - 4 - RelocCode
        ADD     ip, pc, ip          ; base - 4 - RelocCode + RelocCode + 16
                                    ; = base + 12
        SUB     ip, ip, #12         ; ip = header address
        LDR     r0, RelocCode       ; Load NOP instruction
        STR     r0, [ip, #4]        ; Overwrite BL SelfReloc with NOP
                                    ; (prevents re-invocation on re-entry)
        LDR     r9, [ip, #0x2C]     ; min free space requirement (WorkSpace)
        CMPS    r9, #0              ; 0 => no move, just relocate
        BEQ     RelocateOnly
```

### 10.2 Phase 2: Self-Move Up Memory (Optional)

If the WorkSpace field is non-zero, the image moves itself to high memory:

```arm
        LDR     r0, [ip, #0x20]     ; image zero-init size
        ADD     r9, r9, r0          ; space to leave = min free + zero init
        SWI     #0x10               ; OS call: return top of memory in r1
                                    ; (system-specific — replace as needed)
```

Calculate whether a move is possible:

```arm
        ADR     r2, End             ; -> end of relocation data
01      LDR     r0, [r2], #4        ; load relocation offset, advance pointer
        CMNS    r0, #1              ; is it the -1 terminator?
        BNE     %B01                ; no — keep scanning
        SUB     r3, r1, r9          ; MemLimit - freeSpace
        SUBS    r0, r3, r2          ; amount to move by
        BLE     RelocateOnly        ; not enough space to move
        BIC     r0, r0, #15         ; round down to multiple of 16
        ADD     r3, r2, r0          ; destination End = End + shift
        ADR     r8, %F02            ; intermediate limit for copy-up
```

### 10.3 Phase 3: Copy Image (if moving)

Copy four words at a time in descending address order, jumping to the copied copy-loop as soon as it has been relocated:

```arm
02      LDMDB   r2!, {r4-r7}
        STMDB   r3!, {r4-r7}
        CMPS    r2, r8              ; copied the copy loop?
        BGT     %B02                ; not yet
        ADD     r4, pc, r0
        MOV     pc, r4              ; jump to copied copy code
03      LDMDB   r2!, {r4-r7}
        STMDB   r3!, {r4-r7}
        CMPS    r2, ip              ; copied everything?
        BGT     %B03                ; not yet
        ADD     ip, ip, r0          ; new load address of code
        ADD     lr, lr, r0          ; relocated return address
```

### 10.4 Phase 4: Relocation Processing

Whether or not the image moved, control arrives here. Each word in the relocation list is a byte offset from the header base; the word at that offset is adjusted by the relocation delta:

```arm
RelocateOnly
        LDR     r1, [ip, #0x28]     ; code base as set by linker
        SUBS    r1, ip, r1          ; relocation offset = actual - linked
        MOVEQ   pc, lr              ; if zero, nothing to do
        STR     ip, [ip, #0x28]     ; update image base to actual address
        ADR     r2, End             ; start of relocation list
04      LDR     r0, [r2], #4        ; offset of word to relocate
        CMNS    r0, #1              ; -1 terminator?
        MOVEQ   pc, lr              ; yes => return
        LDR     r3, [ip, r0]        ; load word to relocate
        ADD     r3, r3, r1          ; apply relocation delta
        STR     r3, [ip, r0]        ; store relocated word
        B       %B04                ; next relocation
End                                 ; Relocation list starts here,
                                    ; terminated by -1 (0xFFFFFFFF)
```

### 10.5 Customization

The self-relocation and self-moving code can be customized by providing a replacement in an area called `AIF_RELOC` in the first object file in `armlink`'s input list. The AIF header template can be customized by providing an area called `AIF_HDR` in the first object file.

---

## 11. Re-Startability

An AIF image can be re-entered at its first instruction if certain conditions are met:

1. After decompression, the first word of the header (offset 0x00) must be reset to NOP, so decompression is not attempted again.
2. After self-relocation, the second word of the header (offset 0x04) must be reset to NOP.
3. The self-relocation code performs step 2 automatically.
4. On systems with memory protection, both decompression and relocation code must bracket their writes to the read-only code segment with system calls to change the access status (writable, then back to read-only).

---

## 12. Debugging Data

### 12.1 Presence

Debugging data is present only when:
- The image is linked with the linker's `-d` option, AND/OR
- Components are compiled with the compiler's `-g` option.

### 12.2 Address References

- References from debugging tables to code and data are in the form of **relocatable addresses**. After loading at the load address, these are effectively absolute.
- References between debugger table entries are in the form of **offsets from the beginning of the debugging data area**. The debugging data area is therefore position-independent after image relocation.
- A debugger should copy the debugging data to a safe location before the image begins execution, as the zero-initialized data, heap, and stack will overwrite the debugging area.

### 12.3 Debugger Interception

A debugger can seize control at the appropriate moment by:
1. Copying the third word of the AIF header (offset 0x08, the `BL ZeroInit`).
2. Replacing it with a `BL` to the debugger's own initialization code.
3. The debugger's code saves the debugging data, restores the original third word, and then executes it.

---

## 13. 3DO Platform Extensions

The 3DO Interactive Multiplayer console uses AIF with platform-specific extensions.

### 13.1 3DO Binary Header (`_3DOBinHeader`)

When the `AIF_3DOHEADER` flag (bit 30, `0x40000000`) is set in the Work Space field (offset 0x2C), a 128-byte 3DO-specific binary header immediately follows the standard AIF header (at offset 0x80 from the file start, overlapping with the beginning of the code area).

```c
typedef struct _3DOBinHeader {
    ItemNode _3DO_Item;         /* Standard 3DO kernel item node */
    uint8    _3DO_Flags;        /* 3DO-specific flags (see below) */
    uint8    _3DO_OS_Version;   /* Compiled for this OS release */
    uint8    _3DO_OS_Revision;  /* OS revision */
    uint8    _3DO_Reserved;     /* Must be zero */
    uint32   _3DO_Stack;        /* Stack size requirements (bytes) */
    uint32   _3DO_FreeSpace;    /* Preallocate bytes for FreeList */
    uint32   _3DO_Signature;    /* If privileged, offset to signature */
    uint32   _3DO_SignatureLen; /* Length of signature in bytes */
    uint32   _3DO_MaxUSecs;     /* Max microseconds before task switch */
    uint32   _3DO_Reserved0;    /* Must be zero */
    char     _3DO_Name[32];     /* Optional task name on startup */
    uint32   _3DO_Time;         /* Seconds since 1/1/93 00:00:00 GMT */
    uint32   _3DO_Reserved1[7]; /* Must be zero */
} _3DOBinHeader;                /* Total: 128 bytes */
```

### 13.2 3DO Flags (`_3DO_Flags`)

| Bit | Value | Name             | Meaning |
|-----|-------|------------------|---------|
| 5   | 32    | `_3DO_DATADISCOK` | Application willing to accept data discs. |
| 4   | 16    | `_3DO_NORESETOK`  | Application willing to keep running on empty drawer. |
| 3   | 8     | `_3DO_LUNCH`      | OS has been launched, chips may be initialized. |
| 2   | 4     | `_3DO_USERAPP`    | User application requiring authentication. |
| 1   | 2     | `_3DO_PRIVILEGE`  | Privileged application. |
| 0   | 1     | `_3DO_PERMSTACK`  | Permanent stack allocation. |

### 13.3 3DO Debugger Values

The Debug Init Instruction (offset 0x40) and related constants have 3DO-specific interpretations:

| Constant                  | Value        | Meaning |
|---------------------------|--------------|---------|
| `PORTFOLIO_DEBUG_CLEAR`   | `0xE1A02002` | NOP (MOV r2, r2) — debug cleared. |
| `PORTFOLIO_DEBUG_TASK`    | `0xEF00010A` | SWI instruction — alert Portfolio debugger. |
| `PORTFOLIO_DEBUG_UNSET`   | `0xE1A00000` | NOP (MOV r0, r0) — debug not set. |

### 13.4 3DO Reserved Field Usage

The first reserved word at offset 0x38 (`aif_Reserved[0]`) is aliased as `aif_MD4DataSize` in the 3DO SDK, suggesting it may store the size of MD4 hash/authentication data.

---

## 14. Execution Flow Summary

For an executable AIF image, the execution flow is:

1. **Load**: Image loaded at its load address.
2. **Enter**: Execution begins at the first word (offset 0x00).
3. **Decompress** (offset 0x00): If compressed, `BL DecompressCode` decompresses the image. After decompression, this word is reset to NOP.
4. **Self-Relocate** (offset 0x04): If relocatable, `BL SelfRelocCode` performs self-move and/or relocation. After relocation, this word is reset to NOP.
5. **Zero-Initialize** (offset 0x08): If there is zero-initialized data, `BL ZeroInit` zeroes the BSS section.
6. **Entry** (offset 0x0C): `BL ImageEntryPoint` branches to the program's entry point. LR is set to point back into the header, making the header addressable.
7. **Exit** (offset 0x10): If the entry point function returns (which it should not), the Program Exit Instruction is executed as a last resort.

---

## 15. Complete Binary Layout — Byte-Level Header Map

```
Offset  Size  Field
------  ----  -----
0x00    4     BL DecompressCode / NOP (0xE1A00000)
0x04    4     BL SelfRelocCode / NOP
0x08    4     BL DBGInit+ZeroInit / NOP
0x0C    4     BL ImageEntryPoint / EntryPoint offset
0x10    4     Program Exit Instruction (default: 0xEF000011 = SWI OS_Exit)
0x14    4     Image ReadOnly Size
0x18    4     Image ReadWrite Size
0x1C    4     Image Debug Size
0x20    4     Image Zero-Init Size
0x24    4     Image Debug Type (0-3)
0x28    4     Image Base Address
0x2C    4     Work Space / 3DO flags+stack
0x30    4     Address Mode (LSB: 0/26/32; bit 8: separate data base)
0x34    4     Data Base Address
0x38    4     Reserved[0] / Extended AIF offset / 3DO MD4 size
0x3C    4     Reserved[1]
0x40    4     Debug Init Instruction / NOP
0x44    60    Zero-Init Code (15 words)
------  ----
0x80          End of Header (128 bytes total)
              Code begins here (for executable AIF)
```

---

## 16. Identifying an AIF File

An AIF file can be identified by examining the header:

1. **Check offset 0x0C**: If `(word >> 24) == 0xEB`, it is an executable AIF. If `(word >> 28) == 0x0`, it is a non-executable AIF.
2. **Check offset 0x10**: Should contain a plausible program exit instruction (e.g., `SWI 0x11` = `0xEF000011`).
3. **Check offset 0x00**: Should be either NOP (`0xE1A00000`) or a `BL` instruction (`0xEB......`).
4. **Check sizes at offsets 0x14-0x20**: Should be non-negative, multiples of 4, and consistent with the file size.
5. **Extended AIF**: If offset 0x38 is non-zero, the image contains scatter-loaded regions.
6. **3DO**: If `(word_at_0x2C & 0x40000000) != 0`, a 3DO binary header follows at offset 0x80.
