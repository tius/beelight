//  manage i2c bus
//
//  see LICENSE file for terms

#pragma once
#include "lite/io/log.h"
#include "lite/cli/cmd.h"
#include "lite/cli/shell.h"
#include "lite/core/timer.h"
#include <Wire.h>

#define LOG_TAG         i2c
#define LOG_LEVEL       info

//==============================================================================
class I2cBus {
//------------------------------------------------------------------------------    
public:
    I2cBus(lite::CmdShell &shell)
        : timer_tcs_(MSG_THIS(on_timer_tcs_))
        , cmd_i2c_(shell, "i2c", "scan i2c bus", "", METHOD_THIS(on_cmd_i2c_))
    {
        two_wire_.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
        timer_tcs_.start_periodic(1s);
    }

//------------------------------------------------------------------------------    
private:
    static constexpr u8 k_tcs34725_addr_    = 0x29;
    static constexpr u8 k_tcs_cmd_bit_      = 0x80;
    static constexpr u8 k_tcs_reg_enable_   = 0x00;
    static constexpr u8 k_tcs_reg_atime_    = 0x01;
    static constexpr u8 k_tcs_reg_control_  = 0x0F;
    static constexpr u8 k_tcs_reg_id_       = 0x12;
    static constexpr u8 k_tcs_reg_status_   = 0x13;
    static constexpr u8 k_tcs_reg_cdatal_   = 0x14;
    static constexpr u8 k_tcs_status_avalid_ = 0x01;
    static constexpr u8 k_tcs_enable_pon_   = 0x01;
    static constexpr u8 k_tcs_enable_aen_   = 0x02;
    static constexpr u8 k_tcs_atime_100ms_  = 0xD5;
    static constexpr u8 k_tcs_gain_4x_      = 0x01;

    TwoWire     two_wire_;
    lite::Timer timer_tcs_;
    lite::Cmd   cmd_i2c_;
    bool        tcs_online_ = false;

    void on_cmd_i2c_(lite::Out& out, lite::Args args) {
        (void)args;

        out.println("scanning i2c bus...");
        for (u8 addr = 1; addr < 127; addr++) {
            two_wire_.beginTransmission(addr);
            if (two_wire_.endTransmission() == 0) {
                out.printf("device found at address 0x%02X\n", addr);
            }
        }
    }

    void on_timer_tcs_() {
        if (!tcs_online_ && !init_tcs34725_()) {
            return;
        }

        u8 status = 0;
        if (!read_reg_u8_(k_tcs_reg_status_, status)) {
            set_tcs_online_(false);
            return;
        }

        if ((status & k_tcs_status_avalid_) == 0) {
            return;
        }

        u16 clear = 0;
        u16 red = 0;
        u16 green = 0;
        u16 blue = 0;

        if (!read_reg_u16_(k_tcs_reg_cdatal_ + 0u, clear)
            || !read_reg_u16_(k_tcs_reg_cdatal_ + 2u, red)
            || !read_reg_u16_(k_tcs_reg_cdatal_ + 4u, green)
            || !read_reg_u16_(k_tcs_reg_cdatal_ + 6u, blue)) {
            set_tcs_online_(false);
            return;
        }

        LOG_INFO(
            "tcs34725 c=%u r=%u g=%u b=%u",
            static_cast<unsigned>(clear),
            static_cast<unsigned>(red),
            static_cast<unsigned>(green),
            static_cast<unsigned>(blue)
        );
    }

    bool init_tcs34725_() {
        if (!probe_addr_(k_tcs34725_addr_)) {
            set_tcs_online_(false);
            return false;
        }

        u8 id = 0;
        if (!read_reg_u8_(k_tcs_reg_id_, id)) {
            set_tcs_online_(false);
            return false;
        }

        if (id != 0x44u && id != 0x4Du) {
            LOG_WARN("tcs34725 unexpected id=0x%02X", id);
        }

        if (!write_reg_u8_(k_tcs_reg_atime_, k_tcs_atime_100ms_)
            || !write_reg_u8_(k_tcs_reg_control_, k_tcs_gain_4x_)
            || !write_reg_u8_(k_tcs_reg_enable_, k_tcs_enable_pon_)) {
            set_tcs_online_(false);
            return false;
        }

        delay(3);

        if (!write_reg_u8_(
                k_tcs_reg_enable_,
                static_cast<u8>(k_tcs_enable_pon_ | k_tcs_enable_aen_)
            )) {
            set_tcs_online_(false);
            return false;
        }

        set_tcs_online_(true);
        return true;
    }

    void set_tcs_online_(bool is_online) {
        if (tcs_online_ == is_online) {
            return;
        }

        tcs_online_ = is_online;
        if (tcs_online_) {
            LOG_INFO("tcs34725 online at 0x%02X", k_tcs34725_addr_);
            return;
        }

        LOG_WARN("tcs34725 offline");
    }

    bool probe_addr_(u8 addr) {
        two_wire_.beginTransmission(addr);
        return two_wire_.endTransmission() == 0;
    }

    bool write_reg_u8_(u8 reg, u8 value) {
        two_wire_.beginTransmission(k_tcs34725_addr_);
        two_wire_.write(static_cast<u8>(k_tcs_cmd_bit_ | reg));
        two_wire_.write(value);
        return two_wire_.endTransmission() == 0;
    }

    bool read_reg_u8_(u8 reg, u8& value) {
        two_wire_.beginTransmission(k_tcs34725_addr_);
        two_wire_.write(static_cast<u8>(k_tcs_cmd_bit_ | reg));
        if (two_wire_.endTransmission(false) != 0) {
            return false;
        }

        if (two_wire_.requestFrom(static_cast<int>(k_tcs34725_addr_), 1) != 1u) {
            return false;
        }

        value = static_cast<u8>(two_wire_.read());
        return true;
    }

    bool read_reg_u16_(u8 reg, u16& value) {
        two_wire_.beginTransmission(k_tcs34725_addr_);
        two_wire_.write(static_cast<u8>(k_tcs_cmd_bit_ | reg));
        if (two_wire_.endTransmission(false) != 0) {
            return false;
        }

        if (two_wire_.requestFrom(static_cast<int>(k_tcs34725_addr_), 2) != 2u) {
            return false;
        }

        const u8 lo = static_cast<u8>(two_wire_.read());
        const u8 hi = static_cast<u8>(two_wire_.read());
        value = static_cast<u16>((static_cast<u16>(hi) << 8u) | lo);
        return true;
    }
};

#undef LOG_TAG
#undef LOG_LEVEL