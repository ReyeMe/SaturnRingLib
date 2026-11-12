// Tests/src/testsEvent.hpp
// SRL::Types::Event and MemberProxy: static, member and lambda callbacks, order, duplicates, removal,
// null callbacks, reference arguments, changes during Invoke, and the Core sync/v-blank events.
#pragma once

#include <srl.hpp>
#include <srl_log.hpp>
#include <srl_event.hpp>

// https://github.com/siu/minunit
#include "minunit.h"

using namespace SRL;
using SRL::Types::Event;
using SRL::Types::MemberProxy;

extern "C" {
extern const uint8_t buffer_size;
extern char buffer[];
}

void event_test_setup(void) {}
void event_test_teardown(void) {}

void event_test_output_header(void)
{
    if (!suite_error_counter++)
    {
        if (Log::GetLogLevel() == Logger::LogLevels::TESTING)
        {
            LogDebug("****UT_EVENT****");
        }
        else
        {
            LogInfo("****UT_EVENT_ERROR(S)****");
        }
    }
}

/** @brief Call log shared by the callbacks: each call appends its id. */
static char event_log[32];
static int event_log_length = 0;
static int event_last_value = 0;

static void event_log_reset()
{
    event_log_length = 0;
    event_log[0] = '\0';
    event_last_value = 0;
}

static void event_log_add(char id)
{
    if (event_log_length < static_cast<int>(sizeof(event_log)) - 1)
    {
        event_log[event_log_length++] = id;
        event_log[event_log_length] = '\0';
    }
}

static void event_static_a(int value) { event_log_add('a'); event_last_value = value; }
static void event_static_b(int value) { event_log_add('b'); event_last_value = value; }

/** @brief Class with a member callback. */
struct EventListener
{
    char id;
    int received = 0;
    MemberProxy<int> Proxy = MemberProxy<int>(this, &EventListener::OnEvent);

    explicit EventListener(char id) : id(id) {}

    void OnEvent(int value)
    {
        this->received = value;
        event_log_add(this->id);
    }
};

    /**
 * @brief A static callback and a member callback receive the Invoke arguments.
 */
MU_TEST(event_static_and_member)
{
    event_log_reset();
    Event<int> event;
    EventListener listener('m');
    event += event_static_a;
    event += &listener.Proxy;

    event.Invoke(42);
    snprintf(buffer, buffer_size, "Invoke(42): calls '%s', static got %d, member got %d", event_log, event_last_value, listener.received);
    mu_assert(strcmp(event_log, "am") == 0 && event_last_value == 42 && listener.received == 42, buffer);
}

    /**
 * @brief A lambda proxy keeps its captures and is called on Invoke.
 */
MU_TEST(event_lambda_proxy)
{
    int counter = 0;
    int sum = 0;
    MemberProxy<int> proxy([&counter, &sum](int value) { counter++; sum += value; });
    Event<int> event;
    event += &proxy;

    event.Invoke(3);
    event.Invoke(4);
    snprintf(buffer, buffer_size, "Lambda proxy after Invoke(3), Invoke(4): %d calls, sum %d (expected 2, 7)", counter, sum);
    mu_assert(counter == 2 && sum == 7, buffer);
}

    /**
 * @brief Order: static callbacks run first in the order added, then member callbacks in the order added.
 */
MU_TEST(event_call_order)
{
    event_log_reset();
    Event<int> event;
    EventListener x('x');
    EventListener y('y');
    event += &x.Proxy;
    event += event_static_b;
    event += &y.Proxy;
    event += event_static_a;

    event.Invoke(1);
    snprintf(buffer, buffer_size, "Call order '%s' (expected 'baxy': statics in order added, then members)", event_log);
    mu_assert(strcmp(event_log, "baxy") == 0, buffer);
}

    /**
 * @brief The same callback added twice runs twice; removing it once leaves one; removing it again leaves none.
 */
MU_TEST(event_duplicates_and_removal)
{
    event_log_reset();
    Event<int> event;
    EventListener m('m');
    event += event_static_a;
    event += event_static_a;
    event += &m.Proxy;
    event += &m.Proxy;

    event.Invoke(1);
    snprintf(buffer, buffer_size, "Added twice: calls '%s' (expected 'aamm')", event_log);
    mu_assert(strcmp(event_log, "aamm") == 0, buffer);

    event_log_reset();
    event -= event_static_a;
    event -= &m.Proxy;
    event.Invoke(1);
    snprintf(buffer, buffer_size, "Removed once: calls '%s' (expected 'am')", event_log);
    mu_assert(strcmp(event_log, "am") == 0, buffer);

    event_log_reset();
    event -= event_static_a;
    event -= &m.Proxy;
    event.Invoke(1);
    snprintf(buffer, buffer_size, "Removed twice: calls '%s' (expected none)", event_log);
    mu_assert(event_log_length == 0, buffer);
}

    /**
 * @brief Removing a callback that was never added changes nothing; Invoke on an empty event does nothing.
 */
MU_TEST(event_remove_unknown_and_empty)
{
    event_log_reset();
    Event<int> empty;
    empty.Invoke(5);
    mu_assert(event_log_length == 0, "Invoke on an empty event should call nothing");

    Event<int> event;
    EventListener m('m');
    EventListener other('o');
    event += event_static_a;
    event += &m.Proxy;
    event -= event_static_b;
    event -= &other.Proxy;

    event.Invoke(5);
    snprintf(buffer, buffer_size, "After removing callbacks never added: calls '%s' (expected 'am')", event_log);
    mu_assert(strcmp(event_log, "am") == 0, buffer);
}

    /**
 * @brief Null callbacks (a null function pointer, a null proxy pointer, a proxy with no function) are skipped.
 */
MU_TEST(event_null_callbacks)
{
    event_log_reset();
    Event<int> event;
    MemberProxy<int> emptyProxy{std::function<void(int)>()};
    event += static_cast<Event<int>::CallbackStatic>(nullptr);
    event += static_cast<Event<int>::CallbackMember>(nullptr);
    event += &emptyProxy;
    event += event_static_a;

    event.Invoke(9);
    snprintf(buffer, buffer_size, "With null callbacks: calls '%s', value %d (expected 'a', 9)", event_log, event_last_value);
    mu_assert(strcmp(event_log, "a") == 0 && event_last_value == 9, buffer);
}

static void event_increment(int &value) { value++; }

    /**
 * @brief Reference arguments: every callback sees and changes the caller's variable.
 */
MU_TEST(event_reference_argument)
{
    Event<int &> event;
    MemberProxy<int &> doubler([](int &value) { value *= 2; });
    event += event_increment;
    event += &doubler;

    int value = 5;
    event.Invoke(value);
    snprintf(buffer, buffer_size, "Event<int&>: value 5 became %d (expected (5 + 1) * 2 = 12)", value);
    mu_assert(value == 12, buffer);
}

/** @brief Event used by the callbacks that change it while it runs. */
static Event<int> event_changing;

static void event_remove_self(int) { event_log_add('r'); event_changing -= event_remove_self; }
static void event_add_b(int) { event_log_add('+'); event_changing += event_static_b; }

    /**
 * @brief A callback that removes itself during Invoke does not make the next callback get skipped.
 */
MU_TEST(event_remove_during_invoke)
{
    event_log_reset();
    event_changing -= event_remove_self;
    event_changing -= event_static_a;
    event_changing += event_remove_self;
    event_changing += event_static_a;

    event_changing.Invoke(1);
    snprintf(buffer, buffer_size, "Callback removing itself during Invoke: calls '%s' (expected 'ra': the next callback still runs)", event_log);
    event_changing -= event_remove_self;
    event_changing -= event_static_a;
    mu_assert(strcmp(event_log, "ra") == 0, buffer);
}

    /**
 * @brief A callback that adds another callback during Invoke: the callbacks already there run once,
 *        and the new one runs from the next Invoke.
 * @note Risky: adding grows the vector being iterated (it reallocates on every add), so a broken Event reads freed memory.
 */
MU_TEST(event_add_during_invoke)
{
    event_log_reset();
    event_changing += event_add_b;
    event_changing += event_static_a;

    event_changing.Invoke(1);
    char firstCalls[32];
    snprintf(firstCalls, sizeof(firstCalls), "%s", event_log);
    event_changing -= event_add_b;

    event_log_reset();
    event_changing.Invoke(1);
    snprintf(buffer, buffer_size, "Callback adding another during Invoke: first Invoke '%s' (expected '+a'), next Invoke '%s' (expected 'ab')", firstCalls, event_log);
    event_changing -= event_static_a;
    event_changing -= event_static_b;
    mu_assert(strcmp(firstCalls, "+a") == 0 && strcmp(event_log, "ab") == 0, buffer);
}

static int event_before_sync_count = 0;
static int event_after_sync_count = 0;
static int event_vblank_count = 0;
static void event_on_before_sync() { event_before_sync_count++; }
static void event_on_after_sync() { event_after_sync_count++; }
static void event_on_vblank() { event_vblank_count++; }

    /**
 * @brief Core events: each Synchronize calls OnBeforeSync and OnAfterSync once; OnVblank runs about once per frame;
 *        after removal they are no longer called.
 */
MU_TEST(event_core_sync_and_vblank)
{
    event_before_sync_count = event_after_sync_count = event_vblank_count = 0;
    Core::OnBeforeSync += event_on_before_sync;
    Core::OnAfterSync += event_on_after_sync;
    Core::OnVblank += event_on_vblank;

    for (int i = 0; i < 5; i++) Core::Synchronize();

    Core::OnBeforeSync -= event_on_before_sync;
    Core::OnAfterSync -= event_on_after_sync;
    Core::OnVblank -= event_on_vblank;
    const int before = event_before_sync_count, after = event_after_sync_count, vblank = event_vblank_count;

    for (int i = 0; i < 3; i++) Core::Synchronize();

    snprintf(buffer, buffer_size, "5 x Synchronize: OnBeforeSync %d, OnAfterSync %d (expected 5, 5), OnVblank %d (expected about 5)", before, after, vblank);
    mu_assert(before == 5 && after == 5 && vblank >= 4 && vblank <= 12, buffer);
    snprintf(buffer, buffer_size, "After removal: OnBeforeSync %d, OnAfterSync %d, OnVblank %d (should not change)",
        event_before_sync_count, event_after_sync_count, event_vblank_count);
    mu_assert(event_before_sync_count == before && event_after_sync_count == after && event_vblank_count == vblank, buffer);
}

MU_TEST_SUITE(event_test_suite)
{
    MU_SUITE_CONFIGURE_WITH_HEADER(&event_test_setup,
        &event_test_teardown,
        &event_test_output_header);

    MU_RUN_TEST(event_static_and_member);
    MU_RUN_TEST(event_lambda_proxy);
    MU_RUN_TEST(event_call_order);
    MU_RUN_TEST(event_duplicates_and_removal);
    MU_RUN_TEST(event_remove_unknown_and_empty);
    MU_RUN_TEST(event_null_callbacks);
    MU_RUN_TEST(event_reference_argument);
    MU_RUN_TEST(event_core_sync_and_vblank);
    MU_RUN_TEST(event_remove_during_invoke);
    // Last: a broken Event reads freed memory here
    MU_RUN_TEST(event_add_during_invoke);
}
