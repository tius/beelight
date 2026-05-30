//  charger driver contract types
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/status.h"

#include <cstddef>
#include <cstdio>

//==============================================================================
struct ChargerMode {
    using u8 = lite::u8;

    enum : u8 {
        ON_BATTERY = 0,
        EXT_POWER,
        CHARGING,
    };

    u8 code = ON_BATTERY;

    constexpr operator u8() const noexcept {
        return code;
    }

    const char* str() const noexcept {
        switch (code) {
            case EXT_POWER:     return "ext power";
            case CHARGING:      return "charging";
            default:            return "on battery";
        }
    }
};

static_assert(sizeof(ChargerMode) == 1, "unexpected size of ChargerMode");

//==============================================================================
struct ChargerStatus : public lite::Status {
    using u8 = lite::u8;

    enum : u8 {
        OK = 0,
        CHARGE_COMPLETE,
        POWER_LIMIT,
        THERMAL_LIMIT,
        INPUT_FAULT,
        BATTERY_FAULT,
        SAFETY_TIMER,
        WATCHDOG,
        TOO_HOT,
    };

    constexpr explicit operator bool() const noexcept {
        return is_ok();
    }

    constexpr bool is_ok() const noexcept {
        return code == OK || code == CHARGE_COMPLETE;
    }

    constexpr bool is_warning() const noexcept {
        return code == POWER_LIMIT || code == THERMAL_LIMIT;
    }

    constexpr bool is_error() const noexcept {
        return !is_ok() && !is_warning();
    }

    const char* str() const noexcept {
        switch (code) {
            case CHARGE_COMPLETE:   return "charge complete";
            case POWER_LIMIT:       return "power limit";
            case THERMAL_LIMIT:     return "thermal limit";
            case INPUT_FAULT:       return "input fault";
            case BATTERY_FAULT:     return "battery fault";
            case SAFETY_TIMER:      return "safety timer";
            case WATCHDOG:          return "watchdog";
            case TOO_HOT:           return "too hot";
            default:                return lite::Status::str();
        }
    }
};

static_assert(sizeof(ChargerStatus) == 1, "unexpected size of ChargerStatus");

//==============================================================================
struct ChargerState {
    ChargerMode mode;
    ChargerStatus status;

    template <std::size_t N>
    const char* fmt(char (&buffer)[N]) const {
        std::snprintf(
            buffer,
            N,
            "charger_status mode=%s status=%s",
            mode.str(),
            status.str()
        );
        return buffer;
    }
};

static_assert(sizeof(ChargerState) == 2, "unexpected size of ChargerState");