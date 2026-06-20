#define _POSIX_C_SOURCE 199309L

#include "nv_hw.h"

#include "nv_testbranch.h"

#include "mouse_draw.h"

#include "vga_driver.h"



#include <fcntl.h>

#include <stdio.h>

#include <string.h>

#include <sys/mman.h>

#include <unistd.h>



#define NV_PATH_WIN   "W_in.raw"

#define NV_PATH_BIAS  "b.raw"

#define NV_PATH_BETA  "beta.raw"



static const char *const NV_TEST_SEQ[] = {

    "imagem_3.raw",

    "imagem_3.raw",

    "imagem_1.raw",

    "imagem_1.raw",

};

#define NV_TEST_COUNT 4u



static void print_menu(void)

{

    printf("\n========================================\n");

    printf("  NV_APP — coprocessador ELM (api_nova)\n");

    printf("========================================\n");

    printf("  1 - Enviar imagem .raw + inferencia\n");

    printf("  2 - Teste 4x (imagem_3, imagem_3, imagem_1, imagem_1)\n");

    printf("  3 - Desenhar digito (mouse + VGA)\n");

    printf("  4 - Testbranch (testes/0..9, 100 .png/pasta -> logs.csv)\n");

    printf("========================================\n");

    printf("  Outro SSH: sudo ./nv_peek -w 5\n");

    printf("  Verbose:    export NV_VERBOSE=1\n");

    printf("Opcao: ");

    fflush(stdout);

}



static int read_option(void)

{

    char line[16];

    if (!fgets(line, sizeof(line), stdin))

        return -1;

    if (line[0] < '1' || line[0] > '4')

        return -1;

    if (line[1] != '\0' && line[1] != '\n')

        return -1;

    return line[0] - '0';

}



static int read_path(char *buf, size_t n)

{

    printf("Arquivo .raw (784 bytes): ");

    fflush(stdout);

    if (!fgets(buf, (int)n, stdin))

        return -1;

    size_t len = strlen(buf);

    if (len > 0 && buf[len - 1] == '\n')

        buf[len - 1] = '\0';

    if (buf[0] == '\0')

        return -1;

    return 0;

}



static int load_raw_image(const char *path, uint8_t *buf)
{
    int fd = open(path, O_RDONLY);
    ssize_t n;

    if (fd < 0)
        return -1;

    n = read(fd, buf, NV_IMG_PIXELS);
    close(fd);

    if (n != (ssize_t)NV_IMG_PIXELS)
        return -2;

    return 0;
}

static int save_raw_image(const char *path, const uint8_t *buf)

{

    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    ssize_t n;



    if (fd < 0)

        return -1;



    n = write(fd, buf, NV_IMG_PIXELS);

    close(fd);

    return (n == (ssize_t)NV_IMG_PIXELS) ? 0 : -2;

}



static void print_infer_line(unsigned idx, unsigned total,

                             const char *path, const nv_infer_result_t *r)

{

    const char *status;



    if (r->rc != 0)

        status = "ERRO_HW";

    else if (r->infer_ms < 5.0)

        status = "RAPIDO_DEMAIS?";

    else

        status = "OK";



    printf("[%u/%u] %-24s digito=%u  %.2f ms  %s  (done=%d busy=%d err=%d)\n",

           idx, total, path, r->digit, r->infer_ms, status,

           r->after.done, r->after.busy, r->after.error);

    fflush(stdout);

}



static int run_one(nv_ports_t *ports, vga_ports_t *vga, const char *path,
                   unsigned idx, unsigned total)
{
    uint8_t pixels[NV_IMG_PIXELS];
    nv_infer_result_t r;
    int rc;

    if (nv_verbose())
        printf("\n[nv] --- ciclo %u/%u: %s ---\n", idx, total, path);

    rc = load_raw_image(path, pixels);
    if (rc != 0) {
        fprintf(stderr, "[%u/%u] falha leitura '%s' (cod %d)\n", idx, total, path, rc);
        return rc;
    }

    printf("Exibindo imagem no VGA...\n");
    fflush(stdout);
    vga_show_image(vga, pixels, VGA_DEFAULT_SCALE);

    rc = nv_send_image_buf(ports, pixels);
    if (rc != 0) {
        fprintf(stderr, "[%u/%u] falha envio '%s' (cod %d)\n", idx, total, path, rc);
        return rc;
    }

    rc = nv_run_inference(ports, &r);
    print_infer_line(idx, total, path, &r);
    return rc;
}

static void mode_send_one(nv_ports_t *ports, vga_ports_t *vga)
{
    char path[256];

    if (read_path(path, sizeof(path)) != 0) {
        fputs("Entrada invalida.\n", stderr);
        return;
    }

    run_one(ports, vga, path, 1u, 1u);
}



static void mode_test_four(nv_ports_t *ports)

{

    unsigned i;

    unsigned ok = 0;



    printf("\n[nv] Teste automatico 4x iniciado.\n");

    fflush(stdout);



    for (i = 0; i < NV_TEST_COUNT; i++) {

        nv_infer_result_t r;

        int rc = nv_send_image_raw(ports, NV_TEST_SEQ[i]);

        if (rc != 0) {

            fprintf(stderr, "[%u/4] envio falhou '%s' cod=%d\n",

                    i + 1u, NV_TEST_SEQ[i], rc);

            break;

        }

        rc = nv_run_inference(ports, &r);

        print_infer_line(i + 1u, NV_TEST_COUNT, NV_TEST_SEQ[i], &r);

        if (rc == 0 && r.infer_ms >= 5.0)

            ok++;

    }



    printf("\n[nv] Teste 4x: %u/%u com latencia >= 5 ms\n", ok, NV_TEST_COUNT);

    fflush(stdout);

}



static void mode_draw_image(nv_ports_t *ports, vga_ports_t *vga)

{

    uint8_t pixels[NV_IMG_PIXELS];

    nv_infer_result_t r;

    int rc;



    rc = mouse_draw_canvas(vga, pixels, NULL);

    if (rc != 0) {

        fprintf(stderr,

                "Modo desenho falhou (cod %d). "

                "Verifique mouse em /dev/input/event* ou MOUSE_DEV.\n", rc);

        return;

    }



    if (save_raw_image("desenho.raw", pixels) == 0)
        printf("Buffer salvo em desenho.raw (abra na opcao 1 para comparar)\n");

    rc = nv_send_image_buf(ports, pixels);

    if (rc != 0) {

        fprintf(stderr, "Erro ao enviar desenho (cod %d)\n", rc);

        return;

    }



    rc = nv_run_inference(ports, &r);

    if (rc != 0) {

        fprintf(stderr, "Inferencia falhou (cod %d)\n", rc);

        return;

    }



    printf("\n----------------------------------------\n");

    printf("  RESULTADO INFERENCIA (desenho)\n");

    printf("----------------------------------------\n");

    printf("  Digito:   %u\n", r.digit);

    printf("  Latencia: %.3f ms\n", r.infer_ms);

    printf("----------------------------------------\n");

    fflush(stdout);

}



int main(void)

{

    nv_ports_t ports;

    vga_ports_t vga;

    void *mapped = MAP_FAILED;

    int fd_mem = -1;

    int rc;



    rc = nv_mmap_open(&ports, &mapped, &fd_mem);

    if (rc != 0) {

        fprintf(stderr, "mmap /dev/mem falhou (%d)\n", rc);

        return 1;

    }



    vga_init_from_map(&vga, mapped);



    printf("Carregando modelo (%s, %s, %s)...\n",

           NV_PATH_WIN, NV_PATH_BIAS, NV_PATH_BETA);

    fflush(stdout);



    rc = nv_model_load(&ports, NV_PATH_WIN, NV_PATH_BIAS, NV_PATH_BETA);

    if (rc != 0) {

        fprintf(stderr, "Carga do modelo falhou (cod %d)\n", rc);

        nv_mmap_close(mapped, fd_mem);

        return 1;

    }

    printf("Modelo carregado.\n");



    for (;;) {

        print_menu();

        rc = read_option();

        if (rc < 0) {

            fputs("Opcao invalida.\n", stderr);

            continue;

        }

        if (rc == 1)
            mode_send_one(&ports, &vga);

        else if (rc == 2)

            mode_test_four(&ports);

        else if (rc == 3)

            mode_draw_image(&ports, &vga);

        else if (rc == 4)

            nv_mode_testbranch(&ports, NV_TESTBRANCH_ROOT_DEFAULT);

    }



    return 0;

}


