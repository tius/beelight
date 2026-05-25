//  ir transmitter using timer1 carrier backend
//
//  caveats:
//  - timer1 has one callback slot
//  - active tx overrides other setTimer1Callback users
//
//  operation:
//  - timer1 callback emits the 38 khz carrier by direct GPOS/GPOC writes
//  - direct gpio writes keep the carrier path fast and isr-safe
//  - marks are represented by carrier edge counts
//  - spaces are scheduled as cpu-cycle delays
//  - timer1 may fire early, so next_edge_at gates every scheduled edge
//  - next symbol is preloaded before the current space expires
//  - copy the current space delay before loading the next symbol
//
//  why not startWaveform:
//  - esp8266 waveform helpers also own timer1
//  - waveform helpers are not safe to call from the timer1 callback
//  - starting or stopping waveforms from the callback can trip the soft wdt
//
//  why tick:
//  - setTimer1Callback(nullptr) is not allowed from the timer1 callback
//  - callback marks tx done, turns tx off, and leaves timer1 idle
//  - tick() sees done and runs cleanup from normal loop context
//  - cleanup detaches timer1 and returns the engine to idle
//
//  constraints:
//  - direct GPOS/GPOC writes require gpio 0..15
//  - use tx_on_ and tx_off_ so active-low hardware keeps semantic on/off
//
//  see LICENSE file for terms

#pragma once
#include "settings.h"
#include "ir_tx_base.h"

#include "lite/core/int_arith.h"
#include "lite/core/types.h"

#include <Arduino.h>
#include <core_esp8266_waveform.h>
#include <esp8266_peri.h>

using lite::u8;
using lite::u16;
using lite::u32;
using lite::s32;

//==============================================================================
class IrTxTimer1 : public IrTxBase<IrTxTimer1> {
//------------------------------------------------------------------------------
public:
    IrTxTimer1() {
        tx_engine_.init();
    }

    bool is_ready() const noexcept {
        return tx_engine_.is_ready();
    }

    void tick() {
        tx_engine_.tick();
    }

    bool send_data_frame(u32 raw_frame, u16 mark_us, u16 space_us) {
        if (!is_ready()) {
            return false;
        }
        tx_engine_.start(raw_frame, mark_us, space_us);
        return true;
    }

    bool send_repeat_frame(u16 mark_us, u16 space_us) {
        if (!is_ready()) {
            return false;
        }
        tx_engine_.start_repeat(mark_us, space_us);
        return true;
    }

//------------------------------------------------------------------------------
private:
    struct TxEngine {
        static_assert(IR_TX_GPIO < 16,  "IrTxTimer1 supports only gpio 0..15");

        enum class State : u8 {
            idle,
            symbol,
            done,
        };

        static constexpr u32 TX_MASK = 1u << IR_TX_GPIO;

        //  38 khz carrier timing, about 31 % duty
        static constexpr u16 CARRIER_ON_US      = 8;
        static constexpr u16 CARRIER_OFF_US     = 18;

        //  nec timing
        static constexpr u16 BIT_MARK_US        = 560;
        static constexpr u16 ZERO_SPACE_US      = 560;
        static constexpr u16 ONE_SPACE_US       = 1690;
        static constexpr u8  DATA_BIT_CNT       = 32;

        //  precomputed values for timer1 scheduling
        static constexpr u32 CARRIER_ON_DELAY   = microsecondsToClockCycles(CARRIER_ON_US);
        static constexpr u32 CARRIER_OFF_DELAY  = microsecondsToClockCycles(CARRIER_OFF_US);
        static constexpr u32 CARRIER_PERIOD     = CARRIER_ON_DELAY + CARRIER_OFF_DELAY;

        static constexpr u32 BIT_MARK_DELAY     = microsecondsToClockCycles(BIT_MARK_US);
        static constexpr u32 ZERO_SPACE_DELAY   = microsecondsToClockCycles(ZERO_SPACE_US);
        static constexpr u32 ONE_SPACE_DELAY    = microsecondsToClockCycles(ONE_SPACE_US);

        //  timer 1 limits
        static constexpr u32 DELAY_MIN          = microsecondsToClockCycles(4);
        static constexpr u32 DELAY_MAX          = microsecondsToClockCycles(10000);

        //  compute number of carrier edges for a given delay
        static constexpr u32 delay_to_carrier_cnt_(u32 delay) {
            const u32 carrier_periods = lite::div_round_up(delay, CARRIER_PERIOD);
            return carrier_periods * 2;
        }

        //----------------------------------------------------------------------
        inline static u32   data;
        inline static u8    bit_cnt;

        inline static u32   carrier_cnt;
        inline static u32   space_delay;
        inline static u32   next_edge_at;

        inline static volatile State state = State::idle;

