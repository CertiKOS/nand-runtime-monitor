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

#define __driver
#define __monitor

enum rm_err_t nand_monitor(token_t *token, ms_t *machine_state);
void nand_monitor_reset();

__driver bool rm_monitor(token_t *token, ms_t *machine_state);
__driver void rm_monitor_reset();

#ifdef MONITOR_INTERFACE_MSGQ

enum rm_msg_type_t
{
    MSG_TOKEN,
    MSG_RESET,
};

typedef struct
{
    enum rm_msg_type_t type;
    token_t token;
    ms_t machine_state;
} token_msg_t;

typedef struct
{
    enum rm_msg_type_t type;
} empty_msg_t;

typedef union
{
    enum rm_msg_type_t type;
    token_msg_t token;
    empty_msg_t empty;
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

__monitor void rm_monitor_monitor(rm_monitor_t * m);
__monitor void rm_monitor_close(rm_monitor_t * m);
__monitor enum rm_msg_type_t rm_monitor_next_token(rm_monitor_t * m, token_t *token, ms_t *machine_state);
__monitor void rm_monitor_set_result(rm_monitor_t * m, enum rm_err_t succ, ms_t machine_state);

#endif /* MONITOR_INTERFACE_MSGQ */


#if __cplusplus
}
#endif
