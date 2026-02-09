# ARM Image Format (AIF) - Complete File Format Specification

## Overview

The ARM Image Format (AIF) is a simple executable format designed for ARM-based systems, originally developed by Acorn Computers for RISC OS and later used on other platforms including the 3DO game console. AIF provides a self-contained executable image with optional support for compression, self-relocation, and debugging.

### History

- **1987**: Initial version developed for Arthur/RISC OS (PLG-AIF)
- **1989**: Major revision with improved relocatable image support and ASD/Dbug support
- **1993**: Extended AIF format introduced for scatter-loaded images
- **1997-1998**: ARM Limited standardization (DUI 0041C)

### Variants

AIF exists in three main variants:

1. **Executable AIF**: Most common variant where header is part of the executable image
2. **Non-executable AIF**: Header describes image but is discarded during loading
3. **Extended AIF**: Special non-executable AIF containing scatter-loaded image with region descriptors

## File Structure

### Overall Layout

An AIF file consists of the following components in order:

```
┌─────────────────────────────────────┐
│ AIF Header (128 bytes, fixed)      │
├─────────────────────────────────────┤
│ Read-Only Area (code & constants)    │
├─────────────────────────────────────┤
│ Read-Write Area (initialized data)   │
├─────────────────────────────────────┤
│ Debugging Data (optional)             │
├─────────────────────────────────────┤
│ Self-Relocation Code (position-indep.)│
├─────────────────────────────────────┤
│ Relocation List (word offsets + -1)    │
└─────────────────────────────────────┘
```

### Compressed AIF Layout

For compressed images, the layout differs:

```
┌─────────────────────────────────────┐
│ AIF Header (128 bytes, not compressed)│
├─────────────────────────────────────┤
│ Compressed Image Data                │
├─────────────────────────────────────┤
│ Decompression Data (position-indep.)   │
├─────────────────────────────────────┤
│ Decompression Code (position-indep.)  │
└─────────────────────────────────────┘
```

### Post-Relocation Layout

After self-relocation code execution (or for non-relocating images):

```
┌─────────────────────────────────────┐
│ AIF Header                         │
├─────────────────────────────────────┤
│ Read-Only Area                     │
├─────────────────────────────────────┤
│ Read-Write Area                    │
├─────────────────────────────────────┤
│ Debugging Data (optional)             │
└─────────────────────────────────────┘
```

## AIF Header Specification

The AIF header is exactly 128 bytes (32 words) with the following structure:

### Header Layout

| Offset | Name | Description |
|--------|------|-------------|
| 0x00   | BL DecompressCode | `BL` to decompression code, or `NOP` if not compressed |
| 0x04   | BL SelfRelocCode | `BL` to self-relocation code, or `NOP` if not self-relocating |
| 0x08   | BL ZeroInit | `BL` to zero-initialization code, or `NOP` if no zero-init area |
| 0x0C   | BL ImageEntryPoint | `BL` to entry point for executable AIF, or entry point offset for non-executable AIF |
| 0x10   | Program Exit | Exit instruction (typically `SWI OS_Exit`) |
| 0x14   | Image ReadOnly Size | Size of read-only area in bytes (includes header size for executable AIF) |
| 0x18   | Image ReadWrite Size | Size of read-write area in bytes (exact, multiple of 4) |
| 0x1C   | Image Debug Size | Size of debugging data in bytes (exact, multiple of 4). Bits 0-3 = type, bits 4-31 = low-level debug size |
| 0x20   | Image Zero-Init Size | Size of zero-initialized area in bytes (exact, multiple of 4) |
| 0x24   | Image Debug Type | Debug data type: 0=none, 1=low-level, 2=source-level, 3=both |
| 0x28   | Image Base | Address where image (code) was linked |
| 0x2C   | Work Space | Obsolete (was minimum workspace in bytes) |
| 0x30   | Address Mode | Address mode information (see below) |
| 0x34   | Data Base | Address where image data was linked (if separate data base used) |
| 0x38   | Reserved Words | Two reserved words (0 for standard AIF, used for Extended AIF) |
| 0x40   | Debug Init | Debug initialization instruction, or `NOP` if unused |
| 0x44   | Zero-Init Code | 15 words of zero-initialization code |

