//  manage wake button
//
//  remarks:
//      - the button is active low and connected to GPIO16
//      - the reset logic triggers a short reset pulse when GPIO16 goes low
//      - gpio16 does not support interrupts
//      - ticker is used to detect button release as timer is not yet ready
//      - ticker keeps rtc uptime fresh until the delayed report fires
//      - recorded times are early-boot observations, not physical edge times
//
//  see LICENSE file for terms

#pragma once

#include <Ticker.h>

#include "settings.h"
#include "app_event.h"
#include "lite/io/log.h"
#include "lite/sys/gpio.h"
#include "lite/sys/clock.h"
#include "lite/sys/rtc_mem.h"
#include "lite/core/timer.h"
#include "lite/core/event_bus.h"
#include "lite/core/method_bind.h"

#define LOG_TAG		button
#define LOG_LEVEL	WAKE_INFO_LOG

//=============================================================================
class WakeInfo final {

using EventBus  = lite::EventBus<AppEvent>;
using WakeUptime = lite::sys::RtcVar<u32>;

//-----------------------------------------------------------------------------
public:
    WakeInfo(WakeUptime& wake_uptime, EventBus& event_bus) noexcept
        : wake_uptime_(wake_uptime), event_bus_(event_bus) 
    {
        //  defer first rtc read to ticker context to keep constructor short
        //  prev_uptime = wake_uptime_.get();

        //  sys context is intentional, loop may start too late
        uptime_ticker_.attach_ms(10, CALLBACK_THIS(on_uptime_tick));
        on_uptime_tick();

        report_timer_.start(lite::duration_ms{2000});
    }


private:
    using ButtonGpio = lite::gpio::Input<WAKE_INFO_GPIO>;

    WakeUptime&     wake_uptime_;
    EventBus&       event_bus_;
    ButtonGpio      button_gpio_;
    
    Ticker          uptime_ticker_;
    lite::Timer     report_timer_ { MSG_THIS(on_report_timer) };

    //  early wake window uses u16 millisecond values by design
    u16             prev_uptime = 0xFFFF;
    u16             button_release_at     = 0xFFFF;

    void on_uptime_tick() {
        //  previous boot uptime is read once, after ticker startup
        if (prev_uptime == 0xFFFF) {
            prev_uptime = (u16) wake_uptime_.get();
        }

        u32 now = lite::now_ms();
        if ( button_gpio_.is_hi() ) {
            if ( button_release_at == 0xFFFF ) {
                //  first observed release after firmware startup
                button_release_at = now;
            }
        }

        //  keep heartbeat alive until on_report_timer detaches the ticker
        wake_uptime_ = now;
    }

    void on_report_timer() {
        uptime_ticker_.detach();

        event_bus_.publish({ AppEvent::Id::WAKE_INFO, { .wake = {
            .prev_uptime        = prev_uptime,
            .button_release_at  = button_release_at,
        }}});

        LOG_INFO("previous boot uptime %u ms", prev_uptime);
        LOG_INFO("button release seen at %u ms", button_release_at);
    }
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL
