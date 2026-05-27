//  boot mode selection
//
//  see LICENSE for terms

#pragma once

#include "boot/mode.h"

namespace boot {

constexpr Mode select_mode() noexcept {
    return Mode::app;
}

} // namespace boot