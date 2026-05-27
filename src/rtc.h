//  application rtc memory access
//
//  see LICENSE file for terms

#pragma once

#include "boot/mode.h"
#include "wake/morse_state.h"

#include "lite/core/types.h"
#include "lite/sys/rtc_mem.h"
#include "lite/sys/rtc_slot.h"

namespace rtc {

//=============================================================================
enum class RtcAddr : lite::u8 {
    RTC_ALLOC(boot_request, boot::Mode),
    RTC_ALLOC(wake_uptime, lite::u32),
    RTC_ALLOC(wake_morse, WakeMorseState),
    RTC_ALLOC(boot_count, lite::u32),
    count,
};

static_assert(RTC_COUNT <= 127u, "rtc layout exceeds esp8266 user memory");

//-----------------------------------------------------------------------------
inline lite::sys::RtcMem& mem() {
    static lite::sys::RtcMem instance {RTC_COUNT};
    return instance;
}

//-----------------------------------------------------------------------------
[[nodiscard]] inline lite::sys::RtcVar<boot::Mode> boot_request() {
    return {mem(), RTC_SLOT(boot_request)};
}

//-----------------------------------------------------------------------------
[[nodiscard]] inline lite::sys::RtcVar<lite::u32> wake_uptime() {
    return {mem(), RTC_SLOT(wake_uptime)};
}

//-----------------------------------------------------------------------------
[[nodiscard]] inline lite::sys::RtcVar<WakeMorseState> wake_morse() {
    return {mem(), RTC_SLOT(wake_morse)};
}

//-----------------------------------------------------------------------------
[[nodiscard]] inline lite::sys::RtcVar<lite::u32> boot_count() {
    return {mem(), RTC_SLOT(boot_count)};
}

} // namespace rtc
