//  manage light sensor veml3328
//
//  see LICENSE file for terms

#pragma once

#include "lite/core/status.h"
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"

#define LOG_TAG         veml3328
#define LOG_LEVEL       VEML3328_LOG

//==============================================================================
class Veml3328 {
//------------------------------------------------------------------------------
public:
    struct DeviceStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_WRITE_CFG,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:     return "probe failed";
                case ERR_WRITE_CFG: return "config write failed";
                default:            return lite::Status::str();
            }
        }
    };

    struct ReadStatus : public lite::Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_READ_DATA,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:  return "not initialized";
                case ERR_READ_DATA: return "data read failed";
                default:            return lite::Status::str();
            }
        }
    };

    struct Result {
        ReadStatus read_state;
        u16 y, r, g, b;

        operator bool() const noexcept {
            return read_state.is_ok();
        }
    };

    explicit Veml3328(lite::Twi& twi)
        : twi_(twi)
    {
        device_status_ = init();
    }

    auto status() const noexcept {
        return device_status_;
    }

    operator bool() const noexcept {
        return device_status_.is_ok();
    }

    Result read_data() {
        if (!device_status_) {
            return { .read_state = { ReadStatus::ERR_NOT_INIT } };
        }

        u16 r = 0;
        u16 g = 0;
        u16 b = 0;
        u16 y = 0;

        if (
                !read_reg_u16(REG_R_DATA, r)
            || !read_reg_u16(REG_G_DATA, g)
            || !read_reg_u16(REG_B_DATA, b)
            || !read_reg_u16(REG_C_DATA, y)
        ) {
            return { .read_state = { ReadStatus::ERR_READ_DATA } };
        }

        return {
            .read_state = { ReadStatus::OK },
            .y = y,
            .r = r,
            .g = g,
            .b = b,
        };
    }

    auto full_scale_counts() const noexcept {
        return FULL_SCALE_COUNTS;
    }

//------------------------------------------------------------------------------
private:
    lite::Twi& twi_;
    DeviceStatus device_status_;

    static constexpr u8 I2C_ADDR = 0x10;
    static constexpr u8 REG_CONF = 0x00;
    static constexpr u8 REG_R_DATA = 0x05;
    static constexpr u8 REG_G_DATA = 0x06;
    static constexpr u8 REG_B_DATA = 0x07;
    static constexpr u8 REG_C_DATA = 0x04;

    //  keep default power-on configuration, with shutdown bit cleared
    static constexpr u16 CONF_DEFAULT_ACTIVE = 0x0000;
    static constexpr u32 FULL_SCALE_COUNTS = 65535u;

    DeviceStatus init() {
        if (twi_.probe(I2C_ADDR).is_error()) {
            return { DeviceStatus::ERR_PROBE };
        }

        if (!write_reg_u16(REG_CONF, CONF_DEFAULT_ACTIVE)) {
            LOG_WARN("config failed");
            return { DeviceStatus::ERR_WRITE_CFG };
        }

        return { DeviceStatus::OK };
    }

    bool write_reg_u16(u8 reg, u16 value) {
        lite::lh16 val{value};
        const u8 frame[] = {
            reg,
            val.lo,
            val.hi,
        };
        return twi_.write(I2C_ADDR, frame, sizeof(frame)).is_ok();
    }

    bool read_reg_u16(u8 reg, u16& value) {
        lite::lh16 data;
        if (twi_.write_read(I2C_ADDR, reg, data).is_error()) {
            return false;
        }

        value = data;
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
