#include <benchmark/benchmark.h>

#include "nand_monitor.h"
#include "nand_constants.h"

static void ns_parse(token_t token)
{
}

static void ns_reset(void)
{
}

static ms_t ns_machine_state;

static void DoSetup(const benchmark::State& state) {
    // Perform setup here
#if defined(MONITOR_INTERFACE_ASYNC)
    ns_machine_state = MS_INITIAL_STATE;
    static bool setup = false;
    if (!setup)
    {
        rm_monitor_async_setup(&ns_machine_state, ns_parse, ns_reset);
        setup = true;
    }
#endif
}

static void DoTeardown(const benchmark::State& state) {
    // Perform teardown here
}

static void DoWarmup(benchmark::State& state){
    // Perform warmup here
    token_t token;
    ms_t machine_state = MS_INITIAL_STATE;

    nand_monitor_reset();
    token.ready = true;
    token.command = C_READ_SETUP;
    nand_monitor(&token, &machine_state);
    nand_monitor(&token, &machine_state);
    nand_monitor(&token, &machine_state);

    token.command = C_READ_EXECUTE;
    for (auto _ : state)
    {
        nand_monitor(&token, &machine_state);
    }
    nand_monitor_reset();
}

static void BM_nand_monitor(benchmark::State& state) {
    // Code inside this loop is measured repeatedly
    token_t token;
    ms_t machine_state = MS_INITIAL_STATE;

    nand_monitor_reset();
    token.ready = true;
    token.command = C_READ_SETUP;
    nand_monitor(&token, &machine_state);
    nand_monitor(&token, &machine_state);
    nand_monitor(&token, &machine_state);

    token.command = C_READ_EXECUTE;
    for (auto _ : state) {
        nand_monitor(&token, &machine_state);
    }
}

#if defined(MONITOR_INTERFACE_FUNC)
#define MIF "func"
#elif defined(MONITOR_INTERFACE_MSGQ)
#define MIF "msgq"
#elif defined(MONITOR_INTERFACE_THINROS)
#define MIF "thinros"
#else
#define MIF "unknown"
#endif

#if !defined(MONITOR_INTERFACE_ASYNC)
static void BM_rm_monitor(benchmark::State& state) {
    token_t token;
    ms_t machine_state = MS_INITIAL_STATE;
    state.SetLabel("rm_monitor_" MIF);

    token.ready = true;
    token.command = C_READ_SETUP;
    rm_monitor(&token, &machine_state);
    rm_monitor(&token, &machine_state);
    rm_monitor(&token, &machine_state);

    token.command = C_READ_EXECUTE;
    for (auto _ : state) {
        rm_monitor(&token, &machine_state);
    }
}
#endif



#if defined(MONITOR_INTERFACE_ASYNC)

static void BM_rm_monitor_async(benchmark::State& state) {
    token_t token;
    ms_t machine_state = MS_INITIAL_STATE;
    state.SetLabel("rm_monitor_" MIF "_async");

    token.ready = true;
    token.command = C_READ_SETUP;
    rm_monitor_async(&token, &machine_state);
    rm_monitor_async(&token, &machine_state);
    rm_monitor_async(&token, &machine_state);
    await_rm_monitor_async();

    token.command = C_READ_EXECUTE;
    size_t i = 0;
    for (auto _ : state)
    {
        rm_monitor_async(&token, &machine_state);
        await_rm_monitor_async();
    }
}
#endif

BENCHMARK(DoWarmup)->Iterations(10)->Setup(DoSetup);
BENCHMARK(BM_nand_monitor);
#if !defined(MONITOR_INTERFACE_ASYNC)
BENCHMARK(BM_rm_monitor);
#endif

#if defined(MONITOR_INTERFACE_ASYNC)
BENCHMARK(BM_rm_monitor_async)->Setup(DoSetup);
#endif

BENCHMARK_MAIN();
