#include <iostream>

#include <boost/sml.hpp>
#include <boost/sml/utility/dispatch_table.hpp>

#include "nand_constants.h"
#include "nand_monitor.h"
#include "nand_types.h"

#ifdef DEBUG
#define log(...) fprintf(stdout, __VA_ARGS__)
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

class NandFlash
{
public:
    ms_t state;
    bool ready;
    bool to_reset;
};

auto is_nand_ready = [](NandFlash& nand) { return nand.ready; };
auto nand_reset = [](NandFlash& nand) { nand.to_reset = true; };
template<size_t S> auto nand_set_state = [](NandFlash& nand) { nand.state = S; };

class NandMonitorStateMachine
{
public:
    auto operator()()
    {
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

        return make_transition_table(
            // clang-format off
            // initial
          * idle + on_entry<_> / nand_set_state<MS_INITIAL_STATE>,
            idle + read_setup = read_awaiting_block_address ,
            idle + program_setup = program_awaiting_block_address,
            idle + erase_setup = erase_awaiting_block_address,
            idle + error = bug,
            idle + unexpected_event<_> = bug,
            // bug
            bug + on_entry<_> / nand_set_state<MS_BUG>,
            bug + reset / nand_reset = idle,
            bug + read_setup [is_nand_ready] / nand_reset = read_awaiting_block_address,
            bug + program_setup [is_nand_ready] / nand_reset = program_awaiting_block_address,
            bug + erase_setup [is_nand_ready] / nand_reset = erase_awaiting_block_address,
            bug + unexpected_event<_> = bug,
            // read_awaiting_block_address
            read_awaiting_block_address + on_entry<_> / nand_set_state<MS_READ_AWAITING_BLOCK_ADDRESS>,
            read_awaiting_block_address + read_setup [is_nand_ready] = read_awaiting_page_address,
            read_awaiting_block_address + error = bug,
            // read_awaiting_page_address
            read_awaiting_page_address + on_entry<_> / nand_set_state<MS_READ_AWAITING_PAGE_ADDRESS>,
            read_awaiting_page_address + read_setup [is_nand_ready] = read_awaiting_byte_address,
            read_awaiting_page_address + error = bug,
            // read_awaiting_byte_address
            read_awaiting_byte_address + on_entry<_> / nand_set_state<MS_READ_AWAITING_BYTE_ADDRESS>,
            read_awaiting_byte_address + read_setup [is_nand_ready] = read_awaiting_execute,
            read_awaiting_byte_address + error = bug,
            // read_awaiting_execute
            read_awaiting_execute + on_entry<_> / nand_set_state<MS_READ_AWAITING_EXECUTE>,
            read_awaiting_execute + read_execute [is_nand_ready] = read_providing_data,
            read_awaiting_execute + error = bug,
            // read_providing_data
            read_providing_data + on_entry<_> / nand_set_state<MS_READ_PROVIDING_DATA>,
            read_providing_data + dummy [is_nand_ready] = read_providing_data,
            read_providing_data + read_execute [is_nand_ready] = read_providing_data,
            read_providing_data + read_setup [is_nand_ready] = read_awaiting_block_address,
            read_providing_data + program_setup [is_nand_ready] = program_awaiting_block_address,
            read_providing_data + erase_setup [is_nand_ready] = erase_awaiting_block_address,
            read_providing_data + error = bug,
            // program_awaiting_block_address
            program_awaiting_block_address + on_entry<_> / nand_set_state<MS_PROGRAM_AWAITING_BLOCK_ADDRESS>,
            program_awaiting_block_address + program_setup [is_nand_ready] = program_awaiting_page_address,
            program_awaiting_block_address + error = bug,
            // program_awaiting_page_address
            program_awaiting_page_address + on_entry<_> / nand_set_state<MS_PROGRAM_AWAITING_PAGE_ADDRESS>,
            program_awaiting_page_address + program_setup [is_nand_ready] = program_awaiting_byte_address,
            program_awaiting_page_address + error = bug,
            // program_awaiting_byte_address
            program_awaiting_byte_address + on_entry<_> / nand_set_state<MS_PROGRAM_AWAITING_BYTE_ADDRESS>,
            program_awaiting_byte_address + program_setup [is_nand_ready] = program_accepting_data,
            program_awaiting_byte_address + error = bug,
            // program_accepting_data
            program_accepting_data + on_entry<_> / nand_set_state<MS_PROGRAM_ACCEPTING_DATA>,
            program_accepting_data + dummy [is_nand_ready] = program_accepting_data,
            program_accepting_data + program_execute [is_nand_ready] = program_accepting_data,
            program_accepting_data + read_setup [is_nand_ready] = read_awaiting_block_address,
            program_accepting_data + program_setup [is_nand_ready] = program_awaiting_block_address,
            program_accepting_data + erase_setup [is_nand_ready] = erase_awaiting_block_address,
            program_accepting_data + error = bug,
            // erase_awaiting_block_address
            erase_awaiting_block_address + on_entry<_> / nand_set_state<MS_ERASE_AWAITING_BLOCK_ADDRESS>,
            erase_awaiting_block_address + erase_setup [is_nand_ready] = erase_awaiting_execute,
            erase_awaiting_block_address + error = bug,
            // erase_awaiting_execute
            erase_awaiting_execute + on_entry<_> / nand_set_state<MS_ERASE_AWAITING_EXECUTE>,
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
    bool do_execute(size_t command);
    void execute(token_t& token, ms_t& machine_state);
    bool ok() {return !sm.is("bug"_s); }
    bool need_to_reset() const {return nand.to_reset;}

    size_t state() const;
    void reset();
};

bool
NandMonitor::do_execute(size_t command)
{
    switch (command)
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
    nand.state = machine_state;
    nand.ready = token.ready;

    if (!do_execute(token.command))
    {
        sm.process_event(error_event{}); // if transition failed, go to bug state
    }
}

static inline const char*
token_name(const token_t& token)
{
    static const char* const token_command_str[] = C_NAMES;
    return token.command <= C_DUMMY ? token_command_str[token.command]
                                    : "C_UNKNOWN";
}

size_t
NandMonitor::state() const
{
    return nand.state;
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
