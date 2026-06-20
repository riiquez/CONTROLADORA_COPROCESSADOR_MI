        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .equ OPCODE_START, 5
        .equ MASK_DONE,    (1 << 4)
        .equ MASK_BUSY,    (1 << 5)
        .equ MASK_ERROR,   (1 << 6)

        .text
        .align 2
        .global nv_inference_start
        .type   nv_inference_start, %function

nv_inference_start:
        push    {r4-r6, lr}

        ldr     r4, [r0, #0]
        ldr     r5, [r0, #4]
        ldr     r6, [r0, #8]

.Lidle:
        ldr     r12, [r6]
        tst     r12, #MASK_BUSY
        bne     .Lidle

        mov     r12, #2
        str     r12, [r5]
        dsb     sy
        mov     r12, #0
        str     r12, [r5]
        dsb     sy

.Lwait_done_low:
        ldr     r12, [r6]
        tst     r12, #MASK_DONE
        bne     .Lwait_done_low

        mov     r3, #OPCODE_START
        str     r3, [r4]
        dsb     sy

        mov     r12, #1
        str     r12, [r5]
        dsb     sy
        mov     r12, #0
        str     r12, [r5]
        dsb     sy

.Lwait_busy_high:
        ldr     r12, [r6]
        tst     r12, #MASK_BUSY
        beq     .Lwait_busy_high

.Lpoll:
        ldr     r12, [r6]
        tst     r12, #MASK_ERROR
        bne     .Lerr
        tst     r12, #MASK_DONE
        beq     .Lpoll

        mov     r12, #2
        str     r12, [r5]
        dsb     sy
        mov     r12, #0
        str     r12, [r5]
        dsb     sy

.Lwait_done_clear:
        ldr     r12, [r6]
        tst     r12, #MASK_DONE
        bne     .Lwait_done_clear

        mov     r0, #0
        pop     {r4-r6, pc}

.Lerr:
        mov     r0, #-2
        pop     {r4-r6, pc}

        .size   nv_inference_start, .-nv_inference_start
