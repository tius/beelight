//  app runtime
//
//  see LICENSE for terms

#pragma once

#include "settings.h"
#include "rtc.h"
#include "runtime_core.h"
#include "boot/request.h"
#include "event/event.h"
#include "front_leds/front_show.h"
#include "ir/infrared.h"
#include "sensor/light_meter.h"
#include "sensor/acc_meter.h"
#include "wake/wake_morse.h"
#include "wake/wake_info.h"

#include "lite/io/log.h"
#include "lite/sys/deep_sleep.h"
#include "lite/core/timer.h"
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

        core_.ready();

        front_show_.start();

        // timer_.start(60s);
    }

    // prevent copying
    AppRun(const AppRun&) = delete;
    AppRun& operator=(const AppRun&) = delete;

	void loop() {
        wake_morse_.tick();
        core_.tick();
        infrared_.tick();
	}

//------------------------------------------------------------------------------
private:
    RuntimeCore         core_       {};

    WakeMorse           wake_morse_ {rtc::wake_morse(), core_.event_bus()};
    WakeInfo<WakeMorse> wake_info_  {rtc::wake_uptime(), wake_morse_};
    event::Hook         boot_hook_  {core_.event_bus(), METHOD_THIS(on_event)};

    lite::Twi           twi_        {I2C_SDA_GPIO, I2C_SCL_GPIO, I2C_CLOCK_HZ};
    lite::cmd::TwiCmd   twi_cmd_    {core_.shell(), twi_};

    LightMeter          light_meter_{twi_, core_.event_bus()};
    AccMeter            acc_meter_  {twi_, core_.event_bus()};
    Infrared            infrared_   {core_.event_bus()};
    FrontShow          front_show_  {core_.front_leds()};

    lite::Timer         timer_      { MSG_THIS(on_timer) };

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
