# ARM Image Format (AIF) Specification

## 1. Overview

ARM Image Format (AIF) is a simple executable image format for ARM processors. An AIF file consists of a 128-byte header followed by the image's code, followed by the image's initialised static data. AIF was introduced by Acorn Computers Ltd for use on ARM-based systems (Archimedes, RiscPC, RISC OS) and was subsequently adopted for other platforms including the 3DO console.

AIF supports:

- Self-decompression of compressed images
- Self-relocation at load time (one-time position independence or self-move-to-high-memory)
- Zero-initialisation of BSS-style data areas
- Optional debugging data (low-level and source-level)
- Both 26-bit and 32-bit ARM processor modes

---

## 2. AIF Variants

There are three variants of AIF:

### 2.1 Executable AIF

Executable AIF can be loaded at its load address and entered at the same point (at the first word of the AIF header). The header is part of the image itself. Code embedded in the header ensures the image is properly prepared for execution (decompressed, relocated, zero-initialised) before being entered at its entry address.

**Distinguishing feature:** The fourth word (offset `0x0C`) of an executable AIF header is a `BL entrypoint` instruction. The most significant byte of this word (in the target byte order) is `0xEB`.

**Base address:** The address at which the AIF header should be loaded. The image code begins at `base + 0x80` (128 bytes past the header).

On RISC OS, the default base address for executable AIF images is `0x8000`.

### 2.2 Non-Executable AIF

Non-executable AIF must be processed by an image loader that loads the image at its load address and prepares it for execution as detailed in the AIF header. After preparation, the header is discarded. The header is not part of the image; it only describes the image.

**Distinguishing feature:** The fourth word (offset `0x0C`) is the offset of the entry point from the base address. The most significant nibble of this word (in the target byte order) is `0x0`.

**Base address:** The address at which the image code should be loaded (the header is not loaded with the image).

### 2.3 Extended AIF

Extended AIF is a special type of non-executable AIF that contains a scatter-loaded image. It has an AIF header that points to a chain of load region descriptors within the file. The image loader should place each region at the location in memory specified by the load region descriptor.

**Distinguishing feature:** The word at offset `0x38` in the header is non-zero. It contains the byte offset within the file of the header for the first non-root load region.

---

## 3. Assumptions and Entry Conditions

On entry to an executable AIF program, the general registers contain nothing of value to the program. The program communicates with its operating environment using SWI instructions or by calling functions at known, fixed addresses.

The load address may be:

1. An implicit property of the file type (e.g. RISC OS Absolute file types load at `0x8000`).
2. Read by the program loader from offset `0x28` in the file (the Image Base field).
3. Given by some other means (e.g. instructing an OS or debugger to load at a specified address).

---

## 4. AIF Image Layout

### 4.1 Compressed AIF Image

```
+---------------------------+
| Header (128 bytes)        |   NOT compressed
+---------------------------+
| Compressed image          |
+---------------------------+
| Decompression data        |   Position-independent
+---------------------------+
| Decompression code        |   Position-independent
+---------------------------+
```

The header is never compressed, even in a compressed AIF image. An AIF image is compressed by a separate utility which adds self-decompression code and data tables.

### 4.2 Uncompressed AIF Image

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only area            |   Code and read-only data
+---------------------------+
| Read-Write area           |   Initialised writable data
+---------------------------+
| Debugging data            |   Optional
+---------------------------+
| Self-relocation code      |   Position-independent
+---------------------------+
| Relocation list           |   Word offsets, terminated by -1
+---------------------------+
```

- **Debugging data** is absent unless the image was linked with the `-d` option and (for source-level debugging) compiled with the `-g` option.
- **The relocation list** is a list of byte offsets from the beginning of the AIF header, identifying words to be relocated, followed by a terminating word containing `-1` (`0xFFFFFFFF`). Only word-sized relocations are supported.

### 4.3 Post-Relocation / Runtime Layout

After self-relocation (or if the image is not self-relocating), the image has the layout:

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only area            |
+---------------------------+
| Read-Write area           |
+---------------------------+
| Debugging data            |   Optional (overwritten by zero-init/heap/stack)
+---------------------------+
```

At this stage, a debugger should copy any debugging data to a safe location before zero-initialisation overwrites it. A debugger can seize control by copying and then modifying the third word of the AIF header (the `BL ZeroInit` instruction).

---

## 5. AIF Header Layout

The AIF header is exactly 128 bytes (32 words) long.

### 5.1 Header Field Table

