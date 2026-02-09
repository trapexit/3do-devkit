# ARM Library Format (ALF) - Complete File Format Specification

## Overview

The ARM Library Format (ALF) is a container format used by the ARM librarian (armlib) and linker (armlink) to group multiple AOF (ARM Object Format) files into a single library file. ALF provides efficient access to individual object modules and supports symbol table lookups for selective linking.

### History

- **Late 1980s**: Developed alongside AOF for ARM development toolchain
- **Version 1.00**: Initial library format specification
- **Enhanced Versions**: Added object code library support with external symbol tables

### Design Principles

ALF was designed with these goals:
- Simple and efficient access to library members
- Extensible through chunk-based architecture
- Support for incremental updates and modifications
- Fast symbol resolution for selective linking
- Compatibility with AOF endianness rules

## File Structure

### Chunk-Based Architecture

Like AOF, ALF is built on the Chunk File Format, providing a uniform way to access distinct data chunks within a single file.

#### Chunk File Header

```
┌─────────────────────────────────────┐
│ ChunkFileId (0xC3CBC6C5)           │  Word 0
├─────────────────────────────────────┤
│ maxChunks                           │  Word 1
├─────────────────────────────────────┤
│ numChunks                           │  Word 2
├─────────────────────────────────────┤
│ Chunk Entry 1                       │  4 words
├─────────────────────────────────────┤
│ Chunk Entry 2                       │  4 words
├─────────────────────────────────────┤
│ ... (maxChunks entries total)         │
└─────────────────────────────────────┘
```

#### Chunk Entry Format

Each chunk entry is 4 words (16 bytes):

| Word | Field | Description |
|------|--------|-------------|
| 0    | chunkId (8 bytes) | 8-character identifier, endian-independent |
| 1    | fileOffset | Byte offset from start of file (word-aligned) |
| 2    | size | Exact byte size of chunk contents |

### ALF Chunk Types

ALF defines four standard chunk classes:

| Chunk Name | Identifier | Description | Required |
|------------|-------------|-------------|-----------|
| LIB_DIRY   | `LIB_DIRY`  | Directory of library members | Yes |
| LIB_TIME   | `LIB_TIME`  | Library modification timestamp | Yes |
| LIB_VRSN   | `LIB_VRSN`  | Version information | Yes |
| LIB_DATA   | `LIB_DATA`  | Library member data | Yes (multiple) |

For object code libraries, two additional chunks are defined:

| Chunk Name | Identifier | Description | Required |
|------------|-------------|-------------|-----------|
| OFL_SYMT   | `OFL_SYMT`  | External symbol table | Yes |
| OFL_TIME   | `OFL_TIME`  | Symbol table timestamp | Yes |

Chunks may appear in any order within the file, though LIB_DIRY, LIB_TIME, and LIB_VRSN conventionally appear first.

## LIB_DIRY Chunk - Library Directory

The directory chunk contains entries describing each library member and its location within the file.

### Directory Entry Format

Each directory entry is variable length but an integral number of words:

| Word Offset | Field | Description |
|-------------|-------|-------------|
| 0x00        | ChunkIndex | Index of corresponding LIB_DATA chunk (0 if unused) |
| 0x04        | EntryLength | Total size of this directory entry in bytes (multiple of 4) |
| 0x08        | DataLength | Size of data section in bytes (multiple of 4) |
| 0x0C        | Data | Variable-length data section |

### Data Section Structure

The data section contains, in order:

1. **Member Name** (NUL-terminated string)
   - ISO-8859 non-control characters (codes 32-126, 160-255)
   - Identifies the library member
   - Typically the original filename without extension

2. **Additional Information** (optional, often empty)
   - Tool-specific data
   - May contain compiler version, build info, etc.

3. **Time Stamp** (2 words, 8 bytes)
   - Encodes last modification time
   - Format described in Time Stamps section

### Directory Properties

- **ChunkIndex**: 0-based index in chunk file header
  - Conventionally ≥ 3 (first 3 chunks are LIB_DIRY, LIB_TIME, LIB_VRSN)
  - 0 indicates unused directory entry
- **EntryLength**: Always multiple of 4 bytes
- **DataLength**: Must be ≤ EntryLength
- **Padding**: Any unused bytes after NUL terminator should be 0

## LIB_TIME Chunk - Library Timestamp

Contains a 2-word timestamp recording when the library was last modified.

### Time Stamp Format

A library timestamp encodes:

- **6-byte count** of centiseconds since 00:00:00 January 1, 1900
- **2-byte count** of microseconds since the last centisecond

| Word | Bytes | Description |
|------|-------|-------------|
| 0    | 0-1   | Most significant 4 bytes of centisecond count |
| 1    | 2-3   | Least significant 2 bytes of centisecond count |
| 1    | 4-5   | Microsecond count (usually 0) |

### Encoding

- Stored in target system byte order (same as library endianness)
- Total timestamp resolution: 10 microseconds
- Microsecond field typically 0 unless high precision needed

## LIB_VRSN Chunk - Version Information

Contains library format version information.

### Format

```
┌─────────────────────────────────────┐
│ Version (word)                       │ Value = 1
└─────────────────────────────────────┘
```

- Single word containing value 1
- Indicates ALF version 1.00
- Reserved for future version compatibility

## LIB_DATA Chunk - Library Member Data

Contains the actual data for a single library member (typically an AOF file).

### Format

```
┌─────────────────────────────────────┐
│ Library Member Data                 │ Variable size
└─────────────────────────────────────┘
```

### Properties

- **Data Type**: By convention, contains AOF file data
- **Endianness**: Same as containing library file
- **Interpretation**: Library tools place no interpretation on contents
- **Possibilities**: 
  - AOF file (most common)
  - Another library file (nested libraries)
  - Any data in chunk file format

## OFL_SYMT Chunk - External Symbol Table

Present only in object code libraries, this chunk provides an index of external symbols defined by library members.

### Symbol Entry Format (similar to LIB_DIRY)

| Word Offset | Field | Description |
|-------------|-------|-------------|
| 0x00        | ChunkIndex | Index of LIB_DATA chunk containing symbol definition |
| 0x04        | EntryLength | Size of this symbol table entry in bytes (multiple of 4) |
| 0x08        | DataLength | Size of symbol name + padding in bytes (multiple of 4) |
| 0x0C        | Data | Symbol name (NUL-terminated) + padding |

### Symbol Entry Properties

- **ChunkIndex**: Points to LIB_DATA chunk defining the symbol
- **Symbol Name**: NUL-terminated string
  - Must be valid symbol identifier
  - Case-sensitive matching
- **Padding**: Bytes after NUL terminator must be 0
- **No Time Stamp**: Unlike directory entries, no timestamp stored

### Symbol Resolution

- Enables linker to quickly locate which library member defines a symbol
- Supports selective loading of only required library members
- Provides fast symbol resolution without scanning all members

## OFL_TIME Chunk - Symbol Table Timestamp

Records when the external symbol table was last modified.

### Format

Identical to LIB_TIME chunk:
- 2-word timestamp
- Same encoding as library timestamps
- Indicates when OFL_SYMT was last updated

## Data Types and Alignment

### Basic Data Types

| Type | Size | Alignment | Description |
|-------|-------|------------|-------------|
| byte   | 1 byte | Any boundary | Unsigned 8-bit value |
| halfword | 2 bytes | 2-byte boundary | Unsigned 16-bit value |
| word   | 4 bytes | 4-byte boundary | Unsigned 32-bit value |
| string | variable | Any boundary | NUL-terminated sequence |

### Endianness

Two variants of ALF exist, matching AOF:

#### Little-Endian ALF
- Least significant byte at lowest address
- Used by DEC, Intel, Acorn systems
- Default for most ARM development

#### Big-Endian ALF  
- Most significant byte at lowest address
- Used by IBM, Motorola, Apple systems
- Must match target system endianness

### Compatibility Rules

- Library members must have same endianness as library
- Linker accepts either endian format but rejects mixed inputs
- All word values stored in target system byte order
- Strings stored in ascending address order (endianness-independent)

