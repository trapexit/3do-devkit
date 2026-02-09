# ARM Image Format (AIF) - Complete File Format Specification

## Overview

The **ARM Image Format (AIF)** is a simple, efficient executable format for ARM processor-based systems. Originally developed by Acorn Computers Ltd. for the Archimedes and RISC OS computer ranges, it remains in use on RISC OS systems today alongside modern formats like ELF. AIF is designed to be minimal, position-independent, and self-contained.

## Format Variants

AIF exists in three distinct variants:

### 1. Executable AIF
- **Purpose**: Standard executable programs that can be loaded and run directly
- **Header**: Part of the image itself, executed starting at the first word
- **Entry Point**: Fourth word is `BL <entrypoint>` (MSB = 0xEB)
- **Base Address**: Address where header should be loaded; code starts at `base + 0x80`
- **Use Case**: Most common for RISC OS applications (e.g., `!RunImage` files)

### 2. Non-Executable AIF
- **Purpose**: Images that require preparation by a loader program
- **Header**: Describes the image but is not part of it; discarded after loading
- **Entry Point**: Fourth word is the offset from base address (MSN = 0x0)
- **Base Address**: Address where code should be loaded
- **Use Case**: Systems requiring custom loading mechanisms

### 3. Extended AIF
- **Purpose**: Scatter-loaded images with multiple load regions
- **Header**: Points to a chain of load region descriptors within the file
- **Features**: Supports non-contiguous memory layouts
- **Identification**: Word at offset 0x38 is non-zero, containing offset to first region header

## File Layout

### Standard AIF Layout (Uncompressed)

```
+----------------------+  Offset 0x00
| AIF Header (128 bytes)|
+----------------------+  Offset 0x80
| Read-Only Area       |
| (Code and constants) |
+----------------------+
| Read-Write Area      |
| (Initialized data)   |
+----------------------+
| Debugging Data       |  (Optional)
+----------------------+
| Self-Relocation Code |  (Position-independent)
+----------------------+
| Relocation List      |  (Terminated by -1)
+----------------------+
```

### Compressed AIF Layout

```
+----------------------+  Offset 0x00
| AIF Header (128 bytes)|  (Not compressed)
+----------------------+  
| Compressed Image     |
+----------------------+
| Decompression Data   |  (Position-independent)
+----------------------+
| Decompression Code   |  (Position-independent)
+----------------------+
```

### Post-Relocation Layout

After self-relocation completes, the image appears as:

```
+----------------------+
| AIF Header           |
+----------------------+
| Read-Only Area       |
+----------------------+
| Read-Write Area      |
+----------------------+
| Debugging Data       |  (Optional, to be copied by debugger)
+----------------------+
```

**Note**: The debugging data area is overwritten by zero-initialized data, heap, or stack unless the debugger copies it first.

## AIF Header Structure

The AIF header is exactly 128 bytes (32 words) in size.

### Header Field Reference

| Offset | Size | Field | Description |
|--------|------|-------|-------------|
| 0x00 | 4 bytes | BL DecompressCode | Branch to decompression code, or NOP (0xE1A00000) if not compressed |
| 0x04 | 4 bytes | BL SelfRelocCode | Branch to self-relocation code, or NOP if not self-relocating |
| 0x08 | 4 bytes | BL ZeroInit / BL DBGInit | Branch to zero-init code, or NOP if no zero-init data |
| 0x0C | 4 bytes | BL ImageEntryPoint or EntryPoint Offset | `BL` for executable AIF (makes header addressable via R14); offset for non-executable |
| 0x10 | 4 bytes | Program Exit Instruction | Last-ditch exit (usually SWI OS_Exit = 0xEF000011) |
| 0x14 | 4 bytes | Image ReadOnly Size | Size in bytes (includes header for executable AIF) |
| 0x18 | 4 bytes | Image ReadWrite Size | Exact size, multiple of 4 bytes |
| 0x1C | 4 bytes | Image Debug Size | Exact size, multiple of 4 bytes; bits 0-3 hold debug type |
| 0x20 | 4 bytes | Image Zero-Init Size | Exact size, multiple of 4 bytes |
| 0x24 | 4 bytes | Image Debug Type | 0=None, 1=Low-level, 2=Source-level, 3=Both |
| 0x28 | 4 bytes | Image Base | Address where image was linked |
| 0x2C | 4 bytes | Work Space | Minimum workspace to reserve (obsolete in modern AIF) |
| 0x30 | 4 bytes | Address Mode | LSB=26 or 32; bit 8 set = separate data base |
| 0x34 | 4 bytes | Data Base | Address where data was linked (if bit 8 of Address Mode set) |
| 0x38 | 4 bytes | Reserved Word 1 | Initially 0; non-zero indicates Extended AIF |
| 0x3C | 4 bytes | Reserved Word 2 | Initially 0 |
| 0x40 | 4 bytes | Debug Init Instruction | NOP if unused, or SWI to alert debugger |
| 0x44-0x7C | 60 bytes | Zero-Init Code | 15 words of zero-initialization code |

