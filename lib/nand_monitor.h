#pragma once
#include <stdbool.h>

#include "nand_types.h"

#if __cplusplus
extern "C" {
#endif

enum rm_err_t
{
    ERR_OK = 0,
    ERR_RESET_OK,
    ERR_FAIL,
};

enum rm_err_t nand_monitor(token_t *token, ms_t *machine_state);
void nand_monitor_reset();

#ifdef MONITOR_INTERFACE_MSGQ

typedef struct
{
    token_t token;
    ms_t machine_state;
} msg_t;

typedef struct
{
    bool succ;
    ms_t machine_state;
} result_t;

#define MSG_SIZE sizeof(msg_t)
#define RESULT_SIZE sizeof(result_t)

typedef struct
{
    enum rm_err_t ready;
    int monitor, driver;
} rm_monitor_t;

#define __driver
#define __monitor

__monitor void rm_monitor_monitor(rm_monitor_t * m);
__monitor void rm_monitor_next_token(rm_monitor_t * m, token_t *token, ms_t *machine_state);
__monitor void rm_monitor_set_result(rm_monitor_t * m, enum rm_err_t succ, ms_t machine_state);

#endif /* MONITOR_INTERFACE_MSGQ */


#if __cplusplus
}
#endif
