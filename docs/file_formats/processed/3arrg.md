# Plain Binary Format

An image in plain binary format is a sequence of bytes to be loaded into memory at a known address. How this address is communicated to the loader, and where to enter the loaded image, are not the business of the linker.

## Requirements

In order to produce a plain binary output there must be:

- No unresolved symbolic references between the input objects (each reference must resolve directly or via an input library).
- An absolute base address (given by the `-Base` option to *armlink*).
- Complete performance of all relocation directives.

## Layout

Input areas having the read-only attribute are placed at the low-address end of the image; initialised writable areas follow; 0-initialised areas are consolidated at the end of the file where a block of zeroes of the appropriate size is written.
