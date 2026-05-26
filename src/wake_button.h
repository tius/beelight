//  manage wake button
//
//  remarks:
//      - the button is active low and connected to GPIO16
//      - the reset logic triggers a short reset pulse when GPIO16 goes low
//      - gpio16 does not support interrupts
//      - polling is used to detect button release
//
//  see LICENSE file for terms

#pragma once

#include "app_event.h"
#include "lite/io/log.h"
#include "lite/sys/gpio.h"
#include "lite/sys/clock.h"
#include "lite/core/changed.h"
#include "lite/core/timer.h"
#include "lite/core/event_bus.h"

#define LOG_TAG		button
#define LOG_LEVEL	debug

using lite::changed;

//=============================================================================
class WakeButton final {

using EventBus = lite::EventBus<AppEvent>;

//-----------------------------------------------------------------------------
public:
    WakeButton(EventBus& event_bus) noexcept
        : event_bus_(event_bus) 
    {
        timer_.start_periodic(10ms);
    }


private:
    using ButtonGpio = lite::gpio::Input<WAKE_BUTTON_GPIO>;

    EventBus&       event_bus_;
    ButtonGpio      button_gpio_;
    lite::Timer     timer_      { MSG_THIS(on_timer) };
    bool            is_high_    = false;

    void on_timer() {
        if ( button_gpio_.is_hi() ) {
            if ( changed( is_high_, true ) ) {
                auto now = lite::now_ms();
                LOG_DEBUG("button released at %u ms", now);
            }
        }
    }
};

//=============================================================================
#undef LOG_TAG
#undef LOG_LEVEL
