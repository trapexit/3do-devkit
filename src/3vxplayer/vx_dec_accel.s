;
; vx_dec_accel.s - ARMv3 fast block painters for the 3VX decoder.
;
; Framebuffer = 16bpp LR (vertical line-pair interleave): one framebuffer
; word covers pixel (even line, col j) in the high halfword and pixel
; (odd line, col j) in the low halfword. A 4x4 block occupies two
; rowpair bands of 4 consecutive words each, at word pointers rp0/rp1.
;
; Codebooks are pre-shattered to LR-word form at load time:
;   V1 entry (4 words): {tl,tr,bl,br} each = color|color<<16
;       block rp0 = {tl,tl,tr,tr}, rp1 = {bl,bl,br,br}
;   V4 entry (2 words): {t0<<16|t1, t2<<16|t3}
;       block rp0 = {e0w0,e0w1,e1w0,e1w1}; rp1 likewise from e2,e3
;
; Run painters paint n consecutive blocks; the C dispatcher guarantees a
; run never crosses a block-row boundary, so the band pointers simply
; advance 16 bytes per block. APCS: a1-a4 args (5th arg on stack).
;

        AREA    |C$$code|, CODE, READONLY

        EXPORT  vx_put_v1_asm
        EXPORT  vx_put_v4_asm
        EXPORT  vx_v4_run_asm
        EXPORT  vx_v1_run_asm
        EXPORT  vx_v4rep_run_asm
        EXPORT  vx_v1rep_run_asm

