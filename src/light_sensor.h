//  manage light sensor tcs34725
//
//  see LICENSE file for terms

#pragma once
#include "app_event.h"
#include "driver/Tcs34725.h"

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
class LightSensor {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<AppEvent>;

public:
    LightSensor(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_tcs_(MSG_THIS(on_timer_))
        , tcs_(twi, LIGHT_SENSOR_I2C_ADDR)
    {
        set_state_( tcs_.state() ? ONLINE : OFFLINE );
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

    void set_state_(u8 new_state) {
        if ( lite::changed(state_, new_state) ) {
            if (state_) LOG_INFO("online");
            else        LOG_WARN("offline");
            event_bus_.publish( {AppEventId::LIGHT_STATE, state_} );
        }
    }

    //--------------------------------------------------------------------------
    void on_timer_() {
        u16 data_u16[4];

        auto r = tcs_.read_data(data_u16);
        set_state_( tcs_.state() ? ONLINE : OFFLINE );
        if (!r) return;

        LOG_INFO("c=%05u r=%05u g=%05u b=%05u", data_u16[0], data_u16[1], data_u16[2], data_u16[3]);

        u8 data_u8[4];
        for (int i = 0; i < 4; i++) {
            data_u8[i] = lite::gamma_u10_to_u8( to_linear_u10_(data_u16[i]) );
        }
        event_bus_.publish({AppEventId::LIGHT_DATA, pack_data_(data_u8)});
    }

    //--------------------------------------------------------------------------
    u16 to_linear_u10_(u16 raw_count) {
        auto counts = tcs_.full_scale_counts();

        const u32 clamped = std::min(static_cast<u32>(raw_count), counts);
        const u32 scaled = (clamped * 1023u + (counts / 2u)) / counts;
        return static_cast<u16>(scaled);
    }

    static u32 pack_data_(const u8* data) {
        return (static_cast<u32>(data[0]) << 24u)
            | (static_cast<u32>(data[1]) << 16u)
            | (static_cast<u32>(data[2]) << 8u)
            | static_cast<u32>(data[3]);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
