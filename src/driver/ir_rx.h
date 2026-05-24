//  manage ir receiver
//
//  see LICENSE file for terms

#pragma once

#include "app_event.h"

#include "lite/io/log.h"

#define IR_RECEIVE_PIN              IR_RX_GPIO       
#define NO_LED_SEND_FEEDBACK_CODE
#include "TinyIRReceiver.hpp"
#undef IR_RECEIVE_PIN
#undef NO_LED_SEND_FEEDBACK_CODE

#define LOG_TAG         irrx
#define LOG_LEVEL       IR_RX_LOG

//==============================================================================
class IrRx {
//------------------------------------------------------------------------------    
public:
	IrRx() noexcept	{
        initPCIInterruptForTinyReceiver();
    }

    struct Result {
        bool    is_valid;
        u8      addr;
        u8     cmd;
        bool    is_repeat;
    };

    Result read() {
        if (!TinyIRReceiverDecode()) {
            return { .is_valid = false };
        }
        
        return {
            .is_valid   = true,
            .addr       = TinyIRReceiverData.Address,
            .cmd        = TinyIRReceiverData.Command,
            .is_repeat  = bool(TinyIRReceiverData.Flags & IRDATA_FLAGS_IS_REPEAT)
        };
    }
};

#undef LOG_TAG
#undef LOG_LEVEL