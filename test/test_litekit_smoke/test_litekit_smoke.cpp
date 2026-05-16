#if defined(ARDUINO)
	#include <Arduino.h>
#endif

#include <type_traits>

#include <unity.h>

#include "lite/core/timer.h"
#include "lite/io/out.h"
#include "lite/io/serial_out.h"
#include "lite/sys/clock.h"
#include "lite/sys/uart.h"

namespace {

class NullOut final : public lite::Out {
public:
	std::size_t write(char) override {
		return 1u;
	}
};

void test_overlay_clock_and_timer_contract() {
	TEST_ASSERT_EQUAL_UINT32(10u, lite::Timer::ticks(lite::duration_ms{10}));

	const auto now = lite::now();
	(void)now;
}

void test_overlay_uart_and_serial_out_contract() {
	static_assert(
		std::is_constructible_v<lite::SerialOut, lite::Uart&, const char*>
	);
	TEST_ASSERT_TRUE(true);
}

void test_std_out_guard_restores_previous_pointer() {
	NullOut out_a;
	NullOut out_b;

	TEST_ASSERT_NULL(lite::std_out);

	{
		lite::StdOut guard_a(out_a);
		TEST_ASSERT_EQUAL_PTR(&out_a, lite::std_out);

		{
			lite::StdOut guard_b(out_b);
			TEST_ASSERT_EQUAL_PTR(&out_b, lite::std_out);
		}

		TEST_ASSERT_EQUAL_PTR(&out_a, lite::std_out);
	}

	TEST_ASSERT_NULL(lite::std_out);
}

void run_all_tests() {
	RUN_TEST(test_overlay_clock_and_timer_contract);
	RUN_TEST(test_overlay_uart_and_serial_out_contract);
	RUN_TEST(test_std_out_guard_restores_previous_pointer);
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
