#ifndef NV_TESTBRANCH_H
#define NV_TESTBRANCH_H

#include "nv_hw.h"

#define NV_TESTBRANCH_ROOT_DEFAULT  "testes"
#define NV_TESTBRANCH_SAMPLES       100u
#define NV_TESTBRANCH_LOG_CSV       "logs.csv"
#define NV_TESTBRANCH_LOG_DETAIL    "logs_detalhe.csv"
#define NV_TESTBRANCH_LOG_ERRORS    "logs_erros.csv"

int nv_mode_testbranch(nv_ports_t *ports, const char *testes_root);

#endif
