#define _POSIX_C_SOURCE 199309L
/*
 * IGNORAR - ARQUIVO UTILIZADO PARA MONITORAMENTO DA MEMORIA PARA TENTAR IDENTIFICAR O ERRO QUE ESTAVAMOS TENDO DO DONE MANTENDO ALTO NA 2 INFERENCIA
 */

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define NV_LW_BASE       0xFF200000u
#define NV_LW_SPAN       0x00010000u
#define NV_DATA_OUT_OFF  0x50u

#define NV_MASK_DIGIT    0xFu
#define NV_MASK_DONE     (1u << 4)
#define NV_MASK_BUSY     (1u << 5)
#define NV_MASK_ERROR    (1u << 6)
#define NV_MASK_N_DONE   (1u << 7)

static void usage(const char *prog)
{
    fprintf(stderr,
            "uso: %s [-w MS] [-n COUNT]\n"
            "\n"
            "  MMIO DATA_OUT @ 0xFF200050 (base LW 0xFF200000)\n"
            "  [3:0] digito  [4] done  [5] busy  [6] err  [7] n_done (rede)\n"
            "\n"
            "  -w MS     intervalo entre leituras (0 = uma vez)\n"
            "  -n COUNT  numero de leituras (0 = infinito)\n",
            prog);
}

static void print_line(volatile uint32_t *data_out, unsigned seq)
{
    uint32_t raw = *data_out;
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    printf("[%4u] %ld.%03ld  raw=0x%08X  dig=%u  done=%d  n_done=%d  busy=%d  err=%d\n",
           seq,
           (long)ts.tv_sec % 100000L,
           (long)(ts.tv_nsec / 1000000L),
           raw,
           (unsigned)(raw & NV_MASK_DIGIT),
           (raw & NV_MASK_DONE)   ? 1 : 0,
           (raw & NV_MASK_N_DONE) ? 1 : 0,
           (raw & NV_MASK_BUSY)   ? 1 : 0,
           (raw & NV_MASK_ERROR)  ? 1 : 0);

    if ((raw & NV_MASK_BUSY) && !(raw & NV_MASK_DONE))
        printf("         -> inferencia/memoria em andamento\n");
    else if (!(raw & NV_MASK_BUSY) && (raw & NV_MASK_DONE))
        printf("         -> operacao concluida (aguardando clr no SW)\n");
    else if (!(raw & NV_MASK_BUSY) && !(raw & NV_MASK_DONE) && (raw & NV_MASK_N_DONE))
        printf("         -> ANOMALIA: n_done=1 sem done SW\n");
    else if (!(raw & NV_MASK_BUSY) && !(raw & NV_MASK_DONE))
        printf("         -> IDLE\n");
    fflush(stdout);
}

int main(int argc, char **argv)
{
    int opt;
    unsigned wait_ms = 0;
    unsigned count = 1;
    unsigned seq = 0;
    int fd;
    void *mapped;
    volatile uint32_t *data_out;

    while ((opt = getopt(argc, argv, "w:n:h")) != -1) {
        switch (opt) {
        case 'w':
            wait_ms = (unsigned)atoi(optarg);
            break;
        case 'n':
            count = (unsigned)atoi(optarg);
            break;
        case 'h':
        default:
            usage(argv[0]);
            return opt == 'h' ? 0 : 1;
        }
    }

    if (wait_ms > 0 && count == 1)
        count = 0;

    fd = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }

    mapped = mmap(NULL, NV_LW_SPAN, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
                  (off_t)NV_LW_BASE);
    if (mapped == MAP_FAILED) {
        perror("mmap");
        close(fd);
        return 1;
    }

    data_out = (volatile uint32_t *)((uintptr_t)mapped + NV_DATA_OUT_OFF);

    printf("nv_peek @ 0x%08X (Ctrl+C para sair)\n",
           (unsigned)(NV_LW_BASE + NV_DATA_OUT_OFF));

    do {
        print_line(data_out, seq++);
        if (count > 0 && seq >= count)
            break;
        if (wait_ms > 0)
            usleep(wait_ms * 1000u);
    } while (wait_ms > 0);

    munmap(mapped, NV_LW_SPAN);
    close(fd);
    return 0;
}
