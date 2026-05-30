//  manage mp2667 charger
//
//  see LICENSE file for terms

#pragma once

#include "power/power_state.h"

#include "lite/core/compile_time.h"
#include "lite/core/status.h"
#include "lite/core/text_buffer.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"

#include <cstddef>

//==============================================================================
//  config values and defaults
//------------------------------------------------------------------------------
#ifndef MP2667_I2C_ADDR
    #define MP2667_I2C_ADDR         0x09
#endif
#ifndef MP2667_LOG
    #define MP2667_LOG              none
#endif
#ifndef MP2667_INPUT_MIN_MV
    #define MP2667_INPUT_MIN_MV     4600
#endif
#ifndef MP2667_INPUT_MA
    #define MP2667_INPUT_MA         470
#endif
#ifndef MP2667_CHARGE_MA
    #define MP2667_CHARGE_MA        257
#endif
#ifndef MP2667_TERM_MA
    #define MP2667_TERM_MA          52
#endif
#ifndef MP2667_DISCHARGE_MA
    #define MP2667_DISCHARGE_MA     1000
#endif
#ifndef MP2667_BATTERY_REG_MV
    #define MP2667_BATTERY_REG_MV   4200
#endif
#ifndef MP2667_BATTERY_UVLO_MV
    #define MP2667_BATTERY_UVLO_MV  2800
#endif
#ifndef MP2667_PRECHARGE_MV
    #define MP2667_PRECHARGE_MV     3000
#endif
#ifndef MP2667_RECHARGE_MV
    #define MP2667_RECHARGE_MV      300
#endif
#ifndef MP2667_WATCHDOG_S
    #define MP2667_WATCHDOG_S       0
#endif
#ifndef MP2667_SAFETY_TIMER_H
    #define MP2667_SAFETY_TIMER_H   12
#endif
#ifndef MP2667_TMR2X_ENABLED
    #define MP2667_TMR2X_ENABLED    true
#endif
#ifndef MP2667_THERMAL_REG_C
    #define MP2667_THERMAL_REG_C    100
#endif
#ifndef MP2667_NTC_ENABLED
    #define MP2667_NTC_ENABLED      false
#endif

#define LOG_TAG         mp2667
#define LOG_LEVEL       MP2667_LOG

namespace mp2667_detail {

constexpr int INPUT_MA[]        = { 77, 118, 345, 470, 540, 635, 734, 993 };
constexpr int PRECHARGE_MV[]    = { 2800, 3000 };
constexpr int RECHARGE_MV[]     = { 150, 300 };
constexpr int WATCHDOG_S[]      = { 0, 40, 80, 160 };
constexpr int SAFETY_TIMER_H[]  = { 20, 5, 8, 12 };
constexpr int THERMAL_REG_C[]   = { 60, 80, 100, 120 };

constexpr bool valid_step(
    int value,
    int min,
    int max,
    int step
) noexcept {
    return value >= min && value <= max && ((value - min) % step) == 0;
}

constexpr lite::u8 step_code(int value, int min, int step) noexcept {
    return static_cast<lite::u8>((value - min) / step);
}

template <std::size_t N>
constexpr lite::u8 table_code(const int (&values)[N], int value) noexcept {
    return static_cast<lite::u8>(lite::array_index_of(values, value));
}

} // namespace mp2667_detail

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
            ERR_WRITE_CFG,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:     return "probe failed";
                case ERR_READ_INFO: return "read info failed";
                case ERR_WRITE_CFG: return "write config failed";
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
                case ERR_READ_STATUS:   return "read status failed";
                case ERR_READ_FAULT:    return "read fault failed";
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

    bool enter_shipping_mode() {
        if (!device_status_) {
            return false;
        }

        const u8 value = static_cast<u8>(misc_operation_value() | MISC_FET_DIS);
        return write_reg_u8(REG_MISC_OPERATION, value);
    }

    template <std::size_t N>
    const char* fmt_device_info(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf("mp2667 rev %u", revision_);
        return buffer;
    }

    template <std::size_t N>
    const char* fmt_last_read_details(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);

        if (last_read_status_.code == ReadStatus::ERR_NOT_INIT) {
            return buffer;
        }

        if (last_read_status_.code == ReadStatus::ERR_READ_STATUS) {
            return buffer;
        }

        if (last_read_status_.code == ReadStatus::ERR_READ_FAULT) {
            text.appendf("status 0x%02X", last_status_);
            return buffer;
        }

        const auto state = decode_state(last_status_, last_fault_);

        append_charge_detail(text, last_status_, state);
        append_status_details(text, last_status_);
        append_fault_details(text, last_status_, last_fault_);

        return buffer;
    }

