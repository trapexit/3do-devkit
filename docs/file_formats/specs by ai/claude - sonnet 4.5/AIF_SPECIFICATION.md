# ARM Image Format (AIF) - Complete File Format Specification

## Document Information

- **Format Name**: ARM Image Format (AIF) / RISC OS Application Image Format
- **File Extension**: Typically none, or .aif
- **Byte Order**: Little-endian (ARM standard)
- **Version**: This specification covers versions from 1989 through 1993+
- **Copyright**: Based on Acorn Computers Limited documentation (1989-1998)

---

## 1. Overview

ARM Image Format (AIF) is a simple executable image format for ARM microprocessors. It consists of a 128-byte header followed by the executable code and initialized static data. AIF was introduced by Acorn Computers Ltd. for use on ARM-based systems including the Archimedes, RiscPC, and RISC OS computers, and was also adopted by the 3DO console.

### 1.1 Key Properties

1. **Self-Contained Execution**: An AIF image is loaded into memory at its load address and entered at its first word
2. **Optional Compression**: Images may be compressed with self-decompression capability
3. **Self-Relocation**: Images can relocate themselves in two distinct ways:
   - **One-time Position-Independence**: Load at any address and execute there
   - **Specified Working Space Relocation**: Move to high memory leaving requested workspace
4. **Debug Support**: Supports ARM Symbolic Debugger (ASD) at both low-level and source-level
5. **Zero-Initialization**: Can create and initialize its own zero-initialized data area

---

## 2. AIF Variants

There are three distinct variants of AIF:

### 2.1 Executable AIF

- **Most Common Variant**: Used for RISC OS application `!RunImage` files
- **Header is Part of Image**: Code in header prepares image for execution
- **Load Address**: Header loaded at base address; code starts at `base + 0x80`
- **Entry Point**: Entered at first word of header
- **Fourth Word Signature**: `BL entrypoint` - most significant byte is `0xEB`
- **Self-Preparation**: Can relocate itself and create zero-initialized area
- **Base Address**: On RISC OS, typically `0x8000` (virtual address via MMU)

### 2.2 Non-Executable AIF

- **Loader Required**: Must be processed by an image loader
- **Header Not Part of Image**: Header describes but is separate from image
- **Discarded After Load**: Header is discarded after loader prepares image
- **Fourth Word Signature**: Offset to entry point - most significant nibble is `0x0`
- **Base Address**: Where code should be loaded (not header)

### 2.3 Extended AIF

- **Special Type**: Non-executable AIF variant
- **Scatter-Loaded Image**: Contains multiple load regions
- **Descriptor Chain**: Header points to chain of load region descriptors
- **Reserved Word**: Word at offset `0x38` is non-zero, containing file offset to first non-root region header

---

## 3. Image Layout

### 3.1 Compressed AIF Image Layout

```
+---------------------------+
| Header (128 bytes)        |  NOT compressed
+---------------------------+
| Compressed Image          |
+---------------------------+
| Decompression Data        |  Position-independent
+---------------------------+
| Decompression Code        |  Position-independent
+---------------------------+
```

### 3.2 Uncompressed/Decompressed AIF Image Layout

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only Area            |
+---------------------------+
| Read-Write Area           |
+---------------------------+
| Debugging Data            |  Optional
+---------------------------+
| Self-Relocation Code      |  Position-independent
+---------------------------+
| Relocation List           |  Terminated by -1
+---------------------------+
```

### 3.3 Post-Relocation Layout

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only Area            |
+---------------------------+
| Read-Write Area           |
+---------------------------+
| Debugging Data            |  Optional, should be copied by debugger
+---------------------------+
```

Note: After relocation, debugging data should be copied by the debugger before being overwritten by zero-initialized data, heap, or stack.

---

## 4. AIF Header Structure

The AIF header is exactly **128 bytes (32 words)** and has the following structure:

### 4.1 Complete Header Layout

