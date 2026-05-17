//  manage light sensor
//
//  see LICENSE file for terms

#pragma once
#include "app_event.h"
#include "driver/tcs34725.h"

#include "lite/color/gamma_lut.h"
#include "lite/core/event_bus.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"
#include "lite/core/timer.h"
#include "lite/core/changed.h"

#define LOG_TAG         light
#define LOG_LEVEL       LIGHT_SENSOR_LOG

//==============================================================================
class LightMeter {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<AppEvent>;

public:
    LightMeter(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_tcs_(MSG_THIS(on_timer_))
        , tcs_(twi, LIGHT_SENSOR_I2C_ADDR)
    {
        set_state_();
        timer_tcs_.start_periodic(1s);
    }

//------------------------------------------------------------------------------    
private:
    EventBus&       event_bus_;
    lite::Timer     timer_tcs_;
    Tcs34725        tcs_;

    //--------------------------------------------------------------------------
    enum : u8 {
        OFFLINE,
        ONLINE,
        UNKNOWN
    };
    u8 state_  = UNKNOWN;

    void set_state_() {
        if ( lite::changed(state_, tcs_.state() ? ONLINE : OFFLINE) ) {
            if (state_) LOG_INFO("online");
            else        LOG_WARN("offline");
            event_bus_.publish( {AppEventId::LIGHT_STATE, { .light_state = { bool(state_) }}} );
        }
    }

    //--------------------------------------------------------------------------
    void on_timer_() {
        auto r = tcs_.read_data();
        set_state_();
        if (!r.valid) return;

        LOG_INFO("c=%05u r=%05u g=%05u b=%05u", r.y, r.r, r.g, r.b);

        event_bus_.publish({AppEventId::LIGHT_LUM, { .light_lum = { 
            .y = raw_to_u8_(r.y)
        }}});
        event_bus_.publish({AppEventId::LIGHT_RGB, { .light_rgb = { 
            .r = raw_to_u8_(r.r), 
            .g = raw_to_u8_(r.g), 
            .b = raw_to_u8_(r.b) } 
        }});
    }

    //--------------------------------------------------------------------------
    u8 raw_to_u8_(u16 raw_count) {
        auto counts = tcs_.full_scale_counts();

        const u32 clamped = std::min(static_cast<u32>(raw_count), counts);
        const u32 scaled = (clamped * 1023u + (counts / 2u)) / counts;
        return lite::gamma_u10_to_u8(scaled);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
