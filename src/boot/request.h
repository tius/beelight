//  one-shot boot mode request
//
//  see LICENSE for terms

#pragma once

#include "boot/mode.h"
#include "rtc.h"

#include "lite/sys/restart.h"

namespace boot {

inline void reboot(Mode mode) {
    auto request = rtc::boot_request();

    request = normalize(mode);
    lite::sys::restart();
}

//-----------------------------------------------------------------------------
[[nodiscard]] inline Mode startup_mode() {
    auto request = rtc::boot_request();

    const Mode mode = request.get();
    if (mode == Mode::app) {
        return Mode::app;
    }

    request = Mode::app;
    return normalize(mode);
}

} // namespace boot
