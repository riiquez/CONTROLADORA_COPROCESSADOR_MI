#ifndef VGA_DRIVER_H
#define VGA_DRIVER_H

#include <stdint.h>

#define VGA_DATA_OFF     0x70u
#define VGA_CTRL_OFF     0x80u
#define VGA_STATUS_OFF   0x90u

#define VGA_WIDTH        320u
#define VGA_HEIGHT       240u
#define IMG_W            28u
#define IMG_H            28u
#define IMG_PIXELS       (IMG_W * IMG_H)

#define VGA_CTRL_ENABLE  (1u << 0)
#define VGA_STATUS_DONE  (1u << 0)
#define VGA_DEFAULT_SCALE 4u

typedef struct {
    volatile uint32_t *data;
    volatile uint32_t *ctrl;
    volatile uint32_t *status;
} vga_ports_t;

static inline void vga_init_from_map(vga_ports_t *vga, void *mapped)
{
    uintptr_t base = (uintptr_t)mapped;
    vga->data   = (volatile uint32_t *)(base + VGA_DATA_OFF);
    vga->ctrl   = (volatile uint32_t *)(base + VGA_CTRL_OFF);
    vga->status = (volatile uint32_t *)(base + VGA_STATUS_OFF);
}

static inline uint32_t vga_pack_pixel(unsigned x, unsigned y,
                                      unsigned r3, unsigned g3, unsigned b3)
{
    return (x & 0x1FFu)
         | ((y & 0xFFu) << 9)
         | ((r3 & 7u) << 17)
         | ((g3 & 7u) << 20)
         | ((b3 & 7u) << 23);
}

static inline void vga_write_pixel(vga_ports_t *vga, unsigned x, unsigned y,
                                   unsigned r3, unsigned g3, unsigned b3)
{
    *vga->data = vga_pack_pixel(x, y, r3, g3, b3);
    *vga->ctrl = 0;

    int timeout = 10000000;
    *vga->ctrl = VGA_CTRL_ENABLE;

    while (((*vga->status & VGA_STATUS_DONE) == 0) && timeout > 0)
        timeout--;

    *vga->ctrl = 0;
}

static inline void vga_clear_screen(vga_ports_t *vga)
{
    for (unsigned y = 0; y < VGA_HEIGHT; y++) {
        for (unsigned x = 0; x < VGA_WIDTH; x++)
            vga_write_pixel(vga, x, y, 0, 0, 0);
    }
}

static inline void vga_draw_cell(vga_ports_t *vga, unsigned col, unsigned row,
                                 uint8_t gray, unsigned scale)
{
    const unsigned draw_w = IMG_W * scale;
    const unsigned draw_h = IMG_H * scale;
    const unsigned x0 = (VGA_WIDTH  - draw_w) / 2u;
    const unsigned y0 = (VGA_HEIGHT - draw_h) / 2u;
    const unsigned c3 = (unsigned)(gray >> 5);

    for (unsigned sy = 0; sy < scale; sy++) {
        for (unsigned sx = 0; sx < scale; sx++) {
            vga_write_pixel(vga,
                            x0 + col * scale + sx,
                            y0 + row * scale + sy,
                            c3, c3, c3);
        }
    }
}

static inline void vga_clear_canvas_area(vga_ports_t *vga, unsigned scale)
{
    const unsigned draw_w = IMG_W * scale;
    const unsigned draw_h = IMG_H * scale;
    const unsigned x0 = (VGA_WIDTH  - draw_w) / 2u;
    const unsigned y0 = (VGA_HEIGHT - draw_h) / 2u;

    for (unsigned y = y0; y < y0 + draw_h; y++) {
        for (unsigned x = x0; x < x0 + draw_w; x++)
            vga_write_pixel(vga, x, y, 0, 0, 0);
    }
}

static inline void vga_draw_image(vga_ports_t *vga, const uint8_t *pixels,
                                  unsigned scale)
{
    const unsigned draw_w = IMG_W * scale;
    const unsigned draw_h = IMG_H * scale;
    const unsigned x0 = (VGA_WIDTH  - draw_w) / 2u;
    const unsigned y0 = (VGA_HEIGHT - draw_h) / 2u;

    for (unsigned row = 0; row < IMG_H; row++) {
        for (unsigned col = 0; col < IMG_W; col++) {
            uint8_t gray = pixels[row * IMG_W + col];
            unsigned c3 = (unsigned)(gray >> 5);

            for (unsigned sy = 0; sy < scale; sy++) {
                for (unsigned sx = 0; sx < scale; sx++) {
                    vga_write_pixel(vga,
                                    x0 + col * scale + sx,
                                    y0 + row * scale + sy,
                                    c3, c3, c3);
                }
            }
        }
    }
}

static inline void vga_show_image(vga_ports_t *vga, const uint8_t *pixels,
                                  unsigned scale)
{
    vga_clear_screen(vga);
    vga_draw_image(vga, pixels, scale);
}

#endif
