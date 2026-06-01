//  manage bq27421 battery gauge
//
//  see LICENSE file for terms

#pragma once

#include <Arduino.h>

#include "lite/core/fmt.h"
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
#ifndef BQ27421_HIBERNATE_CURRENT_MA
    #define BQ27421_HIBERNATE_CURRENT_MA 3
#endif
#ifndef BQ27421_I2C_PACKET_WAIT_US
    #define BQ27421_I2C_PACKET_WAIT_US 70
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
            ERR_CONFIGURE,
            ERR_CLEAR_HIBERNATE,
            ERR_WAIT_INIT_COMPLETE,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:         return "probe failed";
                case ERR_READ_INFO:     return "read info failed";
                case ERR_CONFIGURE:     return "configure failed";
                case ERR_CLEAR_HIBERNATE: return "clear hibernate failed";
                case ERR_WAIT_INIT_COMPLETE:
                    return "init complete timeout";
                default:                return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct UpdateStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_READ_SOC,
            ERR_READ_CURRENT,
            ERR_READ_VOLTAGE,
            ERR_READ_FULL_CHARGE_CAP,
            ERR_ARM_HIBERNATE,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:     return "not initialized";
                case ERR_READ_SOC:     return "read soc failed";
                case ERR_READ_CURRENT: return "read current failed";
                case ERR_READ_VOLTAGE: return "read voltage failed";
                case ERR_READ_FULL_CHARGE_CAP:
                    return "read full charge capacity failed";
                case ERR_ARM_HIBERNATE: return "arm hibernate failed";
                default:               return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct ReadStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_NOT_READY,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:  return "not initialized";
                case ERR_NOT_READY: return "not ready";
                default:            return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct DetailsStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_NOT_READY,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:  return "not initialized";
                case ERR_NOT_READY: return "not ready";
                default:            return lite::Status::str();
            }
        }
    };

    //--------------------------------------------------------------------------
    struct State {
        u8 soc_percent = 0;
        s16 average_current_ma = 0;

        [[nodiscard]] constexpr bool charging() const noexcept {
            return average_current_ma > BQ27421_CHARGE_DETECT_MA;
        }

        [[nodiscard]] constexpr bool discharging() const noexcept {
            return average_current_ma < -BQ27421_CHARGE_DETECT_MA;
        }
    };

    //--------------------------------------------------------------------------
    struct Details : public lite::WithFmtArray<Details> {
        using lite::WithFmtArray<Details>::fmt;

        DetailsStatus read_state;
        u16 voltage_mv = 0;
        u16 full_charge_capacity_mah = 0;

        constexpr explicit operator bool() const noexcept {
            return read_state.is_ok();
        }

        constexpr bool operator==(const Details& other) const noexcept {
            return read_state.code == other.read_state.code
                && voltage_mv == other.voltage_mv
                && full_charge_capacity_mah == other.full_charge_capacity_mah;
        }

        const char* fmt(char* buffer, std::size_t size) const {
            lite::TextBuffer text(buffer, size);

            if (read_state.is_error()) {
                text.append(read_state.str());
                return buffer;
            }

            text.appendf(
                "%umv %umAh",
                static_cast<unsigned>(voltage_mv),
                static_cast<unsigned>(full_charge_capacity_mah)
            );
            return buffer;
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
    UpdateStatus update() {
        if (!device_status_) {
            return { UpdateStatus::ERR_NOT_INIT };
        }

        const auto read_state = update_cache();
        advance_update_step();

        if (!request_hibernate()) {
            return { UpdateStatus::ERR_ARM_HIBERNATE };
        }

        return read_state;
    }

    Result read_status() const {
        if (!device_status_) {
            return { .read_state = { ReadStatus::ERR_NOT_INIT } };
        }

        if (!cache_has(CACHE_SOC | CACHE_CURRENT)) {
            return { .read_state = { ReadStatus::ERR_NOT_READY } };
        }

        return {
            .read_state = { ReadStatus::OK },
            .state = state_,
        };
    }

    Details read_details() const {
        Details details = details_;

        if (!device_status_) {
            details.read_state = { DetailsStatus::ERR_NOT_INIT };
            return details;
        }

        if (!cache_has(CACHE_VOLTAGE | CACHE_FULL_CHARGE_CAP)) {
            details.read_state = { DetailsStatus::ERR_NOT_READY };
            return details;
        }

        details.read_state = { DetailsStatus::OK };
        return details;
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

//------------------------------------------------------------------------------
private:
    static constexpr u8 I2C_ADDR = BQ27421_I2C_ADDR;

    static constexpr u8 CMD_CONTROL         = 0x00;
    static constexpr u8 CMD_VOLTAGE         = 0x04;
    static constexpr u8 CMD_FLAGS           = 0x06;
    static constexpr u8 CMD_FULL_CHARGE_CAPACITY = 0x0E;
    static constexpr u8 CMD_AVERAGE_CURRENT = 0x10;
    static constexpr u8 CMD_SOC             = 0x1C;
    static constexpr u8 CMD_DATA_CLASS      = 0x3E;
    static constexpr u8 CMD_DATA_BLOCK      = 0x3F;
    static constexpr u8 CMD_BLOCK_DATA      = 0x40;
    static constexpr u8 CMD_BLOCK_DATA_CHECKSUM = 0x60;
    static constexpr u8 CMD_BLOCK_DATA_CONTROL  = 0x61;

    static constexpr u16 FLAG_CFGUPMODE      = 1u << 4u;

    static constexpr u16 CTRL_STATUS_INITCOMP  = 1u << 7u;

    static constexpr u16 CTRL_CONTROL_STATUS   = 0x0000;
    static constexpr u16 CTRL_DEVICE_TYPE      = 0x0001;
    static constexpr u16 CTRL_FW_VERSION       = 0x0002;
    static constexpr u16 CTRL_CHEM_ID          = 0x0008;
    static constexpr u16 CTRL_SET_CFGUPDATE    = 0x0013;
    static constexpr u16 CTRL_SET_HIBERNATE    = 0x0011;
    static constexpr u16 CTRL_CLEAR_HIBERNATE  = 0x0012;
    static constexpr u16 CTRL_SOFT_RESET       = 0x0042;

    static constexpr u16 CFGUPDATE_TIMEOUT_MS  = 1000;
    static constexpr u16 INIT_COMPLETE_TIMEOUT_MS = 10000;

    static constexpr u8 SUBCLASS_POWER         = 68;
    static constexpr u8 SUBCLASS_IT_CFG        = 80;

    static constexpr u8 POWER_HIBERNATE_I      = 7;
    static constexpr u8 POWER_HIBERNATE_V      = 9;

    static constexpr u8 IT_CFG_FH_SETTING_0    = 11;
    static constexpr u8 IT_CFG_FH_SETTING_1    = 24;
    static constexpr u8 IT_CFG_FH_SETTING_2    = 26;
    static constexpr u8 IT_CFG_FH_SETTING_3    = 33;

    static constexpr u16 NORMAL_HIBERNATE_V_MV = 2200;
    static constexpr u16 NORMAL_FH_SETTING_0   = 60;
    static constexpr u16 NORMAL_FH_SETTING_1   = 100;
    static constexpr u16 NORMAL_FH_SETTING_2   = 18000;
    static constexpr u8 NORMAL_FH_SETTING_3    = 25;

    static constexpr u8 DATA_BLOCK_SIZE        = 32;

    struct DataPatch {
        u8 offset = 0;
        u8 value = 0;
    };

    enum CacheFlag : u8 {
        CACHE_SOC             = 1u << 0u,
        CACHE_CURRENT         = 1u << 1u,
        CACHE_VOLTAGE         = 1u << 2u,
        CACHE_FULL_CHARGE_CAP = 1u << 3u,
    };

    enum UpdateStep : u8 {
        UPDATE_SOC = 0,
        UPDATE_CURRENT,
        UPDATE_VOLTAGE,
        UPDATE_FULL_CHARGE_CAP,
        UPDATE_COUNT,
    };

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
    static_assert(
        BQ27421_HIBERNATE_CURRENT_MA >= 0
            && BQ27421_HIBERNATE_CURRENT_MA <= 8000,
        "BQ27421_HIBERNATE_CURRENT_MA must be 0..8000"
    );
    static_assert(
        BQ27421_I2C_PACKET_WAIT_US >= 66,
        "BQ27421_I2C_PACKET_WAIT_US must be at least 66"
    );

    lite::Twi&      twi_;
    DeviceStatus    device_status_;
    u16             device_type_ = 0;
    u16             firmware_version_ = 0;
    u16             chem_id_ = 0;
    State           state_;
    Details         details_;
    u8              cache_flags_ = 0;
    u8              update_step_ = UPDATE_SOC;

    static constexpr u8 clamp_soc_percent(u16 soc_percent) noexcept {
        return static_cast<u8>(soc_percent > 100u ? 100u : soc_percent);
    }

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

        if (!configure_normal_hibernate()) {
            return { DeviceStatus::ERR_CONFIGURE };
        }

        if (!clear_hibernate()) {
            return { DeviceStatus::ERR_CLEAR_HIBERNATE };
        }

        if (!wait_init_complete()) {
            return { DeviceStatus::ERR_WAIT_INIT_COMPLETE };
        }

        return { DeviceStatus::OK };
    }

    bool cache_has(u8 flags) const noexcept {
        return (cache_flags_ & flags) == flags;
    }

    void set_cache_flag(u8 flag) noexcept {
        cache_flags_ = static_cast<u8>(cache_flags_ | flag);
    }

    void advance_update_step() noexcept {
        update_step_ = static_cast<u8>(update_step_ + 1u);
        if (update_step_ >= UPDATE_COUNT) {
            update_step_ = UPDATE_SOC;
        }
    }

    UpdateStatus update_cache() {
        switch (update_step_) {
            case UPDATE_SOC:             return update_soc();
            case UPDATE_CURRENT:         return update_current();
            case UPDATE_VOLTAGE:         return update_voltage();
            case UPDATE_FULL_CHARGE_CAP: return update_full_charge_cap();
            default:                     return { UpdateStatus::ERR_READ_SOC };
        }
    }

    UpdateStatus update_soc() {
        u16 soc_percent = 0;
        if (!read_word(CMD_SOC, soc_percent)) {
            return { UpdateStatus::ERR_READ_SOC };
        }

        state_.soc_percent = clamp_soc_percent(soc_percent);
        set_cache_flag(CACHE_SOC);
        return { UpdateStatus::OK };
    }

    UpdateStatus update_current() {
        if (!read_s16(CMD_AVERAGE_CURRENT, state_.average_current_ma)) {
            return { UpdateStatus::ERR_READ_CURRENT };
        }

        set_cache_flag(CACHE_CURRENT);
        return { UpdateStatus::OK };
    }

    UpdateStatus update_voltage() {
        if (!read_word(CMD_VOLTAGE, details_.voltage_mv)) {
            return { UpdateStatus::ERR_READ_VOLTAGE };
        }

        set_cache_flag(CACHE_VOLTAGE);
        return { UpdateStatus::OK };
    }

    UpdateStatus update_full_charge_cap() {
        if (!read_word(
            CMD_FULL_CHARGE_CAPACITY,
            details_.full_charge_capacity_mah
        )) {
            return { UpdateStatus::ERR_READ_FULL_CHARGE_CAP };
        }

        set_cache_flag(CACHE_FULL_CHARGE_CAP);
        return { UpdateStatus::OK };
    }

    bool read_control(u16 subcommand, u16& value) {
        return write_control(subcommand)
            && read_word(CMD_CONTROL, value);
    }

    bool request_hibernate() {
        return write_control(CTRL_SET_HIBERNATE);
    }

    bool clear_hibernate() {
        return write_control(CTRL_CLEAR_HIBERNATE);
    }

    bool wait_init_complete() {
        return wait_control_status(
            CTRL_STATUS_INITCOMP,
            true,
            INIT_COMPLETE_TIMEOUT_MS
        );
    }

    bool configure_normal_hibernate() {
        return configure_hibernate(
            BQ27421_HIBERNATE_CURRENT_MA,
            NORMAL_HIBERNATE_V_MV,
            NORMAL_FH_SETTING_0,
            NORMAL_FH_SETTING_1,
            NORMAL_FH_SETTING_2,
            NORMAL_FH_SETTING_3
        );
    }

    bool configure_hibernate(
        u16 hibernate_current_ma,
        u16 hibernate_voltage_mv,
        u16 fh_setting_0,
        u16 fh_setting_1,
        u16 fh_setting_2,
        u8 fh_setting_3
    ) {
        if (!enter_cfg_update()) {
            return false;
        }

        const bool configured = write_power_cfg(
                hibernate_current_ma,
                hibernate_voltage_mv
            )
            && write_it_cfg(
                fh_setting_0,
                fh_setting_1,
                fh_setting_2,
                fh_setting_3
            );
        const bool exited = exit_cfg_update();

        return configured && exited;
    }

    bool enter_cfg_update() {
        if (!write_control(CTRL_SET_CFGUPDATE)) {
            return false;
        }

        return wait_flags(FLAG_CFGUPMODE, true, CFGUPDATE_TIMEOUT_MS);
    }

    bool exit_cfg_update() {
        if (!write_control(CTRL_SOFT_RESET)) {
            return false;
        }

        return wait_flags(FLAG_CFGUPMODE, false, CFGUPDATE_TIMEOUT_MS);
    }

    bool write_power_cfg(u16 hibernate_current_ma, u16 hibernate_voltage_mv) {
        const DataPatch patches[] = {
            { POWER_HIBERNATE_I, hi(hibernate_current_ma) },
            {
                static_cast<u8>(POWER_HIBERNATE_I + 1u),
                lo(hibernate_current_ma)
            },
            { POWER_HIBERNATE_V, hi(hibernate_voltage_mv) },
            {
                static_cast<u8>(POWER_HIBERNATE_V + 1u),
                lo(hibernate_voltage_mv)
            },
        };

        return write_data_block(
            SUBCLASS_POWER,
            0,
            patches,
            static_cast<u8>(sizeof(patches) / sizeof(patches[0]))
        );
    }

    bool write_it_cfg(
        u16 fh_setting_0,
        u16 fh_setting_1,
        u16 fh_setting_2,
        u8 fh_setting_3
    ) {
        const DataPatch block_0_patches[] = {
            { IT_CFG_FH_SETTING_0, hi(fh_setting_0) },
            { static_cast<u8>(IT_CFG_FH_SETTING_0 + 1u), lo(fh_setting_0) },
            { IT_CFG_FH_SETTING_1, hi(fh_setting_1) },
            { static_cast<u8>(IT_CFG_FH_SETTING_1 + 1u), lo(fh_setting_1) },
            { IT_CFG_FH_SETTING_2, hi(fh_setting_2) },
            { static_cast<u8>(IT_CFG_FH_SETTING_2 + 1u), lo(fh_setting_2) },
        };
        const DataPatch block_1_patches[] = {
            {
                static_cast<u8>(IT_CFG_FH_SETTING_3 % DATA_BLOCK_SIZE),
                fh_setting_3
            },
        };

        return write_data_block(
                SUBCLASS_IT_CFG,
                0,
                block_0_patches,
                static_cast<u8>(
                    sizeof(block_0_patches) / sizeof(block_0_patches[0])
                )
            )
            && write_data_block(
                SUBCLASS_IT_CFG,
                1,
                block_1_patches,
                static_cast<u8>(
                    sizeof(block_1_patches) / sizeof(block_1_patches[0])
                )
            );
    }

    bool write_data_block(
        u8 subclass,
        u8 block_idx,
        const DataPatch* patches,
        u8 patch_count
    ) {
        u8 data[DATA_BLOCK_SIZE] = {};

        if (!select_data_block(subclass, block_idx)) {
            return false;
        }
        if (!read_data_block(data)) {
            return false;
        }

        for (u8 patch_idx = 0; patch_idx < patch_count; ++patch_idx) {
            const auto& patch = patches[patch_idx];
            if (patch.offset >= DATA_BLOCK_SIZE) {
                return false;
            }

            data[patch.offset] = patch.value;
            if (!write_reg_u8(
                static_cast<u8>(CMD_BLOCK_DATA + patch.offset),
                patch.value
            )) {
                return false;
            }
        }

        if (!write_reg_u8(CMD_BLOCK_DATA_CHECKSUM, checksum(data))) {
            return false;
        }

        ::delay(1);
        return verify_data_block(patches, patch_count);
    }

    bool select_data_block(u8 subclass, u8 block_idx) {
        if (
               !write_reg_u8(CMD_BLOCK_DATA_CONTROL, 0)
            || !write_reg_u8(CMD_DATA_CLASS, subclass)
            || !write_reg_u8(CMD_DATA_BLOCK, block_idx)
        ) {
            return false;
        }

        ::delay(1);
        return true;
    }

    bool read_data_block(u8 (&data)[DATA_BLOCK_SIZE]) {
        const u8 command = CMD_BLOCK_DATA;
        wait_i2c_packet();
        return twi_.write_read(
            I2C_ADDR,
            &command,
            sizeof(command),
            data,
            sizeof(data)
        ).is_ok();
    }

    bool verify_data_block(const DataPatch* patches, u8 patch_count) {
        u8 data[DATA_BLOCK_SIZE] = {};
        if (!read_data_block(data)) {
            return false;
        }

        for (u8 patch_idx = 0; patch_idx < patch_count; ++patch_idx) {
            const auto& patch = patches[patch_idx];
            if (data[patch.offset] != patch.value) {
                return false;
            }
        }

        return true;
    }

    bool wait_flags(u16 mask, bool expected, u16 timeout_ms) {
        const unsigned long deadline = ::millis() + timeout_ms;

        while (static_cast<long>(deadline - ::millis()) >= 0) {
            u16 flags = 0;
            if (!read_word(CMD_FLAGS, flags)) {
                ::delay(10);
                continue;
            }

            const bool is_set = (flags & mask) == mask;
            if (is_set == expected) {
                return true;
            }

            ::delay(10);
        }

        return false;
    }

    bool wait_control_status(u16 mask, bool expected, u16 timeout_ms) {
        const unsigned long deadline = ::millis() + timeout_ms;

        while (static_cast<long>(deadline - ::millis()) >= 0) {
            u16 control_status = 0;
            if (!read_control(CTRL_CONTROL_STATUS, control_status)) {
                ::delay(10);
                continue;
            }

            const bool is_set = (control_status & mask) == mask;
            if (is_set == expected) {
                return true;
            }

            ::delay(10);
        }

        return false;
    }

    bool write_control(u16 subcommand) {
        return write_reg_u8(
                CMD_CONTROL,
                static_cast<u8>(subcommand & 0xFFu)
            )
            && write_reg_u8(
                static_cast<u8>(CMD_CONTROL + 1u),
                static_cast<u8>(subcommand >> 8u)
            );
    }

    bool write_reg_u8(u8 command, u8 value) {
        wait_i2c_packet();
        return twi_.write(I2C_ADDR, command, value).is_ok();
    }

    bool read_word(u8 command, u16& value) {
        u8 data[2] = {};
        wait_i2c_packet();
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

    static void wait_i2c_packet() {
        //  bq27421 needs bus-free time between packets at 400 khz
        ::delayMicroseconds(BQ27421_I2C_PACKET_WAIT_US);
    }

    static constexpr u8 hi(u16 value) noexcept {
        return static_cast<u8>(value >> 8u);
    }

    static constexpr u8 lo(u16 value) noexcept {
        return static_cast<u8>(value & 0xFFu);
    }

    static u8 checksum(const u8 (&data)[DATA_BLOCK_SIZE]) noexcept {
        u16 sum = 0;
        for (u8 data_idx = 0; data_idx < DATA_BLOCK_SIZE; ++data_idx) {
            sum = static_cast<u16>(sum + data[data_idx]);
        }
        return static_cast<u8>(0xFFu - (sum & 0xFFu));
    }
};

#undef LOG_TAG
#undef LOG_LEVEL