| Offset | Size | Field Name | Description |
|--------|------|------------|-------------|
| 0x00 | 4 bytes | BL DecompressCode / NOP | Branch to decompression code, or NOP (0xEA000000 or MOV r0,r0) if not compressed |
| 0x04 | 4 bytes | BL SelfRelocCode / NOP | Branch to self-relocation code, or NOP if not self-relocating |
| 0x08 | 4 bytes | BL ZeroInit / NOP | Branch to zero-initialization code, or NOP if none |
| 0x0C | 4 bytes | BL ImageEntryPoint / Offset | Executable: BL to entry point; Non-executable: offset to entry point |
| 0x10 | 4 bytes | Program Exit Instruction | Last resort exit (typically SWI 0x11 / OS_Exit) |
| 0x14 | 4 bytes | Image ReadOnly Size | Size of RO area; includes header size for executable AIF only |
| 0x18 | 4 bytes | Image ReadWrite Size | Exact size of RW area (multiple of 4) |
| 0x1C | 4 bytes | Image Debug Size | Exact size of debug area (multiple of 4) |
| 0x20 | 4 bytes | Image Zero-Init Size | Exact size of zero-init area (multiple of 4) |
| 0x24 | 4 bytes | Image Debug Type | 0=None, 1=Low-level, 2=Source-level, 3=Both |
| 0x28 | 4 bytes | Image Base | Address where image was linked |
| 0x2C | 4 bytes | Work Space | Min workspace for self-moving image (obsolete in 1990s) |
| 0x30 | 4 bytes | Address Mode | 0, 26, or 32; bit 8 set = separate data base |
| 0x34 | 4 bytes | Data Base | Address where image data was linked |
| 0x38 | 8 bytes | Reserved (2 words) | For Extended AIF: offset to first region header |
| 0x40 | 4 bytes | Debug Init / NOP | Debug initialization instruction or NOP |
| 0x44 | 60 bytes | Zero-Init Code | 15 words of zero-initialization code |

**Total Size**: 128 bytes (0x80 bytes)

### 4.2 Field Descriptions

#### Offset 0x00: BL DecompressCode / NOP

- **Purpose**: Branch to decompression code if image is compressed
- **Compressed Image**: `BL` instruction to decompression code at end of image
- **Uncompressed Image**: `NOP` encoded as `MOV r0, r0` (0xE1A00000) or `BLNV 0` (0xFB000000)
- **Re-entry Requirement**: After decompression, this word must be reset to NOP for re-entrability

#### Offset 0x04: BL SelfRelocCode / NOP

- **Purpose**: Branch to self-relocation code if image is relocatable
- **Relocatable Image**: `BL` instruction to relocation code at end of image
- **Non-Relocatable**: `NOP` encoded as `MOV r0, r0` or `BLNV 0`
- **Re-entry Requirement**: After relocation, this word must be reset to NOP

#### Offset 0x08: BL ZeroInit / NOP

- **Purpose**: Branch to zero-initialization code
- **With Zero-Init**: `BL` instruction to code at offset 0x40 in header
- **No Zero-Init**: `NOP`
- **Addressability**: BL makes header addressable via R14

#### Offset 0x0C: BL ImageEntryPoint / EntryPoint Offset

- **Executable AIF**: `BL` instruction to actual entry point of program
- **Non-Executable AIF**: Offset (word value) from image base to entry point
- **Detection**: MSB = 0xEB for executable, MSB nibble = 0x0 for non-executable
- **Purpose**: Makes header addressable via R14 in position-independent manner
- **Return Warning**: Application should NEVER return; exit instruction at 0x10 is failsafe

#### Offset 0x10: Program Exit Instruction

- **Typical Value**: `SWI 0x11` (OS_Exit on RISC OS)
- **Alternative**: Branch-to-self on systems without exit SWI
- **Purpose**: Last-ditch exit if program incorrectly returns
- **Customization**: Can be set via AIF_HDR area in first object file to linker

#### Offset 0x14: Image ReadOnly Size

- **Executable AIF**: Includes header size (128 bytes) plus RO code/data
- **Non-Executable AIF**: Excludes header size (code/data only)
- **Includes**: Any padding to maintain alignment
- **Multiple**: Must be multiple of 4 bytes

#### Offset 0x18: Image ReadWrite Size

- **Content**: Size of initialized read-write data
- **Exact Size**: Precise byte count (multiple of 4)
- **Location**: Follows read-only area in memory

#### Offset 0x1C: Image Debug Size

- **Content**: Size of debugging data (low-level and/or source-level)
- **Exact Size**: Multiple of 4 bytes
- **Bit Encoding** (some versions):
  - Bits 0-3: Debug type
  - Bits 4-31: Low-level debug size

