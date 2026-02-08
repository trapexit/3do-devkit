# The Overlay Manager

This section describes in detail how a static overlay manager operates. The details of a dynamic overlay manager are very similar. In both cases, details specific to the target system are omitted.

The job of the overlay manager is to load, swap, and unload, overlay segments. This is done by trapping inter-segment function calls.

References to data are resolved statically by the linker when each overlay segment is created. De-referencing a datum cannot cause an overlay segment to be loaded.

Every inter-segment procedure call is indirected through a table in the root segment that traps unloaded target overlay segments (the procedure call indirection table, or PCIT). PCITs are created by the linker.

Each overlay segment contains the data required to initialise its section of the PCIT when it is loaded. This is simply a table of `B fn` instructions, one for each function exported by the overlay segment. As the linker knows the locations of each segment of the PCIT and of each function exported by each overlay segment, it can create these `B fn` instructions at link time.

(In a dynamic overlay scheme, all segments, including the root, are assumed to be linked at 0, and a type 7 relocation directive is generated to describe the relocation of each of the initializing branch instructions).

Initially, every sub-section of the procedure-call indirection table (PCIT) in the root segment is initialised with:

```asm
STR    LR, [PC, #-8]
```

(One for each procedure exported by the corresponding overlay segment.)

A call to an entry in the root PCIT overwrites that entry, and every following entry, with the return address, until control falls off the end of that section of the PCIT into code which:

- Finds which entry was called.
- Loads the corresponding overlay segment (and executes its relocation directives, if it is relocatable).
- Overwrites the PCIT subsection with the associated branch vector (from the just-loaded overlay segment).
- Retries the call.

Future calls to this section of the PCIT will encounter instructions of the form `B fn`, adding only a few cycles to the procedure call overhead. This will persist until some function call, function return, or explicit call to the overlay manager causes this PCIT segment to be overwritten.

The load-segment code not only loads an overlay, but also re-initialises the PCIT sections corresponding to segments with which it cannot co-reside. It also installs handlers to catch returns to segments which have been unloaded.

## The Structure of a PCIT Section

