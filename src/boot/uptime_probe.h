//  boot uptime probe
//
//  tracks rtc uptime for a short window after button release,
//  returns previous boot uptime to caller
//
//  see LICENSE for terms

#pragma once

#include <Ticker.h>

#include "settings.h"
#include "rtc.h"

#include "lite/sys/clock.h"
#include "lite/io/log.h"
#include "lite/core/method_bind.h"

#define LOG_TAG     uptime_probe
#define LOG_LEVEL   UPTIME_PROBE_LOG

//==============================================================================
class UptimeProbe final {

using u16 = lite::u16;

//------------------------------------------------------------------------------
public:
    u16 track() {
        const u16 prev = static_cast<u16>(rtc::wake_uptime().get());

        ticks_left_ = UPTIME_PROBE_DURATION_MS / 10;
        ticker_.attach_ms(10, CALLBACK_THIS(on_tick));

        LOG_TRACE("started, prev: %u ms", prev);
        return prev;
    }

//------------------------------------------------------------------------------
private:
    void on_tick() {
        rtc::wake_uptime() = lite::now_ms();

        if (--ticks_left_ == 0) {
            ticker_.detach();
        }
    }

    Ticker  ticker_;
    u16     ticks_left_;
};

#undef LOG_TAG
#undef LOG_LEVEL
