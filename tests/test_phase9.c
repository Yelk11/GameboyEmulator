#include "gb.h"
#include "unity.h"

void test_phase9_breakpoints(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    TEST_ASSERT_TRUE(debugger_add_breakpoint(&gb.debugger, 0x0100u));
    TEST_ASSERT_TRUE(debugger_add_breakpoint(&gb.debugger, 0x0040u));
    TEST_ASSERT_TRUE(debugger_should_break(&gb.debugger, 0x0100u));
    TEST_ASSERT_FALSE(debugger_should_break(&gb.debugger, 0x0200u));
    gb_destroy(&gb);
}

void test_phase9_debugger_state(void)
{
    GB gb;

    TEST_ASSERT_TRUE(gb_init(&gb));
    gb.debugger.opcode_trace = true;
    gb.debugger.memory_trace = true;
    gb.debugger.paused = true;
    gb.debugger.step_instruction = true;
    TEST_ASSERT_TRUE(gb.debugger.paused);
    TEST_ASSERT_TRUE(gb.debugger.step_instruction);
    TEST_ASSERT_TRUE(gb.debugger.opcode_trace);
    TEST_ASSERT_TRUE(gb.debugger.memory_trace);
    gb_destroy(&gb);
}
