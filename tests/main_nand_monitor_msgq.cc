#include "nand_monitor.h"

int main(int argc, char *argv[]) {
    token_t token;
    ms_t machine_state;
    rm_monitor_t m;
    enum rm_err_t status;

    rm_monitor_monitor(&m);

    while (true)
    {
        rm_monitor_next_token(&m, &token, &machine_state);
        status = nand_monitor(&token, &machine_state);
        rm_monitor_set_result(&m, status, machine_state);
    }
    return 0;
}