| Offset | Size    | Field                         | Description |
|--------|---------|-------------------------------|-------------|
| `0x00` | 1 word  | BL DecompressCode / NOP       | Branch-with-link to decompression code. `NOP` (`MOV R0, R0` = `0xE1A00000`) if the image is not compressed. In early AIF (1989), this was encoded as `BLNV 0` (`0xFB000000`) instead of `MOV R0, R0` for NOP. |
| `0x04` | 1 word  | BL SelfRelocCode / NOP        | Branch-with-link to self-relocation code. `NOP` if the image is not self-relocating. |
| `0x08` | 1 word  | BL ZeroInit / NOP             | Branch-with-link to zero-initialisation code. `NOP` if the image has no zero-initialised data. Also serves as the entry to DBGInit on some versions. |
| `0x0C` | 1 word  | BL ImageEntryPoint / Offset   | **Executable AIF:** `BL` to the image entry point. The BL makes the header addressable via R14 in a position-independent manner. The application shall not return through this BL. **Non-executable AIF:** The byte offset of the entry point from the image base (MSN = `0x0`). |
| `0x10` | 1 word  | Program Exit Instruction      | A last-resort instruction executed if the program returns to the header. Typically `SWI 0x11` (OS_Exit on RISC OS). On systems lacking such a SWI, a branch-to-self (`B .`) is recommended. Customisable via the `AIF_HDR` area. |
| `0x14` | 1 word  | Image ReadOnly Size           | Size of the read-only area in bytes. **Includes** the 128-byte header size if the AIF is executable. **Excludes** the header size if non-executable. |
| `0x18` | 1 word  | Image ReadWrite Size          | Exact size of the read-write area in bytes. Must be a multiple of 4. |
| `0x1C` | 1 word  | Image Debug Size              | Exact size of the debugging data in bytes. Must be a multiple of 4. In SDT 2.50: bits 0-3 hold the debug type; bits 4-31 hold the low-level debug size. |
| `0x20` | 1 word  | Image Zero-Init Size          | Exact size of the zero-initialised area in bytes. Must be a multiple of 4. |
| `0x24` | 1 word  | Image Debug Type              | See Section 5.2. |
| `0x28` | 1 word  | Image Base                    | The address at which the image (code) was linked. Updated to the actual load address after self-relocation. |
| `0x2C` | 1 word  | Work Space                    | Minimum workspace in bytes to be reserved by a self-moving relocatable image. On the 3DO, the upper nibble encodes flags (see Section 5.4). Marked obsolete in SDT 2.50. |
| `0x30` | 1 word  | Address Mode                  | See Section 5.3. |
| `0x34` | 1 word  | Data Base                     | Address where the image data was linked. Meaningful only if bit 8 of Address Mode is set. |
| `0x38` | 1 word  | Reserved / Extended AIF Ptr   | In standard AIF: reserved, must be 0. In Extended AIF: byte offset within the file of the header for the first non-root load region (non-zero). |
| `0x3C` | 1 word  | Reserved                      | Must be 0. |
| `0x40` | 1 word  | Debug Init Instruction / NOP  | Optional SWI instruction to alert a resident debugger that a debuggable image is starting. `NOP` if unused. Customisable via the `AIF_HDR` area. |
| `0x44` | 15 words| Zero-Init Code                | The built-in zero-initialisation routine. Together with the Debug Init instruction at `0x40`, these 16 words (plus the 16 words before them) total 32 words = 128 bytes for the complete header. |

### 5.2 Image Debug Type Values

| Value | Meaning                                    |
|-------|--------------------------------------------|
| 0     | No debugging data present                  |
| 1     | Low-level debugging data present           |
| 2     | Source-level (ASD) debugging data present   |
| 3     | Both low-level and source-level data present|

All other values are reserved.

### 5.3 Address Mode Word (Offset 0x30)

The Address Mode word is either 0 or contains in its least significant byte (using target byte order):

| Value | Meaning |
|-------|---------|
| 0     | Old-style 26-bit AIF header (pre-APCS-3) |
| 26    | Image linked for 26-bit ARM mode; may not execute correctly in 32-bit mode |
| 32    | Image linked for 32-bit ARM mode; may not execute correctly in 26-bit mode |

**Bit 8** (`0x100`): If set, the image was linked with separate code and data bases. In this case, the word at offset `0x34` (Data Base) contains the base address of the image's data.

### 5.4 Work Space Flags (3DO-Specific)

On the 3DO, the Work Space word at offset `0x2C` has additional flag encodings:

| Bits      | Mask           | Meaning |
|-----------|----------------|---------|
| 0-23      | `0x00FFFFFF`   | Stack size (maximum 16 MB) |
| 28-31     | `0xF0000000`   | Flags |
| Bit 30    | `0x40000000`   | `AIF_3DOHEADER`: A 128-byte 3DO binary header follows the AIF header |

If `AIF_3DOHEADER` is set, the system reads task parameters from a `_3DOBinHeader` structure immediately following the AIF header, rather than from the AIF header's workspace/stack encoding.

### 5.5 Extended AIF Region Header Format

For Extended AIF (scatter-loaded images), each non-root load region is described by a 44-byte header:

| Word  | Size     | Description |
|-------|----------|-------------|
| 0     | 1 word   | File offset of header of next region (0 if this is the last) |
| 1     | 1 word   | Load address for this region |
| 2     | 1 word   | Size in bytes (must be a multiple of 4) |
| 3-10  | 8 words  | Region name (32 characters, padded with zero bytes) |

The initialising data for each region immediately follows its header.

---

## 6. NOP Encoding

Throughout the AIF header, `NOP` is encoded as:

```
MOV R0, R0      (encoding: 0xE1A00000)
```

**Historical note:** In the earliest AIF specification (1987-1989, version 0.04), NOP was encoded as `BLNV 0` (`0xFB000000`), a branch-with-link using the "never" condition code. Later revisions switched to `MOV R0, R0`.

---

## 7. BL Instruction Usage in the Header

`BL` (Branch with Link) instructions are used in header offsets `0x00` through `0x0C` to make the header addressable via R14 (the Link Register) in a position-independent manner. When a `BL` is executed, R14 receives the return address (PC + 4 in 32-bit mode, or PC + PSR in 26-bit mode). The code at the target of each `BL` can compute the address of the AIF header from R14.

Care is taken to ensure the instruction sequences that compute addresses from R14 work correctly in both 26-bit and 32-bit ARM modes.

---

## 8. Restartability

An AIF image is restartable if, and only if, the program it contains is restartable. An AIF image is **not** reentrant.

For restartability:

1. After decompression, the first word of the header must be set to `NOP` (so decompression is not repeated).
2. After self-relocation, the second word of the header must be set to `NOP` (so relocation is not repeated).

On systems with memory protection, both the decompression code and self-relocation code must be bracketed by system calls to change the access status of the read-only section (first to writable, then back to read-only).

---

## 9. Self-Relocation

Two kinds of self-relocation are supported:

### 9.1 Relocate to Load Address

The image can be loaded at any address and will execute where loaded. The relocation code adjusts all position-dependent values by the difference between the actual load address and the linked base address (stored at offset `0x28`).

### 9.2 Self-Move to High Memory

The image is loaded at a low address, moves itself to the highest address that leaves the required workspace (specified at offset `0x2C`) free, and then relocates itself. This requires a system call that returns the top of available memory (e.g. `SWI 0x10` / `SWI GetEnv` on RISC OS).

### 9.3 Self-Relocation Code

The self-relocation code is appended to the end of an uncompressed AIF image, immediately before the relocation list. It is entered via a `BL` from the second word of the AIF header (offset `0x04`), so on entry R14 points to `AIFHeader + 8`.

The relocation code:

1. Calculates the actual header address from R14 in a CPU-mode-independent fashion.
2. Replaces offset `0x04` with `NOP` so relocation is not repeated on re-entry.
3. Reads the Work Space field (`0x2C`); if zero, skips the self-move step.
4. If self-move is required:
   - Queries available memory via a system call.
   - Calculates image size including relocation data.
   - Copies the image upward in memory, 4 words at a time, in descending address order.
   - Jumps to the copied copy code midway through the process.
5. Processes the relocation list:
   - Reads the linked base from offset `0x28`.
   - Computes the relocation offset (`actual_load_address - linked_base`).
   - If offset is zero, returns immediately.
   - Stores the actual load address at offset `0x28`.
   - Iterates through the relocation list, adding the offset to each referenced word.
   - Terminates when `-1` is encountered.

The self-relocation code can be customised by providing an area called `AIF_RELOC` in the first object file in the linker's input list.

---

## 10. Zero-Initialisation Code

The zero-initialisation code resides in the header from offset `0x40` to `0x7C` (16 words). It is entered via `BL` from offset `0x08`.

### 10.1 Algorithm (SDT 2.50 / 32-bit Mode Version)

