# Community Examples

| Example | Description |
|---------|-------------|
| `find_items` | Demonstrates locating a named kernel item (`MathSemaphore`) via `FindSemaphore()` and dereferencing it with `LookupItem()` to inspect internal fields such as the `n_Item` number. |
| `folio` | Demonstrates creating and using a custom SWI folio (`ExampleFolio`) with both user-callable and SWI-callable functions (`AddI32`, `SubI32`, `MADAMRev`, `CLIORev`), showing how to open, invoke, and close a folio. |
| `overwrite_folio_func` | Demonstrates low-level runtime patching of a folio's function table by using `svc_mem` to directly overwrite the `MulUF16` and `Cross3_F16` function pointers inside the Operamath folio with custom implementations. |
| `read_rom` | Demonstrates reading hardware memory regions using `svc_mem`, dumping 1MB of ROM1, 1MB of ROM2, and 32KB of NVRAM into RAM, then computing and displaying CRC32B checksums for each region. |
