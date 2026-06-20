#define _POSIX_C_SOURCE 199309L

#include "nv_hw.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static double ms_between(const struct timespec *a, const struct timespec *b)
{
    int64_t s = (int64_t)b->tv_sec - (int64_t)a->tv_sec;
    int64_t ns = (int64_t)b->tv_nsec - (int64_t)a->tv_nsec;
    return (double)(s * 1000000000LL + ns) / 1e6;
}

int nv_send_image_buf(nv_ports_t *ports, const uint8_t *buf)
{
    nv_hw_clear(ports);

    if (nv_verbose())
        printf("[nv] enviando 784 pixels...\n");

    for (unsigned i = 0; i < NV_IMG_PIXELS; i++) {
        int rc = nv_store_img_pixel(ports, buf[i], i);
        if (rc != 0)
            return -10 - (int)i;
    }

    nv_hw_clear(ports);
    return 0;
}

int nv_send_image_raw(nv_ports_t *ports, const char *path_raw)
{
    uint8_t buf[NV_IMG_PIXELS];
    int fd = open(path_raw, O_RDONLY);
    ssize_t n;

    if (fd < 0)
        return -1;

    n = read(fd, buf, NV_IMG_PIXELS);
    close(fd);

    if (n != (ssize_t)NV_IMG_PIXELS)
        return -2;

    return nv_send_image_buf(ports, buf);
}

int nv_run_inference(nv_ports_t *ports, nv_infer_result_t *out)
{
    struct timespec t0, t1;
    int rc;

    memset(out, 0, sizeof(*out));
    out->before = nv_read_state(ports);

    if (nv_verbose()) {
        printf("[nv] reset hardware (ctrl bit2)...\n");
        nv_print_state(stdout, "antes reset", out->before);
    }

    /*
     * Protocolo do colega: reset antes de START limpa FSM da rede/coprocessador.
     * clr so limpa fl_processor_done; reset zera registradores e estado neural.
     */
    nv_hw_reset(ports);
    nv_hw_clear(ports);

    if (nv_verbose())
        nv_print_state(stdout, "apos reset+clr", nv_read_state(ports));

    clock_gettime(CLOCK_MONOTONIC, &t0);
    rc = nv_inference_start(ports);
    clock_gettime(CLOCK_MONOTONIC, &t1);

    out->rc = rc;
    out->infer_ms = ms_between(&t0, &t1);
    out->after = nv_read_state(ports);
    out->digit = out->after.digit;

    if (nv_verbose())
        nv_print_state(stdout, "apos inferencia", out->after);

    return rc;
}

int nv_model_load(nv_ports_t *ports,
                  const char *path_win,
                  const char *path_bias,
                  const char *path_beta)
{
    int fd;
    ssize_t n;
    void *blob;
    unsigned addr;
    int e;

    fd = open(path_win, O_RDONLY);
    if (fd < 0)
        return -20;
    blob = malloc(NV_WEIGHT_COUNT * sizeof(uint16_t));
    if (!blob) {
        close(fd);
        return -21;
    }
    n = read(fd, blob, NV_WEIGHT_COUNT * sizeof(uint16_t));
    close(fd);
    if ((size_t)n != NV_WEIGHT_COUNT * sizeof(uint16_t)) {
        free(blob);
        return -22;
    }
    {
        const uint16_t *w = (const uint16_t *)blob;
        for (addr = 0; addr < NV_WEIGHT_COUNT; addr++) {
            e = nv_weights_send_addr(ports, addr);
            if (e != 0) {
                free(blob);
                return -30 - (int)addr;
            }
            nv_hw_wait_idle(ports);
            e = nv_weights_send_value(ports, (unsigned)w[addr]);
            if (e != 0) {
                free(blob);
                return -40 - (int)addr;
            }
        }
    }
    free(blob);

    fd = open(path_bias, O_RDONLY);
    if (fd < 0)
        return -71;
    blob = malloc(NV_BIAS_COUNT * sizeof(uint16_t));
    if (!blob) {
        close(fd);
        return -72;
    }
    n = read(fd, blob, NV_BIAS_COUNT * sizeof(uint16_t));
    close(fd);
    if ((size_t)n != NV_BIAS_COUNT * sizeof(uint16_t)) {
        free(blob);
        return -73;
    }
    {
        const uint16_t *b = (const uint16_t *)blob;
        for (addr = 0; addr < NV_BIAS_COUNT; addr++) {
            e = nv_store_bias(ports, addr, (unsigned)b[addr]);
            if (e != 0) {
                free(blob);
                return -74 - (int)addr;
            }
        }
    }
    free(blob);

    fd = open(path_beta, O_RDONLY);
    if (fd < 0)
        return -50;
    blob = malloc(NV_BETA_COUNT * sizeof(uint16_t));
    if (!blob) {
        close(fd);
        return -51;
    }
    n = read(fd, blob, NV_BETA_COUNT * sizeof(uint16_t));
    close(fd);
    if ((size_t)n != NV_BETA_COUNT * sizeof(uint16_t)) {
        free(blob);
        return -52;
    }
    {
        const uint16_t *bt = (const uint16_t *)blob;
        for (addr = 0; addr < NV_BETA_COUNT; addr++) {
            e = nv_store_beta(ports, addr, (unsigned)bt[addr]);
            if (e != 0) {
                free(blob);
                return -53 - (int)addr;
            }
        }
    }
    free(blob);

    nv_hw_reset(ports);
    nv_hw_clear(ports);
    return 0;
}
