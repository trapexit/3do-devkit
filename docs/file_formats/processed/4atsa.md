# The ARM Procedure Call Standard

This section defines the ARM Procedure Call Standard. Explanatory text, not itself part of the standard, is bracketed by "(Aside:" and ")".

(Aside: This explanation may help you to understand the APCS but is not, itself, part of the APCS. If an explanation appears to conflict with the standard then the standard should be considered definitive and the narrative merely an indication of intent).

A program fragment which conforms to the APCS while making a call to an external function (one which is visible between compilation units) is said to be *conforming*. A program which conforms to the APCS at all instants of execution is said to be *strictly conforming* or to *conform strictly*.

(Aside: In general, compiled code is expected to be strictly conforming; hand-written code merely conforming).

Whether or not, and when program fragments for a particular ARM-based environment are required to conform strictly to the APCS is part of the definition of that environment.

In the following sections, clauses following *shall* and *shall not* are obligations which must be met in order to conform to the APCS.

## Register Names

The ARM has 15 visible general registers, a program counter register and 8 floating-point registers.

(Aside: In non-user machine modes, some general registers are shadowed. In all modes, the availability of the floating-point instruction set depends on the processor model, hardware and operating system).

## General Registers

| Name  | Number | APCS Role                                            |
|-------|--------|------------------------------------------------------|
| a1    | 0      | argument1 / integer result / scratch register        |
| a2    | 1      | argument 2 / scratch register                        |
| a3    | 2      | argument 3 / scratch register                        |
| a4    | 3      | argument 4 / scratch register                        |
|       |        |                                                      |
| v1    | 4      | register variable                                    |
| v2    | 5      | register variable                                    |
| v3    | 6      | register variable                                    |
| v4    | 7      | register variable                                    |
| v5    | 8      | register variable                                    |
|       |        |                                                      |
| sb/v6 | 9      | static base / register variable                      |
| sl/v7 | 10     | stack limit / stack chunk handle / reg. variable     |
| fp    | 11     | frame pointer                                        |
| ip    | 12     | scratch register / new-sb in inter-link-unit calls   |
| sp    | 13     | lower end of current stack frame                     |
| lr    | 14     | link address / scratch register                      |
| pc    | 15     | program counter                                      |

