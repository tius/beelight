//  scan twi bus
//
//  see LICENSE file for terms

#pragma once
#include "lite/io/log.h"
#include "lite/sys/twi.h"
#include "lite/cli/cmd.h"
#include "lite/cli/shell.h"

#define LOG_TAG         twi
#define LOG_LEVEL       info

//==============================================================================
class TwiScanCmd {
//------------------------------------------------------------------------------    
public:
    TwiScanCmd(lite::CmdShell &shell, lite::Twi& twi)
        : twi_(twi)
        , cmd_i2c_(shell, "i2c", "scan i2c bus", "", METHOD_THIS(on_cmd_i2c_))
    {}

//------------------------------------------------------------------------------    
private:
    lite::Twi&  twi_;
    lite::Cmd   cmd_i2c_;

    void on_cmd_i2c_(lite::Out& out, lite::Args args) {
        (void)args;

        out.println("scanning i2c bus...");
        for (u8 addr = 1; addr < 127; addr++) {
            if ( twi_.probe(addr).is_ok() ) {
                out.printf("device found at address 0x%02X\n", addr);
            }
        }
    }
};

#undef LOG_TAG
#undef LOG_LEVEL
