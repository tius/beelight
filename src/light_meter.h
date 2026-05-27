//  manage light sensor
//
//  see LICENSE file for terms

#pragma once
#include "status.h"
#include "run_event.h"

#include "lite/color/gamma_lut.h"
#include "lite/core/event_bus.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"
#include "lite/core/timer.h"

//==============================================================================
//  config values
//------------------------------------------------------------------------------
#ifndef LIGHT_SENSOR_LOG
    #define LIGHT_SENSOR_LOG       none        // log level for light sensor events
#endif
#ifndef LIGHT_SENSOR_TYPE
    #error LIGHT_SENSOR_TYPE not defined
#endif

//------------------------------------------------------------------------------
#define LIGHT_SENSOR_TYPE_TCS34725  1
#define LIGHT_SENSOR_TYPE_VEML3328  2
#define LIGHT_SENSOR_TYPE_NUM       XCAT(LIGHT_SENSOR_TYPE_, LIGHT_SENSOR_TYPE)

//------------------------------------------------------------------------------
#if LIGHT_SENSOR_TYPE_NUM == LIGHT_SENSOR_TYPE_TCS34725
    #include "driver/tcs34725.h"
#elif LIGHT_SENSOR_TYPE_NUM == LIGHT_SENSOR_TYPE_VEML3328
    #include "driver/veml3328.h"
#endif

#define LOG_TAG         light
#define LOG_LEVEL       LIGHT_SENSOR_LOG

//==============================================================================
class LightMeter {
//------------------------------------------------------------------------------    
using EventBus = lite::EventBus<RunEvent>;

#if LIGHT_SENSOR_TYPE_NUM == LIGHT_SENSOR_TYPE_TCS34725
    using Driver = Tcs34725;
#elif LIGHT_SENSOR_TYPE_NUM == LIGHT_SENSOR_TYPE_VEML3328
    using Driver = Veml3328;
#else
    #error unsupported LIGHT_SENSOR_TYPE
#endif

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
        , timer_sensor_(MSG_THIS(on_timer))
        , sensor_(twi)
    {
        if (sensor_) {
            timer_sensor_.start_periodic(1s);
        }
        else {
            LOG_ERROR("init failed: %s", sensor_.status().str());
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
    lite::Timer     timer_sensor_;
    Driver    sensor_;
    MeterStatus    device_status_;

    //--------------------------------------------------------------------------
    void on_timer() {
        auto r = sensor_.read_data();
        if (!r.read_state.is_ok()) return;

        LOG_INFO("c=%05u r=%05u g=%05u b=%05u", r.y, r.r, r.g, r.b);

        event_bus_.publish({RunEventId::LIGHT_LUM, { .light_lum = { 
            .y = raw_to_u8(r.y)
        }}});
        event_bus_.publish({RunEventId::LIGHT_RGB, { .light_rgb = { 
            .r = raw_to_u8(r.r),
            .g = raw_to_u8(r.g),
            .b = raw_to_u8(r.b)
        }}});
    }

    //--------------------------------------------------------------------------
    u8 raw_to_u8(u16 raw_count) {
        auto counts = sensor_.full_scale_counts();

        const u32 clamped = std::min(static_cast<u32>(raw_count), counts);
        const u32 scaled = (clamped * 1023u + (counts / 2u)) / counts;
        return lite::gamma_u10_to_u8(scaled);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
