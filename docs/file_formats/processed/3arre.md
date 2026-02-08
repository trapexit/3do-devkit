# The Handling of Relocation Directives

The linker implements the relocation directives defined by ARM Object Format. This section describes their function, omitting the fine details given in the ARM Object Format technical specification.

## The Subject Field

A relocation directive describes the relocation of a single *subject field*, the value of which may be:

- A byte
- A half-word (2 bytes)
- A word (4 bytes)
- A value derived from a suitable sequence of instructions

The relocation of a word value cannot overflow. In the other three cases, overflow is detected and faulted by the linker. The relocation of sequences of instructions is discussed later in this section.

## The Relocation Value

A relocation directive either refers to the value of a symbol, or to the base address of an AOF area in the same object file as the AOF area containing the directive. This value is called the *relocation value*, and the subject field is modified by it, as described in the following subsections.

## PC-Relative Relocation

A PC-relative relocation directive requests the following modification of the subject field:

```
subject-field = subject-field + relocation-value
                - base-of-area-containing(subject-field)
```

A special case of PC-relative relocation occurs when the relocation value is specified to be the base of the area containing the subject field. In this case, the relocation value is not added and:

```
subject-field = subject-field - base-of-area-containing(subject-field)
```

which caters for a PC-relative branch to a fixed location, for example.

## Forcing Use of an Inter-Link-Unit Entry Point

A second special case of PC-relative relocation (specified by `REL_B` being set in the `rel_flags` field -- see the ARM Object Format specification for details) applies when the relocation value is the value of a code symbol. It requests that the *inter*-link-unit value of the symbol be used, rather than the *intra*-link-unit value. Unless the symbol is marked with the `SYM_LEAFAT` attribute (by a compiler or via the assembler's `EXPORT` directive), the *inter*-link-unit value will be 4 bytes beyond the *intra*-link-unit value.

This directive allows the tail-call optimisation to be performed on reentrant code. For more information about tail call continuation see the Function Entry documentation.

## Additive Relocation

A plain additive relocation directive requests that the subject field be modified as follows:

```
subject-field = subject-field + relocation-value
```

## Based Area Relocation

A based area relocation directive relocates a subject field by the offset of the relocation value within the consolidated area containing it:

```
subject-field = subject-field + relocation-value
                - base-of-area-group-containing(relocation-value)
```

For example, when compiling reentrant code, the C compiler places address constants in an *adcon* area called `sb$$adcons` based on register *sb*, and generates code to load them using *sb*-relative LDRs. At link time, separate adcon areas are merged, so *sb* no longer points where presumed at compile time (except for the first area in the consolidated group). The offset field of each LDR (other than those in the first area) must be modified by the offset of the base of the appropriate adcon area in the consolidated group:

```
LDR-offset = LDR-offset + base-of-my-sb$$adcons-area
             - sb$$adcons$$Base
```

## The Relocation of Instruction Sequences

The linker recognises the following instruction sequences as defining a relocatable value:

- A `B` or `BL`
- An `LDR` or `STR`
- 1 to 3 `ADD` or `SUB` instructions, having a common destination register and a common intermediate source register, and optionally followed by an `LDR` or `STR` with that register as base

For example, the following is a relocatable instruction sequence:

```asm
ADD    Rb, rx, #V1
ADD    Rb, Rb, #V2
LDR    ry, [Rb, #V3]
```

with value `V = V1 + V2 + V3`.

The length of sequence recognised may be further restricted to 1, 2 or 3 instructions only by the relocation directive itself (see the Relocation Directives specification).

After relocation, the new value of V is split between the instructions as follows:

- If the original offset in the `LDR` or `STR` can be preserved, it will be preserved. This is possible if the difference between the new value and the original LDR offset can be encoded in the available number of `ADD`/`SUB` instructions. This preservation allows a sequence of ADDs and SUBs to compute a common base address for several following LDRs or STRs.

The remainder of the new value is split between the ADDs or SUBs as follows:

- If the new value is negative, it is negated, ADDs are changed to SUBs (or vice versa) and LDR/STR *up* is changed to LDR/STR *down* (or vice versa).
- Each `ADD` or `SUB` instruction, in turn, removes the most significant part of the (now positive) new value, which can be represented by an 8-bit constant, shifted left by an even number of bit positions (i.e. which can be represented by an ARM data-processing instruction's immediate value).

If there is no following `LDR` or `STR`, and the value remaining is non-zero, then the relocation has overflowed.

If there is a following `LDR` or `STR`, then any value remaining is assigned to it as an immediate offset. If this value is greater than 4095, then the relocation has overflowed.

In the relocation of a `B` or `BL` instruction, word offsets are converted to and from byte offsets. A `B` or `BL` is always relocated by itself, never in conjunction with any other instruction.