(Aside: The 16 integer registers are divided into 3 sets:

- 4 argument registers which can also be used as scratch registers or as caller-saved register variables
- 5 callee-saved registers, conventionally used as register variables
- 7 registers which have a dedicated role, at least some of the time, in at least one variant of APCS-3 (see [APCS Variants](4atsb.md))

The 5 frame registers fp, ip, sp, lr and pc have dedicated roles in all variants of the APCS.

The ip register has a dedicated role only during function call; at other times it may be used as a scratch register.

(Aside: Conventionally, ip is used by compiler code generators as the/a local code generator temporary register).

There are dedicated roles for sb and sl in some variants of the APCS; in other variants they may be used as callee-saved registers.

The APCS permits lr to be used as a register variable when not in use during a function call. It further permits an ARM system specification to forbid such use in some, or all, non-user ARM processor modes.

## Floating Point Registers

(Aside: Each ARM floating-point (FP) register holds one FP value of single, double, extended or internal precision. A single-precision value occupies 1 machine word; a double-precision value 2 words; an extended precision value occupies 3 words, as does an internal precision value).

| Name | Number | APCS Role                                              |
|------|--------|--------------------------------------------------------|
| f0   | 0      | FP argument 1 / FP result / FP scratch register       |
| f1   | 1      | FP argument 2 / FP scratch register                   |
| f2   | 2      | FP argument 3 / FP scratch register                   |
| f3   | 3      | FP argument 4 / FP scratch register                   |
|      |        |                                                        |
| f4   | 4      | floating point register variable                       |
| f5   | 5      | floating point register variable                       |
| f6   | 6      | floating point register variable                       |
| f7   | 7      | floating point register variable                       |

(Aside: The floating-point (FP) registers are divided into two sets, analogous to the subsets a1-a4 and v1-v5/v7 of the general registers:

- Registers f0-f3 need not be preserved by called functions; f0 is the FP result register and f0-f3 may hold the first four FP arguments (see [Data Representation and Argument Passing](#data-representation-and-argument-passing) and [APCS Variants](4atsb.md))
- Registers f4-f7, the so called 'variable' registers, preserved by callees.)

## The Stack

The stack is a singly-linked list of *activation records*, linked through a *stack backtrace data structure* (see below), stored at the high-address end of each activation record.

The stack shall be readable and writable by the executing program.

Each contiguous chunk of the stack shall be allocated to activation records in descending address order. At all instants of execution, sp shall point to the lowest used address of the most recently allocated activation record.

There may be multiple stack chunks, and there are no constraints on the ordering of these chunks in the address space.

Associated with sp is a possibly-implicit stack chunk limit, below which sp shall not be decremented. (See [APCS Variants](4atsb.md)).

At all instants of execution, the memory between sp and the stack chunk limit shall contain nothing of value to the executing program: it may be modified unpredictably by the execution environment.

The stack chunk limit is said to be implicit if chunk overflow is detected and handled by the execution environment. Otherwise it is explicit.

If the stack chunk limit is implicit, sl may be used as v7, an additional callee-saved variable register.

If the conditions of the remainder of this subsection hold at all instants of execution, then the program conforms strictly to the APCS; otherwise, if they hold at and during external (inter-compilation-unit-visible) function calls, the program merely conforms to the APCS.

If the stack chunk limit is explicit, then:

- sl shall point at least 256 bytes above it
- sl shall identify the current stack chunk in a system-defined manner
- At all times, sl shall identify the same chunk as sp points into

(Aside: sl >= stack chunk limit + 256 allows the most common limit checks to be made very cheaply during function entry).

(Aside: This final requirement implies that on changing stack chunks, sl and sp must be loaded simultaneously by means of an:

```arm
LDM ..., {..., sl, sp}.
```

In general, this means that return from a function executing on an extension chunk, to one executing on an earlier-allocated chunk, should be via an intermediate function invocation, specially fabricated when the stack was extended).

The values of sl, fp and sp shall be multiples of 4.

## The Stack Backtrace Data Structure

The value in fp shall be zero or shall point to a list of stack backtrace data structures which partially describe the sequence of outstanding function calls.

(Aside: If this constraint holds when external functions are called, the program is conforming; if it holds at all instants of execution, the program is strictly conforming).

The stack backtrace data structure has the format shown below:

```
save code pointer                [fp]        <-fp points to here
return link value                [fp, #-4]
return sp value                  [fp, #-8]
return fp value                  [fp, #-12]
[saved v7 value]
[saved v6 value]
[saved v5 value]
[saved v4 value]
[saved v3 value]
[saved v2 value]
[saved v1 value]
[saved a4 value]
[saved a3 value]
[saved a2 value]
[saved a1 value]
[saved f7 value]                three words
[saved f6 value]                three words
[saved f5 value]                three words
[saved f4 value]                three words
```

The above picture shows between four and twenty-seven words, with those words higher on the page being at higher addresses in memory. The values shown in brackets are optional, and their presence need not imply the presence of any other. The floating point values are stored in an internal format, and occupy three words each.

## Function Invocations and Backtrace Structures

If function invocation A calls function B, then A is called a *direct ancestor* of the invocation of B. If invocation A[1] calls invocation A[2] calls... calls B, then each of the A[i] is an ancestor of B and invocation A[i] is *more recent* than invocation A[j] if i > j.

The *return fp value* shall be 0, or shall be a pointer to a stack backtrace data structure created by an ancestor of the function invocation which created the backtrace structure pointed to by fp. No more recent ancestor shall have created a backtrace structure.

(Aside: There may be any number of tail-called invocations between invocations which create backtrace structures).

The *return link value*, *return sp value* and *return fp value* are, respectively, the values to restore to pc, sp and fp at function exit.

In the 32-bit PC variant of the APCS, the *save code pointer* shall point twelve bytes beyond the start of the sequence of instructions that created the stack backtrace data structure.

In the 26-bit PC variant of the APCS, the *save code pointer*, when cleared of PSR and mode bits, shall point twelve bytes beyond the start of the sequence of instructions that created the stack backtrace data structure.

## Control Arrival

At the instant when control arrives at the target function:

- pc contains the address of an entry point to the target function

(Aside: reentrant functions may have two entry points).

- lr shall contain the value to restore to pc on exit from the function (the *return link value* - see [The Stack Backtrace Data Structure](#the-stack-backtrace-data-structure))

(Aside: In 26-bit variants of the APCS, lr contains the PC + PSR value to restore to pc on exit from the function. See [APCS Variants](4atsb.md)).

- sp shall point at or above the current stack chunk limit; if the limit is explicit, it shall point at least 256 bytes above it (see [The Stack](#the-stack))
- fp shall contain 0 or shall point to the most recently created stack backtrace structure (see [The Stack Backtrace Data Structure](#the-stack-backtrace-data-structure))
- The space between sp and the stack chunk limit shall be readable, writable memory which can be used by the called function as temporary workspace, and overwritten with any values before the function returns (see [The Stack](#the-stack))
- Arguments shall have been marshalled as described below

If the target function is reentrant (see [APCS Variants](4atsb.md)) then it has two entry points and control arrives:

- At the *intra-link-unit entry point* if the caller has been directly linked with the callee
- At the *inter-link-unit entry point* if the caller has been separately linked with a *stub* of the callee

(Aside: Sometimes the two entry points are at the same address; usually they will be separated by a single instruction).

On arrival at the intra-link-unit entry point, sb shall identify the static data of the link unit which contains both the caller and the callee.

On arrival at the inter-link-unit entry point, ip shall identify the static data of the link unit containing the target function, or the target function shall make neither direct nor indirect use of static data.

(Aside: In practice this usually means the callee must be a leaf function making no direct use of static data).

(Aside: The way in which sb *identifies* the static data of a link unit is not specified by the APCS.)

(Aside: If the call is by tail continuation, *calling function* means that which would be returned to, were the tail continuation converted to a return).

(Aside: If code is not required to be reentrant or sharable then sb may be used as v6, an additional variable register).

## Data Representation and Argument Passing

Argument passing in the APCS is defined in terms of an ordered list of machine-level values passed from the caller to the callee, and a single word or floating point result passed back from the callee to the caller. Each value in the argument list shall be:

- A word-sized, integer value
- A floating point value (of size 1, 2 or 3 words)

A callee may corrupt any of its arguments, howsoever passed.

(Aside: The APCS does not define the layout in store of records, arrays and so forth, used by ARM-targeted compilers for C, Pascal, Fortran-77, etc.; nor does it prescribe the order in which language-level arguments are mapped into their machine-level representations. In other words, the mapping from language-level data types, and arguments to APCS words is defined by each language implementation, not by the APCS. Indeed, there is no formal reason why two ARM-targeted implementations of the same language should not use different mappings and, hence, not support cross-calling. Obviously, it would be very unhelpful to stand by this formal position so implementors are encouraged to adopt not just the letter of the APCS but also the natural mappings of source language objects into argument words. Guidance about this is given in [C Language Calling Conventions](4atsc.md)).

At the instant control arrives at the target function, the argument list shall be allocated as follows:

- In APCS variants which support the passing of floating-point arguments in floating-point registers (see [APCS Variants](4atsb.md)), the first 4 floating-point arguments (or fewer if the number of floating-point arguments is less than 4) shall be in machine registers f0-f3
- The first 4 remaining argument words (or fewer if there are fewer than 4 argument words remaining in the argument list) shall be in machine registers a1-a4
- The remainder of the argument list (if any) shall be in memory, at the location addressed by sp and higher-addressed words thereafter

A floating-point value not passed in a floating-point register is treated as 1, 2 or 3 integer values, as appropriate to its precision.

## Control Return

When the return link value for a function call is placed in the pc:

- sp, fp, sl/v7, sb/v6, v1-v5, and f4-f7 shall contain the same values as they did at the instant of control arrival
- If the function returns a simple value of size one word or less, then that value shall be in a1

(Aside: a language implementation is not obliged to consider *all* single-word values simple. See [C Language Calling Conventions](4atsc.md)).

- If the function returns a simple floating point value then that value shall be in f0

(Aside: The values of ip, lr, a2-a4, f1-f3 and any stacked arguments are undefined).

(Aside: The definition of control return means that this is a *callee saves* standard).

(Aside: In 32-bit ARM modes, the caller's PSR flags are not preserved across a function call. In 26-bit ARM modes, the caller's PSR flags are naturally reinstated when the return link pointer is placed in pc. Note that the N, Z, C and V flags from lr at the instant of entry must be reinstated; it is not sufficient merely to preserve the PSR across the call. Consider, a function `ProcA` which tail continues to `ProcB` as follows:

```arm
    CMPS   a1, #0
    MOVLT  a2, #255
    MOVGE  a2, #0
    B      ProcB
```

If `ProcB` merely preserves the flags it sees on entry, rather than restoring those from lr, the wrong flags may be set when `ProcB` returns direct to `ProcA`'s caller. See [The ARM Procedure Call Standard](#the-arm-procedure-call-standard)).
