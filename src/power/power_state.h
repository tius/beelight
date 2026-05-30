//  app-level power state

#pragma once

#include "lite/core/types.h"

//==============================================================================
struct PowerState {
    using u8 = lite::u8;

    enum : u8 {
        ON_BATTERY = 0,
        CHARGING,
        EXT_POWER,
        POWER_LIMITED,
        CHARGE_FAULT,
        TEMP_FAULT,
    };

    u8 code = ON_BATTERY;

    constexpr operator u8() const noexcept {
        return code;
    }

    constexpr bool is_ok() const noexcept {
        return code == ON_BATTERY
            || code == CHARGING
            || code == EXT_POWER;
    }

    constexpr bool is_external() const noexcept {
        return code != ON_BATTERY;
    }

    const char* str() const noexcept {
        switch (code) {
            case CHARGING:       return "charging";
            case EXT_POWER:      return "ext power";
            case POWER_LIMITED:  return "power limited";
            case CHARGE_FAULT:   return "charge fault";
            case TEMP_FAULT:     return "temp fault";
            default:             return "on battery";
        }
    }
};

static_assert(sizeof(PowerState) == 1, "unexpected size of PowerState");