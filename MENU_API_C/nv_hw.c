#define _POSIX_C_SOURCE 199309L

#include "nv_hw.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

void nv_dsb(void)
{
    __asm__ volatile("dsb sy" ::: "memory");
}

int nv_verbose(void)
{
    const char *e = getenv("NV_VERBOSE");
    return e && e[0] == '1';
}

nv_state_t nv_read_state(const nv_ports_t *ports)
{
    uint32_t raw = *ports->data_out;
    nv_state_t s;

    s.raw      = raw;
    s.digit    = (unsigned)(raw & NV_MASK_DIGIT);
    s.done     = (raw & NV_MASK_DONE)   ? 1 : 0;
    s.n_done   = (raw & NV_MASK_N_DONE) ? 1 : 0;
    s.busy     = (raw & NV_MASK_BUSY)   ? 1 : 0;
    s.error    = (raw & NV_MASK_ERROR)  ? 1 : 0;
    return s;
}

void nv_print_state(FILE *out, const char *tag, nv_state_t s)
{
    fprintf(out,
            "[DATA_OUT] %s raw=0x%08X dig=%u done=%d n_done=%d busy=%d err=%d\n",
            tag, s.raw, s.digit, s.done, s.n_done, s.busy, s.error);
    fflush(out);
}

int nv_mmap_open(nv_ports_t *ports, void **mapped, int *fd_mem)
{
    int fd = open("/dev/mem", O_RDWR | O_SYNC);
    void *m;

    if (fd < 0)
        return -1;

    m = mmap(NULL, NV_LW_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
             (off_t)NV_LW_BASE);
    if (m == MAP_FAILED) {
        close(fd);
        return -2;
    }

    *mapped = m;
    *fd_mem = fd;
    ports->data_in  = (volatile uint32_t *)((uintptr_t)m + NV_DATA_IN_OFF);
    ports->ctrl     = (volatile uint32_t *)((uintptr_t)m + NV_CTRL_OFF);
    ports->data_out = (volatile uint32_t *)((uintptr_t)m + NV_DATA_OUT_OFF);
    return 0;
}

void nv_mmap_close(void *mapped, int fd_mem)
{
    if (mapped && mapped != MAP_FAILED)
        munmap(mapped, NV_LW_SPAN);
    if (fd_mem >= 0)
        close(fd_mem);
}

void nv_hw_wait_idle(nv_ports_t *ports)
{
    while (*ports->data_out & NV_MASK_BUSY)
        ;
}

void nv_hw_clear(nv_ports_t *ports)
{
    nv_hw_wait_idle(ports);
    *ports->ctrl = 2u;
    nv_dsb();
    *ports->ctrl = 0u;
    nv_dsb();
    while (*ports->data_out & NV_MASK_DONE)
        ;
}

void nv_hw_reset(nv_ports_t *ports)
{
    (void)nv_reset(ports);
    nv_dsb();
}
