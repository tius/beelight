//  manage accelerometer
//
//  see LICENSE file for terms

#pragma once
#include "app_event.h"
#include "driver/bma253.h"

#include "lite/core/event_bus.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/timer.h"
#include "lite/core/changed.h"

#define LOG_TAG         acc
#define LOG_LEVEL       ACC_METER_LOG

//==============================================================================
class AccMeter {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<AppEvent>;

public:
    AccMeter(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_acc_(MSG_THIS(on_timer_))
        , acc_(twi, BMA253_I2C_ADDR)
    {
        set_state_();
        timer_acc_.start_periodic(1s);
    }

//------------------------------------------------------------------------------    
private:
    EventBus&       event_bus_;
    lite::Timer     timer_acc_;
    Bma253          acc_;

    //--------------------------------------------------------------------------
    enum : u8 {
        OFFLINE,
        ONLINE,
        UNKNOWN
    };
    u8 state_  = UNKNOWN;

    void set_state_() {
        if ( lite::changed(state_,  acc_.state() ? ONLINE : OFFLINE ) ) {
            if (state_) LOG_INFO("online");
            else        LOG_WARN("offline");
            event_bus_.publish( {AppEventId::ACC_STATE, { .acc_state = { bool(state_) }}} );
        }
    }

    //--------------------------------------------------------------------------
    void on_timer_() {
        auto r = acc_.read_data();
        set_state_();
        if (!r.valid) return;

        LOG_INFO(
            "x=%+4d y=%+4d z=%+4d %d°C",
            int(r.x), int(r.y), int(r.z), int(r.temp)
        );
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
