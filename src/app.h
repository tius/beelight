//  main application entry point
//
//  see LICENSE for terms

#include "settings.h"
#include "app_event.h"
#include "rgb_show.h"
#include "stripe.h"
#include "infrared.h"
#include "light_meter.h"
#include "acc_meter.h"
#include "event_logger.h"
#include "wake_morse.h"
#include "wake_info.h"

#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"
#include "lite/sys/clock.h"
#include "lite/sys/rtc_mem.h"
#include "lite/core/event_bus.h"
#include "lite/core/timer.h"
#include "lite/cmd/sys_cmd.h"
#include "lite/cmd/twi_cmd.h"

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
        wake_morse_.tick();

		//  execute due timers
		lite::Timer::spin( lite::now() );

		//  process serial input
		if (auto c = uart_.rx(); c >= 0) {
			console_.process(char(c));
		}
        stripe_.tick();
        infrared_.tick();
	}

//------------------------------------------------------------------------------
private:
    using AppLogger = lite::CustomLogger<LOG_ANSI_COLOR, LOG_TIMESTAMP, LOG_LEVEL_PREFIX>;
    using EventBus  = lite::EventBus<AppEvent>;
    using RtcMem    = lite::sys::RtcMem;
    template <typename X>
    using RtcVar    = lite::sys::RtcVar<X>;

    enum class RtcAddr : u8 {
        RTC_ALLOC(wake_uptime, u32),
        RTC_ALLOC(wake_morse, WakeMorseState),
        RTC_ALLOC(test, u32),
        count,
    };
    EventBus            event_bus_  {};

    RtcMem              rtc_mem_    {RTC_COUNT};
    RtcVar<u32>         rtc_wake_uptime_{rtc_mem_, RTC_SLOT(wake_uptime)};
    RtcVar<WakeMorseState> rtc_wake_morse_{rtc_mem_, RTC_SLOT(wake_morse)};
    RtcVar<u32>         rtc_test_   {rtc_mem_, RTC_SLOT(test)};

    WakeMorse           wake_morse_ {rtc_wake_morse_, event_bus_};
    WakeInfo<WakeMorse> wake_info_  {rtc_wake_uptime_, wake_morse_};
    
    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    lite::CmdShell      shell_      {};
    lite::Console       console_    {shell_, serial_out_};
    lite::cmd::SysCmd   cmd_sys_    {shell_};

    EventLogger         event_logger_{event_bus_};

    lite::Twi           twi_        {I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_CLOCK_HZ};
    lite::cmd::TwiCmd   twi_cmd_    {shell_, twi_};

    LightMeter          light_meter_{twi_, event_bus_};
    AccMeter            acc_meter_  {twi_, event_bus_};
    Infrared            infrared_   {event_bus_};

    RgbLed              rgb_led_    {};
    RgbShow             rgb_show_   {rgb_led_};
    lite::Cmd           cmd_led_    {shell_, "led", "set rgb status state", "<state>", METHOD_THIS(on_cmd_led)};
    Stripe              stripe_     {};

    lite::Timer         timer_      { MSG_THIS(on_timer) };
    
    App() {
        update_boot_count();

        if (!light_meter_) {
            LOG_ERROR("light meter not available: %s", light_meter_.status().str());
        }
        if (!acc_meter_) {
            LOG_ERROR("acc meter not available: %s", acc_meter_.status().str());
        }
        if (!infrared_) {
            LOG_ERROR("infrared not responding: %s", infrared_.status().str());
        }

        lite::std_out->println(APP_BANNER_TEXT);

        console_.ready();
        rgb_show_.set(RgbState::CHARGE);

        stripe_.clr( lite::k_rgb_red );
        stripe_.update();

        // timer_.start(60s);
    }

    // prevent copying
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void on_cmd_led(lite::Out& out, lite::Args args) {
        (void)out;
        rgb_show_.set( args.get_u16() );
    }

    void update_boot_count() {
        auto boot_count = rtc_test_.get();
        ++boot_count;
        rtc_test_ = boot_count;

        LOG_INFO("boot count: %lu", static_cast<unsigned long>(boot_count));
    }

    void on_timer() {
        lite::sys::deep_sleep();
    }
};
