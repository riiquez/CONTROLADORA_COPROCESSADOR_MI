#define _POSIX_C_SOURCE 199309L

#include "nv_png_img.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define NV_IMG_W 28
#define NV_IMG_H 28

static uint8_t sample_bilinear(const uint8_t *src, int sw, int sh,
                               float fx, float fy)
{
    int x0, y0, x1, y1;
    float dx, dy;
    float v00, v10, v01, v11;
    float v0, v1;
    int v;

    if (fx < 0.0f)
        fx = 0.0f;
    if (fy < 0.0f)
        fy = 0.0f;
    if (fx > (float)(sw - 1))
        fx = (float)(sw - 1);
    if (fy > (float)(sh - 1))
        fy = (float)(sh - 1);

    x0 = (int)floorf(fx);
    y0 = (int)floorf(fy);
    x1 = (x0 + 1 < sw) ? x0 + 1 : x0;
    y1 = (y0 + 1 < sh) ? y0 + 1 : y0;
    dx = fx - (float)x0;
    dy = fy - (float)y0;

    v00 = (float)src[y0 * sw + x0];
    v10 = (float)src[y0 * sw + x1];
    v01 = (float)src[y1 * sw + x0];
    v11 = (float)src[y1 * sw + x1];
    v0 = v00 * (1.0f - dx) + v10 * dx;
    v1 = v01 * (1.0f - dx) + v11 * dx;
    v = (int)floorf(v0 * (1.0f - dy) + v1 * dy + 0.5f);

    if (v < 0)
        v = 0;
    if (v > 255)
        v = 255;
    return (uint8_t)v;
}

static void resize_grey_to_28x28(const uint8_t *src, int sw, int sh,
                                 uint8_t *dst)
{
    unsigned y, x;

    for (y = 0; y < NV_IMG_H; y++) {
        for (x = 0; x < NV_IMG_W; x++) {
            float fx = ((float)x + 0.5f) * (float)sw / (float)NV_IMG_W - 0.5f;
            float fy = ((float)y + 0.5f) * (float)sh / (float)NV_IMG_H - 0.5f;
            dst[y * NV_IMG_W + x] = sample_bilinear(src, sw, sh, fx, fy);
        }
    }
}

int nv_load_image_png(const char *path, uint8_t *buf)
{
    int w, h, comp;
    unsigned char *data;

    /* Decodifica PNG -> cinza; preenche buf[784] (row-major, igual .raw). */
    data = stbi_load(path, &w, &h, &comp, 1);
    if (!data) {
        if (nv_verbose())
            fprintf(stderr, "[png] stbi_load falhou '%s': %s\n",
                    path, stbi_failure_reason());
        return -1;
    }

    if (w == NV_IMG_W && h == NV_IMG_H) {
        memcpy(buf, data, NV_IMG_PIXELS);
    } else {
        if (nv_verbose())
            fprintf(stderr,
                    "[png] redimensionando %dx%d -> 28x28 ('%s')\n",
                    w, h, path);
        resize_grey_to_28x28(data, w, h, buf);
    }

    stbi_image_free(data);
    return 0;
}

int nv_send_image_png(nv_ports_t *ports, const char *path)
{
    uint8_t buf[NV_IMG_PIXELS];

    if (nv_load_image_png(path, buf) != 0)
        return -1;

    /* Mesmo caminho da opcao 1: 784 pixels via STORE_IMG. */
    return nv_send_image_buf(ports, buf);
}
