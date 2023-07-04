/* Stub parser for runtime monitor with no real functionality.
 * This parser won't pass the test suite; it exists solely to
 * demonstrate the system tests running (and sometimes failing).
 */

#include <stdbool.h>
#include <assert.h>

#include "nand_constants.h"
#include "nand_types.h"
#include "ns.h"
#include "rm.h"

enum rm_err_t
{
    ERR_OK = 0,
    ERR_RESET_OK,
    ERR_FAIL,
};

extern ms_t rm_machine_state;
extern void rm_monitor_async_setup(ms_t* machine_state,
        void (*parse_callback)(token_t token), void (*reset_callback)(void));
extern void rm_monitor_async(token_t *token, ms_t *machine_state);

void
rm_parse(token_t token) {

    static bool has_setup = false;
    if (!has_setup)
    {
        rm_monitor_async_setup(&rm_machine_state, ns_parse, ns_reset);
        has_setup = true;
    }
    rm_monitor_async(&token, &rm_machine_state);
}

