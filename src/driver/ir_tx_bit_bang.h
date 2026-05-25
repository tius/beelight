//  ir transmitter bit-bang backend
//
//  see LICENSE file for terms

#pragma once
#include "settings.h"
#include "ir_tx_base.h"

#include "lite/core/types.h"
#include "lite/sys/gpio.h"

#include <Arduino.h>

using lite::s32;
using lite::u8;
using lite::u16;
using lite::u32;

//==============================================================================
class IrTxBitBang : public IrTxBase<IrTxBitBang> {
//------------------------------------------------------------------------------
public:
    void send_data_frame(u32 raw_frame, u16 mark_us, u16 space_us) {
        mark(mark_us);
        space(space_us);

        for (u8 bit_idx = 0; bit_idx < 32; ++bit_idx) {
            mark(BIT_MARK_US);

            const bool is_one = (raw_frame & 1u) != 0;
            space(is_one ? ONE_SPACE_US : ZERO_SPACE_US);
            raw_frame >>= 1;
        }

        mark(BIT_MARK_US);
        carrier_off();
    }

    void send_repeat_frame(u16 mark_us, u16 space_us) {
        mark(mark_us);
        space(space_us);
        mark(BIT_MARK_US);
        carrier_off();
    }

//------------------------------------------------------------------------------
private:
    using TxPin = lite::gpio::Output<
        IR_TX_GPIO,
        lite::gpio::OutputCfg{
            .initial_on = false,
            .active_lo  = IR_TX_ACTIVE_LOW,
        }
    >;

    static constexpr u16 BIT_MARK_US      = 560;
    static constexpr u16 ZERO_SPACE_US    = 560;
    static constexpr u16 ONE_SPACE_US     = 1690;
    static constexpr u16 CARRIER_PERIOD_US = 26;
    static constexpr u16 CARRIER_ON_US     = 8;

    TxPin tx_pin_{};

    void carrier_off() {
        tx_off();
    }

    void mark(u16 mark_us) {
        u32 now_us = micros();
        u32 period_end_us = now_us;
        const u32 mark_end_us = now_us + mark_us;

        while (true) {
            noInterrupts();
            tx_on();
            delayMicroseconds(CARRIER_ON_US);
            tx_off();
            interrupts();

            period_end_us += CARRIER_PERIOD_US;

            do {
                now_us = micros();
                if (static_cast<s32>(now_us - mark_end_us) >= 0) {
                    return;
                }
            } while (static_cast<s32>(now_us - period_end_us) < 0);
        }
    }

    void space(u16 space_us) {
        carrier_off();
        if (space_us > 0) {
            delayMicroseconds(space_us);
        }
    }

    void tx_on() {
        tx_pin_.on();
    }

    void tx_off() {
        tx_pin_.off();
    }
};
