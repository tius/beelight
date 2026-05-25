//  edge-driven ir receiver implementation
//
//  operation:
//  - a change interrupt records every edge on the demodulated ir input
//  - the edge-to-edge duration is classified by the current pin level
//  - the isr runs the timing state machine and fills a single ready slot
//  - foreground read() atomically copies and clears the ready slot
//  - data frames accept any non-reserved 32-bit value with nec-like timing
//  - repeat frames use nec repeat timing and publish IrCode::repeat()
//
//  static state:
//  - attachInterrupt requires a plain isr entry without object context
//  - static methods and data keep the isr path short and direct
//  - this also means only one IrRxEdge instance can be active
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"
#include "ir_code.h"

#include "lite/core/types.h"

#include <Arduino.h>
#include <esp8266_peri.h>

using lite::u8;
using lite::u32;

//==============================================================================
class IrRxEdge {
//------------------------------------------------------------------------------    
public:
    IrRxEdge() noexcept {
        Decoder::init();
    }

    IrCode read() {
        return Decoder::read();
    }

//------------------------------------------------------------------------------
private:
    class Decoder {
    public:
        static void init() {
            pinMode(IR_RX_GPIO, INPUT_PULLUP);

            noInterrupts();
            reset();
            frame_ready = false;
            frame_raw = IrCode::k_invalid_raw;
            has_last_data = false;
            last_edge_us = micros();
            interrupts();

            attachInterrupt(
                digitalPinToInterrupt(IR_RX_GPIO),
                on_edge,
                CHANGE
            );
        }

        static IrCode read() {
            noInterrupts();
            const bool is_ready = frame_ready;
            const u32 raw = frame_raw;
            frame_ready = false;
            interrupts();

            if (!is_ready) {
                return IrCode::invalid();
            }

            return IrCode::from_raw(raw);
        }

    private:
        static_assert(IR_RX_GPIO < 16, "IrRxEdge supports only gpio 0..15");

        enum class State : u8 {
            idle,
            header_space,
            repeat_mark,
            bit_mark,
            bit_space,
        };

        static constexpr u32 HEADER_MARK_US = 9000;
        static constexpr u32 HEADER_SPACE_US = 4500;
        static constexpr u32 REPEAT_SPACE_US = 2250;
        static constexpr u32 BIT_MARK_US = 560;
        static constexpr u32 ZERO_SPACE_US = 560;
        static constexpr u32 ONE_SPACE_US = 1690;
        static constexpr u8 DATA_BIT_CNT = 32;

        inline static volatile bool frame_ready = false;
        inline static volatile u32 frame_raw = IrCode::k_invalid_raw;

        inline static u32 last_edge_us = 0;
        inline static u32 data = 0;
        inline static u8 bit_cnt = 0;
        inline static State state = State::idle;
        inline static bool has_last_data = false;

        static void IRAM_ATTR on_edge() {
            const u32 now_us = micros();
            const u32 duration_us = now_us - last_edge_us;
            last_edge_us = now_us;

            if (level_is_high()) {
                consume_mark(duration_us);
                return;
            }

            consume_space(duration_us);
        }

        static bool IRAM_ATTR level_is_high() {
            return GPIP(IR_RX_GPIO);
        }

        static void IRAM_ATTR consume_mark(u32 duration_us) {
            switch (state) {
                case State::idle:
                    if (matches(duration_us, HEADER_MARK_US)) {
                        state = State::header_space;
                    }
                    return;

                case State::repeat_mark:
                    if (matches(duration_us, BIT_MARK_US)) {
                        publish_repeat();
                    }
                    reset();
                    return;

                case State::bit_mark:
                    if (matches(duration_us, BIT_MARK_US)) {
                        state = State::bit_space;
                        return;
                    }
                    reset();
                    return;

                default:
                    reset();
                    return;
            }
        }

        static void IRAM_ATTR consume_space(u32 duration_us) {
            switch (state) {
                case State::idle:
                    return;

                case State::header_space:
                    consume_header_space(duration_us);
                    return;

                case State::bit_space:
                    consume_bit_space(duration_us);
                    return;

                default:
                    reset();
                    return;
            }
        }

        static void IRAM_ATTR consume_header_space(u32 duration_us) {
            if (matches(duration_us, HEADER_SPACE_US)) {
                data = 0;
                bit_cnt = 0;
                state = State::bit_mark;
                return;
            }

            if (matches(duration_us, REPEAT_SPACE_US)) {
                state = State::repeat_mark;
                return;
            }

            reset();
        }

        static void IRAM_ATTR consume_bit_space(u32 duration_us) {
            if (matches(duration_us, ZERO_SPACE_US)) {
                append_bit(false);
                return;
            }

            if (matches(duration_us, ONE_SPACE_US)) {
                append_bit(true);
                return;
            }

            reset();
        }

        static void IRAM_ATTR append_bit(bool is_one) {
            if (is_one) {
                data |= static_cast<u32>(1) << bit_cnt;
            }

            ++bit_cnt;
            if (bit_cnt == DATA_BIT_CNT) {
                publish_data();
                reset();
                return;
            }

            state = State::bit_mark;
        }

        static void IRAM_ATTR publish_data() {
            if (!raw_is_data(data)) {
                return;
            }

            has_last_data = true;
            publish_raw(data);
        }

        static void IRAM_ATTR publish_repeat() {
            if (!has_last_data) {
                return;
            }

            publish_raw(IrCode::k_repeat_raw);
        }

        static void IRAM_ATTR publish_raw(u32 raw) {
            frame_raw = raw;
            frame_ready = true;
        }

        static bool IRAM_ATTR raw_is_data(u32 raw) {
            return raw != IrCode::k_invalid_raw
                && raw != IrCode::k_repeat_raw;
        }

        static void IRAM_ATTR reset() {
            data = 0;
            bit_cnt = 0;
            state = State::idle;
        }

        static bool IRAM_ATTR matches(u32 duration_us, u32 target_us) {
            const u32 tolerance_us = target_us / 3;
            return duration_us >= target_us - tolerance_us
                && duration_us <= target_us + tolerance_us;
        }
    };
};
