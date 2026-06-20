#ifndef NV_HW_H
#define NV_HW_H

#include <stdint.h>
#include <stdio.h>

#define NV_LW_BASE        0xFF200000u
#define NV_LW_SPAN        0x00010000u
#define NV_DATA_IN_OFF    0x40u
#define NV_DATA_OUT_OFF   0x50u
#define NV_CTRL_OFF       0x60u

#define NV_IMG_PIXELS     784u

#define NV_MASK_DIGIT     0xFu
#define NV_MASK_DONE      (1u << 4)   /* fl_processor_done */
#define NV_MASK_BUSY      (1u << 5)
#define NV_MASK_ERROR     (1u << 6)
#define NV_MASK_N_DONE    (1u << 7)   /* inference_done rede (se exposto no data_out) */

#define NV_WEIGHT_COUNT   100352u
#define NV_BIAS_COUNT     128u
#define NV_BETA_COUNT     1280u

typedef struct {
    volatile uint32_t *data_in;
    volatile uint32_t *ctrl;
    volatile uint32_t *data_out;
} nv_ports_t;

typedef struct {
    uint32_t raw;
    unsigned digit;
    int done;
    int n_done;
    int busy;
    int error;
} nv_state_t;

typedef struct {
    int rc;
    unsigned digit;
    double infer_ms;
    nv_state_t before;
    nv_state_t after;
} nv_infer_result_t;

nv_state_t nv_read_state(const nv_ports_t *ports);
void nv_print_state(FILE *out, const char *tag, nv_state_t s);

int  nv_mmap_open(nv_ports_t *ports, void **mapped, int *fd_mem);
void nv_mmap_close(void *mapped, int fd_mem);

void nv_dsb(void);
int  nv_reset(nv_ports_t *ports);
void nv_hw_reset(nv_ports_t *ports);
void nv_hw_clear(nv_ports_t *ports);
void nv_hw_wait_idle(nv_ports_t *ports);

int nv_verbose(void);

/* assembly */
int nv_store_img_pixel(nv_ports_t *ports, unsigned pixel, unsigned index);
int nv_weights_send_addr(nv_ports_t *ports, unsigned addr);
int nv_weights_send_value(nv_ports_t *ports, unsigned value_q4_12);
int nv_store_bias(nv_ports_t *ports, unsigned addr, unsigned value16);
int nv_store_beta(nv_ports_t *ports, unsigned addr, unsigned value16);
int nv_inference_start(nv_ports_t *ports);

int nv_model_load(nv_ports_t *ports,
                  const char *path_win,
                  const char *path_bias,
                  const char *path_beta);

int nv_send_image_raw(nv_ports_t *ports, const char *path_raw);
int nv_send_image_buf(nv_ports_t *ports, const uint8_t *buf);

int nv_run_inference(nv_ports_t *ports, nv_infer_result_t *out);

#endif
