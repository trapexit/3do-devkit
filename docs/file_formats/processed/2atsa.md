# About AOF

AOF is a simple object format, similar in complexity and expressive power to Unix's *a.out* format. As will be seen, it provides a generalised superset of a.out's descriptive facilities (though this was neither an original design goal nor an influence on the early development of AOF).

AOF was designed to be simple to generate and to process, rather than to be maximally expressive or maximally compact.

## Terminology

In the rest of this document, the term *object file* refers to a file in ARM Object Format, and the term *linker* refers to the ARM linker.

The terms *byte*, *half word*, *word*, and *string* are used to mean:

| Term      | Definition                                                                                                      |
|-----------|-----------------------------------------------------------------------------------------------------------------|
| byte      | 8 bits; considered unsigned unless otherwise stated; usually used to store flag bits or characters               |
| half word | 16 bits, or 2 bytes; usually considered unsigned                                                                |
| word      | 32 bits, or 4 bytes; usually considered unsigned                                                                |
| string    | A sequence of bytes terminated by a NUL (`0x00`) byte. The NUL byte is part of the string but is not counted in the string's length |

## Byte Sex or Endian-ness

There are two sorts of AOF: *little-endian* and *big-endian*.

- In **little-endian** AOF, the least significant byte of a word or half-word has the lowest address of any byte in the (half-)word. Used by DEC, Intel and Acorn, amongst others.
- In **big-endian** AOF, the most significant byte of a (half-)word has the lowest address. Used by IBM, Motorola and Apple, amongst others.

For data in a file, *address* means offset from the start of the file.

There is no guarantee that the endian-ness of an AOF file will be the same as the endian-ness of the system used to process it (the endian-ness of the file is always the same as the endian-ness of the target ARM system).

The two sorts of AOF cannot be mixed (the target system cannot have mixed endian-ness: it must have one or the other). Thus the ARM linker will accept inputs of either sex and produce an output of the same sex, but will reject inputs of mixed endian-ness.

## Alignment

Strings and bytes may be aligned on any byte boundary.

AOF fields defined in this document make no use of half-words and align words on 4-byte boundaries.

Within the contents of an AOF file the alignment of words and half-words is defined by the use to which AOF is being put. For all current ARM-based systems, words are aligned on 4-byte boundaries and half-words on 2-byte boundaries.
