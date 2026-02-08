# Some Examples

This section is not intended to be a general guide to the writing of code generators, but it seems worthwhile to highlight some of the optimisations that appear particularly relevant to the ARM and to this standard.

In order to make effective use of the APCS, compilers must compile code a procedure at a time. Line at a time compilation is insufficient.

In the case of leaf functions, much of the standard entry sequence can be omitted. In very small functions, such as those that frequently occur implementing data abstractions, the function-call overhead can be tiny. Consider:

```c
typedef struct {...; int a; ...} foo;
int foo_get_a(foo* f) {return(f->a);}
```

The function `foo_get_a` can compile to just:

```arm
LDR    a1, [a1, #aOffset]
MOV    pc, lr                    ; MOVS in 26-bit modes
```

In functions with a conditional as the top level statement, in which one or other arm of the conditional is leaf (calls no functions), the formation of a stack frame can be delayed. For example, the C function:

```c
int get(Stream *s)
{
    if (s->cnt > 0)
    {    --s;
        return *(s->p++);
    }
    else
    {
        ...
    }
}
```

...could be compiled (non-reentrantly) into:

```arm
get MOV    a3, a1
; if (s->cnt > 0)
    LDR    a2, [a3, #cntOffset]
    CMPS   a2, #0
; try the fast case, frameless and heavily conditionalized
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

This is only worthwhile if the test can be compiled using any spare of a1-a4 and ip, as scratch registers. This technique can significantly accelerate certain speed-critical functions, such as read and write character.

Finally, it is often worth applying the tail call optimisation, especially to procedures which need to save no registers. For example:

```c
extern void *malloc(size_t n)
{
    return primitive_alloc(NOTGCABLEBIT, BYTESTOWORDS(n));
}
```

...is compiled (non-reentrantly) by the C compiler into:

```arm
malloc
    ADD    a1, a1, #3             ; 1S
    MOV    a2, a1, LSR #2         ; 1S - BYTESTOWORDS(n)
    MOV    a1, #1073741824        ; 1S - NOTGCABLEBIT
    B      primitive_alloc        ; 1N+2S = 4S
```

In this case, the optimisation avoids saving and restoring the call-frame registers and saves 5 instructions (and many cycles - 17 S cycles on an uncached ARM with N=2S).
