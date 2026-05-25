//  manage ir receiver
//
//  see LICENSE file for terms

#pragma once

#include "settings.h"

#include "lite/core/types.h"

#include <Arduino.h>
#include <esp8266_peri.h>

using lite::u8;
using lite::u32;

//==============================================================================
class IrRx {
//------------------------------------------------------------------------------    
public:
    IrRx() noexcept {
        Decoder::init();
    }

    struct Result {
        bool	is_valid;
        u8		addr;
        u8		cmd;
        bool	is_repeat;
    };

    Result read() {
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
            frame_addr = 0;
            frame_cmd = 0;
            frame_repeat = false;
            last_addr = 0;
            last_cmd = 0;
            has_last_data = false;
            last_edge_us = micros();
            interrupts();

            attachInterrupt(
                digitalPinToInterrupt(IR_RX_GPIO),
                on_edge,
                CHANGE
            );
        }

        static Result read() {
            noInterrupts();
            const bool is_ready = frame_ready;
            const u8 addr = frame_addr;
            const u8 cmd = frame_cmd;
            const bool is_repeat = frame_repeat;
            frame_ready = false;
            interrupts();

            if (!is_ready) {
                return { .is_valid = false };
            }

            return {
                .is_valid	= true,
                .addr		= addr,
                .cmd		= cmd,
                .is_repeat	= is_repeat
            };
        }

    private:
        static_assert(IR_RX_GPIO < 16, "IrRx supports only gpio 0..15");

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
        inline static volatile u8 frame_addr = 0;
        inline static volatile u8 frame_cmd = 0;
        inline static volatile bool frame_repeat = false;

        inline static u32 last_edge_us = 0;
        inline static u32 data = 0;
        inline static u8 bit_cnt = 0;
        inline static State state = State::idle;
        inline static u8 last_addr = 0;
        inline static u8 last_cmd = 0;
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
            const u8 addr = static_cast<u8>(data);
            const u8 addr_inv = static_cast<u8>(data >> 8);
            const u8 cmd = static_cast<u8>(data >> 16);
            const u8 cmd_inv = static_cast<u8>(data >> 24);

            if (static_cast<u8>(~addr) != addr_inv) {
                return;
            }

            if (static_cast<u8>(~cmd) != cmd_inv) {
                return;
            }

            last_addr = addr;
            last_cmd = cmd;
            has_last_data = true;
            publish(addr, cmd, false);
        }

        static void IRAM_ATTR publish_repeat() {
            if (!has_last_data) {
                return;
            }

            publish(last_addr, last_cmd, true);
        }

        static void IRAM_ATTR publish(u8 addr, u8 cmd, bool is_repeat) {
            frame_addr = addr;
            frame_cmd = cmd;
            frame_repeat = is_repeat;
            frame_ready = true;
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
