; Stats-free ARMv3 VEC interpreter. All command bounds are checked before
; painting. r4=input, r5=end, r6/r7=rowpair destinations, r8=row remaining,
; r9=rows remaining, r10=V1 table, r11=rowpair stride bytes.
; Stack locals: [0]=blocks per row, [4]=current run length.
; r0-r3/ip/lr are paint temporaries. All APCS callee-saved registers survive.
        AREA |C$$code|, CODE, READONLY
        EXPORT vx_run_vec_asm
vx_run_vec_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #8
        mov r4, r1
        add r5, r1, r2
        ldr r6, [r0]
        ldr r1, [r0, #8]
        mov r8, r1, lsr #16
        mov r9, r1, lsl #16
        mov r9, r9, lsr #16
        str r8, [sp]
        mov r11, r8, lsl #4
        add r7, r6, r11
        add r10, r0, #16
        cmp r9, #0
        beq vxv_ok
        cmp r8, #0
        beq vxv_bad
vxv_next
        cmp r4, r5
        bhs vxv_bad
        ldrb r0, [r4], #1
        cmp r0, #255
        beq vxv_repeat4
        and lr, r0, #63
        add lr, lr, #1
        cmp lr, r8
        bhi vxv_bad
        cmp r0, #64
        blo vxv_skip
        cmp r0, #128
        blo vxv_literal1
        cmp r0, #192
        blo vxv_literal4
; V1 repeat: one index followed by repeated expanded quadrant colors.
        cmp r4, r5
        bhs vxv_bad
        sub r8, r8, lr
        ldrb r0, [r4], #1
        add r0, r10, r0, lsl #4
        ldmia r0, {r0-r3}
vxv_repeat1_loop
        str r0, [r6], #4
        str r0, [r6], #4
        str r1, [r6], #4
        str r1, [r6], #4
        str r2, [r7], #4
        str r2, [r7], #4
        str r3, [r7], #4
        str r3, [r7], #4
        subs lr, lr, #1
        bne vxv_repeat1_loop
        b vxv_advance
vxv_skip
        sub r8, r8, lr
        add r6, r6, lr, lsl #4
        add r7, r7, lr, lsl #4
        b vxv_advance
vxv_literal1
        sub r1, r5, r4
        cmp r1, lr
        blo vxv_bad
        sub r8, r8, lr
vxv_literal1_loop
        ldrb r0, [r4], #1
        add r0, r10, r0, lsl #4
        ldmia r0, {r0-r3}
        str r0, [r6], #4
        str r0, [r6], #4
        str r1, [r6], #4
        str r1, [r6], #4
        str r2, [r7], #4
        str r2, [r7], #4
        str r3, [r7], #4
        str r3, [r7], #4
        subs lr, lr, #1
        bne vxv_literal1_loop
        b vxv_advance
vxv_literal4
        sub r1, r5, r4
        cmp r1, lr, lsl #2
        blo vxv_bad
        sub r8, r8, lr
        add ip, r10, #4096
vxv_literal4_loop
        ldrb r0, [r4], #1
        add r0, ip, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, ip, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r6!, {r0-r3}
        ldrb r0, [r4], #1
        add r0, ip, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, ip, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_literal4_loop
        b vxv_advance
vxv_repeat4
        sub r1, r5, r4
        cmp r1, #5
        blo vxv_bad
        ldrb lr, [r4], #1
        add lr, lr, #1
        cmp lr, r8
        bhi vxv_bad
        sub r8, r8, lr
        str lr, [sp, #4]
        add ip, r10, #4096
        ldrb r0, [r4], #1
        add r0, ip, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, ip, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_top
        stmia r6!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat4_top
        ldr lr, [sp, #4]
        ldrb r0, [r4], #1
        add r0, ip, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, ip, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_bottom
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat4_bottom
vxv_advance
        cmp r8, #0
        bne vxv_next
        subs r9, r9, #1
        beq vxv_ok
        ldr r8, [sp]
        add r6, r6, r11
        add r7, r7, r11
        b vxv_next
vxv_ok
        mov r0, #0
        b vxv_return
vxv_bad
        mov r0, #6
vxv_return
        add sp, sp, #8
        ldmfd sp!, {r4-r11, pc}
        END
