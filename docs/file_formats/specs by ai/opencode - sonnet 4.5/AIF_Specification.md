# ARM Image Format (AIF) - Complete Specification

## Document Information

**Format Name:** ARM Image Format (AIF) / Acorn Image Format  
**Also Known As:** RISC OS Application Image Format, Arthur Image Format  
**Purpose:** Executable program images for ARM processors  
**Platform:** RISC OS, 3DO, embedded ARM systems  
**Header Size:** 128 bytes (32 words)  
**Base Load Address (typical):** 0x8000 (RISC OS)  

---

## 1. Overview

ARM Image Format (AIF) is a simple, efficient format for ARM executable images consisting of:

1. **128-byte header** - Contains metadata and initialization code
2. **Binary image** - Executable code and initialized data
3. **Optional features** - Debugging data, self-relocation, compression

### 1.1 Key Features

- **Self-contained execution** - Loaded and entered at first word
- **Self-initialization** - Can decompress, relocate, zero-initialize itself
- **Position independence** - Can execute at any address (if created with appropriate options)
- **Debug support** - Integrates with ARM Symbolic Debugger (ASD)
- **Compression support** - Self-decompressing images for faster loading
- **Legacy compatibility** - Compatible with old Arthur/Brazil ADFS images

### 1.2 AIF Variants

Three main variants exist:

| Variant | Header Status | Entry Point | Use Case |
|---------|--------------|-------------|----------|
| **Executable AIF** | Part of image | First word of header | RISC OS applications, standalone programs |
| **Non-executable AIF** | Describes image only | Offset from base | Loader-based systems, embedded applications |
| **Extended AIF** | Describes scatter-loaded image | Chain of descriptors | Complex memory layouts, ROM images |

---

## 2. Fundamental Concepts

### 2.1 Byte Ordering

AIF images use **little-endian** byte ordering (LSB first):
- Words: 32 bits, LSB at lowest address
- ARM instruction encoding follows ARM architecture byte order
- Consistent with DEC/Intel byte sex

### 2.2 Address Modes

AIF supports both ARM addressing modes:

| Mode | Value at 0x30 | Description |
|------|---------------|-------------|
| **26-bit** | 26 | Legacy mode; PSR in top bits of PC |
| **32-bit** | 32 | Modern mode; separate CPSR register |
| **Unspecified** | 0 | Old 26-bit AIF header format |

### 2.3 Base Addresses

**Executable AIF:**
- **Image base** = address where AIF header is loaded
- **Code base** = image base + 0x80 (128 bytes)
- Standard RISC OS load address: 0x8000

**Non-executable AIF:**
- **Image base** = address where code should be loaded (header separate)
- **Code base** = image base

---

## 3. File Structure Overview

### 3.1 Compressed AIF Layout

```
┌────────────────────────┐  ← Load address
│ Header (128 bytes)     │
├────────────────────────┤
│ Compressed image data  │
├────────────────────────┤
│ Decompression data     │  (Position-independent)
├────────────────────────┤
│ Decompression code     │  (Position-independent)
└────────────────────────┘
```

### 3.2 Uncompressed/Decompressed AIF Layout

```
┌────────────────────────┐  ← Load address
│ Header (128 bytes)     │
├────────────────────────┤
│ Read-Only area (code)  │
├────────────────────────┤
│ Read-Write area (data) │
├────────────────────────┤
│ Debug data (optional)  │
├────────────────────────┤
│ Self-relocation code   │  (Position-independent)
├────────────────────────┤
│ Relocation list        │  (Terminated by -1)
└────────────────────────┘
```

### 3.3 Post-Relocation Layout

After execution completes:

```
┌────────────────────────┐
│ Header (128 bytes)     │
├────────────────────────┤
│ Read-Only area         │
├────────────────────────┤
│ Read-Write area        │
├────────────────────────┤
│ Debug data (optional)  │  ← Should be copied by debugger
├────────────────────────┤
│ Zero-initialized area  │  ← Overwrites relocation code/data
├────────────────────────┤
│ Heap/Stack             │
└────────────────────────┘
```

---

## 4. AIF Header Structure

The 128-byte (32-word) header contains executable code and metadata.

### 4.1 Header Layout

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| **0x00** | 4 | BL DecompressCode / NOP | Jump to decompression OR NOP if not compressed |
| **0x04** | 4 | BL SelfRelocCode / NOP | Jump to self-relocation OR NOP if not relocating |
| **0x08** | 4 | BL ZeroInit / NOP | Jump to zero-init code OR NOP if none |
| **0x0C** | 4 | BL ImageEntryPoint / Offset | Jump to entry point OR offset for non-executable |
| **0x10** | 4 | Program Exit Instruction | SWI OS_Exit or equivalent (fallback) |
| **0x14** | 4 | Image RO Size | Read-only area size (includes header if executable) |
| **0x18** | 4 | Image RW Size | Read-write area size (exact, multiple of 4) |
| **0x1C** | 4 | Image Debug Size | Debug area size (exact, multiple of 4) |
| **0x20** | 4 | Image ZI Size | Zero-init area size (exact, multiple of 4) |
| **0x24** | 4 | Image Debug Type | Debug data type: 0, 1, 2, or 3 |
| **0x28** | 4 | Image Base | Address where image was linked |
| **0x2C** | 4 | Work Space | Minimum workspace for self-moving images |
| **0x30** | 4 | Address Mode | 0, 26, or 32 (bit 8 set if separate data base) |
| **0x34** | 4 | Data Base | Address where data was linked (if bit 8 of 0x30 set) |
| **0x38** | 8 | Reserved | Two reserved words (initially 0) |
| **0x40** | 4 | Debug Init / NOP | Debug initialization SWI OR NOP |
| **0x44** | 60 | ZeroInit Code | Zero-initialization code (15 words) |

### 4.2 Field Details

#### 0x00: BL DecompressCode / NOP

**Compressed images:**
```arm
BL DecompressCode    ; 0xEB00xxxx (branch to decompression code)
```

**Uncompressed images:**
```arm
NOP                  ; 0xE1A00000 (MOV r0, r0)
```

**Legacy alternative:**
```arm
BLNV 0              ; 0xFB000000 (never executes, equivalent to NOP)
```

#### 0x04: BL SelfRelocCode / NOP

**Relocatable images:**
```arm
BL SelfRelocCode    ; 0xEB00xxxx (branch to relocation code)
```

**Non-relocatable images:**
```arm
NOP                 ; 0xE1A00000
```

**Important:** After relocation completes, this word **must be reset to NOP** to allow image re-entry.

#### 0x08: BL ZeroInit / NOP

**Images with zero-initialized data:**
```arm
BL ZeroInit         ; 0xEB00000C (branch to zero-init code at 0x44)
```

**Images without zero-init:**
```arm
NOP                 ; 0xE1A00000
```

#### 0x0C: BL ImageEntryPoint / EntryPoint Offset

**Executable AIF:**
```arm
BL ImageEntryPoint  ; 0xEB00xxxx (branch to program entry)
```
- Most significant byte = 0xEB identifies executable AIF
- Uses BL to make header addressable via r14 (link register)
- Application **must not return**

**Non-executable AIF:**
```
Entry point offset  ; 0x0000xxxx (offset from base address)
```
- Most significant nibble = 0x0 identifies non-executable AIF
- Loader interprets this as offset to entry point

#### 0x10: Program Exit Instruction

Fallback exit mechanism if application incorrectly returns.

**RISC OS:**
```arm
SWI OS_Exit         ; 0xEF000011 (exit to OS)
```

**3DO / Embedded:**
```arm
SWI 0x11            ; 0xEF000011 (custom exit handler)
```

**Systems without SWI:**
```arm
B .                 ; Branch to self (infinite loop)
```

**Customization:** Can be overridden by providing AIF_HDR area in first object file to linker.

#### 0x14: Image RO Size (Read-Only Size)

Size in bytes of read-only area (code + read-only data).

**Executable AIF:** Includes 128-byte header size  
**Non-executable AIF:** Excludes header size  

#### 0x18: Image RW Size (Read-Write Size)

Exact size in bytes of read-write (initialized data) area. Must be multiple of 4.

#### 0x1C: Image Debug Size

Exact size in bytes of debug data. Multiple of 4.

**Encoding:**
- **Bits 0-3:** Debug type (see 0x24)
- **Bits 4-31:** Low-level debug size

**Note:** Some sources indicate entire field is size; check debug type at 0x24 for definitive type.

#### 0x20: Image ZI Size (Zero-Init Size)

