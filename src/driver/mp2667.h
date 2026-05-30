//  manage mp2667 charger
//
//  see LICENSE file for terms

#pragma once

#include "power/power_state.h"

#include "lite/core/status.h"
#include "lite/core/text_buffer.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"

#include <cstddef>

#ifndef MP2667_I2C_ADDR
    #define MP2667_I2C_ADDR     0x09
#endif
#ifndef MP2667_LOG
    #define MP2667_LOG          none
#endif

#define LOG_TAG         mp2667
#define LOG_LEVEL       MP2667_LOG

//==============================================================================
class Mp2667 {
using u8 = lite::u8;

//------------------------------------------------------------------------------
public:
    struct DeviceStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_READ_INFO,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:     return "probe failed";
                case ERR_READ_INFO: return "read info failed";
                default:            return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct ReadStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_READ_STATUS,
            ERR_READ_FAULT,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:      return "not initialized";
                case ERR_READ_STATUS:   return "status read failed";
                case ERR_READ_FAULT:    return "fault read failed";
                default:                return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct Result {
        ReadStatus read_state;
        PowerState state;

        operator bool() const noexcept {
            return read_state.is_ok();
        }
    };

    //--------------------------------------------------------------------------
    explicit Mp2667(lite::Twi& twi)
        : twi_(twi)
    {
        device_status_ = init();
    }

    DeviceStatus status() const noexcept {
        return device_status_;
    }

    operator bool() const noexcept {
        return device_status_.is_ok();
    }

    //--------------------------------------------------------------------------
    Result read_status() {
        if (!device_status_) {
            last_read_status_ = { ReadStatus::ERR_NOT_INIT };
            return { .read_state = last_read_status_ };
        }

        u8 status = 0;
        if (!read_reg_u8(REG_SYSTEM_STATUS, status)) {
            last_read_status_ = { ReadStatus::ERR_READ_STATUS };
            return { .read_state = last_read_status_ };
        }
        last_status_ = status;

        u8 fault = 0;
        if (!read_reg_u8(REG_FAULT, fault)) {
            last_read_status_ = { ReadStatus::ERR_READ_FAULT };
            return { .read_state = last_read_status_ };
        }
        last_fault_ = fault;
        last_read_status_ = { ReadStatus::OK };

        return {
            .read_state = last_read_status_,
            .state = decode_state(status, fault),
        };
    }

    template <std::size_t N>
    const char* fmt_device_info(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("mp2667 rev %u", revision_);
        return text.c_str();
    }

    template <std::size_t N>
    const char* fmt_last_read_details(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);

        if (last_read_status_.code == ReadStatus::ERR_NOT_INIT) {
            return text.c_str();
        }

        if (last_read_status_.code == ReadStatus::ERR_READ_STATUS) {
            return text.c_str();
        }

        if (last_read_status_.code == ReadStatus::ERR_READ_FAULT) {
            text.appendf("status 0x%02X", last_status_);
            return text.c_str();
        }

        const auto state = decode_state(last_status_, last_fault_);

        append_charge_detail(text, last_status_, state);
        append_status_details(text, last_status_);
        append_fault_details(text, last_status_, last_fault_);

        return text.c_str();
    }

