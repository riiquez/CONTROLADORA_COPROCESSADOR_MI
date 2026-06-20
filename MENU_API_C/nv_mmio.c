#define _POSIX_C_SOURCE 199309L
#include "nv_hw.h"

#include <stdio.h>

#define NV_OPC_STORE_IMG        0u
#define NV_OPC_WEIGHT_ADDR      1u
#define NV_OPC_WEIGHT_VALUE     2u
#define NV_OPC_STORE_BIAS       3u
#define NV_OPC_STORE_BETA       4u
#define NV_OPC_START            5u

static void nv_pulse_enable(nv_ports_t *ports)
{
    *ports->ctrl = 1u;
    nv_dsb();
    *ports->ctrl = 0u;
    nv_dsb();
}

static void nv_pulse_clr(nv_ports_t *ports)
{
    *ports->ctrl = 2u;
    nv_dsb();
    *ports->ctrl = 0u;
    nv_dsb();
}

static void nv_wait_not_busy(nv_ports_t *ports)
{
    while (*ports->data_out & NV_MASK_BUSY)
        ;
}

static void nv_wait_done_low(nv_ports_t *ports)
{
    while (*ports->data_out & NV_MASK_DONE)
        ;
}

static void nv_wait_busy_high(nv_ports_t *ports)
{
    while (!(*ports->data_out & NV_MASK_BUSY))
        ;
}

static int nv_poll_done_or_err(nv_ports_t *ports)
{
    uint32_t st;

    for (;;) {
        st = *ports->data_out;
        if (st & NV_MASK_ERROR)
            return -2;
        if (st & NV_MASK_DONE)
            break;
    }

    nv_pulse_clr(ports);
    return 0;
}

static int nv_issue_and_poll(nv_ports_t *ports, uint32_t data_in, int wait_idle_first)
{
    if (wait_idle_first)
        nv_wait_not_busy(ports);

    *ports->data_in = data_in;
    nv_dsb();
    nv_pulse_enable(ports);
    return nv_poll_done_or_err(ports);
}

/* --- API publica (mesmas funcoes do assembly) --- */

int nv_reset(nv_ports_t *ports)
{
    *ports->ctrl = 4u;
    nv_dsb();
    *ports->ctrl = 0u;
    nv_dsb();
    return 0;
}

int nv_store_img_pixel(nv_ports_t *ports, unsigned pixel, unsigned index)
{
    uint32_t word;

    if (index >= NV_IMG_PIXELS)
        return -3;

    word = (uint32_t)((index << 3) | ((pixel & 0xFFu) << 13) | NV_OPC_STORE_IMG);

    if (nv_verbose())
        printf("[mmio] STORE_IMG idx=%u px=%u word=0x%08X\n", index, pixel, word);

    return nv_issue_and_poll(ports, word, 1);
}

int nv_weights_send_addr(nv_ports_t *ports, unsigned addr)
{
    uint32_t word;

    if (addr >= NV_WEIGHT_COUNT)
        return -3;

    word = (uint32_t)((addr << 3) | NV_OPC_WEIGHT_ADDR);

    *ports->data_in = word;
    nv_dsb();
    nv_pulse_enable(ports);
    return 0;
}

int nv_weights_send_value(nv_ports_t *ports, unsigned value_q4_12)
{
    uint32_t word;

    word = (uint32_t)(((value_q4_12 & 0xFFFFu) << 3) | NV_OPC_WEIGHT_VALUE);

    *ports->data_in = word;
    nv_dsb();
    nv_pulse_enable(ports);
    return nv_poll_done_or_err(ports);
}

int nv_store_bias(nv_ports_t *ports, unsigned addr, unsigned value16)
{
    uint32_t word;

    if (addr >= NV_BIAS_COUNT)
        return -3;

    word = (uint32_t)(((value16 & 0xFFFFu) << 10) | (addr << 3) | NV_OPC_STORE_BIAS);
    return nv_issue_and_poll(ports, word, 0);
}

int nv_store_beta(nv_ports_t *ports, unsigned addr, unsigned value16)
{
    uint32_t word;

    if (addr >= NV_BETA_COUNT)
        return -3;

    word = (uint32_t)(((value16 & 0xFFFFu) << 14) | (addr << 3) | NV_OPC_STORE_BETA);
    return nv_issue_and_poll(ports, word, 0);
}

int nv_inference_start(nv_ports_t *ports)
{
    int rc;

    if (nv_verbose())
        printf("[mmio] inference_start...\n");

    nv_wait_not_busy(ports);
    nv_pulse_clr(ports);
    nv_wait_done_low(ports);

    *ports->data_in = NV_OPC_START;
    nv_dsb();
    nv_pulse_enable(ports);

    nv_wait_busy_high(ports);

    rc = nv_poll_done_or_err(ports);
    if (rc != 0)
        return rc;

    nv_wait_done_low(ports);
    return 0;
}