### Key Header Fields

#### Address Mode (0x30)

The least significant byte contains:
- **26**: Image linked for 26-bit ARM mode (obsolete)
- **32**: Image linked for 32-bit ARM mode
- **0**: Old-style 26-bit AIF header

If bit 8 (0x100) is set, image uses separate code and data bases.

#### Distinguishing Executable vs Non-executable AIF

- **Executable AIF**: 4th word (0x0C) is `BL entrypoint`. Most significant byte = 0xEB
- **Non-executable AIF**: 4th word is entry point offset. Most significant nibble = 0x0

#### Extended AIF Region Header (44 bytes)

When word at 0x38 is non-zero, it contains offset to first non-root load region:

| Word | Description |
|------|-------------|
| 0    | File offset of header of next region (0 if none) |
| 1    | Load address |
| 2    | Size in bytes (multiple of 4) |
| 3-10 | Region name (32 chars, padded with zeros) |

## Code Components

### Zero-Initialization Code

Standard zero-initialization code (15 words):

```arm
ZeroInit:
    NOP                       ; or <Debug Init Instruction>
    SUB    ip, lr, pc         ; base+12+[PSR]-(ZeroInit+12+[PSR]) = base-ZeroInit
    ADD    ip, pc, ip         ; base-ZeroInit+ZeroInit+16 = base+16
    LDMIB  ip, {r0,r1,r2,r4}  ; various sizes
    SUB    ip, ip, #16        ; image base
    ADD    ip, ip, r0         ; + RO size
    ADD    ip, ip, r1         ; + RW size = base of 0-init area
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMPS   r4, #0
00  MOVLE  pc, lr             ; nothing left to do
    STMIA  ip!, {r0,r1,r2,r3} ; always zero a multiple of 16 bytes
    SUBS   r4, r4, #16
    B      %B00
```

### Self-Relocation Code

Position-independent code for relocating images. Entered via `BL` from header word 0x04.

#### Header Address Calculation

```arm
RelocCode:
    NOP                   ; required by ensure_byte_order()
    SUB    ip, lr, pc     ; base+8+[PSR]-(RelocCode+12+[PSR]) = base-4-RelocCode
    ADD    ip, pc, ip     ; base-4-RelocCode+RelocCode+16 = base+12
    SUB    ip, ip, #12    ; -> header address
    LDR    r0, RelocCode  ; NOP
    STR    r0, [ip, #4]   ; won't be called again on image re-entry
    LDR    r9, [ip, #&2C]  ; min free space requirement
    CMPS   r9, #0         ; 0 => no move, just relocate
    BEQ    RelocateOnly
```

#### Finding Top of Memory

```arm
    LDR    r0, [ip, #&20]     ; image zero-init size
    ADD    r9, r9, r0         ; space to leave = min free + zero init
    SWI    #&10               ; return top of memory in r1
```

#### Move Distance Calculation

```arm
    ADR    r2, End            ; -> End
01  LDR    r0, [r2], #4       ; load relocation offset, increment r2
    CMNS   r0, #1             ; terminator?
    BNE    %B01               ; No, so loop again
    SUB    r3, r1, r9         ; MemLimit - freeSpace
    SUBS   r0, r3, r2         ; amount to move by
    BLE    RelocateOnly       ; not enough space to move...
    BIC    r0, r0, #15        ; a multiple of 16...
    ADD    r3, r2, r0         ; End + shift
    ADR    r8, %F02           ; intermediate limit for copy-up
```

#### Copy and Relocate

