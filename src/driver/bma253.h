//  manage accelerometer sensor bma253
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/status.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"

#define LOG_TAG         bma253
#define LOG_LEVEL       BMA253_LOG

//==============================================================================
class Bma253 {

using s16   = lite::s16;
using s8    = lite::s8;
using lh16  = lite::lh16;
    
//------------------------------------------------------------------------------    
public:
    struct DeviceStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_ID_READ,
            ERR_ID_VALUE,
            ERR_WRITE_CFG
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:         return "probe failed";
                case ERR_ID_READ:       return "read id failed";
                case ERR_ID_VALUE:      return "id mismatch";
                case ERR_WRITE_CFG:     return "write config failed";
                default:                return lite::Status::str();
            }
        }
    };

    struct ReadStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_READ_DATA
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:      return "not initialized";
                case ERR_READ_DATA:     return "read data failed";
                default:                return lite::Status::str();
            }
        }
    };

    struct Result {
        ReadStatus  read_state;
        s16         x, y, z;
        s16         celsius10;

        operator bool() const noexcept {
            return read_state.is_ok();
        }
    };

    explicit Bma253(lite::Twi& twi)
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

    Result read_data() {
        if ( !device_status_ ) {
            return { .read_state = { ReadStatus::ERR_NOT_INIT } };
        }

        struct RawData {
            lh16    x;
            lh16    y;
            lh16    z;
            u8      temp;
        } raw = {};

        if (twi_.write_read(I2C_ADDR, REG_ACC_X_LSB, raw).is_error()) {
            return { .read_state = { ReadStatus::ERR_READ_DATA } };
        }

        return {
            .read_state = { ReadStatus::OK },
            .x = decode_axis(raw.x),
            .y = decode_axis(raw.y),
            .z = decode_axis(raw.z),
            .celsius10 = decode_temp_c10(raw.temp),
        };
    }

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    DeviceStatus device_status_;

    static constexpr u8 I2C_ADDR        = 0x19;
    static constexpr u8 REG_CHIP_ID     = 0x00;
    static constexpr u8 REG_ACC_X_LSB   = 0x02;
    static constexpr u8 REG_PMU_RANGE   = 0x0F;
    static constexpr u8 REG_PMU_BW      = 0x10;

    static constexpr u8 CHIP_ID         = 0xFA;
    static constexpr u8 RANGE_2G        = 0x03;
    static constexpr u8 BW_62_HZ        = 0x0B;

    //--------------------------------------------------------------------------
    DeviceStatus init() {
        //  check for device presence on the bus
        if ( twi_.probe(I2C_ADDR).is_error() ) {
            return { DeviceStatus::ERR_PROBE };
        }

        //  check id register for expected value
        u8 id = 0;
        if (!read_reg_u8(REG_CHIP_ID, id)) {
            return { DeviceStatus::ERR_ID_READ };
        }

        if (id != CHIP_ID) {
            return { DeviceStatus::ERR_ID_VALUE };
        }

        if (
                !write_reg_u8(REG_PMU_RANGE, RANGE_2G)
            || !write_reg_u8(REG_PMU_BW, BW_62_HZ)
        ) {
            return { DeviceStatus::ERR_WRITE_CFG };
        }

        return { DeviceStatus::OK };
    }

    //--------------------------------------------------------------------------
    bool write_reg_u8(u8 reg, u8 value) {
        return twi_.write(I2C_ADDR, reg, value).is_ok();
    }

    bool read_reg_u8(u8 reg, u8& value) {
        return twi_.write_read(I2C_ADDR, reg, value).is_ok();
    }

    static s16 decode_axis(u16 raw_word) {
        const u16 raw10 = static_cast<u16>((raw_word >> 6u) & 0x03FFu);
        if ((raw10 & 0x0200u) != 0u) {
            return static_cast<s16>(raw10 | 0xFC00u);
        }
        return static_cast<s16>(raw10);
    }

    //  bma253 temperature register: 0 -> 24.0 deg C, 1 lsb -> 0.5 deg C
    static s16 decode_temp_c10(u8 raw_temp) {
        const s16 raw_signed = static_cast<s8>(raw_temp);
        const s16 temp_x2 = static_cast<s16>(48 + raw_signed);
        return static_cast<s16>(temp_x2 * 5);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
