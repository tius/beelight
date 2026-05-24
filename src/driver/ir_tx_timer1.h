//  ir transmitter using timer1 backend
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
class IrTxTimer1 : public IrTxBase<IrTxTimer1> {
//-----------------------------------------------------------------------------
using EventBus = lite::EventBus<AppEvent>;
using EventHook = lite::EventHook<AppEvent>;
//------------------------------------------------------------------------------
public:
    explicit IrTxTimer1(EventBus& event_bus) noexcept
        : event_bus_(event_bus) {}

    void send_data_frame(u32 raw_frame, u16 mark_us, u16 space_us) {
    }

    void send_repeat_frame(u16 mark_us, u16 space_us) {
    }

//------------------------------------------------------------------------------
private:
    using TxPin = lite::gpio::Output<IR_TX_GPIO>;

    EventBus&   event_bus_;
    TxPin       tx_pin_{};

};
