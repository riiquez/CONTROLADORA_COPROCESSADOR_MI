        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .equ OPCODE_WEIGHT_ADDR,  1
        .equ OPCODE_WEIGHT_VALUE, 2
        .equ WIN_MAX_ENTRIES,    100352
        .equ MASK_DONE,          (1 << 4)
        .equ MASK_ERROR,         (1 << 6)

        .text
        .align 2
        .global nv_weights_send_addr
        .type   nv_weights_send_addr, %function

nv_weights_send_addr:
        push    {r4-r8, lr}
        ldr     r12, =WIN_MAX_ENTRIES
        cmp     r1, r12
        bhs     .Lbad_addr

        ldr     r4, [r0, #0]
        ldr     r5, [r0, #4]

        mov     r3, OPCODE_WEIGHT_ADDR
        mov     r12, r1
        lsl     r12, r12, #3
        orr     r3, r3, r12

        str     r3, [r4]
        mov     r12, #1
        str     r12, [r5]
        mov     r12, #0
        str     r12, [r5]

        mov     r0, #0
        pop     {r4-r8, pc}

.Lbad_addr:
        mov     r0, #-3
        pop     {r4-r8, pc}
        .size   nv_weights_send_addr, .-nv_weights_send_addr

        .global nv_weights_send_value
        .type   nv_weights_send_value, %function

nv_weights_send_value:
        push    {r4-r8, lr}
        ldr     r4, [r0, #0]
        ldr     r5, [r0, #4]
        ldr     r6, [r0, #8]

        mov     r3, OPCODE_WEIGHT_VALUE
        uxth    r12, r1
        lsl     r12, r12, #3
        orr     r3, r3, r12

        str     r3, [r4]
        mov     r12, #1
        str     r12, [r5]
        mov     r12, #0
        str     r12, [r5]

.Lpollw:
        ldr     r12, [r6]
        tst     r12, #MASK_ERROR
        bne     .Lhw_err_w
        tst     r12, #MASK_DONE
        beq     .Lpollw

        mov     r12, #2
        str     r12, [r5]
        mov     r12, #0
        str     r12, [r5]

        mov     r0, #0
        pop     {r4-r8, pc}

.Lhw_err_w:
        mov     r0, #-2
        pop     {r4-r8, pc}
        .size   nv_weights_send_value, .-nv_weights_send_value
