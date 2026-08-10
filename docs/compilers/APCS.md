# ARM Procedure Call Standard (APCS)

Source: [ARM Procedure Call Standard (APCS.txt)](https://www.chiark.greenend.org.uk/~theom/riscos/docs/CodeStds/APCS.txt)

## Introduction

The ARM Procedure Call Standard (APCS) is a set of rules which
regulate and facilitate calls between separately compiled or assembled
program fragments.

The APCS defines:

 *  constraints on the use of registers;

 *  stack conventions;

 *  the format of a stack-based data structure, used by stack tracing
    programs to reconstruct a sequence of outstanding calls;

 *  the passing of machine-level arguments, and the return of
    machine-level results at externally visible function/procedure
    calls;

 *  support for the ARM shared library mechanism; a standard way for
    shared (reentrant) code to address the static data of its clients,
    (See "ARM Shared Library Format" of the Reference Manual for
    details).

Since the ARM CPU is used in a wide variety of systems, the APCS is
not a single standard, but a consistent family of standards. See
section "APCS Variants" for details of the variants in the family.
Implementors of run-time systems, operating systems, embedded control
monitors, etc., must choose the variant(s) most appropriate to their
requirements.

Naturally, there can be no binary compatibility between program
fragments which conform to different members of the APCS family. Those
concerned with long-term binary compatibility must choose their
options carefully.

`function` is used to mean function, procedure or subroutine.


### Design Criteria

Throughout its history, the APCS has compromised between `fastest`,
`smallest` and `easiest to use`.

The criteria we have considered to be important are:

 *  Function call should be fast and it should be easy for compilers
    to optimise function entry sequences.

 *  The function call sequence should be as compact as possible.

 *  Extensible stacks and multiple stacks should be accommodated.

 *  The standard should encourage the production of reentrant code,
    with writable data separated from code.

 *  The standard should be simple enough to be used by assembly
    language programmers, and should support simple approaches to link
    editing, debugging and run-time error diagnosis.

Overall, we have tended to rank compact code and a clear definition
most highly, with simplicity and ease of use ahead of performance in
matters of fine detail where the impact on performance is small.


## The ARM Procedure Call Standard

This section defines the ARM Procedure Call Standard. Explanatory
text, not itself part of the standard, is bracketed by "(Aside:" and
")".

(Aside: This explanation may help you to understand the APCS but is
not, itself, part of the APCS. If an explanation appears to conflict
with the standard then the standard should be considered definitive
and the narrative merely an indication of intent).

A program fragment which conforms to the APCS while making a call to
an external function (one which is visible between compilation units)
is said to be `conforming`. A program which conforms to the APCS at
all instants of execution is said to be `strictly conforming` or to
`conform strictly`.

(Aside: In general, compiled code is expected to be strictly
conforming; hand-written code merely conforming).

Whether or not, and when program fragments for a particular ARM-based
environment are required to conform strictly to the APCS is part of
the definition of that environment.

In the following sections, clauses following `shall` and `shall not`
are obligations which must be met in order to conform to the APCS.


### Register Names

The ARM has 15 visible general registers, a program counter register
and 8 floating-point registers.

(Aside: In non-user machine modes, some general registers are
shadowed. In all modes, the availability of the floating-point
instruction set depends on the processor model, hardware and operating
system).


### General Registers

```
    Name    Number      APCS Role

    a1      0           argument 1 / integer result / scratch register
    a2      1           argument 2 / scratch register
    a3      2           argument 3 / scratch register
    a4      3           argument 4 / scratch register

    v1      4           register variable
    v2      5           register variable
    v3      6           register variable
    v4      7           register variable
    v5      8           register variable

    sb/v6   9           static base / register variable
    sl/v7   10          stack limit / stack chunk handle / reg. variable
    fp      11          frame pointer
    ip      12          scratch register / new-sb in inter-link-unit calls
    sp      13          lower end of current stack frame
    lr      14          link address / scratch register
    pc      15          program counter
```

(Aside: The 16 integer registers are divided into 3 sets:

 *  4 argument registers which can also be used as scratch registers
    or as caller-saved register variables;

 *  5 callee-saved registers, conventionally used as register
    variables;

 *  7 registers which have a dedicated role, at least some of the
    time, in at least one variant of APCS-3 (see "APCS Variants").

The 5 frame registers fp, ip, sp, lr and pc have dedicated roles in
all variants of the APCS.

The ip register has a dedicated role only during function call; at
other times it may be used as a scratch register.

(Aside: Conventionally, ip is used by compiler code generators as
the/a local code generator temporary register).

There are dedicated roles for sb and sl in some variants of the APCS;
in other variants they may be used as callee-saved registers.

The APCS permits lr to be used as a register variable when not in use
during a function call. It further permits an ARM system specification
to forbid such use in some, or all, non-user ARM processor modes.


### Floating Point Registers

(Aside: Each ARM floating-point (FP) register holds one FP value of
single, double, extended or internal precision. A single-precision
value occupies 1 machine word; a double-precision value 2 words; an
extended precision value occupies 3 words, as does an internal
precision value).

```
    Name    Number      APCS Role

    f0      0           FP argument 1 / FP result / FP scratch register
    f1      1           FP argument 2 / FP scratch register
    f2      2           FP argument 3 / FP scratch register
    f3      3           FP argument 4 / FP scratch register

    f4      4           floating point register variable
    f5      5           floating point register variable
    f6      6           floating point register variable
    f7      7           floating point register variable
```

(Aside: The floating-point (FP) registers are divided into two sets,
analogous to the subsets a1-a4 and v1-v5/v7 of the general registers:

 *  registers f0-f3 need not be preserved by called functions; f0 is
    the FP result register and f0-f3 may hold the first four FP
    arguments (see "Data Representation and Argument Passing" and
    "APCS Variants");

 *  registers f4-f7, the so called 'variable' registers, preserved by
    callees.)


### The Stack

The stack is a singly-linked list of `activation records`, linked
through a `stack backtrace data structure` (see below), stored at the
high-address end of each activation record.

The stack shall be readable and writable by the executing program.

Each contiguous chunk of the stack shall be allocated to activation
records in descending address order. At all instants of execution, sp
shall point to the lowest used address of the most recently allocated
activation record.

There may be multiple stack chunks, and there are no constraints on
the ordering of these chunks in the address space.

Associated with sp is a possibly-implicit stack chunk limit, below
which sp shall not be decremented. (See "APCS Variants").

At all instants of execution, the memory between sp and the stack
chunk limit shall contain nothing of value to the executing program:
it may be modified unpredictably by the execution environment.

The stack chunk limit is said to be implicit if chunk overflow is
detected and handled by the execution environment. Otherwise it is
explicit.

If the stack chunk limit is implicit, sl may be used as v7, an
additional callee-saved variable register.

If the conditions of the remainder of this subsection hold at all
instants of execution, then the program conforms strictly to the APCS;
otherwise, if they hold at and during external
(inter-compilation-unit-visible) function calls, the program merely
conforms to the APCS.

If the stack chunk limit is explicit, then:

 *  sl shall point at least 256 bytes above it;

 *  sl shall identify the current stack chunk in a system-defined
    manner;

 *  at all times, sl shall identify the same chunk as sp points into.

(Aside: sl >= stack chunk limit + 256 allows the most common limit
checks to be made very cheaply during function entry).

(Aside: This final requirement implies that on changing stack chunks,
sl and sp must be loaded simultaneously by means of an:

    LDM ..., {..., sl, sp}.

In general, this means that return from a function executing on an
extension chunk, to one executing on an earlier-allocated chunk,
should be via an intermediate function invocation, specially
fabricated when the stack was extended).

The values of sl, fp and sp shall be multiples of 4.


### The Stack Backtrace Data Structure

The value in fp shall be zero or shall point to a list of stack
backtrace data structures which partially describe the sequence of
outstanding function calls.

(Aside: If this constraint holds when external functions are called,
the program is conforming; if it holds at all instants of execution,
the program is strictly conforming).

The stack backtrace data structure has the format shown below:

```
    save code pointer       [fp]        <-fp points to here
    return link value       [fp, #-4]
    return sp value         [fp, #-8]
    return fp value         [fp, #-12]
    [saved v7 value]
    [saved v6 value]
    [saved v5 value}
    [saved v4 value]
    [saved v3 value]
    [saved v2 value]
    [saved v1 value]
    [saved a4 value]
    [saved a3 value}
    [saved a2 value]
    [saved a1 value]
    [saved f7 value]        three words
    [saved f6 value]        three words
    [saved f5 value]        three words
    [saved f4 value]        three words
```

The above picture shows between four and twenty-seven words, with
those words higher on the page being at higher addresses in
memory. The values shown in brackets are optional, and their presence
need not imply the presence of any other. The floating point values
are stored in an internal format, and occupy three words each.


### Function Invocations and Backtrace Structures

If function invocation A calls function B, then A is called a `direct
ancestor` of the invocation of B. If invocation A[1] calls invocation
A[2] calls... calls B, then each of the A[i] is an ancestor of B and
invocation A[i] is `more recent` than invocation A[j] if i > j.

The `return fp value` shall be 0, or shall be a pointer to a stack
backtrace data structure created by an ancestor of the function
invocation which created the backtrace structure pointed to by fp. No
more recent ancestor shall have created a backtrace structure.

(Aside: There may be any number of tail-called invocations between
invocations which create backtrace structures).

The `return link value`, `return sp value` and `return fp value` are,
respectively, the values to restore to pc, sp and fp at function exit.

In the 32-bit PC variant of the APCS, the `save code pointer` shall
point twelve bytes beyond the start of the sequence of instructions
that created the stack backtrace data structure.

In the 26-bit PC variant of the APCS, the `save code pointer`, when
cleared of PSR and mode bits, shall point twelve bytes beyond the
start of the sequence of instructions that created the stack backtrace
data structure.


### Control Arrival

At the instant when control arrives at the target function:

 *  pc contains the address of an entry point to the target function;

    (Aside: reentrant functions may have two entry points).

 *  lr shall contain the value to restore to pc on exit from the
    function (the `return link value` - see "The Stack Backtrace Data
    Structure" starting on page42);

    (Aside: In 26-bit variants of the APCS, lr contains the PC + PSR
    value to restore to pc on exit from the function. See "APCS
    Variants" starting on page45).

 *  sp shall point at or above the current stack chunk limit; if the
    limit is explicit, it shall point at least 256 bytes above it (see
    "The Stack" );

 *  fp shall contain 0 or shall point to the most recently created
    stack backtrace structure (see "The Stack Backtrace Data
    Structure" starting on page42);

 *  the space between sp and the stack chunk limit shall be readable,
    writable memory which can be used by the called function as
    temporary workspace, and overwritten with any values before the
    function returns (see "The Stack" );

 *  arguments shall have been marshalled as described below.

If the target function is reentrant (see "APCS Variants") then it
has two entry points and control arrives:

 *  at the `intra-link-unit entry point` if the caller has been
    directly linked with the callee;

 *  at the `inter-link-unit entry point` if the caller has been
    separately linked with a `stub` of the callee.

(Aside: Sometimes the two entry points are at the same address;
usually they will be separated by a single instruction).

On arrival at the intra-link-unit entry point, sb shall identify the
static data of the link unit which contains both the caller and the
callee.

On arrival at the inter-link-unit entry point, ip shall identify the
static data of the link unit containing the target function, or the
target function shall make neither direct nor indirect use of static
data.

(Aside: In practice this usually means the callee must be a leaf
function making no direct use of static data).

(Aside: The way in which sb `identifies` the static data of a link
unit is not specified by the APCS. See "ARM Shared Library Format"
of the Reference Manual for details of support for reentrant code and
shared libraries).

(Aside: If the call is by tail continuation, `calling function` means
that which would be returned to, were the tail continuation converted
to a return).

(Aside: If code is not required to be reentrant or sharable then sb
may be used as v6, an additional variable register).


### Data Representation and Argument Passing

Argument passing in the APCS is defined in terms of an ordered list of
machine-level values passed from the caller to the callee, and a
single word or floating point result passed back from the callee to
the caller. Each value in the argument list shall be:

 *  a word-sized, integer value;

 *  a floating point value (of size 1, 2 or 3 words).

A callee may corrupt any of its arguments, howsoever passed.

(Aside: The APCS does not define the layout in store of records,
arrays and so forth, used by ARM-targeted compilers for C, Pascal,
Fortran-77, etc.; nor does it prescribe the order in which
language-level arguments are mapped into their machine-level
representations. In other words, the mapping from language-level data
types, and arguments to APCS words is defined by each language
implementation, not by the APCS. Indeed, there is no formal reason why
two ARM-targeted implementations of the same language should not use
different mappings and, hence, not support cross-calling. Obviously,
it would be very unhelpful to stand by this formal position so
implementors are encouraged to adopt not just the letter of the APCS
but also the natural mappings of source language objects into argument
words. Guidance about this is given in "C Language Calling
Conventions").

At the instant control arrives at the target function, the argument
list shall be allocated as follows:

 *  in APCS variants which support the passing of floating-point
    arguments in floating-point registers (see "APCS Variants"), the
    first 4 floating-point arguments (or fewer if the number of
    floating-point arguments is less than 4) shall be in machine
    registers f0-f3;

 *  the first 4 remaining argument words (or fewer if there are fewer
    than 4 argument words remaining in the argument list) shall be in
    machine registers a1-a4;

 *  the remainder of the argument list (if any) shall be in memory, at
    the location addressed by sp and higher-addressed words
    thereafter.

A floating-point value not passed in a floating-point register is
treated as 1, 2 or 3 integer values, as appropriate to its precision.


### Control Return

When the return link value for a function call is placed in the pc:

 *  sp, fp, sl/v7, sb/v6, v1-v5, and f4-f7 shall contain the same
    values as they did at the instant of control arrival;

 *  if the function returns a simple value of size one word or less,
    then that value shall be in a1;

    (Aside: a language implementation is not obliged to consider `all`
    single-word values simple. See "C Language Calling Conventions"
    starting on page47).

 *  if the function returns a simple floating point value then that
    value shall be in f0.

(Aside: The values of ip, lr, a2-a4, f1-f3 and any stacked arguments
are undefined).

(Aside: The definition of control return means that this is a `callee
saves` standard).

(Aside: In 32-bit ARM modes, the caller's PSR flags are not preserved
across a function call. In 26-bit ARM modes, the caller's PSR flags
are naturally reinstated when the return link pointer is placed in
pc. Note that the N, Z, C and V flags from lr at the instant of entry
must be reinstated; it is not sufficient merely to preserve the PSR
across the call. Consider, a function ProcA which tail continues to
ProcB as follows:

```
        CMPS   a1, #0
        MOVLT  a2, #255
        MOVGE  a2, #0
        B      ProcB
```

If ProcB merely preserves the flags it sees on entry, rather than
restoring those from lr, the wrong flags may be set when ProcB returns
direct to ProcA's caller. See "APCS Variants").


## APCS Variants

There are, currently, 2 x 2 x 2 x 2 = 16 APCS variants, derived from
four independent choices.

The first choice - 32-bit PC vs 26-bit PC - is fixed by your ARM CPU.

The second choice - implicit vs explicit stack-limit checking - is
fixed by a combination of memory-management hardware and operating
system software: if your ARM-based environment supports implicit
stack-limit checking then use it; otherwise use explicit stack-limit
checking.

The third choice - of how to pass floating-point arguments - supports
efficient argument passing in both of the following circumstances:

 *  the floating point instruction set is emulated by software and
    floating point operations are dynamically very rare;

 *  the floating point instruction set is supported by hardware or
    floating point operations are dynamically common.

In each case, code conforming to one variant is not compatible with
code conforming to the other.

Only the choice between reentrant and non-reentrant variants is a true
user level choice. Further, as the alternatives are compatible, each
may be used where appropriate.


### 32-bit PC vs 26-bit PC

Older ARM CPUs and the 26-bit compatibility mode of newer CPUs use a
24-bit, word-address program counter, and pack the 4 status flags
(NZCV) and 2 interrupt-enable flags (IF) into the top 6 bits of r15,
and the 2 mode bits (m0, m1) into the least-significant bits of
r15. Thus r15 implements a combined PC + PSR.

Newer ARM CPUs use a 32-bit program counter (in r15) and a separate
PSR.

In 26-bit CPU modes, the PC + PSR is written to r14 by an ARM branch
with link instruction, so it is natural for the APCS to require the
reinstatement of the caller's PSR at function exit (a caller's PSR is
preserved across a function call).

In 32-bit CPU modes this reinstatement would be unacceptably expensive
in comparison to the gain from it, so the APCS does not require it and
a caller's PSR flags may be corrupted by a function call.


### Implicit vs Explicit Stack-Limit Checking

ARM-based systems vary widely in the sophistication of their memory
management hardware. Some can easily support multiple, auto-extending
stacks, while others have no memory management hardware at all.

Safe programming practices demand that stack overflow be detected.

The APCS defines conventions for software stack-limit checking
sufficient to support efficiently most requirements (including those
of multiple threads and chunked stacks).

The majority of ARM-based systems are expected to require software
stack-limit checking.


### Floating-Point Arguments in Floating-Point Registers

Historically, many ARM-based systems have made no use of the floating
point instruction set, or they used a software emulation of it.

On systems using a slow software emulation and making little use of
floating-point, there is a small disadvantage to passing
floating-point arguments in floating-point registers: all variadic
functions (such as printf) become slower, while only function calls
which actually take floating-point arguments become faster.

If your system has no floating-point hardware and is expected to make
little use of floating point, then it is better not to pass
floating-point arguments in floating-point registers. Otherwise, the
opposite choice is best.


### Reentrant vs Non-Reentrant Code

The reentrant variant of the APCS supports the generation of code free
of relocation directives (position independent and addressing all data
(indirectly) via a static base register). Such code is ideal for
placement in ROM and can be multiply threaded (shared between several
client processes). See "ARM Shared Library Format" of the Reference
Manual for further details.

In general, code to be placed in ROM or loaded into a shared library
is expected to be reentrant, while applications are expected not to
be.

See also <"C Language Calling Conventions".


### APCS-2 Compatibility

(APCS-2 - the second definition of The ARM Procedure Call Standard -
is recorded in Technical Memorandum `PLG-APCS, issue 4.00, 18-Apr-89`,
reproduced in the following Acorn publications: `RISC OS Programmer's
Reference Manual, vol IV, 1989`, (Acorn part number 0483,023); `ANSI C
Release 3, September 1989` , (Acorn part number 0470,101)).

APCS-R (APCS-2 for Acorn's RISC OS) is the following variant of
APCS-3:

 *  26-bit PC;
 *  explicit stack-limit checking;
 *  no passing of floating-point arguments in floating-point
    registers;
 *  non-reentrant code;

with the Acorn-specific constraints on the use of sl noted in APCS-2.

APCS-U (APCS-2 for Acorn's RISCiX) is the following variant of APCS-3:

 *  26-bit PC;
 *  implicit stack-limit checking (with sl reserved to Acorn);
 *  no passing of floating-point arguments in floating-point
    registers;
 *  non-reentrant code.

The (in APCS-2) obsolescent APCS-A has no equivalent in APCS-3.


## C Language Calling Conventions


### Argument Representation

A floating point value occupies 1, 2, or 3 words, as appropriate to
its type.  Floating point values are encoded in IEEE 754 format, with
the most significant word of a double having the lowest address.

The C compiler widens arguments of type float to type double to
support inter-working between ANSI C and classic C.

Char, short, pointer and other integral values occupy 1 word in an
argument list. Char and short values are widened by the C compiler
during argument marshalling.

On the ARM, characters are naturally unsigned. In -pcc mode, the C
compiler treats a plain char as signed, widening its value
appropriately when used as an argument, (classic C lacks the signed
char type, so plain chars are considered signed; ANSI C has signed,
unsigned and plain chars, the third, conventionally reflecting the
natural signedness of characters).

A structured value occupies an integral number of integer words (even
if it contains only floating point values).


### Argument List Marshalling

Argument values are marshalled in the order written in the source
program.

If passing floating-point (FP) arguments in FP registers, the first 4
FP arguments are loaded into FP registers.

The first 4 of the remaining argument words are loaded into a1-a4, and
the remainder are pushed on to the stack in reverse order (so that
arguments later in the argument list have higher addresses than those
earlier in the argument list). As a consequence, a FP value can be
passed in integer registers, or even split between an integer register
and the stack.

This follows from the need to support variadic functions, (functions
having a variable number of arguments, such as printf, scanf,
etc.). Alternatives which avoid the passing of FP values in integer
registers require that a caller know that a variadic function is being
called, and use different argument marshalling conventions for
variadic and non-variadic functions.


### Non-Simple Value Return

A non-simple type is any non-floating-point type of size greater than
1 word (including structures containing only floating-point fields),
and certain 1 word structured types.

A structure is called integer-like if its size is less than or equal
to one word, and the offset of each of its addressable sub-fields is
zero. An integer-like structured result is considered simple and is
returned in a1.

struct {int a:8, b:8, c:8, d:8;} and union {int i; char *p;} are both
integer-like; struct {char a; char b; char c; char d;} is not.

A multi-word or non-integer-like result is returned to an address
passed as an additional first argument to the function call. At the
machine level:

```
    TT tt = f(x, ...);
```

is implemented as:

```
    TT tt; f(&tt, x, ...);
```

### Function Entry - Introduction

A complete discussion of function entry is complex; here we discuss a
few of the most important issues and special cases.

The important issues for function entry are:

 *  establishing the static base (if the function is to be reentrant);
 *  creating the stack backtrace data structure (if needed);
 *  saving the floating point variable registers (if required);
 *  checking for stack overflow (if the stack chunk limit is
    explicit).

A function is called `leaf` if its body contains no function calls.

If function F calls function G immediately before an exit from F, the
call- exit sequence can often be replaced instead by a `return to
G`. After this transformation, the return to G is called a `tail call`
or `tail continuation`.

There are many subtle difficulties with tail continuations. Suppose
stacked arguments are unstacked by callers (almost mandatory for
variadic callees), then G cannot be directly tail-called if G itself
takes stacked arguments. This is because there is no return to F to
unstack them. Of course, if this call to G takes fewer arguments than
the current call to F, then some of F's stacked arguments can be
replaced by G's stacked arguments. However, this can be hard to assert
if F is variadic. More straightforwardly, there may be no tail-call of
G if the address of any of F's arguments or local variables has
"leaked out" of F. This is because on return to G, the address may be
invalidated by adjustment of the stack pointer. In general, this
precludes tail calls if any local variable or argument has its address
taken.

If a function is a leaf function, or all function calls from its body
are tail calls and, in both cases, the function uses no v-registers
(v1-v7) then the function need create no stack backtrace structure
(such functions will also be termed `frameless`).

A leaf function which makes no use of static data need not establish a
static base.


### Function Entry - Establishing the Static Base

(See also "The Shared Library Addressing Architecture" of the
Reference Manual).

The ARM shared library mechanism supports both the direct linking
together of functions into a `link unit`, and the indirect linking of
functions with the `stubs` of other link units. Thus a reentrant
function can be entered directly via a call from the same link unit
(an intra-link-unit call), or indirectly via a function pointer or
direct call from another link unit (an inter-link-unit call).

The general scheme for establishing the static base in reentrant code
is:

```
    intra MOV ip, sb  ; intra link unit (LU) calls target here
    inter             ; inter-LU calls target here, having loaded
                      ; ip via an inter-LU or fn-pointer veneer.

          <create backtrace structure, saving sb>

          MOV sb, ip  ; establish sb for this LU

          <rest of entry>
```

Code which is not required to be reentrant need not use a static
base. Code which is reentrant is marked as such, which allows the
linker to create the inter-LU veneers needed between independent
reentrant link units, and between reentrant and non-reentrant code.


### Function Entry - Creating the Stack Backtrace Structure

For non-reentrant, non-variadic functions the stack backtrace
structure can be created in just 3 instructions, as follows:

```
    MOV    ip, sp     ; save current sp, ready to save as old sp
    STMFD  sp!, {a1-a4, v1-v5, sb, fp, ip, lr, pc}  ; as needed
    SUB    fp, ip, #4
```

Each argument register a1-a4 need only be saved if a memory location
is needed for the corresponding parameter (because it has been spilled
by the register allocator or because its address has been taken).

Each of the registers v1-v7 need only be saved if it used by the
called function. The minimum set of registers to be saved is {fp,
old-sp, lr, pc}.

A reentrant function must avoid using ip in its entry sequence:

```
    STMFD  sp!, {sp, lr, pc}
    STMFD  sp!, {a1-a4, v1-v5, sb, fp}              ; as needed
    ADD    fp, sp, #8+4*|{a1-a4, v1-v5, sb, fp}|    ; as used above
```

sb (aka v6) must be saved by a reentrant function if it calls any
function from another link unit (which would alter the value in
sb). This means that, in general, sb must be saved on entry to all
non-leaf, reentrant functions.

For variadic functions the entry sequence is more complicated
again. Usually, it will be desired or required to make a contiguous
argument list on the stack.  For non-reentrant variadic functions this
can be done by:

    MOV    ip, sp           ; save current sp, ready to save as old sp
    STMFD  sp!, {a1-a4}     ; push arguments on stack
    SFMFD  f0, 4, [sp]!     ; push FP arguments on stack...
    STMFD  sp!, {v1-v6, fp, ip, lr, pc}       ; as needed
    SUB    fp, ip, #20      ; if all of a1-a4 pushed...

It is not necessary to push arguments corresponding to fixed
parameters (though saving a1-a4 is little more expensive than just
saving, say, a3-a4).

If floating point arguments are not being passed in floating point
registers then there is no need for the SFMFD. SFM is not supported by
the issue-1 floating-point instruction set and must be simulated by 4
STFEs. See the next section,"Function Entry - Saving and Restoring
Floating Point Registers" starting on page50.

For reentrant variadic functions, the requirements are yet more
complicated and the sequence becomes less elegant.


### Function Entry - Saving and Restoring Floating Point Registers

The issue-2 floating-point instruction set defines two new
instructions, `Store Floating Multiple` (SFM) and `Load Floating
Multiple` (LFM), for saving and restoring the floating-point
registers, as follows:

 *  SFM and LFM are exact inverses;

 *  a SFM will never trap, whatever the IEEE trap mode and the value
    transferred (unlike a STFE which can trap on storing a signalling
    NaN);

 *  SFM and LFM transfer 3-word internal representations of floating
    point values which vary from implementation to implementation, and
    which, in general, are unrelated to any of the supported IEEE
    representations;

 *  any 1-4, cyclically contiguous floating-point registers can be
    transferred by SFM/LFM (e.g. {f4-f7}, {f6, f7, f0}, {f7, f0},
    {f1}).

On function entry, a typical use of SFM might be as follows:

```
    SFMFD  f4, 4, [sp]!           ; save f4-f7 on a Full Descending stack,
                                  ; adjusting sp as values are pushed.
```                                  

On function exit, the corresponding sequence might be as follows:

```
    LFMEA  f4, 4, [fp, #-N]       ; restore f4-f7; fp-N points just
                                  ; above the floating point save area.
```

On function exit, sp-relative addressing may be unavailable if the
stack has been discontiguously extended.

In issue-1 instruction set compatibility modes, SFM and LFM have to be
simulated using sequences of STFEs and LDFEs.


### Function Entry - Checking for Stack Limit Violations

In some environments, stack overflow detection will be implicit: an
off stack reference will cause an address error or memory fault which
may, in turn, cause stack extension or program termination.

In other environments, the validity of the stack must be checked on
function entry and, perhaps at other times. There are three cases:

 *  the function uses 256 bytes or less of stack space;
 *  the function uses more than 256 bytes of stack space, but the
    amount is known and bounded at compile time;
 *  the function uses an amount of stack space unknown until run time.

The third case does not arise in C, save with stack-based
implementations of the non-standard, BSD-Unix alloca() function. The
APCS does not support alloca() in a straightforward manner.

In Modula-2, Pascal and other languages there may be arrays created on
block entry or passed as `open array arguments`, the size of which is
unknown until run time. Spiritually, these are located in the callee's
stack frame, so impact stack limit checking. In practice, this adds
little complication, as discussed in "Stack Limit Checking -
Vari-Sized Frames".

The check for stack limit violation is made at the end of the function
entry sequence, by which time ip is available as a work register. If
the check fails, a standard run-time support function
("__rt_stkovf_split_small" or "__rt_stkovf_split_big") is called. Each
environment which supports explicit stack limit checking must provide
these functions, which can do one of the following:

 *  terminate execution;
 *  extend the existing stack chunk, decrementing sl;
 *  allocate a new stack chunk, resetting sp and sl to point into it,
    and guaranteeing that an immediate repeat of the limit check will
    succeed.


### Stack Limit Checking - Small, Fixed Frames

For frames of 256 bytes or less the limit check is as follows:

```
    <create stack backtrace structure>

    CMPS   sp, sl
    BLLT   |__rt_stkovf_split_small|
    SUB    sp, sp, #<size of locals>    ; <= 256, by hypothesis
```

This adds 2 instructions and, in general, only 2 cycles to function
entry.

After a call to __rt_stkovf_split_small, fp and sp do not,
necessarily, point into the same stack chunk. Arguments passed on the
stack must be addressed by offsets from fp, not by offsets from sp.


### Stack Limit Checking - Large, Fixed Frames

For frames bigger than 256 bytes, the limit check proceeds as follows:

```
    SUB    ip, sp, #FrameSizeBound      ; can be done in 1 instr
    CMPS   ip, sl
    BLLT   |__rt_stkovf_split_big|
    SUB    sp, sp, #InitFrameSize       ; may take more than 1 instr
```

FrameSizeBound can be any convenient constant at least as big as the
largest frame the function will use. Note that functions containing
nested blocks may use different amounts of stack at different instants
during their execution.

InitFrameSize is the initial stack frame size: subsequent adjustments
within the called function require no limit check.

After a call to __rt_stkovf_split_big, fp and sp do not, necessarily,
point into the same stack chunk. Arguments passed on the stack must be
addressed by offsets from fp, not by offsets from sp.


### Stack Limit Checking - Vari-Sized Frames

(For Pascal-like languages).

The handling of frames the size of which is unknown at compile time,
is identical to the handling of large frames, save that:

 *  the computation of the proposed new stack pointer is more
    complicated, involving arguments to the function itself;

 *  the addressing of the vari-sized objects is more complicated than
    the addressing of fixed size objects need be;

 *  the vari-sized objects have to be initialised by the called
    function.

The general scheme for stack layout in this case is as follows:

```
        |                               |
        +-------------------------------+
        | Stack-based arguments         |
        +-------------------------------+
        | Stack backtrace data structure|  <--- fp points here
        | ... reg save area...          |
        +-------------------------------+
        +-------------------------------+
        | Area for vari-sized objects,  |
        | passed by value or created on |
        | block entry                   |
        +-------------------------------+
        | Fixed size remainder of frame |
        +-------------------------------+  <--- sp points here
```

Objects notionally passed by value are actually passed by reference
and copied by the callee.

The callee addresses the copied objects via pointers located in the
fixed size part of the stack frame, immediately above sp. These can be
addressed relative to sp. The original arguments are all addressable
relative to fp.

After a call to __rt_stkovf_split_big, fp and sp do not, necessarily,
point into the same stack chunk. Arguments passed on the stack must be
addressed by offsets from fp, not by offsets from sp.

If a nested block extends the stack by an amount which can't be known
until run time then the block entry must include a stack limit check.


### Function Exit

A great deal of design effort has been devoted to ensuring that
function exit can usually be implemented in a single instruction (this
is not the case if floating-point registers have to be
restored). Typically, there are at least as many function exits as
entries, so it is always advantageous to move an instruction from an
exit sequence to an entry sequence, (Fortran may violate this rule by
virtue of multiple entries, but on average the rule still holds
true). If exit is a single instruction then, in multi-exit functions,
further instructions can be saved by replacing branches to a single
exit by the exit instructions themselves.

Exit from functions which use no stack and save no floating point
registers is particularly simple:

```
    MOV    pc, lr
```

(26-bit compatibility demands MOVS pc, lr to reinstate the caller's
PSR flags, but this must not be used in 32-bit modes).

Exit from other functions which save no floating-point registers is
by:

```
    LDMEA  fp, {v1-v5, sb, fp, sp, pc}        ; as saved
```

Here, it is crucial that fp points just below the `save code pointer`,
as this value is not restored, (LDMEA is a pre-decrement multiple
load).

(26-bit compatibility demands LDMEA fp, {regs}^, to reinstate the
caller's PSR flags, but this must not be used in 32-bit modes).

The saving and restoring of floating-point registers is discussed
above.


## Some Examples

This section is not intended to be a general guide to the writing of
code generators, but it seems worthwhile to highlight some of the
optimisations that appear particularly relevant to the ARM and to this
standard.

In order to make effective use of the APCS, compilers must compile
code a procedure at a time. Line at a time compilation is
insufficient.

In the case of leaf functions, much of the standard entry sequence can
be omitted. In very small functions, such as those that frequently
occur implementing data abstractions, the function-call overhead can
be tiny.  Consider:

```
    typedef struct {...; int a; ...} foo;
    int foo_get_a(foo* f) {return(f-a);}
```

The function foo_get_a can compile to just:

```
    LDR    a1, [a1, #aOffset]
    MOV    pc, lr                 ; MOVS in 26-bit modes
```

In functions with a conditional as the top level statement, in which
one or other arm of the conditional is leaf (calls no functions), the
formation of a stack frame can be delayed. For example, the C
function:

```
    int get(Stream *s
    {
        if (s->cnt > 0)
        { --s;
            return *(s-p++);
        }
        else
        {
            ...
        }
    }
```

... could be compiled (non-reentrantly) into:


```
    get MOV    a3, a1
    ; if (s->cnt > 0)
        LDR    a2, [a3, #cntOffset]
        CMPS   a2, #0
    ; try the fast case,frameless and heavily conditionalized
        SUBGT  a2, a2, #1
        STRGT  a2, [a3, #cntOffset]
        LDRGT  a2, [a3, #pOffset]
        LDRBGT a1, [a2], #1
        STRGT  a2, [a3, #pOffset]
        MOVGT  pc, lr
    ; else, form a stack frame and handle the rest as normal code.
        MOV    ip, sp
        STMDB  sp!, {v1-v3, fp, ip, lr, pc}
        CMP    sp, sl
        BLLT   |__rt_stkovf_split_small|
        ...
        LDMEA  fp, {v1-v3, fp, sp, pc}
```

This is only worthwhile if the test can be compiled using any spare of
a1-a4 and ip, as scratch registers. This technique can significantly
accelerate certain speed-critical functions, such as read and write
character.

Finally, it is often worth applying the tail call optimisation,
especially to procedures which need to save no registers. For example:

```
    extern void *malloc(size_t n)
    {
        return primitive_alloc(NOTGCABLEBIT, BYTESTOWORDS(n));
    }
```

...is compiled (non-reentrantly) by the C compiler into:

```
    malloc
        ADD    a1, a1, #3         ; 1S
        MOV    a2, a1, LSR #2     ; 1S - BITESTOWORDS(n)
        MOV    a1, #1073741824    ; 1S - NOTGCABLEBIT
        B      primitive_alloc    ; 1N+2S = 4S
```

In this case, the optimisation avoids saving and restoring the
call-frame registers and saves 5 instructions (and many cycles-17 S
cycles on an uncached ARM with N=2S).


## The APCS in Non-User ARM Modes

There are some consequences of the ARM's architecture which, while not
explicit in the ARM Procedure Call Standard, need to be understood by
implementors of code intended to run in the ARM's SVC and IRQ modes.

An IRQ corrupts r14_irq, so IRQ-mode code must run with IRQs off until
r14_irq has been saved.

A general solution to this problem is to enter and exit IRQ handlers
written in high-level languages via hand-crafted wrappers, which on
entry save r14_irq, change mode to SVC, and enable IRQs; and on exit
restore the saved r14_irq, IRQ mode and the IRQ-enable state. Thus the
handlers themselves run in SVC mode, avoiding the problem in compiled
code.

SWIs corrupt r14_svc, so care has to be taken when calling SWIs in SVC
mode.

In high-level languages, SWIs are usually called out of line, so it
suffices to save and restore r14 in the calling veneer around the
SWI. If a compiler can generate in-line SWIs, then it should, of
course, also generate code to save and restore r14 in-line around the
SWI, unless it is known that the code will not be executed in SVC
mode.


### Aborts and pre-ARM6-based ARMs

With pre-ARM6-based ARMs (ARM2, ARM3), aborts corrupt r14_svc. This
means that care has to be taken when causing aborts in SVC mode.

An abort in SVC mode may be symptomatic of a fatal error, or it may be
caused by page faulting in SVC mode. Page faulting can occur because
an instruction needs to be fetched from a missing page (causing a
prefetch abort), or because of an attempted data access to a missing
page. The latter may occur even if the SVC-mode code is not itself
paged, (consider an unpaged kernel accessing a paged user-space).

A data abort is recoverable provided r14 contains nothing of value at
the instant of the abort. This can be ensured by:

 *  saving R14 on entry to every function and restoring it on exit;
 *  not using R14 as a temporary register in any function;
 *  avoiding page faults (stack faults) in function entry sequences.

A prefetch abort is harder to recover from, and an aborting BL
instruction cannot be recovered, so special action has to be taken to
protect page faulting function calls.

In code compiled from C, r14 is saved in the 2nd or 3rd instruction of
an entry sequence. Aligning all functions at addresses which are 0 or
4 modulo 16, ensures the critical part of the entry sequence cannot
prefetch-abort. A compiler can do this by padding code sections to a
multiple of 16 bytes, and being careful about the alignment of
functions within code sections.

Data-aborts early in function entry sequences can be avoided by using
a software stack-limit check.

A possible way to protect BL instructions from prefetch-aborts, is to
precede each BL by a

```
    MOV    ip, pc
```

instruction. If the BL faults, the prefetch abort handler can safely
overwrite r14 with ip before resuming execution at the target of the
BL. If the prefetch abort is not caused by a BL then this action is
harmless, as r14 has been corrupted anyway, (and, by design, contained
nothing of value at any instant a prefetch abort could occur).
