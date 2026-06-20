/*
 * vga_display.c — exibe imagem .raw (28x28, 1 byte/pixel) no monitor VGA da DE1-SoC.
 *
 * Compilar na placa (SSH):
 *   gcc -O2 -Wall -o vga_display vga_display.c
 *
 * Executar (root necessario para /dev/mem):
 *   sudo ./vga_display imagem_1.raw
 *
 * Mapa MMIO (Lightweight Bridge, base 0xFF200000):
 *   vga_data   +0x70  — posicao (x,y) + cor RGB 3-bit
 *   vga_ctrl   +0x74  — bit0 = enable (pulso para escrever pixel)
 *   vga_status +0x78  — bit0 = done (leitura)
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define LW_BASE          0xFF200000u
#define LW_SPAN          0x00010000u

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

static volatile uint32_t *vga_data;
static volatile uint32_t *vga_ctrl;
static volatile uint32_t *vga_status;

static uint32_t pack_pixel_data(unsigned x, unsigned y,
                                unsigned r3, unsigned g3, unsigned b3)
{
    return (x & 0x1FFu)
         | ((y & 0xFFu) << 9)
         | ((r3 & 7u) << 17)
         | ((g3 & 7u) << 20)
         | ((b3 & 7u) << 23);
}

static void vga_write_pixel(unsigned x, unsigned y,
                            unsigned r3, unsigned g3, unsigned b3)
{
    uint32_t data;

    printf("A: preparando pixel (%u,%u)\n", x, y);
    fflush(stdout);

    data = pack_pixel_data(x, y, r3, g3, b3);

    printf("B: data = 0x%08x\n", data);
    fflush(stdout);

    *vga_data = data;

    printf("C: escreveu DATA\n");
    fflush(stdout);

    printf("D: status antes = 0x%08x\n", *vga_status);
    fflush(stdout);

    printf("E: ctrl antes = 0x%08x\n", *vga_ctrl);
    fflush(stdout);

    *vga_ctrl = 0;

    printf("F: ctrl=0\n");
    fflush(stdout);

    *vga_ctrl = VGA_CTRL_ENABLE;

    printf("G: ctrl=ENABLE\n");
    fflush(stdout);

    printf("H: status depois enable = 0x%08x\n", *vga_status);
    fflush(stdout);

    int timeout = 10000000;

    while (((*vga_status & VGA_STATUS_DONE) == 0) && timeout > 0) {
        timeout--;
    }

    printf("I: saiu do while\n");
    fflush(stdout);

    printf("J: timeout=%d status=0x%08x\n",
           timeout, *vga_status);
    fflush(stdout);

    *vga_ctrl = 0;

    printf("K: ctrl limpo\n");
    fflush(stdout);
}

static int mmap_vga(void **mapped, int *fd_mem)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0)
        return -1;

    void *m = mmap(NULL, LW_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                   (off_t)LW_BASE);
    if (m == MAP_FAILED) {
        close(fd);
        return -2;
    }

    uintptr_t base = (uintptr_t)m;
    vga_data   = (volatile uint32_t *)(base + VGA_DATA_OFF);
    vga_ctrl   = (volatile uint32_t *)(base + VGA_CTRL_OFF);
    vga_status = (volatile uint32_t *)(base + VGA_STATUS_OFF);

    *mapped = m;
    *fd_mem = fd;
    return 0;
}

static void clear_screen(void)
{
    printf("inicio \n");
    for (unsigned y = 0; y < VGA_HEIGHT; y++) {
        printf("print1 \n");
       
    
        for (unsigned x = 0; x < VGA_WIDTH; x++){
            printf("print2\n");
            vga_write_pixel(x, y, 0, 0, 0);
            }
    }
}

static int load_raw(const char *path, uint8_t *buf)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0)
        return -1;

    ssize_t n = read(fd, buf, IMG_PIXELS);
    close(fd);

    if (n != (ssize_t)IMG_PIXELS)
        return -2;

    return 0;
}

static void draw_image(const uint8_t *pixels, unsigned scale)
{
    const unsigned draw_w = IMG_W * scale;
    const unsigned draw_h = IMG_H * scale;
    const unsigned x0 = (VGA_WIDTH  - draw_w) / 2u;
    const unsigned y0 = (VGA_HEIGHT - draw_h) / 2u;

    for (unsigned row = 0; row < IMG_H; row++) {
        for (unsigned col = 0; col < IMG_W; col++) {
            uint8_t gray = pixels[row * IMG_W + col];
            unsigned c3 = (unsigned)(gray >> 5); /* 8-bit -> 3-bit */

            for (unsigned sy = 0; sy < scale; sy++) {
                for (unsigned sx = 0; sx < scale; sx++) {
                    vga_write_pixel(x0 + col * scale + sx,
                                    y0 + row * scale + sy,
                                    c3, c3, c3);
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "uso: %s <imagem.raw> [escala]\n", argv[0]);
        fprintf(stderr, "  imagem.raw: 784 bytes (28x28, escala de cinza)\n");
        fprintf(stderr, "  escala:     fator de ampliacao (padrao 4)\n");
        return 1;
    }

    unsigned scale = 4u;
    if (argc >= 3) {
        unsigned long s = strtoul(argv[2], NULL, 0);
        if (s == 0 || s > 16) {
            fprintf(stderr, "escala invalida (use 1..16)\n");
            return 1;
        }
        scale = (unsigned)s;
    }

    uint8_t pixels[IMG_PIXELS];
    if (load_raw(argv[1], pixels) != 0) {
        perror("falha ao ler imagem .raw");
        return 1;
    }

    void *mapped = MAP_FAILED;
    int fd_mem = -1;

    if (mmap_vga(&mapped, &fd_mem) != 0) {
        fprintf(stderr, "mmap falhou (precisa root?): %s\n", strerror(errno));
        return 1;
    }

    printf("Limpando tela VGA (%ux%u)...\n", VGA_WIDTH, VGA_HEIGHT);
    clear_screen();
    printf("acabou limpeza");

    printf("Desenhando %s (escala %u)...\n", argv[1], scale);
    draw_image(pixels, scale);

    printf("Pronto. Imagem exibida no monitor VGA.\n");

    munmap(mapped, LW_SPAN);
    close(fd_mem);
    return 0;
}
