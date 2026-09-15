; Trusted ARMv3 VEC interpreter: caller must validate stream commands.
; r4=input, r5=V4 table, r6/r7=rowpair destinations, r8=row remaining,
; r9=rows remaining, r10=V1 table, r11=rowpair stride bytes.
; Stack: [0]=row width, [4]=coded-row flag, [8]=decoder, [12]=row count.
; r0-r3/ip/lr are paint temporaries. All APCS callee-saved registers survive.
        AREA |C$$code|, CODE, READONLY
        EXPORT vx_run_vec_asm
vx_run_vec_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #16
        str r0, [sp, #8]
        mov r4, r1
        ldr r6, [r0]
        ldr r1, [r0, #8]
        mov r8, r1, lsr #16
        mov r9, r1, lsl #16
        mov r9, r9, lsr #16
        str r9, [sp, #12]
        str r8, [sp]
        mov r11, r8, lsl #4
        add r7, r6, r11
        add r10, r0, #16
        add r5, r10, #4096
vxv_next
        ldrb r0, [r4], #1
        and lr, r0, #63
        add lr, lr, #1
        cmp r0, #64
        orrhs r9, r9, #&80000000
        blo vxv_skip
        cmp r0, #128
        blo vxv_literal1
        cmp r0, #192
        blo vxv_literal4
        cmp r0, #255
        beq vxv_repeat4
; V1 repeat: one index followed by repeated expanded quadrant colors.
        sub r8, r8, lr
        ldrb ip, [r4], #1
        add ip, r10, ip, lsl #4
        str lr, [sp, #4]
        ldmia ip!, {r0,r2}
        mov r1, r0
        mov r3, r2
vxv_repeat1_loop
        stmia r6!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat1_loop
        ldr lr, [sp, #4]
        ldmia ip, {r0,r2}
        mov r1, r0
        mov r3, r2
vxv_repeat1_bottom
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat1_bottom
        b vxv_advance
vxv_skip
        sub r8, r8, lr
        add r6, r6, lr, lsl #4
        add r7, r7, lr, lsl #4
        b vxv_advance
vxv_literal1
        sub r8, r8, lr
vxv_literal1_loop
        ldrb ip, [r4], #1
        add ip, r10, ip, lsl #4
        ldmia ip!, {r0,r2}
        mov r1, r0
        mov r3, r2
        stmia r6!, {r0-r3}
        ldmia ip, {r0,r2}
        mov r1, r0
        mov r3, r2
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_literal1_loop
        b vxv_advance
vxv_literal4
        sub r8, r8, lr
vxv_literal4_loop
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r6!, {r0-r3}
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        beq vxv_advance
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r6!, {r0-r3}
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        beq vxv_advance
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r6!, {r0-r3}
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        beq vxv_advance
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r6!, {r0-r3}
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_literal4_loop
        b vxv_advance
vxv_repeat4
        ldrb lr, [r4], #1
        add lr, lr, #1
        sub r8, r8, lr
        mov ip, lr
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_top
        stmia r6!, {r0-r3}
        subs lr, lr, #1
        beq vxv_repeat4_top_done
        stmia r6!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat4_top
vxv_repeat4_top_done
        mov lr, ip
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_bottom
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        beq vxv_repeat4_bottom_done
        stmia r7!, {r0-r3}
        subs lr, lr, #1
        bne vxv_repeat4_bottom
vxv_repeat4_bottom_done
vxv_advance
        cmp r8, #0
        bne vxv_next
        tst r9, #&80000000
        beq vxv_clean_row
        bic r9, r9, #&80000000
        ldr r0, [sp, #8]
        add r0, r0, #4096
        ldr r1, [sp, #12]
        sub r1, r1, r9
        ldr r2, [r0, #2064]
        cmp r1, r2
        strlo r1, [r0, #2064]
        add r1, r1, #1
        str r1, [r0, #2068]
vxv_clean_row
        subs r9, r9, #1
        beq vxv_ok
        mov r8, r11, lsr #4
        add r6, r6, r11
        add r7, r7, r11
        b vxv_next
vxv_ok
        mov r0, #0
        add sp, sp, #16
        ldmfd sp!, {r4-r11, pc}
        END
