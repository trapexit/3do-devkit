# ARM Image Format (AIF) Specification

**Consolidated Technical Reference**

---

## 1. Introduction

ARM Image Format (AIF) is a simple executable image format for ARM processors. It was originally designed by Acorn Computers Limited as the "Arthur Image Format" (later renamed "RISC OS Application Image Format") for RISC OS systems (1987-1989). It was subsequently adopted and extended by ARM Limited for cross-platform use in the ARM Software Development Toolkit (SDT). AIF has been used on RISC OS computers (Archimedes, RiscPC), the 3DO Interactive Multiplayer console, set-top boxes, and various other ARM-based embedded systems.

An AIF image consists of:

- A **128-byte header** (32 words at 4 bytes each)
- The image's **executable code** (read-only area)
- The image's **initialised static data** (read-write area)
- Optional **debugging data**
- Optional **self-relocation code and relocation list**

AIF was designed for extreme simplicity. It lacks standardised fields for security and validation, compiler or assembler identification, ABI or OS target information, format versioning, and dynamic linking. It has been largely superseded by ELF on modern ARM systems but remains relevant for legacy platforms and for understanding how ARM executables were historically structured.

---

## 2. Terminology

| Term | Definition |
|------|-----------|
| **Byte** | 8 bits, unsigned unless stated otherwise. Usually used to store flag bits or characters. |
| **Half word** | 16 bits (2 bytes), usually unsigned. The least significant byte has the lowest address (little-endian). Must be aligned on a 2-byte boundary. |
| **Word** | 32 bits (4 bytes), usually used to store a non-negative value. The least significant byte has the lowest address (little-endian). Must be aligned on a 4-byte boundary. |
| **String** | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL is part of the string but is not counted in the string's length. May be aligned on any byte boundary. |
| **NOP** | No Operation. Encoded as `MOV r0, r0` (`0xE1A00000`) in later (1993+) AIF, or as `BLNV 0` (`0xFB000000`) in early (1989) AIF. |

---

## 3. Endianness

AIF images can be produced in either **little-endian** (DEC/Intel/Acorn byte order) or **big-endian** (IBM/Motorola/Apple byte order) format. The endianness of an AIF file always matches the endianness of the target ARM system.

Within a word, in little-endian format, the least significant byte has the lowest address. This is DEC/Intel, or little-endian, byte sex, **not** IBM/Motorola byte sex.

---

## 4. AIF Variants

Three variants of AIF exist, distinguished by the encoding of the fourth word (offset `0x0C`) of the header:

### 4.1 Executable AIF

The most common variant. The header is part of the image itself. The image is loaded at its load address and entered at its first word (the first word of the header). Code in the header ensures the image is properly prepared for execution — performing decompression, self-relocation, and zero-initialisation as necessary — before the program is entered at its entry point.

**Identification:** The fourth word of the header (offset `0x0C`) is a `BL entrypoint` instruction. The most significant byte of this word (in target byte order) is `0xEB`.

**Base address:** The address at which the header should be loaded. The actual code begins at `base + 0x80` (immediately after the 128-byte header).

**Entry conditions:** On entry to a program in AIF, the general registers contain nothing of value to the program. The program is expected to communicate with its operating environment using SWI instructions or by calling functions at known, fixed addresses.

The load address may be determined by:
- An implicit property of the file type (e.g., RISC OS Absolute file type, Unix executable file types)
- Reading the Image Base field at offset `0x28` in the AIF file
- Explicit instruction to an operating system or debugger to load at a specified address

On RISC OS, the standard base address for executable AIF images is `0x8000`. This is a virtual address mapped by the WIMP's MMU-based memory management; different applications each see `0x8000` as their base, but the physical memory pages are distinct for each task.

### 4.2 Non-Executable AIF

The header describes the image but is not part of it. Must be processed by an image loader which interprets the header, prepares the image for execution (performing any required decompression, relocation, and zero-initialisation), and then discards the header.

