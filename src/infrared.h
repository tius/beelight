//  manage infrared receive and transmit
//
//  see LICENSE file for terms

#pragma once

#include "app_event.h"
#include "status.h"
#include "driver/ir_rx.h"

#include "lite/io/log.h"
#include "lite/core/event_bus.h"
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
#define INFRARED_TX_DRIVER_NUM       XCAT(INFRARED_TX_DRIVER_, INFRARED_TX_DRIVER)

#if INFRARED_TX_DRIVER_NUM == INFRARED_TX_DRIVER_BIT_BANG
    #include "driver/ir_tx_bit_bang.h"
#else
    #error unsupported INFRARED_TX_DRIVER
#endif

//------------------------------------------------------------------------------
#define LOG_TAG         ir
#define LOG_LEVEL       INFRARED_LOG

//==============================================================================
class Infrared {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<AppEvent>;

public:
    struct InfraredStatus : public Status {
        enum : u8 {
            OK = 0,
            SELFTEST_FAILED,
        };
        const char* str() const noexcept {
            switch (code) {
                case SELFTEST_FAILED:   return "selftest failed";
                default:                return Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    explicit Infrared(EventBus& event_bus) noexcept
        : event_bus_(event_bus) 
    {
        self_test_();
        timer_.start_periodic(500ms);
    }

    operator bool() const noexcept {
        return status_.is_ok();
    }

    auto status() const noexcept {
        return status_;
    }

    void tick() {
        auto r = ir_rx_.read();
        if (!r.is_valid) {
            return;
        }

        LOG_INFO("ir_rx %02X.%02X %s", r.addr, r.cmd, r.is_repeat ? "r" : "");

        event_bus_.publish({ AppEvent::Id::IR_RX, { .ir_rx = {
            .addr   = r.addr,
            .cmd    = r.cmd,
            .repeat = r.is_repeat
        }}} );
    }

//------------------------------------------------------------------------------    
private:
    EventBus&   event_bus_;
    #if INFRARED_TX_DRIVER_NUM == INFRARED_TX_DRIVER_BIT_BANG
        IrTxBitBang ir_tx_;
    #endif
    IrRx        ir_rx_;
    InfraredStatus status_ = { InfraredStatus::OK };

    lite::Timer timer_      { MSG_THIS(on_timer_) };
    u8          cnt_        = 0;;

    static const u8 k_self_test_tries = 5;

    void on_timer_() {
        if (++cnt_ == 20) {
            timer_.stop();
        }

        tx_nec_(
            0x0000,                     // address
            cnt_ % 2 ? 0x0D : 0x1F      // command
        ); 
    }    

    void self_test_() {
        for (int i = 0; i < k_self_test_tries; ++i) {
            if ( try_self_test_() ) return;
        }
        status_ = { InfraredStatus::SELFTEST_FAILED };
        LOG_ERROR("self test failed");
    }

    bool try_self_test_() {
        tx_nec_(0x12, 0x34);
        delay(10);
        auto r = ir_rx_.read();
        LOG_DEBUG("self test read: %02X.%02X %s", r.addr, r.cmd, r.is_repeat ? "r" : "");
        return r.is_valid && r.addr == 0x12 && r.cmd == 0x34;
    }

    void tx_nec_(u8 addr, u8 cmd) {
        event_bus_.publish({ AppEvent::Id::PWM_SUSPEND });
        ir_tx_.tx_nec(addr, cmd);
        event_bus_.publish({ AppEvent::Id::PWM_RESUME });
    }
};

#undef LOG_TAG
#undef LOG_LEVEL


