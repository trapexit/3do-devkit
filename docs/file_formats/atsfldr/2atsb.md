# The Overall Structure of an AOF File

An AOF file contains a number of separate but related pieces of data. To simplify access to these data, and to give a degree of extensibility to tools which process AOF, the object file format is itself layered on another format called *Chunk File Format*, which provides a simple and efficient means of accessing and updating distinct chunks of data within a single file. The object file format defines five chunks:

- The AOF header
- The AOF areas
- The producer's identification
- The symbol table
- The string table

These are described in detail after the description of chunk file format.

## Chunk File Format

A chunk is accessed via a header at the start of the file. The header contains the number, size, location and identity of each chunk in the file.

The size of the header may vary between different chunk files, but is fixed for each file. Not all entries in a header need be used, thus limited expansion of the number of chunks is permitted without a wholesale copy.

A chunk file can be copied without knowledge of the contents of its chunks.

Pictorially, the layout of a chunk file is as follows:

![Chunk Layout](Figures/ChunkLayout.gif)

- **ChunkFileId** - marks the file as a chunk file. Its value is `0xC3CBC6C5`. The endian-ness of the chunk file can be deduced from this value (if, when read as a word, it appears to be `0xC5C6CBC3` then each word value must be byte-reversed before use).
- **maxChunks** - defines the number of the entries in the header, fixed when the file is created.
- **numChunks** - defines how many chunks are currently used in the file, which can vary from 0 to maxChunks. numChunks is redundant in that it can be found by scanning the entries.

Each entry in the chunk file header consists of four words in order:

| Field      | Description                                                                                                                                                                                                   |
|------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| chunkId    | 8-byte field identifying what data the chunk contains; (note that this is an 8-byte field, not a 2-word field, so it has the same byte order independent of endian-ness)                                      |
| fileOffset | A one word field defining the byte offset within the file of the start of the chunk. All chunks are word-aligned, so it must be divisible by four. A value of zero indicates that the chunk entry is unused     |
| size       | A one word field defining the exact byte size of the chunk's contents (which need not be a multiple of four)                                                                                                  |

The chunkId field provides a conventional way of identifying what type of data a chunk contains. It is split into two parts. The first four characters contain a unique name allocated by a central authority. The remaining four characters can be used to identify component chunks within this domain. The 8 characters are stored in ascending address order, as if they formed part of a NUL-terminated string, independently of endian-ness.

For AOF files, the first part of each chunk's name is `"OBJ_"`; the second components are defined in the next section.
