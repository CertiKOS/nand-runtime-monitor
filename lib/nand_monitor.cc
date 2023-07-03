#include <iostream>

#include <boost/sml.hpp>
#include <boost/sml/utility/dispatch_table.hpp>

#include "nand_constants.h"
#include "nand_monitor.h"
#include "nand_types.h"

#ifdef DEBUG
#define log(...) fprintf(stderr, __VA_ARGS__)
#else
#define log(...)
#endif

namespace sml = boost::sml;

using namespace sml;

struct reset_event{};
struct read_setup_event{};
struct read_execute_event{};
struct program_setup_event{};
struct program_execute_event{};
struct erase_setup_event{};
struct erase_execute_event{};
struct dummy_event{};
struct error_event{};


// events
const auto reset           = sml::event<reset_event>;
const auto read_setup      = sml::event<read_setup_event>;
const auto read_execute    = sml::event<read_execute_event>;
const auto program_setup   = sml::event<program_setup_event>;
const auto program_execute = sml::event<program_execute_event>;
const auto erase_setup     = sml::event<erase_setup_event>;
const auto erase_execute   = sml::event<erase_execute_event>;
const auto dummy           = sml::event<dummy_event>;
const auto error           = sml::event<error_event>;

// states
const auto bug                            = "bug"_s;
const auto idle                           = "idle"_s;
const auto read_awaiting_block_address    = "read_awaiting_block_address"_s;
const auto read_awaiting_page_address     = "read_awaiting_page_address"_s;
const auto read_awaiting_byte_address     = "read_awaiting_byte_address"_s;
const auto read_awaiting_execute          = "read_awaiting_execute"_s;
const auto read_providing_data            = "read_providing_data"_s;
const auto program_awaiting_block_address = "program_awaiting_block_address"_s;
const auto program_awaiting_page_address  = "program_awaiting_page_address"_s;
const auto program_awaiting_byte_address  = "program_awaiting_byte_address"_s;
const auto program_accepting_data         = "program_accepting_data"_s;
const auto erase_awaiting_block_address   = "erase_awaiting_block_address"_s;
const auto erase_awaiting_execute         = "erase_awaiting_execute"_s;

class NandFlash
{
public:
    ms_t state;
    bool ready;
    bool to_reset;
};

auto is_nand_ready = [](NandFlash& nand) { return nand.ready; };
auto nand_reset = [](NandFlash& nand) { nand.to_reset = true; };

