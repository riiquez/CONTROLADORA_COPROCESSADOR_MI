        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .equ OPCODE_STORE_BIAS, 3
        .equ BIAS_ENTRIES,      128
        .equ MASK_DONE,         (1 << 4)
        .equ MASK_ERROR,        (1 << 6)

        .text
        .align 2
        .global nv_store_bias
        .type   nv_store_bias, %function

nv_store_bias:
        push    {r4-r6, lr}
        cmp     r1, #BIAS_ENTRIES
        bhs     .Lbad

        ldr     r4, [r0, #0]
        ldr     r5, [r0, #4]
        ldr     r6, [r0, #8]

        mov     r3, #OPCODE_STORE_BIAS
        orr     r3, r3, r1, lsl #3
        uxth    r12, r2
        orr     r3, r3, r12, lsl #10

        str     r3, [r4]
        mov     r12, #1
        str     r12, [r5]
        mov     r12, #0
        str     r12, [r5]

.Lpoll:
        ldr     r12, [r6]
        tst     r12, #MASK_ERROR
        bne     .Lerr
        tst     r12, #MASK_DONE
        beq     .Lpoll

        mov     r12, #2
        str     r12, [r5]
        mov     r12, #0
        str     r12, [r5]

        mov     r0, #0
        pop     {r4-r6, pc}

.Lerr:
        mov     r0, #-2
        pop     {r4-r6, pc}
.Lbad:
        mov     r0, #-3
        pop     {r4-r6, pc}
        .size   nv_store_bias, .-nv_store_bias
