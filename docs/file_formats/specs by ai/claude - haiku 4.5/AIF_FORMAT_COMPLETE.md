# ARM Image Format (AIF) - Complete File Format Specification

**Version:** 3.00+
**Last Updated:** 2026-02-07
**Based on:** ARM DUI 0041C, RISC OS Programmer's Reference Manuals, ARM STD Reference Guide

---

## Table of Contents

1. [Overview](#overview)
2. [Properties and Variants](#properties-and-variants)
3. [File Layout](#file-layout)
4. [Header Specification](#header-specification)
5. [Image Sections](#image-sections)
6. [Relocation and Self-Relocation](#relocation-and-self-relocation)
7. [Decompression Support](#decompression-support)
8. [Zero-Initialization](#zero-initialization)
9. [Debug Support](#debug-support)
10. [C Structure Definition](#c-structure-definition)

---

## Overview

**ARM Image Format (AIF)** is a simple yet powerful executable image format designed for ARM microprocessors. Originally introduced by Acorn Computers Ltd for the Archimedes and RISC OS systems, AIF remains in use on RISC OS platforms and has been adopted by other systems including the 3DO Interactive Multiplayer.

### Key Characteristics

- **Simple Design:** Minimal overhead, easy to implement
- **Flexible Loading:** Can load at any address with self-relocation support
- **Compression Support:** Optional self-decompressing capability
- **Debug Symbols:** Optional debugging information support
- **Zero-Initialization:** Automatic memory zeroing capability
- **Position Independence:** Can relocate itself at load time

### Format History

| Version | Date | Key Features |
|---------|------|--------------|
| 0.03 | 1987 | Initial version with position independence |
| 0.04 | 1988 | Source-level debugging support |
| 3.00 | 1989 | Extended AIF with scatter-loaded images |
| 3.00+ | 1997+ | 32-bit ARM support, Thumb extensions |

---

## Properties and Variants

### AIF Variant Types

AIF exists in three distinct variants, each serving different purposes:

#### 1. Executable AIF

**Purpose:** Direct execution without loader assistance

**Characteristics:**
- Header is part of the executable image
- Loads and executes from its first word (offset 0x00)
- Prepares itself for execution before entering entry point
- Standard load address on RISC OS: 0x8000 (virtual address)
- Fourth word of header is always: `BL entry-point-address`

**Identification:**
- Fourth word MSB (most significant byte): `0xEB` (ARM instruction prefix for BL)

**Execution Model:**
```
Load at address X (e.g., 0x8000)
├─ Execute header decompression code (word 0)
├─ Execute self-relocation code (word 1)
├─ Execute zero-initialization code (word 2)
└─ Call entry point (word 3)
```

#### 2. Non-Executable AIF

**Purpose:** Loader-based execution

**Characteristics:**
- Header is not part of the executable image
- Header describes the image but is discarded after loading
- Loader interprets header and prepares image
- Load address: base address of code section
- Fourth word is offset from base, not BL instruction

**Identification:**
- Fourth word MSB (most significant nibble): `0x0`

**Use Cases:**
- ROM-based applications
- Custom loader environments
- Embedded systems with minimal loader

#### 3. Extended AIF

**Purpose:** Scatter-loaded complex images

**Characteristics:**
- Special type of Non-Executable AIF
- Contains scatter-loaded image with multiple regions
- Header points to chain of load region descriptors
- Word at offset 0x38 contains byte offset to first region header
- Each region can be loaded at different address

**Region Descriptor Format (44 bytes each):**
```
Offset  Length  Field Description
0       4       Next region header offset (0 if none)
4       4       Load address for region
8       4       Region size in bytes (must be multiple of 4)
12      32      Region name (max 32 chars, null-padded)
```

---

## File Layout

### Uncompressed AIF Structure

The layout of a standard uncompressed AIF image is:

```
Offset  Size        Contents
──────────────────────────────────────────────────────
0x00    0x80 (128)  AIF Header (32 words, exactly 128 bytes)
0x80    variable    Read-Only Area (code, literals, etc.)
        variable    Read-Write Area (initialized data)
        variable    Debugging Data (optional)
        variable    Self-Relocation Code (position-independent)
        variable    Relocation List (terminated by -1)
```

**Section Alignment:** All sections align to 4-byte boundaries

### Compressed AIF Structure

When compressed (indicated by non-NOP first word of header):

```
Offset  Size        Contents
──────────────────────────────────────────────────────
0x00    0x80 (128)  AIF Header (NOT compressed)
0x80    variable    Compressed Image Data
        variable    Decompression Data (position-independent)
        variable    Decompression Code (position-independent)
```

**Key Point:** The header is never compressed, only the image data

### Post-Relocation Layout

After self-relocation (or if not self-relocating):

```
Offset  Size        Contents
──────────────────────────────────────────────────────
0x00    0x80 (128)  AIF Header
0x80    variable    Read-Only Area
        variable    Read-Write Area
        variable    Debugging Data (optional, typically saved by debugger)
```

**Note:** Self-relocation code and relocation list are overwritten by application memory (heap/stack)

---

## Header Specification

### Overview

The AIF header is exactly **128 bytes (32 words)** and contains all metadata needed to properly load and execute the image. The header itself must be position-independent and uses BL instructions to make itself addressable via R14.

### Detailed Header Layout

| Offset | Word# | Field | Type | Size | Description |
|--------|-------|-------|------|------|-------------|
| 0x00 | 0 | BL DecompressCode/NOP | Instr | 4 | Jump to decompression code, or NOP if uncompressed |
| 0x04 | 1 | BL SelfRelocCode/NOP | Instr | 4 | Jump to relocation code, or NOP if no relocation |
| 0x08 | 2 | BL ZeroInit/NOP | Instr | 4 | Jump to zero-init routine, or NOP if none |
| 0x0C | 3 | BL ImageEntryPoint / EntryOffset | Mixed | 4 | Executable: BL to entry; Non-exec: offset from base |
| 0x10 | 4 | Program Exit Instruction | Instr | 4 | SWI or branch-to-self (emergency exit) |
| 0x14 | 5 | Image ReadOnly Size | Word | 4 | Size of RO section (includes header if executable) |
| 0x18 | 6 | Image ReadWrite Size | Word | 4 | Size of RW section (multiple of 4) |
| 0x1C | 7 | Image Debug Size | Word | 4 | Size of debug section (bits 0-3: type, 4-31: size) |
| 0x20 | 8 | Image ZeroInit Size | Word | 4 | Size of zero-init section (multiple of 4) |
| 0x24 | 9 | Image Debug Type | Word | 4 | Debug type: 0/1/2/3 (see Debug Type Table) |
| 0x28 | 10 | Image Base | Word | 4 | Address image was linked at |
| 0x2C | 11 | Work Space | Word | 4 | Obsolete (RISC OS 2 era) |
| 0x30 | 12 | Address Mode (26/32) | Word | 4 | Byte 0: 0/26/32; Bit 8 set = separate data base |
| 0x34 | 13 | Data Base | Word | 4 | Address data section was linked at |
| 0x38-0x3C | 14-15 | Reserved | Words | 8 | Extended AIF: word 14 contains offset to first region |
| 0x40 | 16 | DBGInit Instruction | Instr | 4 | Debug init SWI or NOP |
| 0x44-0x7C | 17-31 | ZeroInit Code | Code | 60 | 15-word zero-initialization routine |

**Total Header Size:** 0x80 (128 bytes)

### Field Details

#### Word 0: Decompression Code Jump
```
If image is NOT compressed:
  Value: 0xE1A00000 (MOV r0, r0 - NOP instruction)
  Encoding: BLNV 0

If image IS compressed:
  Value: BL <decompression_code_address>
  Type: ARM instruction (BL relative branch)
  Target: Position-independent decompression routine
```

#### Word 1: Self-Relocation Code Jump
```
If image does NOT self-relocate:
  Value: 0xE1A00000 (NOP)

If image self-relocates:
  Value: BL <relocation_code_address>
  Type: ARM instruction
  Target: Position-independent relocation routine
  Register on entry: R14 = word 1 address + 8
```

#### Word 2: Zero-Initialization Jump
```
If no zero-init section:
  Value: 0xE1A00000 (NOP)

If zero-init needed:
  Value: BL <zeroinit_code_address>
  Type: ARM instruction
  Target: Standard zero-init routine
```

#### Word 3: Entry Point (Executable vs Non-Executable)
**Executable AIF:**
```
Value: BL <entry_point_address>
Encoding: 0xEB??????
MSB indicates type: 0xEB is ARM BL instruction
```

**Non-Executable AIF:**
```
Value: <offset_from_base>
Type: 32-bit unsigned integer
MSB: 0x0? (upper nibble must be 0)
Entry address: Image_Base + this_offset
```

#### Word 4: Program Exit Instruction
```
Standard values:
  RISC OS: SWI 0x11 (OS_Exit)
  Bare metal: MOV PC, LR (return to caller if reached)
  Embedded: B . (branch to self, hang)

Format: Valid ARM instruction
Purpose: Emergency exit if program returns to header
```

#### Word 5: ReadOnly Size
```
Type: 32-bit unsigned integer
Range: 0 to 0xFFFFFFFF
Units: Bytes
Includes: Header (if Executable AIF) + all RO code and data
Must be: Multiple of 4
Example: 0x000000A0 = 160 bytes
```

#### Word 6: ReadWrite Size
```
Type: 32-bit unsigned integer
Units: Bytes (initialized data)
Must be: Multiple of 4 (4-byte aligned)
Range: 0 if no initialized data
Excludes: Zero-initialized data (separate field)
```

#### Word 7: Debug Size (Composite Field)
```
Bits 0-3:    Debug type (0-3, see below)
Bits 4-31:   Low-level debug data size in bytes
Total size includes both high-level and low-level debug

Example: 0x00000298
  Low 4 bits (type): 0x8 (undefined, use word 9)
  Upper 28 bits (size): 0x00000029 = 41 decimal
```

#### Word 8: ZeroInit Size
```
Type: 32-bit unsigned integer
Units: Bytes
Must be: Multiple of 4
Purpose: Size of memory to zero-initialize
Contains: Uninitialized static data (BSS section)
```

#### Word 9: Debug Type
```
Valid values:
  0 = No debugging data present
  1 = Low-level debugging data present (machine-level symbols)
  2 = Source-level debugging data present (ASD format)
  3 = Both low-level and source-level debug data present

Preference: Check word 9 over word 7 bits 0-3
All other values: Reserved to ARM/Acorn
```

#### Word 10: Image Base
```
Type: 32-bit unsigned address
Purpose: Address at which image was linked
Used by: Relocation code to calculate offset
Standard on RISC OS: 0x8000 (virtual address)
Must be: 4-byte aligned
```

#### Word 11: Work Space (Obsolete)
```
Historical use (RISC OS 2): Minimum free memory required
Current status: DEPRECATED, ignore or set to 0
New systems: Should not rely on this field
Default: Set to 0 for compatibility
```

#### Word 12: Address Mode & Flags
```
Byte 0 (LS byte):
  0   = Old 26-bit AIF (obsolete)
  26  = 26-bit ARM mode (obsolete)
  32  = 32-bit ARM mode (current standard)

Bits 8-31:
  Bit 8 (0x0100): Separate code/data base flag
    0 = Single base address
    1 = Separate Data Base field at 0x34 is valid

Other bits: Reserved (must be 0)

Example: 0x00000020 = 32-bit mode, single base
```

#### Word 13: Data Base
```
Type: 32-bit address
Valid only if: Bit 8 of word 12 is set
Purpose: Base address of data section (if separate)
Applies to: Images with separate code/data addressing
Default: 0 if not separated
```

#### Words 14-15: Reserved (Extended AIF Use)
```
Word 14:
  Extended AIF: Byte offset to first load region descriptor
  Executable AIF: Must be 0

Word 15:
  Reserved: Must be 0

Extended AIF Region Descriptor:
  44 bytes per region
  Linked chain via offset in word 0 of each descriptor
```

#### Word 16: Debug Init Instruction
```
If unused: 0xE1A00000 (NOP)
If used: SWI instruction to alert resident debugger
  Format: SWI <handler_number>
  Purpose: Notify debugger that debuggable image starting
  Current: Often unused (set to NOP)
```

#### Words 17-31: Zero-Initialization Code
```
Length: 15 words (60 bytes) (15 words: word 17 through word 31 is 15 words)
Purpose: Standard routine to zero-initialize memory
Position-independent: Uses PC-relative addressing
Included by: Linker automatically
Execution: Enters via BL from word 2
```

---

## Image Sections

### Read-Only (RO) Section

**Location:** Immediately after header (offset 0x80 in executable AIF)

**Contents:**
- Executable code (ARM or Thumb)
- Position-independent code (if compiled with -fPIC)
- Read-only data (literals, strings, constants)
- Jump tables, lookup tables
- Debugging tables (if present)

**Properties:**
- Should not be modified after loading
- Can be placed in ROM on systems with MMU
- Size specified by word 5 of header
- Must include header size if Executable AIF

**Alignment:** 4-byte minimum (depends on section attributes)

### Read-Write (RW) Section

**Location:** After Read-Only section

**Contents:**
- Initialized global/static variables
- Mutable data structures
- Initial values for variables

**Properties:**
- Must be initialized from file data
- Mutable during execution
- Typically copied to RAM
- Size specified by word 6 of header
- Must be multiple of 4 bytes

**Initialization:** All data must be present in file

### Zero-Initialized (ZI) Section

**Location:** Created at runtime in memory (not in file)

**Contents:**
- Uninitialized static/global variables
- BSS section data
- Heap space (often)
- Stack space (system-dependent)

**Properties:**
- Not stored in file (implied zero value)
- Allocated and zeroed by zero-init code
- Size specified by word 8 of header
- Must be multiple of 4 bytes

**Creation Process:**
1. Memory allocated at address = ImageBase + ROSize + RWSize
2. Zero-init code fills 16 bytes at a time with zeros
3. Loop continues for entire ZI section
4. Execution returns to entry point

### Debugging Data Section

**Location:** After Read-Write section (before relocation data)

**Contents:**
- Symbol tables (if low-level debug)
- Source file information (if source-level debug)
- Type definitions
- Local variable information
- Line number mapping (if ASD format)

**Properties:**
- Present only if Image Debug Type (word 9) > 0
- Position-independent (offset-relative references)
- Typically saved by debugger to safe location
- Can be copied/relocated by debugger
- Size specified in word 7 (bits 4-31) or word 9

**Format:** ARM Symbolic Debugging (ASD) format (see Debug Support section)

### Self-Relocation Code Section

**Location:** After debugging data (optional)

**Contents:**
- Position-independent code to relocate image
- Move code (if self-moving)
- Relocation loop

**Properties:**
- Must be position-independent
- Entered via BL from word 1 of header
- Modifies RO section (writes to relocation addresses)
- Requires MMU write permission on RO section
- Terminates by returning via R14

**Included When:**
- Link flag `-Relocatable` specified
- Self-move relocation requested
- Image expects to be loaded at non-standard address

### Relocation List

**Location:** After self-relocation code (if present)

**Format:**
```
Word 0:   Relocation offset 1 (byte offset from header start)
Word 1:   Relocation offset 2
Word 2:   Relocation offset 3
...
Word N:   0xFFFFFFFF (-1 terminator, marks end of list)
```

**Contents:**
- Byte offsets (relative to header start, offset 0x00)
- Must be aligned to word boundaries
- Each entry is 4 bytes (one word)
- Terminated by word containing -1 (0xFFFFFFFF)
- No other data after terminator

**Relocation Process:**
1. Read offset from list
2. If offset = -1, relocation complete
3. Load word from address ImageBase + offset
4. Add relocation amount (ImageBase - ImageBase_linked)
5. Store word back
6. Continue with next offset

---

## Relocation and Self-Relocation

### One-Time Position Independence

**Purpose:** Allow image to load anywhere, relocate once

**Requirements:**
- Linker must generate relocation list
- All position-dependent references must be in list
- First load of relocation code resets word 1 to NOP
- Second entry to header (if re-entered) skips relocation

**Process:**
```
Load at address L (unknown at link time)
  │
  ├─ Decompress if needed (word 0)
  │
  ├─ Relocation code (word 1):
  │  ├─ Calculate relocation offset = L - ImageBase_linked
  │  ├─ For each offset in relocation list:
  │  │  ├─ Load word at (base + offset)
  │  │  ├─ Add relocation offset
  │  │  └─ Store word back
  │  └─ Reset word 1 to NOP
  │
  ├─ Zero-init (word 2)
  │
  └─ Jump to entry point (word 3)
```

### Self-Move Relocation

**Purpose:** Move image to high memory, preserving workspace

**Setup:**
- Word 11 (obsolete work space) or other means specifies free space needed
- System provides GetEnv SWI to return top of memory
- Image copies itself upward, then relocates

**Memory Layout Before Move:**
```
0x00000000 ┌─────────────────────┐
           │  Low Memory         │
           ├─────────────────────┤
    Load → │  Image (original)   │
           ├─────────────────────┤
           │  Free Space         │
           ├─────────────────────┤
    Top →  │  Heap/Stack Space   │
0xFFFFFFFF └─────────────────────┘
```

**Memory Layout After Move:**
```
0x00000000 ┌─────────────────────┐
           │  Low Memory         │
           ├─────────────────────┤
           │  Free Space         │
           ├─────────────────────┤
    New → │  Image (moved up)    │
           ├─────────────────────┤
           │  Reserved Space     │
    Top →  │  (application use)  │
0xFFFFFFFF └─────────────────────┘
```

**Move Process:**
```
1. Calculate end of relocation list
2. Call GetEnv to get top of memory
3. Determine move distance = (top - reserved_space) - end_of_list
4. If move_distance > 0 AND (move_distance >= min_required):
     └─ Copy image in descending order (bottom-up)
        └─ Jump to copied copy code
        └─ Continue copying from new location
5. Perform relocation (add move_distance to all relocation addresses)
6. Update word 10 (Image Base) with new address
```

### Relocation Code Example

Standard ARM relocation code (from ARM linker):

```arm
RelocCode
    NOP                           ; required for byte-order ops
    SUB    ip, lr, pc            ; calculate relocation offset
    ADD    ip, pc, ip            ; base-4-RelocCode+RelocCode+16
    SUB    ip, ip, #12           ; → header address
    LDR    r0, RelocCode         ; get NOP instruction
    STR    r0, [ip, #4]          ; won't relocate twice

    ; Check if self-move needed
    LDR    r9, [ip, #&2C]        ; min free space requirement
    CMPS   r9, #0                ; 0 => no move, just relocate
    BEQ    RelocateOnly

    ; Calculate space and find end of relocation list
    LDR    r0, [ip, #&20]        ; image zero-init size
    ADD    r9, r9, r0            ; space to leave
    SWI    #&10                  ; get top of memory in r1
    ADR    r2, End               ; → relocation list
01  LDR    r0, [r2], #4          ; load relocation offset
    CMNS   r0, #1                ; check for -1 terminator
    BNE    %B01                  ; loop if not end

    ; Calculate move distance
    SUB    r3, r1, r9            ; MemLimit - freeSpace
    SUBS   r0, r3, r2            ; amount to move by
    BLE    RelocateOnly          ; not enough space
    BIC    r0, r0, #15           ; align to 16-byte boundary
    ADD    r3, r2, r0            ; end + shift
    ADR    r8, %F02              ; copy loop limit

    ; Copy image upward (descending address order)
02  LDMDB  r2!, {r4-r7}          ; load 4 words
    STMDB  r3!, {r4-r7}          ; store to new location
    CMPS   r2, r8                ; copied copy loop?
    BGT    %B02
    ADD    r4, pc, r0            ; calculate jump address
    MOV    pc, r4                ; jump to copied code

03  LDMDB  r2!, {r4-r7}          ; continue copy from new location
    STMDB  r3!, {r4-r7}
    CMPS   r2, ip                ; copied everything?
    BGT    %B03
    ADD    ip, ip, r0            ; update header address
    ADD    lr, lr, r0            ; update return address

RelocateOnly
    LDR    r1, [ip, #&28]        ; get original image base
    SUBS   r1, ip, r1            ; calculate relocation offset
    MOVEQ  pc, lr                ; if 0, no relocation needed
    STR    ip, [ip, #&28]        ; update image base
    ADR    r2, End               ; start of relocation list

04  LDR    r0, [r2], #4          ; load relocation offset
    CMNS   r0, #1                ; terminator?
    MOVEQ  pc, lr                ; return if done
    LDR    r3, [ip, r0]          ; load word at offset
    ADD    r3, r3, r1            ; add relocation amount
    STR    r3, [ip, r0]          ; store back
    B      %B04                  ; next relocation

End                              ; relocation list starts here
```

---

## Decompression Support

### Compression Overview

**Purpose:** Reduce image size on disk/ROM, enable self-decompression

**Detection:**
- Word 0 of header != 0xE1A00000 (NOP)
- Word 0 contains BL to decompression code
- Decompression code is position-independent

**Typical Compression Algorithms:**
- RLE (Run-Length Encoding) - simple, fast
- LZ77/LZSS variants - common in embedded systems
- Huffman coding - better compression ratio
- Custom formats - ARM tools used various approaches

### Decompression Process

```
Entry:
  R14 = address of word 0 (header)

Process:
  1. Decompress entire image data
  2. Write decompressed data at correct offsets in memory
  3. Reset word 0 to NOP (0xE1A00000)
  4. Return to word 0 via R14

Second Entry:
  Word 0 is now NOP, skips decompression
```

### Decompression Code Requirements

**Properties:**
- Must be position-independent
- Must not corrupt its own code
- Must reset word 0 on completion
- Should handle all data before RO section ends
- May use stack for temporary buffers

**Layout in File:**
```
Compressed Image
  ├─ Compressed RO section
  ├─ Compressed RW section
  └─ (Compressed Debug data if present)

Decompression Data
  ├─ Compression dictionaries
  ├─ Huffman trees
  └─ Other algorithm-specific data

Decompression Code
  ├─ Position-independent code
  └─ Algorithm-specific routines
```

### Example: Simple RLE Decompression

```arm
DecompressCode
    ; On entry: R14 = header address
    ; Decompress from word 0x80 onward

    MOV    r0, #0x80            ; output offset
    ADR    r1, CompressedData   ; input offset

DecompLoop
    LDRB   r2, [r1], #1         ; load control byte
    CMPS   r2, #0xFF            ; end marker?
    MOVEQ  pc, r14              ; done, return

    CMPS   r2, #0x80            ; RLE marker?
    BLT    CopyLiteral          ; no, literal byte

    ; RLE sequence
    LDRB   r3, [r1], #1         ; get count
    LDRB   r4, [r1], #1         ; get value

RLELoop
    STRB   r4, [r14, r0]        ; store value
    ADD    r0, r0, #1
    SUBS   r3, r3, #1
    BGT    RLELoop
    B      DecompLoop

CopyLiteral
    STRB   r2, [r14, r0]        ; store literal
    ADD    r0, r0, #1
    B      DecompLoop

CompressedData
    ; Compressed image data follows
```

---

## Zero-Initialization

### Zero-Init Section Overview

**Purpose:**
- Initialize large data areas to zero without bloating file
- Create BSS section
- Prepare workspace for application
- Support stack/heap initialization

**Storage:** Not stored in file (computed size only)

**Creation:** Runtime by zero-init code

### Standard Zero-Init Code

The following 15-word (60-byte) routine is standard across ARM systems:

```arm
ZeroInit
    NOP                          ; 1. Optional debug init instruction
    SUB    ip, lr, pc           ; 2. Calculate header address offset
                                 ;    = base+12+[PSR]-(ZeroInit+12+[PSR])
                                 ;    = base-ZeroInit
    ADD    ip, pc, ip           ; 3. Add PC to offset: base-ZeroInit+ZeroInit+16
                                 ;    = base+16
    LDMIB  ip, {r0,r1,r2,r4}    ; 4. Load sizes:
                                 ;    r0 = RO size (offset 0x14)
                                 ;    r1 = RW size (offset 0x18)
                                 ;    r2 = Debug size (offset 0x1C)
                                 ;    r4 = ZI size (offset 0x20)
    SUB    ip, ip, #16          ; 5. ip = image base
    ADD    ip, ip, r0           ; 6. ip += RO size = RW base
    ADD    ip, ip, r1           ; 7. ip += RW size = ZI base
    MOV    r0, #0               ; 8. Set r0 = 0
    MOV    r1, #0               ; 9. Set r1 = 0
    MOV    r2, #0               ;10. Set r2 = 0
    MOV    r3, #0               ;11. Set r3 = 0
    CMPS   r4, #0               ;12. If ZI size = 0,
00  MOVLE  pc, lr               ;13.   Return to caller
    STMIA  ip!, {r0,r1,r2,r3}   ;14. Store 4 zero words, increment pointer
    SUBS   r4, r4, #16          ;15. Decrement size by 16 bytes, update flags
    BGT    00b                   ;16. If more to zero, loop (16th word)
```

**Word-by-Word Analysis:**

1. **NOP / DBGInit**
   - Often NOP (0xE1A00000)
   - May contain SWI to alert debugger

2. **Header Offset Calculation**
   - On entry: R14 = word 2 address = 0x08
   - PC at SUB instruction = 0x44 (in ZI routine)
   - SUB ip, lr, pc
     - ip = 0x08 - 0x44 = 0xFFFFFFC4 (negative 60 in two's complement)

3. **Calculate Base Address**
   - ADD ip, pc, ip
   - PC at ADD = 0x48
   - ip = 0x48 + 0xFFFFFFC4 = 0x00000010 (header + 16)

4. **Load Sizes**
   - LDMIB ip, {r0,r1,r2,r4}
   - LDMIB = Load Multiple Increment Before (pre-increment)
   - Loads from [ip+4], [ip+8], [ip+12], [ip+16]
   - Corresponds to header offsets 0x14, 0x18, 0x1C, 0x20

5. **Adjust to Image Base**
   - SUB ip, ip, #16
   - ip now points to 0x00 (image base)

6-7. **Calculate ZI Base**
   - ip = base + RO_size + RW_size = ZI base

8-11. **Zero Pattern**
   - Four zero-filled registers for bulk zeroing

12. **Size Check**
   - CMPS r4, #0 sets Z flag if r4 = 0

13. **Conditional Return**
   - MOVLE pc, lr (return if r4 ≤ 0)
   - LE = Less than or Equal

14. **Store Zeros**
   - STMIA ip!, {r0,r1,r2,r3}
   - Store 4 words (16 bytes) and auto-increment pointer

15. **Loop Control**
   - SUBS r4, r4, #16 (subtract 16, update flags)
   - BGT (branch if greater than 0)

### Custom Zero-Init

Systems can provide custom zero-init code by:
1. Providing template in area `ZeroInit_Code` in first object file
2. Different implementations for different architectures
3. Optimization for specific processors

---

## Debug Support

### Debug Type Values

| Value | Meaning | Description |
|-------|---------|-------------|
| 0 | None | No debugging data present |
| 1 | Low-level | Machine-level symbols (variable addresses, function entry points) |
| 2 | Source-level | Source code line information, type definitions (ASD format) |
| 3 | Both | Combined low-level and source-level debugging |

### Debugging Data Format (ASD)

The debugging data uses ARM Symbolic Debugging (ASD) format. This is beyond the scope of this AIF specification (see separate ASD documentation), but consists of:

**Structure:**
- Sections (one per code area)
- Functions (within sections)
- Variables (within functions)
- Types (named type definitions)
- Structs (structure definitions)
- Arrays (array definitions)
- File information (line number mapping)

**Properties:**
- Position-independent (offset-relative references)
- Relocatable (all references are relative to base)
- Copyable (debugger can save separately)
- Can be overwritten by application (must be saved first)

### Debug Data Relocation

**Relocatable References:**
- All references to code/data are absolute addresses after loading
- After relocation, all are relative to new base

**Self-Contained References:**
- All references between debug entries are offsets
- Do not need adjustment after relocation
- Can be copied to safe location as-is

### Debugger Integration

**Typical Debugger Flow:**

```
1. Load AIF file into memory
2. Check Image Debug Type (word 9)
3. If type > 0:
   a. Save debug data to safe location (before relocation)
   b. Modify word 2 (ZeroInit jump):
      - Change from BL to NOP to prevent zero-init
      OR
      - Change from BL to BL <custom_zeroinit>
      - Custom version preserves debug data
4. Execute image (relocation/decompression/zeroinit)
5. Use saved debug data for breakpoints, inspection
```

---

## C Structure Definition

### AIF Header in C

```c
#ifndef AIF_HEADER_H
#define AIF_HEADER_H

#include <stdint.h>

/* ARM Image Format (AIF) Header - 128 bytes */
typedef struct {
    uint32_t bl_decompress_code;    /* 0x00: BL decompress or NOP */
    uint32_t bl_selfreloc_code;     /* 0x04: BL relocation or NOP */
    uint32_t bl_zeroinit_code;      /* 0x08: BL zero-init or NOP */
    uint32_t bl_entry_point;        /* 0x0C: BL entry (exec) or offset (non-exec) */
    uint32_t swi_exit;              /* 0x10: Program exit instruction */
    uint32_t size_ro;               /* 0x14: Read-only section size */
    uint32_t size_rw;               /* 0x18: Read-write section size */
    uint32_t size_debug;            /* 0x1C: Debug section size (bits 0-3: type, 4-31: size) */
    uint32_t size_zeroinit;         /* 0x20: Zero-initialized section size */
    uint32_t debug_type;            /* 0x24: Debug type (0=none, 1=low, 2=source, 3=both) */
    uint32_t image_base;            /* 0x28: Linked image base address */
    uint32_t workspace;             /* 0x2C: Obsolete (was min workspace in RISC OS 2) */
    uint32_t address_mode;          /* 0x30: Address mode (26 or 32) */
    uint32_t data_base;             /* 0x34: Data base (if separate from code) */
    uint32_t reserved_1;            /* 0x38: Reserved (Extended AIF: region offset) */
    uint32_t reserved_2;            /* 0x3C: Reserved */
    uint32_t debug_init_instr;      /* 0x40: Debug init instruction or NOP */
    uint32_t zeroinit_code[15];     /* 0x44-0x7C: 15-word zero-init code routine */
} AIF32Header;

_Static_assert(sizeof(AIF32Header) == 128, "AIF header must be exactly 128 bytes");

/* Helper macros */
#define AIF_HEADER_SIZE                 128
#define AIF_CODE_OFFSET                 0x80

#define AIF_DEBUG_TYPE_NONE             0
#define AIF_DEBUG_TYPE_LOW              1
#define AIF_DEBUG_TYPE_SOURCE           2
#define AIF_DEBUG_TYPE_BOTH             3

#define AIF_MODE_OLD_26BIT              0
#define AIF_MODE_26BIT                  26
#define AIF_MODE_32BIT                  32

#define AIF_MODE_SEPARATE_DATA_MASK     0x0100

/* Check if image is executable AIF */
static inline int aif_is_executable(const AIF32Header *hdr) {
    /* Fourth word (entry point) has BL instruction (0xEB??????) for executable */
    uint32_t entry_word = hdr->bl_entry_point;
    uint8_t msb = (entry_word >> 24) & 0xFF;
    return (msb == 0xEB);
}

/* Get debug type from either word */
static inline int aif_get_debug_type(const AIF32Header *hdr) {
    /* Prefer word 9 (debug_type) over word 7 bits 0-3 */
    if (hdr->debug_type <= 3) {
        return hdr->debug_type;
    }
    /* Fallback to word 7 low 4 bits */
    return (hdr->size_debug & 0x0F);
}

/* Get debug data size */
static inline uint32_t aif_get_debug_size(const AIF32Header *hdr) {
    /* Bits 4-31 of word 7 OR full word 9 if no type in word 7 */
    if ((hdr->size_debug & 0x0F) == 0) {
        /* Use word 9 as debug size */
        return hdr->debug_type;
    }
    return ((hdr->size_debug >> 4) & 0x0FFFFFFF);
}

#endif /* AIF_HEADER_H */
```

### Extended AIF Region Descriptor in C

```c
/* Extended AIF Load Region Descriptor - 44 bytes */
typedef struct {
    uint32_t next_region_offset;    /* 0x00: Offset to next region (0 if last) */
    uint32_t load_address;          /* 0x04: Address to load this region */
    uint32_t size;                  /* 0x08: Size in bytes (must be multiple of 4) */
    uint8_t region_name[32];        /* 0x0C: Region name (null-terminated, zero-padded) */
} AIF_ExtendedRegion;

_Static_assert(sizeof(AIF_ExtendedRegion) == 44, "Extended region must be exactly 44 bytes");
```

---

## Advanced Topics

### Self-Relocation on RISC OS

**Virtual Memory Context:**
- All RISC OS applications load at virtual address 0x8000
- Physical addresses managed by MMU
- Each application gets private virtual address space
- Re-entrancy not required (one app per memory space)

**Relocation Behavior:**
- Relocation code runs immediately after header execution
- Resets word 1 to NOP to prevent re-relocation
- If application re-entered, relocation skipped (word 1 = NOP)

### Thumb Code Support

**Thumb Extension (AIF 3.00+):**
- Area attributes can indicate Thumb code
- Interworking support for ARM/Thumb mixing
- BL/BLX instructions for switching modes
- Relocation of Thumb instructions must account for different encoding

**Detection in Header:**
- Word 12 (Address Mode) bits indicate Thumb capability
- Individual area attributes define actual content
- Debuggers must handle both ARM and Thumb instructions

### Position-Independent Code (PIC)

**Characteristics:**
- No absolute references
- PC-relative addressing only
- Can load at any address
- No relocation needed

**Differences from Relocatable:**
- PIC: No relocation required at load time
- Relocatable: Relocation required once
- PIC is more flexible but requires PIC compilation

---

## File Format Examples

### Example 1: Simple Executable AIF

```
Offset  Hex Value   Description
──────────────────────────────────────────────────────
0x00    E1A00000    Word 0: NOP (no decompression)
0x04    E1A00000    Word 1: NOP (no relocation)
0x08    EB??????    Word 2: BL ZeroInit
0x0C    EB??????    Word 3: BL main
0x10    EF000011    Word 4: SWI OS_Exit
0x14    000000A0    Word 5: RO size = 160 bytes
0x18    00000000    Word 6: RW size = 0
0x1C    00000000    Word 7: Debug size = 0
0x20    00000000    Word 8: ZI size = 0
0x24    00000000    Word 9: Debug type = none
0x28    00008000    Word 10: Image base = 0x8000
0x2C    00000000    Word 11: Work space = 0
0x30    00000020    Word 12: Address mode = 32-bit
0x34    00000000    Word 13: Data base = 0
0x38    00000000    Word 14: Reserved
0x3C    00000000    Word 15: Reserved
0x40    E1A00000    Word 16: DBGInit = NOP
0x44    ...         Words 17-31: ZeroInit code
...
0x80    ...         Code section begins
```

### Example 2: Non-Executable AIF

```
Offset  Hex Value   Description
──────────────────────────────────────────────────────
0x00    E1A00000    Word 0: NOP
0x04    E1A00000    Word 1: NOP
0x08    E1A00000    Word 2: NOP
0x0C    00000080    Word 3: Entry offset = 0x80 (NOT BL instruction)
0x10    EF000011    Word 4: SWI OS_Exit
0x14    00000100    Word 5: RO size = 256 bytes (excludes header)
0x18    00000050    Word 6: RW size = 80 bytes
0x1C    00000000    Word 7: Debug size = 0
0x20    00000100    Word 8: ZI size = 256 bytes
0x24    00000000    Word 9: Debug type = none
0x28    00002000    Word 10: Image base = 0x2000
0x2C    00000000    Word 11: Work space = 0
0x30    00000020    Word 12: Address mode = 32-bit
0x34    00000000    Word 13: Data base = 0
0x38    00000000    Word 14: Reserved
0x3C    00000000    Word 15: Reserved
0x40    E1A00000    Word 16: DBGInit = NOP
0x44    ...         Words 17-31: ZeroInit code
...
0x80    ...         Code section begins (entry point here: 0x2000 + 0x80)
```

---

## Summary Table

| Aspect | Executable AIF | Non-Executable AIF | Extended AIF |
|--------|---|---|---|
| Header in Image | Yes | No | No |
| Load Address | 0x8000 (std) | Variable | Variable |
| Code Address | 0x8080 | Load_Address + 0x80 | Per region |
| Entry Word | BL instruction | Offset value | Offset value |
| Loader Required | Implicit (OS) | Explicit | Explicit |
| Header Size | 128 bytes | 128 bytes | 128 bytes + regions |
| Typical Use | RISC OS apps | Embedded systems | Complex layouts |

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals (PLG-AIF documents)
- ARM STD Reference Guide (ARM Image Format section)
- ARM Symbolic Debugging Format (ASD) specification
- ARM Software Development Toolkit User Guide