**Identification:** The fourth word of the header (offset `0x0C`) is the offset of the entry point from the image's base address. The most significant nibble of this word (in target byte order) is `0x0`.

**Base address:** The address at which the code (not the header) should be loaded.

### 4.3 Extended AIF

A special type of non-executable AIF containing a scatter-loaded image. The AIF header points to a chain of load region descriptors within the file. The image loader should place each region at the memory location specified by its descriptor.

**Identification:** The reserved word at offset `0x38` in the main AIF header is non-zero, containing the byte offset within the file of the header for the first non-root load region.

---

## 5. Image Layout

### 5.1 Compressed AIF Image

An AIF image may be compressed to support faster loading from slow peripherals (floppy disks, slow ROMs) and better use of storage space. Compression is performed by a separate utility which adds self-decompression code and data tables to the image.

```
+---------------------------+
| Header (128 bytes)        |  NOT compressed
+---------------------------+
| Compressed image          |
+---------------------------+
| Decompression data        |  Position-independent
+---------------------------+
| Decompression code        |  Position-independent
+---------------------------+
```

The header is never compressed. Both the decompression data and decompression code must be position-independent.

### 5.2 Uncompressed AIF Image

Once decompressed, or if the image was never compressed:

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only area            |  Executable code
+---------------------------+
| Read-Write area           |  Initialised data
+---------------------------+
| Debugging data            |  Optional
+---------------------------+
| Self-relocation code      |  Position-independent; added by linker
+---------------------------+
| Relocation list           |  Word offsets from header base, terminated by -1
+---------------------------+
```

- **Debugging data** is present only if the image was linked with the linker's `-d` option and, for source-level debugging, if components were compiled with the compiler's `-g` option.
- The **relocation list** is a sequence of byte offsets from the beginning of the AIF header, identifying words to be relocated, terminated by a word containing `-1` (`0xFFFFFFFF`). Relocation of non-word values is not supported.

### 5.3 Post-Relocation / Final Execution Layout

After self-relocation has executed (or if the image is not self-relocating):

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only area            |
+---------------------------+
| Read-Write area           |
+---------------------------+
| Debugging data            |  Optional
+---------------------------+
```

At this stage, a debugger should copy any debugging data to a safe location, as the debugging data will be overwritten by the zero-initialised data, heap, and/or stack of the program. A debugger can intercept execution at this point by copying and then modifying the third word of the AIF header (offset `0x08`).

---

## 6. AIF Header Layout

The header is exactly **128 bytes** (32 words). All offsets are in bytes from the start of the header.

