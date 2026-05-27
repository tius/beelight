//  wake morse rtc state
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/types.h"

//=============================================================================
struct WakeMorseState {
    lite::u8 char_count;
    lite::u8 symbol_len;
    lite::u8 symbol_bits;
    char chars[3];
    lite::u16 last_release_at;
};
