# C Language Calling Conventions

## Argument Representation

A floating point value occupies 1, 2, or 3 words, as appropriate to its type. Floating point values are encoded in IEEE 754 format, with the most significant word of a double having the lowest address.

The C compiler widens arguments of type float to type double to support inter-working between ANSI C and classic C.

Char, short, pointer and other integral values occupy 1 word in an argument list. Char and short values are widened by the C compiler during argument marshalling.

On the ARM, characters are naturally unsigned. In `-pcc` mode, the C compiler treats a plain char as signed, widening its value appropriately when used as an argument (classic C lacks the `signed char` type, so plain chars are considered signed; ANSI C has signed, unsigned and plain chars, the third, conventionally reflecting the natural signedness of characters).

A structured value occupies an integral number of integer words (even if it contains only floating point values).

## Argument List Marshalling

Argument values are marshalled in the order written in the source program.

If passing floating-point (FP) arguments in FP registers, the first 4 FP arguments are loaded into FP registers.

The first 4 of the remaining argument words are loaded into a1-a4, and the remainder are pushed on to the stack in reverse order (so that arguments later in the argument list have higher addresses than those earlier in the argument list). As a consequence, a FP value can be passed in integer registers, or even split between an integer register and the stack.

This follows from the need to support variadic functions (functions having a variable number of arguments, such as printf, scanf, etc.). Alternatives which avoid the passing of FP values in integer registers require that a caller know that a variadic function is being called, and use different argument marshalling conventions for variadic and non-variadic functions.

## Non-Simple Value Return

A non-simple type is any non-floating-point type of size greater than 1 word (including structures containing only floating-point fields), and certain 1 word structured types.

A structure is called integer-like if its size is less than or equal to one word, and the offset of each of its addressable sub-fields is zero. An integer-like structured result is considered simple and is returned in a1.

`struct {int a:8, b:8, c:8, d:8;}` and `union {int i; char *p;}` are both integer-like; `struct {char a; char b; char c; char d;}` is not.

A multi-word or non-integer-like result is returned to an address passed as an additional first argument to the function call. At the machine level:

```c
TT tt = f(x, ...);
```

is implemented as:

```c
TT tt; f(&tt, x, ...);
```

## Function Entry

A complete discussion of function entry is complex; here we discuss a few of the most important issues and special cases.

The important issues for function entry are:

- Establishing the static base (if the function is to be reentrant)
- Creating the stack backtrace data structure (if needed)
- Saving the floating point variable registers (if required)
- Checking for stack overflow (if the stack chunk limit is explicit)

A function is called *leaf* if its body contains no function calls.

If function F calls function G immediately before an exit from F, the call-exit sequence can often be replaced instead by a *return to G*. After this transformation, the return to G is called a *tail call* or *tail continuation*.

There are many subtle difficulties with tail continuations. Suppose stacked arguments are unstacked by callers (almost mandatory for variadic callees), then G cannot be directly tail-called if G itself takes stacked arguments. This is because there is no return to F to unstack them. Of course, if this call to G takes fewer arguments than the current call to F, then some of F's stacked arguments can be replaced by G's stacked arguments. However, this can be hard to assert if F is variadic. More straightforwardly, there may be no tail-call of G if the address of any of F's arguments or local variables has "leaked out" of F. This is because on return to G, the address may be invalidated by adjustment of the stack pointer. In general, this precludes tail calls if any local variable or argument has its address taken.

If a function is a leaf function, or all function calls from its body are tail calls and, in both cases, the function uses no v-registers (v1-v7) then the function need create no stack backtrace structure (such functions will also be termed *frameless*).

A leaf function which makes no use of static data need not establish a static base.

## Function Entry - Establishing the Static Base

(See also the ARM shared library addressing architecture documentation).

The ARM shared library mechanism supports both the direct linking together of functions into a *link unit*, and the indirect linking of functions with the *stubs* of other link units. Thus a reentrant function can be entered directly via a call from the same link unit (an intra-link-unit call), or indirectly via a function pointer or direct call from another link unit (an inter-link-unit call).

The general scheme for establishing the static base in reentrant code is:

```arm
intra MOV ip, sb             ; intra link unit (LU) calls target here
inter                        ; inter-LU calls target here, having loaded
                             ; ip via an inter-LU or fn-pointer veneer.
```

```arm
      MOV sb, ip             ; establish sb for this LU
```