//------------------------------------------------------------------------------
private:
    lite::Twi& twi_;
    DeviceStatus device_status_;
    u8 revision_ = 0;
    ReadStatus last_read_status_ = { ReadStatus::ERR_NOT_INIT };
    u8 last_status_ = 0;
    u8 last_fault_ = 0;

    static constexpr u8 I2C_ADDR = MP2667_I2C_ADDR;

    static constexpr u8 REG_SYSTEM_STATUS = 0x07;
    static constexpr u8 REG_FAULT = 0x08;

    static constexpr u8 STATUS_THERM_STAT = 0x01;
    static constexpr u8 STATUS_PG_STAT = 0x02;
    static constexpr u8 STATUS_PPM_STAT = 0x04;
    static constexpr u8 STATUS_CHG_STAT_SHIFT = 3u;
    static constexpr u8 STATUS_CHG_STAT_MASK = 0x18;
    static constexpr u8 STATUS_REV_SHIFT = 5u;
    static constexpr u8 STATUS_REV_MASK = 0x60;

    static constexpr u8 CHG_PRE_CHARGE = 1;
    static constexpr u8 CHG_CHARGING = 2;
    static constexpr u8 CHG_CHARGE_DONE = 3;

    static constexpr u8 FAULT_SAFETY_TIMER = 1u << 2u;
    static constexpr u8 FAULT_BATTERY = 1u << 3u;
    static constexpr u8 FAULT_THERMAL_SHUTDOWN = 1u << 4u;
    static constexpr u8 FAULT_INPUT = 1u << 5u;
    static constexpr u8 FAULT_WATCHDOG = 1u << 6u;

    DeviceStatus init() {
        if (twi_.probe(I2C_ADDR).is_error()) {
            return { DeviceStatus::ERR_PROBE };
        }

        u8 status = 0;
        if (!read_reg_u8(REG_SYSTEM_STATUS, status)) {
            return { DeviceStatus::ERR_READ_INFO };
        }

        revision_ = decode_revision(status);

        return { DeviceStatus::OK };
    }

    bool read_reg_u8(u8 reg, u8& value) {
        return twi_.write_read(I2C_ADDR, reg, value).is_ok();
    }

    static u8 decode_revision(u8 status) noexcept {
        return static_cast<u8>((status & STATUS_REV_MASK) >> STATUS_REV_SHIFT);
    }

    static PowerState decode_state(u8 status, u8 fault) noexcept {
        if (!power_good(status)) {
            return { PowerState::ON_BATTERY };
        }

        if (temp_fault(status, fault)) {
            return { PowerState::TEMP_FAULT };
        }

        if (charge_fault(fault)) {
            return { PowerState::CHARGE_FAULT };
        }

        if (power_limited(status, fault)) {
            return { PowerState::POWER_LIMITED };
        }

        if (charging(status)) {
            return { PowerState::CHARGING };
        }

        return { PowerState::EXT_POWER };
    }

    static void append_charge_detail(
        lite::TextBuffer& text,
        u8 status,
        PowerState state
    ) {
        const auto state_code = charge_state(status);

        if (state_code == CHG_PRE_CHARGE) {
            append_detail(text, "pre_charge");
            return;
        }

        if (state_code == CHG_CHARGING && state.code != PowerState::CHARGING) {
            append_detail(text, "charging");
            return;
        }

        if (
               state_code == CHG_CHARGE_DONE
            && state.code != PowerState::EXT_POWER
        ) {
            append_detail(text, "charge_done");
        }
    }

    static void append_status_details(
        lite::TextBuffer& text,
        u8 status
    ) {
        if (thermal_regulation(status)) {
            append_detail(text, "thermal_reg");
        }

        if (power_path_management(status)) {
            append_detail(text, "ppm");
        }
    }

    static void append_fault_details(
        lite::TextBuffer& text,
        u8 status,
        u8 fault
    ) {
        append_fault_detail(
            text,
            fault,
            FAULT_THERMAL_SHUTDOWN,
            "thermal_shutdown"
        );
        append_fault_detail(text, fault, FAULT_BATTERY, "battery_fault");

        if (power_good(status)) {
            append_fault_detail(text, fault, FAULT_INPUT, "input_fault");
        }

        append_fault_detail(text, fault, FAULT_SAFETY_TIMER, "safety_timer");
        append_fault_detail(text, fault, FAULT_WATCHDOG, "watchdog");
    }

    static void append_fault_detail(
        lite::TextBuffer& text,
        u8 fault_reg,
        u8 fault,
        const char* detail
    ) {
        if (!fault_active(fault_reg, fault)) {
            return;
        }

        append_detail(text, detail);
    }

    static void append_detail(
        lite::TextBuffer& text,
        const char* detail
    ) {
        if (detail == nullptr || detail[0] == '\0') {
            return;
        }

        if (!text.empty()) {
            text.append(' ');
        }
        text.append(detail);
    }

    static u8 charge_state(u8 status) noexcept {
        return static_cast<u8>(
            (status & STATUS_CHG_STAT_MASK) >> STATUS_CHG_STAT_SHIFT
        );
    }

    static bool power_good(u8 status) noexcept {
        return (status & STATUS_PG_STAT) != 0u;
    }

    static bool charging(u8 status) noexcept {
        const auto state = charge_state(status);
        return state == CHG_PRE_CHARGE || state == CHG_CHARGING;
    }

    static bool thermal_regulation(u8 status) noexcept {
        return (status & STATUS_THERM_STAT) != 0u;
    }

    static bool power_path_management(u8 status) noexcept {
        return (status & STATUS_PPM_STAT) != 0u;
    }

    static bool fault_active(u8 fault_reg, u8 fault) noexcept {
        return (fault_reg & fault) != 0u;
    }

    static bool temp_fault(u8 status, u8 fault) noexcept {
        return thermal_regulation(status)
            || fault_active(fault, FAULT_THERMAL_SHUTDOWN);
    }

    static bool charge_fault(u8 fault) noexcept {
        return fault_active(fault, FAULT_BATTERY)
            || fault_active(fault, FAULT_SAFETY_TIMER)
            || fault_active(fault, FAULT_WATCHDOG);
    }

    static bool power_limited(u8 status, u8 fault) noexcept {
        return power_path_management(status)
            || fault_active(fault, FAULT_INPUT);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL