#include "nand_monitor.h"
#include "msgq_common.h"

__driver void rm_monitor_driver(rm_monitor_t * m)
{
    msgq_driver_open(m);
}

__monitor void rm_monitor_close(rm_monitor_t * m)
{
    msgq_monitor_close(m);
}

__monitor void rm_monitor_monitor(rm_monitor_t * m)
{
    msgq_monitor_open(m);
}

__monitor enum rm_msg_type_t rm_monitor_next_token(rm_monitor_t * m, token_t *token, ms_t *machine_state)
{
    msg_t msg;
    msgq_rx_msg(m, &msg);

    switch (msg.type)
    {
    case MSG_TOKEN:
        *token = msg.token.token;
        *machine_state = msg.token.machine_state;
        break;
    case MSG_RESET:
        nand_monitor_reset();
        break ;
    default:
        break;
    }
    return msg.type;
}

__monitor void rm_monitor_set_result(rm_monitor_t * m, enum rm_err_t succ, ms_t machine_state)
{
    result_t result = {
        .succ = succ,
        .machine_state = machine_state,
    };
    msgq_tx_result(m, &result);
}

static rm_monitor_t monitor =
{
    .ready = false,
};

__driver enum rm_err_t rm_monitor(token_t *token, ms_t *machine_state) {

    if (!monitor.ready)
    {
        rm_monitor_driver(&monitor);
        monitor.ready = true;
    }
    static msg_t msg;
    static result_t result;

    msg.type = MSG_TOKEN;
    msg.token.token = *token;
    msg.token.machine_state = *machine_state;

    msgq_tx_msg(&monitor, &msg);
    msgq_rx_result(&monitor, &result);

    *machine_state = result.machine_state;
    return result.succ;
}

__driver void rm_monitor_reset()
{
    static msg_t msg;
    static result_t result;

    if (!monitor.ready)
    {
        rm_monitor_driver(&monitor);
        monitor.ready = true;
    }

    msg.type = MSG_RESET;
    msgq_tx_msg(&monitor, &msg);
    msgq_rx_result(&monitor, &result);
}