```arm
02  LDMDB  r2!, {r4-r7}
    STMDB  r3!, {r4-r7}
    CMPS   r2, r8             ; copied copy loop?
    BGT    %B02               ; not yet
    ADD    r4, pc, r0
    MOV    pc, r4             ; jump to copied copy code
03  LDMDB  r2!, {r4-r7}
    STMDB  r3!, {r4-r7}
    CMPS   r2, ip             ; copied everything?
    BGT    %B03               ; not yet
    ADD    ip, ip, r0         ; load address of code
    ADD    lr, lr, r0         ; relocated return address
```

#### Relocation Processing

```arm
RelocateOnly:
    LDR    r1, [ip, #&28]     ; header + 0x28 = code base set by Link
    SUBS   r1, ip, r1         ; relocation offset
    MOVEQ  pc, lr             ; relocate by 0 so nothing to do
    STR    ip, [ip, #&28]     ; new image base = actual load address
    ADR    r2, End            ; start of reloc list
04  LDR    r0, [r2], #4       ; offset of word to relocate
    CMNS   r0, #1             ; terminator?
    MOVEQ  pc, lr             ; yes => return
    LDR    r3, [ip, r0]       ; word to relocate
    ADD    r3, r3, r1         ; relocate it
    STR    r3, [ip, r0]       ; store it back
    B      %B04               ; and do the next one
End                           ; The list of offsets of locations to relocate starts here, terminated by -1
```

## Relocation Processing

### Relocation List Format

The relocation list is a sequence of 4-byte word offsets from the beginning of the AIF header, terminated by a word containing -1 (0xFFFFFFFF).

- Each offset points to a word that needs relocation
- Only word-sized values can be relocated
- Relocation value = (actual_load_address - linked_address)
- List is processed after image movement (if any) and before zero-initialization

### Relocation Types

For each word to be relocated, the new value is calculated as:

`new_word = original_word + (actual_base_address - linked_base_address)`

This allows absolute addresses and pointers to be adjusted to the actual load address.

## Debug Support

### Debug Types

| Value | Description |
|-------|-------------|
| 0     | No debugging data present |
| 1     | Low-level debugging data present |
| 2     | Source-level (ASD) debugging data present |
| 3     | Both low-level and source-level debugging present |

### Debug Data Properties

- References from debugging tables to code/data are relocatable addresses
- References between debugger table entries are offsets from beginning of debug data area
- After relocation, debug data area is position-independent
- Debugger may copy debug data to safe location before program execution

### Debug Initialization

The Debug Init instruction (word 0x40) is typically a SWI that alerts a resident debugger that a debuggable image is starting execution.

## Special Values and Constants

### Instruction Encodings

- **NOP**: `MOV r0, r0` = `0xE1A00000`
- **BL instruction**: Most significant byte = `0xEB` for executable AIF
- **SWI OS_Exit**: `0xEF000011`
- **SWI OS_GetEnv**: `0xEF000010`

### Address Constants

- **Default load address**: `0x00008000` (RISC OS standard)
- **Header size**: 128 bytes (0x80)

### 3DO Extensions

For 3DO systems, an additional 128-byte header follows the AIF header when the AIF_3DOHEADER flag (0x40000000) is set in the WorkSpace field.

#### 3DO Binary Header Structure

| Offset | Field | Description |
|--------|-------|-------------|
| 0x00   | ItemNode | Node information |
| 0x04   | _3DO_Flags | Application flags |
| 0x05   | _3DO_OS_Version | Target OS version |
| 0x06   | _3DO_OS_Revision | OS revision |
| 0x07   | _3DO_Reserved | Reserved (0) |
| 0x08   | _3DO_Stack | Stack requirements |
| 0x0C   | _3DO_FreeSpace | Preallocated bytes for FreeList |
| 0x10   | _3DO_Signature | Offset to signature (if privileged) |
| 0x14   | _3DO_SignatureLen | Length of signature |
| 0x18   | _3DO_MaxUSecs | Max microseconds before task switch |
| 0x1C   | _3DO_Reserved0 | Must be zero |
| 0x20   | _3DO_Name[32] | Optional task name |
| 0x40   | _3DO_Time | Seconds since 1/1/93 00:00:00 GMT |
| 0x44   | _3DO_Reserved1[7] | Must be zero |

