//  manage infrared receive and transmit
//
//  see LICENSE file for terms

#pragma once

#include "event/event.h"
#include "driver/ir_rx_edge.h"

#include "lite/core/status.h"
#include "lite/io/log.h"
#include "lite/core/timer.h"

//==============================================================================
//  config values
//------------------------------------------------------------------------------
#ifndef INFRARED_LOG
    #define INFRARED_LOG            none        // log level for infrared events    
#endif
#ifndef INFRARED_TX_DRIVER
    #error INFRARED_TX_DRIVER not defined       // method for infrared transmitter (e.g. BIT_BANG)
#endif
//------------------------------------------------------------------------------
#define INFRARED_TX_DRIVER_BIT_BANG  1
#define INFRARED_TX_DRIVER_TIMER1    2
#define INFRARED_TX_DRIVER_NUM       XCAT(INFRARED_TX_DRIVER_, INFRARED_TX_DRIVER)

#if INFRARED_TX_DRIVER_NUM == INFRARED_TX_DRIVER_BIT_BANG
    #include "driver/ir_tx_bit_bang.h"
    using IrTx = IrTxBitBang;
#elif INFRARED_TX_DRIVER_NUM == INFRARED_TX_DRIVER_TIMER1
    #include "driver/ir_tx_timer1.h"
    using IrTx = IrTxTimer1;
#else
    #error unsupported INFRARED_TX_DRIVER
#endif

using IrRx = IrRxEdge;

//------------------------------------------------------------------------------
#define LOG_TAG         ir
#define LOG_LEVEL       INFRARED_LOG

//==============================================================================
class Infrared {
//------------------------------------------------------------------------------    
using EventBus = event::Bus;

public:
    struct InfraredStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            SELFTEST_FAILED,
        };
        const char* str() const noexcept {
            switch (code) {
                case SELFTEST_FAILED:   return "selftest failed";
                default:                return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    explicit Infrared(EventBus& event_bus) noexcept
        : event_bus_(event_bus) 
    {
        self_test();
        timer_.start_periodic(500ms);
    }

    operator bool() const noexcept {
        return status_.is_ok();
    }

    auto status() const noexcept {
        return status_;
    }

    bool tick() {
        int work_done = ir_tx_.tick();

        if ( IrCode code = ir_rx_.read(); code ) {
            log_code(code);
            event_bus_.publish({ event::Id::IR_RX, { .ir_rx = {
                .code = code
            }}} );
            work_done++;
        }
        return work_done;
    }

//------------------------------------------------------------------------------    
private:
    EventBus&   event_bus_;
    IrTx        ir_tx_;
    IrRx        ir_rx_;
    InfraredStatus status_ = { InfraredStatus::OK };

    lite::Timer timer_      { MSG_THIS(on_timer) };
    u8          cnt_        = 0;;

    static constexpr u8 k_self_test_tries = 5;
    static constexpr u32 k_self_test_raw = 0x12345678u;

    void on_timer() {
        if (++cnt_ == 20) {
            timer_.stop();
        }

        tx(IrCode::from_nec(0x00, cnt_ % 2 ? 0x0D : 0x1F));
    }    

    void self_test() {
        for (int i = 0; i < k_self_test_tries; ++i) {
            if ( try_self_test() ) return;
        }
        status_ = { InfraredStatus::SELFTEST_FAILED };
        LOG_ERROR("self test failed");
    }

    bool try_self_test() {
        tx(IrCode::from_raw(k_self_test_raw));

        while (!ir_tx_.is_ready()) {
            ir_tx_.tick();
        }
 
        delay(10);
        const IrCode code = ir_rx_.read();
        if (code) {
            LOG_DEBUG(
                "self test: received %08lX",
                static_cast<unsigned long>(code.raw())
            );
        }
        else {
            LOG_WARN("self test: no signal");
        }

        return code.raw() == k_self_test_raw;
    }

    void tx(IrCode code) {
        ir_tx_.tx(code);
    }

    void log_code(IrCode code) {
        IrCode::Nec nec;
        if (code.decode_nec(nec)) {
            LOG_INFO("ir_rx %02X.%02X", nec.addr, nec.cmd);
            return;
        }

        if (code.is_repeat()) {
            LOG_INFO("ir_rx repeat");
            return;
        }

        LOG_INFO("ir_rx %08lX", static_cast<unsigned long>(code.raw()));
    }
};

#undef LOG_TAG
#undef LOG_LEVEL


