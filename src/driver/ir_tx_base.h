//  manage ir transmitter
//
//  see LICENSE file for terms

#pragma once

#include "ir_code.h"

#include "lite/core/types.h"
#include "lite/core/crtp.h"

using lite::u8;
using lite::u16;
using lite::u32;

//==============================================================================
template <typename Derived>
class IrTxBase : public lite::CrtpBase<Derived> {
//------------------------------------------------------------------------------    
public:
    IrTxBase() noexcept = default;

    bool is_ready() const noexcept {
        return true;
    }
    void tick() {
        //  default implementation does nothing
    }

    void tx(IrCode code) {
        if (code.is_invalid()) {
            return;
        }

        if (code.is_repeat()) {
            tx_repeat();
            return;
        }

        tx_raw(code.raw());
    }

    void tx_nec(u8 addr, u8 cmd) {
        tx(IrCode::from_nec(addr, cmd));
    }

    void tx_raw(
        u32 raw_frame, 
        u16 header_mark_us = HEADER_MARK_US,
        u16 header_space_us = HEADER_SPACE_US
    ) {
        this->derived().send_data_frame(raw_frame, header_mark_us, header_space_us);
    }

    void tx_repeat() {
        this->derived().send_repeat_frame(HEADER_MARK_US, REPEAT_SPACE_US);
    }

//------------------------------------------------------------------------------    
private:
    static constexpr u16 HEADER_MARK_US   = 9000;
    static constexpr u16 HEADER_SPACE_US  = 4500;
    static constexpr u16 REPEAT_SPACE_US  = 2250;
};