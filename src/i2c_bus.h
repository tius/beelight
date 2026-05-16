//  manage i2c bus
//
//  see LICENSE file for terms

#pragma once
#include "lite/io/log.h"
#include "lite/cli/cmd.h"
#include "lite/cli/shell.h"
#include <Wire.h>

#define LOG_TAG         i2c
#define LOG_LEVEL       info

//==============================================================================
class I2cBus {
//------------------------------------------------------------------------------    
public:
    I2cBus(lite::CmdShell &shell) 
        : cmd_i2c_(shell, "i2c", "scan i2c bus", "", METHOD_THIS(on_cmd_i2c_))
    {
        two_wire_.begin(I2C_SDA_GPIO, I2C_SCL_GPIO);
    }

//------------------------------------------------------------------------------    
private:
    TwoWire     two_wire_;
    lite::Cmd   cmd_i2c_;

    void on_cmd_i2c_(lite::Out& out, lite::Args args) {
        out.println("scanning i2c bus...");
        for (u8 addr = 1; addr < 127; addr++) {
            two_wire_.beginTransmission(addr);
            if (two_wire_.endTransmission() == 0) {
                out.printf("device found at address 0x%02X\n", addr);
            }
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL