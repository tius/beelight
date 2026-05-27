//  app runtime
//
//  see LICENSE for terms

#pragma once

#include "settings.h"
#include "rtc.h"
#include "boot/request.h"
#include "event/event.h"
#include "event/logger.h"
#include "status_led/rgb_show.h"
#include "front_leds/stripe.h"
#include "ir/infrared.h"
#include "sensor/light_meter.h"
#include "sensor/acc_meter.h"
#include "wake/wake_morse.h"
#include "wake/wake_info.h"

#include "lite/cli/cmd.h"
#include "lite/cli/console.h"
#include "lite/io/serial_out.h"
#include "lite/io/log.h"
#include "lite/sys/clock.h"
#include "lite/core/timer.h"
#include "lite/cmd/sys_cmd.h"
#include "lite/cmd/twi_cmd.h"

#define LOG_TAG         app
#define LOG_LEVEL       trace

//==============================================================================
class AppRun {
//------------------------------------------------------------------------------
public:
    AppRun() {
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
    AppRun(const AppRun&) = delete;
    AppRun& operator=(const AppRun&) = delete;

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
    using EventBus  = event::Bus;

    EventBus            event_bus_  {};

    WakeMorse           wake_morse_ {rtc::wake_morse(), event_bus_};
    WakeInfo<WakeMorse> wake_info_  {rtc::wake_uptime(), wake_morse_};
    
    lite::Uart          uart_       {MONITOR_SPEED};
    lite::SerialOut     serial_out_ {uart_, "\n-----\n"};
    lite::StdOut        std_out_    {serial_out_};
    AppLogger           logger_     {serial_out_};

    lite::CmdShell      shell_      {};
    lite::Console       console_    {shell_, serial_out_};
    lite::cmd::SysCmd   cmd_sys_    {shell_};

    event::Logger       event_logger_{event_bus_};
    event::Hook         boot_hook_  {event_bus_, METHOD_THIS(on_event)};

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
    
    void on_cmd_led(lite::Out& out, lite::Args args) {
        (void)out;
        rgb_show_.set( args.get_u16() );
    }

    void on_event(const event::Event& event) {
        if (event.id != event::Id::MORSE_CMD) {
            return;
        }

        on_morse_cmd(event.p1.morse_cmd);
    }

    void on_morse_cmd(const event::MorseCmd& cmd) {
        if (!cmd.is("HS")) {
            return;
        }

        boot::reboot(boot::Mode::hotspot);
    }

    void update_boot_count() {
        auto rtc_boot_count = rtc::boot_count();
        auto boot_count = rtc_boot_count.get();
        ++boot_count;
        rtc_boot_count = boot_count;

        LOG_INFO("boot count: %lu", static_cast<unsigned long>(boot_count));
    }

    void on_timer() {
        lite::sys::deep_sleep();
    }
};