; void vx_put_v1_asm(uint32 *rp0, uint32 *rp1, const uint32 *tile4)
vx_put_v1_asm
        ldmia   a3!, {a4, ip}           ; tl, tr   (a3 += 8)
        str     a4, [a1, #0]
        str     a4, [a1, #4]
        str     ip, [a1, #8]
        str     ip, [a1, #12]
        ldmia   a3, {a4, ip}            ; bl, br
        str     a4, [a2, #0]
        str     a4, [a2, #4]
        str     ip, [a2, #8]
        str     ip, [a2, #12]
        mov     pc, lr

; void vx_put_v4_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
;                    const uint8 *idx4)
vx_put_v4_asm
        stmfd   sp!, {v1, v2, lr}
        ldrb    v1, [a4, #0]            ; e0
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    ip, [a4, #1]            ; e1
        add     ip, a3, ip, lsl #3
        ldmia   ip, {ip, lr}
        stmia   a1, {v1, v2, ip, lr}    ; rp0 = {e0w0,e0w1,e1w0,e1w1}
        ldrb    v1, [a4, #2]            ; e2
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    ip, [a4, #3]            ; e3
        add     ip, a3, ip, lsl #3
        ldmia   ip, {ip, lr}
        stmia   a2, {v1, v2, ip, lr}    ; rp1 = {e2w0,e2w1,e3w0,e3w1}
        ldmfd   sp!, {v1, v2, pc}

; ---------------------------------------------------------------------------
; void vx_v4_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
;                    const uint8 *idxlist, uint32 n)
; idxlist = 4 bytes (e0..e3) per block
; ---------------------------------------------------------------------------
vx_v4_run_asm
        stmfd   sp!, {v1, v2, v3, v4, lr}
        ldr     v4, [sp, #5*4]          ; n (5th arg below 5 saved regs)
        cmp     v4, #0
        beq     vx4r_done
vx4r_loop
        ldrb    v1, [a4, #0]            ; e0
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    v3, [a4, #1]            ; e1
        add     v3, a3, v3, lsl #3
        ldmia   v3, {v3, ip}
        stmia   a1!, {v1, v2, v3, ip}   ; rp0 4 words
        ldrb    v1, [a4, #2]            ; e2
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    v3, [a4, #3]            ; e3
        add     v3, a3, v3, lsl #3
        ldmia   v3, {v3, ip}
        stmia   a2!, {v1, v2, v3, ip}   ; rp1 4 words
        add     a4, a4, #4
        subs    v4, v4, #1
        bne     vx4r_loop
vx4r_done
        ldmfd   sp!, {v1, v2, v3, v4, pc}

; ---------------------------------------------------------------------------
; void vx_v1_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *v1cb,
;                    const uint8 *idxlist, uint32 n)
; idxlist = 1 byte per block; tile = {tl,tr,bl,br} words per entry
;   rp0 = {tl,tl,tr,tr}, rp1 = {bl,bl,br,br}
; ---------------------------------------------------------------------------
vx_v1_run_asm
        stmfd   sp!, {v1, v2, v3, v4, lr}
        ldr     v4, [sp, #5*4]          ; n
        cmp     v4, #0
        beq     vx1r_done
vx1r_loop
        ldrb    v1, [a4]
        add     v1, a3, v1, lsl #4      ; entry addr = cb + idx*16
        ldmia   v1, {v1, v2, v3, ip}    ; tl, tr, bl, br
        ; rp0 = {tl,tl,tr,tr}
        str     v1, [a1], #4
        str     v1, [a1], #4
        str     v2, [a1], #4
        str     v2, [a1], #4
        ; rp1 = {bl,bl,br,br}
        str     v3, [a2], #4
        str     v3, [a2], #4
        str     ip, [a2], #4
        str     ip, [a2], #4
        add     a4, a4, #1
        subs    v4, v4, #1
        bne     vx1r_loop
vx1r_done
        ldmfd   sp!, {v1, v2, v3, v4, pc}


; ---------------------------------------------------------------------------
; void vx_v1rep_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *v1cb,
;                       const uint8 *idx, uint32 n)
; Repeat painter: one V1 index painted into n consecutive blocks; the
; C dispatcher guarantees the run never crosses a block-row boundary, so
; the band pointers simply advance 16 bytes per block. Same LR-packed
; word order as vx_v1_run_asm: rp0 = {tl,tl,tr,tr}, rp1 = {bl,bl,br,br}.
; ---------------------------------------------------------------------------
vx_v1rep_run_asm
        stmfd   sp!, {v1, v2, v3, v4, lr}
        ldrb    a4, [a4, #0]            ; single index, repeated n times
        add     a4, a3, a4, lsl #4      ; entry addr = cb + idx*16
        ldmia   a4, {v1, v2, v3, ip}    ; tl, tr, bl, br
        ldr     v4, [sp, #5*4]          ; n (5th arg below 5 saved regs)
        cmp     v4, #0
        beq     vx1rr_done
vx1rr_loop
        ; rp0 = {tl,tl,tr,tr}
        str     v1, [a1], #4
        str     v1, [a1], #4
        str     v2, [a1], #4
        str     v2, [a1], #4
        ; rp1 = {bl,bl,br,br}
        str     v3, [a2], #4
        str     v3, [a2], #4
        str     ip, [a2], #4
        str     ip, [a2], #4
        subs    v4, v4, #1
        bne     vx1rr_loop
vx1rr_done
        ldmfd   sp!, {v1, v2, v3, v4, pc}

; ---------------------------------------------------------------------------
; void vx_v4rep_run_asm(uint32 *rp0, uint32 *rp1, const uint32 *cb,
;                       const uint8 *entry, uint32 n)
; Repeat painter: the V4 tile selected by the 4 index bytes at entry
; painted into n consecutive blocks. entry is 4 codebook indices
; (e0..e3), exactly like one block of vx_v4_run_asm's idxlist; the tile
; is resolved through the codebook once, then painted n times with the
; same advance and word order:
;   rp0 = {e0w0,e0w1,e1w0,e1w1}; rp1 = {e2w0,e2w1,e3w0,e3w1}
; ---------------------------------------------------------------------------
vx_v4rep_run_asm
        stmfd   sp!, {v1, v2, v3, v4, lr}
        ldrb    v1, [a4, #0]            ; e0
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    v3, [a4, #1]            ; e1
        add     v3, a3, v3, lsl #3
        ldmia   v3, {v3, ip}
        ldr     v4, [sp, #5*4]          ; n (5th arg below 5 saved regs)
        cmp     v4, #0
        beq     vx4rr_done
vx4rr_loop0
        stmia   a1!, {v1, v2, v3, ip}   ; rp0 4 words
        subs    v4, v4, #1
        bne     vx4rr_loop0
        ldrb    v1, [a4, #2]            ; e2
        add     v1, a3, v1, lsl #3
        ldmia   v1, {v1, v2}
        ldrb    v3, [a4, #3]            ; e3
        add     v3, a3, v3, lsl #3
        ldmia   v3, {v3, ip}
        ldr     v4, [sp, #5*4]          ; n
vx4rr_loop1
        stmia   a2!, {v1, v2, v3, ip}   ; rp1 4 words
        subs    v4, v4, #1
        bne     vx4rr_loop1
vx4rr_done
        ldmfd   sp!, {v1, v2, v3, v4, pc}

        END