### Header Field Details

#### NOP Encoding
NOP is encoded as `MOV R0, R0` = 0xE1A00000

#### BL Instructions
All branch-and-link instructions serve dual purposes:
1. **Execution flow control**: Jump to the appropriate handler
2. **Position independence**: On entry, R14 contains the return address, allowing header address calculation

#### Address Mode Word (Offset 0x30)
- **Bits 0-7**: Addressing mode (26 or 32)
  - 0 = Old-style 26-bit AIF header
  - 26 = Linked for 26-bit ARM mode
  - 32 = Linked for 32-bit ARM mode
- **Bit 8**: Set if separate code and data bases used
- **Bits 9-31**: Reserved (0)

#### Debug Type Values (Bits 0-3 of offset 0x1C)

| Value | Meaning |
|-------|---------|
| 0 | No debugging data present |
| 1 | Low-level debugging data present |
| 2 | Source-level debugging data present |
| 3 | Both low-level and source-level present |

### Extended AIF Region Header

For Extended AIF, each region has a 44-byte header:

| Word | Size | Description |
|------|------|-------------|
| 0 | 4 bytes | File offset of next region header (0 if last) |
| 1 | 4 bytes | Load address for this region |
| 2 | 4 bytes | Size in bytes (multiple of 4) |
| 3-10 | 32 bytes | Region name (32 chars, zero-padded) |

Region data immediately follows the header.

## Zero-Initialization Code

The zero-init code at offset 0x44 is responsible for clearing the zero-initialized data area. This is standard code added by the linker.

### Standard Zero-Init Code

```arm
ZeroInit:
    NOP                       ; or Debug Init Instruction at 0x40
    SUB    ip, lr, pc         ; Calculate base-ZeroInit
    ADD    ip, pc, ip         ; Calculate base+16
    LDMIB  ip, {r0,r1,r2,r4}  ; Load sizes (RO, RW, Debug, ZeroInit)
    SUB    ip, ip, #16        ; ip = image base
    ADD    ip, ip, r0         ; ip += RO size
    ADD    ip, ip, r1         ; ip += RW size = base of zero-init area
    MOV    r0, #0
    MOV    r1, #0
    MOV    r2, #0
    MOV    r3, #0
    CMPS   r4, #0             ; Any zero-init data?
00  MOVLE  pc, lr             ; Return if none or done
    STMIA  ip!, {r0,r1,r2,r3} ; Zero 16 bytes
    SUBS   r4, r4, #16        ; Decrement counter
    B      %B00               ; Loop
```

### Algorithm Explanation

1. **Calculate base address**: Uses `lr - pc` math to determine header base address in a position-independent manner
2. **Load size fields**: Loads the four size fields from the header
3. **Calculate zero-init base**: Adds RO size + RW size to base address
4. **Zero memory**: Stores zeros in 16-byte chunks until all zero-init data is cleared
5. **Return**: Returns to caller via `MOV PC, LR`

## Self-Relocation

### Types of Self-Relocation

1. **One-time Position Independence**: Image can be loaded at any address and executes there
2. **Self-Move Up Memory**: Image copies itself to high memory, leaving workspace free

### Relocation List Format

The relocation list follows the self-relocation code:

```
+----------------------+
| Offset 0            |  Offset from AIF header of word to relocate
+----------------------+
| Offset 1            |
+----------------------+
| ...                 |
+----------------------+
| Offset n            |
+----------------------+
| 0xFFFFFFFF          |  Terminator (-1)
+----------------------+
```

Each offset points to a 32-bit word that must be adjusted by the relocation offset.

### Self-Relocation Algorithm

