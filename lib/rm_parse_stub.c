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
extern enum rm_err_t rm_monitor(token_t *token, ms_t *machine_state);

void
rm_parse(token_t token) {

    enum rm_err_t err = rm_monitor(&token, &rm_machine_state);
    if (err == ERR_OK)
    {
        ns_parse(token); /* forward token to NAND Stub */
    }
    else if (err == ERR_RESET_OK)
    {
        ns_reset();
        ns_parse(token);
    }
}

