//  manage accelerometer sensor bma253
//
//  see LICENSE file for terms

#pragma once
#include "status.h"

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
    struct DeviceStatus : public Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_ID_READ,
            ERR_ID_VALUE,
            ERR_CFG_WRITE
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:         return "probe failed";
                case ERR_ID_READ:       return "id read failed";
                case ERR_ID_VALUE:      return "id mismatch";
                case ERR_CFG_WRITE:     return "config write failed";
                default:                return Status::str();
            }
        }
    };

    struct ReadStatus : public Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_DATA_READ
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:      return "not initialized";
                case ERR_DATA_READ:     return "data read failed";
                default:                return Status::str();
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
        device_status_ = init_();
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

        if (twi_.write_read(k_i2c_addr_, k_reg_acc_x_lsb_, raw).is_error()) {
            return { .read_state = { ReadStatus::ERR_DATA_READ } };
        }

        return {
            .read_state = { ReadStatus::OK },
            .x = decode_axis_(raw.x),
            .y = decode_axis_(raw.y),
            .z = decode_axis_(raw.z),
            .celsius10 = decode_temp_c10_(raw.temp),
        };
    }

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    DeviceStatus device_status_;

    static constexpr u8 k_i2c_addr_        = 0x19;
    static constexpr u8 k_reg_chip_id_     = 0x00;
    static constexpr u8 k_reg_acc_x_lsb_   = 0x02;
    static constexpr u8 k_reg_pmu_range_   = 0x0F;
    static constexpr u8 k_reg_pmu_bw_      = 0x10;

    static constexpr u8 k_chip_id_         = 0xFA;
    static constexpr u8 k_range_2g_        = 0x03;
    static constexpr u8 k_bw_62_hz_        = 0x0B;

    //--------------------------------------------------------------------------
    DeviceStatus init_() {
        //  check for device presence on the bus
        if ( twi_.probe(k_i2c_addr_).is_error() ) {
            return { DeviceStatus::ERR_PROBE };
        }

        //  check id register for expected value
        u8 id = 0;
        if (!read_reg_u8_(k_reg_chip_id_, id)) {
            return { DeviceStatus::ERR_ID_READ };
        }

        if (id != k_chip_id_) {
            return { DeviceStatus::ERR_ID_VALUE };
        }

        if (
            !write_reg_u8_(k_reg_pmu_range_, k_range_2g_)
         || !write_reg_u8_(k_reg_pmu_bw_, k_bw_62_hz_)
        ) {
            return { DeviceStatus::ERR_CFG_WRITE };
        }

        return { DeviceStatus::OK };
    }

    //--------------------------------------------------------------------------
    bool write_reg_u8_(u8 reg, u8 value) {
        return twi_.write(k_i2c_addr_, reg, value).is_ok();
    }

    bool read_reg_u8_(u8 reg, u8& value) {
        return twi_.write_read(k_i2c_addr_, reg, value).is_ok();
    }

    static s16 decode_axis_(u16 raw_word) {
        const u16 raw10 = static_cast<u16>((raw_word >> 6u) & 0x03FFu);
        if ((raw10 & 0x0200u) != 0u) {
            return static_cast<s16>(raw10 | 0xFC00u);
        }
        return static_cast<s16>(raw10);
    }

    //  bma253 temperature register: 0 -> 24.0 deg C, 1 lsb -> 0.5 deg C
    static s16 decode_temp_c10_(u8 raw_temp) {
        const s16 raw_signed = static_cast<s8>(raw_temp);
        const s16 temp_x2 = static_cast<s16>(48 + raw_signed);
        return static_cast<s16>(temp_x2 * 5);
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