The per-PCIT-section code and data structures are shown immediately below. These are created by the linker and used by the overlay manager. They are justified and explained in following subsections. The space cost of this code is `(9 + #Clashes + #Entries)` words per overlay segment. Most of the work is done in the function *load_seg_and_go* (which is shared between all PCIT sections), and in *load_segment* (which is the common tail for both *load_seg_and_go* and *load_seg_and_ret*). For an explanation of *load_seg_and_ret* see [Intercepting Returns to Overwritten Segments](#intercepting-returns-to-overwritten-segments).

```asm
        STR    LR, [PC, #-8]           ; guard word
EntryV  STR    LR, [PC, #-8]           ; > one entry for each
        ...                             ; > procedure exported
        STR    LR, [PC, #-8]           ; > by this overlay segment
        BL     load_seg_and_go
PCITSection
Vecsize DCD    .-4-EntryV              ; size of entry vector
Base    DCD    ...                      ; initialised by the linker
Limit   DCD    ...                      ; initialised by the linker
Name    DCB    <segment name>
Flags   DCB    0                        ; ...and a flag byte
ClashSz DCD    PCITEnd-.-4             ; size of table following
Clashes DCD    ...                      ; > table of pointers or offsets
        ...                             ; > to segments which cannot
        DCD    ...                      ; > co-reside with this one
PCITEnd
```

Pointers to clashing segments point to the appropriate `PCITSection` labels (i.e. into the middle of PCIT sections).

(If the overlays are relocatable, then offsets between `PCITSection` labels are used rather than addresses which would themselves require relocation.)

### Symbolic Offsets

Symbolic offsets from `PCITSection` for the data introduced here, used in the *load_seg_and_go* code:

```asm
O_Vecsize   EQU Vecsize-PCITSection
O_Base      EQU Base-PCITSection
O_Limit     EQU Limit-PCITSection
O_Name      EQU Name-PCITSection
O_Flags     EQU Flags-PCITSection
O_ClashSz   EQU ClashSz-PCITSection
O_Clashes   EQU Clashes-PCITSection
```

## The load_seg_and_go Code

The *load_seg_and_go* code contains a register save area which is shared with *load_seg_and_ret*. Both of these code fragments are veneers on *load_segment*. Both occur once in the overlay manager, not once per segment. Note that the register save area could be separated from the code and addressed via an address constant, as *ip* is available for use as a base register. For simplicity we ignore that here. Note also that *load_segment* and its veneers preserve *fp*, *sp*, and *sl*, which is vital.

```asm
    STRLR  STR LR, [PC, #-8]           ; a useful constant
    Rsave  %   10*4                     ; for R0-R9
    LRSave %   4
    PCSave %   4
load_seg_and_go
    STR    R9, RSave+9*4               ; save a base register...
    ADR    R9, RSave
    STMIA  R9, {R0-R8}                 ; ...and some working registers
    BIC    R8, LR, #0xFC000003         ; clear out status bits (26-bit mode)
    LDR    R0, [R8, #-8]              ; saved R14 from EntryV...
    STR    R0, LRSave                  ; ...save here ready for retry
    LDR    R0, STRLR                   ; look for this...
    SUB    R1, R8, #8                  ; ...starting at penultimate overwrite
01  LDR    R2, [R1, #-4]!
    CMP    R2, R0                      ; must stop on guard word...
    BNE    %B01
    ADD    R1, R1, #4                  ; gone one too far...
    AND    R0, LR, #0xFC000003         ; status bits at call (26-bit mode)
    ORR    R1, R1, R0
    STR    R1, PCSave                  ; where to resume at
    B      load_segment                ; ...and off to the common tail
```

On entry to `load_segment`, R9 points to a register save for `{R0-R9,LR,PC}`, and R8 identifies the segment to be loaded. FP, SP and SL are preserved at all times by the overlay segment manager. There is only one copy of *load_seg_and_go*, shared between all PCIT sections.

A similar section of code, called *load_seg_and_ret*, is invoked on return to an unloaded segment (see [Intercepting Returns to Overwritten Segments](#intercepting-returns-to-overwritten-segments)). This code is also a veneer on *load_segment* which shares RSave, LRSave and PCSave, and which branches to *load_segment* with R8 and R9 set up as described above.

Note that the code for `STR LR, [PC, #-8]` is `0xE50FE008`. This address is unlikely to be in application code space, so overwriting indirection table entries with an application's return addresses is safe.

## The load_segment Code

`load_segment` must carry out the following:

- Re-initialise the global PCIT sections for any overlay segment which "clashes" with this one, while checking the stack for return addresses which are invalidated by so doing, and installing return handlers for them.
- Allocate memory for the about-to-be-loaded segment (if the overlay scheme is dynamic) -- this is system-specific.
- Load the required overlay segment (system-specific).
- Execute the loaded segment's relocation directives (if any).
- Copy the overlay segment's PCIT into the global PCIT.
- Restore the saved register state (with pc and lr suitably modified).

On entry to `load_segment`, R9 points to the register save area, and R8 to the PCIT section of the segment to load. First the code must re-initialise the PCIT section (if any) which clashes with this one:

```asm
load_segment
    ADD    R1, R8, #O_Clashes
    LDR    R0, [R8, #O_ClashSz]
01  SUBS   R0, R0, #4
    BLT    Done_Reinit                 ; nothing left to do
    LDR    R7, [R1], #4               ; a clashing segment...
    ADD    R7, R7, R8                  ; only if root is relocatable
    LDRB   R2, [R7, #O_Flags]
    CMPS   R2, #0                      ; is it loaded?
    BEQ    %B01                        ; no, so look again
    MOV    R0, #0
    STRB   R0, [R7, #O_Flags]         ; mark as unloaded
    LDR    R0, [R7, #O_Vecsize]
    SUB    R1, R7, #4                  ; end of vector
    LDR    R2, STRLR                   ; init value to store...
02  STR    R2, [R1, #-4]!             ; >
    SUBS   R0, R0, #4                  ; > loop to initialise the
                                        ; PCIT segment
    BGT    %B02                        ; >
```

Next, the stack of call frames for return addresses invalidated by loading this segment is checked, and handlers are installed for each invalidated return. This is discussed in detail in the next subsection. Note that R8 identifies the segment being loaded, and R7 the segment being unloaded.

```asm
    BL     check_for_invalidated_returns
```

Segment clashes have now been dealt with, as have the re-setting of the segment-loaded flags and the intercepting of invalidated returns. It's now time to load the required segment. This is system specific, so the details are omitted; (the name of the segment is at offset `O_Name` from R8).

On return, calculate and store the real base and limit of the loaded segment and mark it as loaded:

```asm
    BL     _host_load_segment          ; return base address in R0

    LDR    r4, [r8, #PCITSect_Limit]
    LDR    r1, [r8, #PCITSect_Base]
    SUB    r1, r4, r1                  ; length
    STR    r0, [r8, #PCITSect_Base]    ; real base
    ADD    r0, r0, r1                  ; real limit
    STR    r0, [r8, #PCITSect_Limit]
    MOV    r1, #1
    STRB   r1, [r8, #PCITSect_Flags]  ; loaded = 1
```

The segment's entry vector is at the end of the segment; it must be copied to the PCIT section identified by R8, and zeroed in case it is in use as zero-initialised data:

```asm
    LDR    r1, [r8, #PCITSect_Vecsize]
    ADD    r0, r0, r1                  ; end of loaded segment...
    SUB    r3, r8, #8                  ; end of entry vector...
    MOV    r4, #0                      ; for data initialisation
01  LDR    r2, [r0, #-4]!             ; > loop to copy
    STR    r4, [r0]                    ; (zero-init possible data section)
    STR    r2, [r3], #-4              ; > the segment's PCIT
    SUBS   r1, r1, #4                  ; > section into the
    BGT    %B01                        ; > global PCIT...
```

Finally, continue execution:

```asm
    LDMIA  R9, {R0-R9, LR, PC}^
```

## Intercepting Returns to Overwritten Segments

The overlay scheme as described so far is sufficient, provided no function call causes any overlay in the current call chain to be unloaded. As a specific example, consider a root segment and two procedures, A and B in overlays `1_1` and `1_2` respectively. Note that A and B may not be co-resident. Then any pattern of calls like:

```
((root calls A, A returns)* (root calls B, B returns)*)*
```

is unproblematic. However, "A calls B" is disastrous when B tries to return (as B will return to a random address within itself rather than to A).

To fix this deficiency, it is necessary to intercept (some) function returns. Trying to intercept all returns would be hopelessly expensive; at the point of call there are no working registers available, and there is nowhere to store a return address (the stack cannot be used without potentially destroying the current function call's arguments).

The following observations hold the key to an efficient implementation:

- A return address can only be invalidated by loading a segment which displaces a currently loaded segment.
- At the point that a segment is loaded, the stack contains a complete record of return addresses which might be invalidated by the load.

Before loading a segment, the procedure call back-trace (including the value stored in LRSave) must be checked for return addresses which fall in the segment about to be overwritten. Each such return address must be replaced by a pointer to a return handler which will load the segment before continuing the return.

Unfortunately, there is no simple way to avoid using a fixed pool of return handlers. The stack cannot be used (in a language-independent manner) because its layout is only partly defined in mid function call. A variant of the language-specific stack-extension code could be used, but it would complicate the implementation significantly, and make some aspects of the overlay mechanism language specific. Similarly, it would be unwise to make any assumptions about the availability or management of heap space.

Fortunately, using a fixed pool of handlers is not as bad as it first seems. A handler can only be needed if a call is made which overwrites the calling segment. If this is done strictly non-recursively (meaning that if any P in segment 1 calls some Q in segment 2, then no R in segment 2 may call any S in segment 1 until Q has returned), then the number of handlers required is bounded by the number of overlay segments. If recursive calls are made between overlay segments, then performance will be exceedingly poor unless a large amount of work is done by each call. It is hard to envisage an application which would require an unbounded depth of recursion, and would perform significant amounts of work at each level (a recursively invokable CLI is such an example, but in this case it's hard to see why a moderate fixed limit on the depth of recursion would be unacceptable).

Note that only the most recent return should be allocated a return handler. For example, assume that there is a sequence of mutually recursive calls between segments A and B, followed by a call to C which unloads A. Then, only the latest return to A needs to be trapped, because as soon as A has been re-loaded the remainder of the mutually-recursive returns can unwind without being intercepted.

## Return Handler Code

A return handler must store the real return address, the identity of the segment to return to (e.g. the address of its PCIT section), and it must contain a call (indirectly) to the `load_segment` code. In addition, it is assumed that the handler pool is managed as a singly linked list. Then the handler code is:

```asm
        BL     load_seg_and_ret
RealLR  DCD    0                ; space for the real return address
Segment DCD    0                ; -> PCIT section of segment to load
Link    DCD    0                ; -> next in stack order
```

`RealLR`, `Segment` and `Link` are set up by `check_for_invalidated_returns`.

## The load_seg_and_ret Code

*HStack* and *HFree* are set up by `overlay_mgr_init`, and maintained by `check_for_invalidated_returns`. For simplicity, they are shown here as PC-relative-addressable variables. More properly, they are part of the data area shared with *load_seg_and_go*. As already noted, this data area can be addressed via an address constant, as *ip* is available as a base register.

```asm
HStack  DCD    0                        ; top of stack of allocated handlers
HFree   DCD    0                        ; head of free-list

load_seg_and_ret
    STR    R9, RSave+9*4               ; save a base register...
    ADR    R9, RSave
    STMIA  R9, {R0-R8}                 ; ...and some working registers
    BIC    R8, LR, #0xFC000003         ; clear out status bits (26-bit mode)
    LDMIA  R8, {R0, R1, R2}           ; RealLR, Segment, Link
    STR    R0, LRSave
    STR    R0, PCSave
; Now unchain the handler and return it to the free pool
; (by hypothesis, HStack points to this handler...)
    STR    R2, HStack                  ; new top of handler stack
    LDR    R2, HFree
    STR    R2, [R8, #8]               ; Link -> old HFree
    SUB    R2, R8, #4
    STR    R2, HFree                   ; new free list
    MOV    R8, R1                      ; segment to load
    B      load_segment
```

## The check_for_invalidated_returns Code

This code must check LRSave and the chain of call-frames for the first return address invalidated, by loading the segment identified by R8 into the slot identified by R7. R7-R9, FP, SP and SL must be preserved.

```asm
    ADR    R6, LRSave                  ; 1st location to check
    MOV    R0, FP                      ; temporary FP...
01  LDR    R1, [R6]                    ; the saved return address...
    BIC    R1, R1, #0xFC000003         ; ...with status bits masked off
    LDR    R2, [R7, #O_Base]
    CMPS   R1, R2                      ; see if >= base...
    BLT    %F02
    LDR    R2, [R7, #O_Limit]
    CMPS   R1, R2                      ; ...and < limit
    BLT    FoundClash
02  CMPS   R0, #0                      ; bottom of stack?
    MOVEQS PC, LR                      ; yes => return
    SUB    R6, R0, #4
    LDR    R0, [R0, #-12]             ; previous FP
    B      %B01
```

Having found a segment containing a return address invalidated by this segment load, a handler is allocated for it:

```asm
FoundClash
    LDR    R0, HFree                   ; head of chain of free handlers
    CMPS   R0, #0
    BEQ    NoHandlersLeft
                                        ; Transfer the next free handler to
                                        ; head of the handler stack.
    LDR    R1, [R0, #12]              ; next free handler
    STR    R1, HFree
    LDR    R1, HStack                  ; the active handler stack
    STR    R1, [R0, #12]
    STR    R0, HStack                  ; now with the latest handler
                                        ; linked in. Initialise the handler
                                        ; with a BL load_seg_and_ret,
                                        ; RealLR and Segment.
    ADR    R1, load_seg_and_ret
    SUB    R1, R1, R0                  ; byte offset for BL in handler
    SUB    R1, R1, #8                  ; correct for PC off by 8
    MOV    R1, R1, ASR #2             ; word offset
    BIC    R1, #0xFF000000
    ORR    R1, #0xEB000000            ; code for BL
    STR    R1, [R0]
    LDR    R1, [R6]
    STR    R6, [R0, #4]               ; RealLR
    STR    R0, [R6]                    ; patch stack to return to handler
    STR    R7, [R0, #8]               ; segment to re-load on return
    MOVS   PC, LR                      ; and return
NoHandlersLeft
    ...                                 ; omitted for brevity
```

The initial creation of the handler pool is omitted for brevity.
