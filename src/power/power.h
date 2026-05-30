//  manage system power state
//
//  see LICENSE file for terms

#pragma once

#include "event/event.h"
#include "driver/mp2667.h"

#include "lite/core/status.h"
#include "lite/core/text_buffer.h"
#include "lite/core/timer.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"

#define LOG_TAG         power
#define LOG_LEVEL       POWER_LOG

//==============================================================================
class Power {
using EventBus = event::Bus;
using u8 = lite::u8;

//------------------------------------------------------------------------------
public:
    struct PowerStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            CHARGER_ERROR,
        };

        const char* str() const noexcept {
            switch (code) {
                case CHARGER_ERROR: return "charger error";
                default:            return lite::Status::str();
            }
        }
    };

    Power(lite::Twi& twi, EventBus& event_bus)
        : event_bus_(event_bus)
        , timer_(MSG_THIS(on_timer))
        , charger_(twi)
    {
        if (charger_) {
            char device_info[32];
            LOG_INFO("%s", charger_.fmt_device_info(device_info));
            timer_.start_periodic(1s);
        }
        else {
            LOG_ERROR("mp2667 init failed: %s", charger_.status().str());
            status_ = { PowerStatus::CHARGER_ERROR };
        }
    }

    operator bool() const noexcept {
        return status_.is_ok();
    }

    PowerStatus status() const noexcept {
        return status_;
    }

//------------------------------------------------------------------------------
private:
    EventBus& event_bus_;
    lite::Timer timer_;
    Mp2667 charger_;
    PowerStatus status_;

    void on_timer() {
        const auto result = charger_.read_status();
        if (!result) {
            LOG_WARN("status read failed: %s", result.read_state.str());
            return;
        }

        char buffer[64];
        LOG_INFO("charger status: %s", fmt_result(buffer, result));

        event_bus_.publish({ event::Id::CHARGER_STATUS, { .charger_status =
            result.state
        }});
    }

    template <std::size_t N>
    const char* fmt_result(char (&buffer)[N], const Mp2667::Result& result) const {
        lite::TextBuffer text(buffer);

        text.append(result.state.mode.str());

        if (!result.state.status.is_ok()) {
            text.appendf(", %s", result.state.status.str());
        }

        char detail[40];
        charger_.fmt_last_read_details(detail);
        if (detail[0]) {
            text.appendf(" (%s)", detail);
        }

        return text.c_str();
    }
};

#undef LOG_TAG
#undef LOG_LEVEL