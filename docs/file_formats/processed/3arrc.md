# Area Placement and Sorting Rules

Each object module in the input list, and each subsequently included library module contains at least one *area*. AOF areas are the fragments of code and data manipulated by the linker. In all output types except AOF, except where overridden by a `-FIRST` or `-LAST` option, the linker sorts the set of areas first by attribute, then by area name to achieve the following:

- The read-only parts of the image are collected into one contiguous region which can be protected at run time on systems which have memory management hardware. Page alignment between the read-only and read-write portions of the image can be forced using the area alignment attribute of AOF areas, set using the `ALIGN=n` attribute of the ARM assembler `AREA` directive.
- Portions of the image associated with a particular language run-time system are collected together into a minimum number of contiguous regions (this applies particularly to code regions which may have associated exception handling mechanisms).

## Area Ordering by Attribute

More precisely, the linker orders areas by attribute as follows:

```
read-only code
read-only based data
read-only data
read-write code
based data
other initialised data
zero-initialised (uninitialised) data
debugging tables
```

In some image types (AIF, for example), zero-initialised data is created at image initialisation time and does not appear in the image itself.

Debugging tables are included only if the linker's `-Debug` option is used. A debugger is expected to retrieve the debugging tables before the image is entered. The image is free to overwrite its debugging tables once it has started executing.

## Secondary Ordering

Areas unordered by attribute are ordered by AREA name. The comparison of names is lexicographical and case sensitive, using the ASCII collation sequence for characters.

Identically attributed and named areas are ordered according to their relative positions in the input list.

The `-FIRST` and `-LAST` options can be used to force particular areas to be placed first or last, regardless of their attributes, names or positions in the input list.

## Library Area Ordering

As a consequence of these rules, the positioning of identically attributed and named areas included from libraries is not predictable. However, if library L1 precedes library L2 in the input list, then all areas included from L1 will precede each area included from L2. If more precise positioning is required then modules can be extracted manually, and included explicitly in the input list.

## Padding and Alignment

Once areas have been ordered and the base address has been fixed, the linker may insert padding to force each area to start at an address which is a multiple of 2^(area alignment) (but most commonly, *area alignment* is 2, requiring only word alignment).
