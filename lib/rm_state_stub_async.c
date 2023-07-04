
#include <stdbool.h>

#include "nand_constants.h"
#include "nand_types.h"
#include "ns.h"
#include "rm.h"

ms_t rm_machine_state; /* Global parser state machine state */

/* rm_get_state()
 *
 * in:     nothing
 * out:    nothing
 * return: current parser state machine state.
 *
 */

extern void await_rm_monitor_async();

ms_t
rm_get_state(void)
{
    await_rm_monitor_async();
    return rm_machine_state;
}

/* rm_reset()
 *
 * in:     nothing
 * out:    rm_machine_state reset via side effect.
 *         ns_machine_state reset via side effect via ns_reset().
 * return: nothing
 *
 * Resets parser state machine for both the runtime monitor and the
 * NAND stub to their initial states.
 *
 */

extern void rm_monitor_reset();

void
rm_reset(void)
{
    ns_reset();
    rm_monitor_reset();
    rm_machine_state = MS_INITIAL_STATE;
}
