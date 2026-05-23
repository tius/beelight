//  manage light sensor
//
//  see LICENSE file for terms

#pragma once
#include "status.h"
#include "app_event.h"
#include "driver/tcs34725.h"

#include "lite/color/gamma_lut.h"
#include "lite/core/event_bus.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"
#include "lite/core/timer.h"

#define LOG_TAG         light
#define LOG_LEVEL       LIGHT_SENSOR_LOG

//==============================================================================
class LightMeter {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<AppEvent>;

public:
    struct MeterStatus : public Status {
        enum : u8 {
            OK = 0,
            SENSOR_ERROR,
        };
        const char* str() const noexcept {
            switch (code) {
                case SENSOR_ERROR:  return "sensor error";
                default:            return Status::str();
            }
        }
    };

    LightMeter(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_tcs_(MSG_THIS(on_timer_))
        , tcs_(twi, TCS34725_I2C_ADDR)
    {
        if (tcs_) {
            timer_tcs_.start_periodic(1s);
        }
        else {
            LOG_ERROR("tcs34725 init failed: %s", tcs_.status().str());
            device_status_ = { MeterStatus::SENSOR_ERROR };
        }
    }

    operator bool() const noexcept {
        return device_status_.is_ok();
    }
    auto status() const noexcept {
        return device_status_;
    }

//------------------------------------------------------------------------------    
private:
    EventBus&       event_bus_;
    lite::Timer     timer_tcs_;
    Tcs34725        tcs_;
    MeterStatus    device_status_;

    //--------------------------------------------------------------------------
    void on_timer_() {
        auto r = tcs_.read_data();
        if (!r.read_state.is_ok()) return;

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
