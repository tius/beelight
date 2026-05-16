#if defined(ARDUINO)
	#include <Arduino.h>
#endif

#include <unity.h>

#include "lite/cli/args.h"
#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/color/gamma_lut.h"
#include "lite/color/rgb8.h"
#include "lite/core/changed.h"
#include "lite/core/compile_time.h"
#include "lite/core/event_bus.h"
#include "lite/core/macros.h"
#include "lite/core/msg.h"
#include "lite/core/spin_timer.h"
#include "lite/core/timer.h"
#include "lite/io/log.h"
#include "lite/stripe/base.h"
#include "lite/stripe/mapping.h"

#include <cstdarg>
#include <cstring>
#include <type_traits>

namespace {

using lite::u8;
using lite::u16;
using lite::u32;

static_assert(std::is_same_v<lite::uint_for_max_t<100u>, u8>);
static_assert(std::is_same_v<lite::uint_for_max_t<1000u>, u16>);
static_assert(lite::gcd_u32(48u, 18u) == 6u);

class MoveOnlyValue final {
public:
	explicit constexpr MoveOnlyValue(int value)
	: value_(value) {}

	MoveOnlyValue(const MoveOnlyValue&) = delete;
	MoveOnlyValue& operator=(const MoveOnlyValue&) = delete;

	constexpr MoveOnlyValue(MoveOnlyValue&&) noexcept = default;
	constexpr MoveOnlyValue& operator=(MoveOnlyValue&&) noexcept = default;

	[[nodiscard]] friend constexpr bool operator==(
		const MoveOnlyValue& lhs,
		const MoveOnlyValue& rhs
	) {
		return lhs.value_ == rhs.value_;
	}

	[[nodiscard]] constexpr int value() const {
		return value_;
	}

private:
	int value_ = 0;
};

class MsgProbe final {
public:
	void on_no_args() {
		calls++;
	}

	void on_one_arg(u32 value) {
		calls++;
		last0 = value;
	}

	void on_two_args(u32 first, u32 second) {
		calls++;
		last0 = first;
		last1 = second;
	}

	int calls = 0;
	u32 last0 = 0u;
	u32 last1 = 0u;
};

struct EventBusTestEvent final {
	u8 id = 0u;
	u8 p1 = 0u;
};

class EventBusProbe final {
public:
	void on_event(const EventBusTestEvent& event) {
		calls++;
		last_id = event.id;
		last_p1 = event.p1;
	}

	void on_event_no_args() {
		calls++;
	}

	int calls = 0;
	u8 last_id = 0u;
	u8 last_p1 = 0u;
};

void event_bus_mark_one(void* ctx, const EventBusTestEvent&) {
	auto* out = static_cast<int*>(ctx);
	*out = 1;
}

void event_bus_mark_two(void* ctx, const EventBusTestEvent&) {
	auto* out = static_cast<int*>(ctx);
	*out = 2;
}

struct FakeClock final {
	static void begin_window() {
		epoch_ = static_cast<u32>(epoch_ + 100000u);
		now_ = epoch_;
	}

	static void advance(u32 ticks) {
		now_ += ticks;
	}

	static u32 now() {
		return now_;
	}

	static u32 now_;
	static u32 epoch_;
};

u32 FakeClock::now_ = 0u;
u32 FakeClock::epoch_ = 0u;

class TimerProbe final {
public:
	void on_timeout() {
		call_count++;
		last_fire_tick = FakeClock::now();
	}

	void on_periodic() {
		call_count++;
		last_fire_tick = FakeClock::now();
	}

	void on_stop_self() {
		call_count++;
		last_fire_tick = FakeClock::now();
		if (timer != nullptr) {
			timer->stop();
		}
	}

	int call_count = 0;
	u32 last_fire_tick = 0u;
	lite::SpinTimer* timer = nullptr;
};

class CaptureOut final : public lite::Out {
public:
	std::size_t write(char c) override {
		if (len_ + 1u < k_buf_size) {
			buf_[len_++] = c;
			buf_[len_] = '\0';
		}

		return 1u;
	}