Exact size in bytes of zero-initialized area. Must be multiple of 4.

This area is created at runtime by zero-init code; no data stored in image file.

#### 0x24: Image Debug Type

| Value | Meaning |
|-------|---------|
| **0** | No debugging data present |
| **1** | Low-level debugging data present (ASD low-level) |
| **2** | Source-level debugging data present (ASD source-level) |
| **3** | Both low-level and source-level present |

All other values reserved to ARM Limited.

#### 0x28: Image Base

Address where the **code** was linked.

**Executable AIF:** Address of AIF header  
**Non-executable AIF:** Address of code (header excluded)  

**Standard RISC OS:** 0x8000

Used by self-relocation code to calculate relocation offset.

#### 0x2C: Work Space

Minimum workspace in bytes required by a self-moving relocatable image.

**Value 0:** Image does not self-move, only relocates in place

**Value > 0:** Image will move itself to high memory, leaving this much space above code for heap/stack

**3DO Extension:** Upper nibbles contain flags (see section 8.4)

#### 0x30: Address Mode

**Least significant byte:**
- **0:** Old-style 26-bit AIF (no address mode specified)
- **26:** Image linked for 26-bit ARM mode
- **32:** Image linked for 32-bit ARM mode

**Bit 8 (0x00000100):**
- **Set:** Image uses separate data base address (see 0x34)
- **Clear:** Data immediately follows code

#### 0x34: Data Base

Address where image data was linked.

**Only meaningful if bit 8 of 0x30 is set.**

Otherwise, data base = image base + RO size.

#### 0x38-0x3C: Reserved

Two words, must be zero.

**Extended AIF:** Word at 0x38 is non-zero and contains byte offset to first region header (see section 6).

#### 0x40: Debug Init Instruction

Optional debugger initialization.

**Typical use:**
```arm
SWI DDE_Debug       ; 0xEF041D41 (RISC OS)
```

**3DO Portfolio:**
```arm
SWI 0x010A          ; 0xEF00010A (Portfolio debugger)
```

**Unused:**
```arm
NOP                 ; 0xE1A00000
```

**Purpose:** Alerts resident debugger that debuggable image is starting execution.

**Customization:** Can be set via AIF_HDR template.

#### 0x44-0x7F: ZeroInit Code

15 words of position-independent code to zero-initialize data area.

Entered via BL from 0x08, so r14 = header + 0x0C on entry (contains PSR in 26-bit mode).

See section 5.1 for complete code listing.

---

## 5. Initialization Code

### 5.1 Zero-Init Code

Standard zero-initialization code (15 words at 0x44):

```arm
ZeroInit:
    NOP                       ; 0x40: Debug init instruction
    SUB    ip, lr, pc         ; base+12+[PSR]-(ZeroInit+12+[PSR])
                              ; = base-ZeroInit
    ADD    ip, pc, ip         ; base-ZeroInit+ZeroInit+16 = base+16
    LDMIB  ip, {r0,r1,r2,r4}  ; Load RO size, RW size, Debug size, ZI size
    SUB    ip, ip, #16        ; ip = image base
    ADD    ip, ip, r0         ; + RO size
    ADD    ip, ip, r1         ; + RW size = base of ZI area
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMP    r4, #0
    MOVLE  pc, lr             ; Return if nothing to do
ZeroLoop:
    STMIA  ip!, {r0,r1,r2,r3} ; Zero 16 bytes
    SUBS   r4, r4, #16        ; Decrement count
    BGT    ZeroLoop           ; Loop if more to zero
    MOVS   pc, lr             ; Return (restore PSR in 26-bit mode)
```

**Key points:**
- Position-independent (calculates addresses from PC/LR)
- Works in both 26-bit and 32-bit modes
- Always zeros multiple of 16 bytes
- Preserves other registers

### 5.2 Address Calculation Technique

The code uses a clever technique to find the header base:

```
On entry:  lr = header+0x0C (return address from BL at 0x08)
           pc = ZeroInit+12 (when SUB executes, PC is 2 instructions ahead)

Step 1:    ip = lr - pc
             = (header+12) - (ZeroInit+12)
             = header - ZeroInit
             
Step 2:    ip = pc + ip
             = (ZeroInit+16) + (header-ZeroInit)
             = header+16
             
Step 3:    ip = ip - 16
             = header (now pointing to base of image)
```

