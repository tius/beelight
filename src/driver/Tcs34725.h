//  manage light sensor tcs34725
//
//  see LICENSE file for terms

#pragma once
#include "app_event.h"

#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/core/bits.h"

#define LOG_TAG         tcs34725
#define LOG_LEVEL       TCS34725_LOG

//==============================================================================
class Tcs34725 {
//------------------------------------------------------------------------------    
public:
    Tcs34725(lite::Twi& twi, u8 addr)
        : twi_(twi), addr_(addr) 
    {
        state_ = init_();
    }

    bool state() const {
        return state_;
    }

    bool read_data(u16* data) {
        if ( !state_ ) {
            if ( !init_() ) {
                return false;
            }
            state_ = true;
        }

        u8 status = 0;
        if (!read_reg_u8_(k_reg_status_, status)) {
            state_ = false;
            return false;
        }

        if ((status & k_status_avalid_) == 0) {
            return false;
        }

        for (int i = 0; i < 4; i++) {
            if (!read_reg_u16_(k_reg_cdatal_ + (i * 2u), data[i])) {
                state_ = false;
                return false;
            }
        }
        return true;
    }

    auto full_scale_counts() const {
        return k_full_scale_counts_;
    }

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    u8          addr_;
    bool        state_;

    //--------------------------------------------------------------------------
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
    bool init_() {
        //  check for device presence on the bus
        if ( twi_.probe(addr_).is_error() ) {
            return false;
        }

        //  check id register for expected value
        u8 id = 0;
        if (!read_reg_u8_(k_reg_id_, id)) {
            LOG_WARN("no id response");
            return false;
        }
        if (id != 0x44u && id != 0x4Du) {
            LOG_WARN("unexpected id=0x%02X", id);
            return false;
        }

        if (
            !write_reg_u8_(k_reg_atime_, k_atime_100ms_)
         || !write_reg_u8_(k_reg_control_, k_gain_4x_)
         || !write_reg_u8_(k_reg_enable_, k_enable_pon_)
        ) {
            LOG_WARN("config failed");
            return false;
        }

        delay(3);

        if (!write_reg_u8_(
            k_reg_enable_,
            k_enable_pon_ | k_enable_aen_
        )) {
            LOG_WARN("enable failed");
            return false;
        }
        return true;
    }

    //--------------------------------------------------------------------------
    bool write_reg_u8_(u8 reg, u8 value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        return twi_.write(addr_, cmd, value).is_ok();
    }

    bool read_reg_u8_(u8 reg, u8& value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        return twi_.write_read(addr_, cmd, value).is_ok();
    }

    bool read_reg_u16_(u8 reg, u16& value) {
        const u8 cmd = static_cast<u8>(k_cmd_bit_ | reg);
        lite::lh16 data;
        if ( twi_.write_read( addr_, cmd, data ).is_error() ) {
            return false;
        }

        value = data;
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
