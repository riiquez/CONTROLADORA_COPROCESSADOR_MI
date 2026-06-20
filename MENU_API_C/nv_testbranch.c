#define _POSIX_C_SOURCE 199309L

#include "nv_testbranch.h"
#include "nv_png_img.h"

#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <time.h>

typedef struct {
    unsigned expected;
    unsigned n_samples;
    unsigned correct;
    double mean_ms;
    double std_ms;
    double throughput;
} nv_digit_stats_t;

typedef struct {
    char *name;
} nv_file_entry_t;

typedef struct {
    unsigned expected;
    unsigned predicted;
    unsigned correct;
    double latency_ms;
    char filename[192];
} nv_detail_row_t;

static int is_png_file(const char *name)
{
    size_t len = strlen(name);
    if (len < 5)
        return 0;
    return strcasecmp(name + len - 4, ".png") == 0;
}

static int cmp_file_entry(const void *a, const void *b)
{
    const nv_file_entry_t *fa = (const nv_file_entry_t *)a;
    const nv_file_entry_t *fb = (const nv_file_entry_t *)b;
    return strcmp(fa->name, fb->name);
}

static void free_file_list(nv_file_entry_t *list, unsigned n)
{
    if (!list)
        return;
    for (unsigned i = 0; i < n; i++)
        free(list[i].name);
    free(list);
}

static int collect_png_files(const char *dir_path, nv_file_entry_t **out_list,
                             unsigned *out_count)
{
    DIR *dir = opendir(dir_path);
    struct dirent *ent;
    unsigned cap = 64;
    unsigned count = 0;
    nv_file_entry_t *list;

    if (!dir)
        return -1;

    list = calloc(cap, sizeof(nv_file_entry_t));
    if (!list) {
        closedir(dir);
        return -2;
    }

    while ((ent = readdir(dir)) != NULL) {
        if (!is_png_file(ent->d_name))
            continue;

        if (count >= cap) {
            cap *= 2u;
            nv_file_entry_t *grown = realloc(list, cap * sizeof(nv_file_entry_t));
            if (!grown) {
                closedir(dir);
                free_file_list(list, count);
                return -2;
            }
            list = grown;
        }

        list[count].name = strdup(ent->d_name);
        if (!list[count].name) {
            closedir(dir);
            free_file_list(list, count);
            return -2;
        }
        count++;
    }

    closedir(dir);

    if (count > 0)
        qsort(list, count, sizeof(nv_file_entry_t), cmp_file_entry);

    *out_list = list;
    *out_count = count;
    return 0;
}

static double ms_between(const struct timespec *a, const struct timespec *b)
{
    int64_t s = (int64_t)b->tv_sec - (int64_t)a->tv_sec;
    int64_t ns = (int64_t)b->tv_nsec - (int64_t)a->tv_nsec;
    return (double)(s * 1000000000LL + ns) / 1e6;
}

static void stats_compute(double *lat_ms, unsigned n, nv_digit_stats_t *st)
{
    double sum = 0.0;
    double var = 0.0;

    st->n_samples = n;
    if (n == 0) {
        st->mean_ms = 0.0;
        st->std_ms = 0.0;
        return;
    }

    for (unsigned i = 0; i < n; i++)
        sum += lat_ms[i];
    st->mean_ms = sum / (double)n;

    for (unsigned i = 0; i < n; i++) {
        double d = lat_ms[i] - st->mean_ms;
        var += d * d;
    }
    st->std_ms = sqrt(var / (double)n);
}

static void csv_escape(const char *s, char *out, size_t n)
{
    int need_quote = 0;
    size_t i, j = 0;

    for (i = 0; s[i]; i++) {
        if (s[i] == ',' || s[i] == '"' || s[i] == '\n')
            need_quote = 1;
    }

    if (!need_quote) {
        strncpy(out, s, n - 1);
        out[n - 1] = '\0';
        return;
    }

    if (j < n - 1)
        out[j++] = '"';
    for (i = 0; s[i] && j < n - 2; i++) {
        if (s[i] == '"')
            out[j++] = '"';
        out[j++] = s[i];
    }
    if (j < n - 1)
        out[j++] = '"';
    out[j] = '\0';
}

static int write_logs_resumo(const char *path, const nv_digit_stats_t *per_digit,
                             const nv_digit_stats_t *total)
{
    FILE *fp = fopen(path, "w");
    unsigned d;

    if (!fp)
        return -1;

    fprintf(fp,
            "expected_digit,samples,correct,accuracy_pct,mean_latency_ms,"
            "std_latency_ms,throughput_img_s\n");

    for (d = 0; d < 10u; d++) {
        const nv_digit_stats_t *s = &per_digit[d];
        double acc = (s->n_samples > 0)
            ? 100.0 * (double)s->correct / (double)s->n_samples : 0.0;
        fprintf(fp, "%u,%u,%u,%.2f,%.4f,%.4f,%.2f\n",
                s->expected, s->n_samples, s->correct, acc,
                s->mean_ms, s->std_ms, s->throughput);
    }

    {
        double acc = (total->n_samples > 0)
            ? 100.0 * (double)total->correct / (double)total->n_samples : 0.0;
        fprintf(fp, "TOTAL,%u,%u,%.2f,%.4f,%.4f,%.2f\n",
                total->n_samples, total->correct, acc,
                total->mean_ms, total->std_ms, total->throughput);
    }

    fclose(fp);
    return 0;
}

static int write_logs_detalhe(const char *path, const nv_detail_row_t *rows,
                              unsigned n)
{
    FILE *fp = fopen(path, "w");
    unsigned i;

    if (!fp)
        return -1;

    fprintf(fp,
            "expected_digit,arquivo,predicted_digit,acertou,latency_ms\n");

    for (i = 0; i < n; i++) {
        char esc[256];
        csv_escape(rows[i].filename, esc, sizeof(esc));
        fprintf(fp, "%u,%s,%u,%u,%.4f\n",
                rows[i].expected, esc, rows[i].predicted,
                rows[i].correct, rows[i].latency_ms);
    }

    fclose(fp);
    return 0;
}

static int write_logs_erros(const char *path, const nv_detail_row_t *rows,
                            unsigned n)
{
    FILE *fp = fopen(path, "w");
    unsigned i, prev_digit = 99u;

    if (!fp)
        return -1;

    fprintf(fp,
            "expected_digit,arquivo,predicted_digit,latency_ms\n");

    for (i = 0; i < n; i++) {
        char esc[256];

        if (rows[i].correct)
            continue;

        if (rows[i].expected != prev_digit) {
            if (prev_digit != 99u)
                fputc('\n', fp);
            fprintf(fp, "# erros pasta testes/%u (esperado %u)\n",
                    rows[i].expected, rows[i].expected);
            prev_digit = rows[i].expected;
        }

        csv_escape(rows[i].filename, esc, sizeof(esc));
        fprintf(fp, "%u,%s,%u,%.4f\n",
                rows[i].expected, esc, rows[i].predicted, rows[i].latency_ms);
    }

    fclose(fp);
    return 0;
}

