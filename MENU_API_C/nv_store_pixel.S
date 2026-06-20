        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .equ MASK_DONE,   (1 << 4)
        .equ MASK_BUSY,   (1 << 5)
        .equ MASK_ERROR,  (1 << 6)

        .text
        .align 2
        .global nv_store_img_pixel
        .type   nv_store_img_pixel, %function

nv_store_img_pixel:
        push    {r4-r8, lr}

        ldr     r4, [r0, #0]
        ldr     r5, [r0, #4]
        ldr     r6, [r0, #8]

        mov     r7, r1
        mov     r8, r2

        mov     r3, #0
        mov     r12, r7
        lsl     r12, r12, #13
        orr     r3, r3, r12
        mov     r12, r8
        lsl     r12, r12, #3
        orr     r3, r3, r12

.Lwait_idle:
        ldr     r12, [r6]
        tst     r12, #MASK_BUSY
        bne     .Lwait_idle

        str     r3, [r4]

        mov     r12, #1
        str     r12, [r5]
        dsb     sy
        mov     r12, #0
        str     r12, [r5]
        dsb     sy

.Lpoll:
        ldr     r12, [r6]
        tst     r12, #MASK_ERROR
        bne     .Lhw_err
        tst     r12, #MASK_DONE
        beq     .Lpoll

        mov     r12, #2
        str     r12, [r5]
        dsb     sy
        mov     r12, #0
        str     r12, [r5]
        dsb     sy

        mov     r0, #0
        pop     {r4-r8, pc}

.Lhw_err:
        mov     r0, #-2
        pop     {r4-r8, pc}

        .size   nv_store_img_pixel, .-nv_store_img_pixel
