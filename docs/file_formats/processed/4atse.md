# The APCS in Non-User ARM Modes

There are some consequences of the ARM's architecture which, while not explicit in the ARM Procedure Call Standard, need to be understood by implementors of code intended to run in the ARM's SVC and IRQ modes.

An IRQ corrupts r14_irq, so IRQ-mode code must run with IRQs off until r14_irq has been saved.

A general solution to this problem is to enter and exit IRQ handlers written in high-level languages via hand-crafted wrappers, which on entry save r14_irq, change mode to SVC, and enable IRQs; and on exit restore the saved r14_irq, IRQ mode and the IRQ-enable state. Thus the handlers themselves run in SVC mode, avoiding the problem in compiled code.

SWIs corrupt r14_svc, so care has to be taken when calling SWIs in SVC mode.

In high-level languages, SWIs are usually called out of line, so it suffices to save and restore r14 in the calling veneer around the SWI. If a compiler can generate in-line SWIs, then it should, of course, also generate code to save and restore r14 in-line around the SWI, unless it is known that the code will not be executed in SVC mode.

## Aborts and Pre-ARM6-Based ARMs

With pre-ARM6-based ARMs (ARM2, ARM3), aborts corrupt r14_svc. This means that care has to be taken when causing aborts in SVC mode.

An abort in SVC mode may be symptomatic of a fatal error, or it may be caused by page faulting in SVC mode. Page faulting can occur because an instruction needs to be fetched from a missing page (causing a prefetch abort), or because of an attempted data access to a missing page. The latter may occur even if the SVC-mode code is not itself paged (consider an unpaged kernel accessing a paged user-space).

A data abort is recoverable provided r14 contains nothing of value at the instant of the abort. This can be ensured by:

- Saving r14 on entry to every function and restoring it on exit
- Not using r14 as a temporary register in any function
- Avoiding page faults (stack faults) in function entry sequences

A prefetch abort is harder to recover from, and an aborting BL instruction cannot be recovered, so special action has to be taken to protect page faulting function calls.

In code compiled from C, r14 is saved in the 2nd or 3rd instruction of an entry sequence. Aligning all functions at addresses which are 0 or 4 modulo 16, ensures the critical part of the entry sequence cannot prefetch-abort. A compiler can do this by padding code sections to a multiple of 16 bytes, and being careful about the alignment of functions within code sections.

Data-aborts early in function entry sequences can be avoided by using a software stack-limit check.

A possible way to protect BL instructions from prefetch-aborts, is to precede each BL by a:

```arm
MOV    ip, pc
```

instruction. If the BL faults, the prefetch abort handler can safely overwrite r14 with ip before resuming execution at the target of the BL. If the prefetch abort is not caused by a BL then this action is harmless, as r14 has been corrupted anyway (and, by design, contained nothing of value at any instant a prefetch abort could occur).
