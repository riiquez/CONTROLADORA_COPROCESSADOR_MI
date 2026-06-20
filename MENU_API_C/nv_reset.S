        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .text
        .align 2
        .global nv_reset
        .type   nv_reset, %function

nv_reset:
        push    {r4, lr}
        ldr     r4, [r0, #4]
        mov     r1, #4
        str     r1, [r4]
        dsb     sy
        mov     r1, #0
        str     r1, [r4]
        dsb     sy
        mov     r0, #0
        pop     {r4, pc}
        .size   nv_reset, .-nv_reset
