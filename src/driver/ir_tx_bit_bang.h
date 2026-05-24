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
        mark_(mark_us);
        space_(space_us);

        for (u8 bit_idx = 0; bit_idx < 32; ++bit_idx) {
            mark_(k_bit_mark_us_);

            const bool is_one = (raw_frame & 1u) != 0;
            space_(is_one ? k_one_space_us_ : k_zero_space_us_);
            raw_frame >>= 1;
        }

        mark_(k_bit_mark_us_);
        carrier_off_();
    }

    void send_repeat_frame(u16 mark_us, u16 space_us) {
        mark_(mark_us);
        space_(space_us);
        mark_(k_bit_mark_us_);
        carrier_off_();
    }

//------------------------------------------------------------------------------
private:
    using TxPin = lite::gpio::Output<IR_TX_GPIO>;

    static constexpr u16 k_bit_mark_us_      = 560;
    static constexpr u16 k_zero_space_us_    = 560;
    static constexpr u16 k_one_space_us_     = 1690;
    static constexpr u16 k_carrier_period_us_ = 26;
    static constexpr u16 k_carrier_on_us_     = 8;

    TxPin tx_pin_{};

    void carrier_off_() {
        tx_pin_.lo();
    }

    void mark_(u16 mark_us) {
        u32 now_us = micros();
        u32 period_end_us = now_us;
        const u32 mark_end_us = now_us + mark_us;

        while (true) {
            noInterrupts();
            tx_pin_.hi();
            delayMicroseconds(k_carrier_on_us_);
            tx_pin_.lo();
            interrupts();

            period_end_us += k_carrier_period_us_;

            do {
                now_us = micros();
                if (static_cast<s32>(now_us - mark_end_us) >= 0) {
                    return;
                }
            } while (static_cast<s32>(now_us - period_end_us) < 0);
        }
    }

    void space_(u16 space_us) {
        carrier_off_();
        if (space_us > 0) {
            delayMicroseconds(space_us);
        }
    }
};
