@ test_mmio_memory.s — mesmo modelo que test_opcodes.s (syscall + mmap /dev/mem).
@
@ Demonstra UM acesso MMIO aos PIOs DATA_IN / CTRL e le DATA_OUT uma vez ao fim,
@ gravando UM pixel na mem_img do CoProcessor (instrucao STORE_IMG).
@
@ Encoding STORE_IMG (CoProcessor.v, ramo STORE_IMG):
@   [2:0]    OPCODE_STORE_IMG (3'b000)
@   [12:3]   endereco (10 bits, valido se < 784)
@   [20:13] byte do pixel (8 bits)
@   PALAVRA = (pixel << 13) | (endereco << 3)
@
@ Build na placa (exemplo):
@   as -o test_mmio_memory.o test_mmio_memory.s
@   ld -static -o test_mmio_memory test_mmio_memory.o
@   sudo ./test_mmio_memory
@
@ Nota: o bitstream tem de ter DATA_IN na ponte em 0x40, CTRL 0x60, DATA_OUT 0x50.

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
        .equ DATA_OUT_OFF,      0x50
        .equ CTRL_OFF,          0x60

        .equ OPCODE_STORE_IMG,  0               @ bits [2:0] iguais ao RTL
        .equ ELM_SIG_ENABLE,    1

        @ valores de exemplo (altera aqui para outro pixel/endereco)
        .equ IMGIDX,               0               @ mudar IMGIDX (>255?): usar "ldr r1, =IMGIDX"
        .equ PIXEL_TST,           0x42

        .section .rodata
dev_mem:
        .asciz "/dev/mem"

        .section .text
        .global _start
        .align 2

_start:
        ldr     r0, =dev_mem
        ldr     r1, =(O_RDWR | O_SYNC)
        mov     r2, #0
        mov     r7, #__NR_open
        svc     #0
        cmp     r0, #0
        blt     .Lfail_early
        mov     r8, r0

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
        mov     r10, r0

        add     r6, r10, #DATA_IN_OFF
        add     r9, r10, #CTRL_OFF
        add     r11, r10, #DATA_OUT_OFF

        @ Palavra STORE_IMG
        mov     r0, #PIXEL_TST
        lsl     r0, r0, #13
        mov     r1, #IMGIDX
        lsl     r1, r1, #3
        orr     r0, r0, r1
        orr     r0, r0, #OPCODE_STORE_IMG
        str     r0, [r6]

        @ Pulso enable
        mov     r0, #ELM_SIG_ENABLE
        str     r0, [r9]
        mov     r0, #0
        str     r0, [r9]

        @ Le barramento FPGA (valor em data_out apos STORE_IMG — ver readme data_out / flags)
        ldr     r2, [r11]

        mov     r0, r8                 @ fd para close
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