#### Offset 0x20: Image Zero-Init Size

- **Content**: Size of area to be zero-initialized at runtime
- **Exact Size**: Multiple of 4 bytes (typically multiple of 16 for efficiency)
- **Purpose**: BSS segment - uninitialized/zero-initialized variables

#### Offset 0x24: Image Debug Type

| Value | Meaning |
|-------|---------|
| 0 | No debugging data present |
| 1 | Low-level debugging data present |
| 2 | Source-level (ASD) debugging data present |
| 3 | Both low-level and source-level present |

All other values reserved to ARM/Acorn.

#### Offset 0x28: Image Base

- **Content**: Address at which image was linked
- **Set By**: Linker at link time
- **Used For**: Calculating relocation offset
- **Updated**: Set to actual load address after relocation

#### Offset 0x2C: Work Space

- **Content**: Minimum workspace in bytes for self-moving relocatable image
- **Obsolete**: As of 1990s, this field is largely unused
- **Original Purpose**: Reserved space above image after self-move

#### Offset 0x30: Address Mode

- **Byte 0 (LSB)**: Contains 0, 26, or 32
  - **0**: Old-style 26-bit AIF header
  - **26**: Image linked for 26-bit ARM mode (obsolete)
  - **32**: Image linked for 32-bit ARM mode
- **Bit 8**: If set, separate code and data bases used
- **Bytes 1-3**: Flag bytes (usage varies)

#### Offset 0x34: Data Base

- **Content**: Address where image data was linked
- **Valid When**: Bit 8 of Address Mode word is set
- **Purpose**: Support for separate data base in memory layout

#### Offset 0x38-0x3C: Reserved Words

- **Standard AIF**: Two words, must be initially 0
- **Extended AIF**: Word at 0x38 is non-zero, contains byte offset to first non-root load region header

#### Offset 0x40: Debug Init Instruction / NOP

- **Typical**: NOP (unused)
- **If Used**: SWI instruction to alert resident debugger
- **Purpose**: Signal debugger that debuggable image is starting
- **Customization**: Can be set via AIF_HDR area template
- **RISC OS**: Not required for debuggers; typically left as NOP

#### Offset 0x44-0x7C: Zero-Init Code

- **Size**: 60 bytes (15 words)
- **Purpose**: Standard code to zero-initialize BSS segment
- **Position-Independent**: Must work regardless of load address
- **Added By**: Linker when creating AIF image

---

## 5. Zero-Initialization Code

The zero-initialization code is standardized and typically 15 words (60 bytes) at offset 0x44 in the header. It initializes the zero-init area (BSS) to all zeros.

### 5.1 Standard Zero-Init Code

```arm
ZeroInit:
        NOP                       ; or Debug Init Instruction
        SUB    ip, lr, pc         ; Calculate base address offset
                                  ; base+12+[PSR]-(ZeroInit+12+[PSR])
                                  ; = base-ZeroInit
        ADD    ip, pc, ip         ; base-ZeroInit+ZeroInit+16 = base+16
        LDMIB  ip, {r0,r1,r2,r4}  ; Load various sizes from header
        SUB    ip, ip, #16        ; -> image base
        ADD    ip, ip, r0         ; + RO size
        ADD    ip, ip, r1         ; + RW size = base of 0-init area
        MOV    r0, #0
        MOV    r1, #0
        MOV    r2, #0
        MOV    r3, #0
        CMPS   r4, #0
00:     MOVLE  pc, lr             ; Return if nothing left to do
        STMIA  ip!, {r0,r1,r2,r3} ; Zero 16 bytes
        SUBS   r4, r4, #16
        B      %B00                ; Loop until done
```

### 5.2 Alternative Zero-Init Code (1989 Version)

```arm
        BIC    IP, LR, #&FC000003  ; Clear status bits -> header + &C
        ADD    IP, IP, #8          ; -> Image ReadOnly size
        LDMIA  IP, {R0,R1,R2,R3}   ; Load various sizes
        CMPS   R3, #0
        MOVLES PC, LR              ; Nothing to do
        SUB    IP, IP, #&14        ; -> image base
        ADD    IP, IP, R0          ; + RO size
        ADD    IP, IP, R1          ; + RW size = base of 0-init area
        MOV    R0, #0
        MOV    R1, #0
        MOV    R2, #0
        MOV    R4, #0
ZeroLoop:
        STMIA  IP!, {R0,R1,R2,R4}
        SUBS   R3, R3, #16
        BGT    ZeroLoop
        MOVS   PC, LR              ; 16 words total
```

