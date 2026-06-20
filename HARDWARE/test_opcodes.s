@ test_opcodes.s — Linux embarcado em ARM (Cortex-A9). Só assembly + syscalls.
@
@ Mapeia a ponte leve (0xFF200000), escreve em PIO data_in (offset 0x40) e ctrl (offset 0x60),
@ com atraso entre opcodes para conseguir ver LEDs se não usar latch no RTL.
@
@ Montagem na placa (exemplo):
@   as -o test_opcodes.o test_opcodes.s
@   ld -static -o test_opcodes test_opcodes.o
@   sudo ./test_opcodes
@
@ Offsets DATA_IN 0x40, CTRL 0x60 = bytes na ponte leve — iguais a Marco2-driver/hps_0.h.
@ O assembler NÃO inclui hps_0.h: os números estão copiados aqui (.equ). Se mudares
@ o Qsys, altera estes .equ OU gera header e sincroniza à mão.

        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .equ __NR_exit,   1
        .equ __NR_open,   5
        .equ __NR_close,  6
        .equ __NR_mmap2,  192

        .equ O_RDWR,      0x0002
        .equ O_SYNC,      0x00100000

        .equ PROT_RW,     3
        .equ MAP_SHARED,  1

        .equ LW_BASE,           0xFF200000
        .equ LW_SPAN,           0x00010000
        .equ DATA_IN_OFF,       0x40
        .equ CTRL_OFF,         0x60

        .equ ELM_SIG_ENABLE,   1

        .section .rodata
dev_mem:
        .asciz "/dev/mem"

        .section .text
        .global _start
        .align 2

_start:
        @ open("/dev/mem", O_RDWR|O_SYNC, 0)
        ldr     r0, =dev_mem
        ldr     r1, =(O_RDWR | O_SYNC)
        mov     r2, #0
        mov     r7, #__NR_open
        svc     #0
        cmp     r0, #0
        blt     .Lfail_early
        mov     r8, r0                 @ fd

        @ mmap2(0, span, PROT_RW, MAP_SHARED, fd, pgoff)
        mov     r0, #0
        ldr     r1, =LW_SPAN
        mov     r2, #PROT_RW
        mov     r3, #MAP_SHARED
        mov     r4, r8
        ldr     r5, =(LW_BASE >> 12)
        mov     r7, #__NR_mmap2
        svc     #0
        mvn     r1, #0
        cmp     r0, r1
        beq     .Lfail_mmap
        mov     r10, r0                @ virtual base

        ldr     r4, =DATA_IN_OFF
        ldr     r5, =CTRL_OFF
        add     r6, r10, r4            @ ptr data_in
        add     r9, r10, r5            @ ptr ctrl

        @ r11 = opcode 0..7
        mov     r11, #0
.Lopcode_loop:
        @ data_in = opcode nos bits [2:0]
        str     r11, [r6]

        @ ctrl = enable
        mov     r0, #ELM_SIG_ENABLE
        str     r0, [r9]

        @ pequeno atraso
        ldr     r0, =8000000
        bl      delay_r0

        @ ctrl = 0
        mov     r0, #0
        str     r0, [r9]

        ldr     r0, =4000000
        bl      delay_r0

        add     r11, r11, #1
        cmp     r11, #8
        bne     .Lopcode_loop

        mov     r0, r8
        mov     r7, #__NR_close
        svc     #0

        mov     r0, #0
        mov     r7, #__NR_exit
        svc     #0

.Lfail_mmap:
        mov     r0, r8
        mov     r7, #__NR_close
        svc     #0
.Lfail_early:
        mov     r0, #1
        mov     r7, #__NR_exit
        svc     #0

@ delay_r0: decrementa r0 até 0
delay_r0:
        subs    r0, r0, #1
        bne     delay_r0
        bx      lr