```arm
RelocCode:
    NOP
    SUB    ip, lr, pc         ; Calculate header address
    ADD    ip, pc, ip
    SUB    ip, ip, #12        ; ip = header address
    STR    r0, [ip, #4]       ; Clear self-reloc word (NOP for restart)
    LDR    r9, [ip, #0x2C]    ; Load workspace requirement
    CMPS   r9, #0             ; Need to move?
    BEQ    RelocateOnly       ; No, just relocate
    
    ; ... code to calculate move distance and copy image up memory ...
    
RelocateOnly:
    LDR    r1, [ip, #0x28]    ; Load linked base address
    SUBS   r1, ip, r1         ; Calculate relocation offset
    MOVEQ  pc, lr             ; No relocation needed
    STR    ip, [ip, #0x28]    ; Store new base address
    ADR    r2, End            ; Start of relocation list
04  LDR    r0, [r2], #4       ; Load next offset
    CMNS   r0, #1             ; Check for terminator
    MOVEQ  pc, lr             ; Done
    LDR    r3, [ip, r0]       ; Load word to relocate
    ADD    r3, r3, r1         ; Apply relocation
    STR    r3, [ip, r0]       ; Store back
    B      %B04               ; Next
End:
```

### Relocation Considerations

- **Word-only**: Only 32-bit words can be relocated
- **Position independence**: Relocation code itself must be position-independent
- **Memory protection**: On protected systems, code must temporarily make read-only sections writable
- **Restartability**: After relocation, the self-reloc word must be reset to NOP for image restart

## Compression

AIF supports optional compression for reduced storage and faster loading:

### Compressed AIF Structure

1. **Uncompressed Header**: 128-byte header (never compressed)
2. **Compressed Image**: The RO area, RW area, and debugging data compressed
3. **Decompression Data**: Tables required by decompression algorithm
4. **Decompression Code**: Position-independent code to decompress the image

### Decompression Process

1. Entry via `BL DecompressCode` at offset 0x00
2. Decompress image into place
3. Reset first header word to NOP (for restartability)
4. Continue to self-relocation or zero-init as needed

## Debugging Support

### Debug Data Organization

- **Location**: After RW area, before relocation code
- **Format**: Architecture-specific (ASD format historically used)
- **References**: All code/data references are relocatable addresses
- **Internal references**: Offsets from start of debug data area

### Debug Types

1. **Low-level debugging**: Assembly-level symbols and line information
2. **Source-level debugging**: High-level language symbols, types, and line mapping
3. **Combined**: Both low-level and source-level data present

### Debugger Interaction

- Debug data must be copied before zero-init overwrites it
- Debugger can intercept execution by modifying the BL ZeroInit instruction
- Position-independent nature allows debugger to move debug data as needed

## C Structure Definition

```c
#define AIF_HEADER_SIZE 128
#define AIF_NOOP        0xE1A00000  /* MOV R0, R0 */
#define AIF_BLAL        0xEB000000  /* BL base */

/* Debug type values */
#define AIF_DEBUG_NONE  0
#define AIF_DEBUG_LOW   1
#define AIF_DEBUG_SRC   2
#define AIF_DEBUG_BOTH  3

/* Address mode values */
#define AIF_ADDR_26BIT  26
#define AIF_ADDR_32BIT  32
#define AIF_DATABASAT   0x00000100  /* Separate data base */

typedef struct {
    uint32_t bl_decompress;      /* 0x00: BL to decompression code or NOP */
    uint32_t bl_selfreloc;       /* 0x04: BL to self-reloc code or NOP */
    uint32_t bl_zeroinit;        /* 0x08: BL to zero-init or NOP */
    uint32_t bl_entry;           /* 0x0C: BL entrypoint or offset */
    uint32_t swi_exit;           /* 0x10: Program exit SWI */
    uint32_t ro_size;            /* 0x14: Read-only size */
    uint32_t rw_size;            /* 0x18: Read-write size */
    uint32_t debug_size;         /* 0x1C: Debug data size */
    uint32_t zeroinit_size;      /* 0x20: Zero-init size */
    uint32_t debug_type;         /* 0x24: Debug type */
    uint32_t image_base;         /* 0x28: Image base address */
    uint32_t workspace;          /* 0x2C: Workspace requirement */
    uint32_t address_mode;       /* 0x30: 26/32 bit mode flags */
    uint32_t data_base;          /* 0x34: Data base address */
    uint32_t reserved1;          /* 0x38: Reserved */
    uint32_t reserved2;          /* 0x3C: Reserved */
    uint32_t debug_init;         /* 0x40: Debug init instruction */
    uint32_t zeroinit_code[15];  /* 0x44-0x7C: Zero-init code */
} AIFHeader;
```

## Linker Symbols

The linker defines special symbols relating to AIF header fields:

| Symbol | Description |
|--------|-------------|
| `Image$$RO$$Base` | Base of read-only area (= ImageBase + header size for executable AIF) |
| `Image$$RW$$Base` | Base of read-write area (= RO$$Base + RO$$Size) |
| `Image$$ZI$$Base` | Base of zero-init area (= RW$$Base + RW$$Size) |
| `Image$$RW$$Limit` | End of zero-init area (= ZI$$Base + ZI$$Size) |

## 3DO-Specific Extensions

The 3DO platform extends AIF with additional header information:

### 3DO Binary Header

When `AIF_3DOHEADER` flag is set in the workspace field, a 128-byte 3DO header follows the AIF header:

```c
#define AIF_3DOHEADER 0x40000000

typedef struct {
    uint32_t item_node[2];       /* ItemNode structure */
    uint8_t  flags;              /* 3DO flags */
    uint8_t  os_version;         /* Target OS version */
    uint8_t  os_revision;        /* Target OS revision */
    uint8_t  reserved;
    uint32_t stack;              /* Stack requirements */
    uint32_t freespace;          /* FreeList preallocation */
    uint32_t signature;          /* Offset to signature (if privileged) */
    uint32_t signature_len;      /* Signature length */
    uint32_t max_usecs;          /* Max microseconds before task switch */
    uint32_t reserved0;
    char     name[32];           /* Task name */
    uint32_t time;               /* Seconds since 1/1/93 00:00:00 GMT */
    uint32_t reserved1[7];       /* Must be zero */
} _3DOBinHeader;
```

### 3DO Flag Values

| Flag | Value | Description |
|------|-------|-------------|
| _3DO_DATADISCOK | 0x20 | App accepts data discs |
| _3DO_NORESETOK | 0x10 | App keeps running on empty drawer |
| _3DO_LUNCH | 0x08 | OS has been launched, chips may be "lunched" |
| _3DO_USERAPP | 0x04 | User app requiring authentication |
| _3DO_PRIVILEGE | 0x02 | Privileged task |
| _3DO_PERMSTACK | 0x01 | Permanent stack |

## Endianness

AIF files use the endianness of the target ARM system:
- **Little-endian**: Standard for most ARM systems
- **Big-endian**: Supported but less common

The endianness must be consistent throughout the file. Multi-byte values are stored with the least significant byte at the lowest address (DEC/Intel byte order).

## Best Practices

### Creating AIF Images

1. **Use the linker**: Let the ARM linker (armlink) generate proper AIF headers
2. **Specify options correctly**:
   - Use `-aif` for executable AIF output
   - Use `-relocable` for position-independent images
   - Use `-debug` to include debugging information

3. **Header customization**: 
   - Provide `AIF_HDR` area in first object file for custom header template
   - Provide `AIF_RELOC` area for custom relocation code

### Loading AIF Images

1. **Executable AIF**:
   ```c
   /* Load at specified address and jump to first word */
   void *image = load_aif("program", load_address);
   ((void (*)(void))image)();  /* Execute */
   ```

2. **Non-Executable AIF**:
   ```c
   /* Parse header and prepare image manually */
   AIFHeader *hdr = read_aif_header("program");
   void *code = load_code_at(hdr->image_base, hdr->ro_size - 128);
   /* Handle relocation, zero-init, etc. */
   jump_to(hdr->image_base + hdr->bl_entry_offset);
   ```

### Debugging Considerations

1. Save debug data before zero-init runs
2. Modify BL ZeroInit instruction to break into debugger
3. Use Image Base field to calculate actual runtime addresses

## Limitations and Considerations

1. **No standard security/validation fields**: AIF has no built-in checksum or signature mechanism (except 3DO extensions)
2. **No versioning**: No explicit format version in header
3. **No dynamic linking information**: Static linking only (though extensions possible)
4. **26/32-bit compatibility**: Older AIF files may assume 26-bit addressing
5. **Obsolete for new development**: ELF is preferred for modern systems

## Related Formats

- **AOF (ARM Object Format)**: Input to linker, produces AIF output
- **ALF (ARM Library Format)**: Collection of AOF files
- **ELF**: Modern replacement format with more features

## References

- ARM DUI 0041C: ARM Software Development Toolkit Reference Guide
- Acorn RISC OS Programmer's Reference Manuals
- 3DO Portfolio Developer Documentation

## Summary

AIF is a compact, efficient executable format well-suited to embedded and resource-constrained ARM systems. Its self-relocating and self-decompressing capabilities make it ideal for ROM-based systems and systems with simple loaders. While largely superseded by ELF for new development, understanding AIF remains important for legacy RISC OS systems and embedded ARM development.
