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
            mark_(BIT_MARK_US);

            const bool is_one = (raw_frame & 1u) != 0;
            space_(is_one ? ONE_SPACE_US : ZERO_SPACE_US);
            raw_frame >>= 1;
        }

        mark_(BIT_MARK_US);
        carrier_off_();
    }

    void send_repeat_frame(u16 mark_us, u16 space_us) {
        mark_(mark_us);
        space_(space_us);
        mark_(BIT_MARK_US);
        carrier_off_();
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

    void carrier_off_() {
        tx_off_();
    }

    void mark_(u16 mark_us) {
        u32 now_us = micros();
        u32 period_end_us = now_us;
        const u32 mark_end_us = now_us + mark_us;

        while (true) {
            noInterrupts();
            tx_on_();
            delayMicroseconds(CARRIER_ON_US);
            tx_off_();
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

    void space_(u16 space_us) {
        carrier_off_();
        if (space_us > 0) {
            delayMicroseconds(space_us);
        }
    }

    void tx_on_() {
        tx_pin_.on();
    }

    void tx_off_() {
        tx_pin_.off();
    }
};