//------------------------------------------------------------------------------
private:
    static constexpr u8 I2C_ADDR = MP2667_I2C_ADDR;

    static constexpr u8 REG_INPUT_SOURCE_CTRL  = 0x00;
    static constexpr u8 REG_POWER_ON_CFG       = 0x01;
    static constexpr u8 REG_CHARGE_CURRENT     = 0x02;
    static constexpr u8 REG_DISCHARGE_TERM     = 0x03;
    static constexpr u8 REG_CHARGE_VOLTAGE     = 0x04;
    static constexpr u8 REG_CHARGE_TIMER       = 0x05;
    static constexpr u8 REG_MISC_OPERATION     = 0x06;
    static constexpr u8 REG_SYSTEM_STATUS       = 0x07;
    static constexpr u8 REG_FAULT               = 0x08;

    static constexpr u8 STATUS_THERM_STAT       = 0x01;
    static constexpr u8 STATUS_PG_STAT          = 0x02;
    static constexpr u8 STATUS_PPM_STAT         = 0x04;
    static constexpr u8 STATUS_CHG_STAT_SHIFT   = 3u;
    static constexpr u8 STATUS_CHG_STAT_MASK    = 0x18;
    static constexpr u8 STATUS_REV_SHIFT        = 5u;
    static constexpr u8 STATUS_REV_MASK         = 0x60;

    static constexpr u8 CHG_PRE_CHARGE          = 1;
    static constexpr u8 CHG_CHARGING            = 2;
    static constexpr u8 CHG_CHARGE_DONE         = 3;

    static constexpr u8 FAULT_SAFETY_TIMER      = 1u << 2u;
    static constexpr u8 FAULT_BATTERY           = 1u << 3u;
    static constexpr u8 FAULT_THERMAL_SHUTDOWN  = 1u << 4u;
    static constexpr u8 FAULT_INPUT             = 1u << 5u;
    static constexpr u8 FAULT_WATCHDOG          = 1u << 6u;

    static constexpr u8 MISC_FET_DIS            = 1u << 5u;

    static_assert(
        mp2667_detail::valid_step(MP2667_INPUT_MIN_MV, 3880, 5080, 80),
        "MP2667_INPUT_MIN_MV must be 3880..5080 in 80mV steps"
    );
    static_assert(
        lite::array_contains(mp2667_detail::INPUT_MA, MP2667_INPUT_MA),
        "MP2667_INPUT_MA must be 77,118,345,470,540,635,734,993"
    );
    static_assert(
        mp2667_detail::valid_step(MP2667_CHARGE_MA, 26, 1049, 33),
        "MP2667_CHARGE_MA must be 26..1049 in 33mA steps"
    );
    static_assert(
        mp2667_detail::valid_step(MP2667_TERM_MA, 24, 108, 28),
        "MP2667_TERM_MA must be 24..108 in 28mA steps"
    );
    static_assert(
        mp2667_detail::valid_step(
            MP2667_DISCHARGE_MA,
            100,
            1600,
            100
        ),
        "MP2667_DISCHARGE_MA must be 100..1600 in 100mA steps"
    );
    static_assert(
        mp2667_detail::valid_step(MP2667_BATTERY_REG_MV, 3600, 4545, 15),
        "MP2667_BATTERY_REG_MV must be 3600..4545 in 15mV steps"
    );
    static_assert(
        mp2667_detail::valid_step(MP2667_BATTERY_UVLO_MV, 2400, 3100, 100),
        "MP2667_BATTERY_UVLO_MV must be 2400..3100 in 100mV steps"
    );
    static_assert(
        lite::array_contains(mp2667_detail::PRECHARGE_MV, MP2667_PRECHARGE_MV),
        "MP2667_PRECHARGE_MV must be 2800 or 3000"
    );
    static_assert(
        lite::array_contains(mp2667_detail::RECHARGE_MV, MP2667_RECHARGE_MV),
        "MP2667_RECHARGE_MV must be 150 or 300"
    );
    static_assert(
        lite::array_contains(mp2667_detail::WATCHDOG_S, MP2667_WATCHDOG_S),
        "MP2667_WATCHDOG_S must be 0, 40, 80, or 160"
    );
    static_assert(
        lite::array_contains(
            mp2667_detail::SAFETY_TIMER_H,
            MP2667_SAFETY_TIMER_H
        ),
        "MP2667_SAFETY_TIMER_H must be 5, 8, 12, or 20"
    );
    static_assert(
        MP2667_TMR2X_ENABLED == true || MP2667_TMR2X_ENABLED == false,
        "MP2667_TMR2X_ENABLED must be true or false"
    );
    static_assert(
        lite::array_contains(
            mp2667_detail::THERMAL_REG_C,
            MP2667_THERMAL_REG_C
        ),
        "MP2667_THERMAL_REG_C must be 60, 80, 100, or 120"
    );
    static_assert(
        MP2667_NTC_ENABLED == true || MP2667_NTC_ENABLED == false,
        "MP2667_NTC_ENABLED must be true or false"
    );

    lite::Twi&      twi_;
    DeviceStatus    device_status_;
    ReadStatus      last_read_status_ = { ReadStatus::ERR_NOT_INIT };

    u8 revision_    = 0;
    u8 last_status_ = 0;
    u8 last_fault_  = 0;

    DeviceStatus init() {
        if (twi_.probe(I2C_ADDR).is_error()) {
            return { DeviceStatus::ERR_PROBE };
        }

        u8 status = 0;
        if (!read_reg_u8(REG_SYSTEM_STATUS, status)) {
            return { DeviceStatus::ERR_READ_INFO };
        }

        revision_ = decode_revision(status);

        if (!configure_charger()) {
            return { DeviceStatus::ERR_WRITE_CFG };
        }

        return { DeviceStatus::OK };
    }

    bool configure_charger() {
        return write_reg_u8(REG_CHARGE_TIMER, charge_timer_value())
            && write_reg_u8(REG_MISC_OPERATION, misc_operation_value())
            && write_reg_u8(REG_INPUT_SOURCE_CTRL, input_source_value())
            && write_reg_u8(REG_POWER_ON_CFG, power_on_cfg_value())
            && write_reg_u8(REG_CHARGE_CURRENT, charge_current_value())
            && write_reg_u8(REG_DISCHARGE_TERM, discharge_term_value())
            && write_reg_u8(REG_CHARGE_VOLTAGE, charge_voltage_value());
    }

    bool write_reg_u8(u8 reg, u8 value) {
        return twi_.write(I2C_ADDR, reg, value).is_ok();
    }

    bool read_reg_u8(u8 reg, u8& value) {
        return twi_.write_read(I2C_ADDR, reg, value).is_ok();
    }

    static constexpr u8 input_source_value() noexcept {
        return static_cast<u8>(
              (mp2667_detail::step_code(MP2667_INPUT_MIN_MV, 3880, 80) << 3u)
            | mp2667_detail::table_code(
                mp2667_detail::INPUT_MA,
                MP2667_INPUT_MA
            )
        );
    }

    static constexpr u8 power_on_cfg_value() noexcept {
        return mp2667_detail::step_code(MP2667_BATTERY_UVLO_MV, 2400, 100);
    }

    static constexpr u8 charge_current_value() noexcept {
        return mp2667_detail::step_code(MP2667_CHARGE_MA, 26, 33);
    }

    static constexpr u8 discharge_term_value() noexcept {
        return static_cast<u8>(
              (mp2667_detail::step_code(
                    MP2667_DISCHARGE_MA,
                    100,
                    100
                ) << 3u)
            | mp2667_detail::step_code(MP2667_TERM_MA, 24, 28)
        );
    }

    static constexpr u8 charge_voltage_value() noexcept {
        const u8 precharge = MP2667_PRECHARGE_MV == 3000 ? 1u << 1u : 0;
        const u8 recharge = MP2667_RECHARGE_MV == 300 ? 1u : 0;

        return static_cast<u8>(
              (mp2667_detail::step_code(
                    MP2667_BATTERY_REG_MV,
                    3600,
                    15
                ) << 2u)
            | precharge
            | recharge
        );
    }

    static constexpr u8 charge_timer_value() noexcept {
        return static_cast<u8>(
              (1u << 6u)
            | (mp2667_detail::table_code(
                mp2667_detail::WATCHDOG_S,
                MP2667_WATCHDOG_S
            ) << 4u)
            | (1u << 3u)
            | (mp2667_detail::table_code(
                mp2667_detail::SAFETY_TIMER_H,
                MP2667_SAFETY_TIMER_H
            ) << 1u)
        );
    }

    static constexpr u8 misc_operation_value() noexcept {
        const u8 tmr2x = MP2667_TMR2X_ENABLED ? 1u << 6u : 0;
        const u8 ntc = MP2667_NTC_ENABLED ? 1u << 3u : 0;

        return static_cast<u8>(
              tmr2x
            | ntc
            | mp2667_detail::table_code(
                mp2667_detail::THERMAL_REG_C,
                MP2667_THERMAL_REG_C
            )
        );
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