### 6.1 Header Field Map

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| `0x00` | 4 | BL DecompressCode | Branch-with-link to decompression code. **NOP** if image is not compressed. |
| `0x04` | 4 | BL SelfRelocCode | Branch-with-link to self-relocation code. **NOP** if image is not self-relocating. |
| `0x08` | 4 | BL DBGInit/ZeroInit | Branch-with-link to zero-initialisation (and optionally debug init) code at offset `0x40`. **NOP** if image has no zero-initialised data. |
| `0x0C` | 4 | BL ImageEntryPoint / EntryPoint Offset | **Executable AIF:** `BL` to the image entry point (MSB = `0xEB`). The `BL` encoding makes the header addressable via `r14` in a position-independent manner. **Non-executable AIF:** Offset from base to entry point (MSN = `0x0`). |
| `0x10` | 4 | Program Exit Instruction | Executed only if the application returns, which it should not do. Usually `SWI 0x11` (OS_Exit on RISC OS). A branch-to-self is recommended on systems lacking a suitable SWI. Can be customised via an `AIF_HDR` area in the first input object file. |
| `0x14` | 4 | Image ReadOnly Size | Size of the read-only (code) area in bytes. **Includes** the 128-byte header size for executable AIF; **excludes** it for non-executable AIF. Includes any padding. |
| `0x18` | 4 | Image ReadWrite Size | Exact size of the read-write (initialised data) area in bytes. Must be a multiple of 4. |
| `0x1C` | 4 | Image Debug Size | Exact size of debugging data in bytes. Must be a multiple of 4. In later versions of the format, bits 0-3 hold the debug type and bits 4-31 hold the low-level debug size. |
| `0x20` | 4 | Image Zero-Init Size | Exact size of the zero-initialised (BSS) data area in bytes. Must be a multiple of 4. |
| `0x24` | 4 | Image Debug Type | See Debug Type table in section 6.4. |
| `0x28` | 4 | Image Base | The address at which the image (code) was linked. Used by the relocation mechanism to compute the relocation delta. |
| `0x2C` | 4 | Work Space | Minimum workspace in bytes to be reserved above a self-moving relocatable image. A value of 0 means no self-move; relocate in place only. Marked obsolete in later (1997+) specifications. |
| `0x30` | 4 | Address Mode | See Address Mode section 6.5. In the 1989 specification this was one of four reserved words. |
| `0x34` | 4 | Data Base | Address at which the image data was linked. Only meaningful when bit 8 of the Address Mode word is set (separate code and data bases). In the 1989 specification this was a reserved word. |
| `0x38` | 4 | Reserved Word 1 | Must be 0. In Extended AIF: byte offset within the file to the first non-root load region header. |
| `0x3C` | 4 | Reserved Word 2 | Must be 0. |
| `0x40` | 4 | Debug Init Instruction | A SWI instruction to alert a resident debugger that a debuggable image is commencing execution, or **NOP** if unused. Can be customised via an `AIF_HDR` area. |
| `0x44` | 60 | Zero-Init Code | 15 words of zero-initialisation code. Together with the Debug Init Instruction at `0x40`, these 16 words form the complete zero-init routine. The complete header is 32 words (0x80 bytes). |

### 6.2 NOP Encoding History

| AIF Era | NOP Encoding | Instruction | Notes |
|---------|-------------|-------------|-------|
| Early (1989, RISC OS PLG-AIF) | `BLNV 0` | `0xFB000000` | Branch-with-link, Never condition. NV condition causes the instruction to never execute, effectively a NOP. |
| Later (1993+, ARM SDT) | `MOV r0, r0` | `0xE1A00000` | The standard ARM NOP encoding. On early ARMs, `MOV r0,r0` without condition flags is fully decoded but produces no visible effect. |

The change was made because `BLNV` was deprecated as a NOP encoding in later ARM architectures. `MOV r0, r0` became the standard recommendation.

### 6.3 Use of BL in the Header

`BL` (Branch with Link) is used in the first four instruction words of the header for two reasons:

1. It makes the header addressable via `r14` (the link register) in a position-independent manner. When a `BL` executes, `r14` receives the address of the next instruction, allowing the called code to compute the header's actual memory address regardless of where it was loaded.
2. It ensures the header itself is position-independent, as `BL` uses a PC-relative offset.

Care is taken in later (1993+) versions to ensure that instruction sequences which compute addresses from `r14` values work correctly in both 26-bit and 32-bit ARM modes.

### 6.4 Image Debug Type

| Value | Meaning |
|-------|---------|
| 0 | No debugging data present |
| 1 | Low-level debugging data present |
| 2 | Source-level (ASD — ARM Symbolic Debug) debugging data present |
| 3 | Both low-level and source-level debugging data present |

All other values are reserved (originally to Acorn; later to ARM Ltd).

### 6.5 Address Mode Word (Offset `0x30`)

This word was introduced in the 1993 revision. In the 1989 specification, offsets `0x30`-`0x3F` were four reserved words, all initially zero.

The least significant byte (in target byte order) encodes the addressing mode:

| Value | Meaning |
|-------|---------|
| 0 | Old-style 26-bit AIF header (pre-dates the address mode field) |
| 26 | Image linked for 26-bit ARM mode; may not execute correctly in 32-bit mode |
| 32 | Image linked for 32-bit ARM mode; may not execute correctly in 26-bit mode |