This works regardless of load address and preserves position-independence.

---

## 6. Self-Relocation

### 6.1 Relocation Types

AIF supports three relocation strategies:

| Type | Description | Use Case |
|------|-------------|----------|
| **No relocation** | Image runs at link address | ROM images, fixed-address systems |
| **Relocate to load** | Runs wherever loaded | Flexible loading, multiple instances |
| **Self-move** | Moves to high memory | Maximize workspace below image |

### 6.2 Relocation Data Structures

#### Relocation List

List of byte offsets from header start, indicating words to relocate:

```
offset1 (4 bytes)
offset2 (4 bytes)
...
offsetN (4 bytes)
-1 (0xFFFFFFFF) (terminator)
```

Each offset points to a word that contains an address requiring relocation.

#### Relocation Processing

For each offset in list:

```c
word_address = image_base + offset
old_value = *word_address
new_value = old_value + (actual_base - linked_base)
*word_address = new_value
```

### 6.3 Self-Relocation Code

Standard relocate-in-place code:

```arm
RelocCode:
    NOP                      ; Ensures byte order check succeeds
    SUB    ip, lr, pc        ; Calculate header address (see ZeroInit)
    ADD    ip, pc, ip
    SUB    ip, ip, #12       ; ip = header address
    LDR    r0, RelocCode     ; Load NOP instruction
    STR    r0, [ip, #4]      ; Overwrite BL SelfRelocCode with NOP
    LDR    r9, [ip, #0x2C]   ; Load Work Space field
    CMP    r9, #0
    BEQ    RelocateOnly      ; Jump if not self-moving

    ; Self-move code would go here (see section 6.4)
    
RelocateOnly:
    LDR    r1, [ip, #0x28]   ; Load linked image base
    SUBS   r1, ip, r1        ; Calculate relocation offset
    MOVEQ  pc, lr            ; Return if offset is zero
    STR    ip, [ip, #0x28]   ; Update image base to actual address
    ADR    r2, End           ; Point to relocation list
RelocLoop:
    LDR    r0, [r2], #4      ; Load offset, increment pointer
    CMN    r0, #1            ; Check for -1 terminator
    MOVEQ  pc, lr            ; Return if done
    LDR    r3, [ip, r0]      ; Load word to relocate
    ADD    r3, r3, r1        ; Apply relocation offset
    STR    r3, [ip, r0]      ; Store relocated word
    B      RelocLoop         ; Next relocation
End:
    ; Relocation list starts here
```

### 6.4 Self-Move Code

For images that move to high memory (Work Space > 0):

```arm
    ; After checking Work Space != 0:
    
    LDR    r0, [ip, #0x20]   ; Load ZI size
    ADD    r9, r9, r0        ; Space to leave = workspace + ZI
    SWI    GetEnv            ; Returns memory limit in r1 (system-specific!)
    
    ; Calculate end of image + relocation data
    ADR    r2, End
FindEnd:
    LDR    r0, [r2], #4
    CMN    r0, #1
    BNE    FindEnd           ; r2 now points past end of relocation list
    
    SUB    r3, r1, r9        ; Target = mem_limit - space_to_leave
    SUBS   r0, r3, r2        ; Move distance
    BLE    RelocateOnly      ; Not enough space, just relocate
    BIC    r0, r0, #15       ; Round to multiple of 16
    ADD    r3, r2, r0        ; New end address
    ADR    r8, CopyDone      ; Address to jump to after copying
    
    ; Copy image upwards in descending order
CopyUp:
    LDMDB  r2!, {r4-r7}      ; Load 4 words from end
    STMDB  r3!, {r4-r7}      ; Store to new location
    CMP    r2, r8            ; Copied the copy code yet?
    BGT    CopyUp            ; Not yet
    ADD    r4, pc, r0        ; Calculate new PC
    MOV    pc, r4            ; Jump to copied code
    
    ; Now executing from new location
CopyDone:
    LDMDB  r2!, {r4-r7}
    STMDB  r3!, {r4-r7}
    CMP    r2, ip            ; Copied everything?
    BGT    CopyDone          ; Not yet
    ADD    ip, ip, r0        ; Update image base pointer
    ADD    lr, lr, r0        ; Update return address
    B      RelocateOnly      ; Continue with relocation
```

**Critical:** The code must copy itself and jump to the copied version before completing the move.

---

## 7. Debugging Support

### 7.1 Debug Data Layout

Debug data stored after RW area, before relocation code:

```
RW area end
↓
┌────────────────────────┐
│ Low-level debug data   │ (Optional)
├────────────────────────┤
│ Source-level debug     │ (Optional)
│ tables (ASD format)    │
└────────────────────────┘
↑
Debug area end
```

### 7.2 Debug Data Properties

**Addressing:**
- References to code/data: Relocatable addresses (absolute after loading at link address)
- References between debug entries: Offsets from debug area start

**Position Independence:**
- After relocation, debug area is position-independent
- Debugger can copy debug data to safe location
- Original location will be overwritten by zero-init area

### 7.3 Debugger Interaction

**Debugger seizes control by:**

1. Loading image
2. Copying word at 0x08 (BL ZeroInit)
3. Replacing with `BL DebuggerEntry`
4. Allowing decompression and relocation
5. On entry to modified ZeroInit call:
   - Copy debug data to safe location
   - Restore original BL ZeroInit
   - Return to allow zero-init to proceed

### 7.4 Debug Types

| Type | Code | Content |
|------|------|---------|
| **None** | 0 | No debug data |
| **Low-level** | 1 | Assembly-level symbols, addresses |
| **Source-level** | 2 | Source file info, line numbers, types (for C, Fortran, Pascal) |
| **Both** | 3 | Combined low-level and source-level |

---

## 8. Platform-Specific Extensions

### 8.1 RISC OS

**Standard load address:** 0x8000  
**Virtual memory:** Each application at 0x8000 via MMU page tables  
**Exit instruction:** `SWI 0x11` (OS_Exit)  
**Memory limit:** `SWI 0x10` (OS_GetEnv) returns top of memory in r1  

### 8.2 3DO Portfolio

3DO extends AIF with additional header structure.

#### Work Space Flags (0x2C)

| Bits | Mask | Meaning |
|------|------|---------|
| 0-23 | 0x00FFFFFF | Stack size (max 16MB) |
| 24-27 | 0x0F000000 | Reserved |
| 28-31 | 0xF0000000 | Flags |

**Bit 30 (0x40000000):** `AIF_3DOHEADER` - Extended header follows

#### Extended 3DO Header (_3DOBinHeader)

When bit 30 of Work Space is set, a 128-byte _3DOBinHeader follows immediately after standard AIF header at offset 0x80:

```c
struct _3DOBinHeader {
    ItemNode _3DO_Item;           // +0x00: 16 bytes - Item node structure
    uint8    _3DO_Flags;          // +0x10: Flags (see below)
    uint8    _3DO_OS_Version;     // +0x11: Required OS version
    uint8    _3DO_OS_Revision;    // +0x12: Required OS revision
    uint8    _3DO_Reserved;       // +0x13: Must be 0
    uint32   _3DO_Stack;          // +0x14: Stack requirement (bytes)
    uint32   _3DO_FreeSpace;      // +0x18: Preallocated heap (bytes)
    uint32   _3DO_Signature;      // +0x1C: Offset to RSA signature (if privileged)
    uint32   _3DO_SignatureLen;   // +0x20: Signature length
    uint32   _3DO_MaxUSecs;       // +0x24: Max microseconds before task switch
    uint32   _3DO_Reserved0;      // +0x28: Must be 0
    char     _3DO_Name[32];       // +0x2C: Task name (NUL-terminated)
    uint32   _3DO_Time;           // +0x4C: Build time (seconds since 1/1/93 GMT)
    uint32   _3DO_Reserved1[7];   // +0x50: Must be 0
};
```

**_3DO_Flags bits:**

| Bit | Mask | Name | Meaning |
|-----|------|------|---------|
| 0 | 0x01 | _3DO_PERMSTACK | Permanent stack (not returned on exit) |
| 1 | 0x02 | _3DO_PRIVILEGE | Privileged application |
| 2 | 0x04 | _3DO_USERAPP | User application requiring authentication |
| 5 | 0x20 | _3DO_DATADISCOK | App accepts data discs |
| 6 | 0x40 | _3DO_NORESETOK | Keep running on empty tray |
| 7 | 0x80 | _3DO_LUNCH | OS launched (chips may be "lunched") |

#### Debug Init Values (3DO)

