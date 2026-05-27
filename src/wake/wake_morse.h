//  decode wake-button morse input across resets
//
//  remarks:
//      - on_wake_release runs from the wake-info ticker path
//      - ticker-context work is limited to rtc state updates
//      - command events are published later from loop context
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "event/event.h"
#include "wake/morse_state.h"
#include "wake_info.h"

#include "lite/effects/morse.h"
#include "lite/sys/clock.h"
#include "lite/sys/rtc_mem.h"
#include "lite/core/types.h"

static_assert(
    WAKE_MORSE_DOT_MAX_MS < WAKE_MORSE_CHAR_GAP_MS,
    "wake morse dot threshold must be shorter than character gap"
);
static_assert(
    WAKE_MORSE_CHAR_GAP_MS < WAKE_MORSE_DONE_GAP_MS,
    "wake morse character gap must be shorter than done gap"
);
static_assert(
    WAKE_INFO_KEEP_ALIVE_MS >= WAKE_MORSE_DONE_GAP_MS,
    "wake info uptime keepalive must cover wake morse done gap"
);

//=============================================================================
class WakeMorse final {
using EventBus = event::Bus;
using StateVar = lite::sys::RtcVar<WakeMorseState>;

//-----------------------------------------------------------------------------
public:
    WakeMorse(StateVar state_var, EventBus& event_bus) noexcept
        : state_var_(state_var), event_bus_(event_bus) {}

    void on_wake_release(const WakeInfoSample& sample) {
        WakeMorseState state = load_state();

        if (state.symbol_len != 0u) {
            const lite::u16 gap = elapsed(
                sample.prev_uptime,
                state.last_release_at
            );

            if (gap >= WAKE_MORSE_DONE_GAP_MS) {
                state = empty_state();
            }
            else if (gap >= WAKE_MORSE_CHAR_GAP_MS) {
                if (!finish_char(state)) {
                    clear_state();
                    return;
                }

                if (state.char_count >= max_chars) {
                    clear_state();
                    return;
                }
            }
        }

        if (!append_symbol(state, sample.button_release_at)) {
            clear_state();
            return;
        }

        state.last_release_at = sample.button_release_at;
        save_state(state);
    }

    void tick() {
        WakeMorseState state = load_state();
        if (!has_input(state)) {
            return;
        }

        if (state.symbol_len == 0u) {
            clear_state();
            return;
        }

        const lite::u16 idle = elapsed(
            static_cast<lite::u16>(lite::now_ms()),
            state.last_release_at
        );
        if (idle < WAKE_MORSE_DONE_GAP_MS) {
            return;
        }

        if (!finish_char(state)) {
            clear_state();
            return;
        }

        publish_command(state);
    }

//-----------------------------------------------------------------------------
private:
    static constexpr lite::u8 max_chars = 3u;
    static constexpr lite::u8 max_symbols = 5u;

    StateVar state_var_;
    EventBus& event_bus_;

    static WakeMorseState empty_state() noexcept {
        return {};
    }

    static bool is_valid(const WakeMorseState& state) noexcept {
        return state.char_count <= max_chars
            && state.symbol_len <= max_symbols;
    }

    static bool has_input(const WakeMorseState& state) noexcept {
        return state.char_count != 0u || state.symbol_len != 0u;
    }

    static lite::u16 elapsed(lite::u16 now, lite::u16 since) noexcept {
        return static_cast<lite::u16>(now - since);
    }

    WakeMorseState load_state() {
        WakeMorseState state = state_var_.get();
        if (!is_valid(state)) {
            return empty_state();
        }

        return state;
    }

    void save_state(const WakeMorseState& state) {
        state_var_ = state;
    }

    void clear_state() {
        save_state(empty_state());
    }

    static bool append_symbol(WakeMorseState& state, lite::u16 duration_ms) {
        if (state.symbol_len >= max_symbols) {
            return false;
        }

        const lite::u8 bit = duration_ms <= WAKE_MORSE_DOT_MAX_MS ? 1u : 0u;
        state.symbol_bits = static_cast<lite::u8>((state.symbol_bits << 1) | bit);
        ++state.symbol_len;

        return true;
    }

    static bool finish_char(WakeMorseState& state) {
        if (state.symbol_len == 0u) {
            return true;
        }

        if (state.char_count >= max_chars) {
            return false;
        }

        const char c = lite::morse::decode(state.symbol_bits, state.symbol_len);
        if (c == '\0') {
            return false;
        }

        state.chars[state.char_count] = c;
        ++state.char_count;
        state.symbol_bits = 0u;
        state.symbol_len = 0u;

        return true;
    }

    void publish_command(const WakeMorseState& state) {
        event::Payload payload {};
        payload.morse_cmd.len = state.char_count;

        for (lite::u8 idx = 0; idx < state.char_count; ++idx) {
            payload.morse_cmd.text[idx] = state.chars[idx];
        }

        clear_state();
        event_bus_.publish({ event::Id::MORSE_CMD, payload });
    }
};