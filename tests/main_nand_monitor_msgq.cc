#include <iostream>

#include "nand_monitor.h"
#include "nand_constants.h"

static rm_monitor_t monitor;

int main(int argc, char *argv[]) {
    token_t token;
    ms_t machine_state;
    enum rm_err_t status;

    rm_monitor_monitor(&monitor);

    while (true)
    {
        enum rm_msg_type_t t = rm_monitor_next_token(&monitor, &token, &machine_state);
        switch (t)
        {
        case MSG_TOKEN:
            status = nand_monitor(&token, &machine_state);
            break;
        case MSG_RESET:
            nand_monitor_reset();
            machine_state = MS_INITIAL_STATE;
            status = ERR_OK;
            break ;
        default:
            printf("Unknown message type %d\n", t);
            exit(1);
            break;
        }
        rm_monitor_set_result(&monitor, status, machine_state);
    }
    return 0;
}