| Value | Constant | Meaning |
|-------|----------|---------|
| 0xE1A02002 | PORTFOLIO_DEBUG_CLEAR | NOP (MOV R2,R2) - no debugging |
| 0xEF00010A | PORTFOLIO_DEBUG_TASK | SWI 0x010A - debugger present |
| 0xE1A00000 | PORTFOLIO_DEBUG_UNSET | NOP (MOV R0,R0) - unset |

---

## 9. Extended AIF (Scatter-Loaded Images)

### 9.1 Purpose

Extended AIF supports images with multiple load regions at different addresses (scatter loading).

### 9.2 Identification

Extended AIF is identified by:
- Non-executable AIF format (offset at 0x0C)
- Word at 0x38 is **non-zero** and contains file offset to first region header

### 9.3 Region Header Format

Each load region has a 44-byte header:

| Offset | Field | Size | Description |
|--------|-------|------|-------------|
| +0x00 | Next | 4 | File offset to next region header (0 if last) |
| +0x04 | LoadAddress | 4 | Address to load this region |
| +0x08 | Size | 4 | Size in bytes (multiple of 4) |
| +0x0C | Name | 32 | Region name (NUL-padded, max 32 chars) |

Immediately following each header is the region's initializing data (Size bytes).

### 9.4 Processing Extended AIF

```c
offset = header.Reserved[0];  // Word at 0x38
while (offset != 0) {
    region_header = read_from_file(offset, 44);
    region_data = read_from_file(offset + 44, region_header.Size);
    
    load_to_memory(region_data, region_header.LoadAddress, region_header.Size);
    
    offset = region_header.Next;
}
```

### 9.5 Root Region

The "root" region (code starting at image base) is described by main AIF header, not region headers.

---

## 10. Validation and Error Checking

### 10.1 Executable vs. Non-Executable Detection

```c
uint32 word3 = read_word_at(0x0C);
uint8 msb = (word3 >> 24) & 0xFF;

if (msb == 0xEB) {
    // Executable AIF
    entry_type = EXECUTABLE;
    entry_point = decode_bl_target(word3, 0x0C);
} else if ((msb & 0xF0) == 0x00) {
    // Non-executable AIF
    entry_type = NON_EXECUTABLE;
    entry_point_offset = word3;
} else {
    // Invalid or corrupt
    return ERROR;
}
```

### 10.2 Required Validations

1. **Header size:** File size ≥ 128 bytes
2. **Word alignment:** All size fields multiple of 4
3. **Size consistency:** RO + RW + Debug ≤ file size
4. **Address mode:** 0, 26, or 32
5. **Debug type:** 0-3 only
6. **BL instructions:** Valid branch encoding
7. **NOP encoding:** 0xE1A00000 or 0xFB000000

### 10.3 Recommended Validations

1. **Image base:** Reasonable address (typically ≥ 0x8000, < 0x80000000)
2. **Work space:** Reasonable size (< 100MB)
3. **Relocation list:** Properly terminated by -1
4. **Zero-init code:** Matches expected pattern
5. **Reserved fields:** All zeros

---

## 11. Creating AIF Images

### 11.1 Linker Process

ARM linker (armlink) creates AIF from AOF files:

```
armlink -aif -o output.aif input1.o input2.o
```

Options:
- `-aif`: Generate AIF format
- `-reloc`: Include relocation information
- `-debug`: Include debugging tables
- `-ro-base <addr>`: Set image base address
- `-rw-base <addr>`: Set data base address (separate)

### 11.2 Header Generation

Linker fills AIF header:

1. Calculates sizes (RO, RW, ZI, Debug)
2. Determines entry point from AOF entry_area and entry_offset
3. Encodes BL instructions or NOP based on options
4. Copies standard zero-init code to 0x44-0x7F
5. Inserts relocation code if requested
6. Builds relocation list from AOF relocation directives

### 11.3 Customizing Header

Provide `AIF_HDR` area in **first** object file:

```arm
    AREA AIF_HDR, CODE, READONLY
    
    ; Custom header template (128 bytes)
    DCD 0xE1A00000              ; NOP (no compression)
    DCD 0xEB00xxxx              ; BL SelfRelocCode
    DCD 0xEB00000C              ; BL ZeroInit
    DCD 0xEB00yyyy              ; BL entry
    DCD 0xEF000099              ; SWI custom_exit
    ; ... rest of header ...
```