int nv_mode_testbranch(nv_ports_t *ports, const char *testes_root)
{
    nv_digit_stats_t per_digit[10];
    nv_detail_row_t *detail = NULL;
    unsigned detail_cap = 256;
    unsigned detail_n = 0;
    double *all_lat = NULL;
    unsigned total_runs = NV_TESTBRANCH_SAMPLES * 10u;
    unsigned run_idx = 0;
    unsigned total_correct = 0;
    struct timespec bench_t0, bench_t1;
    nv_digit_stats_t total = {0};
    int rc = 0;

    memset(per_digit, 0, sizeof(per_digit));
    for (unsigned d = 0; d < 10u; d++)
        per_digit[d].expected = d;

    detail = calloc(detail_cap, sizeof(nv_detail_row_t));
    all_lat = calloc(total_runs, sizeof(double));
    if (!detail || !all_lat) {
        fputs("Sem memoria para testbranch.\n", stderr);
        free(detail);
        free(all_lat);
        return -1;
    }

    printf("\n[nv] Testbranch: %u imagens .png por pasta (0..9) em '%s/'\n",
           NV_TESTBRANCH_SAMPLES, testes_root);
    printf("[nv] Saidas: %s (resumo), %s (todas), %s (so erros)\n\n",
           NV_TESTBRANCH_LOG_CSV, NV_TESTBRANCH_LOG_DETAIL,
           NV_TESTBRANCH_LOG_ERRORS);
    fflush(stdout);

    if (clock_gettime(CLOCK_MONOTONIC, &bench_t0) != 0) {
        perror("clock_gettime");
        rc = -1;
        goto cleanup;
    }

    for (unsigned digit = 0; digit < 10u; digit++) {
        char dir_path[256];
        nv_file_entry_t *files = NULL;
        unsigned n_files = 0;
        unsigned n_use;
        double *digit_lat;

        snprintf(dir_path, sizeof(dir_path), "%s/%u", testes_root, digit);

        rc = collect_png_files(dir_path, &files, &n_files);
        if (rc != 0 || n_files == 0) {
            fprintf(stderr,
                    "[nv] Pasta '%s' sem arquivos .png ou inexistente.\n",
                    dir_path);
            free_file_list(files, n_files);
            rc = -2;
            goto cleanup;
        }

        n_use = (n_files < NV_TESTBRANCH_SAMPLES) ? n_files : NV_TESTBRANCH_SAMPLES;
        if (n_files < NV_TESTBRANCH_SAMPLES) {
            fprintf(stderr,
                    "[nv] Aviso: pasta %u tem apenas %u .png (usando todas).\n",
                    digit, n_files);
        }

        digit_lat = calloc(n_use, sizeof(double));
        if (!digit_lat) {
            free_file_list(files, n_files);
            rc = -1;
            goto cleanup;
        }

        struct timespec digit_t0, digit_t1;

        printf("--- Pasta testes/%u: digito esperado = %u (%u .png) ---\n",
               digit, digit, n_use);
        fflush(stdout);

        if (clock_gettime(CLOCK_MONOTONIC, &digit_t0) != 0) {
            perror("clock_gettime");
            free(digit_lat);
            free_file_list(files, n_files);
            rc = -1;
            goto cleanup;
        }

        for (unsigned i = 0; i < n_use; i++) {
            char fullpath[512];
            nv_infer_result_t inf;
            struct timespec t0, t1;
            int send_rc;

            snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, files[i].name);

            if (clock_gettime(CLOCK_MONOTONIC, &t0) != 0) {
                perror("clock_gettime");
                free(digit_lat);
                free_file_list(files, n_files);
                rc = -1;
                goto cleanup;
            }

            send_rc = nv_send_image_png(ports, fullpath);
            if (send_rc != 0) {
                fprintf(stderr, "[nv] envio falhou '%s' (cod %d)\n",
                        fullpath, send_rc);
                free(digit_lat);
                free_file_list(files, n_files);
                rc = send_rc;
                goto cleanup;
            }

            send_rc = nv_run_inference(ports, &inf);
            if (clock_gettime(CLOCK_MONOTONIC, &t1) != 0) {
                perror("clock_gettime");
                free(digit_lat);
                free_file_list(files, n_files);
                rc = -1;
                goto cleanup;
            }

            if (send_rc != 0) {
                fprintf(stderr, "[nv] inferencia falhou '%s' (cod %d)\n",
                        fullpath, send_rc);
                free(digit_lat);
                free_file_list(files, n_files);
                rc = send_rc;
                goto cleanup;
            }

            {
                double cycle_ms = ms_between(&t0, &t1);
                unsigned ok = (inf.digit == digit) ? 1u : 0u;
                nv_detail_row_t row;

                digit_lat[i] = inf.infer_ms;
                all_lat[run_idx] = inf.infer_ms;

                if (ok) {
                    per_digit[digit].correct++;
                    total_correct++;
                }

                row.expected = digit;
                row.predicted = inf.digit;
                row.correct = ok;
                row.latency_ms = inf.infer_ms;
                snprintf(row.filename, sizeof(row.filename), "%u/%s",
                         digit, files[i].name);

                if (detail_n >= detail_cap) {
                    detail_cap *= 2u;
                    nv_detail_row_t *grown =
                        realloc(detail, detail_cap * sizeof(nv_detail_row_t));
                    if (!grown) {
                        free(digit_lat);
                        free_file_list(files, n_files);
                        rc = -1;
                        goto cleanup;
                    }
                    detail = grown;
                }
                detail[detail_n++] = row;

                if (!nv_verbose()) {
                    if (!ok)
                        printf("  ERRO: %s pred=%u esperado=%u (%.2f ms)\n",
                               row.filename, inf.digit, digit, inf.infer_ms);
                    else if ((i + 1u) % 25u == 0u || i + 1u == n_use)
                        printf("  [%u] %u/%u  ultima pred=%u (%.2f ms)\n",
                               digit, i + 1u, n_use, inf.digit, inf.infer_ms);
                } else {
                    printf("  [%u] %s pred=%u esperado=%u infer=%.2f ms %s\n",
                           digit, row.filename, inf.digit, digit, inf.infer_ms,
                           ok ? "OK" : "ERRO");
                }
                (void)cycle_ms;
            }

            run_idx++;
        }

        if (clock_gettime(CLOCK_MONOTONIC, &digit_t1) != 0) {
            perror("clock_gettime");
            free(digit_lat);
            free_file_list(files, n_files);
            rc = -1;
            goto cleanup;
        }

        stats_compute(digit_lat, n_use, &per_digit[digit]);
        per_digit[digit].expected = digit;

        {
            double digit_s = ms_between(&digit_t0, &digit_t1) / 1000.0;
            per_digit[digit].throughput =
                (digit_s > 0.0) ? (double)n_use / digit_s : 0.0;
        }

        printf("  Resumo digito %u: %u/%u corretos (%.1f%%) lat media %.2f ms\n",
               digit, per_digit[digit].correct, n_use,
               100.0 * (double)per_digit[digit].correct / (double)n_use,
               per_digit[digit].mean_ms);
        fflush(stdout);

        free(digit_lat);
        free_file_list(files, n_files);
    }

    if (clock_gettime(CLOCK_MONOTONIC, &bench_t1) != 0) {
        perror("clock_gettime");
        rc = -1;
        goto cleanup;
    }

    total.expected = 99u;
    total.n_samples = run_idx;
    total.correct = total_correct;
    stats_compute(all_lat, run_idx, &total);

    {
        double total_s = ms_between(&bench_t0, &bench_t1) / 1000.0;
        total.throughput = (total_s > 0.0) ? (double)run_idx / total_s : 0.0;
    }

    printf("\n========================================\n");
    printf("  RESULTADO TESTBRANCH\n");
    printf("========================================\n");
    printf("  Imagens avaliadas : %u\n", run_idx);
    printf("  Acuracia global   : %u/%u (%.2f%%)\n",
           total_correct, run_idx,
           100.0 * (double)total_correct / (double)run_idx);
    printf("  Latencia media    : %.4f ms\n", total.mean_ms);
    printf("  Throughput global : %.2f imagens/s\n", total.throughput);
    printf("========================================\n");

    if (write_logs_resumo(NV_TESTBRANCH_LOG_CSV, per_digit, &total) != 0)
        perror("falha ao gravar logs.csv");
    if (write_logs_detalhe(NV_TESTBRANCH_LOG_DETAIL, detail, detail_n) != 0)
        perror("falha ao gravar logs_detalhe.csv");
    if (write_logs_erros(NV_TESTBRANCH_LOG_ERRORS, detail, detail_n) != 0)
        perror("falha ao gravar logs_erros.csv");

    printf("Arquivos: %s, %s, %s\n",
           NV_TESTBRANCH_LOG_CSV, NV_TESTBRANCH_LOG_DETAIL,
           NV_TESTBRANCH_LOG_ERRORS);
    fflush(stdout);
    rc = 0;

cleanup:
    free(detail);
    free(all_lat);
    return rc;
}