class NandMonitorStateMachine
{
public:
    auto operator()()
    {
        return make_transition_table(
            // clang-format off
            // initial
          * idle + read_setup = read_awaiting_block_address ,
            idle + program_setup = program_awaiting_block_address,
            idle + erase_setup = erase_awaiting_block_address,
            idle + error = bug,
            idle + unexpected_event<_> = bug,
            // bug
            bug + reset / nand_reset = idle,
            bug + read_setup [is_nand_ready] / nand_reset = read_awaiting_block_address,
            bug + program_setup [is_nand_ready] / nand_reset = program_awaiting_block_address,
            bug + erase_setup [is_nand_ready] / nand_reset = erase_awaiting_block_address,
            bug + unexpected_event<_> = bug,
            // read_awaiting_block_address
            read_awaiting_block_address + read_setup [is_nand_ready] = read_awaiting_page_address,
            read_awaiting_block_address + error = bug,
            read_awaiting_block_address + unexpected_event<_> = bug,
            // read_awaiting_page_address
            read_awaiting_page_address + read_setup [is_nand_ready] = read_awaiting_byte_address,
            read_awaiting_page_address + error = bug,
            read_awaiting_page_address + unexpected_event<_> = bug,
            // read_awaiting_byte_address
            read_awaiting_byte_address + read_setup [is_nand_ready] = read_awaiting_execute,
            read_awaiting_byte_address + error = bug,
            read_awaiting_byte_address + unexpected_event<_> = bug,
            // read_awaiting_execute
            read_awaiting_execute + read_execute [is_nand_ready] = read_providing_data,
            read_awaiting_execute + error = bug,
            read_awaiting_execute + unexpected_event<_> = bug,
            // read_providing_data
            read_providing_data + dummy = read_providing_data,
            read_providing_data + read_execute [is_nand_ready] = read_providing_data,
            read_providing_data + read_setup [is_nand_ready] = read_awaiting_block_address,
            read_providing_data + program_setup [is_nand_ready] = program_awaiting_block_address,
            read_providing_data + erase_setup [is_nand_ready] = erase_awaiting_block_address,
            read_providing_data + error = bug,
            read_providing_data + unexpected_event<_> = bug,
            // program_awaiting_block_address
            program_awaiting_block_address + program_setup [is_nand_ready] = program_awaiting_page_address,
            program_awaiting_block_address + error = bug,
            program_awaiting_block_address + unexpected_event<_> = bug,
            // program_awaiting_page_address
            program_awaiting_page_address + program_setup [is_nand_ready] = program_awaiting_byte_address,
            program_awaiting_page_address + error = bug,
            program_awaiting_page_address + unexpected_event<_> = bug,
            // program_awaiting_byte_address
            program_awaiting_byte_address + program_setup [is_nand_ready] = program_accepting_data,
            program_awaiting_byte_address + error = bug,
            program_awaiting_byte_address + unexpected_event<_> = bug,
            // program_accepting_data
            program_accepting_data + dummy = program_accepting_data,
            program_accepting_data + program_execute [is_nand_ready] = program_accepting_data,
            program_accepting_data + read_setup [is_nand_ready] = read_awaiting_block_address,
            program_accepting_data + program_setup [is_nand_ready] = program_awaiting_block_address,
            program_accepting_data + erase_setup [is_nand_ready] = erase_awaiting_block_address,
            program_accepting_data + error = bug,
            program_accepting_data + unexpected_event<_> = bug,
            // erase_awaiting_block_address
            erase_awaiting_block_address + erase_setup [is_nand_ready] = erase_awaiting_execute,
            erase_awaiting_block_address + unexpected_event<_> = bug,
            // erase_awaiting_execute
            erase_awaiting_execute + dummy = erase_awaiting_execute,
            erase_awaiting_execute + erase_execute [is_nand_ready] = erase_awaiting_execute,
            erase_awaiting_execute + read_setup [is_nand_ready] = read_awaiting_block_address,
            erase_awaiting_execute + program_setup [is_nand_ready] = program_awaiting_block_address,
            erase_awaiting_execute + erase_setup [is_nand_ready] = erase_awaiting_block_address,
            erase_awaiting_execute + error = bug
            // clang-format on
        );
    }
};

struct sm_logger
{
    template <class SM, class TEvent> void log_process_event(const TEvent&)
    {
        log("[%s][E] %s\n", sml::aux::get_type_name<SM>(),
            sml::aux::get_type_name<TEvent>());
    }

    template <class SM, class TGuard, class TEvent>
    void log_guard(const TGuard&, const TEvent&, bool result)
    {
        log("[%s][G] %s %s %s\n", sml::aux::get_type_name<SM>(),
            sml::aux::get_type_name<TGuard>(),
            sml::aux::get_type_name<TEvent>(), (result ? "[OK]" : "[Reject]"));
    }

    template <class SM, class TAction, class TEvent>
    void log_action(const TAction&, const TEvent&)
    {
        log("[%s][A] %s %s\n", sml::aux::get_type_name<SM>(),
            sml::aux::get_type_name<TAction>(),
            sml::aux::get_type_name<TEvent>());
    }

    template <class SM, class TSrcState, class TDstState>
    void log_state_change(const TSrcState& src, const TDstState& dst)
    {
        log("[%s][T] %s -> %s\n", sml::aux::get_type_name<SM>(), src.c_str(),
            dst.c_str());
    }
};

class NandMonitor
{
private:
    NandFlash                                                nand;
#ifdef DEBUG
    sm_logger                                                slog;
    sml::sm<NandMonitorStateMachine, sml::logger<sm_logger>, sml::testing> sm;
#else
    sml::sm<NandMonitorStateMachine, sml::testing> sm;
#endif

public:
    NandMonitor()
        : nand {}
#ifdef DEBUG
        , slog {}
        , sm { nand, slog }
#else
        , sm { nand }
#endif
    {
    }
    bool _execute(token_t& token, ms_t& machine_state);
    void execute(token_t& token, ms_t& machine_state);
    bool ok() {return !sm.is("bug"_s); }
    bool need_to_reset() {return nand.to_reset;}

    size_t state();
    void reset();
};