## Library Operations

### Creating a Library

1. Initialize chunk file header with sufficient entries
2. Create LIB_DIRY chunk with empty entries
3. Create LIB_TIME chunk with current timestamp
4. Create LIB_VRSN chunk with version 1
5. For each member:
   - Read member file (typically AOF)
   - Create LIB_DATA chunk with member data
   - Update corresponding LIB_DIRY entry
6. If object library, build OFL_SYMT chunk
7. Create OFL_TIME chunk if OFL_SYMT present

### Adding Members

1. Find unused directory entry or extend LIB_DIRY
2. Create new LIB_DATA chunk at file end
3. Update directory entry with new member information
4. Update LIB_TIME timestamp
5. If object library, update OFL_SYMT and OFL_TIME
6. Update chunk file header as needed

### Removing Members

1. Mark directory entry as unused (set ChunkIndex to 0)
2. Optionally mark LIB_DATA chunk as unused (set offset to 0)
3. Update LIB_TIME timestamp
4. If object library, remove symbol entries from OFL_SYMT
5. Update OFL_TIME timestamp
6. Compact file if desired (copy active chunks)

### Symbol Lookup

For object code libraries:

```c
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    // char symbol_name[] follows
} OFLSymbolEntry;

OFLSymbolEntry* find_symbol(char* symbol_name, OFLSymbolEntry* symtab, 
                                int num_symbols, char* string_pool) {
    for (int i = 0; i < num_symbols; i++) {
        char* name = string_pool + (symtab[i] - symtab[0]);
        if (strcmp(name, symbol_name) == 0) {
            return &symtab[i];
        }
    }
    return NULL;
}
```

## Usage Examples

### Reading Library Directory

```c
typedef struct {
    uint32_t chunk_index;
    uint32_t entry_length;
    uint32_t data_length;
    // Variable length data follows
} LIBDirEntry;

void process_library_directory(uint8_t* lib_data) {
    // Find LIB_DIRY chunk
    LIBDirEntry* entries = (LIBDirEntry*)find_chunk(lib_data, "LIB_DIRY");
    
    int num_entries = chunk_size / sizeof(LIBDirEntry);
    uint8_t* data_ptr = (uint8_t*)entries + num_entries * sizeof(LIBDirEntry);
    
    for (int i = 0; i < num_entries; i++) {
        if (entries[i].chunk_index == 0) continue; // Unused entry
        
        char* name = (char*)data_ptr;
        printf("Member %d: %s (chunk %d)\n", 
               i, name, entries[i].chunk_index);
        
        // Advance to next entry
        data_ptr += entries[i].entry_length;
    }
}
```

### Extracting Library Members

```c
uint8_t* extract_member(uint8_t* lib_data, const char* member_name) {
    LIBDirEntry* entries = get_directory_entries(lib_data);
    int num_entries = get_directory_count(lib_data);
    uint8_t* data_ptr = get_directory_data(lib_data);
    
    for (int i = 0; i < num_entries; i++) {
        if (entries[i].chunk_index == 0) continue;
        
        char* name = (char*)data_ptr;
        if (strcmp(name, member_name) == 0) {
            // Find corresponding LIB_DATA chunk
            return find_chunk_by_index(lib_data, entries[i].chunk_index);
        }
        
        data_ptr += entries[i].entry_length;
    }
    
    return NULL; // Not found
}
```

### Creating Object Library

```c
void create_object_library(const char** aof_files, int num_files, 
                      const char* output_lib) {
    // Count total symbols needed
    int total_symbols = 0;
    for (int i = 0; i < num_files; i++) {
        total_symbols += count_symbols_in_aof(aof_files[i]);
    }
    
    // Create library structure
    LibraryBuilder* lib = create_library_builder();
    lib->add_chunk(lib, "LIB_DIRY", NULL, 0);
    lib->add_chunk(lib, "LIB_TIME", get_current_timestamp(), 8);
    lib->add_chunk(lib, "LIB_VRSN", "\x01\x00\x00\x00", 4);
    
    // Process each AOF file
    for (int i = 0; i < num_files; i++) {
        uint8_t* aof_data = read_file(aof_files[i]);
        int chunk_index = lib->add_chunk(lib, "LIB_DATA", aof_data, file_size);
        
        // Add directory entry
        LIBDirEntry entry;
        entry.chunk_index = chunk_index;
        entry.entry_length = calculate_entry_length(aof_files[i]);
        entry.data_length = entry.entry_length - sizeof(entry) - timestamp_size;
        lib->add_directory_entry(lib, &entry, aof_files[i]);
        
        // Add symbols to OFL_SYMT
        add_aof_symbols_to_library(lib, aof_data, chunk_index);
    }
    
    lib->write_library(lib, output_lib);
}
```

## Implementation Notes

### Time Stamp Calculations

```c
typedef struct {
    uint32_t high_centiseconds;  // Bits 31-16 of 6-byte count
    uint16_t low_centiseconds;   // Bits 15-0 of 6-byte count  
    uint16_t microseconds;       // Usually 0
} LibraryTimestamp;

LibraryTimestamp get_current_timestamp(void) {
    // Get current time
    time_t now = time(NULL);
    
    // Calculate centiseconds since 1900-01-01 00:00:00
    time_t base = mktime(&base_date_1900);
    uint64_t centiseconds = (now - base) * 100;
    
    LibraryTimestamp ts;
    ts.high_centiseconds = centiseconds >> 16;
    ts.low_centiseconds = centiseconds & 0xFFFF;
    ts.microseconds = 0;
    
    return ts;
}
```

### Chunk Management

```c
typedef struct {
    char id[8];           // "LIB_DIRY", "LIB_DATA", etc.
    uint32_t file_offset;  // Word-aligned offset
    uint32_t size;         // Exact byte size
} ChunkEntry;

typedef struct {
    uint32_t magic;       // 0xC3CBC6C5
    uint32_t max_chunks;
    uint32_t num_chunks;
    ChunkEntry entries[];   // Variable size
} ChunkFileHeader;

ChunkEntry* find_chunk_entries(uint8_t* file_data, const char* chunk_id) {
    ChunkFileHeader* header = (ChunkFileHeader*)file_data;
    ChunkEntry* entries = header->entries;
    
    for (uint32_t i = 0; i < header->num_chunks; i++) {
        if (memcmp(entries[i].id, chunk_id, 8) == 0) {
            return &entries[i];
        }
    }
    
    return NULL;
}
```

## Performance Considerations

### Symbol Lookup Optimization

- **Hash Tables**: Build hash of symbol names for O(1) lookup
- **Binary Search**: Sort symbol entries for binary search
- **Index Caching**: Cache frequently accessed member indices

### File Access Patterns

- **Sequential Reading**: Directory entries are sequential
- **Random Access**: Member data access requires chunk lookup
- **Memory Mapping**: Large libraries benefit from memory mapping

### Compact Storage

- **String Deduplication**: Common strings can be shared
- **Delta Encoding**: Similar timestamps can use delta encoding
- **Compression**: Optional compression for large libraries

## Limitations

### File Size Limits

- **Maximum Chunks**: Limited by max_chunks in header
- **Member Count**: Practical limit ~32K due to chunk indexing
- **Symbol Count**: Limited by available address space

### Functional Limitations

- **No Versioning**: No built-in support for multiple member versions
- **No Dependencies**: No explicit dependency tracking between members
- **No Compression**: No built-in compression (can be added externally)
- **Fixed Endianness**: Cannot mix little and big-endian members

### Performance Limits

- **Linear Directory Scan**: Requires scanning entire directory for operations
- **No Indexing**: No built-in indexing for fast lookups
- **Memory Usage**: Large directories consume significant memory

## References

- ARM DUI 0041C Reference Guide (1997-1998)
- ARM Software Development Toolkit User Guide  
- ARM Librarian (armlib) Documentation
- RISC OS Programmer's Reference Manuals
- AOF and ALF file format specifications