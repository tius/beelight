//  manage accelerometer
//
//  see LICENSE file for terms

#pragma once
#include <cmath>

#include "status.h"
#include "event/event.h"
#include "driver/bma253.h"

#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/timer.h"

#define LOG_TAG         acc
#define LOG_LEVEL       ACC_METER_LOG

//==============================================================================
class AccMeter {
//------------------------------------------------------------------------------    
using EventBus = event::Bus;

public:
    struct MeterStatus : public Status {
        enum : u8 {
            OK = 0,
            SENSOR_ERROR,
        };

        const char* str() const noexcept {
            switch (code) {
                case SENSOR_ERROR:      return "sensor error";
                default:                return Status::str();
            }
        }
    };

    AccMeter(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_acc_(MSG_THIS(on_timer))
        , acc_(twi)
    {
        if (acc_) {
            timer_acc_.start_periodic(1s);
        }
        else {
            LOG_ERROR("bma253 init failed: %s", acc_.status().str());
            device_status_ = { MeterStatus::SENSOR_ERROR };
        }
    }

    operator bool() const noexcept {
        return device_status_.is_ok();
    }

    MeterStatus status() const noexcept {
        return device_status_;
    }

//------------------------------------------------------------------------------    
private:
    EventBus&       event_bus_;
    lite::Timer     timer_acc_;
    Bma253          acc_;
    MeterStatus     device_status_;
    u8              roll_last_ = 128u;

    static constexpr float k_rad_to_deg = 57.2957795131f;
    static constexpr int k_g_counts = 256;
    static constexpr int k_g_sq = k_g_counts * k_g_counts;
    static constexpr int k_norm_min_sq = (k_g_sq * 9) / 25;
    static constexpr int k_norm_max_sq = (k_g_sq * 64) / 25;
    static constexpr float k_roll_norm_min = 8.0f;

    //--------------------------------------------------------------------------
    void on_timer() {
        auto r = acc_.read_data();
        if (!r.read_state.is_ok()) return;

        LOG_INFO(
            "x=%3d y=%6d z=%6d celsius10=%d",
            r.x, r.y, r.z, r.celsius10
        );
        publish_temp(r.celsius10);
        publish_tilt(r.x, r.y, r.z);
    }

    //--------------------------------------------------------------------------
    void publish_temp(s16 celsius10) {
        event_bus_.publish({ event::Id::TEMP, { .temp = { 
            .celsius10 = celsius10 
        }}});
    }

    //--------------------------------------------------------------------------
    void publish_tilt(s16 x, s16 y, s16 z) {
        const int x_i = static_cast<int>(x);
        const int y_i = static_cast<int>(y);
        const int z_i = static_cast<int>(z);

        //  skip orientation update when acceleration norm is implausible
        const int norm_sq = (x_i * x_i) + (y_i * y_i) + (z_i * z_i);
        if (norm_sq < k_norm_min_sq || norm_sq > k_norm_max_sq) {
            LOG_DEBUG("acc norm out of range: n2=%d", norm_sq);
            return;
        }

        const float x_f = static_cast<float>(x_i);
        const float y_f = static_cast<float>(y_i);
        const float z_f = static_cast<float>(z_i);

        const float yz_norm     = std::hypot(y_f, z_f);
        const float pitch_deg   = std::atan2(x_f, yz_norm) * k_rad_to_deg;
        const float roll_deg    = std::atan2(y_f, z_f) * k_rad_to_deg;

        u8 pitch_u8 = map_angle_u8(pitch_deg, -90.0f, 90.0f, 127u);

        //  keep last roll value near gimbal lock to avoid erratic changes
        const bool roll_ok = yz_norm >= k_roll_norm_min;
        u8 roll_u8  = roll_last_;
        if (roll_ok) {
            roll_u8     = map_angle_u8(roll_deg, -180.0f, 180.0f, 255u);
            roll_last_  = roll_u8;
        }
        LOG_INFO("pitch=%u roll=%u", int(pitch_u8), int(roll_u8));

        event_bus_.publish({ event::Id::TILT, { .tilt = {
            .pitch  = pitch_u8,
            .roll   = roll_u8
        }}});
    }


    //  map degrees from [min_deg, max_deg] to [0, max_code]
    static u8 map_angle_u8(
        float deg,
        float min_deg,
        float max_deg,
        u8 max_code = 255u
    ) {
        if (deg < min_deg) {
            deg = min_deg;
        }
        else if (deg > max_deg) {
            deg = max_deg;
        }

        const float norm = (deg - min_deg) / (max_deg - min_deg);
        const float scaled = norm * static_cast<float>(max_code);
        return static_cast<u8>(scaled + 0.5f);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
