#ifndef NV_PNG_IMG_H
#define NV_PNG_IMG_H

#include <stdint.h>

#include "nv_hw.h"

/*
 * PNG -> vetor uint8_t[784] (28x28 cinza, mesma ordem do .raw / STORE_IMG).
 *
 * nv_load_image_png()  : so decodifica o arquivo para o buffer.
 * nv_send_image_png()  : decodifica + envia ao coprocessador (igual opcao 1 com .raw).
 */

int nv_load_image_png(const char *path, uint8_t *buf);

int nv_send_image_png(nv_ports_t *ports, const char *path);

#endif