### 5.3 Zero-Init Code Operation

1. **Calculate Base Address**: Use position-independent code to find header location
2. **Load Size Fields**: Read RO size, RW size, debug size, and zero-init size from header
3. **Calculate Zero-Init Base**: Base + RO size + RW size
4. **Zero Memory**: Store zeros in 16-byte chunks
5. **Loop**: Continue until entire zero-init area is cleared
6. **Return**: Via `MOV pc, lr` or `MOVS pc, lr`

---

## 6. Self-Relocation

AIF supports multiple forms of self-relocation for different use cases.

### 6.1 Relocation Types

#### 6.1.1 One-Time Position Independence (AIF Images)

- **Use Case**: Applications that can load anywhere
- **Frequency**: Relocate once, then execute
- **Limitation**: Cannot be moved after execution starts (acquired position-dependence)
- **Relocation Table**: Overwritten after use (by zero-init data, heap, or stack)

#### 6.1.2 Many-Time Position Independence (RMF Modules)

- **Use Case**: RISC OS Relocatable Modules
- **Frequency**: Can be relocated multiple times
- **Requirement**: Module can shut down, move, and restart
- **Relocation Table**: Preserved, not overwritten

#### 6.1.3 Self-Move and Relocate (AIF Only)

- **Use Case**: Applications needing specific workspace
- **Operation**: Load at low address, move to high address, leaving workspace
- **Requirement**: System call to get top of memory (e.g., SWI GetEnv)
- **Customization**: Linker allows modified self-move code via AIF_RELOC area

### 6.2 Relocation List Format

The relocation list immediately follows the self-relocation code at the end of the image.

- **Entry Format**: Word (4 bytes)
- **Entry Value**: Byte offset from beginning of AIF header to word to be relocated
- **Terminator**: -1 (0xFFFFFFFF)
- **Restriction**: Only word-aligned values supported
- **Processing**: Each listed word is adjusted by (actual_base - linked_base)

Example:
```
+---------------------------+
| 0x00000084                |  <- Offset of first word to relocate
| 0x00000098                |  <- Offset of second word
| 0x000000A4                |  <- Offset of third word
| 0xFFFFFFFF                |  <- Terminator (-1)
+---------------------------+
```

### 6.3 Self-Move and Self-Relocation Code for AIF

This code is added by the linker immediately before the relocation list.

```arm
RelocCode:
        NOP                         ; Required by ensure_byte_order()
        SUB    ip, lr, pc           ; base+8+[PSR]-(RelocCode+12+[PSR])
                                    ; = base-4-RelocCode
        ADD    ip, pc, ip           ; base-4-RelocCode+RelocCode+16 = base+12
        SUB    ip, ip, #12          ; -> header address
        LDR    r0, RelocCode        ; Load NOP instruction
        STR    r0, [ip, #4]         ; Won't be called again on re-entry
        LDR    r9, [ip, #&2C]       ; Min free space requirement
        CMPS   r9, #0               ; 0 => no move, just relocate
        BEQ    RelocateOnly

; Calculate amount to move by
        LDR    r0, [ip, #&20]       ; Image zero-init size
        ADD    r9, r9, r0           ; Space to leave = min free + zero init
        SWI    #&10                 ; Get top of memory in r1 (system-specific)
        ADR    r2, End              ; -> End
01:     LDR    r0, [r2], #4         ; Load relocation offset
        CMNS   r0, #1               ; Terminator?
        BNE    %B01                 ; Loop to find end
        SUB    r3, r1, r9           ; MemLimit - freeSpace
        SUBS   r0, r3, r2           ; Amount to move by
        BLE    RelocateOnly         ; Not enough space
        BIC    r0, r0, #15          ; Multiple of 16
        ADD    r3, r2, r0           ; End + shift
        ADR    r8, %F02             ; Intermediate limit

; Copy image up memory
02:     LDMDB  r2!, {r4-r7}
        STMDB  r3!, {r4-r7}
        CMPS   r2, r8               ; Copied the copy loop?
        BGT    %B02                 ; Not yet
        ADD    r4, pc, r0
        MOV    pc, r4               ; Jump to copied copy code
03:     LDMDB  r2!, {r4-r7}
        STMDB  r3!, {r4-r7}
        CMPS   r2, ip               ; Copied everything?
        BGT    %B03                 ; Not yet
        ADD    ip, ip, r0           ; Relocated header address
        ADD    lr, lr, r0           ; Relocated return address

; Relocate addresses
RelocateOnly:
        LDR    r1, [ip, #&28]       ; Code base set by linker
        SUBS   r1, ip, r1           ; Relocation offset
        MOVEQ  pc, lr               ; No relocation needed
        STR    ip, [ip, #&28]       ; New image base
        ADR    r2, End              ; Start of reloc list
04:     LDR    r0, [r2], #4         ; Offset of word to relocate
        CMNS   r0, #1               ; Terminator?
        MOVEQ  pc, lr               ; Done
        LDR    r3, [ip, r0]         ; Load word to relocate
        ADD    r3, r3, r1           ; Add relocation offset
        STR    r3, [ip, r0]         ; Store relocated word
        B      %B04                 ; Next word
End:                                ; Relocation list starts here
```

