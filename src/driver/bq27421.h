//  manage bq27421 battery gauge
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/status.h"
#include "lite/core/text_buffer.h"
#include "lite/core/types.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"

//==============================================================================
//  config values and defaults
//------------------------------------------------------------------------------
#ifndef BQ27421_I2C_ADDR
    #define BQ27421_I2C_ADDR        0x55
#endif
#ifndef BQ27421_LOG
    #define BQ27421_LOG             none
#endif
#ifndef BQ27421_DESIGN_CAP_MAH
    #define BQ27421_DESIGN_CAP_MAH  500
#endif
#ifndef BQ27421_TERM_VOLTAGE_MV
    #define BQ27421_TERM_VOLTAGE_MV 3000
#endif
#ifndef BQ27421_TAPER_MA
    #define BQ27421_TAPER_MA        50
#endif
#ifndef BQ27421_CHARGE_DETECT_MA
    #define BQ27421_CHARGE_DETECT_MA 5
#endif

#define LOG_TAG         bq27421
#define LOG_LEVEL       BQ27421_LOG

//==============================================================================
class Bq27421 {

using u8 = lite::u8;
using u16 = lite::u16;
using s16 = lite::s16;

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
            ERR_READ_FLAGS,
            ERR_READ_VOLTAGE,
            ERR_READ_SOC,
            ERR_READ_CURRENT,
            ERR_READ_REMAINING_CAP,
            ERR_READ_FULL_CHARGE_CAP,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:     return "not initialized";
                case ERR_READ_FLAGS:   return "read flags failed";
                case ERR_READ_VOLTAGE: return "read voltage failed";
                case ERR_READ_SOC:     return "read soc failed";
                case ERR_READ_CURRENT: return "read current failed";
                case ERR_READ_REMAINING_CAP:
                    return "read remaining capacity failed";
                case ERR_READ_FULL_CHARGE_CAP:
                    return "read full charge capacity failed";
                default:               return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct State {
        u16 flags = 0;
        u16 voltage_mv = 0;
        u8 soc_percent = 0;
        s16 average_current_ma = 0;
        u16 remaining_capacity_mah = 0;
        u16 full_charge_capacity_mah = 0;

        [[nodiscard]] constexpr bool charging() const noexcept {
            return average_current_ma > BQ27421_CHARGE_DETECT_MA;
        }

        [[nodiscard]] constexpr bool discharging() const noexcept {
            return average_current_ma < -BQ27421_CHARGE_DETECT_MA;
        }

        [[nodiscard]] constexpr bool voltage_plausible() const noexcept {
            return voltage_mv >= 2450u && voltage_mv <= 4500u;
        }

        [[nodiscard]] constexpr bool capacity_plausible() const noexcept {
            return full_charge_capacity_mah >= BQ27421_DESIGN_CAP_MAH / 2u
                && full_charge_capacity_mah <= BQ27421_DESIGN_CAP_MAH * 2u;
        }

        [[nodiscard]] constexpr bool remaining_capacity_plausible() const noexcept {
            return capacity_plausible()
                && remaining_capacity_mah <= full_charge_capacity_mah;
        }

        [[nodiscard]] constexpr u8 quality_percent() const noexcept {
            if (!voltage_plausible()) {
                return 0;
            }

            u8 quality = 50;

            if (capacity_plausible()) {
                quality = static_cast<u8>(quality + 25u);
            }
            if (remaining_capacity_plausible()) {
                quality = static_cast<u8>(quality + 15u);
            }

            return quality;
        }
    };

    //--------------------------------------------------------------------------
    struct Result {
        ReadStatus read_state;
        State state;

        constexpr explicit operator bool() const noexcept {
            return read_state.is_ok();
        }
    };

    //--------------------------------------------------------------------------
    explicit Bq27421(lite::Twi& twi)
        : twi_(twi)
    {
        device_status_ = init();
    }

    DeviceStatus status() const noexcept {
        return device_status_;
    }

    constexpr explicit operator bool() const noexcept {
        return device_status_.is_ok();
    }

    //--------------------------------------------------------------------------
    Result read_status() {
        if (!device_status_) {
            last_read_status_ = { ReadStatus::ERR_NOT_INIT };
            return { .read_state = last_read_status_ };
        }

        State state = {};

        if (!read_word(CMD_FLAGS, state.flags)) {
            last_read_status_ = { ReadStatus::ERR_READ_FLAGS };
            return { .read_state = last_read_status_ };
        }

        if (!read_word(CMD_VOLTAGE, state.voltage_mv)) {
            last_read_status_ = { ReadStatus::ERR_READ_VOLTAGE };
            return { .read_state = last_read_status_ };
        }

        u16 soc_percent = 0;
        if (!read_word(CMD_SOC, soc_percent)) {
            last_read_status_ = { ReadStatus::ERR_READ_SOC };
            return { .read_state = last_read_status_ };
        }
        state.soc_percent = static_cast<u8>(soc_percent > 100 ? 100 : soc_percent);

        if (!read_s16(CMD_AVERAGE_CURRENT, state.average_current_ma)) {
            last_read_status_ = { ReadStatus::ERR_READ_CURRENT };
            return { .read_state = last_read_status_ };
        }

        if (!read_word(CMD_REMAINING_CAPACITY, state.remaining_capacity_mah)) {
            last_read_status_ = { ReadStatus::ERR_READ_REMAINING_CAP };
            return { .read_state = last_read_status_ };
        }

        if (!read_word(CMD_FULL_CHARGE_CAPACITY, state.full_charge_capacity_mah)) {
            last_read_status_ = { ReadStatus::ERR_READ_FULL_CHARGE_CAP };
            return { .read_state = last_read_status_ };
        }

        last_read_status_ = { ReadStatus::OK };
        last_state_ = state;

        return {
            .read_state = last_read_status_,
            .state = state,
        };
    }

    //--------------------------------------------------------------------------
    bool enter_hibernate() {
        if (!device_status_) {
            return false;
        }
        return write_control(CTRL_SET_HIBERNATE);
    }

    bool enter_shutdown() {
        if (!device_status_) {
            return false;
        }
        return write_control(CTRL_SHUTDOWN_ENABLE) && write_control(CTRL_SHUTDOWN);
    }

    //--------------------------------------------------------------------------
    template <std::size_t N>
    const char* fmt_device_info(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);
        text.appendf(
            "bq27421 dev=0x%04X fw=0x%04X chem=0x%04X cap=%umAh",
            device_type_,
            firmware_version_,
            chem_id_,
            static_cast<unsigned>(BQ27421_DESIGN_CAP_MAH)
        );
        return buffer;
    }

    template <std::size_t N>
    const char* fmt_last_read_details(char (&buffer)[N]) const {
        lite::TextBuffer text(buffer);

        if (last_read_status_.is_error()) {
            text.append(last_read_status_.str());
            return buffer;
        }

        text.appendf(
            "%umv soc=%u%% quality=%u%% rem=%umAh fcc=%umAh avg=%dmA flags=0x%04X",
            static_cast<unsigned>(last_state_.voltage_mv),
            static_cast<unsigned>(last_state_.soc_percent),
            static_cast<unsigned>(last_state_.quality_percent()),
            static_cast<unsigned>(last_state_.remaining_capacity_mah),
            static_cast<unsigned>(last_state_.full_charge_capacity_mah),
            static_cast<int>(last_state_.average_current_ma),
            static_cast<unsigned>(last_state_.flags)
        );
        return buffer;
    }

//------------------------------------------------------------------------------
private:
    static constexpr u8 I2C_ADDR = BQ27421_I2C_ADDR;

    static constexpr u8 CMD_CONTROL         = 0x00;
    static constexpr u8 CMD_VOLTAGE         = 0x04;
    static constexpr u8 CMD_FLAGS           = 0x06;
    static constexpr u8 CMD_REMAINING_CAPACITY = 0x0C;
    static constexpr u8 CMD_FULL_CHARGE_CAPACITY = 0x0E;
    static constexpr u8 CMD_AVERAGE_CURRENT = 0x10;
    static constexpr u8 CMD_SOC             = 0x1C;

    static constexpr u16 CTRL_DEVICE_TYPE      = 0x0001;
    static constexpr u16 CTRL_FW_VERSION       = 0x0002;
    static constexpr u16 CTRL_CHEM_ID          = 0x0008;
    static constexpr u16 CTRL_SET_HIBERNATE    = 0x0011;
    static constexpr u16 CTRL_SHUTDOWN_ENABLE  = 0x001B;
    static constexpr u16 CTRL_SHUTDOWN         = 0x001C;

    static_assert(
        BQ27421_DESIGN_CAP_MAH >= 250 && BQ27421_DESIGN_CAP_MAH <= 750,
        "BQ27421_DESIGN_CAP_MAH must be 250..750"
    );
    static_assert(
        BQ27421_TERM_VOLTAGE_MV >= 2500 && BQ27421_TERM_VOLTAGE_MV <= 3700,
        "BQ27421_TERM_VOLTAGE_MV must be 2500..3700"
    );
    static_assert(
        BQ27421_TAPER_MA > 0 && BQ27421_TAPER_MA <= BQ27421_DESIGN_CAP_MAH,
        "BQ27421_TAPER_MA must be 1..BQ27421_DESIGN_CAP_MAH"
    );
    static_assert(
        BQ27421_CHARGE_DETECT_MA >= 0,
        "BQ27421_CHARGE_DETECT_MA must be non-negative"
    );

    lite::Twi&      twi_;
    DeviceStatus    device_status_;
    ReadStatus      last_read_status_ = { ReadStatus::ERR_NOT_INIT };
    State           last_state_;
    u16             device_type_ = 0;
    u16             firmware_version_ = 0;
    u16             chem_id_ = 0;

    DeviceStatus init() {
        if (twi_.probe(I2C_ADDR).is_error()) {
            return { DeviceStatus::ERR_PROBE };
        }

        if (
               !read_control(CTRL_DEVICE_TYPE, device_type_)
            || !read_control(CTRL_FW_VERSION, firmware_version_)
            || !read_control(CTRL_CHEM_ID, chem_id_)
        ) {
            return { DeviceStatus::ERR_READ_INFO };
        }

        return { DeviceStatus::OK };
    }

    bool read_control(u16 subcommand, u16& value) {
        return write_control(subcommand)
            && read_word(CMD_CONTROL, value);
    }

    bool write_control(u16 subcommand) {
        const u8 data[] = {
            CMD_CONTROL,
            static_cast<u8>(subcommand & 0xFFu),
            static_cast<u8>(subcommand >> 8u),
        };
        return twi_.write(I2C_ADDR, data, sizeof(data)).is_ok();
    }

    bool read_word(u8 command, u16& value) {
        u8 data[2] = {};
        if (twi_.write_read(I2C_ADDR, &command, sizeof(command), data, sizeof(data)).is_error()) {
            return false;
        }

        value = static_cast<u16>(data[0])
            | static_cast<u16>(static_cast<u16>(data[1]) << 8u);
        return true;
    }

    bool read_s16(u8 command, s16& value) {
        u16 raw = 0;
        if (!read_word(command, raw)) {
            return false;
        }

        value = static_cast<s16>(raw);
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL