//  rgb fsm state definitions
//
//  see LICENSE file for terms

#pragma once

#include "lite/types.h"

using lite::u8;
using lite::u16;

//=============================================================================
struct RgbState {
    enum : u8 {
        OFF,
        CHARGE,
        _COUNT
    };

    u8 id = 0;

    constexpr RgbState() = default;
    constexpr RgbState(u8 v) : id(v) {}
    constexpr operator u8() const { return id; }

    const char* str() const {
        switch (id) {
            case OFF:               return "OFF";
            case CHARGE:            return "CHARGE";
            default:                return "?";
        }
    }
};
