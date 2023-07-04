#pragma once

#include "nand_monitor.h"

#define MSGQ_QUEUE_SIZE    10

#if __cplusplus
extern "C" {
#endif

void msgq_monitor_open(rm_monitor_t * m);
void msgq_driver_open(rm_monitor_t * m);

void msgq_monitor_close(rm_monitor_t * m);
void msgq_driver_close(rm_monitor_t * m);

void msgq_rx_msg(rm_monitor_t * m, msg_t *msg);
void msgq_tx_msg(rm_monitor_t * m, msg_t *msg);

void msgq_rx_result(rm_monitor_t * m, result_t *result);
void msgq_tx_result(rm_monitor_t * m, result_t *result);

void msgq_setup_rx_msg_async(rm_monitor_t* m, void (*callback)(msg_t *));
void msgq_setup_rx_result_async(rm_monitor_t* m, void (*callback)(result_t *));

#if __cplusplus
}
#endif
