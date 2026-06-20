#define _GNU_SOURCE
#include "mouse_draw.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define DRAW_SCALE        VGA_DEFAULT_SCALE
#define DRAW_PEN_VALUE    254u   /* pico tipico MNIST (.raw usa ~0xFE) */
#define DRAW_BRUSH_RADIUS 1u     /* raio em celulas 28x28 (traco mais grosso, estilo MNIST) */
#define MNIST_TARGET_BOX  20u    /* escala minima para desenhos muito pequenos */
#define MNIST_CENTER_MIN  8u     /* abaixo disso: redimensiona; acima: so centraliza */
#define CURSOR_RADIUS     6      /* raio da bola em pixels VGA */
#define CURSOR_RING       2      /* espessura do anel vermelho */
#define BORDER_GRAY       64u
#define POLL_TIMEOUT_MS   50

typedef struct {
    int mx;
    int my;
    int left_down;
    int cursor_visible;
    int cursor_x;
    int cursor_y;
} mouse_state_t;

static int mouse_has_rel_axis(int fd)
{
    unsigned char rel_bits[(REL_MAX + 1) / 8 + 1];

    memset(rel_bits, 0, sizeof(rel_bits));
    if (ioctl(fd, EVIOCGBIT(EV_REL, sizeof(rel_bits)), rel_bits) < 0)
        return 0;

    return (rel_bits[REL_X / 8] >> (REL_X % 8)) & 1;
}

static int open_mouse_device(const char *input_dev)
{
    if (input_dev && input_dev[0] != '\0')
        return open(input_dev, O_RDONLY | O_NONBLOCK);

    const char *env = getenv("MOUSE_DEV");
    if (env && env[0] != '\0') {
        int fd = open(env, O_RDONLY | O_NONBLOCK);
        if (fd >= 0)
            return fd;
    }

    char path[64];
    for (int i = 0; i < 16; i++) {
        snprintf(path, sizeof(path), "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY | O_NONBLOCK);
        if (fd < 0)
            continue;

        if (mouse_has_rel_axis(fd))
            return fd;

        close(fd);
    }

    return -1;
}

static void canvas_origin(unsigned *x0, unsigned *y0)
{
    const unsigned draw_w = IMG_W * DRAW_SCALE;
    const unsigned draw_h = IMG_H * DRAW_SCALE;
    *x0 = (VGA_WIDTH  - draw_w) / 2u;
    *y0 = (VGA_HEIGHT - draw_h) / 2u;
}

static unsigned canvas_index(unsigned row, unsigned col)
{
    /* Mesma ordem dos .raw: indice = linha * 28 + coluna (row-major, topo-esquerda) */
    return row * IMG_W + col;
}

static int vga_to_canvas(int mx, int my, unsigned *col, unsigned *row)
{
    unsigned x0, y0;
    canvas_origin(&x0, &y0);

    if (mx < (int)x0 || my < (int)y0)
        return -1;

    unsigned lx = (unsigned)(mx - (int)x0);
    unsigned ly = (unsigned)(my - (int)y0);
    unsigned draw_w = IMG_W * DRAW_SCALE;
    unsigned draw_h = IMG_H * DRAW_SCALE;

    if (lx >= draw_w || ly >= draw_h)
        return -1;

    *col = lx / DRAW_SCALE;
    *row = ly / DRAW_SCALE;
    return 0;
}

static void paint_cell(vga_ports_t *vga, uint8_t *canvas,
                       unsigned col, unsigned row)
{
    if (col >= IMG_W || row >= IMG_H)
        return;

    unsigned idx = canvas_index(row, col);
    if (canvas[idx] == DRAW_PEN_VALUE)
        return;

    canvas[idx] = (uint8_t)DRAW_PEN_VALUE;
    vga_draw_cell(vga, col, row, (uint8_t)DRAW_PEN_VALUE, DRAW_SCALE);
}

static void paint_brush(vga_ports_t *vga, uint8_t *canvas,
                        unsigned col, unsigned row)
{
    for (int dy = -(int)DRAW_BRUSH_RADIUS; dy <= (int)DRAW_BRUSH_RADIUS; dy++) {
        for (int dx = -(int)DRAW_BRUSH_RADIUS; dx <= (int)DRAW_BRUSH_RADIUS; dx++) {
            int c = (int)col + dx;
            int r = (int)row + dy;
            if (c >= 0 && r >= 0)
                paint_cell(vga, canvas, (unsigned)c, (unsigned)r);
        }
    }
}

static void paint_line(vga_ports_t *vga, uint8_t *canvas,
                       int c0, int r0, int c1, int r1)
{
    int dc = (c1 > c0) ? 1 : ((c1 < c0) ? -1 : 0);
    int dr = (r1 > r0) ? 1 : ((r1 < r0) ? -1 : 0);
    int x = c0;
    int y = r0;

    while (x != c1 || y != r1) {
        if (x >= 0 && y >= 0)
            paint_brush(vga, canvas, (unsigned)x, (unsigned)y);

        if (x != c1)
            x += dc;
        if (y != r1)
            y += dr;
    }

    if (c1 >= 0 && r1 >= 0)
        paint_brush(vga, canvas, (unsigned)c1, (unsigned)r1);
}

static void clamp_mouse_to_canvas(mouse_state_t *st)
{
    unsigned x0, y0;
    canvas_origin(&x0, &y0);

    int max_x = (int)(x0 + IMG_W * DRAW_SCALE - 1u);
    int max_y = (int)(y0 + IMG_H * DRAW_SCALE - 1u);

    if (st->mx < (int)x0)
        st->mx = (int)x0;
    if (st->my < (int)y0)
        st->my = (int)y0;
    if (st->mx > max_x)
        st->mx = max_x;
    if (st->my > max_y)
        st->my = max_y;
}

static void paint_at_vga_pos(vga_ports_t *vga, uint8_t *canvas,
                             mouse_state_t *st, int last_col, int last_row)
{
    unsigned col, row;

    if (vga_to_canvas(st->mx, st->my, &col, &row) != 0)
        return;

    if (last_col >= 0 && last_row >= 0 &&
        ((int)col != last_col || (int)row != last_row)) {
        paint_line(vga, canvas, last_col, last_row, (int)col, (int)row);
    } else {
        paint_brush(vga, canvas, col, row);
    }
}

static void update_last_cell(mouse_state_t *st, int *last_col, int *last_row)
{
    unsigned col, row;

    if (vga_to_canvas(st->mx, st->my, &col, &row) == 0) {
        *last_col = (int)col;
        *last_row = (int)row;
    }
}

static uint8_t canvas_gray_at_vga(const uint8_t *canvas, int vx, int vy)
{
    unsigned col, row;

    if (vga_to_canvas(vx, vy, &col, &row) != 0)
        return 0;

    uint8_t gray = canvas[canvas_index(row, col)];
    if (gray == 0 &&
        (row == 0 || row == IMG_H - 1 || col == 0 || col == IMG_W - 1)) {
        gray = BORDER_GRAY;
    }

    return gray;
}

static void vga_restore_pixel(vga_ports_t *vga, const uint8_t *canvas,
                              int vx, int vy)
{
    uint8_t gray = canvas_gray_at_vga(canvas, vx, vy);
    unsigned c3 = (unsigned)(gray >> 5);

    vga_write_pixel(vga, (unsigned)vx, (unsigned)vy, c3, c3, c3);
}

static void cursor_erase(vga_ports_t *vga, const uint8_t *canvas,
                         int cx, int cy)
{
    const int r = CURSOR_RADIUS + 1;

    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            int vx = cx + dx;
            int vy = cy + dy;

            if (vx < 0 || vy < 0 ||
                vx >= (int)VGA_WIDTH || vy >= (int)VGA_HEIGHT)
                continue;

            if ((dx * dx + dy * dy) > (r * r))
                continue;

            vga_restore_pixel(vga, canvas, vx, vy);
        }
    }
}

static void cursor_draw_ball(vga_ports_t *vga, int cx, int cy)
{
    const int r_out = CURSOR_RADIUS;
    const int r_in  = CURSOR_RADIUS - CURSOR_RING;
    const int r_in_sq = (r_in > 0) ? (r_in * r_in) : 0;
    const int r_out_sq = r_out * r_out;

    for (int dy = -r_out; dy <= r_out; dy++) {
        for (int dx = -r_out; dx <= r_out; dx++) {
            int d2 = dx * dx + dy * dy;
            int vx = cx + dx;
            int vy = cy + dy;

            if (d2 > r_out_sq || vx < 0 || vy < 0 ||
                vx >= (int)VGA_WIDTH || vy >= (int)VGA_HEIGHT)
                continue;

            if (d2 >= r_in_sq) {
                /* anel vermelho — nao altera o buffer do canvas */
                vga_write_pixel(vga, (unsigned)vx, (unsigned)vy, 7, 0, 0);
            }
        }
    }
}

