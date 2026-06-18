//  boot mode selection
//
//  see LICENSE for terms

#pragma once

#include "lite/core/types.h"

namespace boot {

using u8 = lite::u8;

enum class Mode : u8 {
    app         = 0,
    hotspot,
    count_
};

//-----------------------------------------------------------------------------
constexpr bool is_valid(Mode mode) noexcept {
    return mode < Mode::count_;
}
    

//-----------------------------------------------------------------------------
constexpr Mode normalize(Mode mode) noexcept {
    return is_valid(mode) ? mode : Mode::app;
}

} // namespace boot