### 6.4 Self-Relocation Code for RMF Modules

For Relocatable Modules, code is different as it must be reusable:

```arm
        IMPORT  |Image$$RO$$Base|   ; Linked base address
        EXPORT  |__RelocCode|       ; Referenced from RM header

|__RelocCode|:
        LDR    R1, RelocCode        ; Value before relocation
        SUB    IP, PC, #12          ; Current value
        SUBS   R1, IP, R1           ; Relocation offset
        MOVEQS PC, LR               ; No relocation needed
        LDR    IP, ImageBase        ; Image base before relocation
        ADD    IP, IP, R1           ; Actual image location
        ADR    R2, End
RelocLoop:
        LDR    R0, [R2], #4         ; Load offset
        CMNS   R0, #1               ; Terminator?
        MOVLES PC, LR               ; Done
        LDR    R3, [IP, R0]         ; Load word to relocate
        ADD    R3, R3, R1           ; Add offset
        STR    R3, [IP, R0]         ; Store back
        B      RelocLoop
RelocCode:   DCD    |__RelocCode|
ImageBase:   DCD    |Image$$RO$$Base|
End:                                ; Relocation list follows
```

---

## 7. Debugging Data

### 7.1 Debug Data Properties

- **Location**: Follows read-write area in image
- **Relocatable**: References to code/data are relocatable addresses
- **Position-Independent**: After relocation, debug data is position-independent
- **Internal References**: Use offsets from debug area base
- **Debugger Action**: Should copy debug data before it's overwritten by zero-init/heap/stack

### 7.2 Debug Types

| Type | Description |
|------|-------------|
| Low-level | Assembly-level debugging (formerly Dbug) |
| Source-level | High-level language debugging (ASD) |
| Combined | Both low-level and source-level |

### 7.3 Debugger Integration

A debugger can seize control at the appropriate moment by:
1. Copying the third word of the AIF header (BL ZeroInit)
2. Modifying it to branch to debugger initialization
3. Copying debug data to safe location
4. Restoring original header word

---

## 8. Extended AIF Region Header Format

For Extended AIF (scatter-loaded images), each region has a 44-byte header:

| Word | Offset | Size | Field | Description |
|------|--------|------|-------|-------------|
| 0 | 0x00 | 4 | Next Region Offset | File offset to next region header (0 if last) |
| 1 | 0x04 | 4 | Load Address | Address where this region should be loaded |
| 2 | 0x08 | 4 | Size | Size of region in bytes (multiple of 4) |
| 3-10 | 0x0C | 32 | Region Name | ASCII name, zero-padded |

The initializing data for the region follows immediately after the 44-byte header.

---

## 9. Memory Layout Relationships

### 9.1 Linker Pre-defined Symbols

The AIF header fields relate to linker symbols as follows:

