@ aprendizado_registos.s
@
@ So registos + um exemplo de montar UMA palavra de 32 bits (formato estilo CoProcessor).
@ Opcode 001 nos bits [2:0]; exemplo de endereco nos bits [19:3] (como STORE_WEIGTHS_ADDR).
@ Nao enviamos a palavra a nenhum porto — fica guardada num registo (r4).
@
@ No fim: b fim_demo -> unico sitio que chama exit.

        .syntax unified
        .arch armv7-a
        .cpu cortex-a9

        .global _start
        .section .text
        .align 2

_start:
        @ --- Passo 0: ideia ---------------------------------------------------------
        @ Uma instrucao de 32 bits e so um numero. Os "campos" sao pedacos de bits
        @ que o hardware do coprocessor interpreta (opcode, endereco, ...).
        @ Nos juntamos pedacos com deslocamentos (lsl) e OR (orr), como em C:
        @   palavra = (campo_alto << pos) | (campo_baixo).

        @ --- Passo 1: opcode = 001 nos tres bits menos significativos [2:0] ----------
        @ Em binario "001" (3 bits) e o numero 1 em decimal.
        @ Primeiro pomos esse valor na palavra; depois acrescentamos outros campos com OR.

        mov     r4, #1               @ r4 = ...000001  -> bits [2:0] = 001

        @ --- Passo 2: exemplo de endereco no campo [19:3] (17 bits no RTL) -------
        @ O protocolo diz: o endereco nao fica em [19:0] solto; comeca no bit 3.
        @ Logo multiplicamos o endereco por 2^3 = 8, ou seja: endereco << 3.
        @ Exemplo: endereco decimal 42 -> ocupa bits 22..3 quando somamos ao opcode.

        mov     r3, #42              @ valor de exemplo para o subcampo endereco
        lsl     r3, r3, #3           @ r3 = 42 << 3  (alinha bit 3 como “bit 0” do campo)

        @ --- Passo 3: juntar sem apagar uns com os outros -------------------------
        @ OR com mascara nos bits em que os campos nao se sobrepoe:
        @   bits [2:0]  vem do r4 (opcode)
        @   bits [19:3] vem do r3 (ja deslocados)
        @ OR mete 1 onde qualquer um dos operandos tem 1.

        orr     r4, r4, r3           @ r4 = palavra 32-bit montada (resto em zero)

        @ r4 e a instrucao completa neste exemplo; podes inspecionar com gdb: x/w $r4

        b       fim_demo             @ vai para o unico encerramento limpo

fim_demo:
        mov     r0, #0               @ codigo de saida 0
        mov     r7, #1               @ syscall exit (Linux ARM)
        svc     #0