	void clear() {
		len_ = 0u;
		buf_[0] = '\0';
	}

	[[nodiscard]] const char* c_str() const {
		return buf_;
	}

	[[nodiscard]] bool contains(const char* needle) const {
		return std::strstr(buf_, needle) != nullptr;
	}

private:
	static constexpr std::size_t k_buf_size = 1024u;
	char buf_[k_buf_size] = {};
	std::size_t len_ = 0u;
};

class CmdProbe final {
public:
	void on_set(lite::Out& out, lite::Args args) {
		(void)out;
		call_count++;
		last_value = args.get_u16();
	}

	int call_count = 0;
	u16 last_value = 0u;
};

class RecordingShell final : public lite::Shell {
public:
	void banner(lite::Out& out) override {
		banner_calls++;
		out.print("ready");
	}

	void prompt(lite::Out& out) override {
		prompt_calls++;
		out.print('>');
	}

	void process(lite::Out& out, char* line) override {
		(void)out;
		process_calls++;
		std::strncpy(last_line, line ? line : "", sizeof(last_line) - 1u);
		last_line[sizeof(last_line) - 1u] = '\0';
	}

	int banner_calls = 0;
	int prompt_calls = 0;
	int process_calls = 0;
	char last_line[64] = {};
};

struct LogCapture final {
	int count = 0;
	u8 last_level = 0;
	char last_tag[64] = {};
	char last_fmt[128] = {};
};

class CaptureLogger final : public lite::log::Logger {
public:
	explicit CaptureLogger(LogCapture& cap) noexcept
	: cap_(cap) {}

	void write_v(
		u8 level,
		const char* tag,
		const char* fmt,
		va_list
	) const override {
		cap_.count++;
		cap_.last_level = level;
		std::strncpy(cap_.last_tag, tag ? tag : "", sizeof(cap_.last_tag) - 1u);
		cap_.last_tag[sizeof(cap_.last_tag) - 1u] = '\0';
		std::strncpy(cap_.last_fmt, fmt ? fmt : "", sizeof(cap_.last_fmt) - 1u);
		cap_.last_fmt[sizeof(cap_.last_fmt) - 1u] = '\0';
	}

private:
	LogCapture& cap_;
};

void write_with_list(u8 level, const char* tag, const char* fmt, ...) {
	va_list args;
	va_start(args, fmt);
	lite::log::LoggerList::instance().write_v(level, tag, fmt, args);
	va_end(args);
}

class FakeStripe : public lite::stripe::Base {
public:
	explicit FakeStripe(u16 channel_count) {
		if (channel_count > k_max_channels) {
			channel_count = k_max_channels;
		}

		this->size_ = channel_count;
		this->data_ = buf_;
		this->clr(0u);
	}

	[[nodiscard]] u8 at(u16 idx) const {
		if (idx >= this->size_) {
			return 0u;
		}

		return buf_[idx];
	}

private:
	static constexpr u16 k_max_channels = 32u;
	u8 buf_[k_max_channels] = {};
};

void prepare_timer_service() {
	FakeClock::begin_window();
	while (lite::SpinTimerService::instance().spin(FakeClock::now())) {}
}

void test_compile_time_helpers_runtime_contract() {
	TEST_ASSERT_EQUAL_UINT32(6u, lite::gcd_u32(48u, 18u));

	const u8 values[] = {1u, 2u, 3u, 4u};
	TEST_ASSERT_EQUAL_UINT32(4u, static_cast<u32>(lite::array_count(values)));
}

void test_changed_updates_only_on_change() {
	int value = 7;

	TEST_ASSERT_FALSE(lite::changed(value, 7));
	TEST_ASSERT_EQUAL_INT(7, value);

	TEST_ASSERT_TRUE(lite::changed(value, 11));
	TEST_ASSERT_EQUAL_INT(11, value);
}

void test_changed_accepts_move_only_source() {
	MoveOnlyValue value{1};

	TEST_ASSERT_TRUE(lite::changed(value, MoveOnlyValue{5}));
	TEST_ASSERT_EQUAL_INT(5, value.value());
}

void test_msg_call_if_empty_returns_false() {
	lite::Msg msg;

	TEST_ASSERT_FALSE(msg.is_valid());
	TEST_ASSERT_FALSE(msg.call_if());
}

void test_msg_bind_no_arg_member() {
	MsgProbe probe;
	lite::Msg msg = MSG_BIND(&probe, on_no_args);

	TEST_ASSERT_TRUE(msg.is_valid());
	msg.call();
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
}

void test_msg_bind_one_arg_member() {
	MsgProbe probe;
	lite::Msg msg;
	msg = MSG_BIND(&probe, on_one_arg, 77u);
	TEST_ASSERT_TRUE(msg.is_valid());
	TEST_ASSERT_TRUE(msg.call_if());
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
	TEST_ASSERT_EQUAL_UINT32(77u, probe.last0);
}

void test_msg_bind_two_arg_member() {
	MsgProbe probe;
	lite::Msg msg = lite::Msg::bind<&MsgProbe::on_two_args>(&probe, 7u, 9u);

	msg.call();
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
	TEST_ASSERT_EQUAL_UINT32(7u, probe.last0);
	TEST_ASSERT_EQUAL_UINT32(9u, probe.last1);
}

void test_msg_clear_invalidates_message() {
	MsgProbe probe;
	lite::Msg msg = MSG_BIND(&probe, on_no_args);

	TEST_ASSERT_TRUE(msg.is_valid());
	msg.clear();
	TEST_ASSERT_FALSE(msg.is_valid());
	TEST_ASSERT_FALSE(msg.call_if());
	TEST_ASSERT_EQUAL_INT(0, probe.calls);
}

void test_event_bus_publish_empty_is_noop() {
	lite::EventBus<EventBusTestEvent> bus;
	bus.publish(EventBusTestEvent{1u, 2u});
}

void test_event_hook_callback_receives_event() {
	lite::EventBus<EventBusTestEvent> bus;
	EventBusProbe probe;
	lite::EventHook<EventBusTestEvent> hook(bus);

	hook.attach<&EventBusProbe::on_event>(&probe);

	TEST_ASSERT_TRUE(hook.is_attached());

	bus.publish(EventBusTestEvent{7u, 9u});
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
	TEST_ASSERT_EQUAL_UINT8(7u, probe.last_id);
	TEST_ASSERT_EQUAL_UINT8(9u, probe.last_p1);
}

void test_event_hook_supports_no_arg_member_callback() {
	lite::EventBus<EventBusTestEvent> bus;
	EventBusProbe probe;
	lite::EventHook<EventBusTestEvent> hook(bus);

	hook.attach<&EventBusProbe::on_event_no_args>(&probe);
	bus.publish(EventBusTestEvent{3u, 4u});
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
}

void test_event_hook_detach_detaches_callback() {
	lite::EventBus<EventBusTestEvent> bus;
	EventBusProbe probe;
	lite::EventHook<EventBusTestEvent> hook(bus);

	hook.attach<&EventBusProbe::on_event>(&probe);
	bus.publish(EventBusTestEvent{1u, 1u});
	TEST_ASSERT_EQUAL_INT(1, probe.calls);

	hook.detach();
	TEST_ASSERT_FALSE(hook.is_attached());
	bus.publish(EventBusTestEvent{2u, 2u});
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
}

void test_event_bus_dispatch_order_follows_registration() {
	lite::EventBus<EventBusTestEvent> bus;
	int first = 0;
	int second = 0;
	lite::EventHook<EventBusTestEvent> hook_first(bus);
	lite::EventHook<EventBusTestEvent> hook_second(bus);

	hook_first.attach(&event_bus_mark_one, &first);
	hook_second.attach(&event_bus_mark_two, &second);

	bus.publish(EventBusTestEvent{1u, 0u});
	TEST_ASSERT_EQUAL_INT(1, first);
	TEST_ASSERT_EQUAL_INT(2, second);
}

void test_event_hook_auto_detach_on_dtor() {
	lite::EventBus<EventBusTestEvent> bus;
	EventBusProbe probe;

	{
		lite::EventHook<EventBusTestEvent> hook(bus);
		hook.attach<&EventBusProbe::on_event>(&probe);
		bus.publish(EventBusTestEvent{1u, 1u});
		TEST_ASSERT_EQUAL_INT(1, probe.calls);
	}

	bus.publish(EventBusTestEvent{2u, 2u});
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
}

void test_spin_timer_one_shot_fires_once() {
	prepare_timer_service();

	TimerProbe probe;
	lite::SpinTimer timer(MSG_BIND(&probe, on_timeout));
	timer.start(25u);

	TEST_ASSERT_TRUE(timer.is_running());

	FakeClock::advance(24u);
	TEST_ASSERT_FALSE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(0, probe.call_count);

	FakeClock::advance(1u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
	TEST_ASSERT_EQUAL_UINT32(FakeClock::now(), probe.last_fire_tick);
	TEST_ASSERT_FALSE(timer.is_running());
	TEST_ASSERT_EQUAL_UINT32(
		lite::SpinTimerService::NEVER,
		lite::SpinTimerService::instance().next_delay(FakeClock::now())
	);
}

void test_spin_timer_start_if_not_running_does_not_replace_active_run() {
	prepare_timer_service();

	TimerProbe probe;
	lite::SpinTimer timer(MSG_BIND(&probe, on_timeout));
	timer.start(30u);

	TEST_ASSERT_FALSE(timer.start_if_not_running(10u));
	TEST_ASSERT_EQUAL_UINT32(30u, lite::SpinTimerService::instance().next_delay(FakeClock::now()));

	FakeClock::advance(30u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);

	TEST_ASSERT_TRUE(timer.start_if_not_running(10u));
	FakeClock::advance(10u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(2, probe.call_count);
}

void test_spin_timer_periodic_reschedules_without_drift() {
	prepare_timer_service();

	TimerProbe probe;
	lite::SpinTimer timer(MSG_BIND(&probe, on_periodic));
	timer.start_periodic(20u);

	FakeClock::advance(20u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
	TEST_ASSERT_TRUE(timer.is_running());
	TEST_ASSERT_EQUAL_UINT32(20u, lite::SpinTimerService::instance().next_delay(FakeClock::now()));

	FakeClock::advance(45u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(2, probe.call_count);
	TEST_ASSERT_TRUE(timer.is_running());
	TEST_ASSERT_EQUAL_UINT32(15u, lite::SpinTimerService::instance().next_delay(FakeClock::now()));
}

void test_spin_timer_stop_self_suppresses_periodic_reinsert() {
	prepare_timer_service();

	TimerProbe probe;
	lite::SpinTimer timer(MSG_BIND(&probe, on_stop_self));
	probe.timer = &timer;
	timer.start_periodic(10u);

	FakeClock::advance(10u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
	TEST_ASSERT_FALSE(timer.is_running());
	TEST_ASSERT_EQUAL_UINT32(
		lite::SpinTimerService::NEVER,
		lite::SpinTimerService::instance().next_delay(FakeClock::now())
	);

	FakeClock::advance(20u);
	TEST_ASSERT_FALSE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
}

void test_timer_wrapper_one_shot_uses_duration() {
	prepare_timer_service();

	TimerProbe probe;
	lite::Timer timer(MSG_BIND(&probe, on_timeout));
	timer.start(lite::duration_ms{10});

	FakeClock::advance(9u);
	TEST_ASSERT_FALSE(lite::Timer::spin(lite::duration_ms{FakeClock::now()}));
	TEST_ASSERT_EQUAL_INT(0, probe.call_count);

	FakeClock::advance(1u);
	TEST_ASSERT_TRUE(lite::Timer::spin(lite::duration_ms{FakeClock::now()}));
	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
}

void test_timer_ticks_converts_duration() {
	TEST_ASSERT_EQUAL_UINT32(0u, lite::Timer::ticks(lite::duration_ms{0}));
	TEST_ASSERT_EQUAL_UINT32(25u, lite::Timer::ticks(lite::duration_ms{25}));
	TEST_ASSERT_EQUAL_UINT32(1000u, lite::Timer::ticks(lite::duration_ms{1000}));
}

void test_args_parses_tokens_and_numbers() {
	char line[] = "cmd  42 ff";
	lite::Args args(line);

	const char* cmd = args.get_str();
	TEST_ASSERT_NOT_NULL(cmd);
	TEST_ASSERT_EQUAL_STRING("cmd", cmd);

	TEST_ASSERT_EQUAL_UINT16(42u, args.get_u16());
	TEST_ASSERT_EQUAL_UINT16(0xffu, args.get_hex16());
	TEST_ASSERT_TRUE(args.is_empty());
}

void test_args_peek_and_default_values() {
	char line[] = "set 15";
	lite::Args args(line);

	TEST_ASSERT_TRUE(args.peek("set"));
	TEST_ASSERT_FALSE(args.peek("get"));
	TEST_ASSERT_EQUAL_STRING("set", args.get_str());
	TEST_ASSERT_EQUAL_UINT16(15u, args.get_u16(99u));
	TEST_ASSERT_EQUAL_UINT16(99u, args.get_u16(99u));
}

void test_cmd_shell_dispatches_registered_command() {
	CaptureOut out;
	lite::CmdShell shell;
	CmdProbe probe;
	lite::Cmd cmd_set(
		shell,
		"set",
		"set a value",
		"<num>",
		METHOD_BIND(&probe, on_set)
	);

	char line[] = "set 77";
	shell.process(out, line);

	TEST_ASSERT_EQUAL_INT(1, probe.call_count);
	TEST_ASSERT_EQUAL_UINT16(77u, probe.last_value);
}

void test_cmd_shell_unknown_command_reports_usage() {
	CaptureOut out;
	lite::CmdShell shell;

	char line[] = "unknown";
	shell.process(out, line);

	TEST_ASSERT_TRUE(out.contains("unknown?\r\n"));
	TEST_ASSERT_TRUE(out.contains("show command help"));
}

void test_console_ready_and_processes_line() {
	CaptureOut out;
	RecordingShell shell;
	lite::Console console(shell, out);

	console.ready();
	TEST_ASSERT_EQUAL_INT(1, shell.banner_calls);
	TEST_ASSERT_EQUAL_INT(1, shell.prompt_calls);

	console.process('a');
	console.process('b');
	console.process('\b');
	console.process('c');
	console.process('\r');

	TEST_ASSERT_EQUAL_INT(1, shell.process_calls);
	TEST_ASSERT_EQUAL_STRING("ac", shell.last_line);
	TEST_ASSERT_EQUAL_INT(2, shell.prompt_calls);
}

void test_macros_xstr_expands_macro_before_stringify() {
#define LITEKIT_TEST_VALUE 42
	TEST_ASSERT_EQUAL_STRING("42", XSTR(LITEKIT_TEST_VALUE));
#undef LITEKIT_TEST_VALUE
}

void test_macros_xcat_expands_before_paste() {
#define TEST_PREFIX pre_
#define TEST_SUFFIX fix
	int pre_fix = 7;
	TEST_ASSERT_EQUAL_INT(7, XCAT(TEST_PREFIX, TEST_SUFFIX));
#undef TEST_PREFIX
#undef TEST_SUFFIX
}

void test_macros_xcat3_concatenates_three_tokens() {
	int foo_bar_baz = 5;
	TEST_ASSERT_EQUAL_INT(5, XCAT3(foo_, bar_, baz));
}

void test_rgb8_hex_and_saturating_math() {
	const lite::Rgb8 from_hex = lite::Rgb8::from_hex(0x123456u);
	TEST_ASSERT_EQUAL_UINT8(0x12u, from_hex.r);
	TEST_ASSERT_EQUAL_UINT8(0x34u, from_hex.g);
	TEST_ASSERT_EQUAL_UINT8(0x56u, from_hex.b);

	const lite::Rgb8 add = lite::Rgb8{250u, 10u, 100u} + 20u;
	TEST_ASSERT_EQUAL_UINT8(255u, add.r);
	TEST_ASSERT_EQUAL_UINT8(30u, add.g);
	TEST_ASSERT_EQUAL_UINT8(120u, add.b);

	const lite::Rgb8 sub = lite::Rgb8{5u, 2u, 0u} - 10u;
	TEST_ASSERT_EQUAL_UINT8(0u, sub.r);
	TEST_ASSERT_EQUAL_UINT8(0u, sub.g);
	TEST_ASSERT_EQUAL_UINT8(0u, sub.b);

	const lite::Rgb8 mul = lite::Rgb8{255u, 128u, 1u} * 128u;
	TEST_ASSERT_EQUAL_UINT8(128u, mul.r);
	TEST_ASSERT_EQUAL_UINT8(64u, mul.g);
	TEST_ASSERT_EQUAL_UINT8(1u, mul.b);
}

void test_gamma_lut_default_endpoints() {
	TEST_ASSERT_EQUAL_UINT16(0u, lite::gamma_u8_to_u10(0u));
	TEST_ASSERT_EQUAL_UINT16(1023u, lite::gamma_u8_to_u10(255u));

	TEST_ASSERT_EQUAL_UINT8(0u, lite::gamma_u8_to_u8(0u));
	TEST_ASSERT_EQUAL_UINT8(255u, lite::gamma_u8_to_u8(255u));
}

void test_gamma_lut_u10_monotonic() {
	u16 prev = lite::gamma_u8_to_u10(0u);

	for (u16 i = 1u; i <= 255u; ++i) {
		const u16 cur = lite::gamma_u8_to_u10(static_cast<u8>(i));
		TEST_ASSERT_TRUE(cur >= prev);
		prev = cur;
	}
}

void test_log_logger_list_singleton_is_stable() {
	auto* first = &lite::log::LoggerList::instance();
	auto* second = &lite::log::LoggerList::instance();

	TEST_ASSERT_EQUAL_PTR(first, second);
}

void test_log_logger_receives_write_and_unregisters() {
	LogCapture cap = {};
	{
		CaptureLogger logger(cap);

		write_with_list(TOOLKIT_LOG_LEVEL_info, "tlog", "hello");

		TEST_ASSERT_EQUAL_INT(1, cap.count);
		TEST_ASSERT_EQUAL_UINT8(TOOLKIT_LOG_LEVEL_info, cap.last_level);
		TEST_ASSERT_EQUAL_STRING("tlog", cap.last_tag);
		TEST_ASSERT_EQUAL_STRING("hello", cap.last_fmt);
	}

	write_with_list(TOOLKIT_LOG_LEVEL_info, "tlog", "gone");
	TEST_ASSERT_EQUAL_INT(1, cap.count);
}

void test_log_multiple_loggers_fan_out() {
	LogCapture cap_a = {};
	LogCapture cap_b = {};
	CaptureLogger logger_a(cap_a);
	CaptureLogger logger_b(cap_b);

	write_with_list(TOOLKIT_LOG_LEVEL_warning, "tlog", "broadcast");

	TEST_ASSERT_EQUAL_INT(1, cap_a.count);
	TEST_ASSERT_EQUAL_INT(1, cap_b.count);
}

void test_stripe_mapping_grb_maps_channels_and_clear_color() {
	using GrbStripe = lite::stripe::Mapping::grb<FakeStripe>;
	GrbStripe stripe(2u);

	stripe.set(static_cast<u16>(0u), lite::Rgb8{1u, 2u, 3u});
	stripe.set(static_cast<u16>(1u), lite::Rgb8{4u, 5u, 6u});

	TEST_ASSERT_EQUAL_UINT8(2u, stripe.at(0u));
	TEST_ASSERT_EQUAL_UINT8(1u, stripe.at(1u));
	TEST_ASSERT_EQUAL_UINT8(3u, stripe.at(2u));
	TEST_ASSERT_EQUAL_UINT8(5u, stripe.at(3u));
	TEST_ASSERT_EQUAL_UINT8(4u, stripe.at(4u));
	TEST_ASSERT_EQUAL_UINT8(6u, stripe.at(5u));

	stripe.clr(lite::Rgb8{9u, 8u, 7u});
	TEST_ASSERT_EQUAL_UINT8(8u, stripe.at(0u));
	TEST_ASSERT_EQUAL_UINT8(9u, stripe.at(1u));
	TEST_ASSERT_EQUAL_UINT8(7u, stripe.at(2u));
	TEST_ASSERT_EQUAL_UINT8(8u, stripe.at(3u));
	TEST_ASSERT_EQUAL_UINT8(9u, stripe.at(4u));
	TEST_ASSERT_EQUAL_UINT8(7u, stripe.at(5u));
}

void run_all_tests() {
	RUN_TEST(test_compile_time_helpers_runtime_contract);
	RUN_TEST(test_changed_updates_only_on_change);
	RUN_TEST(test_changed_accepts_move_only_source);

	RUN_TEST(test_msg_call_if_empty_returns_false);
	RUN_TEST(test_msg_bind_no_arg_member);
	RUN_TEST(test_msg_bind_one_arg_member);
	RUN_TEST(test_msg_bind_two_arg_member);
	RUN_TEST(test_msg_clear_invalidates_message);
	RUN_TEST(test_event_bus_publish_empty_is_noop);
	RUN_TEST(test_event_hook_callback_receives_event);
	RUN_TEST(test_event_hook_supports_no_arg_member_callback);
	RUN_TEST(test_event_hook_detach_detaches_callback);
	RUN_TEST(test_event_bus_dispatch_order_follows_registration);
	RUN_TEST(test_event_hook_auto_detach_on_dtor);

	RUN_TEST(test_spin_timer_one_shot_fires_once);
	RUN_TEST(test_spin_timer_start_if_not_running_does_not_replace_active_run);
	RUN_TEST(test_spin_timer_periodic_reschedules_without_drift);
	RUN_TEST(test_spin_timer_stop_self_suppresses_periodic_reinsert);
	RUN_TEST(test_timer_wrapper_one_shot_uses_duration);
	RUN_TEST(test_timer_ticks_converts_duration);

	RUN_TEST(test_args_parses_tokens_and_numbers);
	RUN_TEST(test_args_peek_and_default_values);
	RUN_TEST(test_cmd_shell_dispatches_registered_command);
	RUN_TEST(test_cmd_shell_unknown_command_reports_usage);
	RUN_TEST(test_console_ready_and_processes_line);

	RUN_TEST(test_macros_xstr_expands_macro_before_stringify);
	RUN_TEST(test_macros_xcat_expands_before_paste);
	RUN_TEST(test_macros_xcat3_concatenates_three_tokens);

	RUN_TEST(test_rgb8_hex_and_saturating_math);
	RUN_TEST(test_gamma_lut_default_endpoints);
	RUN_TEST(test_gamma_lut_u10_monotonic);

	RUN_TEST(test_log_logger_list_singleton_is_stable);
	RUN_TEST(test_log_logger_receives_write_and_unregisters);
	RUN_TEST(test_log_multiple_loggers_fan_out);

	RUN_TEST(test_stripe_mapping_grb_maps_channels_and_clear_color);
}

} // namespace

void setUp(void) {}

void tearDown(void) {}

#if defined(ARDUINO)

void setup() {
	delay(2000);
	UNITY_BEGIN();
	run_all_tests();
	UNITY_END();
}

void loop() {}

#else

int main(int argc, char** argv) {
	(void)argc;
	(void)argv;

	UNITY_BEGIN();
	run_all_tests();
	return UNITY_END();
}

#endif
