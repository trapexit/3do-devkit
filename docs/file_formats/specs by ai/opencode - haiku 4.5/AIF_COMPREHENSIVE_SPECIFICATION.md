# ARM Image Format (AIF) - Comprehensive Specification

**Version:** 1.0  
**Last Updated:** February 2025  
**Based on:** ARM DUI 0041C, RISC OS PRMs, AIF-1989, AIF-1993

---

## Table of Contents

1. [Overview](#overview)
2. [Format Characteristics](#format-characteristics)
3. [AIF Types](#aif-types)
4. [Overall Structure](#overall-structure)
5. [AIF Header Specification](#aif-header-specification)
6. [Detailed Header Fields](#detailed-header-fields)
7. [Binary Image Section](#binary-image-section)
8. [Relocation Mechanism](#relocation-mechanism)
9. [Compression and Self-Decompression](#compression-and-self-decompression)
10. [Zero-Initialization Code](#zero-initialization-code)
11. [Self-Relocation Code](#self-relocation-code)
12. [Debugging Support](#debugging-support)
13. [Entry Veneers and Position Independence](#entry-veneers-and-position-independence)
14. [Extended AIF Format](#extended-aif-format)
15. [Reference Implementation](#reference-implementation)

---

## Overview

The ARM Image Format (AIF) is a lightweight executable image format designed for ARM microprocessors. It was originally developed by Acorn Computers Ltd for use on the Archimedes computer and RISC OS systems. AIF continues to be used in modern embedded systems, including the 3DO console and various ARM-based devices.

**Key Characteristics:**
- Simple, minimal structure optimized for embedded systems
- Support for self-relocation and self-decompression
- Optional debugging information
- Position-independent code capability
- Zero-initialized data area support
- Reentrant and shared library compatibility

**Primary Use Cases:**
- Executable programs on RISC OS
- Bootloaders and firmware images
- 3DO console applications
- Embedded ARM system images
- Standalone executable kernels

---

## Format Characteristics

### Design Philosophy

AIF is designed with the following principles:

1. **Minimal Overhead:** The format includes only essential information, avoiding unnecessary complexity
2. **Self-Contained:** Images can execute from their load address without external setup (for executable AIF)
3. **Flexibility:** Supports multiple execution modes and runtime configurations
4. **Relocation Support:** Images can relocate themselves or be relocated by loaders
5. **Position Independence:** Code can be made position-independent through appropriate compilation and linking

### Endianness

AIF files are stored in little-endian format (DEC/Intel byte order):
- The least significant byte has the lowest address
- Word values are 32-bit, 4-byte aligned
- Halfword values are 16-bit, 2-byte aligned
- Strings are byte-aligned with null terminators

### Byte Order Considerations

When an AIF file is created, it is created in the byte order of the target ARM system. There is no guarantee that the byte order of the AIF file will match the byte order of the system processing it. Consumers of AIF must be prepared to handle both little-endian and big-endian files.

---

## AIF Types

AIF supports three distinct types, each with different execution characteristics:

### Type 1: Executable AIF

**Characteristics:**
- Can be loaded at its load address and executed immediately from that point
- The AIF header is part of the executable image itself
- Header code prepares the image for execution before jumping to the entry point
- Base address is fixed at header location
- Code starts at `base_address + 0x80`
- Fourth word of header contains: `BL entrypoint` instruction
- Most significant byte is `0xEB` (BL opcode)

**Execution Flow:**
1. Image is loaded into memory at load address (typically 0x8000 on RISC OS)
2. First word (offset 0x00) executes - usually NOP or decompression code
3. Second word (offset 0x04) executes - usually NOP or self-relocation code
4. Third word (offset 0x08) executes - usually NOP or zero-initialization code
5. Fourth word (offset 0x0C) branches to entry point via BL
6. Program counter is available via link register (R14) for position-independent addressing

**Use Cases:**
- Standard RISC OS application executables
- Firmware images that execute from fixed addresses
- Self-relocating images that find their own position in memory

### Type 2: Non-Executable AIF

**Characteristics:**
- Must be processed by an image loader before execution
- Header is not part of the executable image; it only describes the image
- Header is discarded after loading and relocation
- Fourth word contains entry point offset (not a BL instruction)
- Most significant nibble is `0x0`
- Loader is responsible for relocation and zero-initialization

**Execution Flow:**
1. Loader reads AIF header to determine memory requirements
2. Loader allocates memory and loads image at specified address
3. Loader executes relocation code if present
4. Loader executes zero-initialization code
5. Loader jumps to entry point (header.base_address + entry_offset)

**Use Cases:**
- Shared library images
- Relocatable modules
- System overlays
- Images loaded by dynamic linkers

### Type 3: Extended AIF

**Characteristics:**
- Special variant of non-executable AIF
- Contains a scatter-loaded image with multiple load regions
- Header points to chain of region descriptors within the file
- Each region has its own load address and size
- Region descriptors are 44 bytes each
- Non-root regions have their own headers

**Region Descriptor Format (44 bytes):**
```
Offset  Size    Field                Description
0x00    4       Next Region Offset   File offset of next region header (0 if last)
0x04    4       Load Address         Target memory address for region
0x08    4       Region Size          Size in bytes (multiple of 4)
0x0C    32      Region Name          32-byte region name (null-padded)
```

**Use Cases:**
- Multi-segment executable images
- ROM images with scattered sections
- Complex system images with multiple load regions
- Overlay manager systems

---

## Overall Structure

### Uncompressed AIF Layout

```
+---------------------------+
| Header (128 bytes)        |  0x00 - 0x7F
+---------------------------+
| Read-Only Area            |  0x80 - (0x80 + ROSize)
| (Code, Constants, etc.)   |
+---------------------------+
| Read-Write Area           |
| (Initialized Data)        |
+---------------------------+
| Debugging Data (optional) |
+---------------------------+
| Self-Relocation Code      |  (Position-independent)
+---------------------------+
| Relocation List           |  List of offsets, terminated by -1
+---------------------------+
```

### Compressed AIF Layout

```
+---------------------------+
| Header (128 bytes)        |  NOT COMPRESSED
+---------------------------+
| Compressed Image          |  (Decompresses to above format)
+---------------------------+
| Decompression Data        |  (Position-independent)
+---------------------------+
| Decompression Code        |  (Position-independent)
+---------------------------+
```

### Post-Relocation Layout

After execution of self-relocation and zero-initialization code:

```
+---------------------------+
| Header (128 bytes)        |
+---------------------------+
| Read-Only Area            |
+---------------------------+
| Read-Write Area           |
+---------------------------+
| Debugging Data (optional) |
+---------------------------+
| [Rest of memory available |
|  for heap, stack, etc.]   |
+---------------------------+
```

---

## AIF Header Specification

### Header Size and Structure

- **Total Size:** 128 bytes (32 words)
- **Alignment:** 4-byte word aligned throughout
- **Endianness:** Must match target system byte order

### Header Layout

```
Offset  Size    Name                    Description
------  ----    ----                    -----------
0x00    4       BL DecompressCode       Decompression branch or NOP
0x04    4       BL SelfRelocCode        Self-relocation branch or NOP
0x08    4       BL ZeroInitCode         Zero-init branch or NOP
0x0C    4       BL ImageEntryPoint      Entry point branch (exec AIF) or offset (non-exec AIF)
0x10    4       SWI Exit                Program exit instruction (usually SWI 0x11)
0x14    4       Image ReadOnly Size     RO section size (includes header if executable AIF)
0x18    4       Image ReadWrite Size    RW section size (exact, multiple of 4)
0x1C    4       Image Debug Size        Debug section size (bits 0-3: type, bits 4-31: LL debug size)
0x20    4       Image Zero-Init Size    Zero-initialized section size (multiple of 4)
0x24    4       Image Debug Type        0=None, 1=LL, 2=Source, 3=Both
0x28    4       Image Base              Linked load address of code section
0x2C    4       Work Space              Minimum free space requirement (for relocatable images)
0x30    4       Address Mode            26, 32, or 0; bit 8 set = separate data base
0x34    4       Data Base               Linked load address of data section (if Address Mode bit 8 set)
0x38    8       Reserved                Two reserved words (initially 0, used by Extended AIF)
0x40    4       Debug Init              Debug initialization instruction or NOP
0x44    60      Zero-Init Code          Standard zero-initialization code (15 words)
```

---

## Detailed Header Fields

### Word 0x00: BL DecompressCode

**Encoding:**
- **Value:** `0xEB000000` (BL instruction) if image is compressed
- **Value:** `0xE1A00000` (NOP, MOV r0,r0) if image is not compressed

**Purpose:**
- Branches to decompression code if the entire image is compressed
- The branch is PC-relative, allowing position-independent execution
- After decompression, this word must be reset to NOP to make image reentrant

**ARM Assembly:**
```arm
BL DecompressCode      ; Offset calculation: (&decompCode - &BL - 8) >> 2
```

### Word 0x04: BL SelfRelocCode

**Encoding:**
- **Value:** `0xEB000000` (BL instruction) if image is self-relocating
- **Value:** `0xE1A00000` (NOP) if image is not self-relocating

**Purpose:**
- Branches to self-relocation code for position-independent execution
- Allows image to be loaded at any address and relocate itself
- After relocation, this word must be reset to NOP

**Relocation Mechanism:**
- Code calculates actual load address versus linked address
- Adjusts all relocatable references in the image
- Replaces itself with NOP after execution

### Word 0x08: BL ZeroInitCode

**Encoding:**
- **Value:** `0xEB000000` (BL instruction) if zero-initialization code is present
- **Value:** `0xE1A00000` (NOP) if not present

**Purpose:**
- Branches to zero-initialization code
- Initializes the zero-initialized data area to all zeros
- Typically contains a loop that stores zeros in 16-byte chunks

**Call Convention:**
- R14 (link register) points to the instruction address for position independence
- Must return via MOVS PC, LR to restore execution flags
- Modifies R0-R4 and IP; preserves other registers

### Word 0x0C: BL ImageEntryPoint / Entry Point Offset

**For Executable AIF:**
- **Encoding:** `0xEB??????` (BL instruction)
- **Offset:** (entrypoint_address - header_address - 8) >> 2
- **Most Significant Byte:** `0xEB` (BL opcode)

**For Non-Executable AIF:**
- **Encoding:** Plain 32-bit offset value
- **Meaning:** Offset from image base address to entry point
- **Most Significant Nibble:** `0x0` (not a BL instruction)

**Entry Point Considerations:**
- Entry point may differ from linked base + 0x80 if there's C runtime initialization
- Shared library stubs require special entry veneers
- May point to language runtime initialization code

### Word 0x10: Program Exit Instruction

**Standard Value:** `0xEF000011` (SWI 0x11, OS_Exit on RISC OS)

**Purpose:**
- Provides fallback exit mechanism if program returns to header
- Should never execute in normal operation
- Customizable via AIF_HDR template in first object file

**Alternative Values:**
- `0xEAFFFFFE` (infinite loop, branch to self)
- System-specific SWI codes for other platforms

### Word 0x14: Image ReadOnly Size

**Value Range:** 0 to 2^32 - 1 bytes

**Includes:**
- For executable AIF: The AIF header itself (128 bytes minimum)
- Code section
- Read-only data and constants
- All relocatable references in RO section

**Excludes:**
- Read-write section
- Debug section
- Self-relocation code
- Relocation list

**Alignment:** Multiple of 4 bytes

**Position in Image:** RO area starts at offset 0x00 and extends to 0x80 + ROSize

### Word 0x18: Image ReadWrite Size

**Value Range:** 0 to 2^32 - 1 bytes

**Contents:**
- Initialized data section
- Static variables with non-zero initial values
- Global data structures

**Alignment:** Must be multiple of 4 bytes

**Position in Image:** RW area follows RO area immediately

**Memory Layout:**
```
Image Base              = RO Base
Image Base + RO Size    = RW Base
Image Base + RO + RW    = ZI Base
```

### Word 0x1C: Image Debug Size

**Bit Layout:**
```
Bits    Field               Description
----    -----               -----------
0-3     Debug Type          0=None, 1=LL, 2=Source, 3=Both
4-31    LL Debug Size       Low-level debug section size in bytes
```

**Total Debug Size:** (DebugSize & 0xFFFFFFFF) = Low-level debug section size

**Contents:**
- Low-level debugging tables (if bit 0 or 3)
- High-level/source-level debugging tables (if bit 1 or 3)
- Symbol tables, line number information
- File and function metadata

**Absence:** If no debugging data is present, entire field is 0

### Word 0x20: Image Zero-Init Size

**Value Range:** 0 to 2^32 - 1 bytes

**Purpose:**
- Specifies size of uninitialized data area that must be zeroed at runtime
- Linker marks areas with ZI attribute separately

**Alignment:** Must be multiple of 4 bytes (typically 16 bytes)

**Memory Layout:**
```
Zero-Init Base = Image Base + RO Size + RW Size
Zero-Init Size = Value at 0x20
```

**Runtime Initialization:**
- Zero-init code initializes this entire region to 0x00000000
- Typical loop: Store 0 in 16-byte chunks until size reached
- Required for uninitialized static data, heap placeholder, stack space

### Word 0x24: Image Debug Type

**Values:**
```
Value   Meaning
-----   -------
0       No debugging data present
1       Low-level (symbolic assembler) debugging data present
2       Source-level (ASD, high-level) debugging data present
3       Both low-level and source-level debugging data present
```

**Impact on Execution:**
- Debuggers use this field to determine what debugging facilities are available
- No impact on program execution
- Debugger may copy debug section before it's overwritten by heap/stack

### Word 0x28: Image Base

**Value:** Address where image was linked (usually 0x8000 for RISC OS executables)

**Purpose:**
- Reference point for relocation calculations
- Used by self-relocation code to determine actual vs. expected position
- Enables position-independent execution

**Calculation:**
```
Relocation_Offset = Actual_Load_Address - Image_Base
```

**On RISC OS:**
- Typically 0x8000 (32KB) for application images
- Virtual address through MMU (not physical address)
- Relocatable images may use different bases

### Word 0x2C: Work Space

**Value Range:** 0 to 0x0FFFFFFF (30-bit value for size)

**Bit Interpretation:**
```
Bits    Field               Meaning
----    -----               -------
0-23    Stack Size          Amount of stack space to reserve (in bytes)
24-29   Reserved            (typically 0)
30      3DO Header Flag     0x40000000 = 3DO extended header follows
31      (Reserved)          0
```

**Purpose:**
- Specifies minimum free space required above the image
- Used by relocatable images that move themselves
- Indicates 3DO extended header format on 3DO systems

**Calculation for Relocatable Images:**
```
Space_Required = Zero_Init_Size + Work_Space
Target_Address = Memory_Limit - Space_Required
```

**3DO Extension:**
- If bit 30 is set (0x40000000), a 128-byte 3DO binary header follows
- Contains task parameters, flags, memory requirements

### Word 0x30: Address Mode

**Encoding:**
```
Bits    Field               Description
----    -----               -----------
0-7     ARM Mode            26 = 26-bit ARM mode
                            32 = 32-bit ARM mode
                            0  = Old-style 26-bit (default)
8       Data Base Flag      0 = Code and data at same base
                            1 = Separate data base (word 0x34 valid)
9-31    Reserved            Must be 0
```

**Meaning:**
- **26-bit Mode:** Code uses 26-bit addressing, may not run in 32-bit mode
- **32-bit Mode:** Code uses 32-bit addressing, may not run in 26-bit mode
- **0 (Legacy):** Old AIF format, assume 26-bit mode

**APCS Compliance:**
- Indicates which ARM Procedure Call Standard variant was used
- Affects register usage, return address handling, stack frame layout

### Word 0x34: Data Base

**Value:** Address where image data section was linked

**Valid When:** Word 0x30, bit 8 is set

**Purpose:**
- Allows code and data to be linked at different addresses
- Enables ROM code with writable data in RAM
- Used in some embedded systems for XIP (eXecute In Place) configurations

**Memory Layout (Separate Bases):**
```
Code Base (0x28)   ->  Code section (RO area)
Data Base (0x34)   ->  Data section (RW area)
```

**Relocation for Separate Bases:**
```
Code_Relocation_Offset = Actual_Code_Base - Linked_Code_Base
Data_Relocation_Offset = Actual_Data_Base - Linked_Data_Base
```

### Words 0x38-0x3F: Reserved

**Standard Usage:** Set to 0x00000000 for executable AIF

**Extended AIF Usage:**
- Word 0x38: Contains file offset of first non-root load region descriptor
- Word 0x3C: Reserved (set to 0)

### Word 0x40: Debug Init Instruction

**Standard Encoding:** `0xE1A00000` (NOP, MOV r0,r0)

**Alternative:** `0xEF041D41` (RISC OS DDE_Debug SWI)

**Purpose:**
- Optional instruction to alert debugger of image startup
- Executed before zero-initialization
- Allows debugger to seize control before image executes

**On RISC OS:**
- Default: NOP (no special action)
- With debugger: SWI to DDE_Debug service

**On 3DO:**
- PORTFOLIO_DEBUG_TASK (0xEF00010A) for debugger support

### Words 0x44-0x7C: Zero-Init Code (15 words, 60 bytes)

**Standard Implementation:**

```arm
        BIC     IP, LR, #&FC000003      ; clear status bits -> header + &C
        ADD     IP, IP, #8              ; -> Image ReadOnly size
        LDMIA   IP, {R0,R1,R2,R3}       ; various sizes
        CMPS    R3, #0
        MOVLES  PC, LR                  ; nothing to do
        SUB     IP, IP, #&14            ; image base
        ADD     IP, IP, R0              ; + RO size
        ADD     IP, IP, R1              ; + RW size = base of 0-init area
        MOV     R0, #0
        MOV     R1, #0
        MOV     R2, #0
        MOV     R4, #0
ZeroLoop
        STMIA   IP!, {R0,R1,R2,R4}      ; zero 16 bytes at a time
        SUBS    R3, R3, #16
        BGT     ZeroLoop
        MOVS    PC, LR                  ; return
```

**Behavior:**
1. Calculates actual header address from LR and entry instructions
2. Loads size fields (RO, RW, Zero-Init)
3. Calculates zero-init area base address
4. Fills 16-byte chunks with zeros until complete
5. Returns to caller via PC = LR

**Position Independence:**
- Uses BIC to clear status bits from R14
- Calculates all addresses relative to R14
- Works correctly when header is at any address

**Register Usage:**
- R0, R1, R2, R3, R4: Scratch (destroyed)
- IP: Base address calculation
- LR: Entry point (preserved for return)
- Other registers: Preserved

---

## Binary Image Section

### Read-Only Area (RO)

**Location:** Offset 0x80 from image base (immediately after header)

**Size:** Specified in header field 0x14

**Contents:**
- Executable code
- Read-only constants
- Code relocation directives
- ARM instruction sequences

**Characteristics:**
- May be write-protected at runtime
- Must be relocatable if image relocates
- Position-independent for shared code

### Read-Write Area (RW)

**Location:** Immediately after RO area

**Size:** Specified in header field 0x18

**Contents:**
- Initialized static data
- Global variables with non-zero initializers
- Static arrays and structures
- Any mutable data linked at compile time

**Characteristics:**
- Must be writable at runtime
- Copied/relocated with RO area
- Not zero-initialized (already contains initial values)

### Zero-Initialized Area (ZI)

**Location:** Immediately after RW area (not in image file)

**Size:** Specified in header field 0x20

**Contents:**
- Uninitialized static data
- Arrays allocated but not initialized
- Bss section equivalents

**Characteristics:**
- Not stored in image file (saves space)
- Allocated and zeroed by zero-init code at runtime
- Not relocated (location determined by RO + RW + offset calculation)

### Debugging Data

**Location:** Immediately after RW area (if present)

**Size:** Specified in header field 0x1C (bits 4-31)

**Structure:**
- Low-level debug tables (offset information, registers)
- Source-level debug tables (variables, line numbers, files)
- Symbol table references

**Accessibility:**
- Optional; may be absent or stripped from release builds
- Debugger copies this section before execution overwrites it
- Debug data uses relocatable references for position independence

---

## Relocation Mechanism

### Relocation List Format

**Location:** Immediately after self-relocation code (or RW area if no relocation code)

**Structure:**
```
Word    Content
----    -------
0       Offset of word 1 to relocate (from image base)
1       Offset of word 2 to relocate
...
N       Offset of word N to relocate
N+1     -1 (0xFFFFFFFF) - list terminator
```

**Processing:**
1. Read first offset from list
2. If offset == -1, stop processing
3. Load word at (image_base + offset)
4. Add relocation value to word
5. Store result back at (image_base + offset)
6. Repeat from step 1

### Relocation Values

**Base Relocation:**
```
Relocated_Value = Original_Value + (Actual_Base - Linked_Base)
```

**PC-Relative Relocation:**
For branch targets and PC-relative references:
```
Relocated_Value = Original_Value + (Target_Address - Location_Address)
```

### Self-Relocation Code

**Purpose:**
- Executes at runtime to relocate image from linked address to actual address
- Enables position-independent execution
- Allows single image to run from any memory location

**Typical Implementation:**

```arm
RelocCode
        BIC     IP, LR, #&FC000003      ; clear flag bits; -> header + &08
        SUB     IP, IP, #8              ; -> header address
        MOV     R0, #&FB000000          ; BLNV #0
        STR     R0, [IP, #4]            ; mark relocation as done
        
        LDR     R1, [IP, #&28]          ; header + &28 = code base set by Link
        SUBS    R1, IP, R1              ; relocation offset
        MOVEQ   PC, LR                  ; no relocation needed
        STR     IP, [IP, #&28]          ; update image base to actual address
        ADR     R2, End                 ; start of reloc list
        
RelocLoop
        LDR     R0, [R2], #4            ; offset of word to relocate
        CMNS    R0, #1                  ; terminator?
        MOVEQS  PC, LR                  ; yes => return
        LDR     R3, [IP, R0]            ; word to relocate
        ADD     R3, R3, R1              ; relocate it
        STR     R3, [IP, R0]            ; store it back
        B       RelocLoop               ; and do the next one
        
End                                     ; relocation list starts here
        DCD     reloc_offset_1
        DCD     reloc_offset_2
        ...
        DCD     -1                      ; terminator
```

**Execution Flow:**
1. Calculate actual header address from R14
2. Mark relocation as complete (set word 0x04 to NOP)
3. Calculate relocation offset (actual - linked)
4. Update Image Base field to actual address
5. Loop through relocation list and adjust each reference
6. Return to entry point

### Relocation Considerations

**One-Time Relocation:**
- After first relocation, image is "fixed" in memory
- Relocation list is overwritten by zero-init data
- Image cannot be relocated again

**Re-entrance:**
- Image is not reentrant after relocation
- Can be restarted but not by re-running from beginning
- Requires resetting header words and restoring original values

---

## Compression and Self-Decompression

### Compressed Image Format

**Layout:**
```
Header (uncompressed)
Compressed_RO_Area
Compressed_RW_Area
Decompression_Data
Decompression_Code
```

**Header Indication:**
- Word 0x00: Points to decompression code (BL with non-zero offset)
- Word 0x00 is NOT the standard NOP instruction

### Decompression Algorithm

**Process:**
1. Decompression code executes at startup
2. Code decompresses image in-place or to separate area
3. After decompression, word 0x00 is reset to NOP
4. Execution continues with decompressed image

**Position Independence:**
- Decompression code must be position-independent
- Uses PC-relative addressing
- May relocate decompressed image as needed

### Decompression Implementation

**Typical approach:**
- Data is compressed using simple scheme (RLE, LZ77, etc.)
- Decompressor uncompresses to temporary area or in-place
- Must ensure decompressor code isn't overwritten during decompression
- Decompressor may be in high memory, compressed low in file

**Advantages:**
- Reduces image size for slow media (floppy disk, flash)
- Improves loading speed despite decompression overhead
- Transparent to application code

---

## Zero-Initialization Code

### Purpose and Behavior

The zero-initialization code initializes the ZI area (zero-initialized data section) to all zeros. This area consists of uninitialized static data that must be cleared before program execution begins.

### Code Structure

```arm
BIC     IP, LR, #&FC000003             ; Entry: R14 -> ZeroInit + 4 or 8
                                        ; Clear PSR bits -> header + 0xC
ADD     IP, IP, #8                      ; Adjust to point to RO size field (0x14)
LDMIA   IP, {R0,R1,R2,R3}              ; Load 4 size values:
                                        ; R0 = RO size
                                        ; R1 = RW size
                                        ; R2 = Debug size
                                        ; R3 = ZI size
CMPS    R3, #0
MOVLES  PC, LR                          ; If ZI size = 0, return immediately
SUB     IP, IP, #&14                    ; Adjust IP back to point to header
ADD     IP, IP, R0                      ; IP = header + RO size
ADD     IP, IP, R1                      ; IP = header + RO + RW (start of ZI)
MOV     R0, #0
MOV     R1, #0
MOV     R2, #0
MOV     R4, #0                          ; Setup zero values (16 bytes worth)
ZeroLoop
STMIA   IP!, {R0,R1,R2,R4}             ; Store 16 bytes of zeros, increment IP
SUBS    R3, R3, #16                     ; Decrement size counter
BGT     ZeroLoop                         ; Loop if more to zero
MOVS    PC, LR                          ; Return with PSR flags from result
```

### Detailed Analysis

**Position Independence:**
- Uses BIC to mask status bits from LR
- All address calculations relative to actual load address
- Works correctly regardless of where image is loaded

**Register Calculations:**
1. Entry: `R14 = ZeroInit_Address + 4 (or 8 with offset bits)`
2. BIC: `IP = (R14 & ~0xFC000003) = ZeroInit_Address + 0xC (approximately)`
3. ADD IP, IP, #8: `IP = ZeroInit_Address + 0x14` (header offset 0x14)
4. LDMIA loads 4 words starting at 0x14:
   - Offset 0x14: RO Size (into R0)
   - Offset 0x18: RW Size (into R1)
   - Offset 0x1C: Debug Size (into R2)
   - Offset 0x20: ZI Size (into R3)
5. SUB IP, IP, #0x14: Back to header start
6. ADD IP, IP, R0: Skip RO area
7. ADD IP, IP, R1: Skip RW area → Now pointing to ZI base

**Zeroing Loop:**
- STMIA stores 4 words (16 bytes) at a time
- Post-increment addressing (IP!) automatically increments pointer
- SUBS updates condition flags based on remaining count
- BGT branches if count > 0
- MOVS returns with flags from last SUBS (Z flag = 1 if exactly zeroed)

**Register Preservation:**
- Only modifies: R0, R1, R2, R3, R4, IP
- Preserves: R5-R13, R14 (uses MOVS PC, LR to return)
- Safe to call from any context

### Optimization Variants

**Single-Word Zeroing:**
For small ZI areas or alignment, simple loop:
```arm
MOV    R0, #0
.Loop
SUBS   R3, R3, #4
STRGE  R0, [IP, R3]
BGE    .Loop
```

**128-Bit Vectorization:**
For modern ARM with NEON (not in standard AIF):
```arm
VMOV   Q0, #0
.Loop
VSTMIA IP!, {Q0}
SUBS   R3, R3, #16
BGT    .Loop
```

---

## Self-Relocation Code

### Complete Self-Relocation and Self-Move Implementation

**Purpose:**
- Relocate absolute address references to match actual load position
- Optionally move image to high memory for application use

**Implementation:**

```arm
RelocCode
        BIC     IP, LR, #&FC000003      ; Clear flag bits; IP -> header + &08
        SUB     IP, IP, #8              ; Adjust to header start
        MOV     R0, #&FB000000          ; BLNV #0 (NOP-like, never executes)
        STR     R0, [IP, #4]            ; Reset word 0x04 to prevent re-relocation
        
; Check if image needs to be moved
        LDR     R9, [IP, #&2C]          ; Load Work Space field
        CMPS    R9, #0                  ; Test if zero
        BEQ     RelocateOnly            ; No move, just relocate
        
; Calculate move distance
        LDR     R0, [IP, #&20]          ; Load ZI size
        ADD     R9, R9, R0              ; Space needed = WS + ZI size
        SWI     GetEnv                  ; Get MemLimit in R1
        ADR     R2, End                 ; Address of relocation list end
        
; Find end of relocation list to know image size
01      LDR     R0, [R2], #4            ; Load offset from list
        CMNS    R0, #1                  ; Check for terminator (-1)
        BNE     %B01                     ; Loop until end
        
        SUB     R3, R1, R9              ; Target = MemLimit - required space
        SUBS    R0, R3, R2              ; Amount to move
        BLE     RelocateOnly            ; Not enough space, relocate in place
        BIC     R0, R0, #15             ; Round down to 16-byte boundary
        
        ADD     R3, R2, R0              ; New end address
        ADR     R8, %F01                ; Set intermediate limit
        
; Copy image up memory in descending order (in-place safe)
01      LDMDB   R2!, {R4-R7}            ; Load 16 bytes (descending)
        STMDB   R3!, {R4-R7}            ; Store 16 bytes (descending)
        CMP     R2, R8                  ; Check if copy-loop copied
        BGT     %B01                     ; Continue until copy loop reached
        
        ADD     R4, PC, R0              ; Calculate jump address
        MOV     PC, R4                  ; Jump to copied code
        
; Code executing from new location continues relocation
01      LDMDB   R2!, {R4-R7}
        STMDB   R3!, {R4-R7}
        CMP     R2, IP                  ; Check if everything copied
        BGT     %B01
        
        ADD     IP, IP, R0              ; Update header address
        ADD     LR, LR, R0              ; Update return address
        
; Relocation of address constants
RelocateOnly
        LDR     R1, [IP, #&28]          ; Load linked Image Base
        SUBS    R1, IP, R1              ; Calculate relocation offset
        MOVEQ   PC, LR                  ; No relocation if offset is 0
        
        STR     IP, [IP, #&28]          ; Update Image Base to actual address
        ADR     R2, End                 ; Start of relocation list
        
RelocLoop
        LDR     R0, [R2], #4            ; Load relocation offset
        CMNS    R0, #1                  ; Check for terminator
        MOVEQS  PC, LR                  ; Exit if done
        
        LDR     R3, [IP, R0]            ; Load word to relocate
        ADD     R3, R3, R1              ; Add relocation offset
        STR     R3, [IP, R0]            ; Store back
        B       RelocLoop               ; Continue
        
End                                     ; Start of relocation list
        DCD     offset1
        DCD     offset2
        ...
        DCD     -1                      ; Terminator
```

### Relocation Algorithm Details

**Step 1: Mark as Relocated**
```arm
BIC     IP, LR, #&FC000003
SUB     IP, IP, #8
MOV     R0, #&FB000000
STR     R0, [IP, #4]
```
Sets word 0x04 to a non-standard instruction to prevent re-relocation on restart.

**Step 2: Check for Self-Move**
```arm
LDR     R9, [IP, #&2C]
CMPS    R9, #0
BEQ     RelocateOnly
```
If Work Space field is 0, skip the move phase.

**Step 3: Calculate Move Parameters**
```arm
LDR     R0, [IP, #&20]
ADD     R9, R9, R0
SWI     GetEnv
```
- R0 = Zero-Init size
- R9 = Work Space + Zero-Init size = total space needed
- R1 = Memory limit (from GetEnv)
- R3 = Target address = MemLimit - required_space

**Step 4: In-Place Copy**
Uses descending-order copy (LDMDB/STMDB) to safely copy image up in memory without overwriting source before it's copied.

**Step 5: Update Pointers**
```arm
ADD     IP, IP, R0
ADD     LR, LR, R0
```
After move, adjust header address and return address by move distance.

**Step 6: Relocation Loop**
Processes relocation list, adding offset to each referenced word.

---

## Debugging Support

### Debugging Levels

**Type 0: No Debugging**
- Image contains no debugging data
- No debugger support available
- Smallest image size

**Type 1: Low-Level Debugging (Assembler-level)**
- Symbolic assembler debugging tables included
- Maps addresses to assembler symbols, registers
- No source code line information
- Debugger can show instruction-level context

**Type 2: Source-Level Debugging (High-level)**
- Source-level debugging tables (ASD format)
- Maps addresses to source lines, variables, types
- Includes file and line number information
- Debugger can show source code context

**Type 3: Both Low and High Level**
- Complete debugging support
- Both assembler and source-level information
- Maximum debugger capability

### Debug Data Structure

**Format:**
- Follows all initialized data (RO + RW sections)
- Consists of typed records with length headers
- Each record: 4-byte header + variable data

**Header Format:**
```
Byte    Field       Description
----    -----       -----------
0       Class       Record type (8-bit)
1       ClassData   Class-specific data (8-bit)
2-3     Length      Record size including header (16-bit)
```

**Record Types:**
- Section: Starts new debug section (file)
- Function: Function entry point
- Variable: Local or static variable
- Type: Named type definition
- Struct: Structure layout
- Array: Array bounds
- ... (others defined by ASD specification)

### Debugger Data Handling

**Before Relocation:**
- Debugger copies debug section to safe location
- Preserves all debug data outside image memory

**During Execution:**
- After relocation, debug area may be overwritten by heap/stack
- Pre-copied data remains valid for debugging

**Relocation Awareness:**
- Debug references use relocatable addresses
- Debugger must apply same relocation offset as code
- All inter-record references are relative offsets (position-independent)

---

## Entry Veneers and Position Independence

### Direct Entry Veneers

For reentrant, shared code libraries:

```arm
FunctionName
        ADD    ip, sb, #V1              ; Load offset from static base register
        ADD    ip, ip, #V2              ; Add second part if needed (for large offsets)
        LDMIA  ip, {ip, pc}             ; Load new SB and entry point
```

**Data Section (in based area):**
```arm
        DCD    new-sb                   ; Static base for called library
        DCD    entry-point              ; Entry point address
```

### Indirect Entry Veneers

For function pointers or non-reentrant calls:

```arm
FunctionName
        ADD    ip, pc, #0               ; IP = PC + 8
        LDMIA  ip, {ip, pc}             ; Load values and jump
        DCD    new-sb
        DCD    entry-point
```

### Position Independence Implementation

**Requirements:**
- All code PC-relative or register-relative
- No absolute address literals in code
- Data addressed through base register
- Entry point veneers for inter-library calls

**Techniques:**
- MOV pc, lr: Used extensively for position independence
- ADR instruction: Position-relative addressing
- PC-relative branches: BL, B instructions
- Based addressing: LDR/STR with base register

---

## Extended AIF Format

### Region Descriptor Chain

For scatter-loaded images:

**Word 0x38 (First Region Offset):**
- Non-zero: File offset of first region descriptor
- Zero: No extended regions (simple AIF)

**Region Header Format (44 bytes):**

```
Offset  Size    Field               Description
------  ----    -----               -----------
0x00    4       Next Region Ptr     Offset to next region (0 if last)
0x04    4       Load Address        Target memory address
0x08    4       Region Size         Bytes (multiple of 4)
0x0C    32      Region Name         32-char name (null-padded)
```

### Extended AIF Layout

```
Main AIF Header (128 bytes, word 0x38 != 0)
Region 1 Data
Region 2 Data
...
Region 1 Descriptor (points to Region 2)
Region 2 Descriptor (points to Region 3 or 0)
...
```

### Loading Extended AIF

1. Read main header, note Region 1 offset at 0x38
2. Load main image (RO + RW areas)
3. Follow region chain:
   - Read region descriptor
   - Load region data at specified address
   - Follow next pointer
   - Repeat until next pointer = 0

### Use Cases

- Multi-segment kernels
- ROM/RAM split systems
- Complex memory layouts
- Overlay management

---

## Reference Implementation

### C Structure Definition

```c
typedef struct {
    uint32_t bl_decompress;         /* 0x00: BL DecompressCode or NOP */
    uint32_t bl_self_reloc;         /* 0x04: BL SelfRelocCode or NOP */
    uint32_t bl_zero_init;          /* 0x08: BL ZeroInitCode or NOP */
    uint32_t bl_entry;              /* 0x0C: BL EntryPoint or offset */
    uint32_t swi_exit;              /* 0x10: SWI OS_Exit */
    int32_t  size_ro;               /* 0x14: ReadOnly size */
    int32_t  size_rw;               /* 0x18: ReadWrite size */
    int32_t  size_debug;            /* 0x1C: Debug size (with type in bits 0-3) */
    int32_t  size_zi;               /* 0x20: Zero-Init size */
    uint32_t debug_type;            /* 0x24: 0=None, 1=LL, 2=Src, 3=Both */
    uint32_t image_base;            /* 0x28: Linked base address */
    int32_t  workspace;             /* 0x2C: Min free space (lower 30 bits) */
    uint32_t address_mode;          /* 0x30: 26/32 mode indicator */
    uint32_t data_base;             /* 0x34: Separate data base address */
    uint32_t reserved[2];           /* 0x38: Reserved (or ext region offset) */
    uint32_t debug_init;            /* 0x40: Debug init instruction */
    uint32_t zero_init_code[15];    /* 0x44: Zero-init routine (60 bytes) */
} AIFHeader;

#define AIF_HEADER_SIZE 128
#define AIF_RO_OFFSET   0x80
#define AIF_NOP         0xE1A00000  /* MOV R0, R0 */
#define AIF_BL_BASE     0xEB000000  /* BL instruction with offset = 0 */
#define AIF_OS_EXIT     0xEF000011  /* SWI 0x11 */

/* Check if AIF is compressed */
#define IS_COMPRESSED(hdr) ((hdr)->bl_decompress != AIF_NOP)

/* Check if AIF is self-relocating */
#define IS_SELF_RELOCATING(hdr) ((hdr)->bl_self_reloc != AIF_NOP)

/* Check if AIF has zero-init code */
#define HAS_ZERO_INIT(hdr) ((hdr)->bl_zero_init != AIF_NOP)

/* Extract debug type from size field */
#define GET_DEBUG_TYPE(debug_size) ((debug_size) & 0x0F)

/* Extract low-level debug size */
#define GET_LL_DEBUG_SIZE(debug_size) (((debug_size) >> 4) & 0x0FFFFFFF)

/* Check if image is executable AIF (fourth word is BL instruction) */
#define IS_EXECUTABLE_AIF(hdr) (((hdr)->bl_entry & 0xFF000000) == 0xEB000000)

/* Get entry offset for non-executable AIF */
#define GET_ENTRY_OFFSET(hdr) ((hdr)->bl_entry & 0x00FFFFFF)
```

### Loading Function

```c
int load_aif(const char *filename, uint32_t load_address) {
    FILE *f = fopen(filename, "rb");
    if (!f) return -1;
    
    AIFHeader hdr;
    if (fread(&hdr, sizeof(AIFHeader), 1, f) != 1) {
        fclose(f);
        return -1;
    }
    
    /* Allocate memory */
    uint32_t total_size = hdr.size_ro + hdr.size_rw + hdr.size_zi;
    uint8_t *image = malloc(total_size);
    if (!image) {
        fclose(f);
        return -1;
    }
    
    /* Read image data */
    if (fread(image, hdr.size_ro + hdr.size_rw, 1, f) != 1) {
        free(image);
        fclose(f);
        return -1;
    }
    
    /* Zero-initialize ZI area */
    memset(image + hdr.size_ro + hdr.size_rw, 0, hdr.size_zi);
    
    /* Handle relocation */
    if (IS_SELF_RELOCATING(&hdr)) {
        int reloc_offset = load_address - hdr.image_base;
        
        /* Update header base */
        uint32_t *base_ptr = (uint32_t *)(image + 0x28);
        *base_ptr += reloc_offset;
        
        /* Process relocation list (at end of image) */
        uint32_t *reloc_list = (uint32_t *)(image + hdr.size_ro + hdr.size_rw);
        for (int i = 0; reloc_list[i] != 0xFFFFFFFF; i++) {
            uint32_t offset = reloc_list[i];
            uint32_t *addr = (uint32_t *)(image + offset);
            *addr += reloc_offset;
        }
    }
    
    /* Copy to target address */
    memcpy((void *)load_address, image, total_size);
    free(image);
    fclose(f);
    
    /* Jump to entry point */
    typedef void (*entry_func)(void);
    uint32_t entry_point = load_address + 0x80;
    if (!IS_EXECUTABLE_AIF(&hdr)) {
        entry_point = load_address + GET_ENTRY_OFFSET(&hdr);
    }
    
    entry_func entry = (entry_func)entry_point;
    entry();
    
    return 0;
}
```

---

## Summary

The ARM Image Format provides a simple yet powerful mechanism for executable images on ARM systems. Its support for self-relocation, compression, and position-independent code makes it suitable for a wide range of applications from RISC OS executables to 3DO games to embedded firmware images.

Key design features:
- Minimal overhead (128-byte header)
- Three execution modes (executable, non-executable, extended)
- Flexible relocation mechanisms
- Optional debugging support
- Position independence for ROM/shared code
- Compact format suitable for slow media

The format has proven durable and effective over decades of use, remaining relevant in modern ARM-based systems despite the emergence of newer formats like ELF.

---

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- RISC OS Programmer's Reference Manuals
- AIF-1989: Original RISC OS Application Image Format specification
- AIF-1993: Revised specification with enhanced features
