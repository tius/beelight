//  manage light sensor tcs34725
//
//  see LICENSE file for terms

#pragma once
#include "status.h"

#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"

#define LOG_TAG         tcs34725
#define LOG_LEVEL       TCS34725_LOG

//==============================================================================
class Tcs34725 {
//------------------------------------------------------------------------------    
public:
    struct DeviceStatus : public Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_ID_READ,
            ERR_ID_VALUE,
            ERR_CFG_WRITE,
            ERR_ENABLE_WRITE
        };
        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:         return "probe failed";
                case ERR_ID_READ:       return "id read failed";
                case ERR_ID_VALUE:      return "id mismatch";
                case ERR_CFG_WRITE:     return "config write failed";
                case ERR_ENABLE_WRITE:  return "enable write failed";
                default:                return Status::str();
            }
        }
    };

    struct ReadStatus : public Status {
        enum : u8 {
            OK = 0,
            NOT_READY,
            ERR_NOT_INIT,
            ERR_STATUS_READ,
            ERR_DATA_READ
        };
        const char* str() const noexcept {
            switch (code) {
                case NOT_READY:         return "not ready";
                case ERR_NOT_INIT:      return "not initialized";
                case ERR_STATUS_READ:   return "status read failed";
                case ERR_DATA_READ:     return "data read failed";
                default:                return Status::str();
            }
        }
    };

    struct Result {
        ReadStatus read_state;
        u16     y, r, g, b;

        operator bool() const noexcept {
            return read_state.is_ok();
        }
    };

    //--------------------------------------------------------------------------
    explicit Tcs34725(lite::Twi& twi)
        : twi_(twi)
    {
        device_status_ = init_();
    }

    auto status() const noexcept {
        return device_status_;
    }

    operator bool() const noexcept {
        return device_status_.is_ok();
    }

    //--------------------------------------------------------------------------
    Result read_data() {
        if ( !device_status_ ) {
            return { .read_state = { ReadStatus::ERR_NOT_INIT } };
        }

        u8 status = 0;
        if (!read_reg_u8_(k_reg_status_, status)) {
            return { .read_state = { ReadStatus::ERR_STATUS_READ } };
        }

        if ((status & k_status_avalid_) == 0) {
            return { .read_state = { ReadStatus::NOT_READY } };
        }

        u16 d[4];
        for (int i = 0; i < 4; i++) {
            if (!read_reg_u16_(k_reg_cdatal_ + (i * 2u), d[i])) {
                return { .read_state = { ReadStatus::ERR_DATA_READ } };
            }
        }
        return {
            .read_state = { ReadStatus::OK },
            .y = d[0],
            .r = d[1],
            .g = d[2],
            .b = d[3]
        };
    }

    auto full_scale_counts() const noexcept {
        return k_full_scale_counts_;
    }

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    DeviceStatus device_status_;

    //--------------------------------------------------------------------------
    static constexpr u8 k_i2c_addr_         = 0x29;
    static constexpr u8 k_cmd_bit_          = 0x80;
    static constexpr u8 k_reg_enable_       = 0x00;
    static constexpr u8 k_reg_atime_        = 0x01;
    static constexpr u8 k_reg_control_      = 0x0F;
    static constexpr u8 k_reg_id_           = 0x12;
    static constexpr u8 k_reg_status_       = 0x13;
    static constexpr u8 k_reg_cdatal_       = 0x14;
    static constexpr u8 k_status_avalid_    = 0x01;
    static constexpr u8 k_enable_pon_       = 0x01;
    static constexpr u8 k_enable_aen_       = 0x02;
    static constexpr u8 k_atime_100ms_      = 0xD5;
    static constexpr u8 k_gain_4x_          = 0x01;

    //  saturation model from sensor timing and adc width
    static constexpr u32 k_analog_sat_per_cycle_ = 1024u;
    static constexpr u32 k_digital_sat_     = 65535u;
    //  integration cycles for the configured atime value
    static constexpr u32 k_integration_cycles_ =
        256u - static_cast<u32>(k_atime_100ms_);
    //  analog saturation limit for the configured integration time
    static constexpr u32 k_analog_sat_ =
        k_integration_cycles_ * k_analog_sat_per_cycle_;
    //  usable full scale is the lower of analog and digital saturation
    static constexpr u32 k_full_scale_counts_ =
        k_analog_sat_ < k_digital_sat_
        ? k_analog_sat_
        : k_digital_sat_
    ;

    //--------------------------------------------------------------------------
    DeviceStatus init_() {
        //  check for device presence on the bus
        if ( twi_.probe(k_i2c_addr_).is_error() ) {
            return { DeviceStatus::ERR_PROBE };
        }

        //  check id register for expected value
        u8 id = 0;
        if (!read_reg_u8_(k_reg_id_, id)) {
            LOG_WARN("no id response");
            return { DeviceStatus::ERR_ID_READ };
        }
        if (id != 0x44u && id != 0x4Du) {
            LOG_WARN("unexpected id=0x%02X", id);
            return { DeviceStatus::ERR_ID_VALUE };
        }

        if (
            !write_reg_u8_(k_reg_atime_, k_atime_100ms_)
         || !write_reg_u8_(k_reg_control_, k_gain_4x_)
         || !write_reg_u8_(k_reg_enable_, k_enable_pon_)
        ) {
            LOG_WARN("config failed");
            return { DeviceStatus::ERR_CFG_WRITE };
        }

        delay(3);

        if (!write_reg_u8_(
            k_reg_enable_,
            k_enable_pon_ | k_enable_aen_
        )) {
            LOG_WARN("enable failed");
            return { DeviceStatus::ERR_ENABLE_WRITE };
        }
        return { DeviceStatus::OK };
    }

    //--------------------------------------------------------------------------
    bool write_reg_u8_(u8 reg, u8 value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        return twi_.write(k_i2c_addr_, cmd, value).is_ok();
    }

    bool read_reg_u8_(u8 reg, u8& value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        return twi_.write_read(k_i2c_addr_, cmd, value).is_ok();
    }

    bool read_reg_u16_(u8 reg, u16& value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        lite::lh16 data;
        if ( twi_.write_read( k_i2c_addr_, cmd, data ).is_error() ) {
            return false;
        }

        value = data;
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
