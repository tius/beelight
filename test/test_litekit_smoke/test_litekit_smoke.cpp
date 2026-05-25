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

#include "../../src/driver/ir_tx_base.h"

namespace {

class NullOut final : public lite::Out {
public:
	std::size_t write(char) override {
		return 1u;
	}
};

class IrTxProbe : public IrTxBase<IrTxProbe> {
public:
	void send_data_frame(lite::u32 raw_frame, lite::u16 mark_us, lite::u16 space_us) {
		raw = raw_frame;
		mark = mark_us;
		space = space_us;
		++data_cnt;
	}

	void send_repeat_frame(lite::u16 mark_us, lite::u16 space_us) {
		mark = mark_us;
		space = space_us;
		++repeat_cnt;
	}

	lite::u32 raw = 0u;
	lite::u16 mark = 0u;
	lite::u16 space = 0u;
	lite::u8 data_cnt = 0u;
	lite::u8 repeat_cnt = 0u;
};

static_assert(sizeof(IrCode) == sizeof(lite::u32));
static_assert(alignof(IrCode) == alignof(lite::u8));
static_assert(std::is_trivially_copyable_v<IrCode>);
static_assert(IrCode{}.is_invalid());
static_assert(IrCode::repeat().raw() == 0xffffffffu);

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

void test_ir_code_special_values() {
	TEST_ASSERT_TRUE(IrCode{}.is_invalid());
	TEST_ASSERT_TRUE(IrCode::invalid().is_invalid());
	TEST_ASSERT_FALSE(IrCode::invalid().is_valid());

	TEST_ASSERT_TRUE(IrCode::repeat().is_repeat());
	TEST_ASSERT_TRUE(IrCode::repeat().is_valid());
	TEST_ASSERT_FALSE(IrCode::repeat().is_data());
	TEST_ASSERT_EQUAL_HEX32(0xffffffffu, IrCode::repeat().raw());
}

void test_ir_code_nec_roundtrip() {
	const IrCode code = IrCode::from_nec(0x12u, 0x34u);
	IrCode::Nec nec {};

	TEST_ASSERT_EQUAL_HEX32(0xcb34ed12u, code.raw());
	TEST_ASSERT_TRUE(code.is_nec());
	TEST_ASSERT_TRUE(code.decode_nec(nec));
	TEST_ASSERT_EQUAL_HEX8(0x12u, nec.addr);
	TEST_ASSERT_EQUAL_HEX8(0x34u, nec.cmd);
}

void test_ir_code_rejects_non_nec_values() {
	IrCode::Nec nec { .addr = 0x12u, .cmd = 0x34u };
	const IrCode raw_code = IrCode::from_raw(0x12345678u);

	TEST_ASSERT_TRUE(raw_code.is_data());
	TEST_ASSERT_TRUE(raw_code.is_valid());
	TEST_ASSERT_FALSE(raw_code.is_nec());
	TEST_ASSERT_FALSE(raw_code.decode_nec(nec));
	TEST_ASSERT_EQUAL_HEX8(0x12u, nec.addr);
	TEST_ASSERT_EQUAL_HEX8(0x34u, nec.cmd);

	TEST_ASSERT_FALSE(IrCode::invalid().decode_nec(nec));
	TEST_ASSERT_FALSE(IrCode::repeat().decode_nec(nec));
}

void test_ir_tx_accepts_ir_code_data() {
	IrTxProbe tx;

	tx.tx(IrCode::from_nec(0x12u, 0x34u));

	TEST_ASSERT_EQUAL_UINT8(1u, tx.data_cnt);
	TEST_ASSERT_EQUAL_UINT8(0u, tx.repeat_cnt);
	TEST_ASSERT_EQUAL_HEX32(0xcb34ed12u, tx.raw);
	TEST_ASSERT_EQUAL_UINT16(9000u, tx.mark);
	TEST_ASSERT_EQUAL_UINT16(4500u, tx.space);
}

void test_ir_tx_maps_special_codes() {
	IrTxProbe tx;

	tx.tx(IrCode::invalid());
	TEST_ASSERT_EQUAL_UINT8(0u, tx.data_cnt);
	TEST_ASSERT_EQUAL_UINT8(0u, tx.repeat_cnt);

	tx.tx(IrCode::repeat());
	TEST_ASSERT_EQUAL_UINT8(0u, tx.data_cnt);
	TEST_ASSERT_EQUAL_UINT8(1u, tx.repeat_cnt);
	TEST_ASSERT_EQUAL_UINT16(9000u, tx.mark);
	TEST_ASSERT_EQUAL_UINT16(2250u, tx.space);
}

void test_ir_tx_raw_keeps_raw_frame_semantics() {
	IrTxProbe tx;

	tx.tx_raw(IrCode::repeat().raw());

	TEST_ASSERT_EQUAL_UINT8(1u, tx.data_cnt);
	TEST_ASSERT_EQUAL_UINT8(0u, tx.repeat_cnt);
	TEST_ASSERT_EQUAL_HEX32(0xffffffffu, tx.raw);
}

void run_all_tests() {
	RUN_TEST(test_overlay_clock_and_timer_contract);
	RUN_TEST(test_overlay_uart_and_serial_out_contract);
	RUN_TEST(test_std_out_guard_restores_previous_pointer);
	RUN_TEST(test_ir_code_special_values);
	RUN_TEST(test_ir_code_nec_roundtrip);
	RUN_TEST(test_ir_code_rejects_non_nec_values);
	RUN_TEST(test_ir_tx_accepts_ir_code_data);
	RUN_TEST(test_ir_tx_maps_special_codes);
	RUN_TEST(test_ir_tx_raw_keeps_raw_frame_semantics);
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