Code which is not required to be reentrant need not use a static base. Code which is reentrant is marked as such, which allows the linker to create the inter-LU veneers needed between independent reentrant link units, and between reentrant and non-reentrant code.

## Function Entry - Creating the Stack Backtrace Structure

For non-reentrant, non-variadic functions the stack backtrace structure can be created in just 3 instructions, as follows:

```arm
MOV    ip, sp                ; save current sp, ready to save as old sp
STMFD  sp!, {a1-a4, v1-v5, sb, fp, ip, lr, pc}
                             ; as needed
SUB    fp, ip, #4
```

Each argument register a1-a4 need only be saved if a memory location is needed for the corresponding parameter (because it has been spilled by the register allocator or because its address has been taken).

Each of the registers v1-v7 need only be saved if it used by the called function. The minimum set of registers to be saved is `{fp, old-sp, lr, pc}`.

A reentrant function must avoid using ip in its entry sequence:

```arm
STMFD  sp!, {sp, lr, pc}
STMFD  sp!, {a1-a4, v1-v5, sb, fp}      ; as needed
ADD    fp, sp, #8+4*|{a1-a4, v1-v5, sb, fp}|  ; as used above
```

sb (aka v6) must be saved by a reentrant function if it calls any function from another link unit (which would alter the value in sb). This means that, in general, sb must be saved on entry to all non-leaf, reentrant functions.

For variadic functions the entry sequence is more complicated again. Usually, it will be desired or required to make a contiguous argument list on the stack. For non-reentrant variadic functions this can be done by:

```arm
MOV    ip, sp                ; save current sp, ready to save as old sp
STMFD  sp!, {a1-a4}          ; push arguments on stack
SFMFD  f0, 4, [sp]!          ; push FP arguments on stack...
STMFD  sp!, {v1-v6, fp, ip, lr, pc}
                             ; as needed
SUB    fp, ip, #20           ; if all of a1-a4 pushed...
```

It is not necessary to push arguments corresponding to fixed parameters (though saving a1-a4 is little more expensive than just saving, say, a3-a4).

