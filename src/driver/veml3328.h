//  manage light sensor veml3328
//
//  see LICENSE file for terms

#pragma once
#include "status.h"

#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"

#define LOG_TAG         veml3328
#define LOG_LEVEL       VEML3328_LOG

//==============================================================================
class Veml3328 {
//------------------------------------------------------------------------------
public:
    struct DeviceStatus : public Status {
        enum : u8 {
            OK = 0,
            ERR_PROBE,
            ERR_CFG_WRITE,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_PROBE:     return "probe failed";
                case ERR_CFG_WRITE: return "config write failed";
                default:            return Status::str();
            }
        }
    };

    struct ReadStatus : public Status {
        enum : u8 {
            OK = 0,
            ERR_NOT_INIT,
            ERR_DATA_READ,
        };

        const char* str() const noexcept {
            switch (code) {
                case ERR_NOT_INIT:  return "not initialized";
                case ERR_DATA_READ: return "data read failed";
                default:            return Status::str();
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
        device_status_ = init_();
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
            !read_reg_u16_(k_reg_r_data_, r)
         || !read_reg_u16_(k_reg_g_data_, g)
         || !read_reg_u16_(k_reg_b_data_, b)
         || !read_reg_u16_(k_reg_c_data_, y)
        ) {
            return { .read_state = { ReadStatus::ERR_DATA_READ } };
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
        return k_full_scale_counts_;
    }

//------------------------------------------------------------------------------
private:
    lite::Twi& twi_;
    DeviceStatus device_status_;

    static constexpr u8 k_i2c_addr_ = 0x10;
    static constexpr u8 k_reg_conf_ = 0x00;
    static constexpr u8 k_reg_r_data_ = 0x05;
    static constexpr u8 k_reg_g_data_ = 0x06;
    static constexpr u8 k_reg_b_data_ = 0x07;
    static constexpr u8 k_reg_c_data_ = 0x04;

    //  keep default power-on configuration, with shutdown bit cleared
    static constexpr u16 k_conf_default_active_ = 0x0000;
    static constexpr u32 k_full_scale_counts_ = 65535u;

    DeviceStatus init_() {
        if (twi_.probe(k_i2c_addr_).is_error()) {
            return { DeviceStatus::ERR_PROBE };
        }

        if (!write_reg_u16_(k_reg_conf_, k_conf_default_active_)) {
            LOG_WARN("config failed");
            return { DeviceStatus::ERR_CFG_WRITE };
        }

        return { DeviceStatus::OK };
    }

    bool write_reg_u16_(u8 reg, u16 value) {
        lite::lh16 val{value};
        const u8 frame[] = {
            reg,
            val.lo,
            val.hi,
        };
        return twi_.write(k_i2c_addr_, frame, sizeof(frame)).is_ok();
    }

    bool read_reg_u16_(u8 reg, u16& value) {
        lite::lh16 data;
        if (twi_.write_read(k_i2c_addr_, reg, data).is_error()) {
            return false;
        }

        value = data;
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
