# APCS Variants

There are, currently, 2 x 2 x 2 x 2 = 16 APCS variants, derived from four independent choices.

The first choice - 32-bit PC vs 26-bit PC - is fixed by your ARM CPU.

The second choice - implicit vs explicit stack-limit checking - is fixed by a combination of memory-management hardware and operating system software: if your ARM-based environment supports implicit stack-limit checking then use it; otherwise use explicit stack-limit checking.

The third choice - of how to pass floating-point arguments - supports efficient argument passing in both of the following circumstances:

- The floating point instruction set is emulated by software and floating point operations are dynamically very rare
- The floating point instruction set is supported by hardware or floating point operations are dynamically common

In each case, code conforming to one variant is not compatible with code conforming to the other.

Only the choice between reentrant and non-reentrant variants is a true user level choice. Further, as the alternatives are compatible, each may be used where appropriate.

## 32-bit PC vs 26-bit PC

Older ARM CPUs and the 26-bit compatibility mode of newer CPUs use a 24-bit, word-address program counter, and pack the 4 status flags (NZCV) and 2 interrupt-enable flags (IF) into the top 6 bits of r15, and the 2 mode bits (m0, m1) into the least-significant bits of r15. Thus r15 implements a combined PC + PSR.

Newer ARM CPUs use a 32-bit program counter (in r15) and a separate PSR.

In 26-bit CPU modes, the PC + PSR is written to r14 by an ARM branch with link instruction, so it is natural for the APCS to require the reinstatement of the caller's PSR at function exit (a caller's PSR is preserved across a function call).

In 32-bit CPU modes this reinstatement would be unacceptably expensive in comparison to the gain from it, so the APCS does not require it and a caller's PSR flags may be corrupted by a function call.

## Implicit vs Explicit Stack-Limit Checking

ARM-based systems vary widely in the sophistication of their memory management hardware. Some can easily support multiple, auto-extending stacks, while others have no memory management hardware at all.

Safe programming practices demand that stack overflow be detected.

The APCS defines conventions for software stack-limit checking sufficient to support efficiently most requirements (including those of multiple threads and chunked stacks).

The majority of ARM-based systems are expected to require software stack-limit checking.

## Floating-Point Arguments in Floating-Point Registers

Historically, many ARM-based systems have made no use of the floating point instruction set, or they used a software emulation of it.

On systems using a slow software emulation and making little use of floating-point, there is a small disadvantage to passing floating-point arguments in floating-point registers: all variadic functions (such as printf) become slower, while only function calls which actually take floating-point arguments become faster.

If your system has no floating-point hardware and is expected to make little use of floating point, then it is better not to pass floating-point arguments in floating-point registers. Otherwise, the opposite choice is best.

## Reentrant vs Non-Reentrant Code

The reentrant variant of the APCS supports the generation of code free of relocation directives (position independent and addressing all data (indirectly) via a static base register). Such code is ideal for placement in ROM and can be multiply threaded (shared between several client processes).

In general, code to be placed in ROM or loaded into a shared library is expected to be reentrant, while applications are expected not to be.

See also [C Language Calling Conventions](4atsc.md).

## APCS-2 Compatibility

(APCS-2 - the second definition of The ARM Procedure Call Standard - is recorded in Technical Memorandum *PLG-APCS, issue 4.00, 18-Apr-89*, reproduced in the following Acorn publications: *RISC OS Programmer's Reference Manual, vol IV, 1989*, (Acorn part number 0483,023); *ANSI C Release 3, September 1989*, (Acorn part number 0470,101)).

APCS-R (APCS-2 for Acorn's RISC OS) is the following variant of APCS-3:

- 26-bit PC
- Explicit stack-limit checking
- No passing of floating-point arguments in floating-point registers
- Non-reentrant code

with the Acorn-specific constraints on the use of sl noted in APCS-2.

APCS-U (APCS-2 for Acorn's RISCiX) is the following variant of APCS-3:

- 26-bit PC
- Implicit stack-limit checking (with sl reserved to Acorn)
- No passing of floating-point arguments in floating-point registers
- Non-reentrant code

The (in APCS-2) obsolescent APCS-A has no equivalent in APCS-3.
