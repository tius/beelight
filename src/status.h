//  general status code for app and drivers
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"

using lite::u8;

//=============================================================================
struct Status {
//-----------------------------------------------------------------------------    
    enum : u8 {
        OK = 0,
    };

    u8 code = OK;

    constexpr operator u8() const noexcept {
        return code;
    }
    constexpr explicit operator bool() const noexcept {
        return code == OK;
    }
    constexpr bool is_ok() const noexcept {
        return code == OK;
    }
    constexpr bool is_error() const noexcept {
        return code != OK;
    }
    const char* str() const noexcept {
        switch (code) {
            case OK:                return "ok";
            default:                return "error";
        }
    }
};
