# ARM Procedure Call Standard

The *ARM Procedure Call Standard* (*APCS*) is a set of rules which regulate and facilitate calls between separately compiled or assembled program fragments. The APCS defines:

- Constraints on the use of registers
- Stack conventions
- The format of a stack-based data structure, used by stack tracing programs to reconstruct a sequence of outstanding calls
- The passing of machine-level arguments, and the return of machine-level results at externally visible function/procedure calls
- Support for the ARM shared library mechanism; a standard way for shared (reentrant) code to address the static data of its clients

## Chapter Overview

- [The ARM Procedure Call Standard](4atsa.md)
- [APCS Variants](4atsb.md)
- [C Language Calling Conventions](4atsc.md)
- [Some Examples](4atsd.md)
- [The APCS in Non-User ARM Modes](4atse.md)

Since the ARM CPU is used in a wide variety of systems, the APCS is not a single standard, but a consistent family of standards. See [APCS Variants](4atsb.md) for details of the variants in the family. Implementors of run-time systems, operating systems, embedded control monitors, etc., must choose the variant(s) most appropriate to their requirements.

Naturally, there can be no binary compatibility between program fragments which conform to different members of the APCS family. Those concerned with long-term binary compatibility must choose their options carefully.

*function* is used to mean function, procedure or subroutine.

## Design Criteria

Throughout its history, the APCS has compromised between fastest, smallest and easiest to use.

The criteria we have considered to be important are:

- Function call should be fast and it should be easy for compilers to optimise function entry sequences.
- The function call sequence should be as compact as possible.
- Extensible stacks and multiple stacks should be accommodated.
- The standard should encourage the production of reentrant code, with writable data separated from code.
- The standard should be simple enough to be used by assembly language programmers, and should support simple approaches to link editing, debugging and run-time error diagnosis.

Overall, we have tended to rank compact code and a clear definition most highly, with simplicity and ease of use ahead of performance in matters of fine detail where the impact on performance is small.