```arm
ZeroInit
    NOP                           ; or Debug Init Instruction
    SUB    ip, lr, pc             ; compute base-ZeroInit offset
    ADD    ip, pc, ip             ; compute base+16
    LDMIB  ip, {r0,r1,r2,r4}     ; load RO size, RW size, Debug size, ZI size
    SUB    ip, ip, #16            ; -> image base
    ADD    ip, ip, r0             ; + RO size
    ADD    ip, ip, r1             ; + RW size = base of zero-init area
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMPS   r4, #0
00  MOVLE  pc, lr                 ; nothing left to do, return
    STMIA  ip!, {r0,r1,r2,r3}    ; zero 16 bytes at a time
    SUBS   r4, r4, #16
    B      %B00
```

### 10.2 Algorithm (RISC OS 1989 / 26-bit Mode Version)

```arm
    BIC    IP, LR, #0xFC000003   ; clear PSR bits -> header + 0x0C
    ADD    IP, IP, #8            ; -> Image ReadOnly size field
    LDMIA  IP, {R0,R1,R2,R3}    ; load sizes
    CMPS   R3, #0
    MOVLES PC, LR                ; nothing to do, return with PSR restore
    SUB    IP, IP, #0x14         ; -> image base
    ADD    IP, IP, R0            ; + RO size
    ADD    IP, IP, R1            ; + RW size = base of zero-init area
    MOV    R0, #0
    MOV    R1, #0
    MOV    R2, #0
    MOV    R4, #0
ZeroLoop
    STMIA  IP!, {R0,R1,R2,R4}
    SUBS   R3, R3, #16
    BGT    ZeroLoop
    MOVS   PC, LR                ; return with PSR restore
```

The key difference between versions is how the header address is derived from LR (masking PSR bits in 26-bit mode vs. subtraction in 32-bit mode).

---

## 11. Relationship Between Header Sizes and Linker Symbols

```
Image$$RO$$Base  = ImageBase
Image$$RW$$Base  = ImageBase + ROSize
Image$$ZI$$Base  = ImageBase + ROSize + RWSize
Image$$RW$$Limit = ImageBase + ROSize + RWSize + ZeroInitSize
```

These linker-defined symbols can be imported by assembly language programs or referenced as `extern` addresses from C.

---

## 12. Debugging Data

AIF images support debugging by the ARM Symbolic Debugger (armsd) and other debuggers. Low-level and source-level debugging support are orthogonal; both, either, or neither may be present.

- **References from debugging tables to code/data** are in the form of relocatable addresses. After loading at the load address, these values are effectively absolute.
- **References between debugger table entries** are offsets from the beginning of the debugging data area. After image relocation, the debugging data area is internally position-independent and may be copied or moved.

The debugging data format is defined by the ARM Symbolic Debug Table Format (ASD) specification.

---

## 13. Compression

An AIF image may be compressed by a separate utility that:

1. Compresses the image body (everything after the header).
2. Appends position-independent decompression data and code.
3. Updates the first word of the header to `BL DecompressCode`.
4. Updates size fields as needed.

After decompression, the first word of the header is reset to `NOP` for restartability.

---

## 14. 3DO-Specific Extensions

On the 3DO, the AIF format was extended with a 128-byte `_3DOBinHeader` structure that immediately follows the standard AIF header when the `AIF_3DOHEADER` flag (bit 30 of offset `0x2C`) is set.

### 14.1 3DO Binary Header Structure

```c
struct _3DOBinHeader {
    ItemNode _3DO_Item;          // Standard 3DO item node
    uint8    _3DO_Flags;         // See below
    uint8    _3DO_OS_Version;    // Compiled for this OS release
    uint8    _3DO_OS_Revision;
    uint8    _3DO_Reserved;
    uint32   _3DO_Stack;         // Stack requirements in bytes
    uint32   _3DO_FreeSpace;     // Preallocate bytes for free list
    uint32   _3DO_Signature;     // If privileged, offset to signature
    uint32   _3DO_SignatureLen;  // Length of signature
    uint32   _3DO_MaxUSecs;      // Max microseconds before task switch
    uint32   _3DO_Reserved0;     // Must be zero
    char     _3DO_Name[32];      // Optional task name on startup
    uint32   _3DO_Time;          // Seconds since 1/1/93 00:00:00 GMT
    uint32   _3DO_Reserved1[7];  // Must be zero
};
```

### 14.2 3DO Flags

| Bit | Value | Meaning |
|-----|-------|---------|
| 0   | 1     | `_3DO_PERMSTACK`: Permanent stack |
| 1   | 2     | `_3DO_PRIVILEGE`: Privileged task |
| 2   | 4     | `_3DO_USERAPP`: User app requiring authentication |
| 3   | 8     | `_3DO_LUNCH`: OS launched, chips may be lunched |
| 4   | 16    | `_3DO_NORESETOK`: App willing to keep running on empty drawer |
| 5   | 32    | `_3DO_DATADISCOK`: App willing to accept data discs |

### 14.3 3DO Debug Values

| Constant                | Value          | Meaning |
|------------------------|----------------|---------|
| `PORTFOLIO_DEBUG_CLEAR` | `0xE1A02002`  | NOP (MOV R2, R2) - debug cleared |
| `PORTFOLIO_DEBUG_TASK`  | `0xEF00010A`  | SWI DDE_Debug - debug task |
| `PORTFOLIO_DEBUG_UNSET` | `0xE1A00000`  | NOP (MOV R0, R0) - debug unset |

---

## 15. C Structure Representation

```c
#define AIF_NOOP      0xE1A00000    // MOV R0, R0
#define AIF_BLAL      0xEB000000    // BL (always) base encoding
#define OS_EXIT       0xEF000011    // SWI OS_Exit
#define OS_GETENV     0xEF000010    // SWI OS_GetEnv
#define AIF_IMAGEBASE 0x00008000    // Default RISC OS load address
#define AIF_BLZINIT   0xEB00000C    // BL to ZeroInit (typical)
#define DEBUG_TASK    0xEF041D41    // RISC OS SWI DDE_Debug
#define AIF_DBG_SRC   2            // Source-level debug
#define AIF_DBG_LL    1            // Low-level debug

typedef struct AIFHeader {
    uint32 aif_blDecompress;      // 0x00: NOP if not compressed
    uint32 aif_blSelfReloc;       // 0x04: NOP if not self-relocating
    uint32 aif_blZeroInit;        // 0x08: NOP if no zero-init
    uint32 aif_blEntry;           // 0x0C: BL entry or offset
    uint32 aif_SWIexit;           // 0x10: exit instruction
    int32  aif_ImageROsize;       // 0x14: includes header if executable
    int32  aif_ImageRWsize;       // 0x18: exact size
    int32  aif_DebugSize;         // 0x1C: exact size
    int32  aif_ZeroInitSize;      // 0x20: exact size
    int32  aif_ImageDebugType;    // 0x24: 0, 1, 2, or 3
    uint32 aif_ImageBase;         // 0x28: linked base address
    int32  aif_WorkSpace;         // 0x2C: workspace / stack
    uint32 aif_AddressMode;       // 0x30: 0/26/32 + flags
    uint32 aif_DataBaseAddr;      // 0x34: data base if separate
    uint32 aif_Reserved[2];       // 0x38-0x3C: reserved / extended AIF
    uint32 aif_DebugInit;         // 0x40: NOP or debug SWI
    uint32 aif_ZeroInitCode[15];  // 0x44-0x7C: zero-init routine
} AIFHeader;
```

---

## 16. Customisation

### 16.1 AIF Header Template

The contents of certain AIF header fields (such as the Program Exit Instruction at offset `0x10` and the Debug Init Instruction at offset `0x40`) can be customised by providing a template in an area named `AIF_HDR` with a length of exactly 128 bytes, in the **first** object file in the input list to `armlink`.

### 16.2 Self-Relocation Code

The self-move and self-relocation code appended by the linker can be customised by providing an area named `AIF_RELOC` in the **first** object file in the input list to `armlink`.

---

## 17. Endianness

AIF files can be either little-endian or big-endian. The endianness of the file always matches the endianness of the target ARM system. Tools processing AIF files must handle byte-sex appropriately. The `ChunkFileId` magic number `0xC3CBC6C5` (when the file also uses chunk file format) or the structure of BL instructions in the header can be used to determine endianness.

---

## 18. Historical Versions

| Version    | Date       | Key Changes |
|------------|------------|-------------|
| 0.03       | Aug 1987   | Initial AIF concept |
| 0.04       | Sep 1987   | Major revision; NOP encoded as `BLNV 0` |
| 1.00       | Jan 1989   | Improved relocation support; improved ASD/debugging support; editorial review |
| 2.00       | ~1990      | AOF 200, AIF 3.00, Link 3.00; zero-init bit 7 |
| 3.10 (310) | 1997-1998  | SDT 2.50; 32-bit mode support; Thumb support; Extended AIF; separate data base; `MOV R0,R0` as NOP |