```
AIFHeader.ImageBase = Image$$RO$$Base

AIFHeader.ImageBase + AIFHeader.ROSize = Image$$RW$$Base

AIFHeader.ImageBase + AIFHeader.ROSize + AIFHeader.RWSize = Image$$ZI$$Base

AIFHeader.ImageBase + AIFHeader.ROSize + AIFHeader.RWSize + AIFHeader.ZeroInitSize = Image$$RW$$Limit
```

### 9.2 Runtime Memory Map

For an executable AIF loaded at `0x8000` on RISC OS:

```
0x8000      +---------------------------+
            | AIF Header (128 bytes)    |
0x8080      +---------------------------+
            | Read-Only Code/Data       |
0x8000+RO   +---------------------------+
            | Read-Write Data           |
0x8000+RO+RW+---------------------------+
            | Debugging Data (optional) |
0x8000+RO+RW+DBG+----------------------+
            | Self-Relocation Code      |
            +---------------------------+
            | Relocation List           |
            +---------------------------+
            | Zero-Init Area (BSS)      |
            +---------------------------+
            | Heap                      |
            +---------------------------+
            | Stack                     |
            +---------------------------+
```

---

## 10. Re-entrability and Restartability

### 10.1 Re-entry Requirements

An AIF image is re-enterable at its first instruction if:

1. **After Decompression**: First word reset to NOP
2. **After Relocation**: Second word reset to NOP
3. **Program Design**: The program itself supports restarting

Note: AIF images are **not** reentrant (multiple simultaneous instances), only restartable.

### 10.2 Memory Protection Considerations

On systems with memory protection:

1. **Decompression Code**: Must be bracketed by calls to:
   - Make RO section writable
   - Reset first word to NOP
   - Restore RO protection

2. **Relocation Code**: Must be bracketed by calls to:
   - Make RO section writable
   - Reset second word to NOP
   - Update relocation targets
   - Restore RO protection

---

## 11. Customization

### 11.1 Custom AIF Header

To customize the AIF header (exit instruction, debug init):

1. Create an area named `AIF_HDR` in assembly
2. Define custom header template
3. Place in **first** object file in linker input list
4. Linker will use this template instead of default

### 11.2 Custom Relocation Code

To customize self-relocation/self-move code:

1. Create an area named `AIF_RELOC` in assembly
2. Define custom relocation code
3. Place in **first** object file in linker input list
4. Linker will use this code instead of default

Example customization points:
- Different SWI for getting top of memory
- Different memory management strategy
- Different alignment requirements
- Additional initialization steps

---

## 12. Version History and Compatibility

### 12.1 Version Identification

- **File Type Word** (offset 0x00 in file, not header): Often identifies AIF
- **Version Field**: Some formats include version identifier (e.g., 310 = 0x136)
- **Address Mode Word** (offset 0x30): 0 indicates old 26-bit header

### 12.2 Historical Versions

| Version | Date | Key Changes |
|---------|------|-------------|
| 0.03 | 1987 | Early version with basic relocation |
| 0.04 | 1989 | Added symbolic assembler debugging support |
| 1.00 | 1989 | Major revision: improved relocation, ASD/Dbug support |
| 3.00 | ~1993 | Added 32-bit mode support, separate data base |

### 12.3 Compatibility Notes

- **26-bit vs 32-bit**: Address mode word indicates compatibility
- **Byte Sex**: Always matches target ARM system (typically little-endian)
- **Old Formats**: Address mode = 0 indicates 26-bit AIF header
- **Modern Use**: 32-bit mode with Address mode = 32

---

## 13. Use on 3DO Platform

The 3DO console adopted AIF for its executable format with some platform-specific considerations:

### 13.1 3DO-Specific Characteristics

- **Load Address**: Platform-specific, not RISC OS standard 0x8000
- **Exit Instruction**: Platform-specific SWI or equivalent
- **Memory Layout**: Determined by 3DO operating environment
- **Debugging**: Debug chapters not included in 3DO documentation edition

### 13.2 3DO Documentation Notes

The 3DO edition explicitly excludes:
- ARM Symbolic Debug Table Format
- ARM Remote Debug Interface
- ARM Remote Debug Protocol
- ARM Debug Monitor

---

## 14. Validation and Error Checking

### 14.1 Header Validation

To validate an AIF header:

1. **Check Size**: Must be exactly 128 bytes
2. **Identify Variant**: Check fourth word (offset 0x0C):
   - Executable: MSB = 0xEB
   - Non-executable: MSB nibble = 0x0
3. **Verify Alignment**: All size fields should be multiples of 4
4. **Check Debug Type**: Must be 0, 1, 2, or 3
5. **Validate Address Mode**: Should be 0, 26, or 32

### 14.2 Image Integrity

1. **Relocation List**: Must end with -1 (0xFFFFFFFF)
2. **Offsets**: All relocation offsets must be word-aligned (divisible by 4)
3. **Ranges**: Relocation offsets must be within image bounds
4. **Code Signatures**: First instruction patterns (NOP or BL)

---

## 15. Tools and Utilities

### 15.1 Creating AIF Images

- **ARM Linker (armlink)**: Primary tool for creating AIF from AOF
- **Compression Utilities**: Separate tools add compression wrapper
- **Options**: Linker flags control relocatable/absolute, debug inclusion

### 15.2 Examining AIF Images

- **ARMalyser**: RISC OS tool for analyzing AIF structure
- **DDT (DeskDebugger)**: RISC OS debugger with AIF support
- **Debugger CLI**: RISC OS 5 command-line debugger
- **Hex Editors**: Can view header structure directly

---

## 16. Example Header Decoding

### 16.1 Sample Executable AIF Header (Hex Dump)

```
Offset   00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
0x0000   E1 A0 00 00 E1 A0 00 00 EB 00 00 0C EB 00 00 18
0x0010   EF 00 00 11 A0 00 00 00 00 00 00 00 98 02 00 00
0x0020   00 00 00 00 00 00 00 00 00 80 00 00 00 00 00 00
0x0030   20 00 00 00 00 80 00 00 00 00 00 00 00 00 00 00
0x0040   E1 A0 00 00 E0 4E C0 0F E0 8F CE 0C E8 DC 00 16
0x0050   E2 4C CE 10 E0 8C CE 00 E0 8C CE 01 E3 A0 00 00
0x0060   E3 A0 10 00 E3 A0 20 00 E3 A0 30 00 E3 54 00 00
0x0070   D8 AC 00 0F E2 54 40 10 CA FF FF FC E1 B0 F0 0E
```

### 16.2 Decoded Header Fields

| Offset | Value | Field | Interpretation |
|--------|-------|-------|----------------|
| 0x00 | 0xE1A00000 | NOP | MOV r0,r0 - not compressed |
| 0x04 | 0xE1A00000 | NOP | MOV r0,r0 - not self-relocating |
| 0x08 | 0xEB00000C | BL ZeroInit | Branch to offset 0x40 |
| 0x0C | 0xEB000018 | BL Entry | Branch to entry point |
| 0x10 | 0xEF000011 | SWI 0x11 | OS_Exit |
| 0x14 | 0x000000A0 | RO Size | 160 bytes (includes header) |
| 0x18 | 0x00000000 | RW Size | 0 bytes |
| 0x1C | 0x00000298 | Debug Size | 664 bytes |
| 0x20 | 0x00000000 | ZI Size | 0 bytes |
| 0x24 | 0x00000000 | Debug Type | No debug data |
| 0x28 | 0x00008000 | Image Base | Linked at 0x8000 |
| 0x2C | 0x00000000 | Work Space | 0 |
| 0x30 | 0x00000020 | Address Mode | 32-bit mode |
| 0x34 | 0x00008000 | Data Base | 0x8000 |

---

## 17. Common Pitfalls and Best Practices

### 17.1 Common Errors

1. **Incorrect Header Size**: Header must be exactly 128 bytes
2. **Misaligned Sizes**: All size fields must be multiples of 4
3. **Forgotten NOP Reset**: After relocation, word 0x04 must become NOP
4. **Wrong Base Address**: Executable AIF includes header in RO size
5. **Missing Terminator**: Relocation list must end with -1
6. **Debug Data Overwrite**: Debugger must copy debug data before zero-init

### 17.2 Best Practices

1. **Use Linker**: Don't hand-craft AIF; use armlink with appropriate options
2. **Test Relocation**: Verify image works when loaded at different addresses
3. **Validate Debug Type**: Ensure debug type matches actual debug data presence
4. **Document Customizations**: If using AIF_HDR or AIF_RELOC, document clearly
5. **Check Alignment**: Verify all offsets and sizes are properly aligned
6. **Version Awareness**: Set Address Mode appropriately for target environment

