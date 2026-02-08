# ARM Shared Library Format

ARM Shared Library format directly supports:

- Shared code in ROM
- Single-address-space, loadable, shared libraries

Output in ARM Shared Library Format generates 2 files:

- A read-only, position-independent, reentrant shared library, written as a plain binary file.
- A *stub* file containing read-write data, entry vectors, etc., written in ARM Object Format, with which clients of the shared library will subsequently be linked.

Optionally, a shared library can contain a read-only copy of its initialised static data which can be copied at run time to a zero-initialised place holder in the stub. Such data must be free of relocation directives.

## Inputs

The outputs are created from:

- A set of input object files, between/from which there must be no unresolved symbolic references.
- A description of the shared library which includes the list of symbols to be exported from the library to its clients and a description of the data areas to be initialised at run time by copying from the shared library image.

Code to be placed in a shared library must be compiled with the reentrant option, or if it is assembled, it must conform to the shared library addressing architecture described in [The Shared Library Addressing Architecture](#the-shared-library-addressing-architecture).

## Stub Properties

The linker can generate a non-reentrant stub for use by non-reentrant client applications, or a reentrant stub which can be linked into another shared library or reentrant application.

The details of how a stub is initialised at load time or run time (so that a call to a stub entry point becomes a call to the corresponding library function) are system-specific. The linker provides a general mechanism for attaching code and data to both the library and the stub to support this. In particular:

- The linker appends a table of offsets of library entry points (the Exported Function Table or EFT) to the library, followed by a parameter block specified in the shared library description input to the linker.
- The linker writes the same parameter block to the stub, and initialises the stub entry vector so that the first call through any element of it is to the *dynamic linking code*. The dynamic linking code can patch the stub entry vector given only a pointer to its shared library's EFT. After dynamic linking, execution resumes by calling through the stub vector entry which initially invoked the dynamic linking code. The dynamic linking code will not be called again (for this shared library).
- If the library contains a read-only copy of its initialised static data, the linker writes the length and relocatable address of the place holder immediately before the stub parameter block and writes the length and offset of the initialising data immediately before the library parameter block. For uniformity of dynamic linking, the length and address or offset can be zero, denoting that neither initialising data nor a stub place holder are present in this shared library (though they may be present in other shared libraries handled by the same dynamic linker).

Provided the stub entry vector is writable, the only system-specific part of the matching of the stub to (a compatible version of) its library, is the location of the library itself. In general, this is expected to be a system service, though it would be equally possible to search a table at a fixed address, or simply search the whole of ROM for a named library (the linker provides support for prepending a name, the offset of the EFT, and anything else that can be assembled to a shared library).

Alternatively, in support of more protected systems, the patching code can simply be a call to a system service which locates the matching library and patches the entry vector.

The patching of shared library entry vectors by the loader at load time is not directly supported. However, it would be a relatively simple extension to AIF to support this. In general, it is considered more efficient to patch on demand in systems with multiple shared libraries.

The user-specified parameter block mechanism allows fine control over, and diagnosis of the compatibility of a stub with a version of its shared library. This supports a variety of approaches to *foreverness*, without mandating foreverness where it would be inappropriate. This issue is discussed in [Versions, Compatibility and Foreverness](#versions-compatibility-and-foreverness).

## The Shared Library Addressing Architecture

The central issue for shared objects is that of addressing their clients' static data.

On ARM processors, it is very difficult, and/or inefficient to avoid the use of address constants when addressing static data, particularly the static data of separately compiled or assembled objects; (an address constant is a pointer which has its value bound at link time -- in effect, it is an execution-time constant).

Typically, in non-reentrant code, these address constants are embedded in each separately compiled or assembled code segment, and are, in turn, addressed relative to the program counter. In this organisation, all threadings of the code address the same, link-time bound static data.

In a reentrant object, these address constants (or adcons) are collected into a separate area (in AOF terminology called a *based* area) which is addressed via the *sb* register. When reentrant objects are linked together, the linker merges these adcon areas into a single, contiguous adcon vector, and relocates the references into the adcon vector appropriately (usually by adjusting the offset of an `LDR ..., [sb, offset]` instruction). The output of this process is termed a *link unit*.

In this organisation, it is possible for different threadings of the code to address different static data, and for the binding of the code to its data to be delayed until execution time (an excellent idea if the code has to be committed to ROM, even if reentrancy is not required).

When control passes into a new link unit, a new value of sb has to be established; when control returns, the old value must be restored. A call between two separately linked program fragments is called an *inter link unit call*, or *inter-LU* call. The inter-LU calls are precisely the calls between a shared library's stub and the library's matching entry points.

Because an LDR instruction has a limited (4KB) offset, the linker packs adcons into the low-address part of the *based-sb* area. It is a current restriction that there can be no more than 1K adcons in a client application (but this number seems adequate to support quite large programs using several megabytes of sharable code).

The linker places the data for the inter-LU entry veneers immediately after the adcon vector (still in the based-sb area). If the stub is reentrant (to support linking into other shared libraries), then the inter-LU entry data consists of:

- The data part of the inter-LU veneer for each direct inter-LU call (which is addressed sb-relative from the separate inter-LU code part).
- The entry veneer for each address-taken library function (i.e. for each function that could be invoked via a function pointer).

If the stub is not reentrant, then the inter-LU entry data is an array of function variable veneers, one for each directly exported or address-taken function in the library.

A reentrant function called via a function pointer or from a non-reentrant caller, must have its sb value loaded pc-relative, as there is no sb value relative to which to load it. In turn, this forces the entry veneer to be part of the client's private data (or there could be no reentrancy).

## Including Data Areas in a Shared Library

Usually, a shared library only includes areas which have both the CODE and READONLY attributes.

When you ask for a read-only copy of a data area to be included in a shared library, the linker checks it is a simple, initialised data area. The following *cannot* be included in a shared library:

- Zero-initialised data (these always remain in the stub)
- COMMON data
- Stub data from the stubs of other shared libraries with which this library is being linked
- Inter-link-unit entry data and address constants

When an area is found to be suitable for inclusion in a shared library, the following is done:

- A clone of the area is created with the name `SHL$$data` and the attribute READONLY. It inherits its initialising data from the original area but it inherits no symbol definitions.
- The original area is renamed `$$0` and given the attribute 0INIT. It inherits all of the symbols defined in the original area.

Area renaming is essential to ensure that multiple input areas will be sorted identically by the linker in both the stub and the sharable library and that both the placeholder and its initialising data will be sorted into contiguous chunks. This identically ordered contiguity -- together with the absence of relocation directives -- allows the placeholder to be initialised by directly copying its initialising image from the sharable library.

Names containing `$$` are reserved to the implementors of the ARM software development tools, so these linker-invented area names cannot clash with any area name you choose yourself.

## Entry Veneers

The inter-LU code for a direct, reentrant inter-LU call is:

```asm
FunctionName
    ADD    ip, sb, #V1          ; calculate offset of veneer data from sb
    ADD    ip, ip, #V2
    LDMIA  ip, {ip, pc}         ; load new-sb and pc values
```

This allows up to 32K entry veneers to be addressed (V1 and V2 are jointly relocated by the linker and support a word offset in the range 0-65K). The corresponding inter-LU data is:

```asm
    DCD    new-sb               ; sb value for called link unit
    DCD    entry-point           ; address of the library entry point
```

Both of these values are created when the stub is patched, as introduced above and described in detail below.

The inter-LU code for an indirect or non-reentrant inter-LU call is:

```asm
FunctionName
    ADD    ip, pc, #0           ; ip = pc+8
    LDMIA  ip, {ip, pc}         ; load new-sb and pc values
    DCD    new-sb               ; sb value for called link unit
    DCD    entry-point           ; address of the library entry point
```

Again, the data values are created when the stub is patched.

### Entry Veneer Initial Values

The linker initialises the data part of each entry veneer as follows:

- **new-sb**: the index of the entry point in the array of entry points (note that the entries may not be of uniform length in the reentrant case).
- **entry-point**: the address of a 4-word code block, placed at the end of the inter-LU data by the linker.

Overall, an adcon/inter-LU-data area for a link unit has the layout:

```
Base                            ; sb points here
    <adcon vector>
    <inter-LU entry data>
End
    STMFD  sp!, {r0-r6,r14}    ; save work registers and lr
    LDR    r0, End-4            ; load address of End
    B      |__rt_dynlink|       ; do the dynamic linking...
    DCD    Params - Base        ; offset to sb-value
Params
    <parameter block>
```

Note the assumption that a stack has been created *before* any attempt is made to access the shared library. Note also that the word preceding *End* is initialised to the address of *End*.

### Entry Veneer Patching

A simple version of the dynamic linking code, `__rt_dynlink`, referred to above, can be implemented as described in this section.

On entry to `__rt_dynlink`, a copy of the pointer is saved to the code/parameter block at the end of the inter-LU data area, and a bound is calculated on the stub size (the entries are in index order).

```asm
|__rt_dynlink|
    MOV    r6, r0
    LDR    r5, [r6, #-8]       ; max-entry-index
    ADD    r5, r5, #1           ; # max entries in stub
    MOV    r4, ip               ; resume index
```

Then it is necessary to locate the matching library, which the following fragment does in a simple system-specific fashion. Note that in a library which contains no read-only static data image, `r0+16` identifies the user parameter block (at the end of the inter-LU data area); if the library contains an initialising image for its static data then `r0+24` identifies the user parameter block.

Here, the library location function is shown as a SWI which takes as its argument in r0 a pointer to the user parameter block and returns the address of the matching External Function Table in r0:

```asm
    ADD    r0, r6, #24          ; stub parameter block address
    SWI    Get_EFT_Address      ; are you there?
    BVS    Botched              ; system-dependent
```

R0 now points to the EFT, which begins with the number of entries in it. A simple sanity check is that if there are fewer entries in the library than in the stub, it has probably been patched incorrectly.

```asm
    LDR    ip, [r0]             ; #entries in lib
    CMPS   ip, r5               ; >= #max entries in stub?
    BLT    Botched              ; no, botched it...
```

If the shared library contains data to be copied into the stub then check the length to copy:

```asm
    LDR    ip, [r6, #16]        ; stub data length
    BIC    ip, ip, #3           ; word aligned, I insist...
    ADD    r3, r6, #4
    LDR    r3, [r3, r5, LSL #2] ; library data length
    CMPS   r3, ip
    BNE    Botched              ; library and stub lengths differ
```

Checking the stub data length and library data length match is a naive, but low-cost, way to check the library and the stub are compatible. Now copy the static data from the library to the stub:

```asm
    LDR    r3, [r6, #20]        ; stub data destination
    SUB    r2, r0, ip           ; library data precedes the EFT
01  SUBS   ip, ip, #4           ; word by word copy loop
    LDRGE  r1, [r2], #4
    STRGE  r1, [r3], #4
    BGE    %B01
```

Then initialise the entry vectors. First, the sb value is computed for the callee:

```asm
    LDR    ip, [r6, #12]        ; length of inter-LU data area
    ADD    r3, r6, #24          ; end of data area...
    SUB    r3, r3, ip           ; start of data area = sb value
```

If there is no static data in the library then `#24` above becomes `#16`.

Then the following loop works backwards through the EFT indices, and backwards through the inter-LU data area, picking out the indices of the EFT entries which need to be patched with an sb, entry-point pair. ip still holds the index of the entry which caused arrival at this point, which is the index of the entry to be retried after patching the stub. The corresponding retry address is remembered in r14, which was saved by the code fragment at the end of the inter-LU data area before it branched to `__rt_dynlink`. A small complication is that the step back through a non-reentrant stub may be either 8 bytes or 16 bytes. However, there can be no confusion between an index (a small integer) and an ADD instruction, which has some top bits set.

```asm
    LDR    r2, [r6, #-8]!       ; index of stub entry
00  SUB    ip, r5, #1           ; index of the lib entry
    CMPS   ip, r2               ; is this lib entry in the stub?
    SUBGT  r5, r5, #1           ; no, skip it
    BGT    %B00
    CMPS   r2, r4               ; found the retry index?
    MOVEQ  lr, r6               ; yes: remember retry address
    LDR    ip, [r0, r5, lsl #2] ; entry point offset
    ADD    ip, ip, r0           ; entry point address
    STMIA  r6, {r3, ip}         ; save {sb, pc}
    LDR    r2, [r6, #-8]!       ; load index and decrement r6...
    TST    r2, #0xFF000000      ; ... or if loaded an instr?
    LDRNE  r2, [r6, #-8]!       ; ...load index and decrement r6
    SUBS   r5, r5, #1           ; #EFT entries left...
    BGT    %B00
```

Finally, when the vector has been patched, the failed call can be retried:

```asm
    MOV    ip, lr               ; retry address
    LDMFD  sp!, {r0-r6, lr}     ; restore saved regs
    LDMIA  ip, {ip, pc}         ; and retry the call
```

## Versions, Compatibility and Foreverness

The mechanisms described so far are very general and, of themselves, give no guarantee that a stub and a library will be compatible, unless the stub and the library were the complementary components produced by a single link operation.

Often, in systems using shared libraries, stubs are bound into applications which must continue to run when a new release of the library is installed. This requirement is especially compelling when applications are constructed by third party vendors or end users.

The general requirements for compatibility are as follows:

- A library must be at least as new as the calling stub.
- Libraries can only be extended, and then only by *disjoint extension* (adding new entries to a library, or by giving to existing entries new interpretations to previously unused parameter value combinations).

In general, the compatibility of a stub and a library can be reduced to the compatibility of their respective versions. The ARM shared library mechanism does not mandate how versions are described, but provides an open-ended parameter block mechanism which can be used to encode version information to suit the intended usage.

Because the addresses of library entry points are not bound into a stub until run-time, the only foreverness guarantees a library must give are:

- Its entry points are in the same order in its EFT (this is a property of the shared library description given to the linker, not of the library's implementation).
- The behaviour of each exported function must be maintained compatibly between releases (beware that it is genuinely difficult to prevent users relying on unintended behaviour -- the curse of bug compatibility).

Because a stub contains the indices of the entry points it needs, it is harmless to add new entry points to a library: the dynamic linking code simply ignores them. Of course, they must be added to the end of the list of exported functions if the first property, above, is to be maintained.

For libraries which export only code, and which make no use of static data, compatibility is straightforward to manage. Use of static data is more hazardous, and the direct export of it is positively lethal.

If a static data symbol is exported from a shared library, what is actually exported is a symbol in the library's stub. This symbol is bound when the stub is linked into an application and, from that instant onwards, cannot be unbound. Thus the direct export of a data symbol fixes the offset and length of the corresponding datum in the shared library's data area, forever (i.e. until the next incompatible release).

The linker does not fault the direct export of data symbols because the ARM shared library mechanism may not be being used to build a shared library, but is instead being used to structure applications for ROM. In this case a prohibition could be irksome. Those specifying or building genuine shared libraries need to be aware of this issue, and should generally not make use of directly exported data symbols. If data must be exported directly then:

- Only export data which has very stable specifications (semantics, size, alignment, etc.).
- Place this data first in the library's data area, to allow all other non-exported data to change size and shape in future releases (subject to its total size remaining constant).

If a shared library makes any use of static data then it is prudent to include some unused space, so that non-exported data may change size and shape (within limits) in future releases without increasing the total size of the data area. Remember that if a *forever binary* guarantee is given, the size of the data area may never be increased.

In practice, it is rare for the direct export of static data to be genuinely necessary. Often a function can be written to take a pointer to its static data as an argument, or a function can be used to return the address of the relevant static data (thus delaying the binding of the offset and size of the datum until run-time, and avoiding the foreverness trap). It is only if references to a datum are frequent and ubiquitous that direct export is unavoidable. For example, a shared library implementation of an ANSI C library might export directly `errno`, `stdin`, `stdout` and `stderr` (and even `errno` could be replaced by `(*__errno())`, with few implications).

## Describing a Shared Library to the Linker

A shared library description consists of a sequence of lines. On all lines, leading white space (blank, tab, VT, CR) is ignored.

If the first significant character of a line is a semicolon (`;`) then the line is ignored. Lines beginning with `;` can be used to embed comments in a shared library description. A comment can also follow a `\` which continues a parameter block description.

If the first significant character of a line is `>` then the line gives the name and parameter block for the library. Such lines can be continued over several contiguous physical lines by ending all but the last line with `\`. For example:

```
> testlib        \        ; the name of the library image file
  "testlib"      \        ; the text name of the library -> parameter block
  101            \        ; the library version number
  0x80000001
```

The first word following the `>` is the name of the file to hold the shared library binary image; the argument to the linker's `-Output` option is used to name the stub. Following tokens are parameter block entries, each of which is either a quoted string literal or a 32-bit integer. In the parameter block, each entry begins on a 4-byte boundary.

Within a quoted string literal, the characters `"` and `\` must be preceded by `\` (the same convention as in the C programming language). Characters of a string are packed into the parameter block in ascending address order, followed by a terminating NUL and NUL padding to the next 4-byte boundary.

An integer is written in any way acceptable to the ANSI C function `strtoul()` with a base of 0. That is, as an optional `-` followed by one of:

- A sequence of decimal digits, not beginning with `0`
- A `0` followed by a sequence of octal digits
- `0x` or `0X` followed by a sequence of hexadecimal digits

Values which overflow or are otherwise invalid are not diagnosed.

### Data Area Inclusion

A line beginning with a `+` describes input data areas to be included, read-only, in the shared library and copied at run time to place holders in the library's clients. The general format of such lines is a list of *object(area)* pairs instructing the linker to include area *area* from object *object*:

```
+ object ( area ) object ( area ) ...
```

If *object* is omitted then any object in the input list will match. For example:

```
+ (C$$data)
```

instructs the linker to include all areas called `C$$data`, whatever objects they are from.

If *area* is omitted too, then all suitable input data areas will be included in the library. This is the most common usage. For example:

```
+ ()
```

Finally, a `+` on its own *excludes* all input data areas from the shared library but instructs the linker to write zero length and address or offset words immediately preceding the stub and library parameter blocks, for uniformity of dynamic linking.

### Export List

All remaining non-comment lines are taken to be the names of library entry points which are to be exported, directly or via function pointers. Each such line has one of the following three forms:

```
entry-name
entry-name()
entry-name(object-name)
```

- The first form names a directly exported global symbol: a direct entry point to the library, or the name of an exported datum (deprecated).
- The second form names a global code symbol which is exported indirectly via a function pointer. Such a symbol may also be exported directly.
- The third form names a non-exported function which, nonetheless, is exported from the library by being passed as a function argument, or by having its address taken by a function pointer.

To clarify this, suppose the library contains:

```c
void f1(...) {...}
void f2(...) {...}
static void f3(...) {...}               /* from object module o3.o */
static void (*fp2)(...) = f2;
void (*pf3)(...) = f3;
```

...and that *f1* is to be exported directly. Then a suitable description is:

```
f1
f2()
f3(o3)
pf3                /* deprecated direct export of a datum */
```

Note that *f2* and *f3* have to be listed even though they are not directly exported, so that function variable veneers can be created for them.

*f3* must be qualified by its object module name, as there could be several non-exported functions with the same name (each in a differently named object module). Note that the *module* name, not the name of the file containing the object module, is used to qualify the function name.

If *f2* were to be exported directly then the following list of entry points would be appropriate:

```
f1
f2
f2()
f3(o3)
pf3
```

Unless all address-taken functions are included in the export list, the linker will complain and refuse to make a shared library.

## Linker Pre-defined Symbols

While a shared library is being constructed the linker defines several useful symbols:

| Symbol | Definition |
|---|---|
| `EFT$$Offset` | Offset of the External Function Table from the beginning of the shared library. |
| `EFT$$Params` | Offset of the shared library's parameter block from its beginning. |
| `$$0$$Base` | The (relocatable) address of the zero-initialised place holder in the stub. |
| `SHL$$data$$Base` | Offset of the start of the read-only copy of the data from the beginning of the shared library. |
| `SHL$$data$$Size` | Size of the shared library's data section, which is also the size of the place holder in the stub. |

`EFT$$Offset` and `EFT$$Params` are exported to the stub and may be referred to in following link steps; the others exist only while the shared library is being constructed.