static void cursor_hide(vga_ports_t *vga, const uint8_t *canvas,
                        mouse_state_t *st)
{
    if (!st->cursor_visible)
        return;

    cursor_erase(vga, canvas, st->cursor_x, st->cursor_y);
    st->cursor_visible = 0;
}

static void cursor_update(vga_ports_t *vga, const uint8_t *canvas,
                          mouse_state_t *st)
{
    if (st->left_down) {
        cursor_hide(vga, canvas, st);
        return;
    }

    if (st->cursor_visible &&
        (st->cursor_x != st->mx || st->cursor_y != st->my)) {
        cursor_erase(vga, canvas, st->cursor_x, st->cursor_y);
        st->cursor_visible = 0;
    }

    if (!st->cursor_visible) {
        cursor_draw_ball(vga, st->mx, st->my);
        st->cursor_x = st->mx;
        st->cursor_y = st->my;
        st->cursor_visible = 1;
    }
}

static int canvas_find_bbox(const uint8_t *canvas,
                            unsigned *min_row, unsigned *max_row,
                            unsigned *min_col, unsigned *max_col)
{
    int found = 0;
    unsigned r0 = IMG_H, c0 = IMG_W, r1 = 0, c1 = 0;

    for (unsigned row = 0; row < IMG_H; row++) {
        for (unsigned col = 0; col < IMG_W; col++) {
            if (canvas[canvas_index(row, col)] == 0)
                continue;

            found = 1;
            if (row < r0) r0 = row;
            if (row > r1) r1 = row;
            if (col < c0) c0 = col;
            if (col > c1) c1 = col;
        }
    }

    if (!found)
        return 0;

    *min_row = r0;
    *max_row = r1;
    *min_col = c0;
    *max_col = c1;
    return 1;
}

static void canvas_center_and_scale(uint8_t *canvas)
{
    unsigned min_row, max_row, min_col, max_col;
    uint8_t tmp[IMG_PIXELS];

    if (!canvas_find_bbox(canvas, &min_row, &max_row, &min_col, &max_col))
        return;

    unsigned box_h = max_row - min_row + 1u;
    unsigned box_w = max_col - min_col + 1u;
    unsigned src_dim = (box_h > box_w) ? box_h : box_w;
    if (src_dim == 0)
        return;

    unsigned dst_dim = MNIST_TARGET_BOX;
    if (src_dim >= dst_dim)
        dst_dim = src_dim;

    int offset_row = (int)((IMG_H - dst_dim) / 2u);
    int offset_col = (int)((IMG_W - dst_dim) / 2u);

    memset(tmp, 0, IMG_PIXELS);

    for (unsigned row = min_row; row <= max_row; row++) {
        for (unsigned col = min_col; col <= max_col; col++) {
            if (canvas[canvas_index(row, col)] == 0)
                continue;

            unsigned rel_r = row - min_row;
            unsigned rel_c = col - min_col;
            unsigned scaled_r = (rel_r * dst_dim) / src_dim;
            unsigned scaled_c = (rel_c * dst_dim) / src_dim;
            int dst_r = offset_row + (int)scaled_r;
            int dst_c = offset_col + (int)scaled_c;

            if (dst_r >= 0 && dst_c >= 0 &&
                (unsigned)dst_r < IMG_H && (unsigned)dst_c < IMG_W) {
                tmp[canvas_index((unsigned)dst_r, (unsigned)dst_c)] =
                    (uint8_t)DRAW_PEN_VALUE;
            }
        }
    }

    memcpy(canvas, tmp, IMG_PIXELS);
}

/* Centraliza o desenho sem redimensionar (preserva espessura do traco). */
static void canvas_center_translate(uint8_t *canvas)
{
    unsigned min_row, max_row, min_col, max_col;
    uint8_t tmp[IMG_PIXELS];
    unsigned box_h, box_w;
    int off_r, off_c;

    if (!canvas_find_bbox(canvas, &min_row, &max_row, &min_col, &max_col))
        return;

    box_h = max_row - min_row + 1u;
    box_w = max_col - min_col + 1u;
    off_r = (int)((IMG_H - box_h) / 2u) - (int)min_row;
    off_c = (int)((IMG_W - box_w) / 2u) - (int)min_col;

    memset(tmp, 0, IMG_PIXELS);

    for (unsigned row = min_row; row <= max_row; row++) {
        for (unsigned col = min_col; col <= max_col; col++) {
            if (canvas[canvas_index(row, col)] == 0)
                continue;
            {
                int dr = (int)row + off_r;
                int dc = (int)col + off_c;
                if (dr >= 0 && dc >= 0 &&
                    (unsigned)dr < IMG_H && (unsigned)dc < IMG_W) {
                    tmp[canvas_index((unsigned)dr, (unsigned)dc)] =
                        (uint8_t)DRAW_PEN_VALUE;
                }
            }
        }
    }

    memcpy(canvas, tmp, IMG_PIXELS);
}

