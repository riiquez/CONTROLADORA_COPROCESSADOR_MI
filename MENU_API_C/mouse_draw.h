#ifndef MOUSE_DRAW_H
#define MOUSE_DRAW_H

#include <stdio.h>
#include <stdint.h>

#include "vga_driver.h"

/**
 * Modo interativo: desenha no canvas 28x28 com o mouse e atualiza o VGA ao vivo.
 *
 * Controles:
 *   - botao esquerdo + movimento: pinta (branco)
 *   - botao direito OU Enter no terminal: finaliza o desenho
 *
 * input_dev: caminho evdev (ex. "/dev/input/event1") ou NULL para auto-detectar.
 * out_pixels: buffer de 784 bytes preenchido ao finalizar.
 */
int mouse_draw_canvas(vga_ports_t *vga, uint8_t *out_pixels,
                      const char *input_dev);

void mouse_canvas_prepare_for_elm(uint8_t *canvas);

/** Imprime no terminal o buffer 28x28 que sera enviado ao coprocessador. */
void mouse_canvas_print_ascii(FILE *out, const uint8_t *canvas);

#endif
