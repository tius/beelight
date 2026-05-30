//  manage mp2667 charger
//
//  see LICENSE file for terms

#pragma once

#include "driver/charger.h"

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
        ChargerState state;

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

        const auto raw = decode_raw_state(status, fault);

        return {
            .read_state = last_read_status_,
            .state = decode_state(raw),
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

        const auto raw = decode_raw_state(last_status_, last_fault_);
        const auto state = decode_state(raw);

        append_charge_detail(text, raw, state);
        append_flag_details(text, raw, state.status);
        append_fault_details(text, raw, state.status);

        return text.c_str();
    }

//------------------------------------------------------------------------------
private:
    struct RawState {
        enum ChargeState : u8 {
            NOT_CHARGING = 0,
            PRE_CHARGE,
            CHARGING,
            CHARGE_DONE,
        };

        enum Flag : u8 {
            THERMAL_REG = 1u << 0u,
            POWER_GOOD  = 1u << 1u,
            PPM         = 1u << 2u,
        };

        enum Fault : u8 {
            SAFETY_TIMER_FAULT = 1u << 2u,
            BAT_FAULT          = 1u << 3u,
            THERMAL_SHUTDOWN   = 1u << 4u,
            VIN_FAULT          = 1u << 5u,
            WATCHDOG_FAULT     = 1u << 6u,
        };

        u8 charge_state = NOT_CHARGING;
        u8 flags = 0;
        u8 faults = 0;

        bool power_good() const noexcept {
            return (flags & POWER_GOOD) != 0u;
        }

        bool power_path_management() const noexcept {
            return (flags & PPM) != 0u;
        }

        bool thermal_regulation() const noexcept {
            return (flags & THERMAL_REG) != 0u;
        }

        bool has_fault() const noexcept {
            return faults != 0u;
        }
    };

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

    static constexpr u8 FAULT_MASK =
                    RawState::SAFETY_TIMER_FAULT
                | RawState::BAT_FAULT
                | RawState::THERMAL_SHUTDOWN
                | RawState::VIN_FAULT
                | RawState::WATCHDOG_FAULT
    ;

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

    static RawState decode_raw_state(u8 status, u8 fault) noexcept {
        return {
            .charge_state = static_cast<u8>(
                (status & STATUS_CHG_STAT_MASK) >> STATUS_CHG_STAT_SHIFT
            ),
            .flags = decode_flags(status),
            .faults = static_cast<u8>(fault & FAULT_MASK),
        };
    }

    static u8 decode_revision(u8 status) noexcept {
        return static_cast<u8>((status & STATUS_REV_MASK) >> STATUS_REV_SHIFT);
    }

    static ChargerState decode_state(const RawState& raw) noexcept {
        return {
            .mode = decode_mode(raw),
            .status = decode_status(raw),
        };
    }

    static ChargerMode decode_mode(const RawState& raw) noexcept {
        if (!raw.power_good()) {
            return { ChargerMode::ON_BATTERY };
        }

        if (
               raw.charge_state == RawState::PRE_CHARGE
            || raw.charge_state == RawState::CHARGING
        ) {
            return { ChargerMode::CHARGING };
        }

        return { ChargerMode::EXT_POWER };
    }

    static ChargerStatus decode_status(const RawState& raw) noexcept {
        if ((raw.faults & RawState::THERMAL_SHUTDOWN) != 0u) {
            return { ChargerStatus::TOO_HOT };
        }
        if ((raw.faults & RawState::BAT_FAULT) != 0u) {
            return { ChargerStatus::BATTERY_FAULT };
        }
        if ((raw.faults & RawState::VIN_FAULT) != 0u) {
            return { ChargerStatus::INPUT_FAULT };
        }
        if ((raw.faults & RawState::SAFETY_TIMER_FAULT) != 0u) {
            return { ChargerStatus::SAFETY_TIMER };
        }
        if ((raw.faults & RawState::WATCHDOG_FAULT) != 0u) {
            return { ChargerStatus::WATCHDOG };
        }
        if (raw.thermal_regulation()) {
            return { ChargerStatus::THERMAL_LIMIT };
        }
        if (raw.power_path_management()) {
            return { ChargerStatus::POWER_LIMIT };
        }
        if (
               raw.power_good()
            && raw.charge_state == RawState::CHARGE_DONE
        ) {
            return { ChargerStatus::CHARGE_COMPLETE };
        }

        return { ChargerStatus::OK };
    }

    static u8 decode_flags(u8 status) noexcept {
        u8 flags = 0;

        if ((status & STATUS_THERM_STAT) != 0u) {
            flags |= RawState::THERMAL_REG;
        }
        if ((status & STATUS_PG_STAT) != 0u) {
            flags |= RawState::POWER_GOOD;
        }
        if ((status & STATUS_PPM_STAT) != 0u) {
            flags |= RawState::PPM;
        }

        return flags;
    }

    static void append_charge_detail(
        lite::TextBuffer& text,
        const RawState& raw,
        ChargerState state
    ) {
        if (raw.charge_state == RawState::PRE_CHARGE) {
            append_detail(text, "pre_charge");
            return;
        }

        if (
               raw.charge_state == RawState::CHARGING
            && state.mode.code != ChargerMode::CHARGING
        ) {
            append_detail(text, "charging");
            return;
        }

        if (
               raw.charge_state == RawState::CHARGE_DONE
            && state.status.code != ChargerStatus::CHARGE_COMPLETE
        ) {
            append_detail(text, "charge_done");
        }
    }

    static void append_flag_details(
        lite::TextBuffer& text,
        const RawState& raw,
        ChargerStatus status
    ) {
        if (
               raw.thermal_regulation()
            && status.code != ChargerStatus::THERMAL_LIMIT
        ) {
            append_detail(text, "thermal_reg");
        }

        if (
               raw.power_path_management()
            && status.code != ChargerStatus::POWER_LIMIT
        ) {
            append_detail(text, "ppm");
        }
    }

    static void append_fault_details(
        lite::TextBuffer& text,
        const RawState& raw,
        ChargerStatus status
    ) {
        append_fault_detail(
            text,
            raw,
            status,
            RawState::THERMAL_SHUTDOWN,
            "thermal_shutdown"
        );
        append_fault_detail(
            text,
            raw,
            status,
            RawState::BAT_FAULT,
            "battery_fault"
        );
        append_fault_detail(
            text,
            raw,
            status,
            RawState::VIN_FAULT,
            "input_fault"
        );
        append_fault_detail(
            text,
            raw,
            status,
            RawState::SAFETY_TIMER_FAULT,
            "safety_timer"
        );
        append_fault_detail(
            text,
            raw,
            status,
            RawState::WATCHDOG_FAULT,
            "watchdog"
        );
    }

    static void append_fault_detail(
        lite::TextBuffer& text,
        const RawState& raw,
        ChargerStatus status,
        u8 fault,
        const char* detail
    ) {
        if ((raw.faults & fault) == 0u) {
            return;
        }

        if (status_represents_fault(status, fault)) {
            return;
        }

        append_detail(text, detail);
    }

    static bool status_represents_fault(
        ChargerStatus status,
        u8 fault
    ) noexcept {
        switch (status.code) {
            case ChargerStatus::TOO_HOT:
                return fault == RawState::THERMAL_SHUTDOWN;
            case ChargerStatus::BATTERY_FAULT:
                return fault == RawState::BAT_FAULT;
            case ChargerStatus::INPUT_FAULT:
                return fault == RawState::VIN_FAULT;
            case ChargerStatus::SAFETY_TIMER:
                return fault == RawState::SAFETY_TIMER_FAULT;
            case ChargerStatus::WATCHDOG:
                return fault == RawState::WATCHDOG_FAULT;
            default:
                return false;
        }
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

    static const char* charge_state_str(u8 state) noexcept {
        switch (state) {
            case RawState::PRE_CHARGE:      return "pre_charge";
            case RawState::CHARGING:        return "charging";
            case RawState::CHARGE_DONE:     return "charge_done";
            default:                        return "not_charging";
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL