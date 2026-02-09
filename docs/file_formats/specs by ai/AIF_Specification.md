# ARM Image Format (AIF) — Comprehensive Specification

**Format Name:** ARM Image Format (AIF)  
**Also Known As:** Acorn Image Format, Arthur Image Format, RISC OS Application Image Format  
**Purpose:** Executable program images for ARM processors  
**Header Size:** 128 bytes (32 words)  
**Byte Order:** Target ARM system endianness (typically little-endian)  
**Word Size:** 32 bits (4 bytes)

**Source References:**
- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide (1997-1998)
- PLG-AIF Issue 1.00: RISC OS Application Image Format (Acorn, 1989)
- RISC OS Programmer's Reference Manuals, Appendix D
- 3DO Portfolio OS SDK Documentation (`aif.h`, Revision 1.21, 1994)

---

## 1. Overview

ARM Image Format (AIF) is a simple, self-contained executable image format designed for ARM processors. An AIF file consists of a fixed 128-byte header followed by the executable code, initialized static data, and optional debugging and relocation information.

AIF was originally developed by Acorn Computers Limited for the Archimedes and RISC OS systems. It was subsequently adopted by ARM Ltd for the ARM Software Development Toolkit (SDT) and by the 3DO Interactive Multiplayer console.

### 1.1 Key Features

- **Self-contained execution** — loaded and entered at its first word
- **Self-decompression** — compressed images can decompress themselves at load time
- **Self-relocation** — images can relocate themselves to execute at any address
- **Zero-initialization** — automatic clearing of BSS-style data areas
- **Debug support** — integration with ARM Symbolic Debugger (ASD/armsd)
- **Position independence** — code can be made position-independent through compilation and linking options

### 1.2 Design Principles

1. **Minimal overhead** — 128-byte fixed header, simple to parse
2. **Self-preparation** — executable images prepare themselves without an external loader
3. **Position independence** — images can run at arbitrary addresses
4. **Extensibility** — optional debugging tables, compression, relocation
5. **Restartability** — images can be re-entered after proper cleanup

### 1.3 Historical Versions

| Version | Date | Key Changes |
|---------|------|-------------|
| 0.03 | Aug 1987 | Initial AIF concept |
| 0.04 | Sep 1987 | Source-level debugging support; NOP encoded as `BLNV 0` |
| 1.00 | Jan 1989 | Improved relocation; improved ASD/debugging; editorial review |
| 3.00 | ~1993 | 32-bit ARM mode support; Extended AIF with scatter loading |
| 3.10 | 1997-1998 | SDT 2.50; Thumb support; separate data base; NOP changed to `MOV R0, R0` |

---

## 2. AIF Variants

AIF exists in three distinct variants. The variant is determined by examining the fourth word (offset `0x0C`) of the header.

### 2.1 Executable AIF

The most common variant. Used for standalone applications (e.g., RISC OS `!RunImage` files, 3DO executables).

**Characteristics:**
- The AIF header is part of the image itself
- Image is loaded into memory and entered at its first word (offset `0x00`)
- Code embedded in the header prepares the image for execution (decompression, relocation, zero-initialization) before branching to the entry point
- Base address = address where header is loaded
- Actual code begins at `base + 0x80` (128 bytes after header start)
- The Image ReadOnly Size field at offset `0x14` **includes** the 128-byte header

**Identification:**
- Fourth word (offset `0x0C`) is a `BL entrypoint` instruction
- Most significant byte = `0xEB`

**Standard load address on RISC OS:** `0x8000` (virtual address via MMU)

### 2.2 Non-Executable AIF

Requires an external image loader to prepare the image for execution.

**Characteristics:**
- The header describes but is not part of the executable image
- The loader interprets the header, performs decompression/relocation/zero-initialization, then discards the header
- Base address = address where code should be loaded (header is separate)
- The Image ReadOnly Size field at offset `0x14` **excludes** the header

**Identification:**
- Fourth word (offset `0x0C`) is the byte offset of the entry point from the image base
- Most significant nibble = `0x0`

**Use cases:** Shared library images, relocatable modules, ROM firmware, custom loading scenarios.

### 2.3 Extended AIF

A special form of non-executable AIF for scatter-loaded images containing multiple load regions.

**Characteristics:**
- Contains a chain of load region descriptors within the file
- Each descriptor specifies a load address and size for its region
- The image loader places each region at the specified memory address

**Identification:**
- Non-executable AIF (offset `0x0C` has MSN = `0x0`)
- Word at offset `0x38` is non-zero, containing the byte offset within the file to the first non-root region header

---

## 3. File Layout

### 3.1 Compressed AIF Layout

```
+----------------------------+  Offset 0x00
| Header (128 bytes)         |  NOT compressed
+----------------------------+
| Compressed Image           |
+----------------------------+
| Decompression Data         |  Position-independent
+----------------------------+
| Decompression Code         |  Position-independent
+----------------------------+
```

The header is never compressed, even in a compressed AIF image. Compression is performed by a separate utility which adds self-decompression code and data tables.

### 3.2 Uncompressed AIF Layout

```
+----------------------------+  Offset 0x00
| Header (128 bytes)         |
+----------------------------+  Offset 0x80
| Read-Only Area             |  Code and read-only data
+----------------------------+
| Read-Write Area            |  Initialized writable data
+----------------------------+
| Debugging Data             |  Optional
+----------------------------+
| Self-Relocation Code       |  Position-independent
+----------------------------+
| Relocation List            |  Word offsets, terminated by -1
+----------------------------+
```

- Debugging data is present only if the image was linked with `-d` and/or compiled with `-g`.
- The relocation list is a sequence of byte offsets from the beginning of the AIF header, identifying words to be relocated. The list is terminated by a word containing `-1` (`0xFFFFFFFF`). Only word-sized relocations are supported.

### 3.3 Post-Relocation / Runtime Layout

After self-relocation (or if the image is not self-relocating):

```
+----------------------------+
| Header (128 bytes)         |
+----------------------------+
| Read-Only Area             |
+----------------------------+
| Read-Write Area            |
+----------------------------+
| Debugging Data             |  Optional — must be copied by debugger
+----------------------------+
| Zero-Initialized Area      |  Created at runtime (overwrites reloc data)
+----------------------------+
| Heap / Stack               |
+----------------------------+
```

A debugger should copy debugging data to a safe location before zero-initialization overwrites it. A debugger can seize control by copying and then modifying the third word of the header (offset `0x08`, the `BL ZeroInit` instruction).

---

## 4. AIF Header Structure

The AIF header is exactly **128 bytes (32 words)**. All multi-byte values use the target ARM system byte order.

### 4.1 Complete Header Layout

| Offset | Word | Size | Field | Description |
|--------|------|------|-------|-------------|
| `0x00` | 0 | 4 | BL DecompressCode / NOP | Branch to decompression code, or NOP if not compressed |
| `0x04` | 1 | 4 | BL SelfRelocCode / NOP | Branch to self-relocation code, or NOP if not self-relocating |
| `0x08` | 2 | 4 | BL ZeroInit / NOP | Branch to zero-initialization code, or NOP if no zero-init data |
| `0x0C` | 3 | 4 | BL ImageEntryPoint / EntryPoint Offset | Executable AIF: BL to entry point. Non-executable AIF: byte offset from base |
| `0x10` | 4 | 4 | Program Exit Instruction | Last-resort exit (typically `SWI 0x11` / OS_Exit) |
| `0x14` | 5 | 4 | Image ReadOnly Size | RO area size in bytes. Includes header if executable AIF |
| `0x18` | 6 | 4 | Image ReadWrite Size | Exact RW area size in bytes (multiple of 4) |
| `0x1C` | 7 | 4 | Image Debug Size | Exact debug data size in bytes (multiple of 4) |
| `0x20` | 8 | 4 | Image Zero-Init Size | Exact ZI area size in bytes (multiple of 4) |
| `0x24` | 9 | 4 | Image Debug Type | Debug data type: 0, 1, 2, or 3 |
| `0x28` | 10 | 4 | Image Base | Address where image was linked |
| `0x2C` | 11 | 4 | Work Space | Min workspace for self-moving image (see notes) |
| `0x30` | 12 | 4 | Address Mode | Address mode flags (see section 4.3) |
| `0x34` | 13 | 4 | Data Base | Address where data was linked (if bit 8 of Address Mode is set) |
| `0x38` | 14 | 4 | Reserved Word 1 | Must be 0. Extended AIF: offset to first region header |
| `0x3C` | 15 | 4 | Reserved Word 2 | Must be 0 |
| `0x40` | 16 | 4 | Debug Init Instruction | NOP if unused; SWI to alert debugger if used |
| `0x44-0x7C` | 17-31 | 60 | Zero-Init Code | 15-word zero-initialization subroutine |

**Total: 32 words = 128 bytes = 0x80 bytes**

### 4.2 Detailed Field Specifications

#### Offset 0x00: BL DecompressCode / NOP

**Compressed image:**
```arm
BL DecompressCode    ; 0xEB00xxxx — branch to decompression routine
```

**Uncompressed image:**
```arm
MOV R0, R0           ; 0xE1A00000 — NOP
```

**Historical note:** In the earliest AIF specification (versions 0.03-0.04, 1987-1989), NOP was encoded as `BLNV 0` (`0xFB000000`) — a branch-with-link using the NV (never) condition code. Later revisions switched to `MOV R0, R0`.

After decompression completes, this word **must** be reset to NOP to support image re-entry.

#### Offset 0x04: BL SelfRelocCode / NOP

**Self-relocating image:**
```arm
BL SelfRelocCode     ; 0xEB00xxxx — branch to relocation routine
```

**Non-relocating image:**
```arm
MOV R0, R0           ; 0xE1A00000 — NOP
```

After relocation completes, this word **must** be reset to NOP. The self-relocation code does this automatically.

#### Offset 0x08: BL ZeroInit / NOP

**With zero-initialized data:**
```arm
BL ZeroInit          ; 0xEB00000C — branch to code at offset 0x44
```

**Without zero-initialized data:**
```arm
MOV R0, R0           ; 0xE1A00000 — NOP
```

The `BL` makes the header addressable via R14 (Link Register) in a position-independent manner.

#### Offset 0x0C: BL ImageEntryPoint / EntryPoint Offset

**Executable AIF:**
```arm
BL ImageEntryPoint   ; 0xEB00xxxx — branch to program entry
```
- Most significant byte = `0xEB`
- BL is used to make the header addressable via R14
- The application **must not** return from this call; offset `0x10` is a failsafe

**Non-executable AIF:**
```
EntryPoint Offset    ; 0x0000xxxx — byte offset from image base
```
- Most significant nibble = `0x0`
- Entry address = `Image_Base + EntryPoint_Offset`

#### Offset 0x10: Program Exit Instruction

A last-resort instruction executed if the application incorrectly returns.

| Platform | Value | Encoding | Description |
|----------|-------|----------|-------------|
| RISC OS | `SWI 0x11` | `0xEF000011` | OS_Exit |
| 3DO | `SWI 0x11` | `0xEF000011` | Custom exit handler |
| Bare metal | `B .` | `0xEAFFFFFE` | Branch to self (infinite loop) |

Customizable by providing an `AIF_HDR` area template in the first object file to the linker.

#### Offset 0x14: Image ReadOnly Size

Size of the read-only (code) area in bytes, including any padding.

- **Executable AIF:** **Includes** the 128-byte header
- **Non-executable AIF:** **Excludes** the header
- Must be a multiple of 4 bytes

#### Offset 0x18: Image ReadWrite Size

Exact size of the read-write (initialized data) area in bytes. Must be a multiple of 4. Zero if no initialized data.

#### Offset 0x1C: Image Debug Size

Exact size of the debugging data area in bytes. Must be a multiple of 4. Zero if no debugging data.

**Bit encoding (SDT 2.50):**
- Bits 0-3: Debug type (same as offset `0x24`)
- Bits 4-31: Low-level debug section size

**Note:** For definitive debug type, check offset `0x24` rather than bits 0-3 of this field.

#### Offset 0x20: Image Zero-Init Size

Exact size of the zero-initialized area in bytes. Must be a multiple of 4 (typically a multiple of 16 for efficient zeroing). This area is **not stored in the image file** — it is created at runtime by the zero-init code.

#### Offset 0x24: Image Debug Type

| Value | Meaning |
|-------|---------|
| 0 | No debugging data present |
| 1 | Low-level debugging data present |
| 2 | Source-level (ASD) debugging data present |
| 3 | Both low-level and source-level debugging data present |

All other values are reserved to ARM Limited.

#### Offset 0x28: Image Base

The address at which the image was linked. Set by the linker.

- **Executable AIF:** Address of the AIF header in memory
- **Non-executable AIF:** Address of the code section (header excluded)
- **Standard RISC OS value:** `0x8000`
- Updated to the actual load address after self-relocation

Used by self-relocation code to calculate: `relocation_offset = actual_load_address - linked_base`

#### Offset 0x2C: Work Space

Minimum workspace in bytes to be reserved by a self-moving relocatable image.

- **Value 0:** Image does not self-move; only relocates in place
- **Value > 0:** Image will move itself to high memory, leaving this much free space for ZI area + heap/stack

Marked as **obsolete** in SDT 2.50. See section 8 for 3DO extensions to this field.

#### Offset 0x30: Address Mode

**Least significant byte (bits 0-7):**

| Value | Meaning |
|-------|---------|
| 0 | Old-style 26-bit AIF header (pre-APCS-3) |
| 26 | Image linked for 26-bit ARM mode; may not execute correctly in 32-bit mode |
| 32 | Image linked for 32-bit ARM mode; may not execute correctly in 26-bit mode |

**Bit 8 (mask `0x100`):** If set, the image was linked with separate code and data bases. The Data Base field at offset `0x34` is valid.

**Bits 9-31:** Reserved, must be zero.

#### Offset 0x34: Data Base

Address where the image data was linked. Only meaningful when bit 8 of the Address Mode word is set. Used for images with separate code and data segments (e.g., ROM code with writable data in RAM).

#### Offset 0x38-0x3C: Reserved Words

Two words, must be zero in standard AIF.

**Extended AIF:** Word at `0x38` is non-zero and contains the byte offset within the file of the header for the first non-root load region.

**3DO SDK:** Word at `0x38` is aliased as `aif_MD4DataSize`, suggesting possible use for MD4 hash/authentication data size.

#### Offset 0x40: Debug Init Instruction

Optional instruction to alert a resident debugger that a debuggable image is commencing execution. Executed before zero-initialization.

| Platform | Value | Encoding | Description |
|----------|-------|----------|-------------|
| Unused | NOP | `0xE1A00000` | MOV R0, R0 |
| RISC OS | SWI DDE_Debug | `0xEF041D41` | Alert DDE debugger |
| 3DO | SWI 0x010A | `0xEF00010A` | Alert Portfolio debugger |

Customizable via the `AIF_HDR` area template.

#### Offset 0x44-0x7C: Zero-Init Code (15 words)

Standard position-independent zero-initialization subroutine. Together with the Debug Init instruction at `0x40`, these 16 words complete the 32-word header.

See section 5.1 for the complete code listing and analysis.

### 4.3 Header Byte Map

```
Offset  Content
------  ----------------------------------------------------------
0x00    BL DecompressCode / NOP (0xE1A00000)
0x04    BL SelfRelocCode / NOP
0x08    BL ZeroInit / NOP
0x0C    BL ImageEntryPoint (0xEB??????) / EntryOffset (0x0???????)
0x10    Program Exit Instruction (default: 0xEF000011)
0x14    Image ReadOnly Size (bytes)
0x18    Image ReadWrite Size (bytes)
0x1C    Image Debug Size (bytes)
0x20    Image Zero-Init Size (bytes)
0x24    Image Debug Type (0-3)
0x28    Image Base Address
0x2C    Work Space / 3DO flags+stack
0x30    Address Mode (LSB: 0/26/32; bit 8: separate data base)
0x34    Data Base Address
0x38    Reserved[0] / Extended AIF offset / 3DO MD4 size
0x3C    Reserved[1]
0x40    Debug Init Instruction / NOP
0x44    Zero-Init Code word 0
...     ...
0x7C    Zero-Init Code word 14
------  ----------------------------------------------------------
0x80    End of Header — Code begins here (executable AIF)
```

---

## 5. Initialization Code

### 5.1 Zero-Initialization Code

The zero-init code resides in the header at offsets `0x40`-`0x7C` (16 words including the Debug Init instruction). It is entered via the `BL` instruction at offset `0x08`. On entry, R14 (LR) points to `AIFHeader + 0x0C`.

#### 5.1.1 SDT 2.50 / 32-bit Mode Version

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
        ADD     ip, ip, r1          ; + RW size = base of zero-init area
        MOV     r0, #0
        MOV     r1, #0
        MOV     r2, #0
        MOV     r3, #0
        CMPS    r4, #0
00      MOVLE   pc, lr              ; nothing left to do — return
        STMIA   ip!, {r0,r1,r2,r3} ; zero 16 bytes, post-increment
        SUBS    r4, r4, #16
        B       %B00
; Total: 16 words (including Debug Init instruction at 0x40)
```

#### 5.1.2 RISC OS 1989 / 26-bit Mode Version

```arm
        BIC     IP, LR, #0xFC000003 ; clear PSR bits -> header + 0x0C
        ADD     IP, IP, #8          ; -> Image ReadOnly size field (0x14)
        LDMIA   IP, {R0,R1,R2,R3}   ; load sizes
        CMPS    R3, #0
        MOVLES  PC, LR              ; nothing to do, return with PSR restore
        SUB     IP, IP, #0x14       ; -> image base
        ADD     IP, IP, R0          ; + RO size
        ADD     IP, IP, R1          ; + RW size = base of zero-init area
        MOV     R0, #0
        MOV     R1, #0
        MOV     R2, #0
        MOV     R4, #0
ZeroLoop
        STMIA   IP!, {R0,R1,R2,R4}
        SUBS    R3, R3, #16
        BGT     ZeroLoop
        MOVS    PC, LR              ; return with PSR restore
```

The key difference between versions is how the header base address is derived from LR (masking PSR bits in 26-bit mode via `BIC` vs. subtraction in 32-bit mode via `SUB`/`ADD`).

#### 5.1.3 Position-Independent Address Calculation

The SUB/ADD sequence computes the header base address correctly regardless of load position:

```
On entry:  LR = header + 0x0C  (return address from BL at 0x08)
           PC = ZeroInit + 12  (when SUB executes, PC is 2 instructions ahead)

Step 1:    ip = LR - PC
             = (header + 12) - (ZeroInit + 12)
             = header - ZeroInit

Step 2:    ip = PC + ip
             = (ZeroInit + 16) + (header - ZeroInit)
             = header + 16

Step 3:    ip = ip - 16
             = header  (now pointing to base of image)
```

PSR bits cancel out in the subtraction (step 1), making this work in both 26-bit and 32-bit modes.

#### 5.1.4 Algorithm Summary

1. Calculate base address of AIF header from PC and LR (position-independent)
2. Load size fields from header: RO size, RW size, Debug size, ZI size
3. If ZI size is zero, return immediately
4. Compute zero-init base: `header + RO_size + RW_size`
5. Zero 16 bytes at a time using `STMIA` with four zero-filled registers
6. Loop until entire ZI area is cleared
7. Return via `MOV PC, LR`

**Register usage:** Destroys R0-R4, IP. Preserves R5-R13, LR.

---

## 6. Self-Relocation

### 6.1 Relocation Types

AIF supports three relocation strategies:

| Type | Work Space | Description |
|------|-----------|-------------|
| **No relocation** | N/A | Image runs at linked address; no relocation code present |
| **Relocate to load address** | 0 | Image can be loaded anywhere; relocates in place |
| **Self-move to high memory** | > 0 | Image moves itself up in memory, then relocates |

### 6.2 Relocation List Format

The relocation list is a sequence of 4-byte word offsets from the beginning of the AIF header, identifying words that need adjustment. The list is terminated by a word containing `-1` (`0xFFFFFFFF`).

```
+----------------------------+
| Offset 0  (byte offset)   |  → word at header+offset needs adjustment
+----------------------------+
| Offset 1                   |
+----------------------------+
| ...                        |
+----------------------------+
| Offset N                   |
+----------------------------+
| 0xFFFFFFFF                 |  Terminator (-1)
+----------------------------+
```

Each referenced word is adjusted by: `new_value = original_value + (actual_base - linked_base)`

Only word-sized (32-bit) values can be relocated.

### 6.3 Self-Relocation Code

The self-relocation code is appended to the end of an uncompressed AIF image, immediately before the relocation list. It is entered via `BL` from offset `0x04`, so on entry R14 points to `AIFHeader + 0x08`.

```arm
RelocCode
        NOP                         ; required by ensure_byte_order()
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

; --- Self-Move Phase (only if WorkSpace > 0) ---

        LDR     r0, [ip, #0x20]     ; image zero-init size
        ADD     r9, r9, r0          ; space to leave = min free + zero init
        SWI     #0x10               ; get top of memory in r1 (system-specific)

        ADR     r2, End             ; -> end of relocation code
01      LDR     r0, [r2], #4        ; scan to find end of relocation list
        CMNS    r0, #1              ; check for -1 terminator
        BNE     %B01                ; loop until end found

        SUB     r3, r1, r9          ; MemLimit - freeSpace
        SUBS    r0, r3, r2          ; amount to move by
        BLE     RelocateOnly        ; not enough space to move
        BIC     r0, r0, #15         ; round down to multiple of 16
        ADD     r3, r2, r0          ; new end address
        ADR     r8, %F02            ; intermediate limit for copy-up

; Copy image upward in descending address order (safe in-place)
02      LDMDB   r2!, {r4-r7}        ; load 4 words from end
        STMDB   r3!, {r4-r7}        ; store to new location
        CMPS    r2, r8              ; copied the copy loop itself?
        BGT     %B02                ; not yet

        ADD     r4, pc, r0          ; calculate jump address
        MOV     pc, r4              ; jump to copied copy code

; Continue copying from new location
03      LDMDB   r2!, {r4-r7}
        STMDB   r3!, {r4-r7}
        CMPS    r2, ip              ; copied everything?
        BGT     %B03                ; not yet
        ADD     ip, ip, r0          ; update header address
        ADD     lr, lr, r0          ; update return address

; --- Relocation Phase ---

RelocateOnly
        LDR     r1, [ip, #0x28]     ; code base as set by linker
        SUBS    r1, ip, r1          ; relocation offset = actual - linked
        MOVEQ   pc, lr              ; if zero, nothing to do — return
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

### 6.4 Relocation Code Customization

The self-move and self-relocation code can be customized by providing an area named `AIF_RELOC` in the **first** object file in `armlink`'s input list.

---

## 7. Debugging Support

### 7.1 Debug Data Location and Format

Debugging data is stored in the image after the Read-Write area. It uses the ARM Symbolic Debugging (ASD) format.

### 7.2 Address Reference Types

- **References from debug tables to code/data:** Relocatable addresses — effectively absolute after loading at the correct address
- **References between debug table entries:** Offsets from the beginning of the debug data area — position-independent after relocation

### 7.3 Debug Types

| Type | Value | Content |
|------|-------|---------|
| None | 0 | No debugging data |
| Low-level | 1 | Assembly-level symbols, addresses, registers |
| Source-level | 2 | Source file info, line numbers, type definitions, variables (ASD format) |
| Both | 3 | Combined low-level and source-level data |

### 7.4 Debugger Interaction

A debugger seizes control at the appropriate moment by:

1. Copying the third word of the AIF header (offset `0x08`, the `BL ZeroInit`)
2. Replacing it with `BL DebuggerEntry`
3. Allowing decompression and relocation to proceed normally
4. When the debugger's code is entered:
   - Copy debug data to a safe location (before zero-init overwrites it)
   - Restore the original `BL ZeroInit` instruction
   - Allow zero-initialization to proceed
5. Use saved debug data for breakpoints, variable inspection, etc.

---

## 8. 3DO Platform Extensions

### 8.1 Work Space Field Extensions (Offset 0x2C)

On the 3DO, the Work Space field has additional bit-field encoding:

| Bits | Mask | Meaning |
|------|------|---------|
| 0-23 | `0x00FFFFFF` | Stack size (maximum 16 MiB) |
| 24-29 | — | Reserved |
| 30 | `0x40000000` | `AIF_3DOHEADER` flag — 128-byte 3DO binary header follows |
| 31 | — | Reserved |

When `AIF_3DOHEADER` is set, the system reads task parameters from the `_3DOBinHeader` structure rather than from the AIF header's workspace/stack encoding.

### 8.2 3DO Binary Header (`_3DOBinHeader`)

When `AIF_3DOHEADER` (bit 30 of offset `0x2C`) is set, a 128-byte `_3DOBinHeader` immediately follows the standard AIF header at file offset `0x80`:

```c
typedef struct _3DOBinHeader {
    ItemNode _3DO_Item;          /* +0x00: Standard 3DO kernel item node */
    uint8    _3DO_Flags;         /* +0x10: Application flags (see below) */
    uint8    _3DO_OS_Version;    /* +0x11: Compiled for this OS version */
    uint8    _3DO_OS_Revision;   /* +0x12: OS revision */
    uint8    _3DO_Reserved;      /* +0x13: Must be zero */
    uint32   _3DO_Stack;         /* +0x14: Stack size requirements (bytes) */
    uint32   _3DO_FreeSpace;     /* +0x18: Preallocate bytes for FreeList */
    uint32   _3DO_Signature;     /* +0x1C: If privileged, offset to RSA signature */
    uint32   _3DO_SignatureLen;  /* +0x20: Length of signature in bytes */
    uint32   _3DO_MaxUSecs;      /* +0x24: Max microseconds before task switch */
    uint32   _3DO_Reserved0;     /* +0x28: Must be zero */
    char     _3DO_Name[32];      /* +0x2C: Optional task name (NUL-terminated) */
    uint32   _3DO_Time;          /* +0x4C: Seconds since 1/1/93 00:00:00 GMT */
    uint32   _3DO_Reserved1[7];  /* +0x50: Must be zero */
} _3DOBinHeader;                 /* Total: 128 bytes */
```

### 8.3 3DO Flags (`_3DO_Flags`)

| Bit | Value | Name | Meaning |
|-----|-------|------|---------|
| 0 | 0x01 | `_3DO_PERMSTACK` | Permanent stack (not returned on exit) |
| 1 | 0x02 | `_3DO_PRIVILEGE` | Privileged application |
| 2 | 0x04 | `_3DO_USERAPP` | User application requiring authentication |
| 3 | 0x08 | `_3DO_LUNCH` | OS has been launched, chips may be initialized |
| 4 | 0x10 | `_3DO_NORESETOK` | App willing to keep running on empty drawer |
| 5 | 0x20 | `_3DO_DATADISCOK` | App willing to accept data discs |

### 8.4 3DO Debugger Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `PORTFOLIO_DEBUG_CLEAR` | `0xE1A02002` | NOP (MOV R2, R2) — debug cleared |
| `PORTFOLIO_DEBUG_TASK` | `0xEF00010A` | SWI instruction — alert Portfolio debugger |
| `PORTFOLIO_DEBUG_UNSET` | `0xE1A00000` | NOP (MOV R0, R0) — debug not set |

---

## 9. Extended AIF Region Headers

For Extended AIF images (scatter-loaded), each non-root load region is described by a 44-byte region header.

### 9.1 Region Header Format

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| +0x00 | 4 bytes | Next Region Offset | File offset of next region header (0 if last) |
| +0x04 | 4 bytes | Load Address | Target memory address for this region |
| +0x08 | 4 bytes | Region Size | Size in bytes (must be multiple of 4) |
| +0x0C | 32 bytes | Region Name | 32-character name, padded with zero bytes |

**Total: 44 bytes per region header**

The initializing data for each region immediately follows its 44-byte header.

### 9.2 Processing Extended AIF

```
1. Read main AIF header
2. Check word at 0x38 — if non-zero, this is Extended AIF
3. Load root region (described by main header's RO + RW areas)
4. offset = header[0x38]
5. while (offset != 0):
     a. Read 44-byte region header at file offset
     b. Load region data (following header) to specified load address
     c. offset = region_header.next_region_offset
```

---

## 10. Execution Flow

For an executable AIF image, the complete execution flow is:

```
1. LOAD     → Image loaded at its load address
2. ENTER    → Execution begins at first word (offset 0x00)
3. DECOMPRESS (offset 0x00):
              → If BL: decompress image, reset word to NOP
              → If NOP: fall through
4. RELOCATE (offset 0x04):
              → If BL: self-move (optional) + relocate, reset word to NOP
              → If NOP: fall through
5. ZERO-INIT (offset 0x08):
              → If BL: zero-initialize BSS section
              → If NOP: fall through
6. ENTRY    (offset 0x0C):
              → BL to program entry point
              → R14 set to header, making it addressable
7. EXIT     (offset 0x10):
              → If entry point returns (should not happen):
                 execute fallback exit instruction
```

---

## 11. Restartability and Memory Protection

### 11.1 Restartability

An AIF image is restartable if and only if the program it contains is restartable. An AIF image is **not** reentrant (cannot have multiple concurrent executions).

For restartability:
1. After decompression, offset `0x00` must be reset to NOP
2. After relocation, offset `0x04` must be reset to NOP
3. Both resets happen automatically during normal execution

### 11.2 Memory Protection

On systems with memory protection, both decompression and relocation code must:
1. Call the system to make the RO section writable
2. Perform modification (decompress, relocate, reset header words to NOP)
3. Call the system to restore read-only protection

---

## 12. Memory Layout Relationships

### 12.1 Linker Pre-defined Symbols

```
Image$$RO$$Base  = ImageBase
Image$$RW$$Base  = ImageBase + ROSize
Image$$ZI$$Base  = ImageBase + ROSize + RWSize
Image$$RW$$Limit = ImageBase + ROSize + RWSize + ZeroInitSize
```

### 12.2 File Size Calculation

```
Uncompressed:
  File_Size = Header(128) + ROSize + RWSize + DebugSize + RelocCodeSize + RelocListSize

Compressed:
  File_Size = Header(128) + CompressedDataSize + DecompDataSize + DecompCodeSize
```

### 12.3 Runtime Memory Footprint

```
After initialization:
  Memory = ROSize + RWSize + ZeroInitSize + Heap + Stack
```

---

## 13. Endianness

- AIF files use the endianness of the **target ARM system**
- Most ARM systems are little-endian (LSB at lowest address)
- Big-endian ARM systems are supported but less common
- No explicit endianness indicator exists in the AIF header itself
- Endianness can be inferred by examining instruction encodings at known offsets (e.g., NOP = `0xE1A00000` vs byte-swapped)
- If the file also uses chunk file format, the `ChunkFileId` magic number (`0xC3CBC6C5` vs `0xC5C6CBC3`) indicates endianness

---

## 14. File Identification

### 14.1 Detecting AIF vs. Non-AIF

1. Check file size ≥ 128 bytes
2. Check offset `0x00`: Should be NOP (`0xE1A00000`) or BL (`0xEB......`)
3. Check offset `0x10`: Should contain a plausible exit instruction (`0xEF000011`)
4. Check sizes at offsets `0x14`-`0x20`: Should be non-negative, multiples of 4

### 14.2 Detecting Executable vs. Non-Executable

```c
uint32_t fourth_word = read_word_at(0x0C);
if ((fourth_word >> 24) == 0xEB) {
    // Executable AIF — BL instruction
} else if ((fourth_word >> 28) == 0x0) {
    // Non-executable AIF — offset value
} else {
    // Invalid or corrupt
}
```

### 14.3 Detecting Extended AIF

```c
uint32_t reserved1 = read_word_at(0x38);
if (reserved1 != 0) {
    // Extended AIF — reserved1 is offset to first region header
}
```

### 14.4 Detecting 3DO Extension

```c
uint32_t workspace = read_word_at(0x2C);
if (workspace & 0x40000000) {
    // 3DO binary header follows at offset 0x80
}
```

---

## 15. C Structure Definition

```c
#ifndef AIF_HEADER_H
#define AIF_HEADER_H

#include <stdint.h>

/* ===== Constants ===== */
#define AIF_HEADER_SIZE         128
#define AIF_CODE_OFFSET         0x80

#define AIF_NOP                 0xE1A00000  /* MOV R0, R0 */
#define AIF_NOP_LEGACY          0xFB000000  /* BLNV 0 (pre-1989) */
#define AIF_BL_BASE             0xEB000000  /* BL instruction base */
#define AIF_SWI_EXIT            0xEF000011  /* SWI 0x11 (OS_Exit) */
#define AIF_SWI_GETENV          0xEF000010  /* SWI 0x10 (OS_GetEnv) */
#define AIF_DEFAULT_BASE        0x00008000  /* Standard RISC OS load address */
#define AIF_RELOC_TERMINATOR    0xFFFFFFFF  /* Relocation list terminator */

/* Debug type values */
#define AIF_DEBUG_NONE          0
#define AIF_DEBUG_LOW           1
#define AIF_DEBUG_SOURCE        2
#define AIF_DEBUG_BOTH          3

/* Address mode values */
#define AIF_MODE_OLD_26BIT      0
#define AIF_MODE_26BIT          26
#define AIF_MODE_32BIT          32
#define AIF_SEPARATE_DATA_BASE  0x00000100  /* Bit 8 of Address Mode */

/* 3DO flags */
#define AIF_3DOHEADER           0x40000000  /* Bit 30 of Work Space */
#define _3DO_PERMSTACK          0x01
#define _3DO_PRIVILEGE          0x02
#define _3DO_USERAPP            0x04
#define _3DO_LUNCH              0x08
#define _3DO_NORESETOK          0x10
#define _3DO_DATADISCOK         0x20

/* 3DO debugger values */
#define PORTFOLIO_DEBUG_CLEAR   0xE1A02002  /* MOV R2, R2 */
#define PORTFOLIO_DEBUG_TASK    0xEF00010A  /* SWI 0x010A */
#define PORTFOLIO_DEBUG_UNSET   0xE1A00000  /* MOV R0, R0 */

/* ===== AIF Header Structure ===== */
typedef struct AIFHeader {
    uint32_t bl_decompress;         /* 0x00: BL DecompressCode or NOP */
    uint32_t bl_self_reloc;         /* 0x04: BL SelfRelocCode or NOP */
    uint32_t bl_zero_init;          /* 0x08: BL ZeroInit or NOP */
    uint32_t bl_entry;              /* 0x0C: BL EntryPoint or offset */
    uint32_t swi_exit;              /* 0x10: Program exit instruction */
    uint32_t ro_size;               /* 0x14: ReadOnly size (incl. header if exec) */
    uint32_t rw_size;               /* 0x18: ReadWrite size */
    uint32_t debug_size;            /* 0x1C: Debug data size */
    uint32_t zi_size;               /* 0x20: Zero-Init size */
    uint32_t debug_type;            /* 0x24: Debug type (0-3) */
    uint32_t image_base;            /* 0x28: Linked base address */
    uint32_t workspace;             /* 0x2C: Min workspace / 3DO flags+stack */
    uint32_t address_mode;          /* 0x30: 0/26/32 + flags */
    uint32_t data_base;             /* 0x34: Data base if separate */
    uint32_t reserved[2];           /* 0x38: Reserved (Extended AIF uses [0]) */
    uint32_t debug_init;            /* 0x40: Debug init instruction or NOP */
    uint32_t zero_init_code[15];    /* 0x44-0x7C: Zero-init subroutine */
} AIFHeader;                        /* Total: 32 words = 128 bytes */

/* ===== Extended AIF Region Descriptor ===== */
typedef struct AIFRegionHeader {
    uint32_t next_offset;           /* File offset of next region (0 if last) */
    uint32_t load_address;          /* Target memory address */
    uint32_t size;                  /* Region size in bytes (multiple of 4) */
    char     name[32];              /* Region name (NUL-padded) */
} AIFRegionHeader;                  /* Total: 44 bytes */

/* ===== 3DO Binary Header ===== */
typedef struct _3DOBinHeader {
    uint32_t item_node[4];          /* +0x00: ItemNode structure */
    uint8_t  flags;                 /* +0x10: 3DO flags */
    uint8_t  os_version;            /* +0x11: Target OS version */
    uint8_t  os_revision;           /* +0x12: Target OS revision */
    uint8_t  reserved;              /* +0x13: Must be zero */
    uint32_t stack;                 /* +0x14: Stack requirements (bytes) */
    uint32_t freespace;             /* +0x18: FreeList preallocation (bytes) */
    uint32_t signature;             /* +0x1C: Offset to RSA signature */
    uint32_t signature_len;         /* +0x20: Signature length (bytes) */
    uint32_t max_usecs;             /* +0x24: Max microseconds before switch */
    uint32_t reserved0;             /* +0x28: Must be zero */
    char     name[32];              /* +0x2C: Task name (NUL-terminated) */
    uint32_t time;                  /* +0x4C: Seconds since 1/1/93 00:00:00 GMT */
    uint32_t reserved1[7];          /* +0x50: Must be zero */
} _3DOBinHeader;                    /* Total: 128 bytes */

/* ===== Helper Macros ===== */

/* Check if image is executable AIF */
#define AIF_IS_EXECUTABLE(hdr) \
    (((hdr)->bl_entry & 0xFF000000) == 0xEB000000)

/* Check if image is compressed */
#define AIF_IS_COMPRESSED(hdr) \
    ((hdr)->bl_decompress != AIF_NOP && (hdr)->bl_decompress != AIF_NOP_LEGACY)

/* Check if image is self-relocating */
#define AIF_IS_RELOCATING(hdr) \
    ((hdr)->bl_self_reloc != AIF_NOP && (hdr)->bl_self_reloc != AIF_NOP_LEGACY)

/* Check if image has zero-init code */
#define AIF_HAS_ZEROINIT(hdr) \
    ((hdr)->bl_zero_init != AIF_NOP && (hdr)->bl_zero_init != AIF_NOP_LEGACY)

/* Check if extended AIF */
#define AIF_IS_EXTENDED(hdr) \
    ((hdr)->reserved[0] != 0)

/* Check if 3DO header present */
#define AIF_HAS_3DO_HEADER(hdr) \
    (((hdr)->workspace & AIF_3DOHEADER) != 0)

/* Get entry offset for non-executable AIF */
#define AIF_GET_ENTRY_OFFSET(hdr) \
    ((hdr)->bl_entry & 0x00FFFFFF)

/* Get address mode */
#define AIF_GET_MODE(hdr) \
    ((hdr)->address_mode & 0xFF)

/* Check if separate data base */
#define AIF_HAS_SEPARATE_DATA(hdr) \
    (((hdr)->address_mode & AIF_SEPARATE_DATA_BASE) != 0)

#endif /* AIF_HEADER_H */
```

---

## 16. Customization

### 16.1 Custom AIF Header

Provide an area named `AIF_HDR` (exactly 128 bytes) in the **first** object file in the linker's input list. The linker uses this template instead of the default header. This allows customization of the exit instruction, debug init instruction, and other fields.

```arm
    AREA AIF_HDR, CODE, READONLY
    DCD 0xE1A00000              ; NOP (no compression)
    DCD 0xEB00xxxx              ; BL SelfRelocCode
    DCD 0xEB00000C              ; BL ZeroInit
    DCD 0xEB00yyyy              ; BL entry
    DCD 0xEF000099              ; SWI custom_exit
    ; ... rest of 128-byte header ...
```

### 16.2 Custom Relocation Code

Provide an area named `AIF_RELOC` in the **first** object file in the linker's input list. The linker uses this code instead of the default self-relocation/self-move code.

---

## 17. Related Formats

| Format | Purpose | Relationship |
|--------|---------|-------------|
| **AOF** (ARM Object Format) | Relocatable object files | Linker input; linked together to produce AIF output |
| **ALF** (ARM Library Format) | Collections of AOF files | Linker searches ALF libraries for needed AOF modules |
| **ELF** (Executable and Linkable Format) | Modern replacement | Supersedes AIF on modern ARM systems (RISC OS 5+, Linux) |

---

## 18. Linker Commands

```bash
# Create executable AIF
armlink -aif -o output.aif input1.o input2.o

# Create relocatable AIF
armlink -aif -reloc -o output.aif input.o

# Include debugging information
armlink -aif -debug -o output.aif input.o

# Set image base address
armlink -aif -ro-base 0x8000 -o output.aif input.o

# Set separate data base
armlink -aif -ro-base 0x8000 -rw-base 0x20000 -o output.aif input.o
```

---

**End of ARM Image Format (AIF) Specification**
