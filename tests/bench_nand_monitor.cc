#include <benchmark/benchmark.h>

#include "nand_monitor.h"
#include "nand_constants.h"

static void DoSetup(const benchmark::State& state) {
    // Perform setup here
    nand_monitor_reset();
}

static void DoTeardown(const benchmark::State& state) {
    // Perform teardown here
}

static void DoWarmup(benchmark::State& state){
    // Perform warmup here
    token_t token;
    ms_t machine_state = MS_INITIAL_STATE;

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

BENCHMARK(DoWarmup)->Iterations(10)->Setup(DoSetup);
BENCHMARK(BM_nand_monitor)->Setup(DoSetup);
BENCHMARK(BM_rm_monitor)->Setup(DoSetup);

BENCHMARK_MAIN();
