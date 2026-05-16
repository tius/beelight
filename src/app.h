//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "rgb_show.h"
#include "stripe.h"

#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"
#include "lite/sys/clock.h"
#include "lite/core/timer.h"
#include "lite/cli/sys_cmd.h"

#define DECODE_NEC
#define DECODE_HASH

#if IR_RX_ACTIVE_HIGH
    #define IR_INPUT_IS_ACTIVE_HIGH
#endif

#include <IRremote.hpp>

#define LOG_TAG         app
#define LOG_LEVEL       trace

//==============================================================================
class App {
//------------------------------------------------------------------------------
public:
	static App& instance() noexcept {
		static App app;
		return app;
	}    

	void loop() {
		//  execute due timers
		lite::Timer::spin( lite::now() );

		//  process serial input
		if (auto c = uart_.rx(); c >= 0) {
			console_.process(char(c));
		}
        sample_ir_pin_();
        decode_ir_();
        // stripe_.tick();
	}

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP, LOG_LEVEL_PREFIX>;
    
    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    lite::CmdShell      shell_      {};
    lite::Console       console_    {shell_, serial_out_};
    lite::sys::SysCmd   cmd_sys_    {shell_};

    RgbLed              rgb_led_    {};
    RgbShow             rgb_show_   {rgb_led_};
    lite::Cmd           cmd_led_    {shell_, "led", "set rgb status state", "<state>", METHOD_THIS(on_cmd_led_)};
    // Stripe              stripe_     {};

	lite::Timer         ir_diag_timer_ { MSG_THIS(on_ir_diag_timer_) };
    lite::Timer         timer_      { MSG_THIS(on_timer_) };
    unsigned            ir_decode_cnt_ = 0u;
	unsigned            ir_edge_cnt_ = 0u;
	bool                ir_last_lvl_hi_ = true;
    unsigned            ir_last_diag_decode_cnt_ = 0u;
    
    App() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();
        // rgb_show_.set(RgbState::CHARGE);

		IrReceiver.begin(IR_RX_GPIO, DISABLE_LED_FEEDBACK);
        pinMode(IR_RX_GPIO, INPUT_PULLUP);
		LOG_INFO("ir: rx on gpio %u", IR_RX_GPIO);

        #if IR_RX_ACTIVE_HIGH
            LOG_INFO("ir: polarity active-high");
        #else
            LOG_INFO("ir: polarity active-low");
        #endif

		LOG_INFO("ir: press key on remote");
        ir_last_lvl_hi_ = (digitalRead(IR_RX_GPIO) != 0);
        ir_diag_timer_.start_periodic(5s);
	
        // stripe_.clr( lite::k_rgb_red );
        // stripe_.update();

        timer_.start(60s);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_cmd_led_(lite::Out& out, lite::Args args) {
        (void)out;
        // rgb_show_.set( args.get_u16() );
    }

    void sample_ir_pin_() {
        const bool lvl_hi = (digitalRead(IR_RX_GPIO) != 0);
        if (lvl_hi == ir_last_lvl_hi_) {
            return;
        }

        ir_last_lvl_hi_ = lvl_hi;
        ++ir_edge_cnt_;
    }

    void decode_ir_() {
        if (!IrReceiver.decode()) {
            return;
        }

		++ir_decode_cnt_;

        const auto& ir_data = IrReceiver.decodedIRData;
		const bool is_repeat = (ir_data.flags & IRDATA_FLAGS_IS_REPEAT) != 0u;
		LOG_INFO(
            "ir: proto=%u addr=0x%04X cmd=0x%02X raw=0x%llX%s",
			static_cast<unsigned>(ir_data.protocol),
			static_cast<unsigned>(ir_data.address),
			static_cast<unsigned>(ir_data.command),
            static_cast<unsigned long long>(ir_data.decodedRawData),
			is_repeat ? " repeat" : ""
		);

        if (ir_data.protocol == NEC) {
            LOG_INFO(
                "ir nec: addr=0x%04X cmd=0x%02X%s",
                static_cast<unsigned>(ir_data.address),
                static_cast<unsigned>(ir_data.command),
                is_repeat ? " repeat" : ""
            );
        }
        else if (
            ir_data.protocol == UNKNOWN &&
            ir_data.decodedRawData != 0ULL
        ) {
            LOG_INFO(
                "ir hash: key=0x%08llX (not nec)",
                static_cast<unsigned long long>(ir_data.decodedRawData)
            );
        }
        else {
            LOG_INFO("ir: unknown protocol %u", ir_data.protocol);
        }

        IrReceiver.resume();
    }

    void on_ir_diag_timer_() {
		const unsigned dec_in_window =
			ir_decode_cnt_ - ir_last_diag_decode_cnt_;

        LOG_INFO(
            "ir: pin=%u lvl=%u edges=%u dec=%u",
            static_cast<unsigned>(IR_RX_GPIO),
            ir_last_lvl_hi_ ? 1u : 0u,
            ir_edge_cnt_,
            ir_decode_cnt_
        );

		if (ir_edge_cnt_ > 0u && dec_in_window == 0u) {
            #if IR_RX_ACTIVE_HIGH
                LOG_INFO("ir: edges without frame, try IR_RX_ACTIVE_HIGH=false");
            #else
                LOG_INFO("ir: edges without frame, try IR_RX_ACTIVE_HIGH=true");
            #endif
		}

        ir_edge_cnt_ = 0u;
		ir_last_diag_decode_cnt_ = ir_decode_cnt_;
    }

    void on_timer_() {
        lite::sys::deep_sleep();
    }
};