bool
NandMonitor::_execute(token_t& token, ms_t& machine_state)
{
    nand.state = machine_state;
    nand.ready = token.ready;
    switch (token.command)
    {
    case C_READ_SETUP: return sm.process_event(read_setup_event{});
    case C_READ_EXECUTE: return sm.process_event(read_execute_event{});
    case C_PROGRAM_SETUP: return sm.process_event(program_setup_event{});
    case C_PROGRAM_EXECUTE: return sm.process_event(program_execute_event{});
    case C_ERASE_SETUP: return sm.process_event(erase_setup_event{});
    case C_ERASE_EXECUTE: return sm.process_event(erase_execute_event{});
    case C_DUMMY: return sm.process_event(dummy_event{});
    default: return false;
    }
}

void
NandMonitor::execute(token_t& token, ms_t& machine_state)
{
    nand.to_reset = false;
    if (!_execute(token, machine_state))
    {
        sm.process_event(error_event{}); // if transition failed, go to bug state
    }
}

static inline const char * token_name(token_t & token)
{
    switch (token.command)
    {
    case C_READ_SETUP: return "read_setup";
    case C_READ_EXECUTE: return "read_execute";
    case C_PROGRAM_SETUP: return "program_setup";
    case C_PROGRAM_EXECUTE: return "program_execute";
    case C_ERASE_SETUP: return "erase_setup";
    case C_ERASE_EXECUTE: return "erase_execute";
    case C_DUMMY: return "dummy";
    default: return "C_UNKNOWN";
    }
}

size_t
NandMonitor::state()
{
    if (sm.is("idle"_s))
    {
        return MS_INITIAL_STATE;
    }
    else if (sm.is("bug"_s))
    {
        return MS_BUG;
    }
    else if (sm.is("read_awaiting_block_address"_s))
    {
        return MS_READ_AWAITING_BLOCK_ADDRESS;
    }
    else if (sm.is("read_awaiting_page_address"_s))
    {
        return MS_READ_AWAITING_PAGE_ADDRESS;
    }
    else if (sm.is("read_awaiting_byte_address"_s))
    {
        return MS_READ_AWAITING_BYTE_ADDRESS;
    }
    else if (sm.is("read_awaiting_execute"_s))
    {
        return MS_READ_AWAITING_EXECUTE;
    }
    else if (sm.is("read_providing_data"_s))
    {
        return MS_READ_PROVIDING_DATA;
    }
    else if (sm.is("program_awaiting_block_address"_s))
    {
        return MS_PROGRAM_AWAITING_BLOCK_ADDRESS;
    }
    else if (sm.is("program_awaiting_page_address"_s))
    {
        return MS_PROGRAM_AWAITING_PAGE_ADDRESS;
    }
    else if (sm.is("program_awaiting_byte_address"_s))
    {
        return MS_PROGRAM_AWAITING_BYTE_ADDRESS;
    }
    else if (sm.is("program_accepting_data"_s))
    {
        return MS_PROGRAM_ACCEPTING_DATA;
    }
    else if (sm.is("erase_awaiting_block_address"_s))
    {
        return MS_ERASE_AWAITING_BLOCK_ADDRESS;
    }
    else if (sm.is("erase_awaiting_execute"_s))
    {
        return MS_ERASE_AWAITING_EXECUTE;
    }
    return MS_BUG;
}

void
NandMonitor::reset()
{
    nand.state = MS_INITIAL_STATE;
    nand.to_reset = false;
    sm.set_current_states("idle"_s);
}

static NandMonitor* monitor = nullptr;

enum rm_err_t
nand_monitor(token_t* token, ms_t* machine_state)
{
    if (monitor == nullptr)
    {
        monitor = new NandMonitor();
    }

    monitor->execute(*token, *machine_state);
    *machine_state = monitor->state();
    if (!monitor->ok())
    {
        log("executing token `%s, %s` error! (bug state %s)\n",
            token_name(*token),
            token->ready ? "ready" : "busy",
            monitor->ok() ? "no" : "yes");
        return ERR_FAIL;
    }

    return monitor->need_to_reset() ? ERR_RESET_OK : ERR_OK;
}

void nand_monitor_reset()
{
    if (monitor != nullptr)
    {
        monitor->reset();
        log("nand_monitor_reset: now %zu\n", monitor->state());
    }
}
