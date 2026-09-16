; Trusted ARMv3 VEC interpreter: caller must validate stream commands.
; V1/V4 repeat painters peel one block for odd run lengths, then write
; block pairs with a single loop test per pair: (5c stm + 1c subs + taken
; branch)/2 blocks per row pass amortizes the taken-branch refill across
; two blocks instead of one.
; Each handler's tail performs the row-remaining test itself, so the command
; that exhausts a row branches out to vxv_rowend while every other command
; falls straight through into its own back-edge to vxv_next: one taken branch
; per command instead of a branch to a shared advance plus a re-test of r8.
; Where the run length is still live at the tail the test is the "subs" that
; used to be the decrement (skip: lr; V4 repeat: ip); the loop-counted
; handlers decrement r8 at entry as before and compare here.
; r4=input, r5=V4 table, r6/r7=rowpair destinations, r8=row remaining,
; r9=rows remaining; bit31 = this row has coded blocks, bit30 = dirty_first
; not yet written (rows are visited in increasing order, so the first coded
; row is the minimum and needs no compare). r10=V1 table,
; r11=rowpair stride bytes.
; Stack: [0]=dirty-bounds base (dec+4096), [4]=V1-repeat count spill,
; [8]=decoder, [12]=total row count.
; r0-r3/ip/lr are paint temporaries. All APCS callee-saved registers survive.
        AREA |C$$code|, CODE, READONLY
        EXPORT vx_run_vec_asm