## File Identification

### Executable vs Non-executable Detection

Examine the 4th word (offset 0x0C):

```c
uint32_t fourth_word = *(uint32_t*)(aif_data + 0x0C);
if ((fourth_word & 0xFF000000) == 0xEB000000) {
    // Executable AIF - BL instruction
} else if ((fourth_word & 0xF0000000) == 0x00000000) {
    // Non-executable AIF - offset value
}
```

### Endianness Detection

AIF files use the endianness of the target ARM system. No explicit endianness indicator is present in the format.

## Implementation Notes

### Memory Protection

On systems with memory protection:
- Decompression code must temporarily make read-only section writable
- Self-relocation code must temporarily make read-only section writable
- Both must restore read-only protection after completion

### Restart Capability

An AIF image is restartable if and only if:
- The program it contains is restartable
- Following decompression, first word of header is reset to NOP
- Following self-relocation, second word of header is reset to NOP

### Position Independence

- Header must be position-independent for both 26-bit and 32-bit ARM modes
- BL instructions are used to make header addressable via R14 (link register)
- Self-relocation and decompression code must be position-independent

### Alignment Requirements

- All multi-byte values are word-aligned (4-byte boundaries)
- Image sizes are multiples of 4 bytes
- Area addresses are typically word-aligned

## Usage Examples

### Loading an Executable AIF

```c
typedef struct {
    uint32_t bl_decompress;
    uint32_t bl_selfreloc;
    uint32_t bl_zeroinit;
    uint32_t bl_entry;
    uint32_t swi_exit;
    int32_t  ro_size;
    int32_t  rw_size;
    int32_t  debug_size;
    int32_t  zeroinit_size;
    int32_t  debug_type;
    uint32_t image_base;
    int32_t  workspace;
    uint32_t address_mode;
    uint32_t data_base;
    uint32_t reserved[2];
    uint32_t debug_init;
    uint32_t zeroinit_code[15];
} AIFHeader;

// Load at base address
void* load_address = (void*)0x8000;  // RISC OS default
AIFHeader* header = (AIFHeader*)load_address;

// Execute by jumping to first word
typedef void (*entry_func_t)(void);
entry_func_t entry = (entry_func_t)load_address;
entry();
```

### Processing Relocation List

```c
void process_relocations(AIFHeader* header, void* actual_base) {
    uint32_t* reloc_list = (uint32_t*)((char*)header + 
                                   header->ro_size + 
                                   header->rw_size + 
                                   header->debug_size);
    
    int32_t reloc_offset = *(int32_t*)reloc_list;
    while (reloc_offset != -1) {
        uint32_t* word_to_reloc = (uint32_t*)((char*)header + reloc_offset);
        *word_to_reloc += (uint32_t)actual_base - header->image_base;
        reloc_list++;
        reloc_offset = *(int32_t*)reloc_list;
    }
}
```

## Compatibility

### Version History

- **AIF 0.03**: Basic position-independent support
- **AIF 0.04**: ASD/Dbug support (1989)
- **AIF 3.00**: Extended AIF with scatter loading (1993)

### Platform Support

- **RISC OS**: Native support, default load address 0x8000
- **3DO**: Extended with 3DO binary header for game console
- **ARM SDT**: Full toolchain support

### Limitations

- No dynamic linking support
- No security/validation fields
- No compiler/tool identification fields
- No explicit version field in header
- Limited to single-threaded execution model

## References

- ARM DUI 0041C Reference Guide (1997-1998)
- RISC OS Programmer's Reference Manuals
- PLG-AIF Technical Memorandum (1989)
- ARM Software Development Toolkit User Guide
- 3DO Development Documentation