**Bit 8** (mask `0x100`): If set, the image was linked with separate code and data bases (the data is usually placed immediately after the code in memory). When this bit is set, the word at offset `0x34` contains the base address of the image's data.

### 6.6 Extended AIF Region Header

For Extended AIF images, the word at offset `0x38` in the main AIF header is non-zero and contains the byte offset within the file of the first non-root load region header. Each region header is **44 bytes** (11 words):

| Word | Size | Description |
|------|------|-------------|
| 0 | 4 | File offset of the next region header (0 if this is the last region) |
| 1 | 4 | Load address for this region |
| 2 | 4 | Size of this region in bytes (must be a multiple of 4) |
| 3-10 | 32 | Region name (32 characters, padded with zero bytes) |

The initialising data for the region immediately follows its header in the file.

---

## 7. Linker Symbol Relationships

The following relationships hold between header fields and linker-predefined symbols:

```
Image$$RO$$Base    = ImageBase

Image$$RW$$Base    = ImageBase + ROSize

Image$$ZI$$Base    = ImageBase + ROSize + RWSize

Image$$RW$$Limit   = ImageBase + ROSize + RWSize + ZeroInitSize
```

Where:
- `ImageBase` is the value at offset `0x28`
- `ROSize` is the value at offset `0x14`
- `RWSize` is the value at offset `0x18`
- `ZeroInitSize` is the value at offset `0x20`

These symbols are defined by the linker and can be referenced from application code.

---

## 8. Execution Sequence

When an executable AIF image is loaded and entered at its first word, the following sequence occurs:

### Step 1: Decompression (Offset `0x00`)

If the image is compressed, the `BL DecompressCode` instruction at offset `0x00` branches to decompression code and data tables appended after the compressed image. The decompression code decompresses the image in place and **resets the first header word to NOP** for re-startability.

If the image is not compressed, this word is NOP and execution falls through.

### Step 2: Self-Relocation (Offset `0x04`)

If the image is self-relocating, the `BL SelfRelocCode` instruction at offset `0x04` branches to self-relocation code appended after the debugging data. This code may optionally self-move the image up in memory. After processing, it **resets the second header word to NOP** for re-startability.

If the image is not self-relocating, this word is NOP and execution falls through.

### Step 3: Zero-Initialisation (Offset `0x08`)

If the image has zero-initialised data, the `BL ZeroInit` instruction at offset `0x08` branches to zero-init code embedded in the header itself (offsets `0x40`-`0x7F`). This code zeroes the BSS segment.

If the image has no zero-initialised data, this word is NOP and execution falls through.

### Step 4: Entry (Offset `0x0C`)

The `BL ImageEntryPoint` instruction branches to the program's entry point. The `BL` encoding makes the header addressable via `r14`, but the application is expected to **never return** to this point.

The Image EntryPoint may be the start of the application's own code, or it may be the initialisation routine of a runtime library (e.g., SharedCLibrary on RISC OS), which will in turn call the application's `main` function.

### Step 5: Exit Trap (Offset `0x10`)

If the program erroneously returns from its entry point, the instruction at offset `0x10` (typically `SWI 0x11` / OS_Exit) terminates execution. This is a safety net; well-behaved programs exit directly and never reach this instruction.

---

## 9. Re-Startability

An AIF image is re-startable if and only if the program it contains is re-startable. An AIF image is **not** reentrant.

For an image to be re-startable:

1. After decompression: the decompression code must reset the first header word (offset `0x00`) to NOP, so that re-entry does not attempt to decompress an already-decompressed image.
2. After self-relocation: the self-relocation code must reset the second header word (offset `0x04`) to NOP, so that re-entry does not attempt to relocate an already-relocated image.

On systems with memory protection, the decompression and self-relocation code must bracket their writes to the header with system calls to change the read-only code section to writable, then back to read-only. This is not an additional burden, since both decompression and relocation must already write to the code area.

---

## 10. Self-Relocation

### 10.1 Types of Self-Relocation

AIF supports two types of self-relocation, and a third type exists for RISC OS Relocatable Modules (RMF/AMF):