---

## 18. Format Constants and Magic Numbers

### 18.1 Key Constants

| Name | Value | Description |
|------|-------|-------------|
| AIF_HEADER_SIZE | 128 (0x80) | Fixed header size |
| AIF_CODE_OFFSET | 128 (0x80) | Executable AIF code start |
| RELOC_LIST_END | -1 (0xFFFFFFFF) | Relocation list terminator |
| NOP_MOV | 0xE1A00000 | NOP as MOV r0,r0 |
| NOP_BLNV | 0xFB000000 | NOP as BLNV 0 |
| SWI_EXIT | 0xEF000011 | SWI 0x11 (OS_Exit) |
| BL_MASK | 0xEB000000 | BL instruction pattern |
| BL_MSB | 0xEB | Executable AIF signature |
| OFFSET_MSB_MASK | 0x0F | Non-executable signature mask |

### 18.2 Magic Number Detection

To detect AIF variant:
```c
uint32_t fourth_word = header[3];
if ((fourth_word >> 24) == 0xEB) {
    // Executable AIF
} else if ((fourth_word >> 28) == 0x0) {
    // Non-executable AIF
}
```

---

## 19. Related Formats

### 19.1 AOF (ARM Object Format)

- **Purpose**: Object file format (compiler/assembler output)
- **Relationship**: Linker converts AOF → AIF
- **Format**: Chunk-based with relocations
- **See**: Separate AOF specification document

### 19.2 ALF (ARM Object Library Format)

- **Purpose**: Library of AOF modules
- **Relationship**: Linker extracts AOF from ALF, then creates AIF
- **Format**: Chunk-based library container
- **See**: Separate ALF specification document

### 19.3 RMF (Relocatable Module Format)

- **Purpose**: RISC OS system modules
- **Relationship**: Uses many-time relocation similar to AIF
- **Format**: Module header + relocatable code + multi-use relocation
- **Differences**: Module header replaces AIF header, persistent relocation

---

## 20. Conclusion

ARM Image Format (AIF) is a simple yet powerful executable format designed for ARM-based systems. Its key strengths include:

- **Simplicity**: 128-byte header with clear structure
- **Flexibility**: Supports compression, relocation, debugging
- **Self-Contained**: Images handle their own preparation
- **Position-Independent**: Can load and execute at various addresses
- **Well-Documented**: Clear specification from Acorn/ARM

While largely superseded by ELF on modern systems, AIF remains important for:
- Historical RISC OS applications
- Embedded ARM systems
- Retro computing (3DO, Archimedes, etc.)
- Understanding ARM executable evolution

---

## Appendix A: Instruction Encodings

### ARM Branch Instructions

```
BL (Branch with Link):
31 30 29 28 27 26 25 24 23 ... 0
[  condition  ] 1  0  1  1  [  offset  ]

0xEB000000 = BL with offset 0 (unconditional)
```

### NOP Encodings

```
MOV r0, r0:
31 30 29 28 27 26 25 24 23 ... 0
1  1  1  0  0  0  0  1  1  0  1  0  0  0  0  0  ...
= 0xE1A00000

BLNV 0 (obsolete):
1  1  1  1  1  0  1  1  0  0  0  0  0  0  0  0  ...
= 0xFB000000
```

---

## Appendix B: References

### Primary Sources

1. **PLG-AIF** - RISC OS Application Image Format (1989)
   - Acorn Computers Limited
   - Authors: Lee Smith, Lionel Haines

2. **ARM DUI 0041C** - ARM Software Development Toolkit Reference
   - ARM Limited (1997-1998)
   - Chapter 13: ARM Image Format

3. **RISC OS Programmer's Reference Manuals**
   - Appendix D: Code File Formats
   - 3QD Developments Ltd (2015)

### Additional Resources

- ARM Software Development Toolkit Reference Guide
- RISC OS Open Limited documentation
- 3DO Developer Documentation
- Paolo Fabio Zaino's AIF analysis articles

---

**Document Version**: 1.0
**Date**: 2026-02-07
**Status**: Complete Specification
