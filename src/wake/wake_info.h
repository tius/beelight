//  manage wake button
//
//  remarks:
//      - the button is active low and connected to GPIO16
//      - the reset logic triggers a short reset pulse when GPIO16 goes low
//      - gpio16 does not support interrupts
//      - ticker is used to detect button release as timer is not yet ready
//      - ticker keeps rtc uptime fresh until after the release window
//      - recorded times are early-boot observations, not physical edge times
//
//  see LICENSE file for terms

#pragma once

#include <Ticker.h>

#include "settings.h"
#include "lite/sys/gpio.h"
#include "lite/sys/clock.h"
#include "lite/sys/rtc_mem.h"
#include "lite/core/timer.h"
#include "lite/core/method_bind.h"

using lite::u16;
using lite::u32;

//=============================================================================
struct WakeInfoSample {
    u16 prev_uptime;
    u16 button_release_at;
};

//=============================================================================
template <typename WakeHook>
class WakeInfo final {

using WakeUptime = lite::sys::RtcVar<u32>;

//-----------------------------------------------------------------------------
public:
    WakeInfo(WakeUptime wake_uptime, WakeHook& wake_hook) noexcept
        : wake_uptime_(wake_uptime), wake_hook_(wake_hook)
    {
        //  sys context is intentional, loop may start too late
        uptime_ticker_.attach_ms(10, CALLBACK_THIS(on_uptime_tick));
        on_uptime_tick();

        stop_timer_.start(lite::duration_ms{WAKE_INFO_KEEP_ALIVE_MS});
    }


private:
    using ButtonGpio = lite::gpio::Input<BUTTON_GPIO>;

    static constexpr u16 never = 0xFFFFu;

    WakeUptime      wake_uptime_;
    WakeHook&       wake_hook_;
    ButtonGpio      button_gpio_;
    
    Ticker          uptime_ticker_;
    lite::Timer     stop_timer_ { MSG_THIS(on_stop_timer) };

    //  early wake window uses u16 millisecond values by design
    u16             prev_uptime = never;
    volatile u16    button_release_at = never;

    void on_uptime_tick() {
        //  previous boot uptime is read once, after ticker startup
        if (prev_uptime == never) {
            prev_uptime = (u16) wake_uptime_.get();
        }

        const u32 now = lite::now_ms();
        if (button_gpio_.is_hi() && button_release_at == never) {
            button_release_at = static_cast<u16>(now);

            wake_hook_.on_wake_release({
                .prev_uptime = prev_uptime,
                .button_release_at = button_release_at,
            });
        }

        wake_uptime_ = now;
    }

    void on_stop_timer() {
        if (button_release_at == never) {
            stop_timer_.start(lite::duration_ms{WAKE_INFO_KEEP_ALIVE_MS});
            return;
        }

        const u16 elapsed = static_cast<u16>(lite::now_ms() - button_release_at);
        if (elapsed < WAKE_INFO_KEEP_ALIVE_MS) {
            stop_timer_.start(lite::duration_ms{
                static_cast<u16>(WAKE_INFO_KEEP_ALIVE_MS - elapsed)
            });
            return;
        }

        uptime_ticker_.detach();
    }
};

//=============================================================================