Linker uses this template instead of default.

---

## 12. Loading and Executing AIF

### 12.1 Simple Loader (Executable AIF)

```c
void load_and_run_aif(const char *filename) {
    // 1. Load entire file into memory
    void *image = load_file_to_address(filename, 0x8000);
    
    // 2. Cast to function pointer
    void (*entry)(void) = (void (*)(void))image;
    
    // 3. Call first word of image
    entry();
    
    // Should never return!
}
```

### 12.2 Sophisticated Loader (Non-Executable AIF)

```c
void load_aif_image(const char *filename) {
    uint32 header[32];
    
    // 1. Read header
    read_file(filename, 0, header, 128);
    
    // 2. Determine type
    if ((header[3] >> 24) == 0xEB) {
        // Executable - load entire file and jump
        void *image = load_file_to_address(filename, header[10]); // header[10] = image base
        ((void (*)(void))image)();
    } else {
        // Non-executable - process header
        uint32 image_base = header[10];
        uint32 ro_size = header[5];
        uint32 rw_size = header[6];
        uint32 entry_offset = header[3];
        
        // Load code and data
        load_file_to_address(filename, 128, image_base, ro_size + rw_size);
        
        // Zero-initialize ZI area
        uint32 zi_size = header[8];
        memset((void *)(image_base + ro_size + rw_size), 0, zi_size);
        
        // Process relocation if needed
        if (header[1] != 0xE1A00000) { // Not NOP
            relocate_image(image_base, header);
        }
        
        // Jump to entry point
        void (*entry)(void) = (void (*)(void))(image_base + entry_offset);
        entry();
    }
}
```

---

## 13. Common Use Cases

### 13.1 Simple Absolute Image

- No relocation
- No compression
- No debugging
- Fixed load address

**Header:**
- 0x00: NOP
- 0x04: NOP
- 0x08: BL ZeroInit (if ZI area) or NOP
- 0x0C: BL entry
- Image base = link address

### 13.2 Relocatable Application

- Self-relocating
- Position-independent code
- Can run anywhere

**Header:**
- 0x00: NOP
- 0x04: BL SelfRelocCode
- 0x08: BL ZeroInit
- 0x0C: BL entry
- Relocation list present

### 13.3 Compressed ROM Image

- Compressed for space efficiency
- Self-decompressing
- Fixed ROM address after decompression

**Header:**
- 0x00: BL DecompressCode
- 0x04: NOP (or BL if also relocatable)
- 0x08: BL ZeroInit
- 0x0C: BL entry

### 13.4 Debuggable Application

- Source-level debugging
- Relocatable
- Debug data included

**Header:**
- 0x04: BL SelfRelocCode
- 0x08: BL ZeroInit
- 0x0C: BL entry
- 0x24: Debug type = 2 or 3
- 0x40: SWI DDE_Debug
- Debug data after RW area

---

## 14. Interoperability

### 14.1 Restartability

An AIF image is **restartable** if and only if the program it contains is restartable.

**Important:** AIF images are **not reentrant** (cannot have multiple concurrent executions).

**To support restart:**
1. After decompression, set 0x00 to NOP
2. After relocation, set 0x04 to NOP
3. These writes required anyway (code modifies itself)

### 14.2 Memory Protection

On systems with memory protection:

**Decompression code must:**
1. Call system to make RO area writable
2. Decompress
3. Call system to make RO area read-only again

**Relocation code must:**
1. Call system to make RO area writable
2. Relocate
3. Reset 0x04 to NOP
4. Call system to make RO area read-only again

### 14.3 Multiple Instances

**RISC OS:** Uses virtual memory, each instance at 0x8000 in separate address space

**Bare metal:** Cannot load multiple instances of non-relocatable image

**Relocatable images:** Can load multiple instances at different addresses

---

## 15. Size Relationships

### 15.1 Address Calculations

```
Image_RO_Base       = Image_Base (if executable AIF)
Image_RW_Base       = Image_Base + RO_Size
Image_ZI_Base       = Image_Base + RO_Size + RW_Size
Image_RW_Limit      = Image_Base + RO_Size + RW_Size + ZI_Size
```

### 15.2 File Size

```
Uncompressed:
  File_Size = 128 + RO_Size + RW_Size + Debug_Size + Reloc_Size

Compressed:
  File_Size = 128 + Compressed_Size + Decompress_Code_Size
```

### 15.3 Memory Footprint

```
Initial:
  Memory = RO_Size + RW_Size + Debug_Size + Reloc_Size

After init:
  Memory = RO_Size + RW_Size + ZI_Size + Debug_Size (if kept)
```

---

## 16. Binary Format Examples

### 16.1 Minimal Executable AIF Header (Hex Dump)

```
Offset  00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F
------  -----------------------------------------------
0x0000  00 00 A0 E1  00 00 A0 E1  0C 00 00 EB  xx xx xx EB   | ............xxxx
0x0010  11 00 00 EF  00 02 00 00  00 00 00 00  00 00 00 00   | ................
0x0020  00 04 00 00  00 00 00 00  00 80 00 00  00 00 00 00   | ................
0x0030  20 00 00 00  00 00 00 00  00 00 00 00  00 00 00 00   |  ...............
0x0040  00 00 A0 E1  xx xx xx xx  xx xx xx xx  xx xx xx xx   | ....xxxxxxxxxxxx
... (zero-init code continues) ...
```

**Decoded:**
- 0x00: 0xE1A00000 = NOP (no compression)
- 0x04: 0xE1A00000 = NOP (no relocation)
- 0x08: 0xEB00000C = BL ZeroInit (+0x30 bytes → 0x0C + 8 + 0x30 = 0x44)
- 0x0C: 0xEBxxxxxx = BL entry (address depends on code)
- 0x10: 0xEF000011 = SWI 0x11 (OS_Exit)
- 0x14: 0x00000200 = RO size = 512 bytes
- 0x18: 0x00000000 = RW size = 0 bytes
- 0x1C: 0x00000000 = Debug size = 0 bytes
- 0x20: 0x00000400 = ZI size = 1024 bytes
- 0x24: 0x00000000 = Debug type = none
- 0x28: 0x00008000 = Image base = 0x8000
- 0x2C: 0x00000000 = Work space = 0 (no self-move)
- 0x30: 0x00000020 = Address mode = 32-bit
- 0x34: 0x00000000 = Data base = unused
- 0x38-0x3F: zeros (reserved)
- 0x40: 0xE1A00000 = NOP (no debug init)
- 0x44+: Zero-init code

---

## 17. Troubleshooting

### 17.1 Common Errors

| Symptom | Possible Cause |
|---------|---------------|
| Crash at load | Wrong load address, image base mismatch |
| Crash after relocation | Corrupted relocation list, missing relocation |
| Crash in zero-init | Wrong ZI size, memory protection issue |
| No debug symbols | Debug type = 0, debug data stripped |
| Won't self-move | Work space = 0, self-move code missing |
| Decompression fails | Corrupted compressed data, wrong algorithm |

### 17.2 Debugging Tips

1. **Verify header:** Check all fields reasonable, especially sizes
2. **Check alignment:** All sizes multiple of 4
3. **Trace execution:** Set breakpoints at 0x00, 0x04, 0x08, 0x0C
4. **Validate BL targets:** Calculate branch destinations
5. **Test at link address:** Remove relocation as variable
6. **Check memory permissions:** Ensure writable during relocation

---

## 18. Related Formats and Tools

### 18.1 Related Formats

- **AOF** - Source object files linked to create AIF
- **ALF** - Libraries of AOF files
- **ELF** - Modern replacement for AIF (RISC OS 5+)

### 18.2 Tools

- **armlink** - ARM linker, creates AIF from AOF
- **armsd** - ARM symbolic debugger, loads AIF
- **squeeze** - Compress AIF images
- **modbin** - Modify AIF headers (3DO)

---

## 19. References

### 19.1 Official Documentation

1. ARM Software Development Toolkit Reference Guide (DUI0041C), Chapter 13
2. RISC OS Programmer's Reference Manual, AIF specification
3. Technical Memorandum PLG-AIF (Acorn Computers, 1989)

### 19.2 Historical Documents

1. AIF-1989.md - Original 1989 specification
2. AIF-1993.md - Updated 1993 specification with 32-bit support
3. aif.h - 3DO header file with extensions

---

## Document Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2024 | Comprehensive specification compiled from ARM, Acorn, and 3DO documentation |

---

**End of ARM Image Format (AIF) Specification**
