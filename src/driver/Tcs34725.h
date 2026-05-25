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
        if (!read_reg_u8_(REG_STATUS, status)) {
            return { .read_state = { ReadStatus::ERR_STATUS_READ } };
        }

        if ((status & STATUS_AVALID) == 0) {
            return { .read_state = { ReadStatus::NOT_READY } };
        }

        u16 d[4];
        for (int i = 0; i < 4; i++) {
            if (!read_reg_u16_(REG_CDATAL + (i * 2u), d[i])) {
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
        return FULL_SCALE_COUNTS;
    }

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    DeviceStatus device_status_;

    //--------------------------------------------------------------------------
    static constexpr u8 I2C_ADDR         = 0x29;
    static constexpr u8 CMD_BIT          = 0x80;
    static constexpr u8 REG_ENABLE       = 0x00;
    static constexpr u8 REG_ATIME        = 0x01;
    static constexpr u8 REG_CONTROL      = 0x0F;
    static constexpr u8 REG_ID           = 0x12;
    static constexpr u8 REG_STATUS       = 0x13;
    static constexpr u8 REG_CDATAL       = 0x14;
    static constexpr u8 STATUS_AVALID    = 0x01;
    static constexpr u8 ENABLE_PON       = 0x01;
    static constexpr u8 ENABLE_AEN       = 0x02;
    static constexpr u8 ATIME_100MS      = 0xD5;
    static constexpr u8 GAIN_4X          = 0x01;

    //  saturation model from sensor timing and adc width
    static constexpr u32 ANALOG_SAT_PER_CYCLE = 1024u;
    static constexpr u32 DIGITAL_SAT     = 65535u;
    //  integration cycles for the configured atime value
    static constexpr u32 INTEGRATION_CYCLES =
        256u - static_cast<u32>(ATIME_100MS);
    //  analog saturation limit for the configured integration time
    static constexpr u32 ANALOG_SAT =
        INTEGRATION_CYCLES * ANALOG_SAT_PER_CYCLE;
    //  usable full scale is the lower of analog and digital saturation
    static constexpr u32 FULL_SCALE_COUNTS =
        ANALOG_SAT < DIGITAL_SAT
        ? ANALOG_SAT
        : DIGITAL_SAT
    ;

    //--------------------------------------------------------------------------
    DeviceStatus init_() {
        //  check for device presence on the bus
        if ( twi_.probe(I2C_ADDR).is_error() ) {
            return { DeviceStatus::ERR_PROBE };
        }

        //  check id register for expected value
        u8 id = 0;
        if (!read_reg_u8_(REG_ID, id)) {
            LOG_WARN("no id response");
            return { DeviceStatus::ERR_ID_READ };
        }
        if (id != 0x44u && id != 0x4Du) {
            LOG_WARN("unexpected id=0x%02X", id);
            return { DeviceStatus::ERR_ID_VALUE };
        }

        if (
                !write_reg_u8_(REG_ATIME, ATIME_100MS)
            || !write_reg_u8_(REG_CONTROL, GAIN_4X)
            || !write_reg_u8_(REG_ENABLE, ENABLE_PON)
        ) {
            LOG_WARN("config failed");
            return { DeviceStatus::ERR_CFG_WRITE };
        }

        delay(3);

        if (!write_reg_u8_(
            REG_ENABLE,
            ENABLE_PON | ENABLE_AEN
        )) {
            LOG_WARN("enable failed");
            return { DeviceStatus::ERR_ENABLE_WRITE };
        }
        return { DeviceStatus::OK };
    }

    //--------------------------------------------------------------------------
    bool write_reg_u8_(u8 reg, u8 value) {
        const u8 cmd = static_cast<u8>(CMD_BIT | reg);
        return twi_.write(I2C_ADDR, cmd, value).is_ok();
    }

    bool read_reg_u8_(u8 reg, u8& value) {
        const u8 cmd = static_cast<u8>(CMD_BIT | reg);
        return twi_.write_read(I2C_ADDR, cmd, value).is_ok();
    }

    bool read_reg_u16_(u8 reg, u16& value) {
        const u8 cmd = static_cast<u8>(CMD_BIT | reg);
        lite::lh16 data;
        if ( twi_.write_read( I2C_ADDR, cmd, data ).is_error() ) {
            return false;
        }

        value = data;
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