| Type | Scope | Description |
|------|-------|-------------|
| **Relocate to Load Address** | AIF | One-time position independence. The image can be loaded at any address and will execute where loaded. After relocation, the relocation table is overwritten by zero-initialised data, heap, or stack, so relocation can only occur once. |
| **Self-Move Up Memory** | AIF | The image copies itself from its load address to the highest address that leaves the required workspace free (workspace size specified at offset `0x2C`), then relocates to that address. Requires an OS call that returns the top of available memory. |
| **Many-Time Position Independence** | RMF (not AIF) | Relocatable Modules can be shut down, moved in memory, and restarted. Their relocation tables are never overwritten, allowing relocation to be performed multiple times. |

### 10.2 Why Three Types Exist

1. RISC OS applications acquire position-dependence during execution (data pointers become absolute). Once running, an application cannot be moved in memory because not all position-dependent data pointers can be found and updated. Therefore AIF images support only one-time relocation.

2. RISC OS Relocatable Modules are required by the OS to be relocatable at any time. They receive service calls for shutdown and startup, and their relocation tables must persist for repeated use.

3. Relocatable Modules are managed by the RMA (Relocatable Module Area) manager, which controls loading and moving. Applications, in contrast, are loaded at their load address and are on their own until they exit or fault.

### 10.3 Self-Relocation Mechanism (1993+ / 26/32-bit Compatible)

The self-relocation code is appended to the end of an AIF image by the linker, immediately before the relocation list. It is entered via the `BL` instruction at offset `0x04` of the header, so on entry `r14` points to `AIFHeader + 8`. In 26-bit ARM modes, `r14` also contains a copy of the PSR flags.

#### Phase 1: Calculate Header Address

The code computes the actual address of the AIF header in a CPU-mode-independent manner using the `SUB ip, lr, pc` / `ADD ip, pc, ip` / `SUB ip, ip, #12` sequence.

```arm
RelocCode
    NOP                            ; used by ensure_byte_order() and as
                                   ; the NOP value to store into header
    SUB    ip, lr, pc              ; base+8+[PSR]-(RelocCode+12+[PSR])
                                   ; = base-4-RelocCode
    ADD    ip, pc, ip              ; base-4-RelocCode+RelocCode+16 = base+12
    SUB    ip, ip, #12             ; -> header address
    LDR    r0, RelocCode           ; load the NOP instruction
    STR    r0, [ip, #4]            ; overwrite BL SelfRelocCode with NOP
```

#### Phase 2: Determine Self-Move Requirement

```arm
    LDR    r9, [ip, #0x2C]        ; min free space requirement (Work Space)
    CMPS   r9, #0                  ; 0 => no move, just relocate in place
    BEQ    RelocateOnly
```

#### Phase 3: Self-Move (if required)

If the workspace requirement is non-zero, the image calculates the move distance:

```arm
    LDR    r0, [ip, #0x20]        ; image zero-init size
    ADD    r9, r9, r0              ; space to leave = workspace + zero-init
    SWI    #0x10                   ; system-specific: return top of memory in r1
```

It then calculates the image length (including relocation data) by scanning through the relocation list to find the terminator:

```arm
    ADR    r2, End                 ; -> end of relocation code
01  LDR    r0, [r2], #4           ; load relocation entry, advance r2
    CMNS   r0, #1                  ; is it the -1 terminator?
    BNE    %B01                    ; no, keep scanning
    SUB    r3, r1, r9              ; target address = MemLimit - freeSpace
    SUBS   r0, r3, r2              ; amount to move = target - current end
    BLE    RelocateOnly            ; not enough space to move
    BIC    r0, r0, #15             ; round down to multiple of 16
```

The image copies itself upward in memory four words (16 bytes) at a time, in descending address order, jumping to the copied copy-loop as soon as it has been copied:

```arm
    ADD    r3, r2, r0              ; destination end = source end + shift
    ADR    r8, %F02                ; intermediate limit for copy-up
02  LDMDB  r2!, {r4-r7}           ; copy 4 words downward from source
    STMDB  r3!, {r4-r7}           ; write 4 words downward to destination
    CMPS   r2, r8                  ; copied the copy loop itself?
    BGT    %B02                    ; not yet
    ADD    r4, pc, r0
    MOV    pc, r4                  ; jump to the COPIED copy code
03  LDMDB  r2!, {r4-r7}           ; continue copying from the new location
    STMDB  r3!, {r4-r7}
    CMPS   r2, ip                  ; copied everything?
    BGT    %B03
    ADD    ip, ip, r0              ; update header address to new location
    ADD    lr, lr, r0              ; update return address to new location
```

#### Phase 4: Relocation Processing

Whether the image has moved itself or not, control arrives here. Each word in the relocation list is an offset from the image base; the word at that offset is adjusted by the difference between the actual load address and the linked address:

```arm
RelocateOnly
    LDR    r1, [ip, #0x28]        ; original link base address
    SUBS   r1, ip, r1              ; relocation delta = actual - linked
    MOVEQ  pc, lr                  ; delta is 0, nothing to do
    STR    ip, [ip, #0x28]        ; update image base to actual address
    ADR    r2, End                 ; start of relocation list
04  LDR    r0, [r2], #4           ; offset of word to relocate
    CMNS   r0, #1                  ; -1 terminator?
    MOVEQ  pc, lr                  ; yes => return
    LDR    r3, [ip, r0]           ; read word at [image_base + offset]
    ADD    r3, r3, r1              ; add relocation delta
    STR    r3, [ip, r0]           ; write back relocated word
    B      %B04                    ; process next entry
End                                ; relocation list starts here,
                                   ; terminated by -1
```

### 10.4 Self-Relocation Code (1989 / 26-bit Only)

The 1989 version uses `BIC` to mask PSR bits from `r14` rather than the `SUB/ADD` pair:

```arm
RelocCode
    BIC     IP, LR, #0xFC000003   ; clear PSR flag bits; -> AIF header + 0x08
    SUB     IP, IP, #8             ; -> header address
    MOV     R0, #0xFB000000        ; BLNV #0 (the 1989-era NOP)
    STR     R0, [IP, #4]           ; overwrite BL SelfRelocCode
    ; ... remainder is structurally identical ...
```

This only works in 26-bit ARM mode where `r14` contains PSR bits in its top 6 bits.

### 10.5 Self-Relocation Code for Relocatable Modules (RMF)

For completeness: Relocatable Modules do not use an AIF header. Their relocation code must be executable many times and symbolically addressable from the RM header. The relocation code and list are appended as the last area of the image.

```arm
    IMPORT  |Image$$RO$$Base|       ; where the image was linked at
    EXPORT  |__RelocCode|           ; referenced from the RM header

|__RelocCode|
    LDR     R1, RelocCode           ; value of __RelocCode before relocation
    SUB     IP, PC, #12             ; value of __RelocCode now
    SUBS    R1, IP, R1              ; relocation offset
    MOVEQS  PC, LR                  ; offset is 0, nothing to do
    LDR     IP, ImageBase           ; image base prior to relocation
    ADD     IP, IP, R1              ; actual image base
    ADR     R2, End
RelocLoop
    LDR     R0, [R2], #4
    CMNS    R0, #1                  ; -1 terminator?
    MOVLES  PC, LR                  ; yes => return
    LDR     R3, [IP, R0]           ; word to relocate
    ADD     R3, R3, R1              ; add delta
    STR     R3, [IP, R0]           ; store back
    B       RelocLoop
RelocCode   DCD    |__RelocCode|
ImageBase   DCD    |Image$$RO$$Base|
End                                 ; relocation list follows, terminated by -1
```

Note that address values used within this code (e.g., `|__RelocCode|`) appear in the relocation list themselves, allowing the code to be re-executed after a subsequent move.

### 10.6 Customisation

