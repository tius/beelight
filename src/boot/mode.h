//  boot mode selection
//
//  see LICENSE for terms

#pragma once

#include "lite/core/types.h"

namespace boot {

enum class Mode : lite::u8 {
    app = 0,
    hotspot,
};

//-----------------------------------------------------------------------------
[[nodiscard]] constexpr bool is_valid(Mode mode) noexcept {
    switch (mode) {
        case Mode::app:
        case Mode::hotspot:
            return true;
    }

    return false;
}

//-----------------------------------------------------------------------------
[[nodiscard]] constexpr Mode normalize(Mode mode) noexcept {
    return is_valid(mode) ? mode : Mode::app;
}

} // namespace boot