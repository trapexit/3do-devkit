# Linker Pre-defined Symbols

There are several symbols which the linker defines independently of any of its input files. The most important of these start with the string `Image$$`. These symbols, along with all other external names containing `$$`, are reserved by ARM.

## Image-Related Symbols

| Symbol | Description |
|---|---|
| `Image$$RO$$Base` | The address of the start of the read-only area (usually this contains code). |
| `Image$$RO$$Limit` | The address of the byte beyond the end of the read-only area. |
| `Image$$RW$$Base` | The address of the start of the read/write area (usually this contains data). |
| `Image$$RW$$Limit` | Address of the byte beyond the end of the read/write area. |
| `Image$$ZI$$Base` | Address of the start of the 0-initialised area (zeroed at image load or start-up time). |
| `Image$$ZI$$Limit` | Address of the byte beyond the end of the zero-initialised area. |

## Object/Area-Related Symbols

| Symbol | Description |
|---|---|
| `areaname$$Base` | The address of the start of the consolidated area called *areaname*. |
| `areaname$$Limit` | The address of the byte beyond the end of the consolidated area called *areaname*. |

## Notes

`Image$$RO$$Limit` need not be the same as `Image$$RW$$Base`, although it often will be in simple cases of `-AIF` and `-BIN` output formats. `Image$$RW$$Base` is generally different from `Image$$RO$$Limit` if:

- The `-DATA` option is used to set the image's data base (`Image$$RW$$Base`).
- Either of the `-SHL` or `-OVerlay` options is used to create a shared library or overlaid image, respectively.

It is poor programming practice to rely on `Image$$RO$$Limit` being the same as `Image$$RW$$Base`.

Note that the read/write (data) area may contain code, as programs sometimes modify themselves (or better, generate code and execute it). Similarly, the read-only (code) area may contain read-only data (for example string literals, floating-point constants, ANSI C `const` data).

These symbols can be imported and used as relocatable addresses by assembly language programs, or referred to as `extern` addresses from C (using the `-fC` compiler option which allows dollars in identifiers). Image region bases and limits are often of use to programming language run-time systems.

Other image formats (for example shared library format) have linker-defined symbolic values associated with them. These are documented in the relevant sections in this chapter, and in separate documents in the Technical Specifications.
