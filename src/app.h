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
#include "lite/sys/sys_cmd.h"

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
        stripe_.tick();
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
    Stripe              stripe_     {};

    lite::Timer         timer_      { MSG_THIS(on_timer_) };

    App() {
        lite::std_out->println(APP_BANNER_TEXT);
        console_.ready();
        rgb_show_.set(RgbState::CHARGE);

        stripe_.clr( lite::k_rgb_red );
        stripe_.update();

        timer_.start(60s);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_cmd_led_(lite::Out& out, lite::Args args) {
        (void)out;
        rgb_show_.set( args.get_u16() );
    }

    void on_timer_() {
        lite::sys::deep_sleep();
    }
};
