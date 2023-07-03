#include "nand_monitor.h"

enum rm_err_t
rm_monitor(token_t *token, ms_t *machine_state) {
    return nand_monitor(token, machine_state);
}