        //----------------------------------------------------------------------
        void init() {
            tx_off_();
            pinMode(IR_TX_GPIO, OUTPUT);
        }

        bool is_ready() const noexcept {
            return state == State::idle;
        }

        //----------------------------------------------------------------------
        static void start(u32 raw_frame, u16 mark_us_, u16 space_us_) {
            start_(raw_frame, mark_us_, space_us_, DATA_BIT_CNT);
        }

        static void start_repeat(u16 mark_us_, u16 space_us_) {
            start_(0, mark_us_, space_us_, 0);
        }

        //----------------------------------------------------------------------
        void tick() {
            if (state != State::done) {
                return;
            }

            setTimer1Callback(nullptr);
            tx_off_();
            carrier_cnt = 0;
            next_edge_at = 0;
            state = State::idle;
        }

        //----------------------------------------------------------------------
        static void start_(u32 raw_frame, u16 mark_us_, u16 space_us_, u8 bit_cnt_) {
            tx_off_();

            data            = raw_frame;
            carrier_cnt     = 0;
            next_edge_at    = 0;
            bit_cnt         = bit_cnt_ + 1;         // data bits + stop bit

            load_symbol_(
                delay_to_carrier_cnt_(microsecondsToClockCycles(mark_us_)),
                microsecondsToClockCycles(space_us_)
            );

            setTimer1Callback(on_timer1_);
        }

        //----------------------------------------------------------------------
        static u32 IRAM_ATTR on_timer1_() {
            const u32 now = now_cycles_();
            if (!is_due_(now)) {
                return timer_delay_(next_edge_at - now);
            }

            if (carrier_cnt > 0) {
                return send_carrier_(now);
            }

            const State current_state = state;
            switch (current_state) {
                case State::idle:
                    return idle_();
                case State::symbol:
                    return finish_symbol_(now);
                default:
                    return done_();
            }
        }

        static bool IRAM_ATTR is_due_(u32 now) {
            return next_edge_at == 0
                || static_cast<s32>(now - next_edge_at) >= 0;
        }

        static u32 IRAM_ATTR send_carrier_(u32 now) {
            const bool edge_is_on = (carrier_cnt & 1u) == 0;

            --carrier_cnt;

            if (edge_is_on) {
                tx_on_();
                return schedule_(now, CARRIER_ON_DELAY);
            }
            else {
                tx_off_();
                return schedule_(now, CARRIER_OFF_DELAY);
            }
        }

        static u32 IRAM_ATTR finish_symbol_(u32 now) {
            tx_off_();

            const u32 current_space_delay = space_delay;
            if (bit_cnt == 0) {
                state = State::done;
            }
            else {
                load_next_symbol_();
            }
            return schedule_(now, current_space_delay);
        }

        static void IRAM_ATTR load_next_symbol_() {
            const bool is_one = (data & 1u) != 0;
            data >>= 1; // after data bits, zero shifts in for the stop bit
            --bit_cnt;

            load_symbol_(
                delay_to_carrier_cnt_(BIT_MARK_DELAY),
                is_one ? ONE_SPACE_DELAY : ZERO_SPACE_DELAY
            );
        }

        static void IRAM_ATTR load_symbol_(u32 mark_carrier_cnt_, u32 space_delay_) {
            carrier_cnt = mark_carrier_cnt_;
            space_delay = space_delay_;
            state = State::symbol;
        }

        static u32 IRAM_ATTR idle_() {
            tx_off_();
            return wait_idle_();
        }

        static u32 IRAM_ATTR done_() {
            tx_off_();
            state = State::done;
            return wait_idle_();
        }

        static u32 IRAM_ATTR schedule_(u32 now, u32 delay) {
            const u32 wait_delay = timer_delay_(delay);
            next_edge_at = now + wait_delay;
            return wait_delay;
        }

        //----------------------------------------------------------------------
        static u32 IRAM_ATTR now_cycles_() {
            u32 cycles;
            __asm__ __volatile__("rsr %0,ccount" : "=a"(cycles));
            return cycles;
        }

        //----------------------------------------------------------------------
        static u32 IRAM_ATTR timer_delay_(u32 delay) {
            return max(delay, DELAY_MIN);
        }

        static u32 IRAM_ATTR wait_idle_() {
            next_edge_at = 0;
            return DELAY_MAX;
        }

        //----------------------------------------------------------------------
        static void IRAM_ATTR tx_on_() {
            if constexpr (IR_TX_ACTIVE_LOW) {
                GPOC = TX_MASK;
            } else {
                GPOS = TX_MASK;
            }
        }

        static void IRAM_ATTR tx_off_() {
            if constexpr (IR_TX_ACTIVE_LOW) {
                GPOS = TX_MASK;
            } else {
                GPOC = TX_MASK;
            }
        }
    };


    //--------------------------------------------------------------------------
    TxEngine    tx_engine_;

};