If floating point arguments are not being passed in floating point registers then there is no need for the SFMFD. SFM is not supported by the issue-1 floating-point instruction set and must be simulated by 4 STFEs. See the next section, [Function Entry - Saving and Restoring Floating Point Registers](#function-entry---saving-and-restoring-floating-point-registers).

For reentrant variadic functions, the requirements are yet more complicated and the sequence becomes less elegant.

## Function Entry - Saving and Restoring Floating Point Registers

The issue-2 floating-point instruction set defines two new instructions, *Store Floating Multiple* (SFM) and *Load Floating Multiple* (LFM), for saving and restoring the floating-point registers, as follows:

- SFM and LFM are exact inverses
- A SFM will never trap, whatever the IEEE trap mode and the value transferred (unlike a STFE which can trap on storing a signalling NaN)
- SFM and LFM transfer 3-word internal representations of floating point values which vary from implementation to implementation, and which, in general, are unrelated to any of the supported IEEE representations
- Any 1-4, cyclically contiguous floating-point registers can be transferred by SFM/LFM (e.g. {f4-f7}, {f6, f7, f0}, {f7, f0}, {f1})

On function entry, a typical use of SFM might be as follows:

```arm
SFMFD  f4, 4, [sp]!          ; save f4-f7 on a Full Descending stack,
                             ; adjusting sp as values are pushed.
```

On function exit, the corresponding sequence might be as follows:

```arm
LFMEA  f4, 4, [fp, #-N]     ; restore f4-f7; fp-N points just
                             ; above the floating point save area.
```

On function exit, sp-relative addressing may be unavailable if the stack has been discontiguously extended.

In issue-1 instruction set compatibility modes, SFM and LFM have to be simulated using sequences of STFEs and LDFEs.

## Function Entry - Stack Limit Checking

In some environments, stack overflow detection will be implicit: an off stack reference will cause an address error or memory fault which may, in turn, cause stack extension or program termination.

In other environments, the validity of the stack must be checked on function entry and, perhaps at other times. There are three cases:

- The function uses 256 bytes or less of stack space
- The function uses more than 256 bytes of stack space, but the amount is known and bounded at compile time
- The function uses an amount of stack space unknown until run time

The third case does not arise in C, save with stack-based implementations of the non-standard, BSD-Unix `alloca()` function. The APCS does not support `alloca()` in a straightforward manner.

In Modula-2, Pascal and other languages there may be arrays created on block entry or passed as *open array arguments*, the size of which is unknown until run time. Spiritually, these are located in the callee's stack frame, so impact stack limit checking. In practice, this adds little complication, as discussed in [Stack Limit Checking - Vari-Sized Frames](#stack-limit-checking---vari-sized-frames).

The check for stack limit violation is made at the end of the function entry sequence, by which time ip is available as a work register. If the check fails, a standard run-time support function (`__rt_stkovf_split_small` or `__rt_stkovf_split_big`) is called. Each environment which supports explicit stack limit checking must provide these functions, which can do one of the following:

- Terminate execution
- Extend the existing stack chunk, decrementing sl
- Allocate a new stack chunk, resetting sp and sl to point into it, and guaranteeing that an immediate repeat of the limit check will succeed

### Stack Limit Checking - Small Frames

For frames of 256 bytes or less the limit check is as follows:

```arm
CMPS   sp, sl
BLLT   |__rt_stkovf_split_small|
SUB    sp, sp, #<frame size>
```

This adds 2 instructions and, in general, only 2 cycles to function entry.

After a call to `__rt_stkovf_split_small`, fp and sp do not, necessarily, point into the same stack chunk. Arguments passed on the stack must be addressed by offsets from fp, not by offsets from sp.

### Stack Limit Checking - Large Frames

For frames bigger than 256 bytes, the limit check proceeds as follows:

```arm
SUB    ip, sp, #FrameSizeBound    ; can be done in 1 instr
CMPS   ip, sl
BLLT   |__rt_stkovf_split_big|
SUB    sp, sp, #InitFrameSize     ; may take more than 1 instr
```

FrameSizeBound can be any convenient constant at least as big as the largest frame the function will use. Note that functions containing nested blocks may use different amounts of stack at different instants during their execution.

InitFrameSize is the initial stack frame size: subsequent adjustments within the called function require no limit check.

After a call to `__rt_stkovf_split_big`, fp and sp do not, necessarily, point into the same stack chunk. Arguments passed on the stack must be addressed by offsets from fp, not by offsets from sp.

### Stack Limit Checking - Vari-Sized Frames

(For Pascal-like languages).

The handling of frames the size of which is unknown at compile time, is identical to the handling of large frames, save that:

- The computation of the proposed new stack pointer is more complicated, involving arguments to the function itself
- The addressing of the vari-sized objects is more complicated than the addressing of fixed size objects need be
- The vari-sized objects have to be initialised by the called function

The general scheme for stack layout in this case is as follows:

| Section | Position |
|---------|----------|
| Stack-based arguments | |
| Stack backtrace data structure... reg save area... | <-- fp points here |
| | |
| Area for vari-sized objects, passed by value or created on block entry | |
| Fixed size remainder of frame | <-- sp points here |

Objects notionally passed by value are actually passed by reference and copied by the callee.

The callee addresses the copied objects via pointers located in the fixed size part of the stack frame, immediately above sp. These can be addressed relative to sp. The original arguments are all addressable relative to fp.

After a call to `__rt_stkovf_split_big`, fp and sp do not, necessarily, point into the same stack chunk. Arguments passed on the stack must be addressed by offsets from fp, not by offsets from sp.

If a nested block extends the stack by an amount which can't be known until run time then the block entry must include a stack limit check.

## Function Exit

A great deal of design effort has been devoted to ensuring that function exit can usually be implemented in a single instruction (this is not the case if floating-point registers have to be restored). Typically, there are at least as many function exits as entries, so it is always advantageous to move an instruction from an exit sequence to an entry sequence (Fortran may violate this rule by virtue of multiple entries, but on average the rule still holds true). If exit is a single instruction then, in multi-exit functions, further instructions can be saved by replacing branches to a single exit by the exit instructions themselves.

Exit from functions which use no stack and save no floating point registers is particularly simple:

```arm
MOV    pc, lr
```

(26-bit compatibility demands `MOVS pc, lr` to reinstate the caller's PSR flags, but this must not be used in 32-bit modes).

Exit from other functions which save no floating-point registers is by:

```arm
LDMEA  fp, {v1-v5, sb, fp, sp, pc}      ; as saved
```

Here, it is crucial that fp points just below the *save code pointer*, as this value is not restored (`LDMEA` is a pre-decrement multiple load).

(26-bit compatibility demands `LDMEA fp, {regs}^`, to reinstate the caller's PSR flags, but this must not be used in 32-bit modes).

The saving and restoring of floating-point registers is discussed above.