The self-relocation and self-move code generated by `armlink` can be customised by providing replacement code in an area called `AIF_RELOC` in the **first** object file in the linker input list.

The AIF header itself can be customised by providing a template in an area called `AIF_HDR` in the **first** object file in the linker input list.

---

## 11. Zero-Initialisation Code

The zero-init code occupies offsets `0x40`-`0x7F` of the header (16 words total, including the Debug Init Instruction). It is entered via the `BL` instruction at offset `0x08`.

### 11.1 Algorithm

1. Optionally execute a Debug Init Instruction (at `0x40`), or NOP
2. Compute the image base address position-independently from `lr` and `pc`
3. Load the RO size, RW size, debug size, and zero-init size from the header
4. Compute the base of the zero-init area: `ImageBase + ROSize + RWSize`
5. Zero the area in 16-byte (4-word) blocks using `STMIA`
6. Return via `lr`

### 11.2 1989 (26-bit only) Zero-Init Code

Uses `BIC` to strip PSR bits from `r14`:

```arm
    BIC     IP, LR, #0xFC000003  ; clear PSR bits -> header + 0x0C
    ADD     IP, IP, #8           ; -> Image ReadOnly size (offset 0x14)
    LDMIA   IP, {R0,R1,R2,R3}   ; load RO, RW, Debug, ZI sizes
    CMPS    R3, #0
    MOVLES  PC, LR               ; nothing to do
    SUB     IP, IP, #0x14        ; -> image base
    ADD     IP, IP, R0           ; + RO size
    ADD     IP, IP, R1           ; + RW size = base of zero-init area
    MOV     R0, #0
    MOV     R1, #0
    MOV     R2, #0
    MOV     R4, #0
ZeroLoop
    STMIA   IP!, {R0,R1,R2,R4}
    SUBS    R3, R3, #16
    BGT     ZeroLoop
    MOVS    PC, LR               ; 16 words total
```

### 11.3 1993+ (26/32-bit Compatible) Zero-Init Code

Uses `SUB/ADD` pair for position-independent address calculation that works in both 26-bit and 32-bit modes:

```arm
ZeroInit
    NOP                            ; or <Debug Init Instruction>
    SUB    ip, lr, pc              ; base+12+[PSR]-(ZeroInit+12+[PSR])
                                   ; = base-ZeroInit
    ADD    ip, pc, ip              ; base-ZeroInit+ZeroInit+16 = base+16
    LDMIB  ip, {r0,r1,r2,r4}     ; load RO, RW, Debug, ZI sizes
    SUB    ip, ip, #16             ; -> image base
    ADD    ip, ip, r0              ; + RO size
    ADD    ip, ip, r1              ; + RW size = base of zero-init area
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMPS   r4, #0
00  MOVLE  pc, lr                  ; nothing left to do
    STMIA  ip!, {r0,r1,r2,r3}    ; zero 16 bytes at a time
    SUBS   r4, r4, #16
    B      %B00
```

Key differences from the 1989 version:
- Uses `SUB ip, lr, pc` / `ADD ip, pc, ip` instead of `BIC` — works in both 26-bit and 32-bit ARM modes
- Uses `LDMIB` instead of `LDMIA` (loads from `ip+4` rather than `ip`)
- The Debug Init Instruction is separated into its own word at offset `0x40`
- Tests zero-init size (`r4`) after the zeroing loop setup rather than before

---

## 12. Debugging Support

AIF supports debugging by the ARM Symbolic Debugger (armsd/ASD). Low-level and source-level debugging are orthogonal; both, either, or neither may be present in an AIF image.

### 12.1 Debugging Data Properties

- **References from debugging tables to code/data** are stored as relocatable addresses. After loading the image at its load address, these values are effectively absolute.
- **References between debugging table entries** are stored as offsets from the beginning of the debugging data area. This makes the debugging data area position-independent after image relocation, allowing a debugger to copy or move it freely.

### 12.2 Debugger Interception Mechanism

A debugger intercepts execution by:

1. Copying the third word of the AIF header (offset `0x08`, the `BL ZeroInit` instruction)
2. Replacing it with a `BL` to the debugger's own initialisation code
3. When entered, the debugger copies the debugging data to a safe location (before it is overwritten by zero-init data, heap, or stack)
4. Restores the original `BL ZeroInit` instruction and allows execution to continue

### 12.3 Debug Initialisation Instruction

The word at offset `0x40` (the first word of the zero-init code block) is the Debug Initialisation Instruction. If used, it is expected to be a SWI instruction which alerts a resident debugger that a debuggable image is commencing execution. If unused, it is NOP.

The ARM cross-linker sets this field to NOP by default. It can be customised by providing a template in an `AIF_HDR` area.

---

## 13. C Structure Representation

```c
typedef struct {
    uint32_t BL_decompress_code;   /* 0x00: BL to decompress code, or NOP */
    uint32_t BL_selfreloc_code;    /* 0x04: BL to self-reloc code, or NOP */
    uint32_t BL_zeroinit_code;     /* 0x08: BL to zero-init code, or NOP */
    uint32_t BL_imageentrypoint;   /* 0x0C: BL to entry point (exec), or offset (non-exec) */
    uint32_t program_exit;         /* 0x10: SWI OS_Exit or branch-to-self */
    uint32_t size_ro;              /* 0x14: Read-only area size (incl header if exec AIF) */
    uint32_t size_rw;              /* 0x18: Read-write area size */
    uint32_t size_debug;           /* 0x1C: Debug area size */
    uint32_t size_zeroinit;        /* 0x20: Zero-init area size */
    uint32_t debug_type;           /* 0x24: Debug type (0-3) */
    uint32_t image_base;           /* 0x28: Linked base address */
    uint32_t workspace;            /* 0x2C: Min workspace for self-move (obsolete) */
    uint32_t address_mode;         /* 0x30: 26/32 mode + flags */
    uint32_t data_base;            /* 0x34: Data base address (if separate) */
    uint32_t reserved[2];          /* 0x38-0x3C: Reserved / Extended AIF region offset */
    uint32_t debug_init;           /* 0x40: Debug init instruction or NOP */
    uint32_t zeroinit_code[15];    /* 0x44-0x7F: Zero-init routine (15 words) */
} AIF_Header;                      /* Total: 128 bytes (32 words) */
```

---

## 14. Version History

| Era | Designation | Key Characteristics |
|-----|-------------|---------------------|
| 1987 | Arthur Image Format v0.03 | Original design for Arthur/RISC OS on Archimedes. 26-bit only. `BLNV 0` for NOP. Four reserved words at `0x30`-`0x3F`. |
| 1989 | RISC OS AIF v1.00 (PLG-AIF) | Major revision by Lee Smith and Lionel Haines. Improved relocation support (both one-time and self-move). ASD debugging support. Work Space field at `0x2C`. PSR bit masking with `BIC`. |
| ~1993 | ARM Image Format (ARM SDT) | 26/32-bit compatible. `MOV r0,r0` for NOP. Address Mode field at `0x30`. Data Base field at `0x34`. Position-independent zero-init via `SUB/ADD` pair. Debug Init instruction separated at `0x40`. |
| ~1997 | ARM SDT 2.50 (DUI0041C) | Extended AIF for scatter-loaded images. Work Space field marked obsolete. Three named AIF variants (Executable, Non-executable, Extended). ELF increasingly preferred. |

---

## 15. References

- PLG-AIF: "RISC OS Application Image Format", Acorn Computers Limited, Issue 1.00, 1989 (Lee Smith, Lionel Haines)
- ARM SDT Reference Guide, ARM DUI 0041C, Chapter 13: "ARM Image Format", ARM Limited, 1997-1998
- RISC OS Programmer's Reference Manuals, Volume 4, Appendix D: "Code file formats"
- ARM Technical Specifications (3DO edition), Chapter 1: "ARM Image Format"
- "Introduction to the ARM AIF Object File Format", Paolo Fabio Zaino, 2020
