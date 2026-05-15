#if defined(ARDUINO)
	#include <Arduino.h>
#endif

#include <unity.h>

#include "lite/cli/args.h"
#include "lite/color/rgb8.h"
#include "lite/core/changed.h"
#include "lite/core/msg.h"
#include "lite/core/spin_timer.h"

namespace {

using lite::u32;

class MsgProbe final {
public:
	void on_one_arg(u32 value) {
		calls++;
		last_val = value;
	}

	int calls = 0;
	u32 last_val = 0u;
};

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
		calls++;
		last_tick = FakeClock::now();
	}

	int calls = 0;
	u32 last_tick = 0u;
};

void prepare_timer_service() {
	FakeClock::begin_window();
	while (lite::SpinTimerService::instance().spin(FakeClock::now())) {}
}

void test_changed_updates_only_on_change() {
	int value = 7;

	TEST_ASSERT_FALSE(lite::changed(value, 7));
	TEST_ASSERT_EQUAL_INT(7, value);

	TEST_ASSERT_TRUE(lite::changed(value, 11));
	TEST_ASSERT_EQUAL_INT(11, value);
}

void test_msg_bind_and_call_if() {
	MsgProbe probe;
	lite::Msg msg;

	TEST_ASSERT_FALSE(msg.is_valid());
	TEST_ASSERT_FALSE(msg.call_if());

	msg = MSG_BIND(&probe, on_one_arg, 77u);
	TEST_ASSERT_TRUE(msg.is_valid());
	TEST_ASSERT_TRUE(msg.call_if());
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
	TEST_ASSERT_EQUAL_UINT32(77u, probe.last_val);
}

void test_spin_timer_one_shot_fires_once() {
	prepare_timer_service();

	TimerProbe probe;
	lite::SpinTimer timer(MSG_BIND(&probe, on_timeout));
	timer.start(25u);

	TEST_ASSERT_TRUE(timer.is_running());

	FakeClock::advance(24u);
	TEST_ASSERT_FALSE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(0, probe.calls);

	FakeClock::advance(1u);
	TEST_ASSERT_TRUE(lite::SpinTimerService::instance().spin(FakeClock::now()));
	TEST_ASSERT_EQUAL_INT(1, probe.calls);
	TEST_ASSERT_EQUAL_UINT32(FakeClock::now(), probe.last_tick);
	TEST_ASSERT_FALSE(timer.is_running());
	TEST_ASSERT_EQUAL_UINT32(
		lite::SpinTimerService::NEVER,
		lite::SpinTimerService::instance().next_delay(FakeClock::now())
	);
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

void run_all_tests() {
	RUN_TEST(test_changed_updates_only_on_change);
	RUN_TEST(test_msg_bind_and_call_if);
	RUN_TEST(test_spin_timer_one_shot_fires_once);
	RUN_TEST(test_args_parses_tokens_and_numbers);
	RUN_TEST(test_rgb8_hex_and_saturating_math);
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
