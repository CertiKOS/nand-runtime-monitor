#include <gtest/gtest.h>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/sml.hpp>

#include "nand_monitor.h"
#include "nand_constants.h"

class NandMonitorTest : public ::testing::Test
{
protected:
    void SetUp() override { }

    void TearDown() override { }
};

TEST_F(NandMonitorTest, read_unexpected)
{
    std::vector<std::pair<token_t, ms_t>> tokens = {
        {{.command = C_READ_SETUP, .ready = true}, MS_READ_AWAITING_BLOCK_ADDRESS},
        {{.command = C_READ_EXECUTE, .ready = true}, MS_BUG},
    };
    ms_t machine_state = MS_INITIAL_STATE;

    nand_monitor_reset();
    for (auto& [token, expected_s] : tokens)
    {
        nand_monitor(&token, &machine_state);
        EXPECT_EQ(machine_state, expected_s);
    }
}

TEST_F(NandMonitorTest, read_expected)
{
    std::vector<std::pair<token_t, ms_t>> tokens = {
        {{.command = C_READ_SETUP, .ready = true}, MS_READ_AWAITING_BLOCK_ADDRESS},
        {{.command = C_READ_SETUP, .ready = true}, MS_READ_AWAITING_PAGE_ADDRESS},
        {{.command = C_READ_SETUP, .ready = true}, MS_READ_AWAITING_BYTE_ADDRESS},
        {{.command = C_READ_SETUP, .ready = true}, MS_READ_AWAITING_EXECUTE},
        {{.command = C_READ_EXECUTE, .ready = true}, MS_READ_PROVIDING_DATA},
        {{.command = C_READ_EXECUTE, .ready = true}, MS_READ_PROVIDING_DATA},
        {{.command = C_READ_EXECUTE, .ready = true}, MS_READ_PROVIDING_DATA},
    };
    ms_t machine_state = MS_INITIAL_STATE;

    nand_monitor_reset();
    for (auto& [token, expected_s] : tokens)
    {
        nand_monitor(&token, &machine_state);
        EXPECT_EQ(machine_state, expected_s);
    }
}


