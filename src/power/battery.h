//  manage battery gauge state
//
//  see LICENSE file for terms

#pragma once

#include "driver/bq27421.h"
#include "event/event.h"
#include "settings.h"

#include "lite/core/changed.h"
#include "lite/core/status.h"
#include "lite/core/text_buffer.h"
#include "lite/core/timer.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"

#define LOG_TAG         battery
#define LOG_LEVEL       BATTERY_LOG

//==============================================================================
class Battery final {
    using u8 = lite::u8;
    using EventBus = event::Bus;

//------------------------------------------------------------------------------
public:
    struct Status : public lite::Status {
        enum : u8 {
            OK = 0,
            GAUGE_ERROR,
        };

        const char* str() const noexcept {
            switch (code) {
                case GAUGE_ERROR: return "gauge error";
                default:          return lite::Status::str();
            }
        }
    };

    Battery(
        lite::Twi& twi,
        EventBus& event_bus,
        lite::duration_ms timer_offset
    )
        : event_bus_(event_bus)
        , timer_(MSG_THIS(on_timer))
        , gauge_(twi)
    {
        if (gauge_) {
            char device_info[80];
            LOG_INFO("%s", gauge_.fmt_device_info(device_info));
            timer_.start_periodic(
                lite::duration_ms{BATTERY_SAMPLE_MS},
                timer_offset
            );
        }
        else {
            LOG_ERROR("bq27421 init failed: %s", gauge_.status().str());
            status_ = { Status::GAUGE_ERROR };
            publish_error();
        }
    }

    constexpr explicit operator bool() const noexcept {
        return status_.is_ok();
    }

    Status status() const noexcept {
        return status_;
    }

    Bq27421::Result read_state() {
        return gauge_.read_status();
    }

//------------------------------------------------------------------------------
private:
    EventBus&           event_bus_;
    lite::Timer         timer_;
    Bq27421             gauge_;
    Status              status_;
    event::BatteryInfo  published_info_;
    Bq27421::Details    logged_details_;

    void on_timer() {
        const auto update_state = gauge_.update();
        if (update_state.is_error()) {
            LOG_WARN("update failed: %s", update_state.str());
        }

        const auto result = gauge_.read_status();
        if (!result) {
            if (result.read_state.code == Bq27421::ReadStatus::ERR_NOT_INIT) {
                publish_error();
            }
            return;
        }

        const auto info = event::BatteryInfo{
            result.state.soc_percent,
            result.state.average_current_ma,
            result.state.charging(),
            result.state.discharging(),
            result.state.soc_percent <= BATTERY_LOW_SOC_PERCENT,
            result.state.soc_percent <= BATTERY_CRIT_SOC_PERCENT
        };
        if (lite::changed(published_info_, info)) {
            event_bus_.publish({ event::Id::BATTERY_INFO, { .battery_info =
                published_info_
            }});
            char buffer[48];
            LOG_INFO("%s", info.fmt(buffer));
        }

        if constexpr (LOG_ENABLED(trace)) {
            const auto details = gauge_.read_details();
            if (details && lite::changed(logged_details_, details)) {
                char buffer[48];
                LOG_TRACE("%s", details.fmt(buffer));
            }
        }
    }

    void publish_error() {
        const auto info = event::BatteryInfo::error();

        if (lite::changed(published_info_, info)) {
            event_bus_.publish({ event::Id::BATTERY_INFO, { .battery_info =
                published_info_
            }});
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL