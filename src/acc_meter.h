//  manage accelerometer
//
//  see LICENSE file for terms

#pragma once
#include "status.h"
#include "app_event.h"
#include "driver/bma253.h"

#include "lite/core/event_bus.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/timer.h"

#define LOG_TAG         acc
#define LOG_LEVEL       ACC_METER_LOG

//==============================================================================
class AccMeter {
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
                case SENSOR_ERROR:      return "sensor error";
                default:                return Status::str();
            }
        }
    };

    AccMeter(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_acc_(MSG_THIS(on_timer_))
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
    MeterStatus    device_status_;

    //--------------------------------------------------------------------------
    void on_timer_() {
        auto r = acc_.read_data();
        if (!r.read_state.is_ok()) return;

        LOG_INFO(
            "x=%+4d y=%+4d z=%+4d %d°C",
            int(r.x), int(r.y), int(r.z), int(r.temp)
        );
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