vx_run_vec_asm
        stmfd sp!, {r4-r11, lr}
        sub sp, sp, #16
        str r0, [sp, #8]
        add ip, r0, #4096
        str ip, [sp]
        mov r4, r1
        ldr r6, [r0]
        ldr r1, [r0, #8]
        mov r8, r1, lsr #16
        mov r9, r1, lsl #16
        mov r9, r9, lsr #16
        str r9, [sp, #12]
        orr r9, r9, #&40000000
        mov r11, r8, lsl #4
        add r7, r6, r11
        add r10, r0, #16
        add r5, r10, #4096
vxv_next
        ldrb r0, [r4], #1
        and lr, r0, #63
        add lr, lr, #1
        cmp r0, #64
        blo vxv_skip
        orr r9, r9, #&80000000
        sub ip, r0, #128
        cmp ip, #64
        blo vxv_literal4
        cmp r0, #128
        blo vxv_literal1
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
; Odd count peeled first: one solo block, then all pairs. A pair step
; writes exactly two blocks; the count register reaches zero on the
; bne so no tail test is needed inside the loop.
        tst lr, #1
        beq vxv_r1t_pairs
        stmia r6!, {r0-r3}
        sub lr, lr, #1
        beq vxv_r1t_done
vxv_r1t_pairs
        stmia r6!, {r0-r3}
        stmia r6!, {r0-r3}
        subs lr, lr, #2
        beq vxv_r1t_done2
        stmia r6!, {r0-r3}
        stmia r6!, {r0-r3}
        subs lr, lr, #2
        bne vxv_r1t_pairs
vxv_r1t_done2
vxv_r1t_done
        ldr lr, [sp, #4]
        ldmia ip, {r0,r2}
        mov r1, r0
        mov r3, r2
vxv_repeat1_bottom
        tst lr, #1
        beq vxv_r1b_pairs
        stmia r7!, {r0-r3}
        sub lr, lr, #1
        beq vxv_r1b_done
vxv_r1b_pairs
        stmia r7!, {r0-r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #2
        beq vxv_r1b_done2
        stmia r7!, {r0-r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #2
        bne vxv_r1b_pairs
vxv_r1b_done2
vxv_r1b_done
        cmp r8, #0
        bne vxv_next
        b vxv_rowend
vxv_skip
        add r6, r6, lr, lsl #4
        add r7, r7, lr, lsl #4
        subs r8, r8, lr
        bne vxv_next
        b vxv_rowend
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
        cmp r8, #0
        bne vxv_next
        b vxv_rowend
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
        beq vxv_literal4_done
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
        beq vxv_literal4_done
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
        beq vxv_literal4_done
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
vxv_literal4_done
        cmp r8, #0
        bne vxv_next
        b vxv_rowend
vxv_repeat4
        ldrb lr, [r4], #1
        add lr, lr, #1
        mov ip, lr
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_top
        tst lr, #1
        beq vxv_r4t_pairs
        stmia r6!, {r0-r3}
        sub lr, lr, #1
        beq vxv_repeat4_top_done
vxv_r4t_pairs
        stmia r6!, {r0-r3}
        stmia r6!, {r0-r3}
        subs lr, lr, #2
        bne vxv_r4t_pairs
vxv_repeat4_top_done
        mov lr, ip
        ldrb r0, [r4], #1
        add r0, r5, r0, lsl #3
        ldmia r0, {r0,r1}
        ldrb r2, [r4], #1
        add r2, r5, r2, lsl #3
        ldmia r2, {r2,r3}
vxv_repeat4_bottom
        tst lr, #1
        beq vxv_r4b_pairs
        stmia r7!, {r0-r3}
        sub lr, lr, #1
        beq vxv_repeat4_bottom_done
vxv_r4b_pairs
        stmia r7!, {r0-r3}
        stmia r7!, {r0-r3}
        subs lr, lr, #2
        bne vxv_r4b_pairs
vxv_repeat4_bottom_done
        subs r8, r8, ip
        bne vxv_next
; The row remainder is exhausted. Only this tail reaches vxv_rowend by
; fall-through; every other handler branches here, and all of them arrive
; with r8 already zero.
vxv_rowend
        tst r9, #&80000000
        beq vxv_clean_row
        bic r9, r9, #&80000000
        ldr r0, [sp]
        ldr r1, [sp, #12]
        bic r2, r9, #&c0000000
        sub r1, r1, r2
        tst r9, #&40000000
        strne r1, [r0, #2064]
        bic r9, r9, #&40000000
        add r1, r1, #1
        str r1, [r0, #2068]
vxv_clean_row
        sub r9, r9, #1
        tst r9, #&ff
        beq vxv_ok
        mov r8, r11, lsr #4
        add r6, r6, r11
        add r7, r7, r11
        b vxv_next
vxv_ok
        mov r0, #0
        add sp, sp, #16
        ldmfd sp!, {r4-r11, pc}

; Byte-exact copy for the 2-mod-4 destination case: the C library memcpy
; only has a word path when the destination is word aligned, so the AUD0
; ring write (2-byte aligned half the time) falls back to a byte loop.
; This writes the destination's aligned words instead. With dstA = dst & ~3
; and src word-aligned, out[i] (stored at dstA+4i, i>=1) holds source bytes
; 4i-2..4i+1, i.e. (W(i-1) << 16) | (W(i) >> 16); the first word keeps the
; two bytes already present at dstA. Writes exactly `bytes` bytes and reads
; at most bytes-1, so it never runs past the source piece.
;   r0 = dst (2 mod 4, and dst-2 writable), r1 = src (word aligned),
;   r2 = bytes (>= 8). Returns nothing.
        EXPORT vx_copy_shift2
vx_copy_shift2
        stmfd sp!, {r4-r8, lr}
        bic   r3, r0, #3
        ldr   r4, [r3]
        ldr   r5, [r1], #4
        mov   r6, r4, lsr #16
        mov   r6, r6, lsl #16
        orr   r6, r6, r5, lsr #16
        str   r6, [r3], #4
        sub   r2, r2, #2
        cmp   r2, #8
        blo   vxcs2_tail
vxcs2_loop
        ldr   r8, [r1], #4
        mov   r6, r5, lsl #16
        orr   r6, r6, r8, lsr #16
        str   r6, [r3], #4
        mov   r5, r8
        subs  r2, r2, #4
        cmp   r2, #8
        bhs   vxcs2_loop
vxcs2_tail
        sub   r1, r1, #2
        cmp   r2, #0
        beq   vxcs2_done
vxcs2_byte
        ldrb  r6, [r1], #1
        strb  r6, [r3], #1
        subs  r2, r2, #1
        bne   vxcs2_byte
vxcs2_done
        ldmfd sp!, {r4-r8, pc}
        END