static void canvas_dilate_3x3(uint8_t *canvas)
{
    uint8_t tmp[IMG_PIXELS];
    unsigned row, col;

    memcpy(tmp, canvas, IMG_PIXELS);

    for (row = 0; row < IMG_H; row++) {
        for (col = 0; col < IMG_W; col++) {
            if (tmp[canvas_index(row, col)] == 0)
                continue;

            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int r = (int)row + dy;
                    int c = (int)col + dx;
                    if (r >= 0 && c >= 0 &&
                        (unsigned)r < IMG_H && (unsigned)c < IMG_W) {
                        canvas[canvas_index((unsigned)r, (unsigned)c)] =
                            (uint8_t)DRAW_PEN_VALUE;
                    }
                }
            }
        }
    }
}

static int draw_use_legacy_prepare(void)
{
    const char *e = getenv("MOUSE_DRAW_LEGACY");
    return e && (e[0] == '1' || e[0] == 'y' || e[0] == 'Y');
}

static int draw_use_dilate(void)
{
    const char *e = getenv("MOUSE_DRAW_DILATE");
    return e && (e[0] == '1' || e[0] == 'y' || e[0] == 'Y');
}

void mouse_canvas_print_ascii(FILE *out, const uint8_t *canvas)
{
    static const char lut[] = " .:-=+#%@";

    fprintf(out, "\n========== Imagem enviada ao coprocessador (28x28) ==========\n");
    for (unsigned row = 0; row < IMG_H; row++) {
        fputc('|', out);
        for (unsigned col = 0; col < IMG_W; col++) {
            unsigned v = canvas[canvas_index(row, col)];
            unsigned idx = (v * 7u) / 255u;
            fputc(lut[idx], out);
        }
        fputs("|\n", out);
    }
    fputs("=============================================================\n", out);
    fflush(out);
}

void mouse_canvas_prepare_for_elm(uint8_t *canvas)
{
    unsigned min_row, max_row, min_col, max_col;
    unsigned box_h = 0, box_w = 0, box_max = 0;

    /* Fundo 0, traco 0xFE — igual aos .raw de treino */
    for (unsigned i = 0; i < IMG_PIXELS; i++)
        canvas[i] = (canvas[i] > 0) ? (uint8_t)DRAW_PEN_VALUE : 0;

    if (draw_use_legacy_prepare()) {
        canvas_center_and_scale(canvas);
        if (draw_use_dilate())
            canvas_dilate_3x3(canvas);
        return;
    }

    /*
     * Padrao (menos agressivo que o antigo):
     * - desenho medio/grande: so centraliza, sem encolher para 20x20
     * - desenho minusculo: escala para MNIST_TARGET_BOX
     * - dilatacao 3x3 desligada (MOUSE_DRAW_DILATE=1 para ligar)
     */
    if (canvas_find_bbox(canvas, &min_row, &max_row, &min_col, &max_col)) {
        box_h = max_row - min_row + 1u;
        box_w = max_col - min_col + 1u;
        box_max = (box_h > box_w) ? box_h : box_w;

        if (box_max < MNIST_CENTER_MIN)
            canvas_center_and_scale(canvas);
        else
            canvas_center_translate(canvas);
    }

    if (draw_use_dilate())
        canvas_dilate_3x3(canvas);
}

static void draw_canvas_border(vga_ports_t *vga, uint8_t *canvas)
{
    for (unsigned col = 0; col < IMG_W; col++) {
        vga_draw_cell(vga, col, 0, BORDER_GRAY, DRAW_SCALE);
        vga_draw_cell(vga, col, IMG_H - 1, BORDER_GRAY, DRAW_SCALE);
    }
    for (unsigned row = 0; row < IMG_H; row++) {
        vga_draw_cell(vga, 0, row, BORDER_GRAY, DRAW_SCALE);
        vga_draw_cell(vga, IMG_W - 1, row, BORDER_GRAY, DRAW_SCALE);
    }

    (void)canvas;
}

static unsigned count_nonzero(const uint8_t *canvas)
{
    unsigned n = 0;
    for (unsigned i = 0; i < IMG_PIXELS; i++) {
        if (canvas[i] > 0)
            n++;
    }
    return n;
}

int mouse_draw_canvas(vga_ports_t *vga, uint8_t *out_pixels,
                      const char *input_dev)
{
    int mouse_fd = open_mouse_device(input_dev);
    if (mouse_fd < 0)
        return -1;

    memset(out_pixels, 0, IMG_PIXELS);

    printf("\n");
    printf("========================================\n");
    printf("  MODO DESENHO (28x28)\n");
    printf("========================================\n");
    printf("  Anel vermelho              : posicao do cursor\n");
    printf("  Botao ESQUERDO + movimento : pintar\n");
    printf("  Botao DIREITO ou Enter     : finalizar\n");
    printf("========================================\n");
    printf("Olhe o monitor VGA e desenhe o digito.\n\n");
    fflush(stdout);

    vga_clear_canvas_area(vga, DRAW_SCALE);
    draw_canvas_border(vga, out_pixels);

    unsigned x0, y0;
    canvas_origin(&x0, &y0);

    mouse_state_t st = {
        .mx = (int)(x0 + (IMG_W * DRAW_SCALE) / 2u),
        .my = (int)(y0 + (IMG_H * DRAW_SCALE) / 2u),
        .left_down = 0,
        .cursor_visible = 0,
        .cursor_x = 0,
        .cursor_y = 0,
    };

    cursor_update(vga, out_pixels, &st);

    int last_col = -1;
    int last_row = -1;
    int finished = 0;

    struct pollfd fds[2];
    fds[0].fd = mouse_fd;
    fds[0].events = POLLIN;
    fds[1].fd = STDIN_FILENO;
    fds[1].events = POLLIN;

    while (!finished) {
        int pr = poll(fds, 2, POLL_TIMEOUT_MS);
        if (pr < 0) {
            if (errno == EINTR)
                continue;
            close(mouse_fd);
            return -2;
        }

        if (fds[1].revents & POLLIN) {
            char c;
            ssize_t n = read(STDIN_FILENO, &c, 1);
            if (n > 0 && (c == '\n' || c == '\r'))
                finished = 1;
        }

        if (!(fds[0].revents & POLLIN))
            continue;

        struct input_event ev;
        while (read(mouse_fd, &ev, sizeof(ev)) == (ssize_t)sizeof(ev)) {
            if (ev.type == EV_REL && ev.code == REL_X)
                st.mx += ev.value;
            else if (ev.type == EV_REL && ev.code == REL_Y)
                st.my += ev.value;
            else if (ev.type == EV_KEY && ev.code == BTN_LEFT) {
                if (ev.value) {
                    cursor_hide(vga, out_pixels, &st);
                    last_col = last_row = -1;
                    st.left_down = 1;
                    clamp_mouse_to_canvas(&st);
                    paint_at_vga_pos(vga, out_pixels, &st, last_col, last_row);
                    update_last_cell(&st, &last_col, &last_row);
                } else {
                    st.left_down = 0;
                }
            } else if (ev.type == EV_KEY && ev.code == BTN_RIGHT && ev.value) {
                finished = 1;
            }

            clamp_mouse_to_canvas(&st);

            if (ev.type == EV_SYN) {
                if (st.left_down) {
                    paint_at_vga_pos(vga, out_pixels, &st, last_col, last_row);
                    update_last_cell(&st, &last_col, &last_row);
                } else {
                    cursor_update(vga, out_pixels, &st);
                }
            }

            if (finished)
                break;
        }
    }

    cursor_hide(vga, out_pixels, &st);
    close(mouse_fd);

    mouse_canvas_prepare_for_elm(out_pixels);

    printf("Desenho finalizado (%u pixels ativos).\n", count_nonzero(out_pixels));
    printf("Preview do buffer que vai para a inferencia:\n");
    mouse_canvas_print_ascii(stdout, out_pixels);
    printf("(MOUSE_DRAW_LEGACY=1 restaura escala 20x20+dilatacao antiga)\n");
    printf("Atualizando VGA a partir do buffer enviado a inferencia...\n");
    vga_show_image(vga, out_pixels, DRAW_SCALE);

    return 0;
}
