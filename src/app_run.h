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
#include "lite/cli/cmd.h"

#include <cstring>

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
    event::Bus          event_bus_  {};
    WakeMorse           wake_morse_ {rtc::wake_morse(), event_bus_};
    WakeInfo<WakeMorse> wake_info_  {rtc::wake_uptime(), wake_morse_};

    RuntimeCore         core_       {event_bus_};
    event::Hook         boot_hook_  {event_bus_, METHOD_THIS(on_event)};

    LightMeter          light_meter_{
        core_.twi(),
        event_bus_,
        core_.next_timer_offset()
    };
    AccMeter            acc_meter_  {
        core_.twi(),
        event_bus_,
        core_.next_timer_offset()
    };
    Infrared            infrared_   {event_bus_};
    FrontShow           front_show_ {core_.front_leds()};

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
